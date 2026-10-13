"""Start-up services (src/MODULE1.ASM ProbeCpuClass cs:1C0Ch, InitSoundDevices cs:0802h,
InitResourceMemory cs:08F0h, LoadRawFile cs:0F7Fh) vs c/startup.c.

Platform inputs. The C side reads its platform model (pre2_platform_cpu, _sound, _misc, _rtc)
which each case sets from setup. The oracle serves the same facts by its own means:
  * CPU class: the Unicorn CPU is 386+, so class 3 runs as is. Class 0 and class 2 are
    produced at the two instructions that decide it: the shift-count compare (cs:1C34h)
    sees AL = 0 (8086 shift count not masked), or the 386 instruction (cs:1C40h) raises the
    invalid-opcode fault, delivered as the handler's three-word frame at CpuProbeInt06Handler.
  * Sound detection: the probes run as in the original. A device model (Dev) answers their
    port traffic while the probe window is open (probing = 1 from DetectSoundBlaster or
    DetectAdLib until the first state write of InitSoundDevices, set by code hooks). Probe
    traffic is not logged: it is the platform's, and c/startup.c does not repeat it. The
    model: DSP base scan (base+Eh bit 7, base+Ah = AAh), the DMA test block (a write of 14h to
    the DSP completes the block, and the counting ISR of its IRQ runs once: the model
    increments IrqProbeCounts[irq] in DATA), and the OPL timer flags (reg 04h = 63h clears,
    23h gives 40h, 43h gives 20h, when an OPL2 is present). Everything else is 0.
    The DSP write status and the PC speaker port read 0 (ready, nothing pressed).
  * Services of the EMS/XMS checks: INT 67h AH=40h, INT 21h AH=19h and INT 2Fh AX=4300h are
    handlers of the oracle (tests/startup.py), the C side reads the same values from
    pre2_platform_misc.

Cases: the three sound outcomes (Sound Blaster on several IRQs, nothing, AdLib with CPU 3, 2
and 0), /M, the module segment branch with and without OptionBits 40h; the resource-cache
branches (/L, EMS name present but no EMS, XMS drive and driver checks, resident resources
placed by LoadRawFile); LoadRawFile on real assets.
"""
import ctypes, struct
from difftest import Case
from emu import Machine, PortLog, DATA_SEG, CODE_SEG
from unicorn import UC_HOOK_CODE

FREE = 0x2530                 # first paragraph after the loaded image (as tests/sqz.py)
CONTROL_TOP = 0xA000          # ConventionalTopSeg: the heap ends at A000h
NAME_AT = 0xF000              # DATA offset of a file name written by the cases (unused DATA)
HEAP = [(FREE + 0x1000 * k, 0, 0x10000) for k in range(8)]
STATE = {'cpu': 3, 'dev': None}

def cdll():
    """The C side's platform model (c/startup.c, pre2_platform_*), set from setup. The DLL is the
    one the runner loaded (tools/difftest.py may run as __main__ or as module difftest)."""
    import sys
    for name in ('difftest', '__main__'):
        mod = sys.modules.get(name)
        if mod is not None and getattr(mod, '_dll', None) is not None:
            return mod._dll
    raise RuntimeError('difftest has not loaded the C DLL')

# -- oracle-side devices --------------------------------------------------------------------------
class Dev(PortLog):
    """Port model of the oracle: the probe window answers the probes, everything else is the
    default PortLog (logged, IN returns the preset value, 0)."""
    def __init__(self, m, sb=None, adlib=False):
        super().__init__()
        self.m, self.sb, self.adlib = m, sb, adlib     # sb = (base, irq) or None
        self.probing, self.opl_reg, self.opl_v4 = False, 0, 0

    def out(self, port, size, value):
        if self.probing:
            if port == 0x388: self.opl_reg = value
            elif port == 0x389 and self.opl_reg == 4: self.opl_v4 = value
            if self.sb and port == self.sb[0] + 0x0C and value == 0x14:
                self.m.wb(0x0E67 + self.sb[1], 1)     # DMA block done: its counting ISR runs once
            return
        super().out(port, size, value)

    def inp(self, port, size):
        if self.probing:
            if self.sb and port == self.sb[0] + 0x0E: return 0x80        # DSP data ready
            if self.sb and port == self.sb[0] + 0x0A: return 0xAA        # DSP test byte
            if port == 0x388 and self.adlib:
                return {0x63: 0x00, 0x23: 0x40, 0x43: 0x20}.get(self.opl_v4, 0)
            return 0
        return super().inp(port, size)

