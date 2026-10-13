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
import sys as _sys
from common import ROOT as _ROOT
_sys.path.insert(0, str(_ROOT/'tests'))
import _bios as bios
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
        bios.set_mode(m, al)
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
                       regs={'SI': POOL, 'BX': OWN, 'CX': 4}))      # CL = 4 is an input (the shift count)
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

# -- gap closing: sweeps over the branch conditions the first-generation cases never set -------
# Child records written by put_child() carry frame 21xxh/2120h: bit 20h of the record's +5 byte
# ("drawn") makes CullChildAt return at once and several handlers skip their far/fall tests, so
# every far-cull and death branch was unreachable. These sweeps use frames with the bit clear.
TILE_IDS = (0xE1, 0xE2, 0xE3, 0xE4)       # scratch tile ids written around the case position
SPOT_COL, SPOT_ROW = 0x40, 0x40           # tile (column, row) of the case position in the level-4 map

def tile_tables(m, rnd, ids=TILE_IDS):
    """Random entries of the side-class (7E5Eh), ground-type (7F5Eh) and bob-class (8E1Dh) tables."""
    for t in ids:
        m.wb(m.data('GroundTypeTable') + t, rnd.choice((0, 0, 1, 2)))
        m.wb(m.data('SideTileClassTable') + t, rnd.choice((0, 1, 1, 2)))
        m.wb(m.data('TileSolidProps') + t, rnd.choice((0x00, 0x03, 0x05, 0xF8, 0x10, 0x21, 0x31, 0x11, 0x0C)))

def set_cell(m, col, row, tile):
    m.wb(((row << 8) | col) & 0xFFFF, tile, seg=m.rw(m.data('LevelSegment')))

def tile_field(m, rnd, col=SPOT_COL, row=SPOT_ROW, ids=TILE_IDS):
    for dc in (-2, -1, 0, 1, 2):
        for dr in (-3, -2, -1, 0, 1):
            set_cell(m, col + dc, row + dr, rnd.choice(ids))

def child_free(m, si, X, Y, frame, xvel, yvel, state, owner, script=SCR, aux=0, count=0, facing=0):
    """A child record with full control of the velocity words (put_child's facing byte overwrites
    the XVel high byte)."""
    put_child(m, si, X, Y, frame=frame, state=state, owner=owner, script=script, aux=aux, count=count)
    m.ww(si + 8, xvel); m.ww(si + 0x0A, yvel)

VELS = (0, 0, 1, 0x10, 0x30, 0x7F, 0x100, 0xFFFF, 0xFFF0, 0xFFD0, 0xFF00, 0x8000, 0x00F0, 0x00F1, 0xFEFF)
OWNER_FLAGS = (0x00, 0x01, 0x02, 0x03, 0x04, 0x08, 0x09, 0x10, 0x18, 0x40, 0x41, 0x48, 0xC8, 0x49, 0x80, 0x0B)

def child_sweep(kind, state, variant, seed, over=None):
    """One child handler call: a random owner/child/player/tile configuration of the given kind,
    state and variant (0 near, 1 far, 2 near and not drawn-flagged but offset in Y, 3 child far
    below the player). Frame bit 20h is clear except in a quarter of the cases."""
    def s(m):
        rnd = random.Random(seed)
        tile_tables(m, rnd); tile_field(m, rnd)
        cx = SPOT_COL * 16 + rnd.randrange(16); cy = SPOT_ROW * 16 + rnd.randrange(16)
        flags = rnd.choice(OWNER_FLAGS)
        params = tuple(rnd.choice((0, 1, 2, 3, 5, 0x0A, 0x10, 0x14, 0x20, 0x28, 0x40, 0x7F, 0x80, 0xF0, 0xFF))
                       for _ in range(7))
        owner_rec(m, kind=kind, flags=flags, period=rnd.choice((0, 1, 2, 3, 5)),
                  timer=rnd.choice((0, 1, 3, 7, 0x0B, 0x1E, 0xFF)), X=cx + rnd.randrange(-0x20, 0x21),
                  Y=cy + rnd.randrange(-0x20, 0x21), params=params)
        if variant == 0:
            set_player(m, cx + rnd.randrange(-0x60, 0x61), cy + rnd.randrange(-0x40, 0x41))
        elif variant == 1:
            if rnd.randrange(2): set_player(m, cx + rnd.choice((-1, 1)) * rnd.randrange(0x140, 0x300), cy)
            else: set_player(m, cx, cy + rnd.choice((-1, 1)) * rnd.randrange(0x12C, 0x300))
        elif variant == 2:
            set_player(m, cx + rnd.randrange(-0x20, 0x21), cy - rnd.choice((0xA0, 0xAF, 0xB0, 0xB1, 0xC0)))
        else:
            set_player(m, cx + rnd.randrange(-0x20, 0x21), cy + rnd.choice((0x20, 0x40)))
        put_script(m)
        frame = rnd.choice((0x0100, 0x4100, 0x0120, 0x4101)) if rnd.randrange(4) else 0x2100
        child_free(m, POOL, cx, cy, frame, rnd.choice(VELS), rnd.choice(VELS), state, OWN,
                   aux=rnd.randrange(256), count=rnd.randrange(256))
        m.wb(OWN + 0x0E, rnd.choice((0x12, 0x13, 0x14, 0x03, 0x00, 0x80)))
        m.wb(m.data('ScriptFrameFlags'), rnd.choice((0, 0x20, 0x40, 0x80, 0xFF)))
        m.wb(m.data('AttackAnimLock'), rnd.randrange(2))
        m.wb(m.data('ShakeMagnitude'), rnd.choice((0, 0, 4)))
        m.wb(m.data('CurrentLevel'), rnd.choice((4, 5)))
        m.ww(m.data('SnowFrameTick'), rnd.randrange(0x10))
        m.ww(m.data('RandomSeed'), rnd.randrange(0x10000))
        clear_debris(m)
        for k, v in (over or {}).items():          # targeted overrides after the random draws
            if k == 'xvel': m.ww(POOL + 8, v)
            elif k == 'yvel': m.ww(POOL + 0x0A, v)
            elif k == 'x': m.ww(POOL, v)
            elif k == 'px': m.ww(m.data('PlayerX'), v)
            elif k == 'py': m.ww(m.data('PlayerY'), v)
            elif k == 'oflags': m.wb(OWN + 4, v)
            elif k == 'otimer': m.wb(OWN + 7, v)
            elif k == 'operiod': m.wb(OWN + 6, v)
            elif k == 'oparam': m.wb(OWN + 0x0D + v[0], v[1])
            elif k == 'frame': m.ww(POOL + 4, v)
            elif k == 'sff': m.wb(m.data('ScriptFrameFlags'), v)
            elif k == 'tick': m.ww(m.data('SnowFrameTick'), v)
            elif k == 'level': m.wb(m.data('CurrentLevel'), v)
            elif k == 'shake': m.wb(m.data('ShakeMagnitude'), v)
            elif k == 'lock': m.wb(m.data('AttackAnimLock'), v)
            elif k == 'wind':
                wr = random.Random(v)
                m.ww(m.data('WindRandWord'), wr.randrange(0x10000))
                for nm in ('WindRandA', 'WindRandB', 'WindRandC'): m.wb(m.data(nm), wr.randrange(256))
            elif k in ('dpx', 'dpy'): pass
            else: raise KeyError(k)
        if over and 'dpx' in over: m.ww(m.data('PlayerX'), (m.rw(POOL) + over['dpx']) & 0xFFFF)
        if over and 'dpy' in over: m.ww(m.data('PlayerY'), (m.rw(POOL + 2) + over['dpy']) & 0xFFFF)
    return s

