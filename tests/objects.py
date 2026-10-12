"""Object update (src/MODULE3.ASM cs:6822..8294) vs c/objects.c.

Level state: LoadLevel of the oracle on LEVELn.SQZ (CurrentLevel n; file char '1'+n), cached per
level as in tests/render.py. Object records are written in DATA scratch that no label uses
(3E00h..3FFFh, below the level allocations at DS B210h): owners at 3E00h, synthetic entities
at 3E80h, scripts at 3F00h. Child slots are the real render slots (4FD0h), the entity list is
the level's own list or a synthetic one at 8489h.

Coverage: every routine of the region directly (adapters), every child kind in several states
(near and far from the player, owner flags), every entity kind spawning or refusing, the full
and empty child pools, the disable bit with byte_B198 0 and 1, far culls, deaths and re-arming,
the boss states and hazard hits, the level-5 and level-9 sequences, multi-frame sequences on
the real levels, seeded random placements, and the static copies of routines outside the region.
"""
import ctypes, random
from difftest import Case
from common import ROOT
from emu import DATA_SEG, CODE_SEG

FREE = 0x2530                 # first paragraph after the loaded image
OWN = 0x3E00                  # owner (entity) records used by direct handler cases
ENT = 0x3E80                  # synthetic entity records
SCR = 0x3F00                  # child scripts
ENTITY_LIST = 0x8489
POOL = 0x4FD0                 # child slots
PLAYER = 0x4F1C
PARTICLES = 0x50A8
LEVELS = (0, 4, 5, 9)         # CurrentLevel 5 = LEVEL6.SQZ, 9 = LEVELA.SQZ
_levels = {}

# -- VGA and BIOS helpers (as in tests/render.py) ----------------------------------------
def vga_base(m):
    return ctypes.addressof(m.vga.pre2_vga_state().contents)

def vga_bytes(m):
    return bytes(m.vga.pre2_vga_state().contents)

def vga_restore(m, data):
    ctypes.memmove(vga_base(m), data, len(data))

def bios10(m):
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

def base(m):
    m.ww(m.code('ConventionalTopSeg'), 0xA000, seg=CODE_SEG)
    m.ww(m.data('NextFreeSegment'), FREE)
    m.ints[0x10] = bios10
    m.vga.vga_set_retrace(4)

def level_state(m, level):
    if level not in _levels:
        base(m)
        m.call('LoadLevel', {'AX': level}, budget=200_000_000)
        m.ints[0x21].log.clear(); m.ints[0x21].next = 5
        m.ports.log.clear()
        _levels[level] = (m.snapshot(), vga_bytes(m))
    mem, vga = _levels[level]
    m.restore(mem)
    vga_restore(m, vga)
    m.vga.vga_set_retrace(4)

def quiet(m):
    """Clears the logs of the setup-phase oracle runs (the compared run starts clean)."""
    m.ports.log.clear()
    if 0x21 in m.ints: m.ints[0x21].log.clear()

# -- record builders ---------------------------------------------------------------------
def put_child(m, si, X, Y, frame=0x2100, xvel=0, yvel=0, state=0, script=SCR, owner=OWN,
              facing=0, count=0, aux=0, blink=0):
    m.ww(si + 0x00, X); m.ww(si + 0x02, Y); m.ww(si + 0x04, frame)
    m.ww(si + 0x06, owner); m.ww(si + 0x08, xvel); m.ww(si + 0x0A, yvel)
    m.ww(si + 0x0C, script); m.wb(si + 0x0E, state); m.wb(si + 0x0F, aux)
    m.wb(si + 0x10, count); m.wb(si + 0x11, blink); m.wb(si + 0x09, facing)

TILE_KINDS = (0, 10)          # entity kinds that read the tile box (+9 X, +0Ah Y, +0Bh W, +0Ch H)

def put_entity(m, addr, kind, sprite=0x0100, flags=0, period=0, timer=0, X=0x0400, Y=0x0400,
               params=None, stride=0x20):
    """Entity record. Kinds 0 and 10 read a tile box around (X, Y) (6x6 tiles); the others read
    the pixel point (word +9 = X, word +0Bh = Y). The two layouts share bytes +0Bh/+0Ch."""
    m.wb(addr + 0, stride); m.wb(addr + 1, kind); m.ww(addr + 2, sprite)
    m.wb(addr + 4, flags); m.wb(addr + 6, period); m.wb(addr + 7, timer)
    if kind in TILE_KINDS:
        m.wb(addr + 9, (X >> 4) - 2); m.wb(addr + 0x0A, (Y >> 4) - 2)
        m.wb(addr + 0x0B, 6); m.wb(addr + 0x0C, 6)
    else:
        m.ww(addr + 9, X); m.ww(addr + 0x0B, Y)
    for k, v in enumerate(params or ()):
        m.wb(addr + 0x0D + k, v)

def owner_rec(m, kind=0, flags=0, period=3, timer=0, X=0x0400, Y=0x0400, params=None):
    """Owner entity at OWN: word +9 = X, word +0Bh = Y, params 0Dh..13h."""
    p = params or (0x20, 0x10, 0x10, 0x18, 0x02, 0x20, 0x40)
    m.wb(OWN + 1, kind); m.ww(OWN + 2, 0x0100); m.wb(OWN + 4, flags)
    m.wb(OWN + 6, period); m.wb(OWN + 7, timer); m.ww(OWN + 9, X); m.ww(OWN + 0x0B, Y)
    for k, v in enumerate(p):
        m.wb(OWN + 0x0D + k, v)

