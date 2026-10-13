"""Front-end and fixed screens (src/MODULE3.ASM cs:8E45..9286, src/MODULE4.ASM cs:9520..9DFB)
vs c/screens.c.

Frame loops are never run to their natural end in the oracle (their exits wait on the timer
or on keys delivered by the keyboard ISR, and no interrupt runs there). Each case bounds its
loop by state: a preset exit flag (ScreenExitRequested, byte_27E8 through the keyboard flags
or the recorded input log byte_2879 = 1), a preset timer (TimerTickCount 3 or more ahead of
word_27EE, so WaitSoundTicks ends after one call), or a stop hook: an instruction address
(arm_stop) at which the oracle returns, which the C adapter matches by returning at the same
point. Stops are used at a loop's back edge (one walk-in frame of the intro, whose repeat
condition is the player record's drawn flag), at the instruction after a call (the level start
before the intro, the ENTER restart), and before a pop ds whose push the entry does not make
(fade-in passes).

Pictures are the shipped resources: MAP (slot 5), MENU (8), MENU2 (12), GAMEOVER (11), MOTIF
(6), PRESENT.SQZ and TITUS.SQZ, through the original loaders (LoadSqzFile and PageInResource).
Pictures loaded outside the routine under test come from the oracle's own LoadSqzFile, cached
as a memory snapshot. The shifted font tables of RunCallbackMapScreen read a fixed random
block (not a shipped file), and the GAME OVER backdrop is what PageInResource(11) leaves.

Input: keyboard flag bytes (byte_282D, byte_2810), LastScanCode, the input log (byte_2879 = 1
plays it back, one (value, count) word per entry at DATA:003Fh). Joystick: byte_27E4 = FFh
(no joystick), so PollInputState samples the flags only.
"""
import ctypes, random
from difftest import Case
import sys as _sys
from common import ROOT as _ROOT
_sys.path.insert(0, str(_ROOT/'tests'))
import _bios as bios
from common import ROOT
from emu import Machine, DATA_SEG, CODE_SEG, PortLog, DosFiles
from unicorn import UC_HOOK_CODE

FREE = 0x2530                   # first paragraph after the loaded image (as tests/render.py)
CONTROL_TOP = 0xA000            # ConventionalTopSeg for the heap
MODSEG = 0x7000                 # ModuleSegment (resource copies)
HEAP = (FREE, 0, (CONTROL_TOP - FREE) * 16)
VGA_PLANE = 0x10000
FONT_SEG = 0x5000               # word_003D: source of the shifted font tables (fixed random block)
FONT_BYTES = 0x2800
MARK_SEG = 0x9F00               # stage marker sprite (word_667A)
SCAN_TABLE = 0xB068
CODE_BOX = 0xB170
VGA_SEG = 0xA000
CHARS = '0123456789ABCDEF'

def vga_base(m):
    return ctypes.addressof(m.vga.pre2_vga_state().contents)

def vga_bytes(m):
    return bytes(m.vga.pre2_vga_state().contents)

def vga_restore(m, data):
    ctypes.memmove(vga_base(m), data, len(data))

def vga_plane(m, plane, off, data):
    data = bytes(data)
    ctypes.memmove(vga_base(m) + plane * VGA_PLANE + off, data, len(data))

def bios10(m):
    """INT 10h services of the video routines (as tests/render.py; c/platform.c does the same)."""
    ax = m.reg('AX')
    ah, al = ax >> 8, ax & 0xFF
    if ah == 0x00:                                   # set video mode (BIOS data area)
        bios.set_mode(m, al)
        return None
    if ah == 0x10 and al == 0x02:                    # attribute palette + overscan (ES:DX, 17 bytes)
        table = m.read(m.reg('ES'), m.reg('DX'), 17)
        m.vga.vga_in(0x3DA)
        for i in range(16):
            m.vga.vga_out(0x3C0, i | 0x20)
            m.vga.vga_out(0x3C0, table[i])
        m.vga.vga_out(0x3C0, 0x11 | 0x20)
        m.vga.vga_out(0x3C0, table[16])
        return None
    if ah == 0x10 and al == 0x12:                    # DAC block: BX first, CX count, ES:DX data
        first, count = m.reg('BX'), m.reg('CX')
        data = m.read(m.reg('ES'), m.reg('DX'), count * 3)
        m.vga.vga_out(0x3C8, first & 0xFF)
        for b in data:
            m.vga.vga_out(0x3C9, b)
        return None
    return False                                     # not modelled: the oracle stops

# -- stop hooks: the oracle returns where the C adapter returns ---------------------------------
_HOOKED = set()
_ARMED = set()

def arm_stop(m, where):
    """Stop the oracle when it reaches the instruction at label WHERE (or code offset) of this
    case: the same point where the C adapter returns."""
    addr = CODE_SEG * 16 + m.code(where)
    if (id(m), addr) not in _HOOKED:
        def hook(uc, address, size, user, addr=addr):
            if addr in _ARMED:
                m.setreg('CS', CODE_SEG)
                m.setreg('IP', 0)
                uc.emu_stop()
        m.uc.hook_add(UC_HOOK_CODE, hook, None, addr, addr)
        _HOOKED.add((id(m), addr))
    _ARMED.add(addr)

# -- base state ----------------------------------------------------------------------------------
def cvar(m, name, value, size=1):
    (m.wb if size == 1 else m.ww)(m.code(name), value, seg=CODE_SEG)

def dvar(m, name, value, size=1):
    (m.wb if size == 1 else m.ww)(m.data(name), value)

def base(m, cpu=5, option=1, vbyte=5, count=None):
    """Heap top, module segment, BIOS handler, retrace model, no Sound Blaster, no joystick,
    resource flags as InitResourceMemory leaves them without /L (bit 02h clear)."""
    _ARMED.clear()
    _POLL['on'] = False
    m.ww(m.code('ConventionalTopSeg'), CONTROL_TOP, seg=CODE_SEG)
    m.ww(m.data('NextFreeSegment'), FREE)
    m.ww(m.data('ModuleSegment'), MODSEG)
    m.ints[0x10] = bios10
    if m.vga: m.vga.vga_set_retrace(4)
    cvar(m, 'SbDetected', 0)
    cvar(m, 'SoundState', 0)
    cvar(m, 'CpuClass', cpu)
    cvar(m, 'OptionBits', option, size=2)
    cvar(m, 'OptionVByte', vbyte)
    if count is not None:
        cvar(m, 'FadeEntryCount', count)
    cvar(m, 'FadeEntryIndex', 0)
    dvar(m, 'byte_27E4', 0xFF)                       # no joystick
    dvar(m, 'byte_27D9', 1)
    for i in range(18):
        a = m.data('ResourceFlags') + i
        m.wb(a, m.rb(a) & ~0x02)
    dvar(m, 'BiosSeedReady', 1)                      # fixed password seed (see ComputePasswordCode)
    dvar(m, 'BiosSeed', 0x1234, size=2)

def timer(m, last, now):
    m.ww(m.data('word_27EE'), last)
    m.ww(m.code('TimerTickCount'), now, seg=CODE_SEG)

def keys(m, arrow=0, fire=0, confirm=0, scan=0):
    dvar(m, 'byte_282D', fire)
    dvar(m, 'byte_2810', confirm)
    dvar(m, 'LastScanCode', scan)

def log_words(m, words):
    """Input log for PollInputState playback (byte_2879 = 1): one word per entry at DATA:003Fh."""
    dvar(m, 'byte_2879', 1)
    dvar(m, 'word_287A', 0, size=2)
    dvar(m, 'byte_287C', 0)
    dvar(m, 'byte_287D', 0)
    for k, w in enumerate(words):
        m.ww(0x3F + 2 * k, w)

