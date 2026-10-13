"""Contact, effect, debris and cell-change routines (src/MODULE3.ASM cs:8295..94F3) vs c/effects.c.

State: every case starts from the oracle's own LoadLevel state of a shipped level (tile cache,
level map, camera tables, sprite tables; see tests/render.py), then frees the object pools
(render slots, follower slots, effect and draw-list pool, debris pool, cell-change records,
effect sources unless a case uses them) and writes the records of the case. The player is
written at PlayerX/PlayerY. Sprite definitions and a script area live in scratch DATA at
9E00h..9F7Fh (both sides see the same bytes; no routine here reads them outside the case).

Compared: DATA and the code segment (always), the level map where the cell routines write it
(LevelSegment, 64 KiB), the VGA state (ring blits, backdrop copy), the contract registers and
flags of each case, and the port and DOS logs.

Run: python tools/difftest.py effects (with build exclusions if other workers' files are in progress).
"""
import importlib.util, random, struct
from difftest import Case
from common import ROOT
from emu import DATA_SEG, CODE_SEG

_spec = importlib.util.spec_from_file_location('render_levels', ROOT/'tests'/'render.py')
_R = importlib.util.module_from_spec(_spec); _spec.loader.exec_module(_R)

LEVELS = (0, 4, 9, 11)

# scratch DATA (no routine under test reads it except through the case's own records)
SD0, SD1, SD2, SD3 = 0x9E00, 0x9E20, 0x9E40, 0x9E60   # sprite defs: flags +4, debris class +8
SCRIPT = 0x9F00                                       # script word stream; marker 7D00h at 9F10h
ANIM = 0x9F40                                         # StepSpriteAnimation script words

PLAYER = 0x4F1C
PLAYER_FRAME = 0x0002                                  # player sprite frame (widths/heights index)
SLOT = lambda i: 0x4FD0 + 0x12 * i
FOLLOW = lambda i: 0x4F2E + 0x12 * i                   # i = 0..3 (4F0Ah is the extra slot)
POOL = lambda k: 0x50A8 + 0x12 * k                     # effect pool (32) then draw list (20)
DEBRIS = lambda k: 0x5450 + 0x12 * k                   # debris pool, 16 records
CELL = lambda k: 0x8C8D + 5 * k                        # 80 cell-change records
SRC = lambda k: 0x8F1D + 7 * k                         # 70 effect sources

def free_all(m):
    for i in range(12): m.ww(SLOT(i) + 4, 0xFFFF)
    for k in range(52): m.ww(POOL(k) + 4, 0xFFFF); m.ww(POOL(k) + 9, 0xFFFF)
    m.ww(0x4F0A + 4, 0xFFFF)
    for i in range(4): m.ww(FOLLOW(i) + 4, 0xFFFF)
    for k in range(16): m.ww(DEBRIS(k) + 4, 0xFFFF)
    for k in range(80): m.ww(CELL(k) + 3, 0xFFFF)
    for k in range(70): m.ww(SRC(k) + 4, 0xFFFF)
    m.wb(m.data('PlayerAltModeFlag'), 0)
    m.ww(m.data('word_6BE2'), 0)
    m.wb(m.data('byte_6C04'), 0)
    m.wb(m.data('byte_27EA'), 0)
    m.wb(m.data('byte_6BC9'), 0)
    m.wb(m.data('LandingFlag'), 0)
    m.wb(m.data('FollowerOverlapMode'), 0)

def sdef(m, a, flags, cls):
    m.wb(a + 4, flags); m.wb(a + 8, cls)

def obj(m, a, x, y, frame, flags=0x20, sd=SD0, st=0, hp=0, cnt=0, script=SCRIPT, owner=None, vel=0):
    """A render slot, pool, follower or draw record: +0 X, +2 Y, +4 frame, +5 flags, +6 sprite def,
    +8 velocity word, +0Ah word, +0Ch script, +0Eh state, +0Fh hit points, +10h counter."""
    # +4 and +5 are one word: the flags byte is its high byte, so the frame key (frame & 1FFFh)
    # keeps the frame's bits 8..12 only if they are OR-ed into the flags written here
    m.ww(a, x); m.ww(a + 2, y); m.ww(a + 4, frame); m.wb(a + 5, flags | ((frame >> 8) & 0x1F))
    m.ww(a + 6, sd)
    m.ww(a + 8, vel); m.ww(a + 0x0C, script); m.wb(a + 0x0E, st); m.wb(a + 0x0F, hp); m.wb(a + 0x10, cnt)
    if owner is not None: m.ww(a + 9, owner)

def player(m, x, y, vx=0, vy=0, air=0, energy=3, lives=3, hurt=0, facing=0):
    m.ww(PLAYER, x); m.ww(PLAYER + 2, y); m.ww(PLAYER + 4, PLAYER_FRAME)
    m.ww(m.data('PlayerVelX'), vx); m.ww(m.data('PlayerVelY'), vy)
    m.wb(m.data('PlayerAirFlags'), air); m.wb(m.data('Energy'), energy); m.wb(m.data('Lives'), lives)
    m.wb(m.data('PlayerHurtTimer'), hurt); m.ww(m.data('PlayerFacing'), facing)

def script_marker(m):
    m.ww(SCRIPT + 0x10, 0x7D00)

def level_setup(lv, extra=None):
    def s(m):
        _R.level_state(m, lv)
        assert m.rw(m.data('LevelSegment')) == 0x2530, 'level map segment moved'
        free_all(m)
        script_marker(m)
        if extra: extra(m)
    return s

def lcase(title, routine, cfunc, lv=0, extra=None, regs=None, out_regs=(), ranges=(), budget=50_000_000):
    return Case(title, routine, cfunc, regs=regs or {}, setup=level_setup(lv, extra), out_regs=out_regs,
                ranges=ranges, files=True, budget=budget)

LEVEL_RANGE = [(0x2530, 0, 0x10000)]     # the level map (LevelSegment = 2530h after LoadLevel)

CASES = []
add = CASES.append

# ---- SpawnDebrisElement (cs:8875) -------------------------------------------------------
for code in (0x49, 0x4A, 0x4B, 0x50, 0x5A, 0x5B, 0x00, 0xFF):
    add(lcase(f'SpawnDebrisElement: code {code:02X}, player source, debris pool empty', 'SpawnDebrisElement',
              'c_SpawnDebrisElement', lv=0,
              extra=lambda m, c=code: (player(m, 0x120, 0x80), m.ww(DEBRIS(0) + 4, 0xFFFF)),
              regs=dict(AX=code, SI=PLAYER)))
for code in range(0x4A, 0x5B):          # every popup code of table A353h (17 entries) and the one past it
    add(lcase(f'SpawnDebrisElement: popup code {code:02X} (score from A353h, ScoreLow near carry)',
              'SpawnDebrisElement', 'c_SpawnDebrisElement', lv=0,
              extra=lambda m, c=code: (player(m, 0x100, 0x80), m.ww(m.data('ScoreLow'), 0xFF00 + c),
                                       m.ww(m.data('ScoreHigh'), 0), m.ww(DEBRIS(0) + 4, 0xFFFF)),
              regs=dict(AX=code, SI=PLAYER)))
add(lcase('SpawnDebrisElement: score carry (ScoreLow FFF0h, code 5Ah = 10000)', 'SpawnDebrisElement',
          'c_SpawnDebrisElement', lv=4,
          extra=lambda m: (player(m, 0x100, 0x80), m.ww(m.data('ScoreLow'), 0xFFF0), m.ww(m.data('ScoreHigh'), 1)),
          regs=dict(AX=0x5A, SI=PLAYER)))
add(lcase('SpawnDebrisElement: full pool still scores (code 4Ah)', 'SpawnDebrisElement', 'c_SpawnDebrisElement',
          lv=9, extra=lambda m: (player(m, 0x100, 0x80), [m.ww(DEBRIS(k) + 4, 0x2080) for k in range(16)]),
          regs=dict(AX=0x4A, SI=PLAYER)))
add(lcase('SpawnDebrisElement: pool source marks owner dead', 'SpawnDebrisElement', 'c_SpawnDebrisElement',
          lv=11, extra=lambda m: (obj(m, POOL(3), 0x40, 0x40, 0x0123, owner=SLOT(2)),
                                  obj(m, SLOT(2), 0, 0, 0x0200), m.ww(DEBRIS(0) + 4, 0xFFFF)),
          regs=dict(AX=0x52, SI=POOL(3))))
add(lcase('SpawnDebrisElement: pool source with owner -1', 'SpawnDebrisElement', 'c_SpawnDebrisElement',
          lv=0, extra=lambda m: obj(m, POOL(5), 0x40, 0x40, 0x0123, owner=0xFFFF),
          regs=dict(AX=0x4C, SI=POOL(5))))
