"""System services of src/MODULE1.ASM vs c/system.c: sound-effect selector (cs:0282h),
resource activation and paging, the disk path of the resource dispatcher (cs:0492h),
palettes and text helpers (cs:0B74h..0CB2h), input polling with the demo log (cs:0DC1h),
joystick reads, the timer-vector swap (cs:177Fh) and the keyboard interrupt (cs:1823h).

Interaction devices of the oracle side (the harness supplies them per case):
  * port IN values are the constant presets of Case(port_inputs=...);
  * INT 10h AX=1012h (DAC block) is served by a handler installed in setup that does what
    the BIOS does, as port writes into the same VGA model the C side uses;
  * the ESC quit path of KeyboardIsr: the oracle stops at its first instruction (loc_1850),
    the C side returns SYS_QUIT, and both report it through ZF (see c_KeyboardIsr). The
    path past that point cannot finish under constant port presets (the PIT wait at
    loc_190C needs both port 40h >= F0h and <= 60h), so it is a stated boundary.

Not in CASES: CalibrateJoystick. Its 3 x 3200 IN reads (about 86 KB of port log) overflow the
64 KiB log of c/ports.c, so the port-log comparison fails although memory and registers agree
(checked with a larger log in a scratch build). Add its cases when that log is larger.
"""
import random
from difftest import Case
from emu import Machine, DosFiles, PortLog, DATA_SEG, CODE_SEG, STACK_SEG
from unicorn import UC_HOOK_CODE

FREE = 0x2530                 # first paragraph after the loaded image (as in tests/sqz.py)
MODSEG = 0x7000               # ModuleSegment for the resource cases (heap ends at A000h)
FONT_SEG = 0x5000             # decoded ALLFONTS for the text cases
TEXT_STR = 0x3000             # DATA offset of the PrintAsciiz string (read-only use)
NAME_OFF = 0x000C             # DATA offset of an SQZ file name for the helper decoder
CONTROL_TOP = 0xA000          # ConventionalTopSeg
HEAP = (FREE, 0, (CONTROL_TOP - FREE) * 16)       # the whole conventional heap range
IVT = (0, 0, 0x400)                                  # interrupt vector table

# -- oracle helpers ----------------------------------------------------------------------
_decoded = {}

def decode_sqz(name):
    """Bytes of an SQZ/TRK resource decoded by the original loader (sub_107B), run on a
    separate machine so the case itself is untouched. Cached per name."""
    if name not in _decoded:
        m2 = Machine()
        m2.reset_memory()
        m2.ints = {0x21: DosFiles()}
        m2.ports = PortLog()
        m2.write(DATA_SEG, NAME_OFF, name.encode() + b'\0')
        m2.ww(m2.code('ConventionalTopSeg'), CONTROL_TOP, seg=CODE_SEG)
        m2.ww(m2.data('NextFreeSegment'), FREE)
        seg = m2.call('LoadSqzFile', dict(DX=NAME_OFF), budget=200_000_000)['AX']
        top = m2.rw(m2.data('NextFreeSegment'))
        _decoded[name] = m2.read(seg, 0, (top - seg) * 16 + 16)
    return _decoded[name]

def int10_dac(m):
    """Oracle side of INT 10h AX=1012h: DAC registers BX..BX+CX-1 from ES:DX, as the BIOS
    writes them (index on 3C8h, then R, G, B on 3C9h)."""
    if m.reg('AX') != 0x1012:
        return False
    first, count = m.reg('BX'), m.reg('CX')
    seg, off = m.reg('ES'), m.reg('DX')
    m.vga.vga_out(0x3C8, first & 0xFF)
    for i in range(count * 3):
        m.vga.vga_out(0x3C9, m.read(seg, (off + i) & 0xFFFF, 1)[0])
    return True

_QUIT = {'armed': False, 'hooked': set()}

def arm_quit_boundary(m, armed):
    """Stop the oracle at loc_1850 (the ESC quit path entry) when armed. The hook sets the
    instruction pointer to the stop address so the call returns normally."""
    if id(m) not in _QUIT['hooked']:
        addr = CODE_SEG * 16 + m.code('loc_1850')
        def hook(uc, address, size, user):
            if _QUIT['armed']:
                m.setreg('CS', CODE_SEG)
                m.setreg('IP', 0)
                uc.emu_stop()
        m.uc.hook_add(UC_HOOK_CODE, hook, None, addr, addr)
        _QUIT['hooked'].add(id(m))
    _QUIT['armed'] = armed