def font(m, seed=7):
    """Font segment NextFreeSegment (glyph records and shifted tables) and word_003D source."""
    rng = random.Random(seed)
    data = bytes(rng.randrange(256) for _ in range(FONT_BYTES))
    m.write(FONT_SEG, 0, data)
    dvar(m, 'word_003D', FONT_SEG, size=2)
    return data

def scratch_planes(seed):
    def f(m):
        rng = random.Random(seed)
        for p in range(4):
            vga_plane(m, p, 0x0000, bytes(rng.randrange(256) for _ in range(0x2000)))
            vga_plane(m, p, 0x2000, bytes(rng.randrange(256) for _ in range(0x2000)))
            vga_plane(m, p, 0x3F40, bytes(rng.randrange(256) for _ in range(0x2000)))
    return f

def dac_random(seed):
    def f(m):
        rng = random.Random(seed)
        vga_restore_dac(m, rng)
    return f

def vga_restore_dac(m, rng):
    # DAC through the port model (the same register traffic as the routines)
    m.vga.vga_out(0x3C8, 0)
    for i in range(256 * 3):
        m.vga.vga_out(0x3C9, rng.randrange(64))

def dac_set(m, values):
    """DAC entries from a flat list of 6-bit components (index 0 first)."""
    m.vga.vga_out(0x3C8, 0)
    for v in values:
        m.vga.vga_out(0x3C9, v & 0x3F)

# -- oracle-built states (cached) -----------------------------------------------------------------
_pic = {}

def load_picture(m, slot):
    """Picture of resource SLOT decoded by the oracle's LoadSqzFile (cached, restored later).
    Returns its segment; the heap top is left where LoadSqzFile leaves it."""
    if slot not in _pic:
        base(m)
        off = m.call('ResourceNameAt', {'CX': slot})['DX']
        seg = m.call('LoadSqzFile', {'DX': off})['AX']
        m.ints[0x21].log.clear()
        m.ints[0x21].next = 5
        m.ports.log.clear()
        _pic[slot] = (m.snapshot(), seg)
    snap, seg = _pic[slot]
    m.restore(snap)
    return seg

_pw = {}

def password(m, index, cpu):
    """ComputePasswordCode(index) for the fixed seed and CpuClass (oracle)."""
    key = (index, cpu)
    if key not in _pw:
        mm = m
        cvar(mm, 'CpuClass', cpu)
        dvar(mm, 'BiosSeedReady', 1)
        dvar(mm, 'BiosSeed', 0x1234, size=2)
        _pw[key] = mm.call('ComputePasswordCode', {'AX': index}, budget=100_000)['AX']
    return _pw[key]

_scan = None

def scan_char(sc):
    """Character of scan code SC in the table at DATA:B068h (read once from the image)."""
    global _scan
    if _scan is None:
        m = Machine()
        m.reset_memory()
        _scan = bytes(m.read(DATA_SEG, SCAN_TABLE, 128))
    return _scan[sc]

def scan_for(ch):
    """First scan code whose table entry is the character CH."""
    for sc in range(128):
        if scan_char(sc) == ord(ch):
            return sc
    raise ValueError(ch)

def code_digits(code):
    return [CHARS[(code >> s) & 0xF] for s in (12, 8, 4, 0)]

# Entries that never read the incoming ES (they load it before use) run with a probe value in ES, so
# that the ES comparison (pre2_es against the oracle's ES after the case) sees the routine's own ES
# loads and not an input that happens to equal them (DATA or A000h).
ES_PROBE = 0x3456
ES_PROBE_CFUNCS = {
    'c_ShowWorldMapScrollIn', 'c_RunCallbackMapScreen', 'c_RunGameOverScreen', 'c_ShowMode13PictureFadeIn',
    'c_ShowPlanarPicture', 'c_ShowPresentTitle', 'c_ShowTitusScreen', 'c_RunFrontEnd', 'c_loc_8E7B',
    'c_loc_8E7B_full', 'c_loc_8EDE', 'c_loc_8EFD', 'c_loc_8F29', 'c_enter_code_tail', 'c_level_start_tail',
    'c_RunLevelIntroScene', 'c_RunLevelIntroScene_once', 'c_loc_8F96', 'c_loc_8F96_frame', 'c_loc_9002',
    'c_RestoreBackdropToDrawPage', 'c_SetPictureCopyPointers', 'c_loc_91CF', 'c_FadeOutPaletteToBlack',
    'c_CopyBackdropStrip', 'c_RunGameOverScreen', 'c_loc_9C62', 'c_intro_group_setup',
}

def C(title, routine, cfunc, setup=None, regs=None, out_regs=(), ranges=(), budget=20_000_000,
      port_inputs=None, tick_model=False, c_setup=None):
    """Case with the DOS model (the pictures and resources) and the heap compared."""
    ranges = [HEAP] + list(ranges)
    if cfunc in ES_PROBE_CFUNCS:
        regs = dict(regs or {}, ES=ES_PROBE)
    return Case(title, routine, cfunc, regs=regs or {}, setup=setup, out_regs=out_regs, ranges=ranges,
                files=True, budget=budget, port_inputs=port_inputs or {}, tick_model=tick_model, c_setup=c_setup)

# -- scripted key press at a poll point ---------------------------------------------------------------
# The frame wait of the map screens (WaitVerticalRetraceStart) polls once per failed read of the
# status port; the C side calls platform_poll() at the same points. With a retrace period of 1 the
# first frame has no poll (its first status read is a retrace) and every later frame has one, so a
# flag set at poll N reaches the callback in frame N + 1: the earlier frames run through the whole
# loop (copies, then the jump back to the frame head) before the exit flag appears.
_POLL = {'on': False, 'n': 0, 'at': 0, 'flag': ''}
_POLL_HOOKED = set()

def arm_poll_key(m, flag, at=1):
    """Oracle half: set DATA byte FLAG to 1 at the AT-th poll (the je of the status wait taken)."""
    _POLL.update(on=True, n=0, at=at, flag=flag)
    if id(m) not in _POLL_HOOKED:
        addr = CODE_SEG * 16 + m.code('WaitVerticalRetraceStart') + 8
        def hook(uc, address, size, user):
            if _POLL['on'] and m.reg('FLAGS') & 0x40:
                _POLL['n'] += 1
                if _POLL['n'] == _POLL['at']:
                    _POLL['on'] = False
                    m.wb(m.data(_POLL['flag']), 1)
        m.uc.hook_add(UC_HOOK_CODE, hook, None, addr, addr)
        _POLL_HOOKED.add(id(m))

def poll_key_c(flag, at=1):
    """C half: platform_poll runs the same script (pre2_test_poll)."""
    import re
    def setup(d):
        mem = d.pre2_memory().contents
        sym = (ROOT / 'c' / 'pre2_sym.h').read_text()
        off = int(re.search(r'#define DS_%s 0x([0-9A-F]+)u' % flag, sym).group(1), 16)
        st = {'n': 0}
        @ctypes.CFUNCTYPE(None)
        def poll():
            st['n'] += 1
            if st['n'] == at:
                mem[DATA_SEG * 16 + off] = 1
        ctypes.c_void_p.in_dll(d, 'pre2_test_poll').value = ctypes.cast(poll, ctypes.c_void_p).value
        return poll
    return setup

def setup_of(*fns):
    def s(m):
        for f in fns:
            if f is not None:
                f(m)
    return s

BYTE_VARS = {'WavePhase', 'ExpertFlag', 'ScreenExitRequested', 'ScreenFirstFrame', 'CodeEntryState',
             'TextAdvance', 'LastScanCode', 'byte_282D', 'byte_2810', 'FineScroll', 'CurrentLevel',
             'SnowFrameTick', 'EndSignal', 'byte_2879', 'PlayerFrameHigh', 'byte_27F6', 'byte_27F7'}

