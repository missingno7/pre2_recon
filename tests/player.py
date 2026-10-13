"""Player update, player physics and tile collision (src/MODULE3.ASM cs:5850..6821) vs c/player.c.

State: real level state from the oracle's own LoadLevel (tests/render.py, cached per level),
then the player record, camera, tile class tables and map cells are set per case. A frame's
input is the demo playback of PollInputState (byte 2879h = 1): one word per frame at
DATA:word_003F (low byte = input bits, high byte = repeat count), so each 5-bit state index
(27EC<<4 | 27ED<<3 | 27EA<<2 | 27EB<<1 | 27E8) and the 27E9 bit are driven exactly.

Multi-frame cases: setup runs the oracle through the earlier frames on the same machine, so
the compared frame starts from a state the oracle produced; the C side starts from the same
memory snapshot. CX is the caller's leftover register (see c/player.c, attack tail).

The in-game credits (Ctrl+Alt+W, TryShowCredits called by UpdatePlayer) run with the button
wait fed by two playback words (fire down, fire up) before the frame's own word. Not compared:
table targets outside the translated handlers (ProjectileKindHandlers[2..3], table entries past
the translated ones), which return PLAYER_UNSUPPORTED. The random cases keep their tables to
valid classes, so they do not reach those words.
"""
import random, sys
from difftest import Case
from common import ROOT
from emu import CODE_SEG, DATA_SEG
sys.path.insert(0, str(ROOT/'tests'))
import render as RND                      # oracle LoadLevel state (cached) and the BIOS 10h model

LEVELS = (0, 3, 7, 11)                    # 11 is an expert level (10 and up)

# 5-bit state index -> InputAnimTable state (see the table in MODULE1): 0 idle, 1 run,
# 2 jump, 3/6/7 attack, 4 charge, 5 crouch, 8 hurt (by timer, not by input)
IDX_IDLE, IDX_RUN, IDX_JUMP, IDX_CHARGE, IDX_CROUCH = 0, 8, 4, 10, 2
IDX_ATTACK = {3: 1, 6: 5, 7: 3}          # fire alone -> 3; fire+up -> 6; fire+down -> 7

# -- helpers -----------------------------------------------------------------------------
def cell(row, col): return ((row & 0xFF) << 8) | (col & 0xFF)
def W(m, name, v): m.ww(m.data(name), v & 0xFFFF)
def B(m, name, v): m.wb(m.data(name), v & 0xFF)
def lseg(m): return m.rw(m.data('LevelSegment'))

def set_tile(m, tile, ground=None, side=None, flags=None, solid=None):
    if ground is not None: m.wb(m.data('GroundTypeTable') + tile, ground)
    if side is not None: m.wb(m.data('SideTileClassTable') + tile, side)
    if flags is not None: m.wb(m.data('TileFlagsTable') + tile, flags)
    if solid is not None: m.wb(m.data('TileSolidProps') + tile, solid)

def set_cell(m, row, col, tile): m.wb(cell(row, col), tile, seg=lseg(m))
def get_cell(m, row, col): return m.rb(cell(row, col), seg=lseg(m))

def find_floor(m, row0=12, col0=8):
    """First cell (scanning rows then columns from row0, col0) with a ground class under air."""
    gt = m.data('GroundTypeTable')
    for row in range(row0, row0 + 40):
        for col in range(col0, col0 + 200):
            t = get_cell(m, row, col)
            if t and m.rb(gt + t) and not m.rb(gt + get_cell(m, row - 1, col)):
                return row, col
    return row0, col0

def value_of(idx):
    """Playback value of a 5-bit state index: 27ED bit 0, 27EC bit 1, 27EA bit 2, 27EB bit 3, 27E8 bit 4."""
    b = lambda k: (idx >> k) & 1
    return b(3) | (b(4) << 1) | (b(2) << 2) | (b(1) << 3) | (b(0) << 4)

def playback(m, values, counts=None):
    """Feeds one playback word per frame (value | count << 8) from DATA:word_003F."""
    base = m.data('word_003F')
    counts = counts or [0] * len(values)
    for k, v in enumerate(values):
        m.ww(base + 2 * k, (v & 0xFF) | (counts[k] << 8))
    W(m, 'word_287A', 0); B(m, 'byte_287C', 0); B(m, 'byte_287D', 0)
    B(m, 'LastScanCode', 0); B(m, 'byte_2879', 1)

RECORD_FLAGS = ('AirborneFlag', 'PlayerAirFlags', 'PlayerOnPlatform', 'JumpFrame', 'JumpCooldown',
                'FallCounter', 'ChargeLevel', 'AttackRecoveryTimer', 'AttackAnimLock', 'ShakeMagnitude',
                'RespawnState', 'RespawnRequestFlag', 'EndSignal', 'LandingBlockTimer', 'BonusFlashTimer',
                'byte_6BE8', 'StrokeCounter', 'PlayerActionTicks', 'PlayerHurtTimer', 'CurrentAnimId',
                'SubAnimKey', 'ScrollFollowGate', 'MotionShift', 'PlayerAltModeFlag', 'FrameFlagsRaw',
                'SnowFrameTick', 'MomentumCounter', 'BonusLetterMask', 'byte_2830', 'byte_2811',
                'byte_282C', 'byte_2805', 'byte_280D', 'byte_2879', 'GridDirtyFlag', 'AnimVisibleFlag',
                'byte_4F1B')

def clear_player(m, x=0x0A00, y=0x0A00, vx=0, vy=0, facing=1):
    """Default player record, camera, flags and pointers (inputs off until playback())."""
    W(m, 'PlayerX', x); W(m, 'PlayerY', y); W(m, 'PlayerVelX', vx); W(m, 'PlayerVelY', vy)
    W(m, 'PlayerFacing', facing); W(m, 'PlayerFrameWord', 0); W(m, 'StateFrameCounter', 0)
    W(m, 'LastLandY', y); W(m, 'PlayerSprite', 0xFFFF); W(m, 'RenderSlots', x); W(m, 'PlayerRenderY', y)
    W(m, 'PlayerAnimPtr', m.data('AnimSeqPtrTable')); W(m, 'CameraLeftLimit', 0)
    W(m, 'CameraRow', max(0, (y >> 4) - 6)); W(m, 'CameraCol', max(0, (x >> 4) - 10))
    W(m, 'BridgeDipTile', 0x55AA); W(m, 'PopupRingHead', 0x4F76); W(m, 'SnowWind', 0)
    W(m, 'word_6BE2', 0); W(m, 'word_6BB1', m.data('PlayerX')); W(m, 'word_27F0', 0x0123)
    W(m, 'PrevCameraCol', 0x55AA)
    for name in RECORD_FLAGS:
        B(m, name, 0)
    B(m, 'AltMomentumStep', 3); B(m, 'PlayerWeaponType', 3)
    B(m, 'Lives', 3); B(m, 'Energy', 5)

def mk(title, routine, cfunc, setup=None, level=None, regs=None, out=(), es=False, budget=50_000_000,
       files=True):
    """A case: level state (or the zeroed machine), the default record, then setup(m). setup may
    return a dict of register values for this case; es=True passes ES = LevelSegment."""
    regs = dict(regs or {})
    if routine == 'UpdatePlayer': regs.setdefault('CX', 0)     # the caller's CX is an input here
    c = Case(title, routine, cfunc, regs=regs, setup=None, out_regs=out, files=files, budget=budget)

    def s(m):
        if level is not None: RND.level_state(m, level)
        else: RND.base(m)
        clear_player(m)
        extra = setup(m) if setup else None
        if isinstance(extra, dict): c.regs.update(extra)
        if es: c.regs['ES'] = lseg(m)
    c.setup = s
    return c

