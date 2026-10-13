"""Long lockstep runs of MainFrameLoop on real level data with a per-frame input script.

The same script runs at the head of every frame on both sides (the oracle at MainFrameLoop,
the C through the pre2_test_frame hook of c_MainFrameLoop): it holds and releases the
movement, jump and fire keys pseudo-randomly (seeded), so the player walks, jumps, lands,
attacks, takes hits and scrolls the camera over hundreds of frames. A difference in any
frame (collision, camera, objects, rendering) propagates to the compared end state.

Cases: the gorilla boss (level 2) from a checkpoint next to the arena with low energy, so
the fight, the defeat and its drop run; the quake level after it (0Dh, continuation of
stage 3); and long walks on other levels with distinct terrain.
"""
import ctypes, random, re, sys
from difftest import Case
from common import ROOT
from emu import DATA_SEG, CODE_SEG
from unicorn import UC_HOOK_CODE
sys.path.insert(0, str(ROOT/'tests'))
import game as G

KEYS = (0x4B, 0x4D, 0x48, 0x50, 0x39)          # left, right, up (jump), down, space (fire)
_kdt = int(re.search(r'#define DS_KeyDownTable 0x([0-9A-F]+)u', (ROOT/'c'/'pre2_sym.h').read_text()).group(1), 16)

def script(seed, n, bias_right=0.55):
    """Per-frame key states: each key flips with a small probability; right is held more."""
    rng = random.Random(seed)
    held = {k: False for k in KEYS}
    out = []
    for _ in range(n + 1):
        for k in KEYS:
            p = 0.08 if k != 0x4D else (0.03 if held[k] else 0.12 * bias_right * 2)
            if k == 0x4B: p = 0.03 if not held[k] else 0.15
            if rng.random() < p: held[k] = not held[k]
        if held[0x4B] and held[0x4D]: held[0x4B] = False
        out.append(tuple(held[k] for k in KEYS))
    return out

def oracle_script(m, frames):
    st = {'n': 0}
    kdt = m.data('KeyDownTable')
    def hook(uc, address, size, user):
        keys = frames[min(st['n'], len(frames) - 1)]; st['n'] += 1
        for k, down in zip(KEYS, keys): m.wb(kdt + k, 0xFF if down else 0)
    a = CODE_SEG * 16 + m.code('MainFrameLoop')
    m._stop_hooks = getattr(m, '_stop_hooks', []) + [m.uc.hook_add(UC_HOOK_CODE, hook, None, a, a)]

def c_script(frames):
    def setup(d):
        mem = d.pre2_memory().contents
        st = {'n': 0}
        @ctypes.CFUNCTYPE(None)
        def frame():
            keys = frames[min(st['n'], len(frames) - 1)]; st['n'] += 1
            for k, down in zip(KEYS, keys): mem[DATA_SEG * 16 + _kdt + k] = 0xFF if down else 0
        ctypes.c_void_p.in_dll(d, 'pre2_test_frame').value = ctypes.cast(frame, ctypes.c_void_p).value
        return frame
    return setup

def lockstep(title, level, n, seed, state=None):
    frames = script(seed, n)
    base = G.frames_setup(level, n)
    def s(m):
        base(m)
        m.wb(m.data('Lives'), 9)            # the boot-lite state has none: a death would end in the
        if state: state(m)                  # game-over screen, whose loop waits for the timer interrupt
        oracle_script(m, frames)
    return Case(f'lockstep: {title} ({n} frames, seed {seed})', 'MainFrameLoop', 'c_MainFrameLoop',
                regs={'CX': n}, setup=s, files=True, budget=n * 12_000_000, tick_model=True,
                port_inputs={0x201: 0xFF}, c_setup=c_script(frames))

def fight_script(seed, n):
    """Walk right in long holds, jump now and then, fire in short pulses (a held fire key keeps
    the player in the attack state)."""
    rng = random.Random(seed)
    out, right, left, up = [], True, False, False
    for f in range(n + 1):
        if rng.random() < 0.02: right = not right
        left = (not right) and rng.random() < 0.3
        up = rng.random() < 0.04 or (up and rng.random() < 0.6)
        fire = f % rng.choice((18, 24, 30)) in (0, 1)
        out.append((left, right, up, False, fire))
    return out