def sv(**kw):
    """Setup helper: sets DATA variables by name (bytes for the BYTE_VARS, words otherwise)."""
    def f(m):
        for name, value in kw.items():
            dvar(m, name, value, size=1 if name in BYTE_VARS else 2)
    return f

def ww(name, value):
    return lambda m: m.ww(m.data(name), value)

def wb(name, value):
    return lambda m: m.wb(m.data(name), value)

# ==================================================================================================
def cases_wait():
    cs = []
    for period in (4, 1, 3):
        cs.append(C(f'WaitVerticalRetraceStart: retrace period {period}', 'WaitVerticalRetraceStart',
                    'c_WaitVerticalRetraceStart', setup=lambda m, p=period: (base(m), m.vga.vga_set_retrace(p))))
    for period in (4, 1, 2, 3):
        cs.append(C(f'WaitNextVerticalRetrace: retrace period {period}', 'WaitNextVerticalRetrace',
                    'c_WaitNextVerticalRetrace', setup=lambda m, p=period: (base(m), m.vga.vga_set_retrace(p))))
    return cs

def cases_advance():
    cs = []
    def adv(title, cpu, phase, mx, my, pmx=0, pmy=0, tfb=0):
        cs.append(C(title, 'AdvanceMapFrame', 'c_AdvanceMapFrame',
                    setup=setup_of(lambda m: base(m, cpu=cpu), sv(WavePhase=phase, MapScrollX=mx, MapScrollY=my,
                                                          PrevMapScrollX=pmx, PrevMapScrollY=pmy))))
    adv('AdvanceMapFrame: CpuClass 0 (no wave term)', 0, 0x02, 0x0008, 0x0010)
    adv('AdvanceMapFrame: wave term +1 row (index 0Bh)', 5, 0x0A, 0x0100, 0x0020)
    adv('AdvanceMapFrame: wave term 0 (index 0Fh>>4)', 5, 0x0E, 0x0040, 0x0000)
    adv('AdvanceMapFrame: MapScrollX 0 wraps to FFFCh, font bank', 3, 0x20, 0x0000, 0x0005)
    adv('AdvanceMapFrame: phase FFh wraps to 0', 5, 0xFF, 0x0107, 0xFFF0)
    adv('AdvanceMapFrame: wave term of 0Fh at index 10h', 5, 0x0F, 0x0009, 0x0100)
    return cs

def cases_font():
    cs = []
    def df(title, text, adv=4, cursor=0x0C38, bp=0x1FFF, ssoff=0, pssoff=0x28, bank=0x0000):
        def s(m):
            base(m)
            font(m, seed=len(title))
            m.vga.vga_out(0x3C4, 2)
            dvar(m, 'NextFreeSegment', FONT_SEG, size=2)          # font segment for glyphs
            dvar(m, 'TextAdvance', adv)
            dvar(m, 'TextCursorOffset', cursor, size=2)
            dvar(m, 'ScreenStartOffset', ssoff, size=2)
            dvar(m, 'PrevScreenStartOffset', pssoff, size=2)
            dvar(m, 'TextFontBank', bank, size=2)
            m.write(DATA_SEG, 0xB300, text + b'\0' + b'\0')
        cs.append(C(title, 'DrawFontString', 'c_DrawFontString', setup=s,
                    regs={'BX': 0xB300, 'BP': bp, 'ES': VGA_SEG}, out_regs=('BX',)))
    df('DrawFontString: digits and letters, advance 4', b'0123456789ABCDEF')
    df('DrawFontString: space uses glyph 2Bh, advance 3', b'A B C', adv=3)
    df('DrawFontString: stops at hyphen (ignored key)', b'AB-CD')
    df('DrawFontString: stops at NUL first (empty string)', b'')
    df('DrawFontString: punctuation (chars below 30h end the string)', b'A:Z[\\]^_`')
    df('DrawFontString: cursor near the row end, font bank 1080h', b'MODE', cursor=0x013E, bank=0x1080, ssoff=0x1000)
    df('DrawFontString: wrap of the draw origin (ScreenStart FFF0h)', b'EXPERT', ssoff=0xFFF0, pssoff=0x0010)
    rng = random.Random(3)
    df('DrawFontString: random letters and digits', bytes(rng.choice(b'0123456789ABCDEFGHZ') for _ in range(20)),
       ssoff=0x0BCD, pssoff=0x1F00)
    return cs

def cases_mode():
    cs = []
    def mode(title, expert=0, scan=0, fire=0, confirm=0, exitreq=0, ssf=0):
        def s(m):
            base(m)
            font(m, seed=11)
            m.vga.vga_out(0x3C4, 2)
            dvar(m, 'NextFreeSegment', FONT_SEG, size=2)
            dvar(m, 'ExpertFlag', expert)
            dvar(m, 'ScreenExitRequested', exitreq)
            dvar(m, 'MapScrollX', 0x0040, size=2)
            dvar(m, 'MapScrollY', 0x0004, size=2)
            dvar(m, 'WavePhase', 0x0A)
            dvar(m, 'byte_282D', fire)
            dvar(m, 'byte_2810', confirm)
            dvar(m, 'LastScanCode', scan)
            dvar(m, 'TextAdvance', 4)
        cs.append(C(title, 'UpdateModeSelectFrame', 'c_UpdateModeSelectFrame', setup=s,
                    regs={'ES': VGA_SEG}, budget=5_000_000))
    mode('UpdateModeSelectFrame: idle, BEGINNER shown')
    mode('UpdateModeSelectFrame: idle, EXPERT shown', expert=1)
    mode('UpdateModeSelectFrame: up arrow (48h) toggles to EXPERT', scan=0x48)
    mode('UpdateModeSelectFrame: left arrow (4Bh) toggles to BEGINNER', expert=1, scan=0x4B)
    mode('UpdateModeSelectFrame: right arrow (4Dh) toggles', scan=0x4D)
    mode('UpdateModeSelectFrame: down arrow (50h) toggles', expert=1, scan=0x50)
    mode('UpdateModeSelectFrame: other make code (1Eh) is consumed, no toggle', scan=0x1E)
    mode('UpdateModeSelectFrame: break code (C8h) is ignored and kept', scan=0xC8)
    mode('UpdateModeSelectFrame: fire key with an arrow: exit, arrow kept', scan=0x48, fire=1)
    mode('UpdateModeSelectFrame: confirm key counts an exit request', confirm=1, exitreq=3)
    mode('UpdateModeSelectFrame: exit counter wraps FFh to 0', confirm=1, exitreq=0xFF)
    return cs