add(lcase('SpawnDebrisElement: slot source below pool threshold keeps owner', 'SpawnDebrisElement',
          'c_SpawnDebrisElement', lv=4, extra=lambda m: obj(m, SLOT(1), 0x40, 0x40, 0x0100, owner=SLOT(5)),
          regs=dict(AX=0x4B, SI=SLOT(1))))
add(lcase('SpawnDebrisElement: 15 of 16 debris used (last slot)', 'SpawnDebrisElement', 'c_SpawnDebrisElement',
          lv=0, extra=lambda m: (player(m, 0x90, 0x60), [m.ww(DEBRIS(k) + 4, 0x2080) for k in range(15)],
                                 m.ww(DEBRIS(15) + 4, 0xFFFF)),
          regs=dict(AX=0x5A, SI=PLAYER)))

# ---- SpawnEffectBurst (cs:8D1B) / SpawnPlayerBurst (cs:867E) / SpawnFourDebrisParticles ---------
def burst_setup(lv, ax_frame=0x1234, free=None, cnt_x=0x0130, cnt_y=0x0090):
    def e(m):
        m.ww(m.data('EffectFrame'), ax_frame)
        m.ww(m.data('EffectOriginX'), cnt_x); m.ww(m.data('EffectOriginY'), cnt_y)
        if free is not None:
            for k in range(32):
                if k not in free: m.ww(POOL(k) + 4, 0x2222)
    return e

for cx, dx, free, title in [
        (1, 0xFF80, None, 'one particle, empty pool (DX unchanged)'),
        (2, 0xFF80, None, 'two particles (second reduces the seeds)'),
        (14, 0xFF60, None, 'fourteen particles (past di 12: AX zeroed first)'),
        (0, 0xFF80, None, 'count 0 wraps: fills every free record'),
        (40, 0x0010, [3, 7, 20], 'count 40 with three free records (stops at pool end)'),
        (5, 0xFFD0, [], 'full pool: nothing written')]:
    add(lcase(f'SpawnEffectBurst: {title}', 'SpawnEffectBurst', 'c_SpawnEffectBurst', lv=4,
              extra=(lambda m, f=free: (burst_setup(4, free=f)(m))),
              regs=dict(AX=0x30, DX=dx, CX=cx), out_regs=('AX', 'CX', 'DX')))
add(lcase('SpawnEffectBurst: negative velocity seed, odd count', 'SpawnEffectBurst', 'c_SpawnEffectBurst', lv=9,
          extra=burst_setup(9, free=[0, 1, 2, 4, 5, 9, 31]), regs=dict(AX=0xFFA0, DX=0x7000, CX=7),
          out_regs=('AX', 'CX', 'DX')))
add(lcase('SpawnPlayerBurst: no charge (6*0 + phase 0): no-op', 'SpawnPlayerBurst', 'c_SpawnPlayerBurst', lv=0,
          extra=lambda m: (player(m, 0x140, 0x90, energy=0), m.wb(m.data('byte_6BC9'), 0))))
add(lcase('SpawnPlayerBurst: energy 3, phase 2 (20 particles)', 'SpawnPlayerBurst', 'c_SpawnPlayerBurst', lv=0,
          extra=lambda m: (player(m, 0x140, 0x90, energy=3), m.wb(m.data('byte_6BC9'), 2))))
add(lcase('SpawnPlayerBurst: energy 128, phase 7 (6*128 low byte 0, count 7+0x300)', 'SpawnPlayerBurst',
          'c_SpawnPlayerBurst', lv=4, extra=lambda m: (player(m, 0x140, 0x90, energy=128),
                                                       m.wb(m.data('byte_6BC9'), 7))))
add(lcase('SpawnPlayerBurst: energy 0, phase 9 (count 9)', 'SpawnPlayerBurst', 'c_SpawnPlayerBurst', lv=9,
          extra=lambda m: (player(m, 0x140, 0x90, energy=0), m.wb(m.data('byte_6BC9'), 9))))
for lv, rnd in ((0, 0x11), (4, 0x80), (9, 0x00)):
    add(lcase(f'SpawnFourDebrisParticles: level {lv}, origin set, debris unrelated', 'SpawnFourDebrisParticles',
              'c_SpawnFourDebrisParticles', lv=lv,
              extra=lambda m, r=rnd: (burst_setup(lv, cnt_x=0x0A0 + r, cnt_y=0x70 + r)(m),
                                      m.wb(m.data('WindRandB'), r), m.wb(m.data('WindRandC'), r ^ 0x5A),
                                      m.ww(m.data('WindRandWord'), 0x1000 + r))))
add(lcase('SpawnFourDebrisParticles: only two free pool records', 'SpawnFourDebrisParticles',
          'c_SpawnFourDebrisParticles', lv=11,
          extra=lambda m: (burst_setup(11, free=[10, 11], cnt_x=0x60, cnt_y=0x50)(m))))

# ---- RandomDebrisFrame (cs:8C13), NextWindRandom state -------------------------------------
for seed in (0x0000, 0x1234, 0xBEEF, 0x00FF, 0x7F80):
    add(lcase(f'RandomDebrisFrame: wind seed {seed:04X} (rejection when r >= 5Fh)', 'RandomDebrisFrame',
              'c_RandomDebrisFrame', lv=4,
              extra=lambda m, s=seed: (m.ww(m.data('WindRandWord'), s), m.wb(m.data('WindRandA'), s & 0xFF),
                                       m.wb(m.data('WindRandB'), s >> 8), m.wb(m.data('WindRandC'), 0x3C ^ s)),
              out_regs=('AX',)))

# ---- SetEffectOriginFromRecord (cs:8BF6) -----------------------------------------------------
for cell in (0x0000, 0x0C14, 0xFF00, 0x12F7):
    add(lcase(f'SetEffectOriginFromRecord: cell {cell:04X}', 'SetEffectOriginFromRecord',
              'c_SetEffectOriginFromRecord', lv=0,
              extra=lambda m, c=cell: m.ww(CELL(7) + 3, c), regs=dict(DI=CELL(7)), out_regs=('AX', 'CX', 'DX')))

# ---- TestHitboxOverlap (cs:8D7B): edge cases ------------------------------------------------
def hb(title, si, di, frames=(0, 0), pos=((0, 0), (0, 0)), vy=0, mode=0):
    def e(m):
        (xs, ys), (xd, yd) = pos
        if vy: m.ww(m.data('PlayerVelY'), vy)
        m.wb(m.data('FollowerOverlapMode'), mode)
        m.ww(si, xs & 0xFFFF); m.ww(si + 2, ys & 0xFFFF); m.ww(si + 4, frames[0])
        m.ww(di, xd & 0xFFFF); m.ww(di + 2, yd & 0xFFFF); m.ww(di + 4, frames[1])
    return lcase(title, 'TestHitboxOverlap', 'c_TestHitboxOverlap', lv=4, extra=e,
                 regs=dict(SI=si, DI=di), out_regs=('CF',))

# frame 0 and 1 widths/heights: FrameWidths[0]=40, FrameHeights[0]=? (read from the level tables)
HB_SI, HB_DI = 0x9F80, 0x9FA0
add(hb('TestHitboxOverlap: plain overlap, both records in one place', HB_SI, HB_DI, pos=((100, 100), (100, 100))))
add(hb('TestHitboxOverlap: X distance exactly 40h (touching, no overlap)', HB_SI, HB_DI,
       pos=((100, 100), (100 + 0x40, 100))))
add(hb('TestHitboxOverlap: X distance 3Fh (overlap by one column)', HB_SI, HB_DI,
       pos=((100, 100), (100 + 0x3F, 100))))
add(hb('TestHitboxOverlap: Y distance exactly 46h (no overlap)', HB_SI, HB_DI, pos=((100, 100), (100, 100 + 0x46))))
add(hb('TestHitboxOverlap: Y distance 45h', HB_SI, HB_DI, pos=((100, 100), (100, 100 + 0x45))))
add(hb('TestHitboxOverlap: negative coordinates overlap', HB_SI, HB_DI, pos=((-20, -30), (-10, -25))))
add(hb('TestHitboxOverlap: negative and positive, far apart in X (wrapped difference)', HB_SI, HB_DI,
       pos=((-200, 50), (200, 50))))
add(hb('TestHitboxOverlap: contained small box in large (follower mode)', HB_SI, HB_DI, mode=1,
       pos=((100, 100), (104, 101)), frames=(0x0001, 0x0000)))
