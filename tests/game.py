"""Top level (c/game.c) vs the original: level start (loc_01AB .. MainFrameLoop) and runs of
MainFrameLoop iterations from a real started level.

Boot-lite state, produced by the oracle itself: memory top, free paragraph pointer, the
ALLFONTS sheet in word_003D (the HUD/font graphics loaded at startup), the sprite sheet
(LoadSpriteSheet), word_0039, every resource slot set to disk loading (bit 02h cleared, as
InitResourceMemory does without /L), sound off (SoundState 0, no SB). The level start case
enters at loc_01AB with the ES the original would hold (A000h on the CurrentLevel < 0Ah
path, after ShowWorldMapScrollIn) and stops at MainFrameLoop. The frame cases start from a
cached level start, preset the timer so each PresentFrame's tick wait ends at once, and
stop at the loop's back edge after N iterations (the C entry runs N MainFrameSteps).
"""
from difftest import Case
from common import ROOT
from emu import DATA_SEG, CODE_SEG, DosFiles
from unicorn import UC_HOOK_CODE
import sys
sys.path.insert(0, str(ROOT/'tests'))
import render as R

def boot(m):
    R.base(m)
    case_dos = m.ints.get(0x21)
    m.ints[0x21] = DosFiles()             # boot traffic is setup, not part of the case
    m.ww(m.data('NextFreeSegment'), R.FREE)
    r = m.call('LoadSqzFile', {'DX': 0x0C}, budget=200_000_000)        # KEYB.SQZ: the demo log seed
    for i in range(0x800): m.wb(0x3F + i, m.rb(i, seg=r['AX']))         # ds:003F, as ProgramEntry
    m.ww(m.data('NextFreeSegment'), R.FREE)
    r = m.call('LoadSqzFile', {'DX': 0x15}, budget=200_000_000)        # ALLFONTS.SQZ
    m.ww(m.data('word_003D'), r['AX'])
    m.call('LoadSpriteSheet', budget=300_000_000)
    m.ww(m.data('word_0039'), m.rw(m.data('NextFreeSegment')))
    flags = m.data('ResourceFlags')
    for i in range(18):
        m.wb(flags + i, m.rb(flags + i) & ~0x02)
    m.wb(m.code('SoundState'), 0, seg=CODE_SEG)
    m.wb(m.code('SbDetected'), 0, seg=CODE_SEG)
    m.ints[0x21] = case_dos
    m.ports.log.clear()

_boot = {}
def booted(m):
    R.base(m)                             # BIOS handlers for this case
    if 'mem' not in _boot:
        boot(m)
        _boot['mem'] = (m.snapshot(), R.vga_bytes(m))
    mem, vga = _boot['mem']
    m.restore(mem); R.vga_restore(m, vga); m.vga.vga_set_retrace(4)

def stop_after(m, where, hits=1):
    """Stop the oracle the HITS-th time it reaches WHERE (label or code offset)."""
    addr = CODE_SEG * 16 + m.code(where)
    state = {'n': 0}
    def hook(uc, address, size, user):
        state['n'] += 1
        if state['n'] >= hits:
            m.setreg('CS', CODE_SEG); m.setreg('IP', 0); uc.emu_stop()
    h = m.uc.hook_add(UC_HOOK_CODE, hook, None, addr, addr)
    m._stop_hooks = getattr(m, '_stop_hooks', []) + [h]

def clear_stops(m):
    for h in getattr(m, '_stop_hooks', []): m.uc.hook_del(h)
    m._stop_hooks = []

def level_setup(level, es):
    def s(m):
        clear_stops(m)
        booted(m)
        m.wb(m.data('CurrentLevel'), level)
        m.wb(m.data('byte_B198'), 0)
        tick = m.rw(m.code('TimerTickCount'), seg=CODE_SEG)
        m.ww(m.data('word_27EE'), (tick - 3) & 0xFFFF)          # PresentFrame's tick wait ends at once
        stop_after(m, 'MainFrameLoop')
    return s