def put_script(m, addr=SCR, words=(0x0005, 0x0006, 0xFFFC, 0x0007, 0xFFF8, 0x0008, 0x7D00, 0x0009)):
    for k, w in enumerate(words):
        m.ww(addr + 2 * k, w)

def set_player(m, X, Y, vy=0, vx=0):
    m.ww(m.data('PlayerX'), X); m.ww(m.data('PlayerY'), Y)
    m.ww(m.data('PlayerVelY'), vy); m.ww(m.data('PlayerVelX'), vx)

def set_cam(m, row=None, col=None):
    if row is not None: m.ww(m.data('CameraRow'), row)
    if col is not None: m.ww(m.data('CameraCol'), col)

def near(m, X, Y, rnd):
    """A player position close to the child/entity point (inside the visible window)."""
    set_player(m, (X + rnd.randrange(-0x40, 0x41)) & 0xFFFF, (Y + rnd.randrange(-0x30, 0x31)) & 0xFFFF)

def far(m, X, Y):
    set_player(m, (X + 0x0300) & 0xFFFF, (Y - 0x0300) & 0xFFFF)

def clear_pool(m):
    for k in range(12):
        m.ww(POOL + 0x12 * k + 4, 0xFFFF)
    for k in range(32):
        m.ww(PARTICLES + 0x12 * k + 4, 0xFFFF)

def clear_entities(m):
    m.wb(ENTITY_LIST, 0x32)

def reset_boss(m, state=0xFF):
    m.wb(m.data('BossState'), state)
    m.ww(m.data('BossX'), 0x0500); m.ww(m.data('BossY'), 0x0400)
    m.ww(m.data('BossVelX'), 0); m.ww(m.data('BossVelY'), 0)
    m.ww(m.data('BossEnergy'), 0x0064); m.wb(m.data('BossCounter'), 0)
    m.ww(m.data('BossStateTimer'), 0); m.ww(m.data('BossAnimPtr'), 0xA427)
    m.ww(m.data('word_564C'), 0xFFFF)

def table_sprite(m, kind):
    """A sprite id the spawn lookup (ds:A86Fh) finds for entity kind `kind`, or None."""
    for off in range(0xA86F, 0xA86F + 0x400, 2):
        if m.rw(off) == 0x7D01 and m.rw(off + 2) == kind:
            return (m.rw(off + 4) + 0x138) & 0xFFFF
    return None

def spawn_ready(m, kind):
    """Entity list: the synthetic entity of `kind` with a sprite the lookup finds."""
    s = table_sprite(m, kind)
    return s if s is not None else 0x0100

# -- case builder ------------------------------------------------------------------------
def case(title, routine, cfunc, setup, regs=None, out_regs=(), level=None):
    def s(m):
        if level is not None: level_state(m, level)
        else: base(m)
        setup(m)
        quiet(m)
    return Case(title, routine, cfunc, regs=regs or {}, setup=s, out_regs=out_regs, files=True,
                budget=400_000_000)

def frames_then(setup, n_before, rnd_seed=None):
    """Runs n_before oracle frames before the compared one (multi-frame sequences)."""
    def s(m):
        setup(m)
        for _ in range(n_before):
            m.call('UpdateObjects', budget=400_000_000)
    return s