def cvar(m, name, value, size=1):
    (m.wb if size == 1 else m.ww)(m.code(name), value, seg=CODE_SEG)

def dvar(m, name, value, size=1):
    (m.wb if size == 1 else m.ww)(m.data(name), value)

# -- sound effects (cs:0282h) ------------------------------------------------------------
def sfx_setup(mode):
    def setup(m):                       # the shipped sample table at DATA:1009h stays as it is
        cvar(m, 'AdLibEnabled', 1 if mode == 'adlib' else 0)
        cvar(m, 'SbDetected', 1 if mode == 'sb' else 0)
    return setup

def sfx_cases():
    out = []
    for dx in (0x0000, 0x0005, 0x000A, 0x00FF, 0x0305):      # speaker: DH is not used
        out.append(Case(f'speaker sequence, DX={dx:04X}', 'StartSoundEffect', 'c_StartSoundEffect',
                        regs=dict(DX=dx), setup=sfx_setup('speaker'), out_regs=()))
    for mode in ('sb', 'adlib'):
        for dx in (0x0000, 0x0001, 0x0007, 0x000A, 0x0105, 0x0300):   # sample: whole DX * 4
            out.append(Case(f'{mode} sample table, DX={dx:04X}', 'StartSoundEffect', 'c_StartSoundEffect',
                            regs=dict(DX=dx), setup=sfx_setup(mode), out_regs=()))
    return out

# -- resource names and paging (cs:0492h) ------------------------------------------------
def names_case(k):
    return Case(f'ResourceNameAt({k})', 'ResourceNameAt', 'c_ResourceNameAt', regs=dict(CX=k), out_regs=('DX',))

def resource_setup(stale=None, sb=1, option=0, demo=0, active=None):
    """Heap top, module segment (with the stale content of an earlier module when given)
    and the sound flags of the resource cases."""
    def setup(m):
        m.ww(m.code('ConventionalTopSeg'), CONTROL_TOP, seg=CODE_SEG)
        m.ww(m.data('NextFreeSegment'), FREE)
        m.ww(m.data('ModuleSegment'), MODSEG)
        cvar(m, 'SbDetected', sb)
        cvar(m, 'SoundState', 0)
        m.ww(m.code('OptionBits'), option, seg=CODE_SEG)
        dvar(m, 'byte_2879', demo)
        if stale is not None:
            m.write(MODSEG, 0, stale[:0x105B * 16])
        if active is not None:
            m.wb(m.data('ResourceFlags') + active[0], m.rb(m.data('ResourceFlags') + active[0]) | active[1])
    return setup

def page_case(slot, flags_note, stale=None):
    return Case(f'PageInResource slot {slot} ({flags_note})', 'PageInResource', 'c_PageInResource',
                regs=dict(CX=slot), setup=resource_setup(stale=stale), out_regs=(), ranges=[HEAP],
                files=True)

def activate_case(title, slot, **kw):
    return Case(title, 'ActivateResourceSlot', 'c_ActivateResourceSlot', regs=dict(AX=slot),
                setup=resource_setup(**kw), out_regs=(), ranges=[HEAP], files=True)

def resource_cases():
    out = [names_case(k) for k in (0, 1, 2, 3, 5, 8, 12, 17)]
    out.append(page_case(0, 'flags 00h, disk, not pinned'))
    out.append(page_case(3, 'flags 00h, disk, not pinned'))
    out.append(page_case(9, 'flags 00h, disk, not pinned'))
    out.append(page_case(5, 'flags 08h, disk, pinned: no copy'))
    out.append(page_case(11, 'flags 08h, disk, pinned: no copy', stale=decode_sqz('BOULA.TRK')))
    out.append(activate_case('activate PRES.TRK (slot 0), full module build', 0))
    out.append(activate_case('activate PRESENTA.TRK (slot 3), full module build', 3))
    out.append(activate_case('activate BOULA.TRK (slot 17), full module build', 17))
    out.append(activate_case('activate MINES.TRK (slot 9): other active slots lose bit 01h', 9))
    out.append(activate_case('activate MAP.SQZ (slot 5, pinned): header read from stale module',
                             5, stale=decode_sqz('BOULA.TRK')))
    out.append(activate_case('activate GAMEOVER.SQZ (slot 11, pinned): stale module, not a MOD',
                             11, stale=decode_sqz('BOULA.TRK')))
    out.append(activate_case('OptionBits 40h: sound update only, no load', 0, option=0x40))
    out.append(activate_case('no Sound Blaster: returns at once', 0, sb=0))
    out.append(activate_case('slot already active: returns at once', 4, active=(4, 1)))
    out.append(activate_case('demo playback (byte_2879 = 1): returns at once', 0, demo=1))
    return out