add(hb('TestHitboxOverlap: SI below DI, player test (landing depth, mode 0)', PLAYER, HB_DI,
       pos=((100, 130), (100, 110)), vy=0))
add(hb('TestHitboxOverlap: SI above DI, player test, PlayerVelY 80h (landing)', PLAYER, HB_DI,
       pos=((100, 100), (100, 108)), vy=0x80))
add(hb('TestHitboxOverlap: follower SI (not player) lands on DI', HB_SI, HB_DI, pos=((100, 116), (100, 100)),
       frames=(0, 0)))
add(hb('TestHitboxOverlap: follower mode 1 ignores landing', HB_SI, HB_DI, pos=((100, 116), (100, 100)),
       mode=1))
add(hb('TestHitboxOverlap: high frame bits masked (frame 8004h)', HB_SI, HB_DI, pos=((50, 50), (60, 60)),
       frames=(0x8004, 0xE00A)))

def hb_random(seed):
    rng = random.Random(seed)
    def e(m):
        m.wb(m.data('FollowerOverlapMode'), rng.choice((0, 0, 1)))
        m.ww(m.data('PlayerVelY'), rng.choice((0, 0x80, 0xFF80, 0x100, 0x20)))
        for a in (HB_SI, HB_DI):
            m.ww(a, rng.randrange(-40, 340) & 0xFFFF); m.ww(a + 2, rng.randrange(-40, 240) & 0xFFFF)
            m.ww(a + 4, rng.choice((0, 1, 2, 3, 0x1F, 0x20, 0x8000, rng.randrange(0x10000))))
    return lcase(f'TestHitboxOverlap: random record pair {seed}', 'TestHitboxOverlap', 'c_TestHitboxOverlap', lv=11,
                 extra=e, regs=dict(SI=HB_SI, DI=HB_DI), out_regs=('CF',))
for seed in range(10):
    add(hb_random(seed))
add(lcase('TestHitboxOverlap: player record as DI (player record in slot position)', 'TestHitboxOverlap',
          'c_TestHitboxOverlap', lv=0,
          extra=lambda m: (player(m, 0x90, 0x70, vy=0x40), obj(m, SLOT(0), 0x8A, 0x78, 0x0010)),
          regs=dict(SI=SLOT(0), DI=PLAYER), out_regs=('CF',)))

# ---- ApplyFollowerHitToObjects (cs:8C21), DestroyObjectWithDebris (cs:8C72) ---------------------
def follower_hit(title, hp, pa, sflags=0x0, cls=3, lv=4, sd=SD0, vel=0x40):
    def e(m):
        m.wb(m.data('PlayerAttackPower'), pa)
        sdef(m, SD0, 0x00, cls)
        sdef(m, SD2, 0x01, cls)
        obj(m, SLOT(4), 0x80, 0x60, 0x0123, flags=0x20, sd=sd, hp=hp, cnt=0x30, script=SCRIPT, vel=vel)
        obj(m, FOLLOW(0), 0x84, 0x64, 0x0040, flags=sflags)
    return lcase(title, 'ApplyFollowerHitToObjects', 'c_ApplyFollowerHitToObjects', lv=lv, extra=e,
                 regs=dict(SI=FOLLOW(0)), out_regs=('CF',))
add(follower_hit('ApplyFollowerHitToObjects: hit, hp survives (knockback by vel/4)', hp=9, pa=2))
add(follower_hit('ApplyFollowerHitToObjects: hit, hp exactly equal to attack (no borrow)', hp=4, pa=4))
add(follower_hit('ApplyFollowerHitToObjects: hit, hp below attack (destroy with debris)', hp=2, pa=5, cls=0x0A))
add(follower_hit('ApplyFollowerHitToObjects: knockback record (flags 01h) destroys with knockback', hp=1, pa=3,
                 sd=SD2, cls=0x04))
add(follower_hit('ApplyFollowerHitToObjects: contactable-off record (10h) is skipped', hp=9, pa=2, sd=SD1))

def destroy_case(title, di, bx, si, cls, count_ix=None, sflags=0x00, lv=0, script=SCRIPT):
    dflags = {SD2: 0x01, SD3: 0x88}.get(bx, 0x00)
    def e(m):
        m.wb(m.data('PlayerAttackPower'), 0x1C)
        sdef(m, bx, dflags, cls)
        obj(m, di, 0x70, 0x50, 0x0140, flags=0x20, sd=bx, hp=0, cnt=count_ix << 3 if count_ix is not None else 0,
            script=script)
        obj(m, si, 0x66, 0x44, 0x0141, flags=sflags)
        script_marker(m)
    return lcase(title, 'DestroyObjectWithDebris', 'c_DestroyObjectWithDebris', lv=lv, extra=e,
                 regs=dict(DI=di, BX=bx, SI=si))
add(destroy_case('DestroyObjectWithDebris: burst (flags 00h), debris class 0, count table index 0', SLOT(3), SD0,
                 FOLLOW(1), 0, count_ix=0))
add(destroy_case('DestroyObjectWithDebris: index 5 (count 8), class 6, follower flags 80h', SLOT(4), SD0,
                 FOLLOW(2), 6, count_ix=5, sflags=0x80))
add(destroy_case('DestroyObjectWithDebris: knockback (flags 01h), follower flags 00h, class 0x0F', SLOT(5), SD2,
                 PLAYER, 0x0F, count_ix=2, sflags=0x00, lv=9))
add(destroy_case('DestroyObjectWithDebris: knockback, flags 88h (leave bit set), follower 80h', SLOT(6), SD3,
                 PLAYER, 0x05, count_ix=3, sflags=0x80, lv=11))
add(destroy_case('DestroyObjectWithDebris: table index 6 (count 0 wraps to 65536 pieces)', SLOT(7), SD0, FOLLOW(0),
                 0x02, count_ix=6, lv=4))

# ---- UpdateFollowerSlots (cs:88D7) -----------------------------------------------------------
def follower_scan(title, lv, alt=0, vy=0, hit=True, cells=False):
    def e(m):
        m.wb(m.data('PlayerAltModeFlag'), alt)
        m.ww(m.data('PlayerVelY'), vy)
        m.wb(m.data('PlayerAttackPower'), 2)
        sdef(m, SD0, 0x00, 3)
        obj(m, SLOT(0), 0x80, 0x60, 0x0123, flags=0x20, sd=SD0, hp=6, cnt=0x30)
        obj(m, FOLLOW(0), 0x82, 0x62, 0x0040) if hit else obj(m, FOLLOW(0), 0x20, 0x20, 0x0040)
        obj(m, FOLLOW(1), 0x90, 0x70, 0x0040)
        obj(m, 0x4F0A, 0x84, 0x64, 0x0041) if hit else obj(m, 0x4F0A, 0x10, 0x10, 0x0041)
        if cells:
            # a cell record next to the follower (row 4, column 5) and one more to chain from it
            m.ww(CELL(0) + 3, 0x0405); m.wb(CELL(0) + 1, 0x77); m.wb(CELL(0) + 2, 0)
            m.ww(CELL(1) + 3, 0x0406); m.wb(CELL(1) + 1, 0x78); m.wb(CELL(1) + 2, 0x40)
            m.ww(m.data('SnowFrameTick'), 20); m.ww(m.data('LastEffectTick'), 0)
    # ES = 3000h on entry: TriggerCellChangeGroup leaves ES = DATA (the ES model is compared)
    return lcase(title, 'UpdateFollowerSlots', 'c_UpdateFollowerSlots', lv=lv, extra=e,
                 regs=dict(ES=0x3000), out_regs=(), ranges=())
add(follower_scan('UpdateFollowerSlots: hits on follower 0 and the 4F0Ah slot, PlayerVelY 0', 4, hit=True))
add(follower_scan('UpdateFollowerSlots: alt mode skips the 4F0Ah slot', 9, alt=1, vy=0x40))
add(follower_scan('UpdateFollowerSlots: no hits, PlayerVelY 40h stays', 0, hit=False, vy=0x40))
add(follower_scan('UpdateFollowerSlots: hit on the 4F0Ah slot lowers PlayerVelY to FFB0h', 11, vy=0x40))
add(follower_scan('UpdateFollowerSlots: cell records with timers on the follower', 4, hit=False, cells=True))