# -- per-routine cases --------------------------------------------------------------------
def unit_cases():
    cs = []
    rnd = random.Random(0x0B1)
    # ComputeBobOffset: classes through ds:TileSolidProps, child X low nibble
    for cls in range(256):   # every class; the table entry is set to the class so all values reach the branches
        x = (0x0D + 0x10 * (cls % 3)) & 0xFF      # low nibble 0Dh: the phase quotient is 4
        cs.append(case(f'ComputeBobOffset: class {cls:02X} at X {x:02X}', 'ComputeBobOffset',
                       'c_ComputeBobOffset', level=4, out_regs=('AX',),
                       setup=lambda m, xx=x, c=cls: (m.wb(m.data('TileSolidProps') + c, c),
                                                     put_child(m, POOL, xx, 0x0300)),
                       regs={'SI': POOL, 'AX': cls}))
    # ScriptStepBack / ScriptStepForward over scripts with jumps
    for label, routine, cf in [('ScriptStepBack', 'ScriptStepBack', 'c_ScriptStepBack'),
                               ('ScriptStepForward', 'ScriptStepForward', 'c_ScriptStepForward')]:
        for words, at in [((0x0005, 0x0006, 0xFFFC, 0x0007, 0x0008, 0xFFF0, 0x0009), 4),
                          ((0x0001, 0x0002, 0x0003, 0x0004, 0xFFF8, 0x0005), 2),
                          ((0xFFFE, 0x0010, 0x0011), 0)]:
            cs.append(case(f'{label}: script {words[:3]} at word {at}', routine, cf, level=4,
                           setup=lambda m, w=words, a=at: (put_script(m, SCR, w),
                                                          put_child(m, POOL, 0x0300, 0x0300,
                                                                    script=SCR + 2 * a)),
                           regs={'SI': POOL}))
    # SkipScriptToMarker
    cs.append(case('SkipScriptToMarker: marker 7D00h two words on', 'SkipScriptToMarker',
                   'c_SkipScriptToMarker', level=4,
                   setup=lambda m: (put_script(m, SCR, (1, 2, 3, 0x7D00, 9)),
                                    m.ww(OWN + 0x0C, SCR)), regs={'DI': OWN}))
    # AllocChildRecord: empty, one free at index 7, full pool
    for label, free in [('empty pool, slot 0', 0), ('one free slot 7', 7), ('full pool', None)]:
        def sa(m, f=free):
            clear_pool(m)
            for k in range(12):
                if k != f: m.ww(POOL + 0x12 * k + 4, 0x2100)
        cs.append(case(f'AllocChildRecord: {label}', 'AllocChildRecord', 'c_AllocChildRecord',
                       level=4, setup=sa, out_regs=('CF', 'DI')))
    # AllocDebrisRecord: first free at 0, 7, 19
    for k in (0, 7, 19):
        def sd(m, kk=k):
            for j in range(20):
                m.ww(0x7DE6 + 6 * j, 0x1234 if j != kk else 0xFFFF)
        cs.append(case(f'AllocDebrisRecord: first free record {k}', 'AllocDebrisRecord',
                       'c_AllocDebrisRecord', level=4, setup=sd, out_regs=('DI',)))
    # IsInCameraWindow edges and outside, camera at (10, 8)
    for X, Y, lab in [(0x0A * 16, 0x08 * 16, 'centre'), (0x08 * 16 - 1, 0x08 * 16, 'left edge - 1'),
                      (0x08 * 16 - 0x20, 0x08 * 16, 'left edge - 2 tiles'),
                      (0x20 * 16 + 0x0F, 0x08 * 16, 'right edge + 1'),
                      (0x0A * 16, 0x06 * 16, 'top edge - 2'), (0x0A * 16, 0x16 * 16, 'bottom edge + 6')]:
        cs.append(case(f'IsInCameraWindow: {lab}', 'IsInCameraWindow', 'c_IsInCameraWindow', level=4,
                       setup=lambda m, x=X, y=Y: set_cam(m, row=8, col=10),
                       regs={'AX': X, 'DX': Y}, out_regs=('CF',)))
    # TickEntityTimer on owners
    for timer, period in [(0, 3), (0x0B, 3), (0xFF, 0x20), (0x0C, 1)]:
        cs.append(case(f'TickEntityTimer: timer {timer:02X} period {period}', 'TickEntityTimer',
                       'c_TickEntityTimer', level=4,
                       setup=lambda m, t=timer, p=period: owner_rec(m, period=p, timer=t),
                       regs={'BX': OWN}, out_regs=('CF',)))
    # CullChildAt: near, far (low and dying states), drawn, FF state
    for lab, X, Y, st, fr in [('near, state 1', 0x400, 0x400, 1, 0x2100),
                              ('far, state 1 retires', 0x0800, 0x0400, 1, 0x2100),
                              ('far, state 0Ah death path', 0x0800, 0x0400, 0x0A, 0x2100),
                              ('far but drawn', 0x0800, 0x0400, 1, 0x2120),
                              ('far, state FF kept', 0x0800, 0x0400, 0xFF, 0x2100),
                              ('far in Y only, state 2', 0x0400, 0x0900, 2, 0x2100)]:
        def sc(m, x=X, y=Y, s=st, f=fr):
            set_player(m, 0x0400, 0x0400)
            owner_rec(m, flags=0x04, period=3)
            put_child(m, POOL, x, y, frame=f, state=s, owner=OWN)
        cs.append(case(f'CullChildAt: {lab}', 'CullChildAt', 'c_CullChildAt', level=4, setup=sc,
                       regs={'SI': POOL, 'AX': X, 'DX': Y}))
    # SpawnDebrisLine: owner X/Y and spacing
    for al, dl in [(0, 0x40), (3, 0x7F), (0x0C, 0xF0)]:
        cs.append(case(f'SpawnDebrisLine: kind {al:02X} spacing {dl:02X}', 'SpawnDebrisLine',
                       'c_SpawnDebrisLine', level=4,
                       setup=lambda m, a=al, d=dl: (owner_rec(m, X=0x0200, Y=0x0300), clear_debris(m)),
                       regs={'BX': OWN, 'AX': al, 'DX': dl}))
    # SetChildOrbitPosition and SetChildDirToPlayer
    for ang, rad in [(0x10, 0x30), (0xC8, 0x0E)]:
        cs.append(case(f'SetChildOrbitPosition: angle {ang:02X} radius {rad:02X}', 'SetChildOrbitPosition',
                       'c_SetChildOrbitPosition', level=4,
                       setup=lambda m, a=ang, r=rad: (owner_rec(m, X=0x0300, Y=0x0300,
                                                               params=(0, 0, 0, 0, 0)) ,
                                                     m.wb(OWN + 0x0F, a), m.wb(OWN + 0x0D, r),
                                                     clear_debris(m), put_child(m, POOL, 0, 0)),
                       regs={'SI': POOL, 'BX': OWN}))
    for X in (0x0100, 0x0500):
        cs.append(case(f'SetChildDirToPlayer: child X {X:04X}', 'SetChildDirToPlayer', 'c_SetChildDirToPlayer',
                       level=4, setup=lambda m, x=X: (set_player(m, 0x0300, 0x0300), put_child(m, POOL, x, 0)),
                       regs={'SI': POOL}))
    return cs

def clear_debris(m):
    for j in range(20):
        m.ww(0x7DE6 + 6 * j, 0xFFFF)

# -- child handlers (direct) ----------------------------------------------------------------
CHILD_HANDLERS = ['UpdateGroundChild', 'CullChildOnly', 'UpdateBobChild', 'UpdateWalkerChild',
                  'UpdateOrbitChild', 'UpdatePounceChild', 'UpdateWanderingChild', 'UpdateLeaperChild',
                  'UpdateHopChild', 'UpdatePatrolChild', 'UpdateChargerChild', 'UpdateLeapChild',
                  'UpdateLaunchedChild']
CHILD_STATES = (0, 1, 2, 3, 4, 5, 0x0A, 0x0B, 0x0C, 0xFF)