def ground_at(row, col, tile, cls, solid=0x13, flags=0, side=0):
    """Ground tile `tile` at (row, col), class `cls`, air above; the player stands on it."""
    def f(m):
        set_cell(m, row, col, tile)
        set_cell(m, row - 1, col, 0)
        set_tile(m, tile, ground=cls, side=side, flags=flags, solid=solid)
        set_tile(m, 0, ground=0, side=0, flags=0, solid=0)
        W(m, 'PlayerX', col * 16 + 8); W(m, 'PlayerY', row * 16)
        W(m, 'LastLandY', row * 16)
        W(m, 'CameraRow', max(0, row - 6)); W(m, 'CameraCol', max(0, col - 10))
    return f

def stand_on_floor(m, dy=0):
    """Player on the first real floor of the loaded level, `dy` pixels above its top."""
    row, col = find_floor(m)
    W(m, 'PlayerX', col * 16 + 8); W(m, 'PlayerY', row * 16 - dy)
    W(m, 'LastLandY', row * 16)
    W(m, 'CameraRow', max(0, row - 8)); W(m, 'CameraCol', max(0, col - 10))
    return row, col

def randomize_tables(m, rng):
    """Valid classes for every tile id: ground 0..6, side 0..4, ceiling (flags low nibble) 0..2."""
    for t in range(256):
        set_tile(m, t, ground=rng.choice([0, 0, 1, 1, 2, 3, 4, 5, 6]),
                 side=rng.choice([0, 0, 0, 1, 2, 4]),
                 flags=rng.choice([0, 1, 2]) | rng.choice([0, 0, 0x10, 0x20, 0x40]),
                 solid=rng.choice([0, 0, 0x03, 0x13, 0x21, 0x30, 0x07, 0x18]))

def randomize_map(m, rng, row0, col0):
    for r in range(row0 - 4, row0 + 14):
        for c in range(col0 - 14, col0 + 14):
            set_cell(m, r, c, rng.randrange(256))

def advance(m, frames, cx=0):
    """Oracle UpdatePlayer for `frames` frames (the playback log gives each frame's input)."""
    for _ in range(frames):
        m.call('UpdatePlayer', {'CX': cx}, budget=50_000_000)

# -- suites --------------------------------------------------------------------------------
def combo_cases():
    """Every 5-bit input combination on a real floor, normal table and alt table."""
    cs = []
    for level, alt in [(l, 0) for l in LEVELS] + [(0, 1), (7, 1), (11, 1)]:
        for idx in range(32):
            def setup(m, idx=idx, alt=alt):
                stand_on_floor(m)
                B(m, 'PlayerAltModeFlag', alt)
                W(m, 'PlayerVelX', 0x18 if idx & 1 else 0)
                W(m, 'PlayerVelY', 0x20 if idx & 2 else 0)
                playback(m, [value_of(idx)])
            cs.append(mk(f'UpdatePlayer: level {level} input index {idx:02X} alt {alt}', 'UpdatePlayer',
                         'c_UpdatePlayer', level=level, setup=setup, out=('CX',), regs={'CX': 0x0C0D}))
    return cs

def state_cases():
    """Each handler through its table on a floor and in the air, with speeds, anims and timers."""
    cs = []
    def st(title, idx, level=0, vx=0, vy=0, facing=1, alt=0, air=False, fields=None, cx=0):
        def setup(m):
            row, col = stand_on_floor(m, dy=0x30 if air else 0)
            W(m, 'PlayerVelX', vx); W(m, 'PlayerVelY', vy); W(m, 'PlayerFacing', facing)
            B(m, 'PlayerAltModeFlag', alt)
            for k, v in (fields or {}).items():
                if k.startswith('B:'): B(m, k[2:], v)
                else: W(m, k, v)
            playback(m, [value_of(idx)])
        cs.append(mk(title, 'UpdatePlayer', 'c_UpdatePlayer', level=level, setup=setup, out=('CX',),
                     regs={'CX': cx}))
    # state 0 (idle) and its branches
    st('state 0 idle, speed 0', IDX_IDLE)
    st('state 0 idle, speed 3 (turn anim)', IDX_IDLE, vx=3)
    st('state 0 idle, speed 0x40, facing left (turn anim)', IDX_IDLE, vx=0x40, facing=-1)
    st('state 0 idle, speed 0x40 facing right (run-pose key)', IDX_IDLE, vx=0x40, facing=1)
    st('state 0 idle, ActionTicks 1Eh (anim 10h)', IDX_IDLE, fields={'B:PlayerActionTicks': 0x1E})
    st('state 0 idle, ActionTicks 1Eh with 27E9 (table search)', IDX_IDLE,
       fields={'B:PlayerActionTicks': 0x1E, 'B:byte_27E9': 0})
    st('state 0 look-around pans right (27E9, facing right)', IDX_IDLE, fields={'B:byte_27E9': 1})
    st('state 0 look-around pans left (27E9, frame bit 7)', IDX_IDLE,
       fields={'B:byte_27E9': 1, 'B:PlayerFrameHigh': 0x80, 'word_27F0': 0x0040})
    st('state 0 idle in the air, JumpFrame 5 (drift)', IDX_IDLE, vy=0x30, air=True, fields={'B:JumpFrame': 5})
    st('state 0 idle in the air, JumpFrame 2', IDX_IDLE, vy=0x30, air=True, fields={'B:JumpFrame': 2})
    st('state 0 idle with attack lock (ContinueAttackAnim)', IDX_IDLE,
       fields={'B:AttackAnimLock': 0x40, 'B:CurrentAnimId': 3})
    # state 1 (run) and alt-mode run
    st('state 1 run, speed 0x48', IDX_RUN, vx=0x48)
    st('state 1 run, speed -0x60 (wind floor)', IDX_RUN, vx=-0x60, fields={'SnowWind': 0x0200})
    st('state 1 run, alt mode, stroke 16h (trail at 17h)', IDX_RUN, vx=0x50, alt=1,
       fields={'B:StrokeCounter': 0x16})
    st('state 1 run, attack lock', IDX_RUN, vx=0x10, fields={'B:AttackAnimLock': 0x40})
    # state 2 (jump) and its cooldown and arc frames
    st('state 2 jump, JumpFrame 0', IDX_JUMP, air=True, fields={'B:JumpFrame': 0})
    st('state 2 jump, JumpCooldown set: idle', IDX_JUMP, fields={'B:JumpCooldown': 3, 'B:JumpFrame': 7})
    st('state 2 jump, speed 0x10 (accelerate), frame 3', IDX_JUMP, vx=0x10, fields={'B:JumpFrame': 3})
    st('state 2 jump, speed 0x40 (friction), alt', IDX_JUMP, vx=0x40, alt=1, fields={'B:JumpFrame': 6})
    st('state 2 jump, speed -0x10 (unsigned friction)', IDX_JUMP, vx=-0x10, fields={'B:JumpFrame': 1})
    st('state 2 jump, frame 0FFh (gravity)', IDX_JUMP, vy=0x10, fields={'B:JumpFrame': 0xFF})
    # states 3, 6, 7 (attack) with every weapon, projectile and fall state
    for w in (0, 1, 2, 3):
        st(f'state 3 attack, weapon {w}', IDX_ATTACK[3], fields={'B:PlayerWeaponType': w})
    st('state 3 attack, weapon 2, charged', IDX_ATTACK[3], fields={'B:PlayerWeaponType': 2, 'B:ChargeLevel': 0x20})
    st('state 3 attack, weapon 3, falling (projectile, FallCounter 4)', IDX_ATTACK[3],
       air=True, vy=0x30, fields={'B:FallCounter': 4, 'B:PlayerWeaponType': 3})
    st('state 3 attack, weapon 0, falling (no projectile, FallCounter 6)', IDX_ATTACK[3], air=True,
       vy=0x30, fields={'B:FallCounter': 6, 'B:PlayerWeaponType': 0}, cx=0x1234)
    st('state 3 attack, weapon 0, falling, CX 0', IDX_ATTACK[3], air=True, vy=0x30,
       fields={'B:FallCounter': 6, 'B:PlayerWeaponType': 0}, cx=0)
    st('state 6 attack, weapon 1, frame 0101h (render key)', IDX_ATTACK[6],
       fields={'B:PlayerWeaponType': 1, 'PlayerFrameWord': 0x0101})
    st('state 7 attack, weapon 2, projectile slots full, falling', IDX_ATTACK[7], air=True, vy=0x30,
       fields={'B:PlayerWeaponType': 2, 'B:FallCounter': 2})
    st('state 7 attack, weapon 3, falling, slots 4F2Eh frame set', IDX_ATTACK[7], air=True, vy=0x30,
       fields={'B:PlayerWeaponType': 3, 'B:FallCounter': 2, 'word_4F2E': 0x0401},
       cx=0x0042)
    # state 4 (charge), 5 (crouch)
    st('state 4 charge, speed 0x10', IDX_CHARGE, vx=0x10)
    st('state 4 charge, speed 0x30 (idle tail)', IDX_CHARGE, vx=0x30)
    st('state 5 crouch, speed 0x08', IDX_CROUCH, vx=0x08)
    # alt-mode table: states 0..7 (2 is the bounce; 0, 3..7 are crouch)
    for idx, name in ((IDX_IDLE, 'crouch alt'), (IDX_RUN, 'run alt'), (IDX_JUMP, 'bounce alt, low stroke'),
                      (IDX_CHARGE, 'crouch alt (4)'), (IDX_CROUCH, 'crouch alt (5)'),
                      (IDX_ATTACK[3], 'crouch alt (7)'), (IDX_ATTACK[6], 'crouch alt (6)')):
        st(f'alt mode: {name}', idx, alt=1, vx=0x20, air=True, vy=0x18)
    st('alt bounce: StrokeCounter 18h (bounce, PlayerY -3)', IDX_JUMP, alt=1, air=True,
       fields={'B:StrokeCounter': 0x18})
    st('alt air flag 1: momentum block (up)', IDX_JUMP, alt=1, air=True,
       fields={'B:PlayerAirFlags': 1, 'B:MomentumCounter': 0x30, 'B:AltMomentumStep': 2})
    st('alt air flag 1: momentum block (down, step 0)', IDX_CROUCH, alt=1, air=True,
       fields={'B:PlayerAirFlags': 1, 'B:MomentumCounter': 0x10, 'B:AltMomentumStep': 0}, vx=0x30)
    st('alt air flag 1: momentum (up, step 6, vel 0x40)', IDX_IDLE, alt=1, air=True, vy=0x40,
       fields={'B:PlayerAirFlags': 1, 'B:AltMomentumStep': 6, 'B:MomentumCounter': 0x08})
    st('alt, PlayerVelY 0xB0 sets AirFlags 1', IDX_IDLE, alt=1, air=True, vy=0x0B0, fields={})
    st('alt, AltMomentumStep 4 with counter (VelY 0FFC0h)', IDX_IDLE, alt=1, air=True, vy=0x40,
       fields={'B:PlayerAirFlags': 1, 'B:AltMomentumStep': 4, 'B:MomentumCounter': 0x20, 'B:byte_27EA': 0},
       facing=-1)
    return cs

