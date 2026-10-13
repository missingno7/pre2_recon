"""Differential tests: original routine (exact oracle under tools/emu.py) vs its C translation.

    python tools/difftest.py [SUITE ...] [--mutants]

A suite is tests/<name>.py defining CASES: a list of Case objects. Each case builds the
same initial state twice (original address space with the program image at LOAD_SEG),
runs the ASM routine in Unicorn and the C entry `c_<name>(Regs*)` in build/c/pre2core.dll,
and compares:
  * memory: the whole DATA segment, the code-segment variables, and every extra range
    the case lists (output buffers); stack scratch below the entry SP is excluded;
  * the registers and flags named by the case's contract;
  * the DOS file-service request log (open/read/seek/close sequence and sizes).
Any difference fails with the first differing addresses.
"""
from common import *
from emu import Machine, DosFiles, LOAD, CODE_SEG, DATA_SEG, STACK_SEG, MEM_SIZE
import ctypes, importlib.util, os, sys, cbuild

class Regs(ctypes.Structure):
    _fields_ = [(n, ctypes.c_uint16) for n in ('ax', 'bx', 'cx', 'dx', 'si', 'di', 'bp', 'ds', 'es', 'flags')]

class CFiles(ctypes.Structure):
    pass

class Case:
    """name: label of the original routine; cfunc: exported C adapter name.
    setup(m): fill memory (m.write/ww/wb, m is the emu.Machine before the call).
    regs: input registers; out_regs: registers compared after return ('CF', 'ZF' allowed).
    ranges: extra (segment, offset, length) memory ranges to compare. files: use DOS files."""
    def __init__(self, title, routine, cfunc, regs=None, setup=None, out_regs=(), ranges=(), files=False,
                 budget=50_000_000, port_inputs=None, tick_model=False, c_setup=None):
        self.title, self.routine, self.cfunc = title, routine, cfunc
        self.regs, self.setup, self.out_regs = dict(regs or {}), setup, tuple(out_regs)
        self.ranges, self.files, self.budget = tuple(ranges), files, budget
        self.port_inputs = dict(port_inputs or {})     # {port: value IN returns}, both sides
        self.tick_model = tick_model   # each failed frame-wait check advances TimerTickCount
        self.c_setup = c_setup         # c_setup(dll) before the C call (e.g. a pre2_test_poll script)

_dll = None
def dll():
    global _dll
    if _dll is None:
        import os                  # a private copy: a loaded DLL is locked on Windows
        for old in (ROOT/'build'/'c').glob('pre2core_*.dll'):
            try: old.unlink()
            except OSError: pass
        _dll = ctypes.CDLL(str(cbuild.build(ROOT/'build'/'c'/f'pre2core_{os.getpid()}.dll')))
        _dll.pre2_memory.restype = ctypes.POINTER(ctypes.c_uint8 * MEM_SIZE)
        _dll.pre2_files_reset.argtypes = [ctypes.c_char_p]
        _dll.pre2_files_log.restype = ctypes.c_char_p
        _dll.pre2_ports_log.restype = ctypes.c_char_p
        _dll.pre2_vga_state_size.restype = ctypes.c_uint32
        _dll.pre2_vga_state.restype = ctypes.POINTER(ctypes.c_uint8 * _dll.pre2_vga_state_size())
        _dll.vga_read.restype = ctypes.c_uint8
        _dll.vga_in.restype = ctypes.c_uint8
    return _dll

WAIT_JB = 0x1C7E      # WaitSoundTicks: jb loc_1C6F (the failed check)
_tick_hooks = {}
def arm_tick_model(m):
    """Oracle half of the test tick model: when the frame wait's jb is about to be taken,
    advance TimerTickCount (the C half is platform_tick_wait in c/platform.c)."""
    from unicorn import UC_HOOK_CODE
    if id(m) in _tick_hooks: _tick_hooks[id(m)][1][0] = True; return
    flag = [True]
    tick = m.code('TimerTickCount')
    def hook(uc, address, size, user):
        if flag[0] and m.reg('FLAGS') & 1:
            m.ww(tick, m.rw(tick, seg=CODE_SEG) + 1, seg=CODE_SEG)
    h = m.uc.hook_add(UC_HOOK_CODE, hook, None, CODE_SEG*16 + WAIT_JB, CODE_SEG*16 + WAIT_JB)
    _tick_hooks[id(m)] = (h, flag)

def disarm_tick_model(m):
    if id(m) in _tick_hooks: _tick_hooks[id(m)][1][0] = False

def compare_ranges(case):
    rs = [(DATA_SEG, 0, 0x10000), (CODE_SEG, 0, 0x9DFC)]
    return rs + list(case.ranges)