def child_cases():
    cs = []
    for kind, h in enumerate(CHILD_HANDLERS):
        for si_, st in enumerate(CHILD_STATES):
            for place in ('near', 'far'):
                seed = kind * 100 + si_ * 7 + (0 if place == 'near' else 3)
                def s(m, k=kind, state=st, pl=place, sd=seed):
                    rnd = random.Random(sd)
                    cx, cy = 0x0400 + rnd.randrange(0x200), 0x0300 + rnd.randrange(0x100)
                    flags = (0x00, 0x01, 0x10, 0x18, 0x04)[sd % 5]
                    owner_rec(m, kind=k, flags=flags, period=2 + sd % 4, timer=sd % 9, X=cx, Y=cy,
                              params=(0x28, 0x14, 0x12, 0x18, 0x03, 0x20, 0x40))
                    if pl == 'near': near(m, cx, cy, rnd)
                    else: far(m, cx, cy)
                    put_script(m)
                    put_child(m, POOL, cx + rnd.randrange(-0x10, 0x10), cy + rnd.randrange(-8, 9),
                              frame=0x2100 if sd % 2 else 0x2120, xvel=rnd.randrange(-0x40, 0x40),
                              yvel=rnd.randrange(-0x40, 0x40), state=state, owner=OWN, facing=0x80 * (sd % 2))
                    m.wb(OWN + 0x0E, 0x12 + sd % 3)
                    m.wb(m.data('ScriptFrameFlags'), 0x40 * (sd % 2))
                    m.wb(m.data('AttackAnimLock'), sd % 2)
                    m.wb(m.data('ShakeMagnitude'), 0)
                    m.wb(m.data('CurrentLevel'), 4)
                    clear_debris(m)
                cs.append(case(f'{h}: state {st:02X}, {place}, seed {seed}', h, f'c_{h}', level=4,
                               setup=s, regs={'SI': POOL}, out_regs=()))
    return cs

# -- entity handlers (direct, plus the pool-full refusal) ------------------------------------
ENTITY_HANDLERS = ['SpawnDropOverPlayer', 'SpawnChildWhenVisible', 'SpawnFixedChild', 'SpawnChildLatchState37',
                   'SpawnChildResetCounters', 'SpawnChildLatched', 'SpawnChildLatched', 'SpawnChildLatched',
                   'SpawnChildLatched', 'SpawnChildWithFlags', 'SpawnChildOnPlayerBox', 'SpawnChildRandomY',
                   'SpawnChildOnApproach']

def player_for_box(m):
    """A player position (and so a ring column) where SpawnChildOnPlayerBox finds a passable row:
    the scan of its ldxlat column (LevelSegment, table ds:7F5Eh) on the loaded level."""
    seg = m.rw(m.data('LevelSegment'))
    rows = m.rb(m.data('MapRows'))
    off = m.rw(m.data('word_A343') + 4)            # word_A341 is 4 in the cases that use this
    solid = lambda r, c: m.rb(((r << 8) | c) & 0xFFFF, seg) != 0 and         m.rb(m.data('GroundTypeTable') + m.rb(((r << 8) | c) & 0xFFFF, seg)) != 0
    gt = lambda r, c: m.rb(m.data('GroundTypeTable') + m.rb(((r << 8) | c) & 0xFFFF, seg))
    for c in range(4, 200):
        for p in range(4, rows - 6):
            bp = (p + 4) * 256 + c
            for _ in range(10):
                if bp < (rows << 8) and bp >= 0x200 and gt(bp >> 8, bp & 0xFF) != 0 and                         gt((bp - 0x100) >> 8, bp & 0xFF) == 0 and gt((bp - 0x200) >> 8, bp & 0xFF) == 0:
                    return ((c * 16 + 8 - off) & 0xFFFF, p * 16 + 8)
                bp -= 0x100
                if bp < 0x300: break
    return (0x0500, 0x0400)

def entity_cases():
    cs = []
    for kind in range(13):
        label = ENTITY_HANDLERS[kind]
        for mode in ('spawn', 'refuse', 'full'):
            def s(m, k=kind, md=mode):
                m.wb(m.data('CurrentLevel'), 4)
                m.wb(m.data('ShakeMagnitude'), 0)
                clear_pool(m)
                if md == 'full':
                    for j in range(12):
                        m.ww(POOL + 0x12 * j + 4, 0x2100)
                if md == 'refuse':
                    px, py = 0x0500, 0x0400
                    set_player(m, px, py)
                    set_cam(m, row=(py >> 4) - 8, col=(px >> 4) - 10)
                    put_entity(m, ENT, k, sprite=table_sprite(m, k) or 0x0100, flags=0, period=0xFF,
                               timer=0, X=px + 0x0800, Y=py - 0x0600,
                               params=(0x20, 0x10, 0x10, 0x18, 0x03, 0x20, 0x40))
                else:
                    if k == 10:
                        px, py = player_for_box(m)
                    else:
                        px, py = 0x0510, 0x0408
                    set_player(m, px, py)
                    set_cam(m, row=(py >> 4) - 8, col=(px >> 4) - 10)
                    ey = py - 0x0100 if k == 12 else py     # approach needs the player 100h below
                    put_entity(m, ENT, k, sprite=table_sprite(m, k) or 0x0100, flags=0, period=0,
                               timer=0, X=px, Y=ey, params=(0x20, 0x10, 0x10, 0x18, 0x03, 0x20, 0x40))
                m.ww(m.data('LastSpawnedChild'), 0)
                m.ww(m.data('word_A341'), 4)
                m.wb(m.data('SpawnSideToggle'), 0)
            cs.append(case(f'{label}: entity kind {kind}, {mode}', label, f'c_{label}', level=4, setup=s,
                           regs={'SI': ENT}, out_regs=('CF',)))
    return cs

