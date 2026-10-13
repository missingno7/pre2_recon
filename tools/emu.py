"""Bounded real-mode execution of original routines (differential tests, narrow probes).

Loads a linked EXE (normally the exact oracle build/asm/PRE2.EXE) at load segment 1010h
(PSP at 1000h, so the code segment runs at 1030h and DATA at 1A0Fh, the frames the
pre2_port notes use), applies its relocations and reads public symbols from the TLINK
map. A routine is entered with registers and memory chosen by the test and a return
address of cs:0000, where execution stops (cs:0000 holds option data, never code).

There is no DOS and no hardware: port I/O goes to a `ports` object (default: record
writes, IN returns 0) and any software interrupt stops the call unless an `ints`
handler services it. Memory outside the program image is zero unless the test fills
it. Execution is bounded by an instruction budget. This is an investigation and test
harness for single routines or small regions, never for booting the game.
"""
from common import *
from image import mz
from unicorn import Uc, UcError, UC_ARCH_X86, UC_MODE_16, UC_HOOK_INTR, UC_HOOK_INSN, UC_HOOK_CODE, UC_HOOK_BLOCK
from unicorn.x86_const import *
import os, re, struct

LOAD = 0x1010
CODE_SEG = LOAD + 0x20
DATA_SEG = LOAD + 0x9FF
STACK_SEG = LOAD
MEM_SIZE = 0x110000
VGA_LIN = 0xA0000
REG = {r: globals()['UC_X86_REG_' + r] for r in
       ('AX', 'BX', 'CX', 'DX', 'SI', 'DI', 'BP', 'SP', 'CS', 'DS', 'ES', 'SS', 'IP', 'FLAGS')}
FLAG = dict(CF=0x001, PF=0x004, AF=0x010, ZF=0x040, SF=0x080, TF=0x100, IF=0x200, DF=0x400, OF=0x800)

def read_map(path):
    """Public name (upper case) -> code/data offset, from the TLINK map (Publics by Name)."""
    text = Path(path).read_text(errors='replace')
    if 'Publics by Name' not in text: return {}
    text = text[text.index('Publics by Name'):]
    text = text[:text.index('Publics by Value')] if 'Publics by Value' in text else text
    return {m[3]: (int(m[1], 16), int(m[2], 16)) for m in re.finditer(r'^\s*([0-9A-F]{4}):([0-9A-F]{4})\s+(\w+)', text, re.M)}

class PortLog:
    """Default device model for non-VGA ports: record every OUT and IN; IN returns the
    value preset for that port (0 by default), as c/ports.c does."""
    def __init__(self, value=0): self.log, self.value, self.preset = [], value, {}
    def out(self, port, size, value): self.log.append(('out', port, size, value))
    def inp(self, port, size):
        self.log.append(('in', port, size)); return self.preset.get(port, self.value)

class Stop(Exception): pass