def boss_case(title, n, seed, x, y, energy):
    """The gorilla (level 2): the player placed before its arena, the view rebuilt by the
    original's EnterLevelView (it scrolls the camera from the origin to the player, as a level
    start does), lives and a lowered energy so the fight, the defeat and the drop fall inside
    the compared frames."""
    frames = fight_script(seed, n)
    base = G.frames_setup(2, n)
    def s(m):
        base(m)
        m.ww(m.data('PlayerX'), x); m.ww(m.data('PlayerY'), y); m.ww(m.data('PlayerVelY'), 0)
        m.wb(m.data('Lives'), 5); m.wb(m.data('BossEnergy'), energy)
        m.call('EnterLevelView', budget=400_000_000)
        tick = m.rw(m.code('TimerTickCount'), seg=CODE_SEG)
        m.ww(m.data('word_27EE'), (tick - 3) & 0xFFFF)
        oracle_script(m, frames)
    return Case(f'lockstep: {title} ({n} frames, seed {seed})', 'MainFrameLoop', 'c_MainFrameLoop',
                regs={'CX': n}, setup=s, files=True, budget=n * 12_000_000, tick_model=True,
                port_inputs={0x201: 0xFF}, c_setup=c_script(frames))

CASES = []
for seed, energy, x in ((1, 2, 0x0C80), (2, 1, 0x0C80), (3, 5, 0x0C40), (4, 3, 0x0CC0)):
    CASES.append(boss_case(f'gorilla boss (level 2): fight from X {x:X}h, energy {energy}, defeat and drop',
                           600, seed, x, 0x0280, energy))
for seed in (11, 12, 13):
    CASES.append(lockstep('quake level 0Dh from its start', 0x0D, 300, seed))
for level, seed in ((0, 21), (3, 22), (4, 23), (5, 24), (6, 25), (7, 26), (8, 27), (0x0E, 28), (0x0F, 29), (1, 30)):
    CASES.append(lockstep(f'level {level:X}h from its start', level, 250, seed))

# ---- the quake level (0Dh, after the gorilla): from each proximity trigger, running right ----
def run_right_script(seed, n, jump_rate):
    rng = random.Random(seed)
    out, up = [], False
    for f in range(n + 1):
        up = (rng.random() < jump_rate) or (up and rng.random() < 0.7)
        out.append((False, True, up, False, rng.random() < 0.03))
    return out

def placed_case(title, level, n, seed, x, y, jump_rate, lives=9):
    """Player placed at (x, y) with the view rebuilt by the original's EnterLevelView, then
    running right (jumping at jump_rate) for n frames."""
    frames = run_right_script(seed, n, jump_rate)
    base = G.frames_setup(level, n)
    def s(m):
        base(m)
        m.ww(m.data('PlayerX'), x); m.ww(m.data('PlayerY'), y); m.ww(m.data('PlayerVelY'), 0)
        m.wb(m.data('Lives'), lives)
        m.call('EnterLevelView', budget=400_000_000)
        tick = m.rw(m.code('TimerTickCount'), seg=CODE_SEG)
        m.ww(m.data('word_27EE'), (tick - 3) & 0xFFFF)
        oracle_script(m, frames)
    return Case(f'lockstep: {title} ({n} frames, seed {seed})', 'MainFrameLoop', 'c_MainFrameLoop',
                regs={'CX': n}, setup=s, files=True, budget=n * 15_000_000, tick_model=True,
                port_inputs={0x201: 0xFF}, c_setup=c_script(frames))

for (x, y, which), seed, jump in (((0x320, 0x130, 1), 41, 0.0), ((0x320, 0x130, 1), 42, 0.05),
                                  ((0x300, 0x120, 1), 43, 0.03), ((0x400, 0x130, 2), 44, 0.0),
                                  ((0x3F0, 0x120, 2), 45, 0.05), ((0xB90, 0x0F0, 3), 46, 0.0),
                                  ((0xB70, 0x0E0, 3), 47, 0.04)):
    CASES.append(placed_case(f'quake level 0Dh: trigger {which} at X {x:X}h, running right, jumps {jump}',
                             0x0D, 300, seed, x, y, jump))