def hurt_cases():
    """Hurt state (8) through the normal table, for the timer threshold 16h."""
    cs = []
    for ht in (0x15, 0x16, 0xFF):
        def setup(m, ht=ht):
            stand_on_floor(m)
            B(m, 'PlayerHurtTimer', ht); W(m, 'PlayerVelX', 0x20)
            playback(m, [value_of(IDX_IDLE)])
        cs.append(mk(f'hurt timer {ht:02X}: state 8 on the normal table', 'UpdatePlayer', 'c_UpdatePlayer',
                     level=3, setup=setup, out=('CX',)))
    return cs

def random_sequences():
    """Seeded random inputs and valid tile tables over 40 frames; the compared frame is the last."""
    cs = []
    for seed in range(10):
        level = LEVELS[seed % len(LEVELS)]
        def setup(m, seed=seed):
            rng = random.Random(0x5000 + seed)
            row, col = find_floor(m)
            randomize_tables(m, rng)
            randomize_map(m, rng, row, col)
            W(m, 'PlayerX', col * 16 + 8); W(m, 'PlayerY', row * 16 - 0x20)
            W(m, 'CameraRow', max(0, row - 8)); W(m, 'CameraCol', max(0, col - 10))
            W(m, 'SnowWind', rng.choice([0, 0x0040, 0x0200]))
            B(m, 'SnowFrameTick', rng.randrange(256))
            B(m, 'PlayerAltModeFlag', seed & 1)
            B(m, 'PlayerWeaponType', seed % 4)
            playback(m, [rng.randrange(32) for _ in range(40)])
            advance(m, 39, cx=rng.randrange(0x10000))
        cs.append(mk(f'random sequence {seed} (40 frames, level {level})', 'UpdatePlayer', 'c_UpdatePlayer',
                     level=level, setup=setup, out=('CX',), regs={'CX': 0x3A3A}))
    return cs