# ---- BuildEffectDrawList (cs:8922) ----------------------------------------------------------
def draw_list(title, lv, tick, cam=(0x0003, 0x0003), n=70, spread=8):
    """Sources on a grid around the camera: some inside the window (column offset 0..16h, row
    offset 0..2Bh), some outside. Sway bytes cover the signed range so the wrap path runs."""
    def e(m):
        m.ww(m.data('SnowFrameTick'), tick)
        m.ww(m.data('CameraCol'), cam[0]); m.ww(m.data('CameraRow'), cam[1])
        for k in range(n):
            col = cam[0] + (k % spread) * 3 - 4
            row = cam[1] + (k // spread) * 5 - 6
            m.ww(SRC(k), ((col << 4) + 5) & 0xFFFF); m.ww(SRC(k) + 2, ((row << 4) + 9) & 0xFFFF)
            m.ww(SRC(k) + 4, 0x0050 + k)
            m.wb(SRC(k) + 6, (k * 37 + 3) & 0xFF)
    return lcase(title, 'BuildEffectDrawList', 'c_BuildEffectDrawList', lv=lv, extra=e)
add(draw_list('BuildEffectDrawList: level 0, even tick (sway steps)', 0, 0x0040))
add(draw_list('BuildEffectDrawList: level 4, odd tick (no sway step)', 4, 0x0041, n=12))
add(draw_list('BuildEffectDrawList: level 11, 70 sources (20-record limit reached)', 11, 0x0002,
              cam=(0x0008, 0x0008), n=70, spread=10))
add(draw_list('BuildEffectDrawList: camera far away (no records, all terminators)', 9, 0x0042,
              cam=(0x0300, 0x0200), n=30))

# ---- Cell-change records: ApplyCellChangeRecord (cs:8B6E) -----------------------------------
def apply_case(title, lv, cell, cam, tile=0x2F):
    def e(m):
        m.ww(m.data('CameraCol'), cam[0]); m.ww(m.data('CameraRow'), cam[1])
        m.ww(CELL(9) + 3, cell); m.wb(CELL(9) + 1, tile); m.wb(CELL(9) + 2, 0x33)
        m.ww(m.data('PrevCameraCol'), 0x1111)
        m.wb(m.data('AnimVisibleFlag'), 0); m.wb(m.data('GridDirtyFlag'), 0)
        m.ww(m.data('word_2A76'), 5)
    return Case(title, 'ApplyCellChangeRecord', 'c_ApplyCellChangeRecord', regs=dict(DI=CELL(9)),
                setup=level_setup(lv, e), out_regs=('CF',), files=True, ranges=LEVEL_RANGE)

add(apply_case('ApplyCellChangeRecord: visible at the window origin', 4, 0x0D0C, (0x0C, 0x0D)))
add(apply_case('ApplyCellChangeRecord: column CameraCol+19 (last visible column)', 4, 0x0D1F, (0x0C, 0x0D)))
add(apply_case('ApplyCellChangeRecord: column CameraCol+20 (outside: no blit)', 4, 0x0D20, (0x0C, 0x0D)))
add(apply_case('ApplyCellChangeRecord: row CameraRow+11 (last visible row)', 0, 0x1A05, (0x03, 0x19)))
add(apply_case('ApplyCellChangeRecord: row CameraRow+12 (outside)', 0, 0x1A05, (0x03, 0x18)))
add(apply_case('ApplyCellChangeRecord: column left of the camera (borrow)', 9, 0x2A02, (0x05, 0x20)))
add(apply_case('ApplyCellChangeRecord: row left of the camera (borrow)', 9, 0x0B09, (0x02, 0x0C)))

# ---- TickCellChangeTimer (cs:8A5A) -----------------------------------------------------------
def tick_case(title, lv, flags2, ticks=(0x40, 0x3A), last=0x30, level=None, popup=0x4FBE, di=CELL(4),
              cell=0x0A0B, si=FOLLOW(0), cam=(0, 0), frame_none=False):
    def e(m):
        if level is not None: m.wb(m.data('CurrentLevel'), level)
        m.ww(m.data('PopupRingHead'), popup)
        m.ww(m.data('SnowFrameTick'), ticks[0]); m.ww(m.data('LastEffectTick'), last)
        m.wb(m.data('PlayerFacing'), 0)
        m.ww(m.data('PlayerFacing'), 0xFFFF if flags2 & 0x2000 else 0)
        m.ww(m.data('CameraCol'), cam[0]); m.ww(m.data('CameraRow'), cam[1])
        m.ww(di + 3, cell); m.wb(di + 1, 0x51); m.wb(di + 2, flags2 & 0xFF)
        m.ww(si + 6, 0xFFF0 if flags2 & 0x2000 else 0x0010)
        m.ww(m.data('LevelSegment'), m.rw(m.data('LevelSegment')))
    return Case(title, 'TickCellChangeTimer', 'c_TickCellChangeTimer', regs=dict(DI=di, SI=si),
                setup=level_setup(lv, e), out_regs=('CF',), files=True, ranges=LEVEL_RANGE)

for title, flags, kw in [
        ('TickCellChangeTimer: armed countdown 80h (sparks, apply)', 0x80, dict(cam=(0x0A, 0x0B))),
        ('TickCellChangeTimer: armed countdown 81h (due, not applied)', 0x81, {}),
        ('TickCellChangeTimer: unarmed, ticks too close (no change)', 0x00, dict(ticks=(0x34, 0x33), last=0x33)),
        ('TickCellChangeTimer: unarmed timer 0 (random burst, borrow applies)', 0x00, dict(cam=(0x0A, 0x0B))),
        ('TickCellChangeTimer: unarmed timer 05h (countdown, no apply)', 0x05, {}),
        ('TickCellChangeTimer: unarmed 40h, level 0 (burst 134h)', 0x40, dict(level=0)),
        ('TickCellChangeTimer: unarmed 40h, level 3 (burst 132h)', 0x40, dict(level=3)),
        ('TickCellChangeTimer: unarmed 40h, level 7 (burst 12Ch), cell visible', 0x40, dict(level=7, cam=(0x0A, 0x0B))),
        ('TickCellChangeTimer: unarmed 40h, level 8 (136h then burst count 2)', 0x40, dict(level=8, cam=(0x0A, 0x0B))),
        ('TickCellChangeTimer: unarmed 41h (39 path), level 6', 0x41, dict(level=6)),
        ('TickCellChangeTimer: unarmed 42h, follower below player (PlayerFacing left)', 0x42,
         dict(si=FOLLOW(0), level=9)),
        ('TickCellChangeTimer: record for a player slot (SI >= 4F1Ch), negative [si+6]', 0x00,
         dict(si=PLAYER)),
        ('TickCellChangeTimer: popup ring wrap (head 4F80h)', 0x00, dict(popup=0x4F80, ticks=(0x40, 0x3A))),
        ('TickCellChangeTimer: unarmed 40h at ring start (4FBEh), level 4', 0x40, dict(level=4))]:
    add(tick_case(title, 4, flags, **kw))

# ---- TriggerCellChangeGroup (cs:899E) with the cluster ----------------------------------------
def group_case(title, lv, fx, fy, recs, tick=(0x40, 0x3A), cam=(0x00, 0x00), level=None, si=FOLLOW(1)):
    def e(m):
        m.ww(si + 4, 0x0040); m.ww(si, fx); m.ww(si + 2, fy)
        if level is not None: m.wb(m.data('CurrentLevel'), level)
        m.ww(m.data('SnowFrameTick'), tick[0]); m.ww(m.data('LastEffectTick'), tick[1])
        m.ww(m.data('CameraCol'), cam[0]); m.ww(m.data('CameraRow'), cam[1])
        for k, (cell, flags2, tile) in enumerate(recs):
            m.ww(CELL(k) + 3, cell); m.wb(CELL(k) + 1, tile); m.wb(CELL(k) + 2, flags2)
    # ES = 3000h on entry: a cluster application leaves ES = DATA, the other paths keep it
    return Case(title, 'TriggerCellChangeGroup', 'c_TriggerCellChangeGroup', regs=dict(SI=si, ES=0x3000),
                setup=level_setup(lv, e), out_regs=('CF',), files=True, ranges=LEVEL_RANGE)

# follower at column 5, row 4: X = 5 << 4, Y = (4 << 4) + 16
def fpos(col, row): return (col << 4, (row << 4) + 16)
add(group_case('TriggerCellChangeGroup: no record in reach (CF 0)', 4, *fpos(5, 4), recs=[(0x0909, 0, 0x11)]))
add(group_case('TriggerCellChangeGroup: due trigger on the follower cell, cluster of 4 (CF 1)', 4, *fpos(5, 4),
               recs=[(0x0405, 0, 0x21), (0x0406, 0x00, 0x22), (0x0505, 0x00, 0x23), (0x0506, 0x00, 0x24)],
               cam=(0x00, 0x00)))
add(group_case('TriggerCellChangeGroup: chain across rows (pivot cascade) with the camera window', 0,
               *fpos(7, 7), recs=[(0x0707, 0, 0x31), (0x0808, 0x00, 0x32), (0x0908, 0x00, 0x33),
                                   (0x0A09, 0x00, 0x34), (0x0B0A, 0x00, 0x35), (0x0A0B, 0x00, 0x36)],
               cam=(0x04, 0x05)))
add(group_case('TriggerCellChangeGroup: record timer 40h on level 3 (burst then apply)', 11, *fpos(3, 3),
               recs=[(0x0303, 0x40, 0x41), (0x0304, 0x00, 0x42)], tick=(0x40, 0x30), level=3,
               cam=(0x00, 0x00)))
add(group_case('TriggerCellChangeGroup: armed 81h record is due but not applied (CF 1, cluster from pivot)', 9,
               *fpos(2, 2), recs=[(0x0202, 0x81, 0x51), (0x0203, 0x00, 0x52)], cam=(0x00, 0x00)))
add(group_case('TriggerCellChangeGroup: follower far below all records (row check)', 4, *fpos(5, 9),
               recs=[(0x0405, 0x00, 0x21), (0x0505, 0x00, 0x22)], tick=(0x40, 0x3A)))
add(group_case('TriggerCellChangeGroup: two records with the same cell (second skipped)', 4, *fpos(6, 6),
               recs=[(0x0606, 0x00, 0x61), (0x0606, 0x00, 0x62), (0x0607, 0x00, 0x63)], cam=(0x00, 0x00)))

# ---- StepSpriteAnimation (cs:9047) -----------------------------------------------------------
def anim(title, script_words, flags9, frame_wd):
    def e(m):
        for k, w in enumerate(script_words): m.ww(ANIM + 2 * k, w)
        m.ww(FOLLOW(2) + 0x0C, ANIM); m.wb(FOLLOW(2) + 9, flags9); m.ww(FOLLOW(2) + 4, frame_wd)
    return lcase(title, 'StepSpriteAnimation', 'c_StepSpriteAnimation', lv=0, extra=e,
                 regs=dict(SI=FOLLOW(2)))
add(anim('StepSpriteAnimation: positive frame word, flags 00h', [0x0123, 0x0456], 0x00, 0))
add(anim('StepSpriteAnimation: high bits masked, flags 80h set into bit 15', [0xF0FF, 0x0200], 0x80, 0))
add(anim('StepSpriteAnimation: relative script word (-4 back)', [0x0002, 0xFFFC, 0x0055], 0x00, 0))
add(anim('StepSpriteAnimation: relative word -2 and FFFFh offset, flags 80h', [0xFFFE, 0x0077, 0x1FFF], 0x80, 0))

# ---- SkipScriptToMarker and object-level contact pass --------------------------------------------
def contact(title, lv, extra, vy=0, air=0, energy=3, lives=3, hurt=0, x=0x80, y=0x60, alt=0, byte4f2b=None,
            byte27ea=0, word6be2=0):
    def e(m):
        player(m, x, y, vy=vy, air=air, energy=energy, lives=lives, hurt=hurt)
        m.wb(m.data('PlayerAltModeFlag'), alt)
        if byte4f2b is not None: m.wb(0x4F2B, byte4f2b)     # byte_4F2B is PlayerVelY's high byte
        m.wb(m.data('byte_27EA'), byte27ea)
        m.ww(m.data('word_6BE2'), word6be2)
        extra(m)
    return lcase(title, 'ProcessPlayerContacts', 'c_ProcessPlayerContacts', lv=lv, extra=e)

def slot_at_player(i, cnt=0, flags=0x20, sd=SD0, st=0, hp=4, script=SCRIPT, x=0x80, y=0x60, frame=0x0123):
    def f(m):
        obj(m, SLOT(i), x, y, frame, flags=flags, sd=sd, st=st, hp=hp, cnt=cnt, script=script)
    return f

def frame_dims(m, frame, w=0x20, h=0x20, hot=0x10):
    """Width, height and hot X of a frame index (table entries at index frame*2). The level
    tables have no entry for some dispatch frames; the case gives them a box so the overlap
    test sees the record."""
    idx = (frame & 0x1FFF) * 2
    m.wb(m.data('FrameWidths') + idx, w)
    m.wb(m.data('FrameHeights') + idx, h)
    m.wb(m.data('FrameHotX') + idx, hot)

def pool_at_player(k, frame, owner=None, flags=0x20, sd=SD0, e0e=0x10, x=0x80, y=0x60, c0c=0):
    def f(m):
        frame_dims(m, frame)
        obj(m, POOL(k), x, y, frame, flags=flags, sd=sd, script=c0c, owner=owner)
        m.ww(POOL(k) + 0x0E, e0e)
    return f

add(contact('ProcessPlayerContacts: slot hit, no landing, energy 3 (hurt, lose one)', 0, slot_at_player(0)))
add(contact('ProcessPlayerContacts: slot hit, no landing, energy 0 (respawn request)', 0, slot_at_player(0),
            energy=0, lives=2))
add(contact('ProcessPlayerContacts: slot hit, energy 0 and no lives (end signal)', 0, slot_at_player(0),
            energy=0, lives=0))
add(contact('ProcessPlayerContacts: slot hit in alt mode (no hurt)', 0, slot_at_player(0), alt=1))
add(contact('ProcessPlayerContacts: hurt timer set, only the pool runs', 0, slot_at_player(0), hurt=5))
add(contact('ProcessPlayerContacts: PlayerVelY negative (high byte 80h), no landing: hurt', 0, slot_at_player(0),
            vy=0x8100, air=1))
add(contact('ProcessPlayerContacts: stomp, debris class 0 (dl 0), falls to 0FFA0h', 0, slot_at_player(0, cnt=0),
            vy=0x100, air=1))
add(contact('ProcessPlayerContacts: stomp, dl 1', 0, slot_at_player(0, cnt=1), vy=0x100, air=1))
add(contact('ProcessPlayerContacts: stomp, dl 2 (script marker, velocity from the player)', 0, slot_at_player(0, cnt=2),
            vy=0x100, air=1))
add(contact('ProcessPlayerContacts: stomp, dl 3, player moving right (PlayerX above record X)', 0,
            slot_at_player(0, cnt=3), vy=0x100, air=1, x=0x70))
add(contact('ProcessPlayerContacts: stomp with PlayerVelY 20h (not a stomp: 0FFA0h)', 0, slot_at_player(0, cnt=0),
            vy=0x20, air=1))
for cnt in (0x00, 0x04, 0x08, 0x0C, 0x2C, 0x30):
    add(contact(f'ProcessPlayerContacts: landing with no air flags, counter {cnt:02X}', 0,
                slot_at_player(0, cnt=cnt), vy=0x100, air=0))
add(contact('ProcessPlayerContacts: landing with byte_27EA set (0FF20h)', 0, slot_at_player(0, cnt=0x00),
            vy=0x0, air=0, byte27ea=1))
add(contact('ProcessPlayerContacts: word_6BE2 set destroys the slot object', 0, slot_at_player(0, flags=0x20),
            word6be2=1))
add(contact('ProcessPlayerContacts: slot not contactable (10h) or dead (state FFh) or inactive', 0,
            lambda m: (slot_at_player(0, sd=SD1)(m), slot_at_player(1, st=0xFF)(m), slot_at_player(2, flags=0)(m))))
add(contact('ProcessPlayerContacts: twelve slots all in contact (loop)', 4,
            lambda m: [slot_at_player(i, cnt=i, hp=i)(m) for i in range(12)], air=1, vy=0x100))

# pool dispatch: every frame class (raw frame values; codes after -35h and item tallies)
POOL_FRAMES = [
    (0x00E5, 'raw E5 skipped'), (0x012C, 'raw 12Ch skipped'), (0x0132, 'raw 132h skipped'),
    (0x0134, 'raw 134h skipped'), (0x0136, 'raw 136h skipped'), (0x01CA, 'raw 1CAh fatal'),
    (0x01CB, 'raw 1CBh fatal'), (0x00C6, 'key 91h: clears 6EA9h, item path'), (0x0117, 'key E2h: level exit'),
    (0x0137, 'key 102h: level end'), (0x0119, 'key E4h: checkpoint'), (0x00E3, 'key AEh: life'),
    (0x0042, 'key 0Dh: weapon 0'), (0x00EB, 'key B6h: weapon 1'), (0x0061, 'key 2Ch: weapon 2'),
    (0x0115, 'key E0h: weapon 3'), (0x003A, 'key 05h: energy/ring'), (0x0049, 'key 14h: <=14h path'),
    (0x0055, 'key 20h: bonus letter'), (0x005B, 'key 26h: below 27h'), (0x005C, 'key 27h: bonus bit 0'),
    (0x0060, 'key 2Bh: bonus bit 4'), (0x0062, 'key 2Dh: byte_6CA8 bit 0'), (0x0063, 'key 2Eh: loop 70 sources'),
    (0x0067, 'key 32h: byte_6CA8 bit 5 (wide shift)'), (0x006A, 'key 35h: sway/speed 8582'),
    (0x0075, 'key 40h: 8582 end'), (0x0076, 'key 41h: alt mode'), (0x007F, 'key 4Ah: alt mode end'),
    (0x0080, 'key 4Bh: item index 12h'), (0x00DB, 'key A6h: item'), (0x00DC, 'key A7h: lethal'),
    (0x00DD, 'key A8h: lethal'), (0x00DE, 'key A9h: destroy objects'), (0x00DF, 'key AAh: debris at objects'),
    (0x00E0, 'key AB: default'), (0x00E1, 'key AC: default'), (0x00E2, 'key AD: energy up'),
    (0x00E9, 'key B4h: fade'), (0x00EA, 'key B5h: fade'), (0x00C5, 'key 90h: item path')]
for frame, title in POOL_FRAMES:
    add(contact(f'ProcessPlayerContacts: pool frame {frame:04X} ({title})', 0,
                pool_at_player(0, frame, owner=SLOT(2)), y=0x60, x=0x80, vy=0))
add(contact('ProcessPlayerContacts: key E2h with PlayerY at 20h (not a level exit)', 0,
            pool_at_player(0, 0x0117), y=0x20))
add(contact('ProcessPlayerContacts: energy 3 with key ADh (no gain)', 0, pool_at_player(0, 0x00E2), energy=3))
add(contact('ProcessPlayerContacts: energy 2 with key ADh (gain)', 0, pool_at_player(0, 0x00E2), energy=2))
add(contact('ProcessPlayerContacts: key 8582 with [si+0Eh] >= 80h (negation, random direction)', 0,
            pool_at_player(0, 0x006A, e0e=0x0100), vy=0))
add(contact('ProcessPlayerContacts: key 8582 with [si+0Eh] small (item path)', 0,
            pool_at_player(0, 0x006A, e0e=0x0020), vy=0))
add(contact('ProcessPlayerContacts: alt-mode key 41h with alt mode already on', 0,
            pool_at_player(0, 0x0076), alt=1))
add(contact('ProcessPlayerContacts: key B5h while byte_6C04 is set', 0, pool_at_player(0, 0x00EA)))
add(contact('ProcessPlayerContacts: key B4h while byte_6C04 is set', 0, pool_at_player(0, 0x00E9)))
add(contact('ProcessPlayerContacts: key 1CAh with energy 0 (respawn, no burst)', 0,
            pool_at_player(0, 0x01CA), energy=0, lives=2))
add(contact('ProcessPlayerContacts: key 1CBh with energy 1 (hurt burst)', 0,
            pool_at_player(0, 0x01CB), energy=1))
add(contact('ProcessPlayerContacts: pool owner link to a slot (checkpoint decrement)', 0,
            pool_at_player(0, 0x0119, owner=SLOT(4))))
add(contact('ProcessPlayerContacts: pool record with script word above BCh (no contact)', 0,
            lambda m: obj(m, POOL(0), 0x80, 0x60, 0x00E3, flags=0x20, script=0x00C0)))
add(contact('ProcessPlayerContacts: pool record not active (flags 00h, no contact)', 0,
            lambda m: obj(m, POOL(0), 0x80, 0x60, 0x00E3, flags=0x00)))
add(contact('ProcessPlayerContacts: pool key A9h, slots live and in contact (destroy loop)', 0,
            lambda m: (slot_at_player(0, flags=0x20)(m), slot_at_player(1, sd=SD1)(m), pool_at_player(0, 0x00DE)(m))))
add(contact('ProcessPlayerContacts: pool key AAh, slots live (four debris loop)', 0,
            lambda m: (slot_at_player(0, flags=0x20, hp=9)(m), slot_at_player(2, flags=0x20, hp=9)(m),
                       pool_at_player(0, 0x00DF, owner=SLOT(3))(m))))

def full_pool(m):
    for k in range(52):
        frame = POOL_FRAMES[k % len(POOL_FRAMES)][0]
        obj(m, POOL(k), 0x80 + (k % 3), 0x60, frame, flags=0x20, owner=None)
add(contact('ProcessPlayerContacts: every pool record in contact, mixed frames (52 records)', 4, full_pool))

# ---- SpawnDebris from the contact path: score popups over every item code ---------------------
for frame in (0x00DB, 0x0080, 0x0081, 0x0088, 0x0090, 0x009A, 0x00A6):
    add(contact(f'ProcessPlayerContacts: item frame {frame:04X} (tally and popup code)', 0,
                pool_at_player(1, frame), vy=0))

# ---- ComputePasswordCode (render.c exported; cs:932F) -----------------------------------------
def pw(title, seed, cpu, ax=0x1234, rom=None, opt=None):
    def e(m):
        m.wb(m.code('CpuClass'), cpu, seg=CODE_SEG)
        if seed is not None:
            m.ww(m.data('BiosSeed'), seed); m.wb(m.data('BiosSeedReady'), 1)
        else:
            m.wb(m.data('BiosSeedReady'), 0)
            if rom is not None: m.write(0xF000, 0xFFF0, rom)
            if opt is not None: m.write(0xC000, 0, opt)
    return Case(title, 'ComputePasswordCode', 'c_ComputePasswordCode', regs=dict(AX=ax),
                setup=e, out_regs=('AX',), files=False)
for seed in (0x0000, 0x0001, 0x0020, 0x1234, 0xFFFF):
    for cpu in (0, 2, 3):
        add(pw(f'ComputePasswordCode: seed {seed:04X}, CpuClass {cpu}', seed, cpu, ax=0x0ABC))
add(pw('ComputePasswordCode: AX 0 with seed 0 (multiply by zero)', 0x0000, 3, ax=0x0000))
add(pw('ComputePasswordCode: BIOS seed from F000:FFF0 ROM bytes (no option ROM), CpuClass 0', None, 0,
       ax=0x0042, rom=bytes(range(0x10))))
add(pw('ComputePasswordCode: seed sum 0 gives 20h (ROM all zero), CpuClass 2', None, 2, ax=0x0042,
       rom=bytes(16)))
add(pw('ComputePasswordCode: ROM and option ROM 55AAh at C000:0, CpuClass 3', None, 3, ax=0x00FF,
       rom=bytes((0x5A + 7 * i) & 0xFF for i in range(16)),
       opt=bytes([0x55, 0xAA, 0x04] + [0x3C + i for i in range(0x7D)] + [0x00] * 0x7F + [0x11]) * 1))

# ---- sub_94DC / sub_93F6 / sub_93B2 (SQZ-table helpers) ---------------------------------------
def sq_table(title, w5, sign, adj, out_regs=('SI',), fn='sub_93F6', regs=None):
    ptrs = [0xA415, 0xA41B, 0xA40F, 0x9FC0, 0x9FC4]
    def e(m):
        for k, w in enumerate(w5): m.ww(0x9F80 + 2 * k, w)
        for k, w in enumerate(ptrs): m.ww(0x9F8A + 2 * k, w)
        m.wb(m.data('byte_A3FA'), sign)
        m.wb(m.data('byte_A3F9'), adj)
        m.ww(m.data('word_A403'), 0x0010 + sign); m.ww(m.data('word_A405'), 0x0006)
        for k in range(10): m.ww(0x9FC0 + 2 * k, 0x0A0A + k)
    return lcase(title, fn, 'c_' + fn, lv=0, extra=e, regs=regs or dict(SI=0x9F80), out_regs=out_regs)
add(sq_table('sub_93F6: chain 1A2h/199h then 19Ch..19Eh (A3FAh=0, A3F9h=0)', [0x01A2, 0x0199, 0x019C, 0x019D, 0x019E], 0, 0))
add(sq_table('sub_93F6: chain with sign flip (A3FAh=1)', [0x01A2, 0x0199, 0x019C, 0x019D, 0x019E], 1, 0))
add(sq_table('sub_93F6: bit 40h in A3F9h (A40Bh/A411h/A41Dh/A417h adjusted)', [0x01A1, 0x019A, 0x019C, 0x019D, 0x019E], 0, 0x40))
add(sq_table('sub_93F6: chain 1A1h/19Ah with sign and bit 40h', [0x01A1, 0x019A, 0x019C, 0x019D, 0x019E], 1, 0x40))
add(sq_table('sub_93B2: table entries pointing at A415h, A41Bh, A40Fh', [0x01A2, 0x0199, 0x019C, 0x019D, 0x019E],
             0, 0, fn='sub_93B2', regs=dict(SI=0x9F80, ES=0x3000), out_regs=('SI', 'DI')))
add(sq_table('sub_93B2: with sign flip and bit 40h', [0x01A1, 0x019A, 0x019C, 0x019D, 0x019E], 1, 0x40,
             fn='sub_93B2', regs=dict(SI=0x9F80, ES=0x3000), out_regs=('SI', 'DI')))
add(lcase('sub_94DC: key 1A2h,199h returns E9F8h', 'sub_94DC', 'c_sub_94DC', lv=4,
          regs=dict(DX=0x01A2, AX=0x0199), out_regs=('AX', 'DX')))
add(lcase('sub_94DC: last table key 0093h,FFF4h returns 7D01h', 'sub_94DC', 'c_sub_94DC', lv=4,
          regs=dict(DX=0x0093, AX=0xFFF4), out_regs=('AX', 'DX')))

# ---- RestoreBackdropToDrawPage (cs:9071), SetPictureCopyPointers (cs:926E) ---------------------
add(lcase('RestoreBackdropToDrawPage: draw page 2000h (DS and ES left at A000h)', 'RestoreBackdropToDrawPage',
          'c_RestoreBackdropToDrawPage', lv=4, extra=lambda m: m.ww(m.data('DrawPage'), 0x2000),
          out_regs=('DS', 'ES')))
add(lcase('RestoreBackdropToDrawPage: draw page 0 (DS and ES left at A000h)', 'RestoreBackdropToDrawPage',
          'c_RestoreBackdropToDrawPage', lv=9, extra=lambda m: m.ww(m.data('DrawPage'), 0x0000),
          out_regs=('DS', 'ES')))
add(lcase('RestoreBackdropToDrawPage: draw page offset wraps (FFF0h + 1F40h)', 'RestoreBackdropToDrawPage',
          'c_RestoreBackdropToDrawPage', lv=11, extra=lambda m: m.ww(m.data('DrawPage'), 0xFFF0),
          out_regs=('DS', 'ES')))
add(lcase('SetPictureCopyPointers: LevelSegment 2530h gives DS 2560h', 'SetPictureCopyPointers',
          'c_SetPictureCopyPointers', lv=4, out_regs=('DS', 'ES', 'SI', 'DI')))
add(lcase('SetPictureCopyPointers: LevelSegment 0FFD0h wraps to 0000h+30h', 'SetPictureCopyPointers',
          'c_SetPictureCopyPointers', lv=0, extra=lambda m: m.ww(m.data('LevelSegment'), 0xFFD0),
          out_regs=('DS', 'ES', 'SI', 'DI')))

# ---- Extra: contact pass and cell-change timer with the popup ring and level map ------------
add(lcase('TickCellChangeTimer: popup ring head at its last record (ring wraps to 4FBEh)',
          'TickCellChangeTimer', 'c_TickCellChangeTimer', lv=4,
          extra=lambda m: (m.ww(m.data('PopupRingHead'), 0x4F76), m.ww(m.data('SnowFrameTick'), 0x40),
                           m.ww(m.data('LastEffectTick'), 0x30), m.ww(CELL(4) + 3, 0x0A0B),
                           m.wb(CELL(4) + 2, 0x00), m.wb(CELL(4) + 1, 0x52)),
          regs=dict(DI=CELL(4), SI=FOLLOW(0)), out_regs=('CF',)))

# ---- gap closing: contact sweeps with explicit sprite definitions, and the item / exit side effects ------------
# contact() leaves the sprite definition at SD0 unwritten (scratch DATA: on some levels it reads as "not
# contactable"), so the stomp/landing/hurt reactions of the slot loop were reached only on some levels.
def pc_sweep(seed):
    def e(m):
        rnd = random.Random(seed)
        sdef(m, SD0, rnd.choice((0x00, 0x00, 0x01, 0x08)), rnd.choice((0, 3, 6, 0x0A)))
        sdef(m, SD1, 0x10, 3)
        vy = rnd.choice((0x0100, 0x0021, 0x0020, 0x8100, 0x0080, 0x007F, 0x0180, 0xFF00))
        player(m, 0x80, 0x60, vy=vy, air=rnd.choice((0, 1)), energy=rnd.choice((0, 1, 3)), lives=rnd.choice((0, 2)),
               hurt=rnd.choice((0, 0, 0, 0, 5)), vx=rnd.choice((0, 0x30, 0xFFD0)))
        m.wb(m.data('PlayerAltModeFlag'), rnd.choice((0, 0, 1)))
        m.wb(m.data('byte_27EA'), rnd.randrange(2))
        m.ww(m.data('word_6BE2'), rnd.choice((0, 0, 0, 0, 1)))
        m.wb(m.data('byte_6BC9'), rnd.choice((0, 1, 5)))
        for i in range(rnd.randrange(1, 5)):
            obj(m, SLOT(i), 0x80 + rnd.choice((0, 2, -2)), 0x60 + rnd.choice((0, 0x0C, -0x0C, 0x18, -8, 0x20)),
                0x0123, flags=0x20, sd=rnd.choice((SD0, SD0, SD1)), st=rnd.choice((0, 0, 0xFF)),
                hp=rnd.randrange(8), cnt=rnd.choice((0, 1, 2, 3, 6, 8, 0x2C, 0x30)))
        if rnd.randrange(2):
            frame = rnd.choice([f for f, _ in POOL_FRAMES])
            pool_at_player(rnd.randrange(4), frame, owner=rnd.choice((None, SLOT(2), 0xFFFF)))(m)
    return e

for n in range(200):
    add(lcase(f'ProcessPlayerContacts: sweep {n}', 'ProcessPlayerContacts', 'c_ProcessPlayerContacts',
              lv=(0, 4, 9, 11)[n % 4], extra=pc_sweep(0x9C000000 + n)))

# stomp/landing/hurt reactions of the slot loop, each branch on purpose (slot below the player so the overlap
# test swaps the pair and LandingFlag follows the depth rule)
def slot_react(title, vy, air, dy=0, cnt=0, b27=0, lives=3, energy=3, alt=0, vx=0):
    def e(m):
        sdef(m, SD0, 0x00, 3)
        player(m, 0x80, 0x60, vy=vy, air=air, energy=energy, lives=lives, vx=vx)
        m.wb(m.data('PlayerAltModeFlag'), alt); m.wb(m.data('byte_27EA'), b27)
        frame_dims(m, 0x0123, 0x20, 0x20, 0x10)
        obj(m, SLOT(0), 0x80, 0x60 + dy, 0x0123, flags=0x20, sd=SD0, hp=4, cnt=cnt)
    add(lcase(f'ProcessPlayerContacts: {title}', 'ProcessPlayerContacts', 'c_ProcessPlayerContacts', lv=4, extra=e))
for dy in (0, 0x08, 0x10, 0x14, 0x1F, 0x30):
    for vy in (0x0100, 0x0000, 0x8100, 0xFF00):
        slot_react(f'slot {dy:#x} below, PlayerVelY {vy:04X}, airborne', vy, 1, dy=dy, cnt=2)
        slot_react(f'slot {dy:#x} below, PlayerVelY {vy:04X}, grounded, 27EAh', vy, 0, dy=dy, b27=1)
for cnt in (0, 4, 0x2C, 0x28, 0x30):
    slot_react(f'landing on the ground, counter {cnt:02X}', 0x0100, 0, cnt=cnt)
slot_react('landing with byte_27EA set (0FF20h), grounded', 0x0100, 0, b27=1)
slot_react('stomp (dl 2) with PlayerVelY positive: velocity from the player, record left of the player', 0x0100, 1, cnt=2)
slot_react('hurt with alt mode set (energy kept)', 0x0000, 0, alt=1, dy=-0x10)
slot_react('hurt with energy 0 (respawn core)', 0x0000, 0, energy=0, dy=-0x10)
slot_react('hurt with PlayerVelX', 0x0000, 0, vx=0x30, dy=-0x10)

# pool items with their side effects on the effect sources, energy, byte_6C04 and the level exits
def src_rec(m, k, frame4, x=0x40, y=0x40):
    m.ww(SRC(k), x); m.ww(SRC(k) + 2, y); m.ww(SRC(k) + 4, frame4)

def item_case(title, frame, extra=None, lv=4, **kw):
    def e(m):
        sdef(m, SD0, 0x00, 3)
        pool_at_player(0, frame, owner=SLOT(2))(m)
        obj(m, SLOT(2), 0, 0, 0x0200)
        if extra: extra(m)
    add(contact('ProcessPlayerContacts: ' + title, lv, e, **kw))

item_case('key 2Eh turns the exit objects 116h into 117h (70 sources)', 0x0063,
          lambda m: (src_rec(m, 3, 0x0116), src_rec(m, 40, 0x0116), src_rec(m, 69, 0x0116), src_rec(m, 5, 0x0118)))
item_case('key E4h checkpoint turns 118h into 119h and decrements the owner link', 0x0119,
          lambda m: (src_rec(m, 3, 0x0118), src_rec(m, 69, 0x0118), src_rec(m, 8, 0x0116),
                     obj(m, SLOT(2), 0, 0, 0x0200)))
for b6c9, en in ((5, 2), (5, 3), (4, 2), (0, 0), (5, 0)):
    item_case(f'key 05h energy ring with byte_6BC9 {b6c9} and energy {en}', 0x003A,
              lambda m, b=b6c9: m.wb(m.data('byte_6BC9'), b), energy=en)
for en in (3, 2, 0):
    item_case(f'key 14h energy ring, energy {en}', 0x0049, lambda m: m.wb(m.data('byte_6BC9'), 5), energy=en)
for lvn in (0, 1, 2, 3, 6, 0x0D, 0x0F, 0x0C, 0x0E):
    item_case(f'key E2h level exit on CurrentLevel {lvn:02X}', 0x0117,
              lambda m, l=lvn: m.wb(m.data('CurrentLevel'), l), y=0x60)
for k6c04 in (0, 1):
    item_case(f'key B5h fade out with byte_6C04 = {k6c04}', 0x00EA, lambda m, v=k6c04: m.wb(m.data('byte_6C04'), v))
    item_case(f'key B4h fade in with byte_6C04 = {k6c04}', 0x00E9, lambda m, v=k6c04: m.wb(m.data('byte_6C04'), v))
for mask in (0x00, 0x0F, 0x1F):
    for frame in (0x005C, 0x005D, 0x005E, 0x005F, 0x0060, 0x005B, 0x0061):
        item_case(f'BONUS letter frame {frame:04X}, mask {mask:02X}', frame, lambda m, v=mask: m.wb(m.data('BonusLetterMask'), v))
for pre in (0x00, 0x07, 0x1F):
    for frame in (0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068):
        item_case(f'key item frame {frame:04X}, byte_6CA8 {pre:02X}', frame, lambda m, v=pre: m.wb(m.data('byte_6CA8'), v))
for frame in (0x00DB, 0x00DC, 0x00DD, 0x00DE, 0x00DF):
    for linked in (SLOT(2), 0xFFFF):
        add(contact(f'ProcessPlayerContacts: lethal/destroy frame {frame:04X}, owner link {linked:04X}', 4,
                    lambda m, f=frame, ol=linked: (sdef(m, SD0, 0x00, 3), sdef(m, SD1, 0x10, 3),
                                                   slot_at_player(0, sd=SD0)(m), slot_at_player(1, sd=SD1)(m),
                                                   slot_at_player(2, sd=SD0, st=0xFF)(m),
                                                   slot_at_player(3, flags=0, sd=SD0)(m),
                                                   slot_at_player(4, sd=SD0, x=0x300)(m),
                                                   pool_at_player(0, f, owner=ol)(m), obj(m, SLOT(2), 0, 0, 0x0200)),
                    hurt=5))

# ---- TickCellChangeTimer: the tick running behind LastEffectTick, and a record owned by the 4F0Ah slot ----------
add(tick_case('TickCellChangeTimer: SnowFrameTick behind LastEffectTick (|difference| >= 6)', 4, 0x00,
              ticks=(0x30, 0x40), last=0x40, cam=(0x0A, 0x0B)))
add(tick_case('TickCellChangeTimer: SnowFrameTick just behind (|difference| 5, no change)', 4, 0x00,
              ticks=(0x30, 0x35), last=0x35))
for facing in (0x0000, 0x2000):
    add(tick_case(f'TickCellChangeTimer: record for the 4F0Ah slot, PlayerFacing {"set" if facing else "clear"}',
                  4, 0x00 | facing, si=0x4F0A, cam=(0x0A, 0x0B)))

# ---- chains composed from oracle frames: the level 9 finale and the transform timer --------------------------
# The compared call starts from the state the oracle's own earlier routines produced, so each link is
# checked against the state its predecessor really leaves (UpdateObjects -> EmitLevel9Sparkles -> pool).
def _quiet(m):
    m.ports.log.clear()
    if 0x21 in m.ints: m.ints[0x21].log.clear()

def l9_finale_state(m, which, py_off=0):
    """Level 9, one life left on the countdown, a hazard slot inside the window at (B9h, 1Eh): one oracle
    frame of UpdateObjects emits the four sparks (pool frame 2137h); the player is then put on spark `which`."""
    m.wb(m.data('CurrentLevel'), 9)
    player(m, 0x0200, 0x0100)
    m.wb(m.data('RespawnState'), 0)
    m.ww(m.data('word_A519'), 1); m.wb(m.data('byte_A515'), 1); m.ww(m.data('Level9ScriptPtr'), 0xA51B)
    frame_dims(m, 0x0050)
    m.ww(0x4F2E, 0xBA); m.ww(0x4F30, 0x20); m.ww(0x4F32, 0x0050)
    m.call('UpdateObjects', budget=400_000_000)
    for _ in range(11):                      # the sparks age (timer C6h counts down to the contact limit BCh)
        m.call('UpdateBurstParticles', budget=100_000_000)
    _quiet(m)
    sparks = [k for k in range(32) if m.rw(POOL(k) + 4) == 0x2137]
    assert len(sparks) == 4, sparks
    k = sparks[which]
    frame_dims(m, 0x0137)
    player(m, m.rw(POOL(k)) + 2, m.rw(POOL(k) + 2) + py_off)

for which in range(4):
    for py_off in (0, 0x10):
        add(lcase(f'ProcessPlayerContacts: level 9 finale, player on spark {which} (offset {py_off:#x}) -> LevelEndMode FFh',
                  'ProcessPlayerContacts', 'c_ProcessPlayerContacts', lv=9,
                  extra=lambda m, w=which, o=py_off: l9_finale_state(m, w, o), budget=400_000_000))
add(lcase('ProcessPlayerContacts: level 9 finale, player away from the sparks (no level end)',
          'ProcessPlayerContacts', 'c_ProcessPlayerContacts', lv=9,
          extra=lambda m: (l9_finale_state(m, 0), player(m, 0x0300, 0x0200)), budget=400_000_000))

def transform_state(m, preset=0x00, n_items=3):
    """Pool items 30h, 31h, 32h (frames 65h..67h) on the player set byte_6CA8 bits 3..5 in one contact pass;
    sub_67D7 then clears them and arms the 660-frame timer word_6BE2 = 294h."""
    sdef(m, SD0, 0x00, 3)
    m.wb(m.data('byte_6CA8'), preset)
    player(m, 0x80, 0x60)
    for k, frame in enumerate((0x0065, 0x0066, 0x0067)[:n_items]):
        pool_at_player(k, frame, owner=0xFFFF)(m)
    m.call('ProcessPlayerContacts', budget=100_000_000)
    _quiet(m)

add(lcase('ProcessPlayerContacts: three key items 30h..32h set byte_6CA8 bits 3..5', 'ProcessPlayerContacts',
          'c_ProcessPlayerContacts', lv=4,
          extra=lambda m: (sdef(m, SD0, 0x00, 3), player(m, 0x80, 0x60),
                           [pool_at_player(k, f, owner=0xFFFF)(m) for k, f in enumerate((0x0065, 0x0066, 0x0067))])))
for n_items in (2, 3):
    def ts(m, n=n_items):
        transform_state(m, n_items=n)
        m.call('sub_67D7', budget=100_000_000)
        _quiet(m)
        assert m.rw(m.data('word_6BE2')) == (0x294 if n == 3 else 0), hex(m.rw(m.data('word_6BE2')))
        # slots in contact with the player while the timer runs: every touched object is destroyed
        sdef(m, SD1, 0x00, 5)
        for i in range(3): slot_at_player(i, sd=SD1, hp=3)(m)
        slot_at_player(5, sd=SD1, st=0xFF)(m)
    add(lcase(f'ProcessPlayerContacts: transform timer armed by {n_items} key items, slots in contact',
              'ProcessPlayerContacts', 'c_ProcessPlayerContacts', lv=4, extra=ts, budget=100_000_000))

_CASE_TITLES = [c.title for c in CASES]
assert len(set(_CASE_TITLES)) == len(_CASE_TITLES), 'duplicate case titles'