CASES = []
for level in (0, 3, 7):
    CASES.append(Case(f'level start, level {level} (ES = A000h: the slot fill goes to VRAM)', 'loc_01AB',
                      'c_loc_01AB', regs={'ES': 0xA000}, setup=level_setup(level, 0xA000), files=True,
                      budget=400_000_000, tick_model=True, port_inputs={0x201: 0xFF}))
for level, es in ((0x0B, DATA_SEG), (0x0C, 0xA000)):
    CASES.append(Case(f'level start, level {level:X}h with ES = {es:04X}h', 'loc_01AB', 'c_loc_01AB',
                      regs={'ES': es}, setup=level_setup(level, es), files=True, budget=400_000_000,
                      tick_model=True, port_inputs={0x201: 0xFF}))

_started = {}
def started(m, level):
    R.base(m)
    if level not in _started:
        clear_stops(m)
        booted(m)
        m.wb(m.data('CurrentLevel'), level)
        tick = m.rw(m.code('TimerTickCount'), seg=CODE_SEG)
        m.ww(m.data('word_27EE'), (tick - 3) & 0xFFFF)
        stop_after(m, 'MainFrameLoop')
        case_dos = m.ints.get(0x21); m.ints[0x21] = DosFiles()
        m.call('loc_01AB', {'ES': 0xA000}, budget=400_000_000)
        m.ints[0x21] = case_dos
        clear_stops(m)
        m.ports.log.clear()
        _started[level] = (m.snapshot(), R.vga_bytes(m))
    mem, vga = _started[level]
    m.restore(mem); R.vga_restore(m, vga); m.vga.vga_set_retrace(4)

def frames_setup(level, n, keys=()):
    back_edge = None
    def s(m):
        clear_stops(m)
        started(m, level)
        tick = m.rw(m.code('TimerTickCount'), seg=CODE_SEG)
        m.ww(m.data('word_27EE'), (tick - 3) & 0xFFFF)          # first frame wait ends at once
        for k in keys: m.wb(m.data('KeyDownTable') + k, 0xFF)
        stop_after(m, m.code('loc_0261') + 15, n)              # the jmp short MainFrameLoop
        stop_after(m, 'LevelRestart')
    return s

for level in (0, 3):
    for n in (1, 3):
        CASES.append(Case(f'main loop: level {level}, {n} frame(s), no input', 'MainFrameLoop', 'c_MainFrameLoop',
                          regs={'CX': n}, setup=frames_setup(level, n), files=True, budget=600_000_000,
                          tick_model=True, port_inputs={0x201: 0xFF}))

# held keys (scan codes in KeyDownTable): 4Dh right arrow, 48h up arrow (jump), 39h space
for level, n, keys, what in ((0, 10, (0x4D,), 'right held'), (3, 8, (0x4D, 0x48), 'right + up held'),
                             (0, 6, (0x39,), 'space held')):
    CASES.append(Case(f'main loop: level {level}, {n} frames, {what}', 'MainFrameLoop', 'c_MainFrameLoop',
                      regs={'CX': n}, setup=frames_setup(level, n, keys), files=True, budget=900_000_000,
                      tick_model=True, port_inputs={0x201: 0xFF}))

# Composed transitions: a frame whose UpdateLevelState requests a restart (CF = 1), then
# LevelRestart with its AX, up to the next level's first frame (the oracle stops when it
# reaches MainFrameLoop again; the C runs MainFrameStep, then restart_to_level). The ES the
# frame leaves is what the render-slot fill at loc_01C4 uses on the CurrentLevel >= 0Ah paths.
def restart_setup(level, **state):
    def s(m):
        clear_stops(m)
        started(m, level)
        tick = m.rw(m.code('TimerTickCount'), seg=CODE_SEG)
        m.ww(m.data('word_27EE'), (tick - 3) & 0xFFFF)
        for name, v in state.items(): m.wb(m.data(name), v)
        stop_after(m, 'MainFrameLoop', 2)      # the entry is the first hit
        stop_after(m, 'RunFrontEnd')           # AX = 1 paths are not part of these cases
    return s