def physics_cases():
    """Landings on each ground class, the bridge dip, walls, ceilings, edges, respawn."""
    cs = []
    for cls in range(7):
        for vy in (0, 0x40, -0x20):
            def setup(m, cls=cls, vy=vy):
                ground_at(20, 40, 0xE0 + cls, cls, solid=0x13)(m)
                W(m, 'PlayerVelY', vy); W(m, 'PlayerVelX', 0x18 if cls & 1 else 0)
                B(m, 'LandingBlockTimer', cls & 1); B(m, 'FallCounter', 6 if vy > 0 else 0)
                playback(m, [value_of(IDX_IDLE)])
            cs.append(mk(f'ground class {cls} under the foot (VelY {vy:+d})', 'UpdatePlayer', 'c_UpdatePlayer',
                         level=0, setup=setup, out=('CX',)))
    def bridge(m):
        ground_at(20, 40, 0xD0, 1, solid=0x03, flags=0x20)(m)
        set_tile(m, 0xD1, ground=1, flags=0x20, solid=0x03)
        set_tile(m, 0xD2, ground=1, flags=0x00, solid=0x03)
        set_cell(m, 20, 41, 0xD1)
        W(m, 'BridgeDipTile', cell(20, 41))
        playback(m, [value_of(IDX_IDLE)])
    cs.append(mk('bridge dip: foot tile with flag 20h after a dipping tile', 'UpdatePlayer', 'c_UpdatePlayer',
                 level=0, setup=bridge, out=('CX',)))
    for side, vx in (('right', 0x30), ('left', -0x30)):
        def setup(m, vx=vx, side=side):
            ground_at(20, 40, 0xC0, 1, solid=0x03)(m)
            set_tile(m, 0xC1, side=1, flags=0x10)
            for r in (19, 18):
                set_cell(m, r, 40 + (1 if side == 'right' else -1), 0xC1)
            W(m, 'PlayerVelX', vx); W(m, 'PlayerY', 20 * 16 - 0x20)
            playback(m, [value_of(IDX_IDLE)])
        cs.append(mk(f'wall contact {side} (side class 1 block, velocity {vx:+d})', 'UpdatePlayer',
                     'c_UpdatePlayer', level=7, setup=setup, out=('CX',)))
    for nib in (1, 2):
        def setup(m, nib=nib):
            ground_at(20, 40, 0xB0, 1, solid=0x03)(m)
            set_tile(m, 0xB1, ground=1, flags=nib, solid=0x03)
            set_cell(m, 18, 40, 0xB1)
            set_cell(m, 19, 40, 0)
            W(m, 'PlayerY', 20 * 16 - 0x30); W(m, 'PlayerVelY', 0xFFE0)
            playback(m, [value_of(IDX_IDLE)])
        cs.append(mk(f'ceiling above the head, flags nibble {nib} (rising)', 'UpdatePlayer', 'c_UpdatePlayer',
                     level=0, setup=setup, out=('CX',)))
    for title, lvl, x, y, vx, idx, extra in (
            ('X clamp low: step to X < 8 is refused', 0, 0x0040, 0x0A00, -0x80, IDX_RUN, None),
            ('X clamp high: step to X >= 0FF8h is refused', 3, 0x0F00, 0x0A00, 0x200, IDX_RUN, None),
            ('X window: step beyond CameraLeftLimit+20 is refused', 7, 0x0A00, 0x0A00, 0x200, IDX_RUN,
             {'CameraLeftLimit': 0x0060}),
            ('rows beyond MapRows read as tile 0', 0, 0x0A00, 0x3F00, 0x10, IDX_IDLE, None),
            ('PlayerY below the map: respawn', 3, 0x0A00, 0x4800, 0, IDX_IDLE, None),
            ('PlayerY negative (no bound test)', 0, 0x0A00, 0xFF00, 0, IDX_IDLE, None)):
        def setup(m, x=x, y=y, vx=vx, idx=idx, extra=extra):
            clear_player(m, x=x, y=y, vx=vx)
            for k, v in (extra or {}).items(): W(m, k, v)
            playback(m, [value_of(idx)])
        cs.append(mk(title, 'UpdatePlayer', 'c_UpdatePlayer', level=lvl, setup=setup, out=('CX',)))
    for title, fields in (
            ('respawn request with RespawnState 0 (lives left)', {'B:RespawnRequestFlag': 1}),
            ('respawn state set: counted down', {'B:RespawnState': 2}),
            ('respawn request with no lives: end signal', {'B:RespawnRequestFlag': 1, 'B:Lives': 0}),
            ('EndSignal set by byte 2830h', {'B:byte_2830': 1}),
            ('out of range in level 0Eh: EndSignal FFh', {'B:CurrentLevel': 0x0E, 'PlayerY': 0x4800})):
        def setup(m, fields=fields):
            clear_player(m)
            for k, v in fields.items():
                if k.startswith('B:'): B(m, k[2:], v)
                else: W(m, k, v)
            playback(m, [value_of(IDX_RUN)])
        cs.append(mk(title, 'UpdatePlayer', 'c_UpdatePlayer', level=0, setup=setup, out=('CX',)))
    return cs

def first_slot_free(m):
    m.ww(m.data('word_4F2E') + 4, 0xFFFF)                 # slot 0 free, the rest busy
    for k in range(1, 4):
        m.ww(m.data('word_4F2E') + 0x12 * k + 4, 0x0101)

def projectile_cases():
    cs = []
    def mixed(m):
        for k in range(4):
            rec = m.data('word_4F2E') + 0x12 * k
            m.ww(rec, 0x0A00 + 0x30 * k); m.ww(rec + 2, 0x0A00 - 0x10 * k)
            m.ww(rec + 4, 0x0100 + k)
            m.wb(rec + 5, 0x20 if k != 2 else 0x00)
            m.ww(rec + 6, 0x0040 * (k + 1) - 0x80); m.ww(rec + 0x0E, 0x0020 * k - 0x20)
            m.wb(rec + 7, 0x80 if k & 1 else 0)
            m.wb(rec + 8, k & 1)
            m.ww(rec + 0x0C, m.data('AnimSeqPtrTable') + 2 * k)
        script = m.data('AnimSeqPtrTable')
        m.ww(script, 0x0102); m.ww(script + 2, 0xFFF8); m.ww(script + 4, 0x0105)
    def free(m):
        for k in range(4):
            m.ww(m.data('word_4F2E') + 0x12 * k + 4, 0xFFFF)
    # ApplyPlayerFriction: the game writes MotionShift 0..3 only, but the count is CL in full:
    # the oracle masks it to 5 bits (386 shift), so 20h and 21h shift like 0 and 1.
    for sh in (0x20, 0x21, 0x3F, 0xFF):
        for vx in (0x0005, 0xFFF0, 0x0000):
            cs.append(mk(f'ApplyPlayerFriction: MotionShift {sh:02X}, VelX {vx:04X}', 'ApplyPlayerFriction',
                         'c_ApplyPlayerFriction', setup=lambda m, sh=sh, vx=vx: (B(m, 'MotionShift', sh), W(m, 'PlayerVelX', vx))))
    cs.append(mk('projectile update: kinds 0 and 1 live, one freed (flag bit 5 clear)', 'UpdatePlayerProjectiles',
                 'c_UpdatePlayerProjectiles', setup=mixed, level=0))
    cs.append(mk('projectile update: all four slots free', 'UpdatePlayerProjectiles',
                 'c_UpdatePlayerProjectiles', setup=free, level=0))
    cs.append(mk('FindFreeProjectileSlot: first slot free', 'FindFreeProjectileSlot', 'c_FindFreeProjectileSlot',
                 setup=first_slot_free,
                 out=('CF', 'SI'), level=0))
    cs.append(mk('FindFreeProjectileSlot: all busy (CF set, SI 4F76h)', 'FindFreeProjectileSlot',
                 'c_FindFreeProjectileSlot',
                 setup=lambda m: [m.ww(m.data('word_4F2E') + 0x12 * k + 4, 0x0101) for k in range(4)],
                 out=('CF', 'SI'), level=0))
    return cs

def particle_cases():
    cs = []
    for seed in range(3):
        def setup(m, seed=seed):
            rng = random.Random(0x7700 + seed)
            row, col = find_floor(m)
            for k in range(32):
                rec = 0x50A8 + 0x12 * k
                live = rng.random() < 0.8
                m.ww(rec + 4, rng.choice([0x0046, 0x0049, 0x0136, 0x0134, 0x00E5, 0x012C, 0x0132, 0x0010])
                     if live else 0xFFFF)
                m.ww(rec + 0x0C, rng.choice([0, 1, 2, 5, 0x40, 0xFFFF]))
                m.ww(rec, rng.randrange(0x1400)); m.ww(rec + 2, rng.randrange(0x1400))
                m.ww(rec + 6, rng.randrange(0x10000)); m.ww(rec + 0x0E, rng.randrange(0x10000))
                m.wb(rec + 0x11, rng.choice([0, 0, 3]))
            randomize_tables(m, rng)
            randomize_map(m, rng, row, col)
            B(m, 'SnowFrameTick', seed)
        cs.append(mk(f'burst particles: seeded slots {seed}', 'UpdateBurstParticles', 'c_UpdateBurstParticles',
                     setup=setup, level=[0, 7, 11][seed], es=True))
        # ES = DATA on entry: the routine loads ES = LevelSegment itself and leaves it so
        cs.append(mk(f'burst particles: seeded slots {seed}, ES not preset (ES model)', 'UpdateBurstParticles',
                     'c_UpdateBurstParticles', setup=setup, level=[0, 7, 11][seed]))
    for seed in range(2):
        def setup(m, seed=seed):
            rng = random.Random(0x7800 + seed)
            for k in range(16):
                rec = 0x5450 + 0x12 * k
                m.ww(rec + 4, rng.choice([0xFFFF, 0x0001])); m.ww(rec + 2, rng.randrange(400))
                m.ww(rec + 0x0C, rng.choice([1, 2, 0, 5]))
        cs.append(mk(f'debris lifetime tick {seed}', 'TickDebrisSlots', 'c_TickDebrisSlots', setup=setup,
                     level=0))
    return cs