def child_sweep_cases():
    cs = []
    # targeted: the branches the random sweep reaches only by chance
    T = [
        (0, 1, dict(yvel=0, px=0x0500), 'ground state 1 landed, player right'),
        (0, 1, dict(yvel=0, px=0x0300), 'ground state 1 landed, player left'),
        (0, 1, dict(yvel=0x10), 'ground state 1 still falling'),
        (3, 2, dict(x=0xFFE0), 'walker state 2 with negative X'),
        (3, 2, dict(x=0x0420), 'walker state 2 with positive X'),
        (10, 0, dict(yvel=0), 'charger state 0 landed'),
        (10, 0, dict(yvel=0x20), 'charger state 0 airborne'),
        (10, 1, dict(yvel=0), 'charger state 1 landed -> state 2'),
        (10, 1, dict(yvel=8), 'charger state 1 airborne -> state 0'),
        (10, 2, dict(xvel=0, sff=0x40, tick=4, otimer=1), 'charger state 2 starts the charge'),
        (10, 2, dict(xvel=0x0200, sff=0, tick=4, otimer=1), 'charger state 2 timer expires'),
        (10, 2, dict(xvel=0xFE00, sff=0, tick=3, otimer=1), 'charger state 2 off-beat tick'),
        (10, 2, dict(xvel=0, sff=0, otimer=2), 'charger state 2 stationary, no script flags'),
        (10, 3, dict(sff=0x40), 'charger state 3 with script flags (death)'),
        (10, 3, dict(sff=0), 'charger state 3 without flags'),
        (10, 0, dict(level=5, shake=4), 'charger on level 5 with shake'),
        (10, 3, dict(level=5, shake=4), 'charger state 3 on level 5 with shake'),
        (10, 0xFF, dict(level=5, shake=4), 'charger state FFh on level 5 with shake'),
        (10, 2, dict(level=5, shake=4, xvel=0, sff=0x40, tick=4, otimer=7), 'charger state 2 on level 5 with shake'),
        (12, 1, dict(otimer=0x99), 'launched state 1 times out (owner timer reaches 9Ah)'),
        (12, 1, dict(otimer=0x20), 'launched state 1 counting'),
        (12, 1, dict(otimer=0x20, frame=0x2100), 'launched state 1 drawn resets the owner timer'),
        (7, 0, dict(dpx=-0x20, dpy=0), 'leaper state 0, player left of the child in range'),
        (7, 0, dict(dpx=0x20, dpy=0), 'leaper state 0, player right of the child in range'),
        (7, 0, dict(dpx=0x300, dpy=0), 'leaper state 0, player right out of range'),
        (7, 0, dict(dpx=-0x300, dpy=0), 'leaper state 0, player left out of range'),
        (6, 1, dict(dpx=0x20, dpy=0x10, lock=1), 'wanderer state 1 homing, player right, dY < 30h'),
        (6, 1, dict(dpx=-0x20, dpy=0x10, lock=1), 'wanderer state 1 homing, player left, dY < 30h'),
        (6, 1, dict(dpx=0x08, dpy=0x38, lock=1), 'wanderer state 1 homing, dY >= 30h'),
        (6, 1, dict(dpx=0x08, dpy=-0x38, lock=1), 'wanderer state 1 homing, player above'),
        (6, 1, dict(dpx=0x0A, dpy=0x08, lock=1), 'wanderer state 1 homing close (d2 < 0E10h)'),
        (6, 1, dict(dpx=0x50, dpy=0x08, lock=1), 'wanderer state 1 homing mid (0E10h <= d2 < 2710h)'),
        (6, 1, dict(dpx=0x50, dpy=0x08, lock=0), 'wanderer state 1, no attack lock'),
        (6, 1, dict(dpx=0x80, dpy=0x20, lock=1), 'wanderer state 1 far (random steps)'),
        (6, 0, dict(dpx=0x20, dpy=0x00), 'wanderer state 0 in range'),
        (6, 0, dict(dpx=0x280, dpy=0x00), 'wanderer state 0 out of range'),
    ]
    for w in range(24):      # the random-step branch: owner counter negative, varying generator state
        T.append((6, 1, dict(dpx=(-1) ** w * (0x70 + 4 * w), dpy=(-1) ** (w // 2) * (0x10 + 3 * w), lock=w & 1,
                             oparam=(3, 0), wind=0x5100 + w), f'wanderer state 1 random step, generator {w}'))

    for n, (kind, st, over, lab) in enumerate(T):
        for variant in (0, 3):
            cs.append(case(f'{CHILD_HANDLERS[kind]}: targeted {lab} (variant {variant})', CHILD_HANDLERS[kind],
                           f'c_{CHILD_HANDLERS[kind]}', level=4,
                           setup=child_sweep(kind, st, variant, 0xDE700000 + n * 10 + variant,
                                             dict(over, frame=0x0100, oflags=0x00 if kind != 10 else 0x01)),
                           regs={'SI': POOL}))
    for kind, h in enumerate(CHILD_HANDLERS):
        for si_, st in enumerate(CHILD_STATES):
            for variant in range(4):
                seed = 0xC0DE0000 + kind * 1000 + si_ * 10 + variant
                cs.append(case(f'{h}: sweep state {st:02X}, variant {variant}', h, f'c_{h}', level=4,
                               setup=child_sweep(kind, st, variant, seed), regs={'SI': POOL}))
    return cs

def terrain_cases():
    """UpdateObjectTerrain directly: owner flag combinations 08h/40h/80h/20h, the sign of XVel, the
    three tiles it reads (here, above, above-ahead) with random table entries, YVel ranges."""
    cs = []
    n = 0
    for flags in (0x08, 0x48, 0xC8, 0x88, 0x28, 0x68, 0xE8):
        for xs in (-1, 0, 1):
            for rep in range(3):
                seed = 0x7E000000 + n; n += 1
                def s(m, fl=flags, xsign=xs, sd=seed):
                    rnd = random.Random(sd)
                    tile_tables(m, rnd); tile_field(m, rnd)
                    cx = SPOT_COL * 16 + rnd.randrange(16); cy = SPOT_ROW * 16 + rnd.randrange(16)
                    owner_rec(m, kind=3, flags=fl, X=cx, Y=cy)
                    put_script(m)
                    xv = 0 if xsign == 0 else xsign * rnd.choice((0x10, 0x18, 0x30, 0x100))
                    child_free(m, POOL, cx, cy, 0x0100, xv & 0xFFFF,
                               rnd.choice((0, 0x10, 0x30, 0xFFF0, 0xFFC0, 0xFF00, 0x0100, 0x00F8, 0x8000, 0x0021)),
                               0, OWN, script=SCR + 4)
                    m.wb(m.data('ShakeMagnitude'), 0)
                cs.append(case(f'UpdateObjectTerrain: owner flags {flags:02X}, X direction {xs}, seed {n}',
                               'UpdateObjectTerrain', 'c_UpdateObjectTerrain', level=4, setup=s,
                               regs={'SI': POOL}))
    return cs

def entity_gap_cases():
    cs = []
    # SpawnChildOnApproach: player relative to the entity on both sides, around the 140h/280h and
    # B4h/168h thresholds, timer saturation, period gate, full pool
    PX, PY = 0x0510, 0x0408
    def ap(dx, dy, period=0, timer=0, full=False, flags=0):
        def s(m):
            m.wb(m.data('CurrentLevel'), 4); m.wb(m.data('ShakeMagnitude'), 0)
            clear_pool(m)
            if full:
                for j in range(12): m.ww(POOL + 0x12 * j + 4, 0x2100)
            set_player(m, PX, PY)
            set_cam(m, row=(PY >> 4) - 8, col=(PX >> 4) - 10)
            put_entity(m, ENT, 12, sprite=table_sprite(m, 12) or 0x0100, flags=flags, period=period,
                       timer=timer, X=(PX + dx) & 0xFFFF, Y=(PY - dy) & 0xFFFF,
                       params=(0x20, 0x10, 0x10, 0x18, 0x03, 0x20, 0x40))
            m.ww(m.data('LastSpawnedChild'), 0)
        return s
    for dx in (0, 0x13F, -0x13F, 0x140, -0x140, 0x27F, -0x27F, 0x280, -0x280):
        for dy in (0x0B4, 0x0B5, 0x167, 0x168, 0x100, 0, -0x10):
            cs.append(case(f'SpawnChildOnApproach: entity X {dx:+#x} from the player, player {dy:#x} below',
                           'SpawnChildOnApproach', 'c_SpawnChildOnApproach', level=4, setup=ap(dx, dy),
                           regs={'SI': ENT}, out_regs=('CF',)))
    for lab, kw in [('period gate not reached', dict(period=0x10, timer=0x20)),
                    ('timer at 0FFh saturates', dict(period=0x10, timer=0xFF)),
                    ('timer 3Fh equals the period', dict(period=0x0F, timer=0x3C)),
                    ('full pool', dict(full=True))]:
        cs.append(case(f'SpawnChildOnApproach: {lab}', 'SpawnChildOnApproach', 'c_SpawnChildOnApproach', level=4,
                       setup=ap(-0x200, 0x100, **kw), regs={'SI': ENT}, out_regs=('CF',)))
    # SpawnChildWithFlags: level 6 sets bit 80h, other levels not; visible or not; full pool
    for lv_, vis, full in [(6, True, False), (4, True, False), (6, False, False), (6, True, True)]:
        def sw(m, lvn=lv_, v=vis, f=full):
            m.wb(m.data('CurrentLevel'), lvn); m.wb(m.data('ShakeMagnitude'), 0)
            clear_pool(m)
            if f:
                for j in range(12): m.ww(POOL + 0x12 * j + 4, 0x2100)
            set_player(m, PX, PY)
            set_cam(m, row=(PY >> 4) - 8, col=(PX >> 4) - 10)
            put_entity(m, ENT, 9, sprite=table_sprite(m, 9) or 0x0100, flags=0x20, period=0, timer=0,
                       X=PX if v else PX + 0x0800, Y=PY, params=(0x20, 0x10, 0x10, 0x18, 0x03, 0x20, 0x40))
            m.wb(ENT + 0x11, 0x55)
        cs.append(case(f'SpawnChildWithFlags: level {lv_}, {"visible" if vis else "off screen"}'
                       f'{", full pool" if full else ""}', 'SpawnChildWithFlags', 'c_SpawnChildWithFlags',
                       level=4, setup=sw, regs={'SI': ENT}, out_regs=('CF',)))
    # SpawnFallingParticle with the leading particle records in use
    for used in (1, 5, 31, 32):
        def sp(m, u=used):
            clear_pool(m)
            for k in range(u): m.ww(PARTICLES + 0x12 * k + 4, 0x2100)
            m.ww(m.data('WindRandWord'), 0x1234)
        cs.append(case(f'SpawnFallingParticle: first {used} particle records in use', 'SpawnFallingParticle',
                       'c_SpawnFallingParticle', level=4, setup=sp))
    return cs

# -- the shared tails and helpers entered directly at their own labels -------------------------
def helper_cases():
    cs = []
    # ChildDeath (loc_7CFF): BX = owner; owner flag 02h keeps the entity, others retire it
    for fl in (0x00, 0x02, 0x04, 0x06, 0x17, 0xFF):
        def sd(m, f=fl):
            owner_rec(m, flags=f, timer=0x33)
            put_child(m, POOL, 0x0400, 0x0400, frame=0x0100, owner=OWN)
        cs.append(case(f'ChildDeath: owner flags {fl:02X}', 'loc_7CFF', 'c_ChildDeath', level=4, setup=sd,
                       regs={'SI': POOL, 'BX': OWN}))
    # ChildFallOrDie (loc_7CDA): AL = state, BX = owner
    for st in (0x00, 0x0A, 0xFF):
        for fl in (0x00, 0x01):
            for frame in (0x0100, 0x2100):
                for py, yv in ((0x0300, 0x0000), (0x0500, 0x0000), (0x0500, 0x00EF), (0x0500, 0x00F0),
                               (0x0500, 0x8000), (0x0500, 0xFFFF)):
                    def sf(m, s_=st, f=fl, fr=frame, py_=py, yv_=yv):
                        owner_rec(m, flags=f)
                        set_player(m, 0x0400, py_)
                        child_free(m, POOL, 0x0400, 0x0400, fr, 0, yv_, s_, OWN)
                    cs.append(case(f'ChildFallOrDie: state {st:02X}, owner flags {fl:02X}, frame {frame:04X}, '
                                   f'player Y {py:04X}, YVel {yv:04X}', 'loc_7CDA', 'c_ChildFallOrDie',
                                   level=4, setup=sf, regs={'SI': POOL, 'BX': OWN, 'AX': st}))
    # HopStart (loc_7827): BX = owner, CL = 4
    for px in (0x0300, 0x0400, 0x0500):
        for p0e, p0f, fl in ((0x10, 0x08, 0x00), (0xF0, 0xF8, 0xFF), (0x00, 0x00, 0x2C), (0x7F, 0x80, 0x10)):
            def sh(m, px_=px, a=p0e, b=p0f, f=fl):
                owner_rec(m, flags=f, params=(0x20, a, b, 0x18, 0x02, 0x20, 0x40))
                set_player(m, px_, 0x0400)
                put_script(m)
                put_child(m, POOL, 0x0400, 0x0400, frame=0x0100, owner=OWN)
            cs.append(case(f'HopStart: player X {px:04X}, owner +0E/+0F {p0e:02X}/{p0f:02X}, flags {fl:02X}',
                           'loc_7827', 'c_HopStart', level=4, setup=sh, regs={'SI': POOL, 'BX': OWN, 'CX': 4}))
    # SpawnChildAtOwner: SI = entity, free pool / full pool
    for lab, free in (('empty pool', 0), ('only slot 11 free', 11), ('full pool', None)):
        def ss(m, fr=free):
            clear_pool(m)
            for k in range(12):
                if k != fr: m.ww(POOL + 0x12 * k + 4, 0x2100)
            put_entity(m, ENT, 1, sprite=0x01A5, flags=0x20, X=0x0123, Y=0x0345)
            m.ww(m.data('LastSpawnedChild'), 0x7777)
        cs.append(case(f'SpawnChildAtOwner: {lab}', 'SpawnChildAtOwner', 'c_SpawnChildAtOwner', level=4,
                       setup=ss, regs={'SI': ENT}, out_regs=('CF', 'DI')))
    # CullChildFarFromPlayer entered directly (SI = child): near, far, drawn, death state, state FF
    for lab, X, st, fr in (('near', 0x0400, 1, 0x0100), ('far X', 0x0800, 1, 0x0100),
                           ('far X, state 0Ah', 0x0800, 0x0A, 0x0100), ('far X, drawn', 0x0800, 1, 0x2100),
                           ('far X, state FFh', 0x0800, 0xFF, 0x0100), ('far Y', 0x0400, 1, 0x0100)):
        def sc(m, x=X, s_=st, f=fr, lab_=lab):
            set_player(m, 0x0400, 0x0400)
            owner_rec(m, flags=0x07, timer=9)
            put_child(m, POOL, x, 0x0900 if lab_ == 'far Y' else 0x0400, frame=f, state=s_, owner=OWN)
        cs.append(case(f'CullChildFarFromPlayer: {lab}', 'CullChildFarFromPlayer', 'c_CullChildFarFromPlayer',
                       level=4, setup=sc, regs={'SI': POOL}))
    # IncrementBossCounter: saturation
    for v in (0, 1, 0xFE, 0xFF):
        cs.append(case(f'IncrementBossCounter: counter {v:02X}', 'IncrementBossCounter', 'c_IncrementBossCounter',
                       level=4, setup=lambda m, vv=v: m.wb(m.data('BossCounter'), vv)))
    cs.append(case('unused_81B0: stc; ret (no caller)', 'unused_81B0', 'c_unused_81B0', level=4,
                   setup=lambda m: None, out_regs=('CF',)))
    return cs

# -- boss part contacts and hazard hits: parameter sweeps -------------------------------------------
BP = lambda k: 0x5648 + 0x12 * k          # boss part records (word_A421/A423/A425 point at three of them)
HAZ = (0x4F0A, 0x4F2E, 0x4F40, 0x4F52, 0x4F64)       # CheckBossHits slots, in scan order

def frame_box(m, frame, w=0x20, h=0x20, hot=0x10):
    """Width, height and hot X of a frame (index frame*2): both sides read the same box."""
    idx = (frame & 0x1FFF) * 2
    m.wb(m.data('FrameWidths') + idx, w); m.wb(m.data('FrameHeights') + idx, h)
    m.wb(m.data('FrameHotX') + idx, hot)

def part_rec(m, k, X, Y, frame, drawn=True):
    a = BP(k)
    frame_box(m, frame)
    m.ww(a, X); m.ww(a + 2, Y); m.ww(a + 4, (0x2000 if drawn else 0) | frame)

def boss_contact_scene(seed, forced=None):
    """Player against the three boss parts (A421, A423, A425), random overlaps and flags."""
    forced = forced or {}
    def s(m):
        rnd = random.Random(seed)
        reset_boss(m, rnd.choice((0, 1, 2, 3, 4, 5)))
        px, py = 0x0500, 0x0400
        frame_box(m, 2, 0x20, 0x30, 0x10)
        m.ww(PLAYER + 4, 2)
        vy = forced.get('vy', rnd.choice((0, 0x40, 0x7F, 0x80, 0x100, 0xFF80)))
        set_player(m, px, py, vy=vy)
        m.ww(m.data('BossX'), forced.get('bossx', rnd.choice((0x04F0, 0x0500, 0x0510))))
        m.wb(m.data('byte_91FB'), rnd.choice((0, 1, 2, 3, 4, 5)))
        m.wb(m.data('BossCounter'), rnd.choice((0, 1, 3, 4, 0x7F, 0x80, 0xFB, 0xFF)))
        m.wb(m.data('Energy'), rnd.choice((0, 1, 3, 8)))
        m.wb(m.data('byte_6BC9'), rnd.choice((0, 1, 2, 5, 0xFF)))
        m.wb(m.data('Lives'), rnd.choice((0, 2)))
        m.wb(m.data('RespawnState'), forced.get('respawn', rnd.choice((0, 0, 0, 1))))
        m.wb(m.data('byte_27EA'), forced.get('b27ea', rnd.randrange(2)))
        m.wb(m.data('PlayerAirFlags'), rnd.choice((0, 1, 2)))
        m.wb(m.data('LandingFlag'), rnd.randrange(2))
        m.wb(m.data('FollowerOverlapMode'), 0)
        kinds = [rnd.choice(('hit', 'hit', 'miss', 'free', 'undrawn')) for _ in range(3)]
        for k, kind in enumerate(kinds):
            dy = rnd.choice((0, 0x08, -0x08, 0x10, -0x10, 0x18, -0x20))
            dx = rnd.choice((0, 0x10, -0x10, 0x20, -0x20))
            frame = rnd.choice((0x40, 0x41, 0x19C, 0x19D))
            if kind == 'hit': part_rec(m, k, px + dx, py + dy, frame)
            elif kind == 'miss': part_rec(m, k, px + 0x200, py, frame)
            elif kind == 'undrawn': part_rec(m, k, px + dx, py + dy, frame, drawn=False)
            else: part_rec(m, k, px, py, 0x1FFF); m.ww(BP(k) + 4, 0xFFFF)
        m.ww(m.data('word_A421'), BP(0)); m.ww(m.data('word_A423'), BP(1)); m.ww(m.data('word_A425'), BP(2))
        m.wb(m.data('CurrentLevel'), 4)
        clear_pool(m)
    return s

def boss_hit_scene(seed, forced=None):
    """Hazard slots against the parts at word_A423 (frames 19Ch/19Dh free the slot) and word_A425
    (the body: a hit damages the boss), with random cooldown, attack power and energy."""
    forced = forced or {}
    def s(m):
        rnd = random.Random(seed)
        reset_boss(m, rnd.choice((0, 1, 2, 3, 4, 5, 7)))
        bx_, by_ = 0x0500, 0x0400
        m.ww(m.data('BossX'), bx_); m.ww(m.data('BossY'), by_)
        m.ww(m.data('PlayerX'), forced.get('px', rnd.choice((0x04C0, 0x0500, 0x0540))))
        m.ww(m.data('PlayerY'), 0x0400)
        m.ww(m.data('BossEnergy'), forced.get('energy', rnd.choice((0x0001, 0x0003, 0x0010, 0x0040, 0x0064))))
        m.wb(m.data('PlayerAttackPower'), forced.get('power', rnd.choice((1, 2, 3, 0x14, 0x15, 0x20, 0xFF))))
        m.ww(m.data('BossVelY'), forced.get('bvy', rnd.choice((0, 0x40, 0xFFC0, 0xFF00))))
        m.ww(m.data('SnowFrameTick'), 0x0100)
        m.ww(m.data('word_A3FD'), forced.get('last', rnd.choice((0x0100, 0x00EA, 0x00E9, 0x00FF, 0x0116, 0x0117, 0x0200))))
        m.wb(m.data('PlayerAltModeFlag'), rnd.randrange(2))
        m.wb(m.data('BossCounter'), rnd.choice((0, 5, 0xFF)))
        for a in (0x564D, 0x565F, 0x5671, 0x5683, 0x5695): m.wb(a, rnd.randrange(256))
        # body part at A425 and hazard part at A423
        m.ww(m.data('word_A425'), BP(2)); m.ww(m.data('word_A423'), BP(1)); m.ww(m.data('word_A421'), BP(0))
        part_rec(m, 2, bx_, by_, 0x40)
        pf = rnd.choice((0x19D, 0x19C, 0x40, 0x19E))
        part_rec(m, 1, bx_ + rnd.choice((0, 0x10, 0x80)), by_, pf)
        part_rec(m, 0, bx_, by_, 0x41)
        for sl in HAZ:
            mode = rnd.choice(('free', 'free', 'on_body', 'on_hazard', 'away'))
            frame_box(m, 0x50)
            if mode == 'free': m.ww(sl + 4, 0xFFFF)
            else:
                tx = {'on_body': bx_, 'on_hazard': m.rw(BP(1)), 'away': bx_ + 0x300}[mode]
                m.ww(sl, tx + rnd.choice((0, 4, -4))); m.ww(sl + 2, by_ + rnd.choice((0, 4, -4)))
                m.ww(sl + 4, 0x0050)
        m.wb(m.data('CurrentLevel'), 4)
        clear_pool(m)
    return s

def boss_direct_cases():
    cs = []
    # ApplyBossPartContact (DI = part): one part against the player
    for n in range(48):
        def s(m, nn=n):
            rnd = random.Random(0xA11D0000 + nn)
            boss_contact_scene(0xA11D1000 + nn)(m)
            kind = nn % 4        # 0 hit, 1 not drawn, 2 free, 3 miss
            part_rec(m, 3, 0x0500 + rnd.choice((0, 0x10, -0x10)), 0x0400 + rnd.choice((0, 8, -8)), 0x42,
                     drawn=(kind != 1))
            if kind == 2: m.ww(BP(3) + 4, 0xFFFF)
            if kind == 3: m.ww(BP(3), 0x0900)
        cs.append(case(f'ApplyBossPartContact: scene {n} ({("hit", "undrawn", "free", "miss")[n % 4]})',
                       'ApplyBossPartContact', 'c_ApplyBossPartContact', level=4, setup=s, regs={'DI': BP(3), 'SI': PLAYER}))
    # CheckPlayerBossContact: three parts against the player, bounce and landing
    # DrainPlayerEnergy: hurt counter wrap, energy underflow into the respawn core, full particle pool
    for b6c9 in (0, 1, 5, 0xFF):
        for en in (0, 1, 3):
            for lives, full in ((2, False), (0, False), (2, True)):
                def sdr(m, b=b6c9, e=en, l=lives, f=full):
                    m.wb(m.data('byte_6BC9'), b); m.wb(m.data('Energy'), e); m.wb(m.data('Lives'), l)
                    m.wb(m.data('RespawnState'), 0)
                    set_player(m, 0x0500, 0x0400)
                    clear_pool(m)
                    if f:
                        for k in range(32): m.ww(PARTICLES + 0x12 * k + 4, 0x2100)
                cs.append(case(f'DrainPlayerEnergy: counter {b6c9:02X}, energy {en}, lives {lives}'
                               f'{", full pool" if full else ""}', 'DrainPlayerEnergy', 'c_DrainPlayerEnergy',
                               level=4, setup=sdr))
    for n in range(72):
        cs.append(case(f'CheckPlayerBossContact: scene {n}', 'CheckPlayerBossContact', 'c_CheckPlayerBossContact',
                       level=4, setup=boss_contact_scene(0xC0C00000 + n)))
    # CheckHazardHitsBoss (SI = slot) and CheckBossHits (the five-slot pass) and the BossHit tail
    for n in range(40):
        def sh(m, nn=n):
            boss_hit_scene(0xB0550000 + nn)(m)
        cs.append(case(f'CheckHazardHitsBoss: scene {n}', 'CheckHazardHitsBoss', 'c_CheckHazardHitsBoss',
                       level=4, setup=sh, regs={'SI': HAZ[n % 5]}, out_regs=('CF',)))
    for n in range(80):
        cs.append(case(f'CheckBossHits: scene {n}', 'CheckBossHits', 'c_CheckBossHits', level=4,
                       setup=boss_hit_scene(0xB0560000 + n)))
    for n in range(48):
        def sb(m, nn=n):
            boss_hit_scene(0xB0570000 + nn)(m)
            m.ww(HAZ[nn % 5] + 4, 0x0050)
        cs.append(case(f'BossHit: scene {n}', 'loc_80F7', 'c_BossHit', level=4, setup=sb,
                       regs={'SI': HAZ[n % 5]}))
    return cs

# -- UpdateBossFight sweep on the real boss levels (2 and 8) and level 4 ------------------------
BOSS_TIMERS = {0: (0, 1), 1: (0, 0x6C, 0x6D, 0x6E), 2: (0, 0x2A, 0x2B, 0x2C, 0x57, 0x58, 0x59),
               3: (0, 0x41, 0x42, 0x43, 0x99, 0x9A, 0x9B), 4: (0, 0x41, 0x42, 0x43), 5: (0, 0x12, 0x13, 0x14),
               6: (0,), 7: (0, 0x2B, 0x2C)}
BOSS_ANIMS = (0xA427, 0xA434, 0xA43B, 0xA45E, 0xA460, 0xA462, 0xA464, 0xA46B, 0xA470, 0xA475, 0xA47E, 0xA480)

def sar4(v):
    v &= 0xFFFF
    return (v - 0x10000 if v & 0x8000 else v) >> 4

def boss_fight_scene(state, seed, lv, forced=None):
    forced = forced or {}
    def s(m):
        rnd = random.Random(seed)
        def pick(key, seq):
            v = rnd.choice(seq)
            return forced.get(key, v)
        reset_boss(m, state)
        if lv == 4:
            m.ww(m.data('word_91F7'), 0x0300); m.ww(m.data('word_91F9'), 0x0900)
            bx = rnd.choice((0x0300, 0x0480, 0x0900))
            by = 0x0400
        else:
            lo, hi = m.rw(m.data('word_91F7')), m.rw(m.data('word_91F9'))
            bx = rnd.choice((lo, lo + 0x10, (lo + hi) // 2, hi - 0x10, hi))
            by = m.rw(m.data('BossY')) + rnd.choice((0, 0, 0x10, -0x10))
        vx = pick('vx', (0, 0, 0x30, -0x30, 0x50, -0x50, 0x1000, -0x1000))
        vy = pick('vy', (0, 0, 0x10, 0xE0, 0xE1, -0x10, -0x60, 0x40, 0x100))
        m.ww(m.data('BossX'), bx); m.ww(m.data('BossY'), by)
        m.ww(m.data('BossVelX'), vx & 0xFFFF); m.ww(m.data('BossVelY'), vy & 0xFFFF)
        lo, hi = m.rw(m.data('word_91F7')), m.rw(m.data('word_91F9'))
        sx = bx + sar4(vx)
        if 0 <= sx and lo <= sx <= hi: bx2 = sx
        else: bx2 = bx
        by2 = by + sar4(vy)
        # the tile under the boss after the move (read only when VelY >= 0)
        tile = 0xEF
        col, row = (bx2 >> 4) & 0xFF, (by2 >> 4) & 0xFF
        set_cell(m, col, row, tile)
        m.wb(m.data('GroundTypeTable') + tile, pick('ground', (0, 1, 1)))
        if rnd.randrange(4) == 0:
            by2 &= 0xFFF0
        dist = pick('dist', (0, 0x10, 0x19, 0x1A, 0x23, 0x24, 0x4A, 0x4B, 0x4F, 0x50, 0x51, 0x63, 0x64, 0x80, 0xF9, 0xFA,
                           0xFB, 0x18F, 0x190, 0x191, 0x300))
        sgn = pick('sgn', (-1, 1))
        px = (bx2 + sgn * dist) & 0xFFFF
        dy = pick('dy', (0, 0x20, -0x20, 0xF9, 0xFA, 0xFB, -0xFA, -0xFB))
        set_player(m, px, (by2 + dy) & 0xFFFF)
        m.ww(m.data('BossStateTimer'), pick('timer', BOSS_TIMERS[state]))
        m.wb(m.data('BossCounter'), pick('counter', (0, 2, 3, 4, 9, 0x0A, 0x0B, 0xFF)))
        m.ww(m.data('BossEnergy'), pick('energy', (1, 0x18, 0x27, 0x28, 0x29, 0x32, 0x3B, 0x3C, 0x3D, 0x4B, 0x63, 0x64, 0x65, 0x96)))
        m.wb(m.data('AttackAnimLock'), forced.get('lock', rnd.randrange(2)))
        m.ww(m.data('SnowFrameTick'), forced.get('tick', rnd.randrange(0x20)))
        m.wb(m.data('byte_91FB'), pick('b91fb', (0, 1, 2, 3, 4, 5)))
        m.ww(m.data('word_564C'), rnd.choice((0xFFFF, 0x2035, 0x0035, 0x2000)))
        anim = pick('anim', BOSS_ANIMS)
        m.ww(m.data('BossAnimPtr'), anim)
        m.ww(m.data('word_6C0A'), anim if rnd.randrange(2) else 0)
        m.ww(m.data('word_A3FF'), forced.get('a3ff', rnd.randrange(0xA427, 0xA490)))
        m.wb(m.data('RespawnState'), rnd.choice((0, 0, 1)))
        m.wb(m.data('Energy'), rnd.choice((0, 3, 8)))
        m.wb(m.data('byte_6BC9'), rnd.choice((0, 1, 5)))
        m.wb(m.data('Lives'), 2)
        m.wb(m.data('PlayerAttackPower'), rnd.choice((1, 2, 0x14, 0x15, 0x20)))
        m.ww(m.data('word_A3FD'), rnd.choice((0x0000, 0x0100, 0x1000)))
        m.wb(m.data('LandingFlag'), 0)
        m.wb(m.data('ShakeMagnitude'), 0)
        m.wb(m.data('byte_27EA'), rnd.randrange(2))
        m.ww(m.data('PlayerVelY'), rnd.choice((0, 0x40, 0x100)))
        clear_pool(m)
        # hazard slots on the boss body (the animation op places the parts around BossX/BossY)
        for sl in HAZ:
            if rnd.randrange(3) == 0:
                frame_box(m, 0x50)
                m.ww(sl, (bx2 + rnd.randrange(-0x20, 0x40)) & 0xFFFF); m.ww(sl + 2, (by2 - rnd.randrange(0, 0x50)) & 0xFFFF)
                m.ww(sl + 4, 0x0050)
    return s

def boss_fight_cases():
    cs = []
    for st in range(8):
        for n in range(40):
            lv = (2, 8, 4, 2)[n % 4]
            cs.append(case(f'UpdateBossFight: sweep state {st}, level {lv}, scene {n}', 'UpdateBossFight',
                           'c_UpdateBossFight', level=lv, setup=boss_fight_scene(st, 0xB05F0000 + st * 100 + n, lv)))
    # targeted: thresholds and exits the random scenes reach only by chance (ground = no support under)
    T = [
        (1, dict(timer=0x10, counter=5, lock=0, energy=0x50, dist=0x80, vy=0, vx=0), 'state 1 counter 4..9 -> state 2'),
        (1, dict(timer=0x10, counter=0x0A, lock=0, energy=0x50, dist=0x80, vy=0, vx=0), 'state 1 counter >= 0Ah -> state 3'),
        (1, dict(timer=0x10, counter=2, lock=0, energy=0x50, dist=0x80, vy=0, vx=0), 'state 1 counter <= 3 stays'),
        (1, dict(timer=0x10, counter=2, lock=1, tick=8, energy=0x50, dist=0x80, vy=0, vx=0), 'state 1 lock, tick&7 = 0: counter++'),
        (1, dict(timer=0x10, counter=2, lock=1, tick=9, energy=0x50, dist=0x80, vy=0, vx=0), 'state 1 lock, tick&7 != 0'),
        (1, dict(timer=0x6D, counter=2, energy=0x50, dist=0x80, vy=0, vx=0), 'state 1 timer reaches 6Eh -> state 2'),
        (1, dict(timer=0x10, counter=2, energy=0x3B, dist=0x80, vy=0, vx=0), 'state 1 low energy -> state 2'),
        (1, dict(timer=0x10, counter=2, energy=0x50, dist=0xFA, vy=0, vx=0), 'state 1 player far -> state 0'),
        (0, dict(dist=0x80, vy=0, vx=0), 'state 0 player near -> state 1'),
        (0, dict(dist=0xFA, vy=0, vx=0), 'state 0 player at 0FAh resets'),
    ]
    for tm in (0x57, 0x58, 0x59, 0x5A, 0x2B, 0x2C, 0x2D):
        for dist in (0x40, 0x60, 0x90, 0xB0, 0x200):
            for b in (0, 1, 2, 5):
                T.append((2, dict(timer=tm, dist=dist, b91fb=b, vy=0, vx=0, sgn=1 if dist & 0x10 else -1,
                                  energy=0x50, ground=1), f'state 2 timer {tm:02X} dist {dist:X} b91fb {b}'))
    for vyv in (0, 0x10, -0x40):
        T.append((2, dict(timer=0x60, dist=0x60, vy=vyv, vx=0, ground=0), f'state 2 timer past 58h, VelY {vyv}'))
    for d in (0x10, 0x19, 0x1A, 0x23, 0x24, 0x63, 0x64):
        for cnt in (0, 5):
            T.append((3, dict(timer=0x10, dist=d, counter=cnt, vy=0, vx=0, energy=0x55, ground=1), f'state 3 dist {d:X} counter {cnt}'))
    for vyv in (0, 0x10, -0x40):
        T.append((3, dict(timer=0x10, dist=0x70, vy=vyv, vx=0, energy=0x55), f'state 3 VelY {vyv}'))
    for e in (0x32, 0x4A, 0x4B, 0x19, 0x1A):
        T.append((3, dict(timer=0x10, dist=0x70, vy=0, vx=0, energy=e), f'state 3 energy {e:X}'))
    for sgn in (1, -1):
        T.append((3, dict(timer=0x10, dist=0x70, vy=0, vx=0, sgn=sgn), f'state 3 jump toward the player {sgn}'))
        T.append((7, dict(timer=0x10, dist=0x70, vy=0, vx=0, sgn=sgn, ground=1), f'state 7 jump, side {sgn}'))
    T += [(7, dict(timer=0x2C, dist=0x70, vy=0, vx=0), 'state 7 timer expires'),
          (7, dict(timer=0x10, dist=0x70, vy=0x40, vx=0), 'state 7 falling'),
          (7, dict(timer=0x10, dist=0x70, vy=-0x40, vx=0), 'state 7 rising'),
          (4, dict(timer=0x41, dist=0x70, vy=0, vx=0), 'state 4 timer expires'),
          (4, dict(timer=0x10, dist=0x70, vy=0, vx=0, ground=1), 'state 4 shake (VelY 0)'),
          (4, dict(timer=0x10, dist=0x70, vy=0x40, vx=0, ground=0), 'state 4 airborne'),
          (5, dict(timer=0x12, dist=0x70, vy=0, vx=0), 'state 5 timer expires'),
          (5, dict(timer=0x01, dist=0x70, vy=0, vx=0), 'state 5 running'),
          (6, dict(vy=-0x40, dist=0x70, vx=0), 'state 6 rising (defeat pending)'),
          (6, dict(vy=0x10, dist=0x70, vx=0, ground=0), 'state 6 falling'),
          (6, dict(vy=0, dist=0x70, vx=0, ground=1), 'state 6 resting: the defeat explosion'),
          (6, dict(vy=0, dist=0x70, vx=0, ground=1, energy=0x50), 'state 6 resting, other energy')]
    for n, (st, forced, lab) in enumerate(T):
        for lv in (2, 8):
            cs.append(case(f'UpdateBossFight: targeted {lab}, level {lv}', 'UpdateBossFight', 'c_UpdateBossFight',
                           level=lv, setup=boss_fight_scene(st, 0xB05E0000 + n * 3 + lv, lv, forced)))
    return cs

# -- UpdateLevel5Sequence sweep on the real level-5 data (tables at A485h/A4B5h/A4E5h) -----------
L5_PROJ = lambda k: 0x7DAF + 0x0B * k      # projectile records (X, Y, frame, +6 fall, +8 sway, +0Ah step)
L5_SLOT = lambda k: 0x55EE + 0x12 * k      # mirrored render slots

def level5_scene(seed, forced=None):
    forced = forced or {}
    def s(m):
        rnd = random.Random(seed)
        def pick(key, seq):
            v = rnd.choice(seq)
            return forced.get(key, v)
        m.wb(m.data('CurrentLevel'), 5)
        m.wb(m.data('LevelFlags'), 0)
        reset_boss(m, 0xFF)
        m.ww(m.data('Level5Phase'), pick('phase', (0, 0, 1, 1, 2, 2, 3)))
        m.wb(m.data('byte_A324'), pick('a324', (0, 0, 1, 2, 6)))
        m.wb(m.data('byte_A325'), pick('a325', (1, 2, 7)))
        m.wb(m.data('byte_A328'), pick('a328', (0, 1, 2)))
        m.wb(m.data('byte_A329'), pick('a329', (0, 1, 2, 3)))
        m.wb(m.data('byte_A32A'), pick('a32a', (0, 1, 2, 3)))
        m.wb(m.data('byte_A32B'), pick('a32b', (0, 0, 1, 5, 0x40)))
        m.wb(m.data('byte_A32C'), pick('a32c', (1, 1, 2, 0x10)))
        m.ww(m.data('SnowFrameTick'), forced.get('tick', rnd.randrange(8)))
        m.wb(m.data('RespawnState'), pick('respawn', (0, 0, 0, 1)))
        m.wb(m.data('Energy'), rnd.choice((0, 3, 8)))
        m.wb(m.data('byte_6BC9'), rnd.choice((0, 1, 5)))
        m.wb(m.data('Lives'), 2)
        m.wb(m.data('LandingFlag'), rnd.randrange(2))
        m.wb(m.data('byte_27EA'), rnd.randrange(2))
        m.wb(m.data('ShakeMagnitude'), 0)
        px = forced.get('px', 0x03C0 + rnd.choice((-0x30, -0x10, 0, 0x10, 0x30)))
        py = forced.get('py', rnd.choice((0x0790, 0x07A0, 0x07BB, 0x07BC, 0x07C0, 0x0500)))
        set_player(m, px, py, vy=rnd.choice((0, 0x40, 0x100)))
        m.ww(PLAYER + 4, 2); frame_box(m, 2, 0x20, 0x30, 0x10)
        m.wb(m.data('PlayerHurtTimer'), 0)
        frame_box(m, 0x1B3, 0x10, 0x10, 8)
        for k in range(5):
            kind = rnd.choice(('free', 'moving', 'on_player', 'low', 'sway'))
            a = L5_PROJ(k); sl = L5_SLOT(k)
            if kind == 'free':
                m.ww(a + 4, 0xFFFF); m.ww(sl + 4, 0xFFFF)
                continue
            x, y = 0x0400 + 0x20 * k, 0x0480
            if kind == 'on_player': x, y = px, py
            if kind == 'low': x, y = 0x0400, 0x07D8 + rnd.choice((-1, 0, 1, 8))
            m.ww(a, x); m.ww(a + 2, y); m.ww(a + 4, 0x01B3)
            m.ww(a + 6, rnd.choice((0x10, 0x30, 0x7F0, 0xFFF0)))
            m.ww(a + 8, rnd.choice((0, 0x10, 0x1F, 0x20, 0xFFE0, 0xFFE1, 0xFFC0, 0x30)) if kind != 'sway' else 0x1E)
            m.wb(a + 10, rnd.choice((4, 0xFC, 1, 0xF0)))
            m.ww(sl, x); m.ww(sl + 2, y); m.ww(sl + 4, 0x01B3)
        # the boss parts (rewritten from the tables each call, positions near the tables' rows)
        for k in range(5): m.ww(0x5648 + 0x12 * k + 4, 0xFFFF)
        for sl in (0x4F0A, 0x4F2E, 0x4F40):
            mode = rnd.choice(('free', 'free', 'part0', 'part1', 'part2'))
            frame_box(m, 0x50)
            if mode == 'free': m.ww(sl + 4, 0xFFFF)
            else:
                pos = {'part0': (0x03E3, 0x0773), 'part1': (0x03D9, 0x079B), 'part2': (0x03B6, 0x079C)}[mode]
                m.ww(sl, pos[0] + rnd.choice((0, 8))); m.ww(sl + 2, pos[1]); m.ww(sl + 4, 0x0050)
        if forced.get('follower'):
            fx, fy = forced['follower']
            m.ww(0x4F0A, fx); m.ww(0x4F0C, fy); m.ww(0x4F0E, 0x0050)
        clear_pool(m)
    return s

def level5_sweep_cases():
    cs = []
    for n in range(160):
        cs.append(case(f'UpdateLevel5Sequence: sweep scene {n}', 'UpdateLevel5Sequence', 'c_UpdateLevel5Sequence',
                       level=5, setup=level5_scene(0x15000000 + n)))
    T = []
    for ph in (0, 1, 2, 3):
        for a325 in (1, 2):
            T.append((dict(phase=ph, a325=a325, a324=2, respawn=0, follower=(0x03E3, 0x0773), a328=0),
                      f'follower hits part, phase {ph}, A325 {a325}'))
    for a324 in (0, 1, 2):
        for a32b in (0, 3):
            for a32a in (0, 1, 2, 3):
                for rs in (0, 1):
                    T.append((dict(a324=a324, a32b=a32b, a32a=a32a, respawn=rs, a32c=1 if a32b == 0 else 2,
                                   phase=a32a % 3), f'timers A324 {a324} A32B {a32b} A32A {a32a} respawn {rs}'))
    for n, (forced, lab) in enumerate(T):
        for sd in (0, 1):
            cs.append(case(f'UpdateLevel5Sequence: targeted {lab} (variant {sd})', 'UpdateLevel5Sequence',
                           'c_UpdateLevel5Sequence', level=5, setup=level5_scene(0x15100000 + n * 2 + sd, forced)))
    return cs

# -- RunLevel9Script: bytecode at SCR with the repeat counter set (the cursor is not reloaded from the
# table at word_A4F7 while byte_A515 != 0), hits on the window at (B9h, 1Eh), and real-data frame runs
L9_SCRIPTS = (
    (0x11, 0x12, 0x13, 0xFD, 0xFC),                 # sound, jump back 4: lands on a group end
    (0xFF, 0xFF, 0x05, 0x00),                       # two drift particles, group end
    (0xFE, 0xFE, 0xFD, 0x06, 0x00),                 # falling particles, sound, group end
    (0xFF, 0xFE, 0xFD, 0xFF, 0x07),                 # all three opcodes
    (0x01, 0x83, 0x02),                             # 83h: jump back 125
    (0x09, 0x0A, 0x0B, 0x0C, 0xFB, 0x0D),           # jump back 5 (counter decremented)
    (0x03, 0x03, 0xFD, 0xFE, 0xFF, 0xF8, 0x04),     # particles then a jump back 8
    (0x08, 0x08, 0x08, 0x08, 0x08, 0x08),           # plain groups
)

def level9_scene(seed, forced=None):
    forced = forced or {}
    def s(m):
        rnd = random.Random(seed)
        def pick(key, seq):
            v = rnd.choice(seq)
            return forced.get(key, v)
        m.wb(m.data('CurrentLevel'), 9)
        script = pick('script', L9_SCRIPTS)
        m.write(DATA_SEG, SCR, bytes(script) + b'\x00' * 8)
        start = pick('start', (0, 0, 1, 3))
        m.ww(m.data('Level9ScriptPtr'), SCR + start)
        m.wb(m.data('byte_A515'), pick('a515', (1, 2, 5, 0xFF)))
        m.wb(m.data('byte_A516'), pick('a516', (0, 1, 2, 3)))
        m.ww(m.data('word_A519'), pick('a519', (0x0001, 0x0002, 0x0010, 0x0018, 0x0100)))
        m.ww(m.data('word_A4F7'), 0xA4F9)
        set_player(m, 0x0400, 0x0400)
        m.wb(m.data('RespawnState'), 0)
        clear_pool(m)
        # hazard slots: in the window at (B9h, 1Eh) [0, 31h], at the edge, or free
        for k in range(4):
            sl = 0x4F2E + 0x12 * k
            mode = pick(f'slot{k}', ('free', 'free', 'in', 'edge', 'out_x', 'out_y'))
            if mode == 'free': m.ww(sl + 4, 0xFFFF); continue
            x, y = {'in': (0xB9 + rnd.randrange(0x32), 0x1E + rnd.randrange(0x32)), 'edge': (0xEA, 0x4F),
                    'out_x': (0xEB, 0x30), 'out_y': (0xC0, 0x1D)}[mode]
            m.ww(sl, x); m.ww(sl + 2, y); m.ww(sl + 4, 0x0050)
    return s

def level9_gap_cases():
    cs = []
    for n in range(80):
        cs.append(case(f'RunLevel9Script: sweep scene {n}', 'RunLevel9Script', 'c_RunLevel9Script', level=9,
                       setup=level9_scene(0x19000000 + n)))
    for sc in range(len(L9_SCRIPTS)):
        for start in (0, 3):
            cs.append(case(f'RunLevel9Script: script {sc} from offset {start}', 'RunLevel9Script',
                           'c_RunLevel9Script', level=9,
                           setup=level9_scene(0x19100000 + sc, dict(script=L9_SCRIPTS[sc], start=start, a519=0x10,
                                                                  slot0='free', slot1='free', slot2='free', slot3='free'))))
    for a519 in (0, 1, 2, 0x100):
        for hit in ('slot0', 'slot3'):
            cs.append(case(f'RunLevel9Script: countdown {a519:X} with a hit in {hit}', 'RunLevel9Script',
                           'c_RunLevel9Script', level=9,
                           setup=level9_scene(0x19200000 + a519, {'a519': a519, hit: 'in'})))
    for a516 in (0, 1, 2, 3):
        cs.append(case(f'RunLevel9Script: hit with byte_A516 = {a516}', 'RunLevel9Script', 'c_RunLevel9Script',
                       level=9, setup=level9_scene(0x19300000 + a516, dict(a516=a516, a519=0x10, slot0='in'))))
    # the table's end marker (FFFFh, relative -18h) wraps the table cursor back to its start
    for a515 in (0, 1):
        def sw(m, a=a515):
            level9_scene(0x19400000, dict(a515=a, slot0='free', slot1='free', slot2='free', slot3='free'))(m)
            m.ww(m.data('word_A4F7'), 0xA511)
        cs.append(case(f'RunLevel9Script: table end marker, repeat counter {a515}', 'RunLevel9Script',
                       'c_RunLevel9Script', level=9, setup=sw))
    # real data: n frames of the shipped script (table at A4F9h, relative wrap FFFF/FFE8), then the compared frame
    for n in (0, 1, 2, 3, 5, 8, 12, 16, 20, 24, 28, 33, 40, 47, 55, 66):
        def sr(m, nb=n):
            m.wb(m.data('CurrentLevel'), 9)
            set_player(m, 0x0400, 0x0400)
            m.wb(m.data('RespawnState'), 0)
            clear_pool(m)
            m.ww(m.data('word_A519'), 0x0018)
            for _ in range(nb): m.call('RunLevel9Script', budget=400_000_000)
        cs.append(case(f'RunLevel9Script: real script, after {n} frames', 'RunLevel9Script', 'c_RunLevel9Script',
                       level=9, setup=sr))
    return cs

# -- UpdateObjects on the real boss levels, the level-5 sequence and the level-9 finale; the transform remap
def frames_case(title, level, prep, n_before, out=()):
    """n_before oracle frames of UpdateObjects after `prep`, then the compared frame (both sides)."""
    def s(m):
        level_state(m, level)
        prep(m)
        for _ in range(n_before):
            m.call('UpdateObjects', budget=400_000_000)
        quiet(m)
    return Case(title, 'UpdateObjects', 'c_UpdateObjects', regs={}, setup=s, files=True, budget=400_000_000)

def boss_level_prep(lv, dx, dy):
    def prep(m):
        bx, by = m.rw(m.data('BossX')), m.rw(m.data('BossY'))
        set_player(m, (bx + dx) & 0xFFFF, (by + dy) & 0xFFFF)
        set_cam(m, row=max(0, ((by + dy) >> 4) - 8), col=max(0, ((bx + dx) >> 4) - 10))
        m.wb(m.data('RespawnState'), 0)
        m.wb(m.data('PlayerAttackPower'), 0x14)
    return prep

def real_boss_cases():
    cs = []
    for lv in (2, 8):
        for (dx, dy) in ((-0x180, 0), (-0x90, 0x20), (0x40, 0), (-0x30, -0x10)):
            for n in (0, 1, 3, 6, 12, 30):
                cs.append(frames_case(f'UpdateObjects: boss level {lv}, player {dx:+#x},{dy:+#x} from the boss, '
                                      f'after {n} frames', lv, boss_level_prep(lv, dx, dy), n))
    # a hazard slot on the boss body starts the damage path in the real animation
    for lv in (2, 8):
        for n in (2, 8, 20):
            def prep(m, lv_=lv):
                boss_level_prep(lv_, -0x50, 0)(m)
                frame_box(m, 0x50)
                bx, by = m.rw(m.data('BossX')), m.rw(m.data('BossY'))
                for k, sl in enumerate(HAZ[:3]):
                    m.ww(sl, bx + 8 * k); m.ww(sl + 2, by - 0x20); m.ww(sl + 4, 0x0050)
                m.ww(m.data('SnowFrameTick'), 0x0100)
            cs.append(frames_case(f'UpdateObjects: boss level {lv}, hazard slots on the boss, after {n} frames',
                                  lv, prep, n))
    return cs

def real_level5_cases():
    cs = []
    for n in (0, 1, 2, 4, 8, 16, 32, 64):
        for lab, px, py in (('boss area', 0x03C0, 0x07A0), ('low', 0x03C0, 0x07C8), ('far', 0x0100, 0x0100)):
            def prep(m, px_=px, py_=py):
                m.wb(m.data('LevelFlags'), 0)              # as TryCaveTrigger leaves it on level 5
                set_player(m, px_, py_)
                set_cam(m, row=max(0, (py_ >> 4) - 8), col=max(0, (px_ >> 4) - 10))
                m.wb(m.data('RespawnState'), 0)
                m.wb(m.data('byte_A324'), 0); m.wb(m.data('byte_A325'), 7)
                m.wb(m.data('byte_A32B'), 0x10); m.wb(m.data('byte_A32C'), 3)
                m.ww(m.data('Level5Phase'), 0)
            cs.append(frames_case(f'UpdateObjects: level 5 sequence active ({lab}), after {n} frames', 5, prep, n))
    for ph in (1, 2, 3):
        def prep(m, p=ph):
            m.wb(m.data('LevelFlags'), 0)
            set_player(m, 0x03C0, 0x07A0)
            m.ww(m.data('Level5Phase'), p)
        cs.append(frames_case(f'UpdateObjects: level 5 phase {ph}', 5, prep, 2))
    return cs

def real_level9_cases():
    cs = []
    for n in (0, 1, 3):
        def prep(m, nb=n):
            set_player(m, 0x0200, 0x0100)
            m.wb(m.data('RespawnState'), 0)
            m.ww(m.data('word_A519'), 1)
            m.wb(m.data('byte_A515'), 1)
            m.ww(m.data('Level9ScriptPtr'), 0xA51B)
            frame_box(m, 0x50)
            sl = 0x4F2E
            m.ww(sl, 0xBA); m.ww(sl + 2, 0x20); m.ww(sl + 4, 0x0050)
        cs.append(frames_case(f'UpdateObjects: level 9 finale hit (last life of the countdown), after {n} frames',
                              9, prep, n))
    return cs

def transform_cases():
    """word_6BE2 != 0 remaps child frames through the table at A801h (ranges of dx = frame + 138h)."""
    cs = []
    raws = [0x0000, 0x0004, 0x0005, 0x0006, 0x0011, 0x0012, 0x001A, 0x001B, 0x0021, 0x0022, 0x0050, 0x0054,
            0x0055, 0x0059, 0x005A, 0x0060, 0x0064, 0x0065, 0x006A, 0x0073, 0x0074, 0x007A, 0x007B, 0x1000,
            0x2006, 0x4000, 0x6012, 0xC00A]
    for timer in (1, 2, 7, 0x0100, 0x0294):
        for grp in range(0, len(raws), 12):
            def s(m, t=timer, g=grp):
                level_state(m, 4)
                m.wb(m.data('CurrentLevel'), 4)
                m.ww(m.data('word_6BE2'), t)
                m.wb(m.data('ShakeMagnitude'), 0)
                set_player(m, 0x0500, 0x0400)
                set_cam(m, row=0x0018, col=0x0026)
                clear_pool(m)
                m.wb(ENTITY_LIST, 0x32)
                owner_rec(m, kind=1, flags=0x04, period=3, timer=0, X=0x0500, Y=0x0400)
                for k in range(12):
                    w = raws[(g + k) % len(raws)]
                    m.ww(SCR + 0x10 * k, w); m.ww(SCR + 0x10 * k + 2, 0x7D00)
                    put_child(m, POOL + 0x12 * k, 0x0500 + 4 * k, 0x0400, frame=0x0100, state=1, owner=OWN,
                              script=SCR + 0x10 * k)
                quiet(m)
            cs.append(Case(f'UpdateObjects: transform timer {timer:#x}, script words {grp}..', 'UpdateObjects',
                           'c_UpdateObjects', regs={}, setup=s, files=True, budget=400_000_000))
    return cs

CASES = (unit_cases() + child_cases() + entity_cases() + boss_cases() + level5_cases() + level9_cases()
         + copies_cases() + real_frame_cases() + synthetic_cases() + random_cases()
         + child_sweep_cases() + terrain_cases() + entity_gap_cases() + helper_cases() + boss_direct_cases() + boss_fight_cases() + level5_sweep_cases() + level9_gap_cases()
         + real_boss_cases() + real_level5_cases() + real_level9_cases() + transform_cases())