# -- text and palettes (cs:0B74h..0CB2h) -------------------------------------------------
def text_setup(char_row=0x0A28, col=0x0000, page=0x0000, text=None):
    def setup(m):
        m.write(FONT_SEG, 0, decode_sqz('ALLFONTS.SQZ'))
        m.ww(m.data('word_003D'), FONT_SEG)
        m.ww(m.data('TextRowOffset'), char_row)
        m.ww(m.data('TextByteColumn'), col)
        m.ww(m.data('DisplayPage'), page)
        if text is not None:
            m.write(DATA_SEG, TEXT_STR, text)
    return setup

def text_cases():
    out = []
    for ch in (' ', '0', '7', '9', 'A', 'Z', 'a', '~'):
        out.append(Case(f'DrawFontChar {ch!r}', 'DrawFontChar', 'c_DrawFontChar', regs=dict(AX=ord(ch)),
                        setup=text_setup(), out_regs=()))
    out.append(Case('DrawFontChar space at a page offset and column 37', 'DrawFontChar', 'c_DrawFontChar',
                    regs=dict(AX=0x20), setup=text_setup(char_row=0x0140, col=37, page=0x2000), out_regs=()))
    for dx in (0, 7, 1000, 9999, 12345, 65535):
        out.append(Case(f'PrintDecimalWord {dx}', 'PrintDecimalWord', 'c_PrintDecimalWord',
                        regs=dict(DX=dx), setup=text_setup(char_row=0x0500, col=3), out_regs=()))
    out.append(Case('PrintAsciiz string with spaces and letters', 'PrintAsciiz', 'c_PrintAsciiz',
                    regs=dict(DS=DATA_SEG, SI=TEXT_STR),
                    setup=text_setup(text=b'SCORE 1234 Z\0'), out_regs=('SI',)))
    return out

def palette_setup(level=None, vbyte=None):
    def setup(m):
        m.ints[0x10] = int10_dac          # INT 10h AX=1012h, the DAC block service
        if level is not None:
            m.wb(m.data('CurrentLevel'), level)
        if vbyte is not None:
            m.wb(m.code('OptionVByte'), vbyte, seg=CODE_SEG)
    return setup

def palette_cases():
    return [
        Case('SetLevelPalette level 0 (table pointer 2BFCh)', 'SetLevelPalette', 'c_SetLevelPalette',
             setup=palette_setup(level=0), out_regs=()),
        Case('SetLevelPalette level 4', 'SetLevelPalette', 'c_SetLevelPalette',
             setup=palette_setup(level=4), out_regs=()),
        Case('SetLevelPalette level 10', 'SetLevelPalette', 'c_SetLevelPalette',
             setup=palette_setup(level=10), out_regs=()),
        Case('SetStartupPalette, OptionVByte 1 (/V not set): no DAC write', 'SetStartupPalette',
             'c_SetStartupPalette', setup=palette_setup(vbyte=0xFF), out_regs=()),
        Case('SetStartupPalette, OptionVByte 5: DAC 0..15 from DATA:287Eh', 'SetStartupPalette',
             'c_SetStartupPalette', setup=palette_setup(vbyte=5), out_regs=()),
        Case('SetDacBlock0To15IfFlag from the level table, OptionVByte 5', 'SetDacBlock0To15IfFlag',
             'c_SetDacBlock0To15IfFlag', regs=dict(ES=DATA_SEG, DX=0x2BFC), setup=palette_setup(vbyte=5),
             out_regs=()),
    ]