# -- boss, hazards, level 5 and level 9 -------------------------------------------------
def boss_cases():
    cs = []
    for st in range(8):
        for pl in ('near', 'far'):
            for energy in (0x0060, 0x0018):
                def s(m, state=st, p=pl, e=energy):
                    rnd = random.Random(0xB05 + state * 5 + (e & 1))
                    reset_boss(m, state)
                    m.ww(m.data('BossEnergy'), e)
                    m.wb(m.data('BossCounter'), 0x0B if state == 3 else 2)
                    m.ww(m.data('BossVelX'), 0x0030 if state == 4 else 0)
                    m.ww(m.data('BossVelY'), 0x0010 if state == 6 else 0)
                    m.ww(m.data('BossStateTimer'), 0x58 if state == 2 else 0x20)
                    m.wb(m.data('byte_91FB'), 2)
                    if p == 'near': near(m, 0x0500, 0x0400, rnd)
                    else: far(m, 0x0500, 0x0400)
                    m.ww(m.data('PlayerVelY'), 0)
                    m.wb(m.data('RespawnState'), 0)
                    m.wb(m.data('CurrentLevel'), 4)
                    clear_pool(m)
                cs.append(case(f'UpdateBossFight: state {st}, {pl}, energy {energy:04X}', 'UpdateBossFight',
                               'c_UpdateBossFight', level=4, setup=s, out_regs=()))
    cs.append(case('UpdateBossFight: boss inactive (BossState FFh) is not called; defeat path (6, falling)',
                   'UpdateBossFight', 'c_UpdateBossFight', level=4,
                   setup=lambda m: (reset_boss(m, 6), m.ww(m.data('BossVelY'), 0x0010),
                                    set_player(m, 0x0500, 0x0400), m.wb(m.data('CurrentLevel'), 4))))
    # hazard hits: the body part at word_A425 and the part at word_A423 frame 19Dh
    for slot_frame, lab in [(0x2000 | 0x019D, 'part frame 19Dh'), (0x2000 | 0x0150, 'other frame'),
                            (0x2000 | 0x019C, 'part frame 19Ch')]:
        def sh(m, fr=slot_frame):
            reset_boss(m, 2)
            set_player(m, 0x0500, 0x0400)
            m.wb(m.data('PlayerAttackPower'), 0x20)
            m.ww(m.data('SnowFrameTick'), 0x0100)
            m.ww(m.data('word_A3FD'), 0)
            # part records at 5648h.. (sub_93B2 layout); pointers select the parts
            for k in range(5):
                rec = 0x5648 + 12 * k
                m.ww(rec, 0x0500 + 0x10 * k); m.ww(rec + 2, 0x0400); m.ww(rec + 4, 0x2000 | 0x10)
                m.wb(rec + 5, 0x20)
            m.ww(m.data('word_A423'), 0x5648 + 12 * 1)
            m.ww(m.data('word_A421'), 0x5648 + 12 * 2)
            m.ww(m.data('word_A425'), 0x5648 + 12 * 0)
            m.ww(0x5648 + 12 * 1 + 4, fr)
            m.ww(m.data('RenderSlots') + 4, 0x2100)
            m.ww(m.data('RenderSlots') + 0x12 * 2 + 4, 0x2100)
            m.ww(m.data('RenderSlots') + 0x12 * 2, 0x0500); m.ww(m.data('RenderSlots') + 0x12 * 2 + 2, 0x0400)
            m.ww(m.data('RenderSlots') + 0x12 * 3 + 4, 0x2100)
            m.ww(m.data('RenderSlots') + 0x12 * 3, 0x0500); m.ww(m.data('RenderSlots') + 0x12 * 3 + 2, 0x0400)
            m.wb(m.data('RespawnState'), 0)
            m.wb(m.data('LandingFlag'), 0)
            m.wb(m.data('CurrentLevel'), 4)
        cs.append(case(f'CheckBossHits: hazard slot hits ({lab})', 'CheckBossHits', 'c_CheckBossHits',
                       level=4, setup=sh))
    cs.append(case('CheckPlayerBossContact: contact with the body part, landing', 'CheckPlayerBossContact',
                   'c_CheckPlayerBossContact', level=4,
                   setup=lambda m: (reset_boss(m, 2), set_player(m, 0x0500, 0x03F0, vy=0x0040),
                                    m.ww(m.data('word_A425'), 0x5648),
                                    m.ww(m.data('word_A421'), 0x5648 + 12), m.ww(m.data('word_A423'), 0x5648 + 24),
                                    m.wb(m.data('LandingFlag'), 0), m.wb(m.data('RespawnState'), 0),
                                    m.ww(0x5648 + 4, 0x2000 | 0x40), m.wb(0x5648 + 5, 0x20),
                                    m.ww(0x5648 + 12 + 4, 0x2000 | 0x40), m.wb(0x5648 + 12 + 5, 0x20),
                                    m.ww(0x5648 + 24 + 4, 0x2000 | 0x40), m.wb(0x5648 + 24 + 5, 0x20),
                                    m.wb(m.data('byte_27EA'), 1), m.wb(m.data('CurrentLevel'), 4),
                                    m.ww(0x5648, 0x0500), m.ww(0x5648 + 2, 0x03F0))))
    cs.append(case('SetGaugeSprites: count 0', 'SetGaugeSprites', 'c_SetGaugeSprites', level=4,
                   setup=lambda m: None, regs={'CX': 0}))
    for n in (3, 8, 12):
        cs.append(case(f'SetGaugeSprites: count {n}', 'SetGaugeSprites', 'c_SetGaugeSprites', level=4,
                       setup=lambda m: None, regs={'CX': n}))
    return cs

