"""Credits and intro screens (src/MODULE2.ASM cs:23D4h..26A7h) vs c/credits.c.

ReadClockYear reads INT 1Ah AH=04h; the oracle serves it with a handler that returns the case's
CX/DX (BCD century:year, month:day) and CF, the same values the C platform model reports
(pre2_platform_rtc). The year gates and the digit decoding are checked on boundary years, on
the years whose decoding loses an 8-bit carry (2048, 2049), on the clock error path and on
random BCD values (fixed seed).

Screens: frame waits are bounded by state. Each button wait (WaitForButtonPressRelease) gets
two playback entries of the input log (DATA:003Fh, byte_2879 = 1): a button-down poll and a
button-up poll, so both loops of the wait end after one poll each. ShowCreditsTimed's loop
ends at its first poll (playback entry with the button bit set). The C side calls
platform_poll() in that loop, which the oracle never reaches.

The intro picture (ShowIntroBody) decodes LEVELI.SQZ and LEVELH.SQZ through the oracle's own
LoadSqzFile over the shipped assets (DOS file requests logged on both sides) and copies the
planes to VRAM; the VGA model compares the result plane by plane. INT 10h is served by bios10
(mode set, attribute palette, DAC block), with the same register effects as c/platform.c.
"""
import ctypes, random
from difftest import Case
from emu import DATA_SEG, CODE_SEG

FREE = 0x2530                 # first paragraph after the loaded image (as tests/sqz.py)
CONTROL_TOP = 0xA000          # ConventionalTopSeg: the heap ends at A000h
KEYDOWN = 0x27F4              # KeyDownTable (128 bytes)
DEFAULT_CLOCK_YEAR = 0x07BC   # 1980: the image value of ClockYear
VGA_PLANE = 0x10000


# -- VGA and BIOS services ---------------------------------------------------------------------
def vga_base(m):
    return ctypes.addressof(m.vga.pre2_vga_state().contents)

def bios10(m):
    """INT 10h: mode set (BDA 0040:0049h), attribute palette, DAC block (as c/platform.c)."""
    ax = m.reg('AX')
    ah, al = ax >> 8, ax & 0xFF
    if ah == 0x00:
        m.wb(0x49, al, seg=0x40)
        return None
    if ah == 0x10 and al == 0x02:
        table = m.read(m.reg('ES'), m.reg('DX'), 17)
        m.vga.vga_in(0x3DA)
        for i in range(16):
            m.vga.vga_out(0x3C0, i | 0x20)
            m.vga.vga_out(0x3C0, table[i])
        m.vga.vga_out(0x3C0, 0x11 | 0x20)
        m.vga.vga_out(0x3C0, table[16])
        return None
    if ah == 0x10 and al == 0x12:
        first, count = m.reg('BX'), m.reg('CX')
        data = m.read(m.reg('ES'), m.reg('DX'), count * 3)
        m.vga.vga_out(0x3C8, first & 0xFF)
        for b in data:
            m.vga.vga_out(0x3C9, b)
        return None
    return False

def cdll():
    """The C side's platform model (c/startup.c, pre2_platform_*), set from setup. The DLL is the
    one the runner loaded (tools/difftest.py may run as __main__ or as module difftest)."""
    import sys
    for name in ('difftest', '__main__'):
        mod = sys.modules.get(name)
        if mod is not None and getattr(mod, '_dll', None) is not None:
            return mod._dll
    raise RuntimeError('difftest has not loaded the C DLL')

def rtc(ax, cx, dx, carry):
    """INT 1Ah handler: the registers and carry of the clock read."""
    def h(m):
        m.setreg('AX', ax); m.setreg('CX', cx); m.setreg('DX', dx)
        fl = m.reg('FLAGS')
        m.setreg('FLAGS', (fl | 1) if carry else (fl & ~1))
    return h

# -- state -------------------------------------------------------------------------------------
def dvar(m, name, value, size=1):
    (m.wb if size == 1 else m.ww)(m.data(name), value)