# -- timer vectors (cs:177Fh) ----------------------------------------------------------------
def swap_setup(seed):
    def setup(m):
        rng = random.Random(seed)
        m.write(0, 0x20, bytes(rng.randrange(256) for _ in range(0x20)))   # IVT 08h..09h and the rest
        m.write(0, 0x0C, bytes([0x34, 0x12, 0x78, 0x56]))                    # INT 03h
    return setup

def swap_cases():
    return [
        Case('SwapInt08Int09Vectors: install (slot holds the game handlers)', 'SwapInt08Int09Vectors',
             'c_SwapInt08Int09Vectors', setup=swap_setup(1), out_regs=(), ranges=[IVT]),
        Case('SwapInt08Int09Vectors: restore (second call, other IVT contents)', 'SwapInt08Int09Vectors',
             'c_SwapInt08Int09Vectors', setup=swap_setup(2), out_regs=(), ranges=[IVT]),
    ]

# -- input polling (cs:0DC1h) --------------------------------------------------------------
def joy_setup(joy_shift=0, ctl=0):
    def setup(m):
        dvar(m, 'word_27DC', 0x0100, size=2)
        dvar(m, 'word_27E2', 0x0180, size=2)
        dvar(m, 'word_27DE', 0x0100, size=2)
        dvar(m, 'word_27E0', 0x0180, size=2)
        dvar(m, 'word_27DA', 0x0004, size=2)
        dvar(m, 'byte_27E4', joy_shift)
        dvar(m, 'byte_27D9', ctl)
    return setup

def kb_setup(seed, cursor=0, level=2, log=b'', lastscan=0, demo=0, extra=None):
    """Keyboard-derived flag groups (random bits), the demo log at DATA:003Fh and cursor."""
    def setup(m):
        rng = random.Random(seed)
        for off in (0x2804, 0x2810, 0x2812, 0x281A, 0x281B, 0x281D, 0x281F, 0x282D, 0x2840,
                    0x2841, 0x2842, 0x2844, 0x284B, 0x2868, 0x2870, 0x286D, 0x286E, 0x286F,
                    0x283C, 0x283E, 0x283F):
            m.wb(off, rng.choice((0, 0, 0xFF)))
        m.wb(m.data('byte_2832'), 0)
        m.wb(m.data('CurrentLevel'), level)
        m.wb(m.data('LastScanCode'), lastscan)
        m.wb(m.data('byte_2879'), demo)
        m.ww(m.data('word_287A'), cursor)
        if log:
            m.write(DATA_SEG, 0x3F, log)
        if extra is not None:
            extra(m)
    return setup