def _install_hooks(m):
    """Code hooks that open the probe window and select the CPU-class outcome. Installed once
    per machine; they act only on the current STATE."""
    if getattr(m, '_startup_hooks', False): return
    m._startup_hooks = True
    def at(name, fn):
        addr = CODE_SEG * 16 + (name if isinstance(name, int) else m.code(name))
        def hook(uc, address, size, user):
            fn(m)
        m.uc.hook_add(UC_HOOK_CODE, hook, None, addr, addr)
    saved = {}
    def probe(on):
        # The probe polls the VGA status (3DAh, WaitCrtRetrace), which advances the status
        # counter of the VGA model; the C side does not poll, so the window restores the VGA
        # state it started from.
        def f(m):
            d = STATE['dev']
            if d is None: return
            d.probing = on
            if on and 'vga' not in saved:
                saved['vga'] = bytes(m.vga.pre2_vga_state().contents)
            elif not on and 'vga' in saved:
                data = saved.pop('vga')
                ctypes.memmove(ctypes.addressof(m.vga.pre2_vga_state().contents), data, len(data))
        return f
    at('DetectSoundBlaster', probe(True))
    at('loc_1E74', probe(False))              # first state write after the SB probe
    at('DetectAdLib', probe(True))
    at('loc_0839', probe(False))              # PC speaker branch: after the AdLib probe
    at('InitAdlibSampler', probe(False))      # AdLib sampler set-up
    def cpu0(m):                              # cmp al, 1 after shl al, cl: an 8086 gives 0
        if STATE['cpu'] == 0: m.setreg('AX', m.reg('AX') & 0xFF00)
    def cpu2(m):                              # 386 instruction faults: INT 06h frame, handler
        if STATE['cpu'] != 2: return
        sp = (m.reg('SP') - 6) & 0xFFFF
        m.setreg('SP', sp)
        m.write(m.reg('SS'), sp, struct.pack('<HHH', 0x1C40, CODE_SEG, 0x0202))
        m.setreg('IP', 0x1C4B)                # CpuProbeInt06Handler
    at(0x1C34, cpu0)
    at(0x1C40, cpu2)

def set_cpu(cls):
    STATE['cpu'] = cls
    cdll().pre2_platform_cpu(cls)

# -- state ------------------------------------------------------------------------------------------
def dvar(m, name, value, size=1):
    (m.wb if size == 1 else m.ww)(m.data(name), value)

def cvar(m, name, value, size=1):
    (m.wb if size == 1 else m.ww)(m.code(name), value, seg=CODE_SEG)

