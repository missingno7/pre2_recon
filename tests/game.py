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