def adapter_cases():
    """The routines the tests enter directly (registers as in their ASM contracts)."""
    cs = []
    for lvl, dy, vx, vy in ((0, 0, 0, 0), (7, 0x20, 0x30, 0x40), (3, 0x30, -0x30, 0x10)):
        def setup(m, dy=dy, vx=vx, vy=vy):
            row, col = stand_on_floor(m, dy=dy)
            W(m, 'PlayerVelX', vx); W(m, 'PlayerVelY', vy)
            B(m, 'PlayerOnPlatform', 0); B(m, 'FallCounter', 5)
        cs.append(mk(f'UpdatePlayerCollision: level {lvl}, dy {dy:02X}, vx {vx:+d}', 'UpdatePlayerCollision',
                     'c_UpdatePlayerCollision', level=lvl, setup=setup, es=True))
    for row, col, flags in ((19, 40, 0), (19, 40, 0x20), (3, 5, 1)):
        def setup(m, row=row, col=col, flags=flags):
            ground_at(row + 1, col, 0xA0, 2, solid=0x13)(m)
            set_tile(m, 0xA1, ground=3, side=2, flags=0x20 if flags & 0x20 else 0x02, solid=0x21)
            set_cell(m, row + 1, col, 0xA1)
            W(m, 'PlayerY', (row + 1) * 16); W(m, 'PlayerX', col * 16 + 8)
            W(m, 'PlayerVelY', 0x10); W(m, 'PlayerVelX', 0x18)
            W(m, 'BridgeDipTile', 0x55AA)
        cs.append(mk(f'UpdatePlayerTileContact at foot ({row},{col}) flags {flags:02X}',
                     'UpdatePlayerTileContact', 'c_UpdatePlayerTileContact', setup=setup,
                     regs={'BX': cell(row, col)}, level=0, es=True, out=('BX',)))
    for vy, sol in ((0, 0x21), (0x40, 0x13), (-0x10, 0x00), (0x20, 0x30)):
        def land_setup(m, vy=vy, sol=sol):
            row, col = stand_on_floor(m, dy=4)
            W(m, 'PlayerVelY', vy); B(m, 'FallCounter', 12); B(m, 'LandingBlockTimer', 0)
            set_tile(m, get_cell(m, row, col), solid=sol)
            set_tile(m, get_cell(m, row - 1, col), solid=sol ^ 0x10)
            return {'DI': cell(row, col)}
        cs.append(mk(f'LandPlayerOnGround: vy {vy:+d} solid {sol:02X}', 'LandPlayerOnGround',
                     'c_LandPlayerOnGround', setup=land_setup, level=0, es=True))
        def snap_setup(m, vy=vy, sol=sol):
            row, col = stand_on_floor(m, dy=4)
            W(m, 'PlayerVelY', vy); B(m, 'FallCounter', 12)
            set_tile(m, get_cell(m, row, col), solid=sol)
            return {'DI': cell(row - 1, col)}
        cs.append(mk(f'GroundTileSnapOrFall: vy {vy:+d} solid {sol:02X}', 'GroundTileSnapOrFall',
                     'c_GroundTileSnapOrFall', setup=snap_setup, level=0, es=True))
    cs.append(mk('CeilingTileHeadBump: velocity up: stop, sit one row lower', 'CeilingTileHeadBump',
                 'c_CeilingTileHeadBump', setup=lambda m: (W(m, 'PlayerVelY', 0xFFE0), W(m, 'PlayerY', 0x0A07)),
                 level=0, es=True))
    # still, other frame: loads ES = LevelSegment (ES = DATA on entry, so the ES model is exercised);
    # the wall record at word_6BB1 is PlayerX/PlayerY, the cell is read through the new ES
    for px, py in ((0x0148, 0x0A08), (0x0168, 0x0A08)):
        cs.append(mk(f'CeilingTileHeadBump: still, frame 00h: push-out probe at {px:04X},{py:04X}', 'CeilingTileHeadBump',
                     'c_CeilingTileHeadBump',
                     setup=lambda m, px=px, py=py: (W(m, 'PlayerVelY', 0), B(m, 'PlayerFrameWord', 0),
                                                    W(m, 'PlayerX', px), W(m, 'PlayerY', py)),
                     level=0))
    cs.append(mk('CeilingTileHeadBump: still, frame 0Ah: no push', 'CeilingTileHeadBump',
                 'c_CeilingTileHeadBump', setup=lambda m: (W(m, 'PlayerVelY', 0), B(m, 'PlayerFrameWord', 0x0A)),
                 level=0, es=True))
    cs.append(mk('SideTileWallImpactHandler: flags 10h (record written)', 'SideTileWallImpactHandler',
                 'c_SideTileWallImpactHandler',
                 setup=lambda m: (set_tile(m, 0x77, flags=0x10), set_cell(m, 10, 10, 0x77),
                                  W(m, 'PlayerX', 0x0300), W(m, 'PlayerY', 0x0500), {'DI': cell(10, 10)})[-1],
                 level=0, es=True))
    cs.append(mk('SideTileWallImpactHandler: flags 0 (nothing)', 'SideTileWallImpactHandler',
                 'c_SideTileWallImpactHandler',
                 setup=lambda m: (set_tile(m, 0x78, flags=0), set_cell(m, 10, 10, 0x78),
                                  {'DI': cell(10, 10)})[-1], level=0, es=True))
    for vis, bios in ((True, 0), (False, 0), (True, 3)):
        def setup(m, vis=vis, bios=bios):
            RND.set_cam(m, row=5, col=7, dp=0x2000, dd=0)
            if not vis: W(m, 'CameraCol', 200)
            m.wb(m.code('OptionVByte'), bios, seg=CODE_SEG)
        cs.append(mk(f'RedrawTileIfVisible: visible={vis} OptionVByte {bios}', 'RedrawTileIfVisible',
                     'c_RedrawTileIfVisible', setup=setup, regs={'AX': 0x41, 'DI': cell(9, 10)},
                     out=('CF',), level=4))
    cs.append(mk('TriggerPlayerRespawnCore: lives 2 (one life used)', 'TriggerPlayerRespawnCore',
                 'c_TriggerPlayerRespawnCore', setup=lambda m: (B(m, 'Lives', 2), B(m, 'RespawnState', 0)),
                 level=0))
    cs.append(mk('TriggerPlayerRespawnCore: lives 0 (end signal)', 'TriggerPlayerRespawnCore',
                 'c_TriggerPlayerRespawnCore', setup=lambda m: (B(m, 'Lives', 0), B(m, 'RespawnState', 0)),
                 level=0))
    for n in (0x62, 0x63, 0x64, 0x80):
        cs.append(mk(f'IncrementPlayerLives: count {n:02X} (IRET written above 63h)', 'IncrementPlayerLives',
                     'c_IncrementPlayerLives', setup=lambda m, n=n: m.wb(m.data('byte_0666') + 0x2172, n),
                     level=0))
    for bm in (0x1F, 0x05):
        cs.append(mk(f'sub_67D7: bonus mask {bm:02X}', 'sub_67D7', 'c_sub_67D7',
                     setup=lambda m, bm=bm: (B(m, 'BonusLetterMask', bm), W(m, 'PlayerX', 0x0A00),
                                             W(m, 'PlayerY', 0x0B00), B(m, 'byte_6CA8', 0x3F)),
                     level=0))
    for seed in range(4):
        def setup(m, seed=seed):
            rng = random.Random(0x6600 + seed)
            for base in (0x5000, 0x5040):
                m.ww(base, rng.randrange(0x0200)); m.ww(base + 2, rng.randrange(0x0200))
                m.ww(base + 4, rng.randrange(0x0800)); m.wb(base + 5, rng.choice([0x00, 0x80]))
            m.ww(0x5040, m.rw(0x5000) + rng.choice([0, 0x20, 0x3F, 0x50, 0x80]))
        cs.append(mk(f'unused_66D3: record overlap test {seed}', 'unused_66D3', 'c_unused_66D3', setup=setup,
                     regs={'SI': 0x5000, 'DI': 0x5040}, out=('CF',), level=0))
    for flag in (0x00, 0x80):
        cs.append(mk(f'unused_6760: X offset with flag {flag:02X}', 'unused_6760', 'c_unused_6760',
                     setup=lambda m, flag=flag: (m.ww(0x5000, 0x0A00), m.wb(0x5005, flag)),
                     regs={'SI': 0x5000}, out=('AX',), level=0))
    return cs