def cases_code_entry():
    cs = []
    def base_code(m, cpu=5, state=0, count=0, value=0, hist=(0, 0, 0), timer_=0):
        base(m, cpu=cpu)
        font(m, seed=13)
        m.vga.vga_out(0x3C4, 2)
        dvar(m, 'NextFreeSegment', FONT_SEG, size=2)
        dvar(m, 'ExpertFlag', 0)
        dvar(m, 'CodeEntryState', state)
        dvar(m, 'CodeInputCount', count, size=2)
        dvar(m, 'CodeValue', value, size=2)
        dvar(m, 'CodeHistory0', hist[0], size=2)
        dvar(m, 'CodeHistory1', hist[1], size=2)
        dvar(m, 'CodeHistory2', hist[2], size=2)
        dvar(m, 'CodeErrorTimer', timer_, size=2)
        dvar(m, 'ScreenExitRequested', 0)
        dvar(m, 'TextAdvance', 3)
        dvar(m, 'byte_282D', 0)
        dvar(m, 'byte_2810', 0)
        for i, ch in enumerate(b'ABC'):
            m.wb(CODE_BOX + i, ch)
    # every scan code (state 0, first digit, the digit lookup of the table entry)
    for sc in range(0x80):
        cs.append(C(f'UpdateCodeEntryFrame: key {sc:02X}h ({chr(scan_char(sc)) if scan_char(sc) >= 0x20 else "ctl"})',
                    'UpdateCodeEntryFrame', 'c_UpdateCodeEntryFrame',
                    setup=setup_of(lambda m, sc=sc: base_code(m),
                                   lambda m, sc=sc: dvar(m, 'LastScanCode', sc)),
                    regs={'ES': VGA_SEG}, budget=5_000_000))
    cs.append(C('UpdateCodeEntryFrame: break code 9Eh ignored', 'UpdateCodeEntryFrame', 'c_UpdateCodeEntryFrame',
                setup=setup_of(lambda m: base_code(m), lambda m: dvar(m, 'LastScanCode', 0x9E)),
                regs={'ES': VGA_SEG}, budget=5_000_000))
    cs.append(C('UpdateCodeEntryFrame: cancel with fire key cancels the entry', 'UpdateCodeEntryFrame',
                'c_UpdateCodeEntryFrame', setup=setup_of(lambda m: base_code(m, count=2, value=0x0123),
                                                       lambda m: dvar(m, 'byte_282D', 1)),
                regs={'ES': VGA_SEG}, budget=5_000_000))
    cs.append(C('UpdateCodeEntryFrame: confirm key cancels the entry', 'UpdateCodeEntryFrame',
                'c_UpdateCodeEntryFrame', setup=setup_of(lambda m: base_code(m, count=1),
                                                       lambda m: dvar(m, 'byte_2810', 1)),
                regs={'ES': VGA_SEG}, budget=5_000_000))
    cs.append(C('UpdateCodeEntryFrame: state 1, timer 8Ah (still waiting)', 'UpdateCodeEntryFrame',
                'c_UpdateCodeEntryFrame', setup=setup_of(lambda m: base_code(m, state=1, count=0, timer_=0x8A),
                                                       lambda m: dvar(m, 'LastScanCode', 0x1E)),
                regs={'ES': VGA_SEG}, budget=5_000_000))
    cs.append(C('UpdateCodeEntryFrame: state 1, timer 8Bh (boxes reset)', 'UpdateCodeEntryFrame',
                'c_UpdateCodeEntryFrame', setup=setup_of(lambda m: base_code(m, state=1, count=2, timer_=0x8B),
                                                       lambda m: dvar(m, 'LastScanCode', 0x1E)),
                regs={'ES': VGA_SEG}, budget=5_000_000))
    cs.append(C('UpdateCodeEntryFrame: state 2 (accepted): exit request, boxes reset', 'UpdateCodeEntryFrame',
                'c_UpdateCodeEntryFrame', setup=setup_of(lambda m: base_code(m, state=2, count=4, value=0x3A7C),
                                                       lambda m: dvar(m, 'LastScanCode', 0x1E)),
                regs={'ES': VGA_SEG}, budget=5_000_000))
    # wrong code and correct codes: the fourth key completes the code
    for index in (0, 1, 5, 9, 10, 12, 0x11, 0x12):
        code = password(_mach(), index, 5)
        last = scan_for(code_digits(code)[3])
        cs.append(C(f'UpdateCodeEntryFrame: code of level {index} (CpuClass 5): accepted',
                    'UpdateCodeEntryFrame', 'c_UpdateCodeEntryFrame',
                    setup=setup_of(lambda m, c=code: base_code(m, value=c >> 4, count=3),
                                   lambda m, s=last: dvar(m, 'LastScanCode', s)),
                    regs={'ES': VGA_SEG}, budget=5_000_000))
    code = password(_mach(), 7, 0)
    last = scan_for(code_digits(code)[3])
    cs.append(C('UpdateCodeEntryFrame: code of level 7 with CpuClass 0 (other rotation)',
                'UpdateCodeEntryFrame', 'c_UpdateCodeEntryFrame',
                setup=setup_of(lambda m, c=code: base_code(m, cpu=0, value=c >> 4, count=3),
                               lambda m, s=last: dvar(m, 'LastScanCode', s)),
                regs={'ES': VGA_SEG}, budget=5_000_000))
    wrong = password(_mach(), 7, 5) ^ 0x0001
    last = scan_for(code_digits(wrong)[3])
    cs.append(C('UpdateCodeEntryFrame: wrong code: state 1, history shifts',
                'UpdateCodeEntryFrame', 'c_UpdateCodeEntryFrame',
                setup=setup_of(lambda m, c=wrong: base_code(m, value=c >> 4, count=3, hist=(0x1111, 0x2222, 0x3333)),
                               lambda m, s=last: dvar(m, 'LastScanCode', s)),
                regs={'ES': VGA_SEG}, budget=5_000_000))
    cs.append(C('UpdateCodeEntryFrame: hashed history accepts level CodeValue-1 (CodeValue 5 -> level 4)',
                'UpdateCodeEntryFrame', 'c_UpdateCodeEntryFrame',
                setup=setup_of(lambda m: base_code(m, value=0x0000 | 0, count=3, hist=(44510, 49374, 61453)),
                               lambda m: dvar(m, 'LastScanCode', scan_for('5'))),
                regs={'ES': VGA_SEG}, budget=5_000_000))
    return cs

def _mach():
    """A machine without the VGA model for the states that do not touch video (generation time)."""
    global _M
    if _M is None:
        _M = Machine()
        _M.reset_memory()
        _M.ints = {0x21: DosFiles()}
        _M.ints[0x10] = bios10
    return _M
_M = None

def cases_code_checks():
    """Entries into the code check (loc_9A27), the accept (loc_9A9B) and the timer/cancel."""
    cs = []
    code = password(_mach(), 9, 5)
    cs.append(C('loc_9A27: correct code of level 9 (table path)', 'loc_9A27', 'c_loc_9A27',
                setup=setup_of(lambda m: base(m, cpu=5), sv(CodeValue=code, CodeHistory0=1, CodeHistory1=2,
                                                          CodeHistory2=3, CodeErrorTimer=5))))
    cs.append(C('loc_9A27: wrong code: history shift and state 1', 'loc_9A27', 'c_loc_9A27',
                setup=setup_of(lambda m: base(m, cpu=5), sv(CodeValue=code ^ 0x4000, CodeHistory0=1,
                                                           CodeHistory1=2, CodeHistory2=3))))
    cs.append(C('loc_9A27: hashed history, CodeValue 6 accepts level 5', 'loc_9A27', 'c_loc_9A27',
                setup=setup_of(lambda m: base(m, cpu=5), sv(CodeValue=6, CodeHistory0=44510,
                                                           CodeHistory1=49374, CodeHistory2=61453))))
    cs.append(C('loc_9A27: hashed history but CodeValue 15h (above 13h): table miss', 'loc_9A27', 'c_loc_9A27',
                setup=setup_of(lambda m: base(m, cpu=5), sv(CodeValue=0x15, CodeHistory0=44510,
                                                           CodeHistory1=49374, CodeHistory2=61453))))
    cs.append(C('loc_9A9B: level 0Bh (expert 1)', 'loc_9A9B', 'c_loc_9A9B', regs={'DX': 0x0B},
                setup=setup_of(lambda m: base(m))))
    cs.append(C('loc_9AB4: timer 8Ah, counts only', 'loc_9AB4', 'c_loc_9AB4',
                setup=setup_of(lambda m: base(m), sv(CodeErrorTimer=0x89, CodeEntryState=1))))
    cs.append(C('loc_9AB4: timer reaches 8Ch: boxes and state reset', 'loc_9AB4', 'c_loc_9AB4',
                setup=setup_of(lambda m: base(m), sv(CodeErrorTimer=0x8B, CodeEntryState=1, CodeInputCount=3))))
    cs.append(C('loc_9AC4: cancel counts an exit and resets the entry', 'loc_9AC4', 'c_loc_9AC4',
                setup=setup_of(lambda m: base(m), sv(ScreenExitRequested=2, CodeInputCount=2, CodeEntryState=2))))
    return cs