def cvar(m, name, value, size=1):
    (m.wb if size == 1 else m.ww)(m.code(name), value, seg=CODE_SEG)

def base(m, clock=DEFAULT_CLOCK_YEAR, keys=None, gates=(0, 0, 0, 0), rtc_cfg=(0x0400, 0, 0, 0)):
    """Heap and ClockYear, the keyboard tables, the gate bytes (2805h, 2806h, 2811h, 282Ch),
    the BIOS and RTC services and the VGA retrace model."""
    cvar(m, 'ConventionalTopSeg', CONTROL_TOP, size=2)
    dvar(m, 'NextFreeSegment', FREE, size=2)
    dvar(m, 'word_0039', FREE, size=2)
    dvar(m, 'ClockYear', clock, size=2)
    m.write(DATA_SEG, KEYDOWN, bytes(128))
    for k, v in (keys or {}).items():
        m.wb(KEYDOWN + k, v, seg=DATA_SEG)
    dvar(m, 'byte_2805', gates[0])
    dvar(m, 'byte_2806', gates[1])
    dvar(m, 'byte_2811', gates[2])
    dvar(m, 'byte_282C', gates[3])
    m.ints[0x10] = bios10
    m.ints[0x1A] = rtc(*rtc_cfg)
    cdll().pre2_platform_rtc(rtc_cfg[0], rtc_cfg[1], rtc_cfg[2], int(rtc_cfg[3]))
    if m.vga: m.vga.vga_set_retrace(4)

def playback(m, words):
    """Input log for PollInputState (byte_2879 = 1): one word per poll at DATA:003Fh."""
    dvar(m, 'byte_2879', 1)
    dvar(m, 'word_287A', 0, size=2)
    dvar(m, 'byte_287C', 0)
    dvar(m, 'byte_287D', 0)
    for k, w in enumerate(words):
        m.ww(0x3F + 2 * k, w)

BUTTON_DOWN, BUTTON_UP = 0x0010, 0x0000     # bit 4 of the playback value: button 27E8h

def set_up(m, words=(BUTTON_DOWN, BUTTON_UP)):
    playback(m, words)

# -- cases ---------------------------------------------------------------------------------------
CASES = []

def read_clock(title, cx, dx=0, ax_err=0x0400, carry=False, clock=DEFAULT_CLOCK_YEAR):
    def setup(m):
        base(m, clock=clock, rtc_cfg=(ax_err, cx, dx, carry))
    CASES.append(Case(title, 'ReadClockYear', 'c_ReadClockYear', setup=setup, out_regs=('AX', 'CF')))

# Normal decoding: 2026, 1980, the boundary 2066 (stored) and 2067 (not stored, 813h).
read_clock('2026 stored', 0x2026, 0x0512)
read_clock('1980 stored', 0x1980, 0x0101)
read_clock('2066 stored (below 2067)', 0x2066, 0x1231)
read_clock('2067 not stored (813h)', 0x2067, 0x1231)
read_clock('9999 not stored', 0x9999, 0x1231)
read_clock('0000 stored as zero', 0x0000, 0x0000)
# Quirk: AL carry lost, 2048 -> 1792 and 2049 -> 1793 (both stored).
read_clock('2048 quirk 1792', 0x2048, 0x0101)
read_clock('2049 quirk 1793', 0x2049, 0x0101)
read_clock('1994 with clock preset', 0x1994, 0x0101, clock=0x07CA)
# Clock error: CF = 1, AX is the INT status, nothing stored.
read_clock('clock error', 0x2026, 0x0101, ax_err=0x8600, carry=True)