CASES = (combo_cases() + state_cases() + hurt_cases() + random_sequences() + physics_cases()
         + projectile_cases() + particle_cases() + adapter_cases())

# ---- alt mode hurt (the 'LE' handler word) and the P pause ---------------------------------
def alt_hurt_cases():
    """Hurt state in alt mode: AltStateHandlers[8] reads the text 'LE' (word 454Ch, a bare `ret`
    in SetLatchCopyMode), so the state handler does nothing that frame."""
    cs = []
    for ht in (0x16, 0x30):
        def setup(m, ht=ht):
            stand_on_floor(m)
            B(m, 'PlayerAltModeFlag', 1)
            B(m, 'PlayerHurtTimer', ht); W(m, 'PlayerVelX', 0x20)
            playback(m, [value_of(IDX_IDLE)])
        cs.append(mk(f'alt mode, hurt timer {ht:02X}: handler word 454Ch (ret)', 'UpdatePlayer', 'c_UpdatePlayer',
                     level=3, setup=setup, out=('CX',)))
    return cs

P_KEY = 0x19
def pause_cases():
    """P held at the frame's input wait (WaitFrameSyncFlag): wait for the release, a press and
    the release. The same script runs at each busy-wait turn on both sides: the C polls in each
    loop, the oracle reaches the loop heads loc_629B / loc_62A2 / loc_62A9; turns alternate
    release, press, release."""
    import ctypes, re
    from unicorn import UC_HOOK_CODE
    from emu import CODE_SEG as CSEG
    kdt = int(re.search(r'#define DS_KeyDownTable 0x([0-9A-F]+)u', (ROOT/'c'/'pre2_sym.h').read_text()).group(1), 16)
    def setup(m):
        stand_on_floor(m)
        playback(m, [value_of(IDX_IDLE)])
        m.wb(m.data('KeyDownTable') + P_KEY, 0xFF)
        st = {'n': 0}
        def hook(uc, address, size, user):
            st['n'] += 1
            m.wb(m.data('KeyDownTable') + P_KEY, 0 if st['n'] % 2 else 0xFF)
        hooks = []
        for where in ('loc_629B', 'loc_62A2', 'loc_62A9'):
            a = CSEG * 16 + m.code(where)
            hooks.append(m.uc.hook_add(UC_HOOK_CODE, hook, None, a, a))
        m._pause_hooks = hooks
    def c_setup(d):
        mem = d.pre2_memory().contents
        st = {'n': 0}
        @ctypes.CFUNCTYPE(None)
        def poll():
            st['n'] += 1
            mem[DATA_SEG * 16 + kdt + P_KEY] = 0 if st['n'] % 2 else 0xFF
        ctypes.c_void_p.in_dll(d, 'pre2_test_poll').value = ctypes.cast(poll, ctypes.c_void_p).value
        return poll
    c = mk('P pause: held at the input wait, released, pressed, released', 'UpdatePlayer', 'c_UpdatePlayer',
           level=3, setup=setup, out=('CX',))
    inner = c.setup
    def s(m):
        for h in getattr(m, '_pause_hooks', []): m.uc.hook_del(h)
        m._pause_hooks = []
        inner(m)
    c.setup = s
    c.c_setup = c_setup
    return [c]

CASES += alt_hurt_cases() + pause_cases()

# ---- differential-test gap closure ---------------------------------------------------------
# Branches the frame cases above do not reach: the projectile spawn of the attack state (the
# attack frame flag, bit 6 of the animation word, comes only on the 7th/9th frame of the attack
# animation, so the earlier frames run in the oracle first), the alt-mode momentum block with
# up and down held, the camera-relative range checks of UpdatePlayerCollision, side class 2,
# deep landings, the corner nudge of the ceiling probe, the free wall-impact slot, and the
# in-game credits gate of UpdatePlayer.
FONT_SEG = 0x9800

def font_sheet(m, seed=91):
    """Seeded bytes for the font sheet (word_003D): both sides read the same bytes."""
    rng = random.Random(seed)
    m.write(FONT_SEG, 0, bytes(rng.randrange(256) for _ in range(0x4000)))
    W(m, 'word_003D', FONT_SEG)

def all_slots_free(m):
    for k in range(4):
        m.ww(m.data('word_4F2E') + 0x12 * k + 4, 0xFFFF)

def attack_cases():
    """The projectile spawn (weapon records 2 and 3 carry the projectile flag): UpdatePlayer on
    the frame whose animation word has the attack-frame flag. State 3 reaches it on its 7th
    frame, states 6 and 7 on their 9th."""
    cs = []
    for idx, name, frames in ((IDX_ATTACK[3], 'state 3', 7), (IDX_ATTACK[6], 'state 6', 9), (IDX_ATTACK[7], 'state 7', 9)):
        for weapon in (2, 3):
            for facing in (1, -1):
                def setup(m, idx=idx, weapon=weapon, facing=facing, frames=frames):
                    stand_on_floor(m)
                    W(m, 'PlayerFacing', facing)
                    B(m, 'PlayerWeaponType', weapon)
                    all_slots_free(m)
                    playback(m, [value_of(idx)] * frames)
                    advance(m, frames - 1, cx=0x0C0D)
                cs.append(mk(f'attack projectile: {name}, weapon {weapon}, facing {facing:+d}', 'UpdatePlayer',
                             'c_UpdatePlayer', level=0, setup=setup, out=('CX',), regs={'CX': 0x0C0D}))
    # one slot only free in the middle (the scan takes the first free), projectile slot busy
    def second_free(m):
        stand_on_floor(m)
        B(m, 'PlayerWeaponType', 3)
        for k in range(4):
            m.ww(m.data('word_4F2E') + 0x12 * k + 4, 0xFFFF if k == 2 else 0x0101)
        playback(m, [value_of(IDX_ATTACK[3])] * 7)
        advance(m, 6, cx=0)
    cs.append(mk('attack projectile: only slot 2 free', 'UpdatePlayer', 'c_UpdatePlayer', level=3,
                 setup=second_free, out=('CX',)))
    # a charged shot (power x4)
    def charged(m):
        stand_on_floor(m)
        B(m, 'PlayerWeaponType', 2); B(m, 'ChargeLevel', 0xFF)
        all_slots_free(m)
        playback(m, [value_of(IDX_ATTACK[3])] * 7)
        advance(m, 6, cx=0)
    cs.append(mk('attack projectile: charged shot, weapon 2', 'UpdatePlayer', 'c_UpdatePlayer', level=7,
                 setup=charged, out=('CX',)))
    return cs