def map_callback_cases():
    cs = []
    def rcms(title, callback, cpu=5, phase=0x0A, fire=0, confirm=0, expert=0, scan=0, sx=0x0040, sy=0x0004):
        def s(m):
            base(m, cpu=cpu)
            font(m, seed=21)
            dvar(m, 'ExpertFlag', expert)
            dvar(m, 'LastScanCode', scan)
            dvar(m, 'byte_282D', fire)
            dvar(m, 'byte_2810', confirm)
            dvar(m, 'CurrentLevel', 0x0FF)
            dvar(m, 'FrameCallback', 0)
            m.ww(m.data('MapScrollX'), sx)
            m.ww(m.data('MapScrollY'), sy)
            m.ww(m.data('CameraCol'), 0)
            m.wb(m.data('WavePhase'), phase)
            dac_random(5)(m)
        cs.append(C(title, 'RunCallbackMapScreen', 'c_RunCallbackMapScreen', setup=s,
                    regs={'DX': callback, 'ES': VGA_SEG}, out_regs=(), budget=200_000_000))
    rcms('RunCallbackMapScreen: MODE screen, fire key: one frame, row and column copies', 0x991F, fire=1)
    rcms('RunCallbackMapScreen: MODE screen, CpuClass 0: no row copy', 0x991F, cpu=0, confirm=1)
    rcms('RunCallbackMapScreen: ENTER screen, cancel key: one frame', 0x9985, fire=1)
    rcms('RunCallbackMapScreen: ENTER screen, confirm key, CpuClass 2', 0x9985, cpu=2, confirm=1)
    # The screen starts the wave phase at 2 (WavePhase is not an input) and the callback adds one per
    # frame; the vertical term table6F90[phase] >> 4 is positive for phases 1..7Fh and negative for
    # 81h..FFh, so the map rows move up (MapScrollY - PrevMapScrollY < 0, the upward row copy) only after
    # about 80h frames. The exit key arrives at poll N of the status wait (see arm_poll_key): the
    # first frame has no poll, the others one each, so the screen runs N + 1 frames.
    def frames(title, callback, polls, flag, cpu=5):
        def s(m):
            base(m, cpu=cpu)
            font(m, seed=22)
            dvar(m, 'ExpertFlag', 0)
            dvar(m, 'LastScanCode', 0)
            dvar(m, 'byte_282D', 0)
            dvar(m, 'byte_2810', 0)
            dvar(m, 'CurrentLevel', 0x0FF)
            dvar(m, 'FrameCallback', 0)
            m.ww(m.data('MapScrollX'), 0x0044)
            m.ww(m.data('MapScrollY'), 0x0006)
            m.ww(m.data('CameraCol'), 0)
            dac_random(6)(m)
            if m.vga: m.vga.vga_set_retrace(1)
            arm_poll_key(m, flag, polls)
        cs.append(C(title, 'RunCallbackMapScreen', 'c_RunCallbackMapScreen', setup=s,
                    regs={'DX': callback, 'ES': VGA_SEG}, out_regs=(), budget=900_000_000,
                    c_setup=poll_key_c(flag, polls)))
    frames('RunCallbackMapScreen: MODE screen, 2 frames (fire at the second)', 0x991F, 1, 'byte_282D')
    frames('RunCallbackMapScreen: ENTER screen, 3 frames (confirm at the third)', 0x9985, 2, 'byte_2810')
    frames('RunCallbackMapScreen: MODE screen, 0A0h frames: wave phase in the negative half, upward row copy',
           0x991F, 0x9F, 'byte_282D')
    frames('RunCallbackMapScreen: ENTER screen, 0C8h frames, CpuClass 2: phase through the wave minimum',
           0x9985, 0xC7, 'byte_2810', cpu=2)
    frames('RunCallbackMapScreen: MODE screen, CpuClass 0, 8 frames (no wave term)', 0x991F, 7, 'byte_282D', cpu=0)
    return cs

def game_over_cases():
    cs = []
    def go(title, byte2879=0, fire=0, confirm=0, tick=3, cpu=5, timer_last=0x0100, frames=None):
        def s(m):
            base(m, cpu=cpu)
            dvar(m, 'byte_2879', byte2879)
            dvar(m, 'byte_282D', fire)
            dvar(m, 'byte_2810', confirm)
            dvar(m, 'LastScanCode', 0)
            dvar(m, 'word_287A', 0, size=2)
            timer(m, timer_last, timer_last + tick)
            dac_random(9)(m)
        cs.append(C(title, 'RunGameOverScreen', 'c_RunGameOverScreen', setup=s, budget=200_000_000))
    go('RunGameOverScreen: gameplay (byte_2879 set): returns at once', byte2879=1)
    go('RunGameOverScreen: fire key: one frame and fade', fire=1)
    go('RunGameOverScreen: confirm key, timer advance 3 counts a beat', confirm=1, tick=3)
    return cs

def scroll_cases():
    cs = []
    def ws(title, level, words, sz=0x0810, budget=400_000_000):
        def s(m):
            base(m, cpu=5)
            font(m, seed=2)
            m.write(MARK_SEG, 0, bytes(random.Random(4).randrange(256) for _ in range(0x100)))
            dvar(m, 'CurrentLevel', level)
            dvar(m, 'word_62DA', 0x0000, size=2)
            dvar(m, 'word_667A', MARK_SEG, size=2)
            dvar(m, 'word_7522', sz, size=2)
            dvar(m, 'LastScanCode', 0)
            dvar(m, 'EndSignal', 0)
            log_words(m, words)
            dac_random(3)(m)
        cs.append(C(title, 'ShowWorldMapScrollIn', 'c_ShowWorldMapScrollIn', setup=s, budget=budget))
    ws('ShowWorldMapScrollIn: level 0, action in the first frame', 0, [0x0010])
    ws('ShowWorldMapScrollIn: level 4, action in the fifth frame', 4, [0x0300, 0x0010])
    ws('ShowWorldMapScrollIn: level 9, marker 24 x 5 px, action in the third frame', 9,
       [0x0200, 0x0010], sz=0x0518)
    ws('ShowWorldMapScrollIn: level 2, full scroll and hold, no action', 2,
       [0xFF00, 0xFF00, 0xFF00, 0xFF00], budget=400_000_000)
    return cs

def scroll_part_cases():
    cs = []
    def hold(title, words):
        def s(m):
            base(m, cpu=5)
            dvar(m, 'LastScanCode', 0)
            log_words(m, words)
            dac_random(6)(m)
        cs.append(C(title, 'loc_96AE', 'c_loc_96AE', setup=s, budget=50_000_000))
    hold('loc_96AE hold: action at once', [0x0010])
    hold('loc_96AE hold: 70 frames then the fade', [0x4500 | 0x00])     # value 0, count 45h: 70 frames
    def tail(m):
        base(m, cpu=5)
        dac_random(8)(m)
    cs.append(C('loc_96C1 fade tail: palette fade and the two flags cleared', 'loc_96C1', 'c_loc_96C1',
                setup=setup_of(tail, sv(LastScanCode=0x1E, EndSignal=1))))
    return cs