def base(m, cpu=3, option=0x0000, sound_enabled=0xFF, sb=None, adlib=False):
    """Heap, the top of conventional memory, sound flags, the device model and the C platform."""
    _install_hooks(m)
    cvar(m, 'ConventionalTopSeg', CONTROL_TOP, size=2)
    dvar(m, 'NextFreeSegment', FREE, size=2)
    cvar(m, 'OptionBits', option, size=2)
    cvar(m, 'SoundEnabledFlag', sound_enabled)
    cvar(m, 'CpuClass', cpu)
    cvar(m, 'SbDetected', 0)
    cvar(m, 'AdLibEnabled', 0)
    cvar(m, 'SoundState', 0)
    m.ww(0x63, 0x03D4, seg=0x0040)           # BDA CRTC base (WaitCrtRetrace reads it)
    m.ports = Dev(m, sb=sb, adlib=adlib)
    STATE['dev'] = m.ports
    STATE['cpu'] = cpu
    cdll().pre2_platform_cpu(cpu)
    if sb is None and not adlib:
        cdll().pre2_platform_sound(0, 0, 0, 0)
    else:
        cdll().pre2_platform_sound(1 if sb else 0, sb[0] if sb else 0, sb[1] if sb else 0, 1 if adlib else 0)
    cdll().pre2_platform_misc(0x1010, 0x84, 0, 0)
    m.vga.vga_set_retrace(4) if m.vga else None

def sound_case(title, sb=None, adlib=False, cpu=3, option=0x0000, sound_enabled=0xFF, budget=50_000_000):
    def setup(m):
        base(m, cpu=cpu, option=option, sound_enabled=sound_enabled, sb=sb, adlib=adlib)
    return Case(title, 'InitSoundDevices', 'c_InitSoundDevices', setup=setup,
                ranges=[(0, 0, 0x400), (0x0040, 0, 0x100)] + HEAP, budget=budget, files=True)

CASES = []

# ProbeCpuClass: class 3 by the real CPU; classes 0 and 2 at the two decision points.
def cpu_case(title, cls):
    def setup(m):
        base(m)
        set_cpu(cls)
    CASES.append(Case(title, 'ProbeCpuClass', 'c_ProbeCpuClass', setup=setup, out_regs=('DX',),
                      ranges=[(0, 0, 0x100)]))
cpu_case('CPU class 3 (386 executes)', 3)
cpu_case('CPU class 2 (386 instruction faults)', 2)
cpu_case('CPU class 0 (8086 shift count)', 0)

# InitSoundDevices.
CASES.append(sound_case('SB at 220h, IRQ 7, module segment', sb=(0x220, 7)))
CASES.append(sound_case('SB at 220h, IRQ 2', sb=(0x220, 2)))
CASES.append(sound_case('SB at 210h, IRQ 5', sb=(0x210, 5)))
CASES.append(sound_case('SB at 260h, IRQ 3, OptionBits 40h (no module segment)', sb=(0x260, 3), option=0x0040))
CASES.append(sound_case('SB, AdLib also present (SB wins)', sb=(0x220, 7), adlib=True))
CASES.append(sound_case('nothing found: PC speaker', cpu=3))
CASES.append(sound_case('AdLib only, CPU 3', adlib=True, cpu=3))
CASES.append(sound_case('AdLib only, CPU 2', adlib=True, cpu=2))
CASES.append(sound_case('AdLib only, CPU 0: PC speaker', adlib=True, cpu=0))
CASES.append(sound_case('/M: sound disabled', sb=(0x220, 7), sound_enabled=0))

# InitResourceMemory. Flags: bit 02h resident, 08h pinned, 10h not for XMS.
FLAG_PATTERN = [0x0A, 0x02, 0x08, 0x00, 0x1A, 0x12, 0x0E, 0x02, 0x0A, 0x1B, 0x00, 0x08,
                0x06, 0x0A, 0x18, 0x02, 0x08, 0x0E]