def level5_cases():
    cs = []
    for phase in (0, 1, 2, 3):
        for lab, timers in [('wait', (0x00, 0x00, 0x03, 0x02, 0x02)), ('spawn', (0x01, 0x00, 0x01, 0x02, 0x00)),
                            ('contact', (0x06, 0x07, 0x02, 0x00, 0x03))]:
            def s(m, ph=phase, tm=timers):
                rnd = random.Random(0x5000 + ph * 10 + len(lab))
                m.wb(m.data('CurrentLevel'), 5)
                m.ww(m.data('Level5Phase'), ph)
                m.wb(m.data('byte_A324'), tm[0]); m.wb(m.data('byte_A325'), tm[1])
                m.wb(m.data('byte_A328'), tm[2]); m.wb(m.data('byte_A329'), tm[3])
                m.wb(m.data('byte_A32A'), tm[4]); m.wb(m.data('byte_A32B'), 0x03)
                m.wb(m.data('byte_A32C'), 0x02)
                m.wb(m.data('LevelFlags'), 0)
                m.wb(m.data('RespawnState'), 0)
                m.ww(m.data('SnowFrameTick'), 0x0010)
                reset_boss(m, 0xFF)
                set_player(m, 0x0400 + rnd.randrange(0x40), 0x0500 + rnd.randrange(0x40))
                m.ww(m.data('PlayerX'), 0x0400 + 0x10 * ph)
                for k in range(5):
                    rec = 0x7DAF + 0x0B * k
                    m.ww(rec, 0x0400 + 0x20 * k); m.ww(rec + 2, 0x0480); m.ww(rec + 4, 0x1B3 if k % 2 == 0 else 0xFFFF)
                    m.ww(rec + 6, 0x0010 * (k + 1)); m.ww(rec + 8, 0x0010 * k); m.wb(rec + 10, 0x04)
                clear_pool(m)
                m.wb(m.data('ShakeMagnitude'), 0)
            cs.append(case(f'UpdateLevel5Sequence: phase {phase}, {lab}', 'UpdateLevel5Sequence',
                           'c_UpdateLevel5Sequence', level=5, setup=s, out_regs=()))
    # UpdateObjects on level 5 with the level-5 sequence active and a child slot
    return cs

def level9_cases():
    cs = []
    # the script bytecode: FFh drift, FEh falling, FDh sound, 80h.. jump, 00h..7Fh end of group
    scripts = {
        'init': None,
        'drift and group': (0xFF, 0xFE, 0x05, 0xFD, 0x03),
        'jump back': (0x05, 0xF0, 0x06),
        'repeat counter': (0x83, 0xFE, 0x84, 0x02),
    }
    for lab, bytes_ in scripts.items():
        def s(m, b=bytes_):
            m.wb(m.data('CurrentLevel'), 9)
            set_player(m, 0x0400, 0x0400)
            m.ww(m.data('PlayerX'), 0x00B9); m.ww(m.data('PlayerY'), 0x001E)
            m.wb(m.data('RespawnState'), 0)
            clear_pool(m)
            if b is None:
                m.ww(m.data('Level9ScriptPtr'), 0xFFFF)
            else:
                m.write(DATA_SEG, SCR, bytes(b) + b'\x00')
                m.ww(m.data('Level9ScriptPtr'), SCR)
                m.ww(m.data('word_A519'), 0x0010)
                m.wb(m.data('byte_A515'), 0)
                m.ww(m.data('word_A4F7'), 0xA4F9)
        cs.append(case(f'RunLevel9Script: {lab}', 'RunLevel9Script', 'c_RunLevel9Script', level=9, setup=s))
    for panel_row in (0x00, 0x02, 0x07, 0x0C):
        cs.append(case(f'DrawLevel9Panel: row {panel_row:02X}', 'DrawLevel9Panel', 'c_DrawLevel9Panel',
                       level=9, setup=lambda m: None, regs={'AX': panel_row, 'ES': 0xA000}))
    cs.append(case('EmitLevel9Sparkles: four sparks', 'EmitLevel9Sparkles', 'c_EmitLevel9Sparkles', level=9,
                   setup=lambda m: clear_pool(m)))
    cs.append(case('SpawnDriftParticle: pool empty', 'SpawnDriftParticle', 'c_SpawnDriftParticle', level=4,
                   setup=lambda m: clear_pool(m)))
    cs.append(case('SpawnFallingParticle: pool empty', 'SpawnFallingParticle', 'c_SpawnFallingParticle',
                   level=4, setup=lambda m: clear_pool(m)))
    cs.append(case('SpawnDriftParticle: pool full', 'SpawnDriftParticle', 'c_SpawnDriftParticle', level=4,
                   setup=lambda m: [m.ww(PARTICLES + 0x12 * k + 4, 0x2100) for k in range(32)]))
    return cs