def frame_body_cases():
    cs = []
    def body(title, callback, mx, my, pmx, pmy, cpu=5, phase=0x0A, exit_key=None, state=0):
        def s(m):
            base(m, cpu=cpu)
            font(m, seed=31)
            dac_random(4)(m)
            dvar(m, 'FrameCallback', callback, size=2)
            dvar(m, 'MapScrollX', mx, size=2)
            dvar(m, 'MapScrollY', my, size=2)
            dvar(m, 'PrevMapScrollX', pmx, size=2)
            dvar(m, 'PrevMapScrollY', pmy, size=2)
            dvar(m, 'ScreenFirstFrame', 0)
            dvar(m, 'ScreenExitRequested', 0)
            dvar(m, 'WavePhase', phase)
            dvar(m, 'ScreenStartOffset', 0x0520, size=2)
            dvar(m, 'PrevScreenStartOffset', 0x0500, size=2)
            dvar(m, 'ExpertFlag', 1)
            dvar(m, 'CodeEntryState', state)
            dvar(m, 'byte_282D', 1 if exit_key else 0)
            dvar(m, 'NextFreeSegment', FONT_SEG, size=2)
            dvar(m, 'TextAdvance', 4)
            dvar(m, 'LastScanCode', 0)
            m.vga.vga_out(0x3C4, 2)
        cs.append(C(title, 'loc_97A8', 'c_loc_97A8', setup=s, regs={'ES': VGA_SEG}, budget=50_000_000))
    body('loc_97A8 frame: MODE callback, fire key, column and row copies', 0x991F, 0x0108, 0x0004, 0x0000, 0x0002,
         exit_key=True)
    body('loc_97A8 frame: MODE callback, no column copy (bit 3 unchanged)', 0x991F, 0x0100, 0x0004, 0x00F8, 0x0004,
         exit_key=True, cpu=0)
    body('loc_97A8 frame: ENTER callback in state 2 exits through loc_9AC4', 0x9985, 0x0080, 0x0010, 0x0080, 0x0010,
         state=2, exit_key=False)
    return cs

def fade_cases():
    cs = []
    def fo(title, values=None, count=None, vbyte=5, option=1, seed=None):
        def s(m):
            base(m, cpu=5, option=option, vbyte=vbyte, count=count)
            if values is not None:
                dac_set(m, values)
            elif seed is not None:
                dac_random(seed)(m)
        cs.append(C(title, 'FadeOutPaletteToBlack', 'c_FadeOutPaletteToBlack', setup=s, budget=50_000_000))
    fo('FadeOutPaletteToBlack: all components 4 (one step to 0, a second pass)', values=[4] * 768, count=0xFF)
    fo('FadeOutPaletteToBlack: components 5 and 3, count 1 entry', values=[5, 3, 0] * 256, count=1)
    fo('FadeOutPaletteToBlack: random DAC, count 0x20 (one retrace group)', count=0x20, seed=12)
    fo('FadeOutPaletteToBlack: random DAC, count 0xFF', count=0xFF, seed=13)
    fo('FadeOutPaletteToBlack: no OptionBits bit 0: returns at once', seed=14, option=0)
    fo('FadeOutPaletteToBlack: OptionVByte 1 (no fade): returns', seed=15, vbyte=1)
    fo('FadeOutPaletteToBlack: count 0 (one entry processed)', values=[60, 60, 60] * 256, count=0)
    return cs

def fade_in_cases():
    cs = []
    def pic(title, slot, count, bx, option=1, vbyte=5, levelseg=True, level=4):
        def s(m):
            seg = load_picture(m, slot)
            base(m, cpu=5, option=option, vbyte=vbyte, count=count)
            dvar(m, 'LevelSegment', seg, size=2)
            dvar(m, 'CurrentLevel', level)
            cvar(m, 'FadeEntryCount', count)
        cs.append(C(title, 'ShowMode13PictureFadeIn', 'c_ShowMode13PictureFadeIn', setup=s,
                    regs={'CX': count, 'BX': bx}, out_regs=('BX',), budget=200_000_000))
    pic('ShowMode13PictureFadeIn: MENU picture, 0A0h entries (front end)', 8, 0xA0, 0x0000)
    pic('ShowMode13PictureFadeIn: MENU picture, 10h entries (TITUS count)', 8, 0x10, 0x0000)
    pic('ShowMode13PictureFadeIn: MENU picture, 0FFh entries, BX in', 8, 0xFF, 0x1234)
    pic('ShowMode13PictureFadeIn: MENU picture, option bit 0 clear: instant write, BX kept', 8, 0xA0, 0x0A0B,
        option=0)
    pic('ShowMode13PictureFadeIn: MENU2 planar palette source, count 1', 12, 0x01, 0x0000)
    return cs

def fade_pass_cases():
    cs = []
    def fp(title, slot, count, bx, option=1):
        seg = load_picture(_mach(), slot)            # the same segment every time (generation time)
        def s(m):
            load_picture(m, slot)
            base(m, cpu=5, option=option, count=count)
            dvar(m, 'CurrentLevel', 4)
            dac_random(17)(m)
            arm_stop(m, 'loc_926C')                  # the original's pop ds (the push is not entered)
        cs.append(C(title, 'loc_91CF', 'c_loc_91CF', setup=s, regs={'DS': seg, 'BX': bx},
                    out_regs=('BX',), budget=200_000_000))
    fp('loc_91CF fade-in passes over MENU palette, count 40h', 8, 0x40, 0x0000)
    fp('loc_91CF instant write (option bit 0 clear), BX kept', 8, 0x40, 0x0A0B, option=0)
    return cs

def planar_cases():
    cs = []
    def sp(title, slot, level):
        seg = load_picture(_mach(), slot)
        def s(m):
            load_picture(m, slot)
            base(m, cpu=5, count=0xA0)
            dvar(m, 'CurrentLevel', level)
            dac_random(19)(m)
            dvar(m, 'DrawPage', 0x2000, size=2)
            dvar(m, 'DisplayPage', 0, size=2)
        cs.append(C(title, 'ShowPlanarPicture', 'c_ShowPlanarPicture', setup=s,
                    regs={'DS': seg, 'SI': 0}, budget=200_000_000))
    sp('ShowPlanarPicture: MENU2 picture (planes and palette at 7D00h), level 4', 12, 4)
    sp('ShowPlanarPicture: MENU2 picture, level 9 palette', 12, 9)
    return cs

def title_cases():
    cs = []
    def title(name, cfunc, routine, budget=500_000_000):
        def s(m):
            base(m, cpu=5, option=1, vbyte=5, count=0xA0)
            dvar(m, 'NextFreeSegment', FREE, size=2)
        cs.append(C(name, routine, cfunc, setup=s, budget=budget))
    title('ShowPresentTitle: PRESENT.SQZ, fade-in, logo strip, palette morph', 'c_ShowPresentTitle', 'ShowPresentTitle')
    title('ShowTitusScreen: TITUS.SQZ, 16 entries, 70 retraces, fade out', 'c_ShowTitusScreen', 'ShowTitusScreen')
    return cs

def setpic_cases():
    def s(m):
        dvar(m, 'LevelSegment', 0x5000, size=2)
    return [C('SetPictureCopyPointers: DS = LevelSegment + 30h, SI 0, ES A000h, DI 0',
              'SetPictureCopyPointers', 'c_SetPictureCopyPointers', setup=setup_of(lambda m: base(m), s),
              out_regs=('DS', 'SI', 'ES', 'DI'))]

def front_end_cases():
    cs = []
    def fe(title, flag27F6=0, fire=0, cpu=5, exp=1):
        def s(m):
            base(m, cpu=cpu, option=1, vbyte=5, count=0xA0)
            font(m, seed=41)
            dvar(m, 'byte_27F6', flag27F6)
            dvar(m, 'byte_27F7', 0)
            dvar(m, 'byte_282D', fire)
            dvar(m, 'byte_2810', 0)
            dvar(m, 'ExpertFlag', exp)
            dvar(m, 'CurrentLevel', 0)
            dvar(m, 'word_78C4', 0x0000, size=2)
            dvar(m, 'LastScanCode', 0)
            dvar(m, 'MapScrollX', 0, size=2)
            dvar(m, 'word_003D', FONT_SEG, size=2)
            dac_random(23)(m)
        cs.append(C(title, 'RunFrontEnd', 'c_RunFrontEnd', setup=s, budget=500_000_000))
    fe('RunFrontEnd: MODE key: picture, one MODE frame, exit tail (CpuClass 5)', fire=1)
    fe('RunFrontEnd: MODE request byte_27F6 with a key to leave the screen (CpuClass 0)', flag27F6=1, fire=1, cpu=0)
    return cs