def run(case, machine=None):
    m = machine or Machine()
    m.reset_memory()
    dos = DosFiles() if case.files else None
    m.ints = {0x21: dos} if dos else {}
    from emu import PortLog
    m.ports = PortLog(); m.ports.preset = dict(case.port_inputs)
    if m.vga: m.vga.vga_reset()
    if case.setup: case.setup(m)
    if case.tick_model: arm_tick_model(m)
    init = m.snapshot()
    vga_init = bytes(m.vga.pre2_vga_state().contents) if m.vga else None
    regs = dict(case.regs)
    flags = 0x0202
    if os.environ.get('PRE2_POISON'):
        for k, v in POISON.items(): regs.setdefault(k, v)
        flags = 0x0243
    try: out = m.call(case.routine, regs, flags=flags, budget=case.budget)
    finally: disarm_tick_model(m)
    asm_mem = m.snapshot()
    vga_asm = bytes(m.vga.pre2_vga_state().contents) if m.vga else None
    # C side
    d = dll()
    mem = d.pre2_memory().contents
    ctypes.memmove(mem, init, MEM_SIZE)
    if vga_init is not None: ctypes.memmove(d.pre2_vga_state(), vga_init, len(vga_init))
    if case.files: d.pre2_files_reset(str(ROOT/'assets').encode())
    d.pre2_ports_reset()
    d.pre2_platform_tick_model(1 if case.tick_model else 0)
    for port, value in case.port_inputs.items(): d.pre2_ports_preset(port, value)
    r = Regs(**{k.lower(): v for k, v in regs.items() if k in ('AX', 'BX', 'CX', 'DX', 'SI', 'DI', 'BP', 'DS', 'ES')})
    r.ds = regs.get('DS', DATA_SEG); r.es = regs.get('ES', DATA_SEG); r.flags = flags
    c_es = ctypes.c_uint16.in_dll(d, 'pre2_es'); c_es.value = r.es
    keep = case.c_setup(d) if case.c_setup else None
    try: status = getattr(d, case.cfunc)(ctypes.byref(r))
    finally: ctypes.c_void_p.in_dll(d, 'pre2_test_poll').value = None; del keep
    c_mem = bytes(mem)
    if vga_asm is not None:
        vga_c = bytes(d.pre2_vga_state().contents)
        if vga_c[:0x40000] != vga_asm[:0x40000]:
            i = next(k for k in range(0x40000) if vga_c[k] != vga_asm[k])
            problems_vga = [f'VGA plane {i >> 16} offset {i & 0xFFFF:04X} differs (asm {vga_asm[i]:02X} c {vga_c[i]:02X})']
        elif vga_c != vga_asm: problems_vga = ['VGA registers/latches differ']
        else: problems_vga = []
    else: problems_vga = []
    problems = list(problems_vga)
    if status: problems.append(f'C returned status {status}')
    for seg, off, n in compare_ranges(case):
        a = asm_mem[seg*16 + off:seg*16 + off + n]; c = c_mem[seg*16 + off:seg*16 + off + n]
        if a != c:
            diffs = [i for i in range(n) if a[i] != c[i]]
            problems.append(f'memory {seg:04X}:{off + diffs[0]:04X}: {len(diffs)} bytes differ '
                            f'(asm {a[diffs[0]]:02X} c {c[diffs[0]]:02X}; first at ' +
                            ' '.join(f'{off + i:04X}' for i in diffs[:6]) + ')')
    for k in case.out_regs:
        if k in ('CF', 'ZF'):
            bit = {'CF': 1, 'ZF': 0x40}[k]
            a, c = bool(out['FLAGS'] & bit), bool(r.flags & bit)
        else: a, c = out[k], (c_es.value if k == 'ES' else getattr(r, k.lower()))   # ES: the register model
        if a != c: problems.append(f'{k}: asm {a:#x} c {c:#x}' if not isinstance(a, bool) else f'{k}: asm {a} c {c}')
    if out['ES'] != c_es.value:            # the ES register model (pre2_es) after every case
        msg = f'ES after return: asm {out["ES"]:04X} c {c_es.value:04X}'
        if os.environ.get('PRE2_ES_CENSUS'): print(f'ES-CENSUS {case.routine} {case.title}: {msg}')
        else: problems.append(msg)
    plog_a = ';'.join(','.join(str(x) for x in e) for e in m.ports.log)
    plog_c = d.pre2_ports_log().decode()
    if plog_a != plog_c: problems.append(f'port log differs:\n    asm {plog_a[:300]}\n    c   {plog_c[:300]}')
    if dos:
        alog = ';'.join(','.join(str(x) for x in e) for e in dos.log)
        clog = d.pre2_files_log().decode()
        if alog != clog: problems.append(f'DOS file log differs:\n    asm {alog[:300]}\n    c   {clog[:300]}')
    return problems

# PRE2_POISON=1: every register a case does not specify (and CF, ZF) starts as garbage on both
# sides, so a routine that reads a register its C contract does not carry shows as a difference.
POISON = {'AX': 0xA5C3, 'BX': 0x5A3C, 'CX': 0x3C5A, 'DX': 0xC3A5, 'SI': 0x6996, 'DI': 0x9669, 'BP': 0x7E81,
          'ES': 0x7C20}

def load_suite(name):
    path = ROOT/'tests'/f'{name}.py'
    spec = importlib.util.spec_from_file_location(name, path)
    mod = importlib.util.module_from_spec(spec); sys.path.insert(0, str(ROOT/'tools')); spec.loader.exec_module(mod)
    return mod

def main():
    names = [a for a in sys.argv[1:] if not a.startswith('-')] or sorted(p.stem for p in (ROOT/'tests').glob('*.py') if not p.stem.startswith('_'))
    total = failed = 0
    m = Machine(vga=dll())
    for name in names:
        suite = load_suite(name)
        for case in suite.CASES:
            total += 1
            try: problems = run(case, m)
            except Exception as e: problems = [f'error: {e}']
            if problems:
                failed += 1
                print(f'FAIL {name}: {case.title}'); [print('   ', p) for p in problems]
        print(f'{"PASS" if not failed else "...."} {name}: {len(suite.CASES)} cases')
    print(f'{total - failed}/{total} cases equal')
    if m.blocks is not None:                      # PRE2_COVERAGE: for tools/coverage.py
        import json
        (ROOT/'build'/'coverage.json').write_text(json.dumps({'suites': names, 'blocks': sorted(m.blocks)}))
    if failed: raise SystemExit(1)

if __name__ == '__main__':
    main()