# -- copies of routines outside the region (checked against the oracle) -----------------------
def copies_cases():
    cs = []
    cs.append(case('StepRandomSeed: seed 1234h', 'StepRandomSeed', 'c_StepRandomSeed', level=4,
                   setup=lambda m: m.ww(m.data('RandomSeed'), 0x1234), out_regs=('AX',)))
    for lab, sd in [('a', 0x0000), ('b', 0xFFFF), ('c', 0x8001)]:
        cs.append(case(f'TriggerPlayerRespawnCore: lives {lab}', 'TriggerPlayerRespawnCore', 'c_TriggerPlayerRespawnCore',
                       level=4, setup=lambda m, s=sd: (m.wb(m.data('Lives'), 0 if s == 0 else 2),
                                                       m.wb(m.data('RespawnState'), 0 if s != 0xFFFF else 1),
                                                       m.wb(m.data('Energy'), 5))))
    # TestHitboxOverlap copy against the oracle: seeded pairs of records with real frames
    for k in range(24):
        def s(m, kk=k):
            rnd = random.Random(0x7E57 + kk)
            a = (rnd.randrange(0, 0x0700), rnd.randrange(0, 0x0700))
            d = (rnd.randrange(-0x50, 0x51), rnd.randrange(-0x50, 0x51))
            fr_a = 0x2000 | rnd.randrange(0x0100, 0x0400)
            fr_b = 0x2000 | rnd.randrange(0x0100, 0x0400)
            m.ww(0x3E10, a[0]); m.ww(0x3E12, a[1]); m.ww(0x3E14, fr_a)
            m.ww(0x3E18, (a[0] + d[0]) & 0xFFFF); m.ww(0x3E1A, (a[1] + d[1]) & 0xFFFF); m.ww(0x3E1C, fr_b)
            m.ww(m.data('PlayerVelY'), rnd.choice((0x0000, 0x0040, 0xFF60)))
            m.wb(m.data('FollowerOverlapMode'), 1 if kk % 3 == 0 else 0)
            m.wb(m.data('LandingFlag'), 0)
            m.ww(m.data('LandingDepth'), 0)
        def mk(kk=k):
            pass
        cs.append(case(f'TestHitboxOverlap: pair {k}', 'TestHitboxOverlap', 'c_TestHitboxOverlap', level=4,
                       setup=s, regs={'SI': 0x3E10, 'DI': 0x3E18}, out_regs=('CF',)))
    cs.append(case('SpawnEffectBurst: 4 particles, origin set, pool empty', 'SpawnEffectBurst', 'c_SpawnEffectBurst',
                   level=4, setup=lambda m: (clear_pool(m), m.ww(m.data('EffectOriginX'), 0x0120),
                                              m.ww(m.data('EffectOriginY'), 0x0090),
                                              m.ww(m.data('EffectFrame'), 0x2046)),
                   regs={'AX': 0x0030, 'DX': 0xFF80, 'CX': 4}))
    cs.append(case('SpawnEffectBurst: 40 requested, pool with 3 free', 'SpawnEffectBurst', 'c_SpawnEffectBurst',
                   level=4, setup=lambda m: (clear_pool(m), [m.ww(PARTICLES + 0x12 * k + 4, 0x2100) for k in range(29)],
                                              m.ww(m.data('EffectOriginX'), 0x0010),
                                              m.ww(m.data('EffectOriginY'), 0x0020),
                                              m.ww(m.data('EffectFrame'), 0x2137)),
                   regs={'AX': 0x0020, 'DX': 0xFF60, 'CX': 40}))
    cs.append(case('RandomDebrisFrame: seeded generator', 'RandomDebrisFrame', 'c_RandomDebrisFrame', level=4,
                   setup=lambda m: (m.ww(m.data('WindRandWord'), 0x0F0F), m.wb(m.data('WindRandA'), 0x21),
                                    m.wb(m.data('WindRandB'), 0x77), m.wb(m.data('WindRandC'), 0x9A)),
                   out_regs=('AX',)))
    cs.append(case('SpawnFourDebrisParticles: origin set', 'SpawnFourDebrisParticles', 'c_SpawnFourDebrisParticles',
                   level=4, setup=lambda m: (clear_pool(m), m.ww(m.data('EffectOriginX'), 0x0200),
                                              m.ww(m.data('EffectOriginY'), 0x0100))))
    for k in (0, 1, 7):
        cs.append(case(f'sub_93B2: animation op {k}', 'sub_93B2', 'c_sub_93B2', level=4,
                       setup=lambda m, kk=k: (m.wb(m.data('byte_A3FA'), kk & 1), m.wb(m.data('byte_A3F9'), 0x40 if kk & 2 else 0),
                                              m.ww(m.data('BossX'), 0x0500), m.ww(m.data('BossY'), 0x0400)),
                       regs={'SI': 0xA571 + 0x14 * k}))
    return cs

# -- per-frame object update: real levels, synthetic lists, sequences, random placements -----
def real_frame_cases():
    cs = []
    for lv in LEVELS:
        for n in (0, 1, 3):
            for ent in (0, 2):
                def s(m, level=lv, nb=n, idx=ent):
                    rnd = random.Random(0xF00D + level * 17 + nb * 3 + idx)
                    level_state(m, level)
                    ent_addr, kind = find_entity(m, idx)
                    if ent_addr is not None:
                        ex = m.rw(ent_addr + 9); ey = m.rw(ent_addr + 0x0B)
                        near(m, ex, ey, rnd)
                        set_cam(m, row=max(0, (ey >> 4) - 8) & 0xFFFF, col=max(0, (ex >> 4) - 10) & 0xFFFF)
                    m.wb(m.data('RespawnState'), 0)
                    for _ in range(nb):
                        m.call('UpdateObjects', budget=400_000_000)
                cs.append(Case(f'UpdateObjects: level {lv}, entity {ent}, after {n} frames', 'UpdateObjects',
                               'c_UpdateObjects', regs={}, setup=lambda m, f=s: (f(m), quiet(m)),
                               files=True, budget=400_000_000))
    return cs

def find_entity(m, idx):
    """The idx-th entity of the level's list (stride-walked), or (None, None)."""
    addr, seen = ENTITY_LIST, 0
    while m.rb(addr) < 0x32:
        if m.rb(addr + 1) < 0x80 or True:
            if seen == idx: return addr, m.rb(addr + 1)
        seen += 1
        addr += m.rb(addr)
    return None, None