_rng = random.Random(0x1A1A)
for i in range(8):
    y = _rng.randrange(0, 100)
    cent = _rng.choice([0x19, 0x20, 0x21])
    read_clock(f'random BCD {cent:02X}{y:02X}', (cent << 8) | (((y // 10) << 4) | (y % 10)),
               _rng.randrange(0x10000))

# ShowLateYearCredits: shown from 1996 (7CCh) on, with one button wait.
def late(title, clock):
    def setup(m):
        base(m, clock=clock)
        set_up(m)
    CASES.append(Case(title, 'ShowLateYearCredits', 'c_ShowLateYearCredits', setup=setup,
                      ranges=[(0x0040, 0, 0x100)], files=False))
late('late credits at 1996', 0x07CC)
late('late credits at 2010', 0x07DA)
late('no late credits at 1995', 0x07CB)

# TryShowCredits: W (11h), Ctrl (1Dh) and Alt (38h) are excluded from the key test.
def credits(title, gates=(1, 0, 1, 1), keys=None, words=(BUTTON_DOWN, BUTTON_UP), level=0):
    def setup(m):
        base(m, gates=gates, keys=keys)
        set_up(m, words)
        dvar(m, 'CurrentLevel', level)
    CASES.append(Case(title, 'TryShowCredits', 'c_TryShowCredits', setup=setup,
                      out_regs=('CF',), ranges=[(0x0040, 0, 0x100)]))
credits('credits gate passes')
credits('credits gate fails: 2805h clear', gates=(0, 0, 1, 1))
credits('credits gate fails: 2811h clear', gates=(1, 0, 0, 1))
credits('credits gate fails: key down 20h', keys={0x20: 0xFF})
credits('credits gate: W held is excluded', keys={0x11: 0xFF})
credits('credits gate: Ctrl held is excluded', keys={0x1D: 0xFF})
credits('credits gate: key 7Eh down fails', keys={0x7E: 0x01})
credits('credits gate: scancode 7Fh not tested', keys={0x7F: 0x01})
credits('credits gate passes at level 3', level=3)

# ShowCreditsScreen: the credit lines only.
def screen(title):
    def setup(m):
        base(m)
    CASES.append(Case(title, 'ShowCreditsScreen', 'c_ShowCreditsScreen', setup=setup,
                      ranges=[(0x0040, 0, 0x100)]))
screen('credits screen lines')

# ShowCreditsTimed (unused): the playback button bit ends its wait at the first poll.
def timed(title, words):
    def setup(m):
        base(m)
        set_up(m, words)
        dvar(m, 'word_27F0', 0x0100, size=2)
    CASES.append(Case(title, 'ShowCreditsTimed', 'c_ShowCreditsTimed', setup=setup,
                      ranges=[(0x0040, 0, 0x100)]))
timed('timed credits: button ends it', (BUTTON_DOWN,))

# TryShowIntroKeyed: E (12h), Ctrl and Alt are excluded; any other key blocks the intro.
def intro_keyed(title, gates=(0, 1, 1, 1), keys=None):
    def setup(m):
        base(m, gates=gates, keys=keys)
        set_up(m, (BUTTON_DOWN, BUTTON_UP))
    CASES.append(Case(title, 'TryShowIntroKeyed', 'c_TryShowIntroKeyed', setup=setup,
                      out_regs=('CF',), ranges=[(0x0040, 0, 0x100)], files=True))
intro_keyed('keyed intro: gate fails (2806h clear)', gates=(0, 0, 1, 1))
intro_keyed('keyed intro: key 20h blocks', keys={0x20: 0xFF})
intro_keyed('keyed intro: E held is excluded and the intro runs', keys={0x12: 0xFF})
intro_keyed('keyed intro: intro runs', gates=(0, 1, 1, 1))

# ShowIntroPicture: 1994 (7CAh) and later; no key test.
def intro(title, clock):
    def setup(m):
        base(m, clock=clock)
        set_up(m, (BUTTON_DOWN, BUTTON_UP))
    CASES.append(Case(title, 'ShowIntroPicture', 'c_ShowIntroPicture', setup=setup,
                      out_regs=('CF',), ranges=[(0x0040, 0, 0x100)], files=True))
intro('intro picture at 1994', 0x07CA)
intro('intro picture at 2026', 0x07EA)
intro('no intro picture at 1993', 0x07C9)