def poll_cases():
    out = []
    # live sampling: joystick buttons pressed / released, no joystick, recording the frame
    out.append(Case('live: joystick at 00h (buttons pressed), record at cursor 0', 'PollInputState',
                    'c_PollInputState', setup=_chain(joy_setup(0), kb_setup(3)),
                    port_inputs={0x201: 0x00}, out_regs=()))
    out.append(Case('live: joystick at 30h (buttons released), record at cursor 0', 'PollInputState',
                    'c_PollInputState', setup=_chain(joy_setup(0), kb_setup(4)),
                    port_inputs={0x201: 0x30}, out_regs=()))
    out.append(Case('live: line bit set at start: no joystick sample', 'PollInputState', 'c_PollInputState',
                    setup=_chain(joy_setup(0), kb_setup(5)), port_inputs={0x201: 0x01}, out_regs=()))
    out.append(Case('live: byte 27E4 FFh (no joystick)', 'PollInputState', 'c_PollInputState',
                    setup=_chain(joy_setup(0xFF), kb_setup(6)), port_inputs={0x201: 0x00}, out_regs=()))
    out.append(Case('live: joystick routine disabled (27D9 = 1)', 'PollInputState', 'c_PollInputState',
                    setup=_chain(joy_setup(0, ctl=1), kb_setup(7)), port_inputs={0x201: 0x00}, out_regs=()))
    out.append(Case('live: same frame as the previous entry, count 3 -> 4', 'PollInputState',
                    'c_PollInputState', setup=_chain(joy_setup(0), kb_setup(8, cursor=4,
                        log=bytes([0x35, 0x00, 0x35, 0x03]))), port_inputs={0x201: 0x00}, out_regs=()))
    out.append(Case('live: previous count FFh -> new entry at cursor 4', 'PollInputState',
                    'c_PollInputState', setup=_chain(joy_setup(0), kb_setup(9, cursor=4,
                        log=bytes([0x35, 0x00, 0x35, 0xFF]))), port_inputs={0x201: 0x00}, out_regs=()))
    out.append(Case('live: previous frame differs -> new entry at cursor 4', 'PollInputState',
                    'c_PollInputState', setup=_chain(joy_setup(0), kb_setup(10, cursor=4,
                        log=bytes([0x0F, 0x00, 0x35, 0x03]))), port_inputs={0x201: 0x30}, out_regs=()))
    out.append(Case('live: log full (cursor 7FCh): nothing recorded', 'PollInputState', 'c_PollInputState',
                    setup=_chain(joy_setup(0), kb_setup(11, cursor=0x07FC)), port_inputs={0x201: 0x00},
                    out_regs=()))
    out.append(Case('live: cursor 7FAh: the last entry fits', 'PollInputState', 'c_PollInputState',
                    setup=_chain(joy_setup(0), kb_setup(12, cursor=0x07FA)), port_inputs={0x201: 0x00},
                    out_regs=()))
    out.append(Case('live: joystick counts at zero, level 7 stored at cursor 0', 'PollInputState',
                    'c_PollInputState', setup=_chain(joy_setup(0), kb_setup(13, level=7)),
                    port_inputs={0x201: 0x30}, out_regs=()))
    # demo playback: one (value, count) entry per call, bit order as the decoder reads it
    out.append(Case('playback: entry 2Ah count 1 (bits 1, 3, 5)', 'PollInputState', 'c_PollInputState',
                    setup=kb_setup(20, demo=1, log=bytes([0x2A, 0x01])), out_regs=()))
    out.append(Case('playback: end marker 55AAh is taken as an entry and sets EndSignal', 'PollInputState',
                    'c_PollInputState', setup=kb_setup(21, demo=1, cursor=2, log=bytes([0xFF, 0x00, 0xAA, 0x55])),
                    out_regs=()))
    out.append(Case('playback: LastScanCode nonzero sets EndSignal', 'PollInputState', 'c_PollInputState',
                    setup=kb_setup(22, demo=1, lastscan=0x1C, log=bytes([0x01, 0x00])), out_regs=()))
    out.append(Case('playback with byte_2879 = 2: decodes the entry, then live sampling continues',
                    'PollInputState', 'c_PollInputState',
                    setup=kb_setup(23, demo=2, log=bytes([0x03, 0x00])), port_inputs={0x201: 0x00}, out_regs=()))
    return out

def _chain(a, b):
    def setup(m):
        a(m); b(m)
    return setup

def wait_cases():
    # playback: frames 0,0,0 (no press), press (bit 4), then a release frame ends the wait
    log = bytes([0x00, 0x02, 0x10, 0x00, 0x00, 0x01])
    return [
        Case('WaitForButtonPressRelease: press then release from the demo log', 'WaitForButtonPressRelease',
             'c_WaitForButtonPressRelease', setup=kb_setup(30, demo=1, log=log), out_regs=()),
        Case('WaitForButtonPressRelease: two frames, press and release, then done', 'WaitForButtonPressRelease',
             'c_WaitForButtonPressRelease', setup=kb_setup(31, demo=1, log=bytes([0x10, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00])),
             out_regs=()),
    ]

def joy_read_case(title, preset, shift=0, ctl=0, ok=True):
    """ok=False: CF is set. The ASM contract leaves AX/BX undefined then; the one thing
    that changes is AL (the port byte last read, shifted), which no caller uses, so only CF
    is compared."""
    def setup(m):
        dvar(m, 'byte_27E4', shift)
        dvar(m, 'byte_27D9', ctl)
        dvar(m, 'word_27DA', 0x0004, size=2)
        dvar(m, 'byte_27E8', 0)
        dvar(m, 'byte_27E9', 0)
    return Case(title, 'ReadJoystickTimes', 'c_ReadJoystickTimes', regs=dict(AX=0x1234, BX=0x5678),
                setup=setup, port_inputs={0x201: preset}, out_regs=('AX', 'BX', 'CF') if ok else ('CF',))