def level_start_cases():
    """The level start of RunFrontEnd: entered at its loop head with the timer at 10Eh, stopped
    before the intro scene (the intro runs multi-frame and is tested on its own), then the tail
    after the intro, and loc_8F29."""
    cs = []
    head = _mach().code('loc_8E7B')
    def start(m, count=0xA0):
        base(m, cpu=5, option=1, vbyte=5, count=count)
        font(m, seed=43)
        dvar(m, 'byte_27F6', 0)
        dvar(m, 'byte_27F7', 0)
        dvar(m, 'byte_282D', 0)
        dvar(m, 'byte_2810', 0)
        dvar(m, 'word_27F0', 0x010E, size=2)
        dvar(m, 'CurrentLevel', 4)
        dac_random(43)(m)
    def head_setup(m):
        start(m)
        arm_stop(m, head + 0x35)                       # the call of RunLevelIntroScene
    # the loop head expects AL = 0 (the original's xor al, al before loc_8E7B): AX is the contract
    cs.append(C('RunFrontEnd level start (loc_8E7B, timer 10Eh): fade, picture 0Ch, planar picture',
                'loc_8E7B', 'c_loc_8E7B', setup=head_setup, regs={'AX': 0}, budget=500_000_000))
    def tail_setup(m):
        base(m, cpu=5, option=1, vbyte=5, count=0xA0)
        dac_random(44)(m)
        dvar(m, 'byte_083E', 0x0B, size=1)
        dvar(m, 'byte_083D', 1, size=1)
        dvar(m, 'CurrentLevel', 2)
        dvar(m, 'byte_B198', 0)
        dvar(m, 'byte_2879', 0)
        arm_stop(m, 'loc_8F29')
    cs.append(C('RunFrontEnd level start tail after the intro: fade, level 0Bh from 083Eh, byte_2879 set',
                head + 0x38, 'c_level_start_tail', setup=tail_setup, budget=50_000_000))
    # The demo start path end to end: the intro scene runs (ES = A000h), then the level and expert
    # flag come from 083Eh and 083Dh (here a level number above 9, as the front end's caller tests).
    def full_setup(level):
        def s(m):
            start(m)
            for plane in range(4):
                vga_plane(m, plane, 0, bytes(random.Random(91 + plane).randrange(256) for _ in range(0x2000)))
            dvar(m, 'DrawPage', 0x2000, size=2)
            timer(m, 0x0200, 0x0203)
            dvar(m, 'LastScanCode', 0)
            dvar(m, 'byte_083E', level)
            dvar(m, 'byte_083D', 1)
            dvar(m, 'byte_B198', 0)
        return s
    for level in (0x0B, 0x03):
        cs.append(C(f'RunFrontEnd level start end to end (demo path): intro scene, level {level:02X}h from 083Eh',
                    'loc_8E7B', 'c_loc_8E7B_full', setup=full_setup(level), regs={'AX': 0}, budget=3_000_000_000,
                    tick_model=True))
    def fade_only(m):
        base(m, cpu=5, option=1, vbyte=5, count=0xA0)
        dac_random(45)(m)
    cs.append(C('loc_8F29: fade out and return', 'loc_8F29', 'c_loc_8F29', setup=fade_only, budget=50_000_000))
    return cs

def enter_cases():
    """ENTER CODE request: a cancel or confirm key ends the code screen with CurrentLevel still
    FFh, so the original restarts RunFrontEnd (the restart is the stop point). An accepted code
    needs four key presses in separate frames, which the oracle cannot deliver (the map loop does
    not poll the keyboard), so only the restart is run."""
    cs = []
    restart = _mach().code('loc_8EDE') + 0x1A                 # jmp near ptr RunFrontEnd
    def enter(title, fire=0, confirm=0):
        def s(m):
            base(m, cpu=5)
            font(m, seed=47)
            dvar(m, 'byte_27F6', 0)
            dvar(m, 'byte_27F7', 1)
            dvar(m, 'byte_282D', fire)
            dvar(m, 'byte_2810', confirm)
            dvar(m, 'LastScanCode', 0)
            dvar(m, 'ExpertFlag', 0)
            dvar(m, 'CurrentLevel', 0)
            dac_random(47)(m)
            arm_stop(m, restart)
        cs.append(C(title, 'loc_8EDE', 'c_loc_8EDE', setup=s, budget=50_000_000))
    enter('ENTER CODE, cancel key: CurrentLevel stays FFh, RunFrontEnd restarts', fire=1)
    enter('ENTER CODE, confirm key: restart as well', confirm=1)
    # After the code screen (the compare of CurrentLevel): an accepted code (CurrentLevel >= 0) takes
    # the exit tail with ES = DATA, a cancelled one (FFh) restarts. The oracle cannot deliver the four
    # keys of a real acceptance, so the state the screen leaves is constructed.
    after = _mach().code('loc_8EDE') + 0x13
    def accepted(title, level, expert, cpu):
        def s(m):
            base(m, cpu=cpu)
            dac_random(48)(m)
            dvar(m, 'CurrentLevel', level)
            dvar(m, 'ExpertFlag', expert)
            dvar(m, 'word_78C4', 0x0000, size=2)
        cs.append(C(title, after, 'c_enter_code_tail', setup=s, budget=50_000_000))
    accepted('ENTER CODE accepted, level 7: exit tail (ES = DATA)', 7, 0, 5)
    accepted('ENTER CODE accepted, level 0Bh (>= 0Ah): exit tail, expert', 0x0B, 1, 2)
    accepted('ENTER CODE accepted, level 7Fh (largest non-negative)', 0x7F, 1, 3)
    def cancelled(m):
        base(m, cpu=5)
        dvar(m, 'CurrentLevel', 0xFF)
        arm_stop(m, restart)
    cs.append(C('ENTER CODE cancelled (CurrentLevel FFh): restart, ES untouched', after, 'c_enter_code_tail',
                setup=cancelled, budget=5_000_000))
    return cs

def exit_tail_cases():
    cs = []
    def tail(title, level, expert, cpu):
        def s(m):
            base(m, cpu=cpu, option=1, vbyte=5, count=0xA0)
            dac_random(29)(m)
            dvar(m, 'CurrentLevel', level)
            dvar(m, 'ExpertFlag', expert)
            dvar(m, 'word_78C4', 0x0000, size=2)
        cs.append(C(title, 'loc_8EFD', 'c_loc_8EFD', setup=s, budget=50_000_000))
    tail('loc_8EFD exit tail: level 5, beginner, CpuClass 5 (word_78C4 set)', 5, 0, 5)
    tail('loc_8EFD exit tail: level 0Bh (expert), CpuClass 2 (no word_78C4)', 0x0B, 1, 2)
    return cs