def momentum_cases():
    """Alt mode with PlayerAirFlags bit 0 set: the momentum block with up (27EAh) and down
    (27EBh) held, for every AltMomentumStep boundary."""
    cs = []
    def mom(title, idx, step, counter, vx=0x20, vy=0x40, facing=1):
        def setup(m):
            stand_on_floor(m, dy=0x30)
            B(m, 'PlayerAltModeFlag', 1); B(m, 'PlayerAirFlags', 1)
            B(m, 'AltMomentumStep', step); B(m, 'MomentumCounter', counter)
            W(m, 'PlayerVelX', vx); W(m, 'PlayerVelY', vy); W(m, 'PlayerFacing', facing)
            playback(m, [value_of(idx)])
        cs.append(mk(title, 'UpdatePlayer', 'c_UpdatePlayer', level=3, setup=setup, out=('CX',)))
    # up held (idx 4: byte_27EA): step 6 with VelY above / at / below 10h; step < 6 increments
    mom('momentum up: step 6, VelY 40h sets air flag bit 1', IDX_JUMP, 6, 0x30, vy=0x40)
    mom('momentum up: step 6, VelY 10h keeps flags', IDX_JUMP, 6, 0x30, vy=0x10)
    mom('momentum up: step 6, VelY negative', IDX_JUMP, 6, 0x00, vy=-0x20)
    mom('momentum up: step 3 -> 4 with counter, VelY forced to -40h', IDX_JUMP, 3, 0x30)
    mom('momentum up: step 5 -> 6 with counter', IDX_JUMP, 5, 0x08)
    mom('momentum up: step 1 -> 2 (< 4) counter 1', IDX_JUMP, 1, 0x01)
    mom('momentum up: counter 0 falls to the down test, down released', IDX_JUMP, 2, 0x00)
    # down held (idx 2: byte_27EB) and up released
    mom('momentum down: step 3 decrements, ja 1 skips the counter', IDX_CROUCH, 3, 0x10)
    mom('momentum down: step 1 -> 0, counter below 5x speed', IDX_CROUCH, 1, 0x02, vx=0x40)
    mom('momentum down: step 1 -> 0, negative speed, counter below', IDX_CROUCH, 1, 0x02, vx=-0x40)
    mom('momentum down: step 2 -> 1, counter at the limit', IDX_CROUCH, 2, 0xFF, vx=-0x50)
    mom('momentum down: step 0 stays 0, counter grows by speed', IDX_CROUCH, 0, 0x00, vx=0x30)
    mom('momentum down: step 0, zero speed', IDX_CROUCH, 0, 0x03, vx=0)
    # up and down held together (idx 6): up wins while the counter lasts
    mom('momentum up+down: up branch first', 6, 3, 0x20)
    return cs

def collision_cases():
    cs = []
    # UpdatePlayerCollision range checks relative to the camera. The cells around the foot are
    # cleared, so the foot has no ground and the airborne tail runs.
    def rng_case(title, px, py, camrow, camcol, flags=0, level=0, lives=3, platform=0, vy=0, vx=0):
        def setup(m):
            clear_player(m, x=px, y=py, vx=vx, vy=vy)
            set_tile(m, 0, ground=0, side=0, flags=0, solid=0)
            for r in range((py >> 4) - 4, (py >> 4) + 3):
                for c in range((px >> 4) - 2, (px >> 4) + 3):
                    set_cell(m, r, c, 0)
            W(m, 'CameraRow', camrow); W(m, 'CameraCol', camcol)
            B(m, 'LevelFlags', flags); B(m, 'Lives', lives); B(m, 'PlayerOnPlatform', platform)
            B(m, 'FallCounter', 3)
        cs.append(mk(title, 'UpdatePlayerCollision', 'c_UpdatePlayerCollision', setup=setup, level=level, es=True))
    rng_case('range: player left of the camera column (neg ax)', 0x0200, 0x0140, 0x11, 40)
    rng_case('range: player right of the camera column', 0x0600, 0x0140, 0x11, 30)
    rng_case('range: player 21 columns right: out of range, respawn', 0x0900, 0x0140, 0x11, 0x23)
    rng_case('range: player 21 columns left: out of range', 0x0200, 0x0140, 0x11, 0x2F)
    rng_case('range: row above the camera by 12: out of range', 0x0500, 0x0140, 0x20, 0x48)
    rng_case('range: LevelFlags bit 2, above the camera top: out of range', 0x0500, 0x0138, 0x14, 0x48, flags=4)
    rng_case('range: LevelFlags bit 2, above the camera top, no lives', 0x0500, 0x0138, 0x14, 0x48, flags=4, lives=0)
    rng_case('range: LevelFlags bit 2, below the camera top: inside', 0x0500, 0x0170, 0x14, 0x48, flags=4)
    rng_case('range: LevelFlags bit 2, exactly on the camera top', 0x0500, 0x0140, 0x14, 0x48, flags=4)
    rng_case('range: airborne on a platform: FinishPlayerLanding', 0x0500, 0x0140, 0x11, 0x1F, platform=1, vy=0x10)
    rng_case('range: airborne on a platform, rising', 0x0500, 0x0140, 0x11, 0x1F, platform=1, vy=-0x10)
    rng_case('range: airborne, no platform: air motion and FallCounter', 0x0500, 0x0140, 0x11, 0x1F, vy=0x10)
    rng_case('range: airborne, no platform, rising', 0x0500, 0x0140, 0x11, 0x1F, vy=-0x10)
    for lvl in (0x0E, 3):
        rng_case(f'range: below the map in level {lvl:02X}h', 0x0A00, 0x4800, 0x3F, 0x3C, level=lvl)
    return cs

def spike_cases():
    """Side class 2 (spikes, TriggerPlayerRespawn) beside the player, UpdatePlayer frames."""
    cs = []
    for side, vx in (('right', 0x30), ('left', -0x30)):
        for lives in (3, 0):
            for upper in (False, True):
                def setup(m, vx=vx, side=side, lives=lives, upper=upper):
                    ground_at(20, 40, 0xC0, 1, solid=0x03)(m)
                    set_tile(m, 0xC1, side=2, flags=0)
                    col = 40 + (1 if side == 'right' else -1)
                    for r in ((19,) if upper else (19, 18)):
                        set_cell(m, r, col, 0xC1)
                    B(m, 'Lives', lives)
                    W(m, 'PlayerVelX', vx); W(m, 'PlayerY', 20 * 16 - 0x20)
                    playback(m, [value_of(IDX_IDLE)])
                cs.append(mk(f'spikes (side class 2) {side}, lives {lives}{", one cell" if upper else ""}',
                             'UpdatePlayer', 'c_UpdatePlayer', level=3, setup=setup, out=('CX',)))
    return cs