def joy_read_cases():
    out = [
        joy_read_case('joystick reads 00h: CF clear, both buttons pressed', 0x00),
        joy_read_case('joystick reads 30h: CF clear, buttons released', 0x30),
        joy_read_case('lines high at start (bit 0): CF set', 0x01, ok=False),
        joy_read_case('bit 2 line high after the shift (cl = 2): CF set', 0x04, shift=2, ok=False),
        joy_read_case('shift count 22h is masked to 2 (8086 rule): CF set', 0x04, shift=0x22, ok=False),
        joy_read_case('byte 27E4 FFh (no joystick): CF set', 0x00, shift=0x80, ok=False),
        joy_read_case('joystick routine disabled (27D9 = 1): CF set', 0x00, ctl=1, ok=False),
        joy_read_case('button bits after shift 3 (value 6): CF set', 0x30, shift=3, ok=False),
    ]
    return out

# -- keyboard interrupt (cs:1823h) -------------------------------------------------------
def kbd_setup(table_value=None, dos_active=0, sb=0, adlib=0, soundstate=0):
    def setup(m):
        m.ww(0x200, CODE_SEG, seg=STACK_SEG)       # IRET frame: CS and FLAGS (IP is the call's)
        m.ww(0x202, 0x0202, seg=STACK_SEG)
        dvar(m, 'DosCallActive', dos_active)
        cvar(m, 'SbDetected', sb)
        cvar(m, 'AdLibEnabled', adlib)
        cvar(m, 'SoundState', soundstate)
        if table_value is not None:
            for code, v in table_value.items():
                m.wb(m.data('KeyDownTable') + code, v)
    return setup

def kbd_case(title, code, ctl=0x30, table_value=None, quit_=False, **kw):
    def setup(m):
        arm_quit_boundary(m, quit_)
        kbd_setup(table_value, **kw)(m)
    return Case(title, 'KeyboardIsr', 'c_KeyboardIsr', setup=setup,
                port_inputs={0x60: code, 0x61: ctl}, out_regs=('ZF',))

def kbd_cases():
    out = [
        kbd_case('make A (1Eh): table entry FFh, EOI', 0x1E),
        kbd_case('break A (9Eh): table entry 00h', 0x9E, table_value={0x1E: 0xFF}),
        kbd_case('break of ESC (81h): table entry 1 cleared, not a quit', 0x81, table_value={1: 0xFF}),
        kbd_case('scan 0FFh break: table index 7Fh cleared', 0xFF, table_value={0x7F: 0xFF}),
        kbd_case('ESC while in a DOS call: table entry 1 set, not a quit', 0x01, dos_active=1),
        kbd_case('ESC outside DOS calls: quit boundary (SYS_QUIT, ZF set)', 0x01, quit_=True),
        kbd_case('F3 make, no sound card: sound on, port 61h bits 0 and 1', 0x3D, ctl=0x30),
        kbd_case('F3 make with sound on: sound off, port 61h bits 0 and 1 cleared', 0x3D, ctl=0x33,
                 soundstate=1),
        kbd_case('F3 make, AdLib: no port 61h change', 0x3D, adlib=1),
        kbd_case('F3 make with Sound Blaster and sound on: turns off, no block start', 0x3D, sb=1,
                 soundstate=1),
    ]
    return out

def cases_all():
    return (sfx_cases() + resource_cases() + text_cases() + palette_cases() + swap_cases() +
            poll_cases() + wait_cases() + joy_read_cases() + kbd_cases())

CASES = cases_all()


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
    ('PageInResource', '', 6), ('ActivateResourceSlot', '', 12),
    ('SetLevelPalette', '', 3), ('SetStartupPalette', '', 2),
    ('SwapInt08Int09Vectors', '', 2), ('DrawFontChar', '', 2), ('PrintAsciiz', '', 1), ('PrintDecimalWord', '', 1),
    ('PollInputState', '', 3), ('WaitForButtonPressRelease', '', 2), ('StartSoundEffect', '', 2),
    ('KeyboardIsr', '', 4), ('ReadJoystickTimes', '', 1), ('ResourceNameAt', '', 1)])