def resource_case(title, option=0x0010, sb=False, flags=FLAG_PATTERN, ems_name=False, ems_ah=0x84,
                  drive=0, xms_al=0):
    def setup(m):
        base(m, option=option, sb=(0x220, 7) if sb else None)
        for i, f in enumerate(flags):
            m.wb(m.data('ResourceFlags') + i, f)
        cvar(m, 'SbDetected', 1 if sb else 0)
        d = m.ints[0x21]
        def dos21(m, d=d):
            if (m.reg('AX') >> 8) == 0x19:               # current drive
                m.setreg('AX', (m.reg('AX') & 0xFF00) | drive)
                return None
            return d(m)
        m.ints[0x21] = dos21
        def int67(m):                                     # EMS status
            m.setreg('AX', (ems_ah << 8) | (m.reg('AX') & 0xFF))
        m.ints[0x67] = int67
        def int2f(m):                                     # XMS installed check
            m.setreg('AX', (m.reg('AX') & 0xFF00) | xms_al)
        m.ints[0x2F] = int2f
        if ems_name:                                      # INT 67h vector -> driver with the name
            seg = 0x7800
            m.write(seg, 0x0A, m.read(DATA_SEG, 0x0B49, 8))     # the name the ASM compares with
            m.ww(0x19E, seg, seg=0x0000)
        else:
            m.ww(0x19E, 0x0000, seg=0x0000)
        cdll().pre2_platform_misc(0x1010, ems_ah, drive, xms_al)
    return Case(title, 'InitResourceMemory', 'c_InitResourceMemory', setup=setup,
                ranges=[(0, 0, 0x400)] + HEAP, files=True)

CASES.append(resource_case('/L: resident bits cleared', option=0x0000))
CASES.append(resource_case('no EMS/XMS, no SB: pinned and resident only', option=0x0010))
CASES.append(resource_case('SB detected, OptionBits 40h', option=0x0050, sb=True))
CASES.append(resource_case('SB detected, OptionBits 10h', option=0x0010, sb=True))
CASES.append(resource_case('EMS name present, EMS not present (INT 67h AH=84h)', option=0x0014, ems_name=True))
CASES.append(resource_case('XMS drive B: and driver absent', option=0x0018, drive=1, xms_al=0))
CASES.append(resource_case('XMS drive C: skipped', option=0x0018, drive=2, xms_al=0x80))
CASES.append(resource_case('OptionBits 20h: resident resources loaded from disk', option=0x0030))
CASES.append(resource_case('OptionBits 20h with SB detected', option=0x0030, sb=True,
                           flags=[0x0A, 0x0A] + [0x00] * 16))   # two pinned resident resources

# LoadRawFile: DX = name offset, returns the segment in AX (on real assets).
def raw_case(title, name, free=FREE):
    def setup(m):
        base(m)
        m.write(DATA_SEG, NAME_AT, name.encode('ascii') + b'\0')
        m.ww(m.data('NextFreeSegment'), free)
        cdll().pre2_platform_misc(0x1010, 0x84, 0, 0)
    return Case(title, 'LoadRawFile', 'c_LoadRawFile', setup=setup, regs=dict(DX=NAME_AT), out_regs=('AX',),
                ranges=[(free + 0x1000 * k, 0, 0x10000) for k in range(4)], files=True)
CASES.append(raw_case('LoadRawFile PRES.TRK', 'PRES.TRK'))
CASES.append(raw_case('LoadRawFile MAP.SQZ (raw read, not decoded)', 'MAP.SQZ'))
CASES.append(raw_case('LoadRawFile GLACE.TRK at another heap top', 'GLACE.TRK', free=0x3000))


# -- ES register model: the same cases entered with ES not equal to DATA --------------------------
# (a routine that must leave ES alone, or set it, is only visible when the entry ES differs from
# what it would load; tools/difftest.py compares pre2_es with the oracle's ES after every case)
def _es_variants(cases, picks, es=0x7C20):
    """picks: (routine, title substring or '', how many) -> clones of the first matching cases."""
    import copy
    out = []
    for routine, sub, limit in picks:
        n = 0
        for c in cases:
            if c.routine == routine and sub in c.title and n < limit:
                d = copy.copy(c)
                d.title = c.title + ' [entry ES 7C20h]'
                d.regs = dict(c.regs, ES=es)
                out.append(d)
                n += 1
    return out
CASES += _es_variants(CASES, [
    ('InitSoundDevices', '', 10), ('InitResourceMemory', '', 9), ('LoadRawFile', '', 2), ('ProbeCpuClass', '', 2)])