def synthetic_cases():
    """A synthetic entity list with one entity of every kind near the player; frames in sequence."""
    cs = []
    for n in (0, 1, 2, 5):
        for b198 in (0, 1):
            def s(m, nb=n, bb=b198):
                rnd = random.Random(0x5EED + nb * 2 + bb)
                base(m)
                level_state(m, 4)
                m.wb(m.data('byte_B198'), bb)
                m.wb(m.data('CurrentLevel'), 4)
                px, py = 0x0500, 0x0400
                set_player(m, px, py)
                set_cam(m, row=0x0018, col=0x0026)
                clear_pool(m)
                addr = ENTITY_LIST
                for k in range(13):
                    sprite = table_sprite(m, k) or 0x0100
                    # kind bit 7 (80h) disables the entity while byte_B198 != 1
                    kind = k | 0x80 if (k == 12 and bb == 0) else k
                    put_entity(m, addr, kind, sprite=sprite, flags=0, period=0, timer=k % 3,
                               X=px + rnd.randrange(-0x20, 0x21), Y=py + 0x08 + rnd.randrange(-0x20, 0x21),
                               params=(0x20, 0x10, 0x10, 0x18, 0x03, 0x20, 0x40))
                    addr += 0x20
                m.wb(addr, 0x32)
                for _ in range(nb):
                    m.call('UpdateObjects', budget=400_000_000)
            cs.append(Case(f'UpdateObjects: synthetic entity of every kind, B198={b198}, after {n} frames',
                           'UpdateObjects', 'c_UpdateObjects', regs={}, setup=lambda m, f=s: (f(m), quiet(m)),
                           files=True, budget=400_000_000))
    # the full child pool with an active child per owner kind, and the empty pool
    for lab, full in [('full child pool', True), ('empty child pool', False)]:
        def s(m, fu=full):
            rnd = random.Random(0xCA11 if fu else 0xCA12)
            level_state(m, 4)
            set_player(m, 0x0500, 0x0400)
            set_cam(m, row=0x0018, col=0x0026)
            clear_pool(m)
            put_script(m)
            for k in range(12):
                owner = OWN + 0x20 * k
                put_entity(m, owner, k % 13, sprite=0x0100, flags=0, period=2, timer=0, X=0x0500 + 8 * k,
                           params=(0x20, 0x10, 0x10, 0x18, 0x03, 0x20, 0x40))
                if fu:
                    put_child(m, POOL + 0x12 * k, 0x0500 + 8 * k, 0x0400 + 4, frame=0x2100 | (k & 0xF),
                              xvel=0x0010 * (k % 3), yvel=0x0010, state=k % 4, owner=owner)
            m.wb(ENTITY_LIST, 0x32)
        cs.append(Case(f'UpdateObjects: {lab}', 'UpdateObjects', 'c_UpdateObjects', regs={},
                       setup=lambda m, f=s: (f(m), quiet(m)), files=True, budget=400_000_000))
    return cs

def random_cases():
    """Seeded random placements of the player, the boss and the level-5/9 state on the level."""
    cs = []
    for k in range(12):
        def s(m, kk=k):
            rnd = random.Random(0xA11 + kk)
            lv = LEVELS[kk % len(LEVELS)]
            level_state(m, lv)
            set_player(m, rnd.randrange(0x0100, 0x0A00), rnd.randrange(0x0100, 0x0700),
                       vy=rnd.choice((0, 0x40, 0xFF80)), vx=rnd.choice((0, 0x30, 0xFFD0)))
            set_cam(m, row=rnd.randrange(0, 0x20), col=rnd.randrange(0, 0x40))
            m.wb(m.data('RespawnState'), 0)
            reset_boss(m, rnd.choice((0xFF, 0, 1, 2, 3, 4, 5, 7)))
            m.ww(m.data('BossX'), rnd.randrange(0x0200, 0x0800))
            m.ww(m.data('BossY'), rnd.randrange(0x0200, 0x0700))
            clear_pool(m)
            for j in range(6):
                owner = OWN + 0x20 * j
                put_entity(m, owner, rnd.randrange(13), sprite=0x0100, flags=rnd.choice((0, 0x04, 0x08)),
                           period=rnd.randrange(0, 6), timer=rnd.randrange(0, 20),
                           X=rnd.randrange(0x0100, 0x0A00), Y=rnd.randrange(0x0100, 0x0700),
                           params=tuple(rnd.randrange(256) for _ in range(7)))
                put_child(m, POOL + 0x12 * j, rnd.randrange(0x0100, 0x0A00), rnd.randrange(0x0100, 0x0700),
                          frame=0x2000 | rnd.randrange(0x400), xvel=rnd.randrange(-0x80, 0x80),
                          yvel=rnd.randrange(-0x80, 0x80), state=rnd.choice((0, 1, 2, 3, 0x0A, 0x0B, 0xFF)),
                          owner=owner, facing=rnd.choice((0, 0x80)))
            m.wb(ENTITY_LIST, 0x32)
            m.wb(m.data('ScriptFrameFlags'), rnd.randrange(256))
        cs.append(Case(f'UpdateObjects: random placement {k}', 'UpdateObjects', 'c_UpdateObjects', regs={},
                       setup=lambda m, f=s: (f(m), quiet(m)), files=True, budget=400_000_000))
    return cs

CASES = (unit_cases() + child_cases() + entity_cases() + boss_cases() + level5_cases() + level9_cases()
         + copies_cases() + real_frame_cases() + synthetic_cases() + random_cases())