class Machine:
    def __init__(self, exe=None, ports=None, vga=None):
        exe = Path(exe or ROOT/'build'/'asm'/'PRE2.EXE')
        _, image, relocations, _ = mz(exe.read_bytes())
        image = bytearray(image)
        for s, o in relocations:
            p = s*16 + o
            struct.pack_into('<H', image, p, (struct.unpack_from('<H', image, p)[0] + LOAD) & 0xFFFF)
        self.image = bytes(image)
        self.symbols = read_map(exe.with_suffix('.MAP'))
        self.ports = ports or PortLog()
        self.ints = {}          # interrupt number -> handler(machine) ; return False to stop
        self.uc = Uc(UC_ARCH_X86, UC_MODE_16)
        # vga: a loaded build/c/pre2core.dll (ctypes); A000:0000..FFFF then goes to its
        # VGA model and ports 3C0..3DA to its registers (see c/vga.h)
        self.vga = vga
        if vga:
            self.uc.mem_map(0, VGA_LIN)
            self.uc.mem_map(VGA_LIN + 0x10000, MEM_SIZE - VGA_LIN - 0x10000)
            self.uc.mmio_map(VGA_LIN, 0x10000, self._vga_read, None, self._vga_write, None)
        else:
            self.uc.mem_map(0, MEM_SIZE)
        self.reset_memory()
        self.uc.hook_add(UC_HOOK_INSN, self._in, None, 1, 0, UC_X86_INS_IN)
        self.uc.hook_add(UC_HOOK_INSN, self._out, None, 1, 0, UC_X86_INS_OUT)
        self.uc.hook_add(UC_HOOK_INTR, self._intr)
        self.stop_at = CODE_SEG*16
        self.uc.hook_add(UC_HOOK_CODE, self._sentinel, None, self.stop_at, self.stop_at)
        self.error = None
        # PRE2_COVERAGE: record the executed basic blocks of the code segment (code offset,
        # size) for tools/coverage.py; off by default (it slows execution)
        self.blocks = None
        if os.environ.get('PRE2_COVERAGE'):
            self.blocks = set()
            base = CODE_SEG*16
            self.uc.hook_add(UC_HOOK_BLOCK, lambda uc, a, n, u: self.blocks.add((a - base, n)), None,
                             base, base + 0xFFFF)

    def reset_memory(self):
        self.restore(b'\0'*MEM_SIZE)
        self.uc.mem_write(LOAD*16, self.image)

    def snapshot(self):
        """The address space as bytes (the VGA window reads as zero when modelled)."""
        if not self.vga: return bytes(self.uc.mem_read(0, MEM_SIZE))
        return bytes(self.uc.mem_read(0, VGA_LIN)) + b'\0'*0x10000 + \
            bytes(self.uc.mem_read(VGA_LIN + 0x10000, MEM_SIZE - VGA_LIN - 0x10000))
    def restore(self, data):
        if not self.vga: self.uc.mem_write(0, bytes(data)); return
        self.uc.mem_write(0, bytes(data[:VGA_LIN]))
        self.uc.mem_write(VGA_LIN + 0x10000, bytes(data[VGA_LIN + 0x10000:]))

    def _vga_read(self, uc, offset, size, user):
        v = 0
        for i in range(size): v |= self.vga.vga_read(offset + i) << (8*i)
        return v
    def _vga_write(self, uc, offset, size, value, user):
        for i in range(size): self.vga.vga_write(offset + i, (value >> (8*i)) & 0xFF)

    # -- addressing helpers -------------------------------------------------
    def lin(self, seg, off): return (seg*16 + off) & 0xFFFFF
    def read(self, seg, off, n): return bytes(self.uc.mem_read(self.lin(seg, off), n))
    def write(self, seg, off, data): self.uc.mem_write(self.lin(seg, off), bytes(data))
    def rb(self, off, seg=DATA_SEG): return self.read(seg, off, 1)[0]
    def rw(self, off, seg=DATA_SEG): return struct.unpack('<H', self.read(seg, off, 2))[0]
    def wb(self, off, v, seg=DATA_SEG): self.write(seg, off, bytes([v & 0xFF]))
    def ww(self, off, v, seg=DATA_SEG): self.write(seg, off, struct.pack('<H', v & 0xFFFF))
    def reg(self, name): return self.uc.reg_read(REG[name])
    def setreg(self, name, v): self.uc.reg_write(REG[name], v & 0xFFFF)
    def label(self, name):
        """Linear image offset of any label of the last build (tools/where.py)."""
        if not hasattr(self, '_labels'):
            import where
            self._labels = where.labels()
        return self._labels[name.upper()]
    def code(self, name_or_off):
        if isinstance(name_or_off, int): return name_or_off
        return self.label(name_or_off) - 0x200
    def data(self, name):
        """ds offset of a DATA label."""
        return self.label(name) - 0x9FF0

    # -- hooks ----------------------------------------------------------------
    def _in(self, uc, port, size, user):
        if self.vga and 0x3C0 <= port <= 0x3DA:
            return self.vga.vga_in(port) | (self.vga.vga_in(port + 1) << 8 if size == 2 else 0)
        return self.ports.inp(port, size) & ((1 << (8*size)) - 1)
    def _out(self, uc, port, size, value, user):
        if self.vga and 0x3C0 <= port <= 0x3DA:
            self.vga.vga_out(port, value & 0xFF)
            if size == 2: self.vga.vga_out(port + 1, value >> 8)
            return
        self.ports.out(port, size, value)
    def _intr(self, uc, n, user):
        if n == 0x03 or n not in self.ints or self.ints[n](self) is False:
            self.error = f'INT {n:02X}h at {self.reg("CS"):04X}:{self.reg("IP"):04X}'
            uc.emu_stop()
    def _sentinel(self, uc, address, size, user):
        uc.emu_stop()

    # -- calls ------------------------------------------------------------------
    def call(self, routine, regs=None, flags=0x0202, budget=5_000_000, far=False):
        """Run the near routine `routine` (label or code offset) until it returns.
        Returns the register dict after return. Raises RuntimeError on a stop elsewhere."""
        regs = dict(regs or {})
        sp = regs.pop('SP', 0x0200)
        ss = regs.pop('SS', STACK_SEG)
        for k, v in dict(DS=DATA_SEG, ES=DATA_SEG).items(): regs.setdefault(k, v)
        for k, v in regs.items(): self.setreg(k, v)
        self.setreg('SS', ss)
        frame = struct.pack('<HH', 0, CODE_SEG) if far else struct.pack('<H', 0)
        sp -= len(frame)
        self.write(ss, sp, frame)
        self.setreg('SP', sp)
        self.setreg('FLAGS', flags)
        self.setreg('CS', CODE_SEG)
        start = self.code(routine)
        self.error = None
        try:
            self.uc.emu_start(CODE_SEG*16 + start, 0, count=budget)
        except UcError as e:
            self.error = f'{e} at {self.reg("CS"):04X}:{self.reg("IP"):04X}'
        here = self.reg('CS')*16 + self.reg('IP')
        if self.error is None and here != self.stop_at:
            self.error = f'budget exhausted at {self.reg("CS"):04X}:{self.reg("IP"):04X}'
        if self.error: raise RuntimeError(self.error)
        return {k: self.reg(k) for k in REG}