def intro_cases():
    cs = []
    def scene_state(m):
        base(m, cpu=5, option=1, vbyte=5)
        for plane in range(4):
            vga_plane(m, plane, 0, bytes(random.Random(51 + plane).randrange(256) for _ in range(0x2000)))
        dvar(m, 'DrawPage', 0x2000, size=2)
        dac_random(33)(m)
        timer(m, 0x0200, 0x0203)
        dvar(m, 'byte_2879', 0)
        dvar(m, 'LastScanCode', 0)
    back_edge = lambda m: m.code('loc_8F96') + 0x2E          # jne loc_8F96 of the walk-in
    def walk_once(title, routine, cfunc, regs):
        def s(m):
            scene_state(m)
            arm_stop(m, back_edge(m))
        cs.append(C(title, routine, cfunc, setup=s, regs=regs, out_regs=('BP',), budget=50_000_000))
    walk_once('RunLevelIntroScene: setup and one walk-in frame (stops at the back edge)',
              'RunLevelIntroScene', 'c_RunLevelIntroScene_once', {})
    walk_once('loc_8F96: one walk-in frame from the loop head (BP 0)', 'loc_8F96', 'c_loc_8F96_frame',
              {'BP': 0})
    def group(m):
        base(m, cpu=5)
        dvar(m, 'PlayerX', 0x0010, size=2)
        dvar(m, 'PlayerY', 0xA9, size=2)
        dvar(m, 'word_4F2E', 0x0030, size=2)
        dvar(m, 'PlayerAnimPtr', 0x7BA7, size=2)
        arm_stop(m, 'loc_9002')
    push_ds = _mach().code('loc_8F96') + 0x30               # the instruction after the walk-in loop
    cs.append(C('intro group set-up: follower records placed from PlayerX, PlayerY (to loc_9002)',
                push_ds, 'c_intro_group_setup', setup=group, budget=5_000_000))
    return cs

def walk_off_cases():
    cs = []
    def wo(title, px, tick=3):
        def s(m):
            base(m, cpu=5)
            dvar(m, 'PlayerX', px, size=2)
            dvar(m, 'PlayerY', 0xA9, size=2)
            dvar(m, 'PlayerAnimPtr', 0x7BA7, size=2)
            dvar(m, 'word_4F2E', 0x30, size=2)
            dvar(m, 'word_4F30', 0xA9, size=2)
            dvar(m, 'PlayerFacing', 0, size=2)
            dvar(m, 'DrawPage', 0x2000, size=2)
            dvar(m, 'FineScroll', 0)
            timer(m, 0x0100, 0x0100 + tick)
            dac_random(37)(m)
            vga_plane(m, 0, 0x3F40, bytes(range(256)) * 31)
        cs.append(C(title, 'loc_9002', 'c_loc_9002', setup=s, budget=200_000_000))
    wo('loc_9002 walk-off: PlayerX 5 goes negative after one frame', 5)
    wo('loc_9002 walk-off: PlayerX 0 then tick advance 4', 0, tick=4)
    return cs

def gameover_parts():
    cs = []
    def bd(title, fs, dp, seed=61):
        def s(m):
            base(m, cpu=5)
            scratch_planes(seed)(m)
            dvar(m, 'FineScroll', fs)
            dvar(m, 'DrawPage', dp, size=2)
        cs.append(C(title, 'CopyBackdropStrip', 'c_CopyBackdropStrip', setup=s, budget=50_000_000))
    bd('CopyBackdropStrip: FineScroll 0, page 2000h', 0x00, 0x2000)
    bd('CopyBackdropStrip: FineScroll 40h, page 0', 0x40, 0x0000)
    bd('CopyBackdropStrip: FineScroll B9h (last row), page 0A34h', 0xB9, 0x0A34)
    return cs

def sprite_record_cases():
    cs = []
    def rec(title, routine, cfunc, si, di, seed):
        def s(m):
            base(m)
            rng = random.Random(seed)
            m.write(DATA_SEG, si, bytes(rng.randrange(256) for _ in range(0x12)))
            m.write(DATA_SEG, di, bytes(rng.randrange(256) for _ in range(0x12)))
        cs.append(C(title, routine, cfunc, setup=s, regs={'SI': si, 'DI': di, 'ES': DATA_SEG}))
    rec('SwapSpriteRecords: two records at 5138h and 514Ah', 'SwapSpriteRecords', 'c_SwapSpriteRecords', 0x5138, 0x514A, 71)
    rec('SwapSpriteRecords: records at 4FD0h and 4F30h', 'SwapSpriteRecords', 'c_SwapSpriteRecords', 0x4FD0, 0x4F30, 72)
    rec('CopySpriteRecord: 5138h to 4FD0h', 'CopySpriteRecord', 'c_CopySpriteRecord', 0x5138, 0x4FD0, 73)
    rec('CopySpriteRecord: 515Ch to 4FF8h', 'CopySpriteRecord', 'c_CopySpriteRecord', 0x515C, 0x4FF8, 74)
    return cs

def sprite_update_cases():
    cs = []
    def upd(title, fine, tick, w5088, seed, phases=(0x00, 0x20, 0x40), paths=(0xB020, 0xB032, 0xB054)):
        def s(m):
            base(m)
            rng = random.Random(seed)
            dvar(m, 'FineScroll', fine)
            dvar(m, 'SnowFrameTick', tick)
            dvar(m, 'word_5088', w5088, size=2)
            for k in range(8):
                si = 0x4F1C + 0x12 * k
                m.ww(si, rng.randrange(0, 0x200))
                m.ww(si + 2, rng.randrange(0, 0x180))
                m.ww(si + 0x0E, rng.randrange(0, 8))
                m.wb(si + 0x0E, rng.randrange(0, 16))
            for k, (ph, path) in enumerate(zip(phases, paths)):
                si = 0x5138 + 0x12 * k
                m.ww(si, rng.randrange(0, 0x200))
                m.ww(si + 2, rng.randrange(0x40, 0x180))
                m.wb(si + 8, ph)
                m.ww(si + 0x0C, path, seg=DATA_SEG)
                m.ww(si + 4, 0x0000)
        cs.append(C(title, 'UpdateGameOverSprites', 'c_UpdateGameOverSprites', setup=s, budget=5_000_000))
    upd('UpdateGameOverSprites: FineScroll B8h advances, snow tick 0 updates 5088h', 0xB8, 0, 0x0065, 81)
    upd('UpdateGameOverSprites: FineScroll B9h holds, snow tick 1 leaves 5088h', 0xB9, 1, 0x0065, 82)
    upd('UpdateGameOverSprites: 5088h wraps to 68h past 6Dh', 0x10, 0, 0x006D, 83)
    upd('UpdateGameOverSprites: velocity wraps at 8 (-7..7)', 0x20, 4, 0x0040, 84)
    # y of an item = 135h + (table7090[phase] * 10 >> 6): rising phases give items whose later neighbour is
    # not below the earlier one, which the bubble pass swaps (it restarts after every swap)
    upd('UpdateGameOverSprites: items in ascending y: bubble swaps until sorted', 0x20, 0, 0x0040, 85,
        phases=(0x40, 0x20, 0x00))
    upd('UpdateGameOverSprites: one adjacent pair out of order, one swap', 0x20, 0, 0x0040, 86, phases=(0x20, 0x00, 0x40))
    # a path script at an end marker (FFF0h, FFE0h, FFEEh): the relative jump goes back to the loop start
    upd('UpdateGameOverSprites: path pointer at the FFF0h marker jumps back 16 bytes', 0x20, 0, 0x0040, 87,
        paths=(0xB030, 0xB032, 0xB054))
    upd('UpdateGameOverSprites: path pointers at the FFE0h and FFEEh markers', 0x20, 0, 0x0040, 88,
        paths=(0xB052, 0xB066, 0xB054))
    return cs

def cases_all():
    out = []
    for f in (cases_wait, cases_advance, cases_font, cases_mode, cases_code_entry, cases_code_checks,
              map_callback_cases, scroll_cases, scroll_part_cases, frame_body_cases, game_over_cases,
              gameover_parts, sprite_record_cases, sprite_update_cases, fade_cases, fade_in_cases,
              fade_pass_cases, planar_cases, title_cases, setpic_cases, front_end_cases, level_start_cases, enter_cases,
              exit_tail_cases,
              intro_cases, walk_off_cases):
        out += f()
    return out

CASES = cases_all()