def landing_cases():
    cs = []
    # LandPlayerOnGround: the hard landing (FallCounter > 4, fall >= 20h, VelY >= 50h) and its branches
    for fc, vy, lf, tag in ((0x05, 0x60, 0, 'FallCounter 5, VelY 60h'),
                            (0x0C, 0x60, 0, 'FallCounter 12 bounces to -20h'),
                            (0x0C, 0x60, 1, 'FallCounter 12, LevelFlags bit 0 keeps VelY'),
                            (0x0A, 0x60, 0, 'FallCounter 10 is a soft landing'),
                            (0x16, 0x60, 0, 'FallCounter 22, VelY 60h: no shake'),
                            (0x16, 0xB0, 0, 'FallCounter 22, VelY B0h: shake 8, bounce'),
                            (0x14, 0xA0, 0, 'FallCounter 20, VelY A0h: no shake'),
                            (0x14, 0xA1, 1, 'FallCounter 20, VelY A1h: shake, no bounce velocity'),
                            (0x13, 0xB0, 0, 'FallCounter 19, VelY B0h: no shake')):
        def setup(m, fc=fc, vy=vy, lf=lf):
            row, col = stand_on_floor(m, dy=4)
            W(m, 'PlayerVelY', vy); B(m, 'FallCounter', fc); B(m, 'LevelFlags', lf)
            W(m, 'LastLandY', row * 16 - 0x80)
            B(m, 'LandingBlockTimer', 0)
            return {'DI': cell(row, col)}
        cs.append(mk(f'LandPlayerOnGround: hard landing, {tag}', 'LandPlayerOnGround', 'c_LandPlayerOnGround',
                     setup=setup, level=0, es=True))
    # the same landing through a frame: falling onto the floor from a height
    for fc, vy, lf in ((0x0C, 0x70, 0), (0x16, 0xB0, 0)):
        def setup(m, fc=fc, vy=vy, lf=lf):
            row, col = stand_on_floor(m, dy=0x08)
            W(m, 'PlayerVelY', vy); B(m, 'FallCounter', fc); B(m, 'LevelFlags', lf); B(m, 'PlayerAirFlags', 1)
            W(m, 'LastLandY', row * 16 - 0x100)
            playback(m, [value_of(IDX_IDLE)])
        cs.append(mk(f'UpdatePlayer: falling {fc:02X}/{vy:02X} lands hard', 'UpdatePlayer', 'c_UpdatePlayer',
                     level=0, setup=setup, out=('CX',)))
    # ground class 5 (GroundTileReleaseLand) under a falling player: the same landing tail
    for fc, vy in ((0x0C, 0x70), (0x16, 0xB0), (0x05, 0x60)):
        def setup(m, fc=fc, vy=vy):
            ground_at(20, 40, 0xE5, 5, solid=0x13)(m)
            W(m, 'PlayerVelY', vy); B(m, 'FallCounter', fc); B(m, 'PlayerAirFlags', 1)
            W(m, 'PlayerY', 20 * 16 - 8); W(m, 'LastLandY', 20 * 16 - 0x100)
            playback(m, [value_of(IDX_IDLE)])
        cs.append(mk(f'UpdatePlayer: ground class 5 hard landing {fc:02X}/{vy:02X}', 'UpdatePlayer',
                     'c_UpdatePlayer', level=0, setup=setup, out=('CX',)))
    # SideTileWallImpactHandler: a free slot (first word 55AAh) is filled
    for slot in (0, 7, 19):
        def setup(m, slot=slot):
            set_tile(m, 0x77, flags=0x10); set_cell(m, 10, 10, 0x77)
            W(m, 'PlayerX', 0x0300); W(m, 'PlayerY', 0x0500)
            for k in range(20):
                m.ww(0x6EA9 + 8 * k, 0x55AA if k == slot else 0x1234 + k)
            for k in range(8):
                m.wb(0x6EA9 + 8 * slot + k + 2, 0xEE)
            return {'DI': cell(10, 10)}
        cs.append(mk(f'SideTileWallImpactHandler: free slot {slot} written', 'SideTileWallImpactHandler',
                     'c_SideTileWallImpactHandler', setup=setup, level=0, es=True))
    # CeilingTileHeadBump: the corner nudge (still, frame not 0Ah/15h, side class bit 0 under the head)
    for left_class, tag in ((1, 'left neighbour blocks: +10h'), (0, 'left neighbour open: -10h')):
        def setup(m, left_class=left_class):
            W(m, 'PlayerVelY', 0); B(m, 'PlayerFrameWord', 0)
            W(m, 'PlayerX', 0x0148); W(m, 'PlayerY', 0x00A8)
            set_tile(m, 0x71, side=1); set_tile(m, 0x72, side=left_class)
            set_cell(m, 0x0A, 0x14, 0x71); set_cell(m, 0x0A, 0x13, 0x72)
        cs.append(mk(f'CeilingTileHeadBump: corner nudge, {tag}', 'CeilingTileHeadBump', 'c_CeilingTileHeadBump',
                     setup=setup, level=0))
    return cs

def jump_state_cases():
    cs = []
    def st(title, idx, vx=0, vy=0, facing=1, level=0, fields=None, dy=0):
        def setup(m):
            stand_on_floor(m, dy=dy)
            W(m, 'PlayerVelX', vx); W(m, 'PlayerVelY', vy); W(m, 'PlayerFacing', facing)
            for k, v in (fields or {}).items():
                if k.startswith('B:'): B(m, k[2:], v)
                else: W(m, k, v)
            playback(m, [value_of(idx)])
        cs.append(mk(title, 'UpdatePlayer', 'c_UpdatePlayer', level=level, setup=setup, out=('CX',)))
    st('state 4 charge, negative speed (neg ax)', IDX_CHARGE, vx=-0x10)
    st('state 4 charge, negative speed past the limit', IDX_CHARGE, vx=-0x30)
    return cs

def credits_cases():
    """The in-game credits: Ctrl+Alt+W held and no other key. TryShowCredits shows the screen,
    waits for a button (two playback entries: down, up), repaints the palette, and UpdatePlayer
    then redraws the status bar and runs the frame with the next playback word."""
    cs = []
    def keys_down(m, extra=()):
        for sc in (0x1D, 0x38, 0x11) + tuple(extra):
            m.wb(m.data('KeyDownTable') + sc, 0xFF)
    for title, extra, idx in (('gate passes: credits screen, button wait, repaint', (), IDX_IDLE),
                              ('gate passes, run input in the frame', (), IDX_RUN),
                              ('gate fails: another key (Q) down', (0x10,), IDX_IDLE),
                              ('gate fails: key 7Eh down', (0x7E,), IDX_RUN)):
        def setup(m, extra=extra, idx=idx):
            stand_on_floor(m)
            m.ints[0x10] = RND.bios10                       # palette and mode services of the screen
            font_sheet(m)
            keys_down(m, extra)
            playback(m, [0x10, 0x00, value_of(idx)])
        c = mk(f'UpdatePlayer: Ctrl+Alt+W, {title}', 'UpdatePlayer', 'c_UpdatePlayer', level=0, setup=setup,
               out=('CX',))
        c.ranges = ((0x0040, 0, 0x100),) + tuple(c.ranges)
        cs.append(c)
    def no_w(m):
        stand_on_floor(m)
        for sc in (0x1D, 0x38): m.wb(m.data('KeyDownTable') + sc, 0xFF)
        playback(m, [value_of(IDX_IDLE)])
    cs.append(mk('UpdatePlayer: Ctrl+Alt without W: no credits', 'UpdatePlayer', 'c_UpdatePlayer', level=0,
                 setup=no_w, out=('CX',)))
    return cs

CASES += (attack_cases() + momentum_cases() + collision_cases() + spike_cases() + landing_cases()
          + jump_state_cases() + credits_cases())