if __name__ == '__main__':
    m = Machine()
    print('loaded', len(m.image), 'bytes;', len(m.symbols), 'public symbols')

class DosFiles:
    """INT 21h file services over a directory (default assets/), logging every request.

    Implements 3Dh open (read only), 3Eh close, 3Fh read, 42h seek. Errors set CF and
    AX like DOS (2 = file not found, 6 = invalid handle). Any other function stops."""
    def __init__(self, root=ROOT/'assets'):
        self.root, self.files, self.log, self.next = Path(root), {}, [], 5
    def __call__(self, m):
        ah = m.reg('AX') >> 8
        def ok(ax=None):
            if ax is not None: m.setreg('AX', ax)
            m.setreg('FLAGS', m.reg('FLAGS') & ~1)
        def fail(code):
            m.setreg('AX', code); m.setreg('FLAGS', m.reg('FLAGS') | 1)
        if ah == 0x3D:
            raw = m.read(m.reg('DS'), m.reg('DX'), 128).split(b'\0')[0].decode('latin-1')
            path = self.root/raw.upper()
            self.log.append(('open', raw))
            if not path.exists(): return fail(2)
            h = 5                                   # DOS: the lowest free handle
            while h in self.files: h += 1
            self.files[h] = [path.read_bytes(), 0]
            return ok(h)
        h = m.reg('BX')
        if ah in (0x3E, 0x3F, 0x42) and h not in self.files: return fail(6)
        if ah == 0x3E:
            self.log.append(('close', h)); del self.files[h]; return ok()
        if ah == 0x3F:
            data, pos = self.files[h]
            n = min(m.reg('CX'), len(data) - pos)
            m.write(m.reg('DS'), m.reg('DX'), data[pos:pos + n])
            self.files[h][1] = pos + n
            self.log.append(('read', h, m.reg('CX'), n))
            return ok(n)
        if ah == 0x42:
            data, pos = self.files[h]
            off = (m.reg('CX') << 16) | m.reg('DX')
            base = {0: 0, 1: pos, 2: len(data)}[m.reg('AX') & 0xFF]
            new = (base + off) & 0xFFFFFFFF
            self.files[h][1] = new
            self.log.append(('seek', h, m.reg('AX') & 0xFF, off))
            m.setreg('DX', new >> 16); return ok(new & 0xFFFF)
        return False