for level, what, state in (
        (3, 'warp from level 3 into bonus level 0Ah', {'LevelEndMode': 2}),
        (1, 'warp from level 1 into bonus level 0Ch', {'LevelEndMode': 2}),
        (2, 'warp from level 2 into bonus level 0Bh', {'LevelEndMode': 2}),
        (9, 'warp from level 9 into bonus level 0Eh', {'LevelEndMode': 2}),
        (6, 'warp from level 6 to level 0 (world map, level 0 start)', {'LevelEndMode': 2}),
        (0x0A, 'bonus level 0Ah ends (mode 1): next id 0Bh', {'LevelEndMode': 1}),
        (0x0B, 'bonus level 0Bh warp exit (mode 2): back to level 2, its exit sequence and map',
         {'LevelEndMode': 2}),
        (3, 'checkpoint restart on level 3', {'RespawnState': 1, 'EndSignal': 0}),
        (0x0B, 'checkpoint restart in bonus level 0Bh', {'RespawnState': 1, 'EndSignal': 0})):
    CASES.append(Case(f'restart: {what}', 'MainFrameLoop', 'c_MainFrameLoopRestart', regs={'CX': 1},
                      setup=restart_setup(level, **state), files=True, budget=3_000_000_000,
                      tick_model=True, port_inputs={0x201: 0xFF}))

# ---- scripted button presses ---------------------------------------------------------------
# WaitForButtonPressRelease polls the input until a press, then until the release; the C loop
# reaches platform_poll() right before each PollInputState call, the original executes its
# `call PollInputState` at loc_0BC3 / loc_0BCF. The same script runs at those points on both
# sides: alternately press and release RETURN (KeyDownTable[1Ch]), so every wait ends after
# one press and one release.
import ctypes
RETURN_KEY = 0x1C

def press_release_oracle(m):
    st = {'n': 0}
    def hook(uc, address, size, user):
        st['n'] += 1
        m.wb(m.data('KeyDownTable') + RETURN_KEY, 0xFF if st['n'] % 2 else 0)
    for where in ('loc_0BC3', 'loc_0BCF'):
        a = CODE_SEG * 16 + m.code(where)
        m._stop_hooks = getattr(m, '_stop_hooks', []) + [m.uc.hook_add(UC_HOOK_CODE, hook, None, a, a)]

def press_release_c(d):
    from difftest import DATA_SEG as DS
    mem = d.pre2_memory().contents
    import re
    sym = (ROOT/'c'/'pre2_sym.h').read_text()
    kdt = int(re.search(r'#define DS_KeyDownTable 0x([0-9A-F]+)u', sym).group(1), 16)
    st = {'n': 0}
    @ctypes.CFUNCTYPE(None)
    def poll():
        st['n'] += 1
        mem[DS * 16 + kdt + RETURN_KEY] = 0xFF if st['n'] % 2 else 0
    ctypes.c_void_p.in_dll(d, 'pre2_test_poll').value = ctypes.cast(poll, ctypes.c_void_p).value
    return poll                                  # kept alive for the call

# ---- every level: the level start and three frames ----------------------------------------
for level in (1, 2, 4, 5, 6, 8, 9, 0x0A, 0x0D, 0x0E, 0x0F):
    CASES.append(Case(f'level start, level {level:X}h (ES = A000h)', 'loc_01AB', 'c_loc_01AB',
                      regs={'ES': 0xA000}, setup=level_setup(level, 0xA000), files=True,
                      budget=400_000_000, tick_model=True, port_inputs={0x201: 0xFF}))
for level in (1, 2, 4, 5, 6, 7, 8, 9, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F):
    CASES.append(Case(f'main loop: level {level:X}h, 3 frames, no input', 'MainFrameLoop', 'c_MainFrameLoop',
                      regs={'CX': 3}, setup=frames_setup(level, 3), files=True, budget=900_000_000,
                      tick_model=True, port_inputs={0x201: 0xFF}))

# ---- the ending, game over and the CASTLE picture (up to the front end) --------------------
def restart_keys_setup(level, held=(), **state):
    """restart_setup plus the scripted button presses; `held`: scan codes down from the start
    (the game-over screen's loop ends on the action flag or a timer the tests do not run)."""
    base = restart_setup(level, **state)
    def s(m):
        base(m)
        for k in held: m.wb(m.data('KeyDownTable') + k, 0xFF)
        press_release_oracle(m)
    return s

for level, what, state in (
        (0x0E, 'ending: EndSignal FFh in level 0Eh (THEEND, intro picture gate, front end)', {'EndSignal': 0xFF}),
        (0, 'game over: EndSignal 1 in level 0 (death bounce, game-over screen left by RETURN, front end)',
         {'EndSignal': 1, 'held': (RETURN_KEY,)}),
        (0x0B, 'game over in bonus level 0Bh', {'EndSignal': 1, 'held': (RETURN_KEY,)}),
        (7, 'beginner reaches level 8: exit of level 7, CASTLE picture, front end', {'LevelEndMode': 1, 'byte_B198': 0})):
    CASES.append(Case(f'restart: {what}', 'MainFrameLoop', 'c_MainFrameLoopRestart', regs={'CX': 1},
                      setup=restart_keys_setup(level, **state), files=True, budget=4_000_000_000,
                      tick_model=True, port_inputs={0x201: 0xFF}, c_setup=press_release_c))

# ---- the command-line switches (ProgramEntry up to loc_007E) -------------------------------
PSP = 0x1000
def options_setup(tail):
    def s(m):
        clear_stops(m)
        m.wb(0x80, len(tail), seg=PSP)
        for i, ch in enumerate(tail.encode()): m.wb(0x81 + i, ch, seg=PSP)
        stop_after(m, 'loc_007E')
    return s
for tail in ('', ' /F', ' /m', ' /A', ' /E', ' /X', ' /L', ' /D', ' /B', ' /V', ' /Z', ' /E /M', ' /V /F',
             ' /v/x', 'F', ' x/L', ' /V /V /B', ' /'):
    CASES.append(Case(f'command line {tail!r}', 'ProgramEntry', 'c_parse_options', regs={'DS': PSP, 'ES': PSP},
                      setup=options_setup(tail)))

# ---- demo playback (the attract mode's gameplay) --------------------------------------------
def demo_frames_setup(level, n, expert):
    base = frames_setup(level, n)
    def s(m):
        base(m)
        m.wb(m.data('byte_2879'), 1); m.ww(m.data('word_287A'), 0)
        m.wb(m.data('byte_287C'), 0); m.wb(m.data('byte_287D'), 0); m.wb(m.data('byte_B198'), expert)
    return s
CASES.append(Case('demo playback: level 0 expert from the KEYB log, 12 frames', 'MainFrameLoop', 'c_MainFrameLoop',
                  regs={'CX': 12}, setup=demo_frames_setup(0, 12, 1), files=True, budget=1_500_000_000,
                  tick_model=True, port_inputs={0x201: 0xFF}))
CASES.append(Case('restart: demo aborted by a key (LastScanCode): game over without its screen, front end',
                  'MainFrameLoop', 'c_MainFrameLoopRestart', regs={'CX': 1},
                  setup=restart_keys_setup(0, byte_2879=1, LastScanCode=0x1C), files=True, budget=4_000_000_000,
                  tick_model=True, port_inputs={0x201: 0xFF}, c_setup=press_release_c))
