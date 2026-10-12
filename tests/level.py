"""HUD, terrain, particles, camera, level state, tally, triggers and resets (src/MODULE3.ASM
cs:454E..584F; EnterLevelView at src/MODULE2.ASM cs:3AF2) vs c/level.c.

Level state: every case that needs a level gets it from the oracle's own LoadLevel run on
LEVELn.SQZ (shared with tests/render.py: level_state, base, set_cam, bios10), so the tile cache,
backdrop, class tables, map, patch lists and render slots are the shipped data. Compared: DATA,
code-segment variables, the VGA planes and latches (every case) and the extra ranges listed.

Not covered (region edge, see c/level.h): the branches that enter routines of the intro and
credits gates (ShowEndingPicture, GameOverPath: TryShowIntroKeyed and ShowIntroPicture are not
translated and return LEVEL_UNSUPPORTED), SpawnDebrisElement and IncrementPlayerLives (a score block not yet
awarded), and TestHitboxOverlap (a landing). UpdateHud's divide fault (quotient above FFFFh) is a
quirk documented in c/level.c and is not run: the oracle would take the #DE interrupt.
"""
import ctypes, random, sys
from difftest import Case
from common import ROOT
from emu import DATA_SEG, CODE_SEG

sys.path.insert(0, str(ROOT/'tests'))
import render as R                 # shared level state, BIOS services and camera setup
import sprites as S                # LoadSpriteSheet setup (name slot and tables)
from emu import DosFiles

LEVELS = (0, 4, 5, 9, 11)
BIG = 300_000_000                  # budget for frame loops (tally flights, exit walks)

# -- helpers ----------------------------------------------------------------------------
HUD_SEG = 0x9800                     # the HUD sheet (word_003D): not in a level state, so filled here

def hud_sheet(m, seed=81):
    """A HUD graphic sheet of seeded bytes (the shipped sheet is loaded at startup, not by
    LoadLevel): both sides read the same bytes, so the planar copies are compared on content."""
    rng = random.Random(seed)
    m.write(HUD_SEG, 0, bytes(rng.randrange(256) for _ in range(0x4000)))
    m.ww(m.data('word_003D'), HUD_SEG)

def case(title, routine, cfunc, setup=None, regs=None, out_regs=(), ranges=(), level=None,
         files=True, budget=50_000_000, tick_model=False):
    def s(m):
        R.base(m)                       # BIOS services and memory top; level_state restores on top
        if level is not None:
            R.level_state(m, level)
        hud_sheet(m)
        if setup:
            setup(m)
    return Case(title, routine, cfunc, regs=regs or {}, setup=s, out_regs=out_regs, ranges=ranges,
                files=files, budget=budget, tick_model=tick_model)

def ds(m, name, value, size=1):
    (m.wb if size == 1 else m.ww)(m.data(name), value)

def ds_words(m, name, values):
    for k, v in enumerate(values):
        m.ww(m.data(name) + 2 * k, v)

def cs_byte(m, name, value):
    m.wb(m.code(name), value, seg=CODE_SEG)

def hud_state(m, lives=3, energy=3, score=0, elb=None, bonus_mask=0, flash=0, tick=0, caches=None):
    """HUD inputs. caches: None = invalidated (every field redraws); 'same' = caches equal to the
    current values (nothing redraws); a tuple (lo, hi, lives, energy, bonus) = explicit."""
    m.wb(m.data('Lives'), lives)
    m.wb(m.data('Energy'), energy)
    m.ww(m.data('ScoreLow'), score & 0xFFFF)
    m.ww(m.data('ScoreHigh'), score >> 16)
    m.ww(m.data('ExtraLifeBlocks'), score // 25000 if elb is None else elb)
    m.wb(m.data('RespawnState'), 0)
    m.wb(m.data('BonusFlashTimer'), flash)
    m.wb(m.data('BonusLetterMask'), bonus_mask)
    m.wb(m.data('SnowFrameTick'), tick)
    if caches is None:
        m.ww(m.data('HudCacheScoreLow'), 0x1111)
        m.ww(m.data('HudCacheScoreHigh'), 0x2222)
        m.wb(m.data('HudCacheLives'), 0xEE)
        m.wb(m.data('HudCacheEnergy'), 0xEE)
        m.wb(m.data('HudCacheBonus'), 0xEE)
    elif caches == 'same':
        m.ww(m.data('HudCacheScoreLow'), score & 0xFFFF)
        m.ww(m.data('HudCacheScoreHigh'), score >> 16)
        m.wb(m.data('HudCacheLives'), lives)
        m.wb(m.data('HudCacheEnergy'), energy)
        m.wb(m.data('HudCacheBonus'), bonus_mask if not flash else 0xFF)
    else:
        lo, hi, l, e, b = caches
        m.ww(m.data('HudCacheScoreLow'), lo); m.ww(m.data('HudCacheScoreHigh'), hi)
        m.wb(m.data('HudCacheLives'), l); m.wb(m.data('HudCacheEnergy'), e); m.wb(m.data('HudCacheBonus'), b)

def pages(m, dp=0x2000, dd=0):
    R.set_cam(m, dp=dp, dd=dd)

def rnd_bytes(rng, n):
    return bytes(rng.randrange(256) for _ in range(n))

def load_sprites(m):
    """LoadSpriteSheet in the oracle over the current machine (frame tables for the HUD glyphs).
    Its DOS and port traffic is setup, not the case: the logs are cleared as level_state does."""
    S.load_setup(m, R.FREE)
    m.call('LoadSpriteSheet', budget=BIG)
    m.ints[0x21].log.clear(); m.ints[0x21].next = 5
    m.ports.log.clear()

def view(m, row=0, col=0, dp=0x2000, dd=0):
    R.set_cam(m, row=row, col=col, dp=dp, dd=dd)

# -- HUD (DrawStatusBarBackground, UpdateHud, glyph blitters, FormatDecimal32) ---------------
def hud_cases():
    cs = []
    cs.append(case('DrawStatusBarBackground: level 4, both pages', 'DrawStatusBarBackground',
                   'c_DrawStatusBarBackground', level=4,
                   setup=lambda m: pages(m, dp=0x2000, dd=0x0000)))
    cs.append(case('DrawStatusBarBackground: level 9, pages swapped', 'DrawStatusBarBackground',
                   'c_DrawStatusBarBackground', level=9, setup=lambda m: pages(m, dp=0x0000, dd=0x2000)))
    for score, tag in ((0, 'zero'), (9, 'one digit'), (10, 'two digits'), (24999, 'below 25000'),
                       (25000, 'block 1 (ELB 1)'), (99999, 'five nines'), (100000, 'six digits'),
                       (999999, 'six nines'), (1000000, 'seven digits'), (65535, 'low word max'),
                       (65536, 'high word 1'), (0x0F423F, 'mixed high word'), (1638399999, 'DIV boundary')):
        cs.append(case(f'UpdateHud: score {score} ({tag})', 'UpdateHud', 'c_UpdateHud', level=4,
                       setup=(lambda sc: lambda m: (pages(m), hud_state(m, score=sc, bonus_mask=0x15)))(score),
                       out_regs=()))
    for lives in (0, 1, 9, 99, 100):
        cs.append(case(f'UpdateHud: lives {lives} (display cap at 9)', 'UpdateHud', 'c_UpdateHud', level=4,
                       setup=(lambda l: lambda m: (pages(m), hud_state(m, lives=l, score=1200,
                                                                     caches=(1200, 0, 0xEE, 2, 0xEE))))(lives)))
    for energy in (0, 1, 2, 3):
        cs.append(case(f'UpdateHud: energy {energy} pips', 'UpdateHud', 'c_UpdateHud', level=4,
                       setup=(lambda e: lambda m: (pages(m), hud_state(m, energy=e, score=500)))(energy)))
    cs.append(case('UpdateHud: caches equal, nothing redraws', 'UpdateHud', 'c_UpdateHud', level=4,
                   setup=lambda m: (pages(m), hud_state(m, lives=2, energy=2, score=75000, bonus_mask=0x0A,
                                                        caches='same'))))
    cs.append(case('UpdateHud: bonus flash, SnowFrameTick odd', 'UpdateHud', 'c_UpdateHud', level=4,
                   setup=lambda m: (pages(m), hud_state(m, score=300, flash=1, tick=1))))
    cs.append(case('UpdateHud: bonus flash, SnowFrameTick even', 'UpdateHud', 'c_UpdateHud', level=4,
                   setup=lambda m: (pages(m), hud_state(m, score=300, flash=3, tick=2))))
    for mask in (0x00, 0x07, 0x10, 0x1F):
        cs.append(case(f'UpdateHud: BONUS letter mask {mask:02X}', 'UpdateHud', 'c_UpdateHud', level=4,
                       setup=(lambda k: lambda m: (pages(m), hud_state(m, bonus_mask=k, score=40)))(mask)))
    cs.append(case('UpdateHud: RespawnState 1 returns at once', 'UpdateHud', 'c_UpdateHud', level=4,
                   setup=lambda m: (pages(m), hud_state(m, score=77777), m.wb(m.data('RespawnState'), 1))))
    cs.append(case('UpdateHud: RespawnState 2 returns at once', 'UpdateHud', 'c_UpdateHud', level=4,
                   setup=lambda m: (pages(m), hud_state(m, score=77777), m.wb(m.data('RespawnState'), 2))))
    for v in (0, 7, 9999, 123456789, 0xFFFF, 0x10000, 0x7FFFFFFF, 0xFFFFFFFF, 1000000000, 1638399999):
        cs.append(case(f'FormatDecimal32: value {v}', 'FormatDecimal32', 'c_FormatDecimal32', level=4,
                       regs=dict(CX=v >> 16, DX=v & 0xFFFF)))
    for g in (0, 5, 9, 0x0A, 0x0B, 0x0C, 0x10):
        cs.append(case(f'BlitHudGlyph: glyph {g:02X}', 'BlitHudGlyph', 'c_BlitHudGlyph', level=4,
                       setup=lambda m: pages(m), regs=dict(AX=g, DI=0x2000 + 0x1CED)))
    for d in (0, 3, 9):
        cs.append(case(f'BlitHudDigit: digit {d}', 'BlitHudDigit', 'c_BlitHudDigit', level=4,
                       setup=lambda m: pages(m), regs=dict(AX=d, DI=0x2000 + 0x1CF1)))
    for idx, tag in ((0x1A, "'%' sprite"), (0x00, "'A' letter"), (0x19, "'Z' letter"), (0xE4, "letter below 'A'")):
        cs.append(case(f'BlitHudSpriteGlyph: {tag}', 'BlitHudSpriteGlyph', 'c_BlitHudSpriteGlyph',
                       level=4, setup=lambda m: (pages(m), load_sprites(m)),
                       regs=dict(AX=idx, DI=0x2000 + 0x0410), budget=BIG))
    cs.append(case('ClearBonusLetterRow: AH mask kept', 'ClearBonusLetterRow', 'c_ClearBonusLetterRow',
                   level=4, setup=lambda m: pages(m), regs=dict(AX=0x1500), out_regs=('AX',)))
    return cs

# -- player render slot and terrain entities -----------------------------------------------
def pose_key(m, k):
    return m.rw(0x7B29 + 6 * k)

def pose_cases():
    cs = []
    cs.append(case('UpdatePlayerRenderSlot: AltMode 0 returns at once', 'UpdatePlayerRenderSlot',
                   'c_UpdatePlayerRenderSlot', level=4,
                   setup=lambda m: (ds(m, 'PlayerAltModeFlag', 0), ds(m, 'PlayerFrameWord', pose_key(m, 0)))))
    for k in (0, 2, 5):
        cs.append(case(f'UpdatePlayerRenderSlot: pose key {k} on the ground', 'UpdatePlayerRenderSlot',
                       'c_UpdatePlayerRenderSlot', level=4,
                       setup=(lambda kk: lambda m: (
                           ds(m, 'PlayerAltModeFlag', 1), ds(m, 'PlayerAirFlags', 0),
                           ds(m, 'PlayerFrameWord', pose_key(m, kk) | 0x8000),
                           ds(m, 'AltMomentumStep', 1), ds(m, 'PlayerX', 0x0A80), ds(m, 'PlayerY', 0x0600),
                           ds(m, 'StrokeCounter', 0x20), ds(m, 'byte_27EA', 0), ds(m, 'byte_27EB', 0)))(k)))
    cs.append(case('UpdatePlayerRenderSlot: airborne, velocity 0x50, momentum 2', 'UpdatePlayerRenderSlot',
                   'c_UpdatePlayerRenderSlot', level=4,
                   setup=lambda m: (
                       ds(m, 'PlayerAltModeFlag', 1), ds(m, 'PlayerAirFlags', 1),
                       ds(m, 'PlayerFrameWord', pose_key(m, 1)), ds(m, 'PlayerVelX', 0x50, 2),
                       ds(m, 'AltMomentumStep', 2), ds(m, 'PlayerX', 0x0900), ds(m, 'PlayerY', 0x0800),
                       ds(m, 'StrokeCounter', 0x30), ds(m, 'byte_27EA', 0), ds(m, 'byte_27EB', 0))))
    cs.append(case('UpdatePlayerRenderSlot: airborne, fast negative velocity, momentum 5', 'UpdatePlayerRenderSlot',
                   'c_UpdatePlayerRenderSlot', level=4,
                   setup=lambda m: (
                       ds(m, 'PlayerAltModeFlag', 1), ds(m, 'PlayerAirFlags', 1),
                       ds(m, 'PlayerFrameWord', pose_key(m, 3) | 0x4000), ds(m, 'PlayerVelX', 0xFF30, 2),
                       ds(m, 'AltMomentumStep', 5), ds(m, 'PlayerX', 0x0900), ds(m, 'PlayerY', 0x0800),
                       ds(m, 'byte_27EA', 0), ds(m, 'byte_27EB', 0))))
    cs.append(case('UpdatePlayerRenderSlot: step key set, momentum unchanged', 'UpdatePlayerRenderSlot',
                   'c_UpdatePlayerRenderSlot', level=4,
                   setup=lambda m: (
                       ds(m, 'PlayerAltModeFlag', 1), ds(m, 'PlayerAirFlags', 0),
                       ds(m, 'PlayerFrameWord', pose_key(m, 4)), ds(m, 'AltMomentumStep', 3),
                       ds(m, 'byte_27EA', 1), ds(m, 'PlayerX', 0x0700), ds(m, 'PlayerY', 0x0500))))
    cs.append(case('UpdatePlayerRenderSlot: no pose key matches', 'UpdatePlayerRenderSlot',
                   'c_UpdatePlayerRenderSlot', level=4,
                   setup=lambda m: (ds(m, 'PlayerAltModeFlag', 1), ds(m, 'PlayerAirFlags', 0),
                                    ds(m, 'PlayerFrameWord', 0x1FFE), ds(m, 'PlayerX', 0x0700),
                                    ds(m, 'PlayerY', 0x0500))))
    return cs

def entity_records(m, rng, camera_col, camera_row, falling=True, all_visible=False):
    """Writes 16 terrain entities at 9107h (stride 0Fh). Positions are random around the camera
    (all_visible puts them all in view); types alternate between falling (8) and the eight directions."""
    for i in range(16):
        base = 0x9107 + 0x0F * i
        x = camera_col * 16 + (0x80 if all_visible else rng.randrange(-0x40, 0x1A0))
        y = camera_row * 16 + (0x40 if all_visible else rng.randrange(-0x20, 0xE0))
        kind = 8 if (falling and i % 3 == 0) else rng.randrange(0, 8)
        rec = bytearray(0x0F)
        rec[0:2] = (x & 0xFFFF).to_bytes(2, 'little')
        rec[2:4] = (y & 0xFFFF).to_bytes(2, 'little')
        rec[4:6] = (0x0100 + i).to_bytes(2, 'little')
        rec[6] = (kind | (0x40 if i % 4 == 1 else 0) | (0x80 if i % 5 == 2 else 0)) & 0xFF
        rec[7] = rng.randrange(0, 0x30)                 # timer / speed phase
        rec[8] = rng.randrange(0, 0x10)
        rec[9] = rng.randrange(1, 0x20)                 # reload value
        rec[0x0A] = rng.randrange(0, 3)                 # falling state (0 idle, 1 fall, 2 rest)
        rec[0x0B:0x0D] = (rng.randrange(0, 0x30)).to_bytes(2, 'little')
        rec[0x0D] = rng.randrange(0, 6)
        rec[0x0E] = rng.randrange(0, 5)
        m.write(DATA_SEG, base, bytes(rec))
    if rng.random() < 0.5:
        m.wb(0x9107 + 0x0F * 15 + 4, 0xFF, DATA_SEG)   # one free slot
        m.wb(0x9107 + 0x0F * 15 + 5, 0xFF, DATA_SEG)

def terrain_cases():
    cs = []
    def terr(seed, col, row, vy, on_plat, falling=True, all_visible=False):
        def f(m):
            view(m, row=row, col=col)
            ds(m, 'PlayerVelY', vy, 2); ds(m, 'PlayerOnPlatform', on_plat)
            ds(m, 'PlayerY', 0x7FF0, 2)     # below every entity: the landing test is not entered
            entity_records(m, random.Random(seed), col, row, falling=falling, all_visible=all_visible)
        return f
    cs.append(case('UpdateTerrainEntities: level 4, directional entities, player on platform', 'UpdateTerrainEntities',
                   'c_UpdateTerrainEntities', level=4, setup=terr(11, 40, 30, 0, 1, falling=False)))
    cs.append(case('UpdateTerrainEntities: falling entities (type 8), level 9', 'UpdateTerrainEntities',
                   'c_UpdateTerrainEntities', level=9, setup=terr(12, 30, 20, 0xFFF0, 0)))
    cs.append(case('UpdateTerrainEntities: 16 visible entities (terminator loop wraps)',
                   'UpdateTerrainEntities', 'c_UpdateTerrainEntities', level=4, budget=BIG,
                   setup=terr(13, 40, 30, 0xFFF0, 1, all_visible=True)))
    cs.append(case('UpdateTerrainEntities: small velocity, player on platform, level 5', 'UpdateTerrainEntities',
                   'c_UpdateTerrainEntities', level=5, setup=terr(14, 20, 15, 4, 1)))
    return cs

# -- effect particles, camera shake, popup ring, fireflies, triggers ----------------------
def particle_setup(seed, row, col, fine):
    def f(m):
        rng = random.Random(seed)
        for i in range(20):
            base = 0x7DE6 + 6 * i
            x = (col * 16 + rng.randrange(-0x40, 0x180)) & 0xFFFF
            y = (row * 16 + rng.randrange(-0x40, 0x120)) & 0xFFFF
            m.ww(base, x); m.ww(base + 2, y)
            m.wb(base + 4, rng.randrange(256)); m.wb(base + 5, rng.randrange(1, 9))
        if rng.random() < 0.5:
            m.ww(0x7DE6 + 6 * 3, 0xFFFF)               # a free particle
        ds(m, 'CameraRow', row, 2); ds(m, 'CameraCol', col, 2); ds(m, 'FineScroll', fine)
    return f

def particle_cases():
    cs = []
    cs.append(case('UpdateEffectParticles: random particles, camera 5/3', 'UpdateEffectParticles',
                   'c_UpdateEffectParticles', level=4, setup=lambda m: (pages(m), particle_setup(21, 5, 3, 2)(m))))
    cs.append(case('UpdateEffectParticles: particles near the view edges', 'UpdateEffectParticles',
                   'c_UpdateEffectParticles', level=9, setup=lambda m: (pages(m), particle_setup(22, 0, 0, 0)(m))))
    cs.append(case('UpdateEffectParticles: camera 12/20, fine scroll 7', 'UpdateEffectParticles',
                   'c_UpdateEffectParticles', level=11, setup=lambda m: (pages(m), particle_setup(23, 12, 20, 7)(m))))
    return cs

def shake_cases():
    cs = []
    for mag, tick, anim, tag in ((0, 1, 0, 'idle'), (1, 0, 0, 'clear'), (2, 1, 0, 'odd tick'),
                                 (3, 0, 0x20, 'even tick, anim 20h'), (0xFF, 1, 5, 'wrap at 255, anim 5'),
                                 (7, 3, 0x10, 'magnitude 7, odd tick')):
        cs.append(case(f'UpdateCameraShake: magnitude {mag:02X} ({tag})', 'UpdateCameraShake',
                       'c_UpdateCameraShake', level=4,
                       setup=(lambda mg, tk, an: lambda m: (
                           ds(m, 'ShakeMagnitude', mg), ds(m, 'SnowFrameTick', tk), ds(m, 'CurrentAnimId', an),
                           ds(m, 'PlayerY', 0x0500, 2), ds(m, 'ShakeRows', 0x33, 2)))(mag, tick, anim)))
    return cs

def popup_cases():
    cs = []
    def setup(seed):
        def f(m):
            rng = random.Random(seed)
            for k in range(5):
                base = 0x4F76 + 0x12 * k
                m.ww(base + 2, rng.randrange(0x10, 0xC0))
                m.ww(base + 4, rng.choice((0xFFFF, 0x0003, 0x0030, 0x0039, 0x0038, 0x2001)))
            ds(m, 'PopupRingHead', 0x4F76 + 0x12 * rng.randrange(5), 2)
        return f
    cs.append(case('UpdatePopupRing: random records, head at slot 2', 'UpdatePopupRing', 'c_UpdatePopupRing',
                   level=4, setup=setup(31)))
    cs.append(case('UpdatePopupRing: frames reach the free limit', 'UpdatePopupRing', 'c_UpdatePopupRing',
                   level=4, setup=setup(32)))
    return cs

def firefly_setup(seed, tick, px, py, row, col):
    def f(m):
        rng = random.Random(seed)
        for k in range(20):
            base = 0x6EA9 + 8 * k
            if rng.random() < 0.2:
                m.ww(base, 0x55AA)
                continue
            m.ww(base, (col * 16 * 8 + rng.randrange(-0x200, 0x600)) & 0xFFFF)     # X in 1/8 pixel
            m.ww(base + 2, (row * 16 * 8 + rng.randrange(-0x200, 0x600)) & 0xFFFF)  # Y
            m.wb(base + 4, rng.randrange(256))
            m.wb(base + 5, rng.randrange(256))
            m.wb(base + 6, rng.choice((0, 0xFF, 3, 0x80, 1)))
            m.wb(base + 7, rng.randrange(256))
        ds(m, 'SnowFrameTick', tick)
        ds(m, 'PlayerX', px, 2); ds(m, 'PlayerY', py, 2)
        ds(m, 'CameraRow', row, 2); ds(m, 'CameraCol', col, 2)
        ds(m, 'RandomSeed', 0x1234 + seed, 2)
    return f

def firefly_cases():
    cs = []
    cs.append(case('UpdateFireflies: random flies, tick 0 (re-seed on timer)', 'UpdateFireflies',
                   'c_UpdateFireflies', level=4, setup=lambda m: (pages(m), firefly_setup(41, 0, 0x0900, 0x0700, 8, 20)(m))))
    cs.append(case('UpdateFireflies: tick 5, player left of the flies', 'UpdateFireflies', 'c_UpdateFireflies',
                   level=9, setup=lambda m: (pages(m), firefly_setup(42, 5, 0x0100, 0x0200, 4, 3)(m))))
    cs.append(case('UpdateFireflies: tick 16, player far right and low', 'UpdateFireflies', 'c_UpdateFireflies',
                   level=11, setup=lambda m: (pages(m), firefly_setup(43, 16, 0x3000, 0x2000, 0, 10)(m))))
    return cs

def trigger_cases():
    cs = []
    cs.append(case('TryCaveTrigger: LandingBlockTimer 0, nothing happens', 'TryCaveTrigger', 'c_TryCaveTrigger',
                   level=4, setup=lambda m: (ds(m, 'LandingBlockTimer', 0), ds(m, 'PlayerX', 0x0800, 2),
                                             ds(m, 'PlayerY', 0x0600, 2))))
    cs.append(case('TryCaveTrigger: alt mode set, nothing happens', 'TryCaveTrigger', 'c_TryCaveTrigger',
                   level=4, setup=lambda m: (ds(m, 'LandingBlockTimer', 1), ds(m, 'PlayerAltModeFlag', 1),
                                             ds(m, 'PlayerX', 0x0800, 2), ds(m, 'PlayerY', 0x0600, 2))))
    cs.append(case('TryCaveTrigger: no table entry for the player tile', 'TryCaveTrigger', 'c_TryCaveTrigger',
                   level=4, setup=lambda m: (ds(m, 'LandingBlockTimer', 1), ds(m, 'PlayerAltModeFlag', 0),
                                             ds(m, 'PlayerX', 0x7F0, 2), ds(m, 'PlayerY', 0x7F0, 2))))
    def prox(seed, px, py, tick, states):
        def f(m):
            rng = random.Random(seed)
            ds(m, 'PlayerX', px, 2); ds(m, 'PlayerY', py, 2); ds(m, 'SnowFrameTick', tick)
            for k in range(15):
                base = 0x83F3 + 0x0A * k
                tile = ((rng.randrange(0, 0x30) & 0xFF) << 8) | rng.randrange(0, 0x40)
                m.ww(base + 4, states[k % len(states)](tile, px, py))
                m.ww(base, 0x1000 + rng.randrange(0, 0x800))
                m.ww(base + 2, 0x0200 + rng.randrange(0, 0x400))
                m.ww(base + 6, 0x0400 + 0x100 * k)
                m.wb(base + 8, rng.randrange(1, 4))
        return f
    idle = lambda tile, px, py: 0xFFFF
    near = lambda tile, px, py: (((py >> 4) << 8 | (px >> 4)) - 3) & 0xFFFF   # 3 before the tile: fires
    far = lambda tile, px, py: 0x4040
    fired = lambda tile, px, py: 0xFFFE
    cs.append(case('UpdateProximityTriggers: all idle', 'UpdateProximityTriggers', 'c_UpdateProximityTriggers',
                   level=4, setup=prox(51, 0x0800, 0x0600, 0, (idle,))))
    cs.append(case('UpdateProximityTriggers: triggers within 8 of the player', 'UpdateProximityTriggers',
                   'c_UpdateProximityTriggers', level=4, setup=prox(52, 0x0800, 0x0600, 1, (near, far))))
    cs.append(case('UpdateProximityTriggers: fired triggers, tick 1 (no reveal step)', 'UpdateProximityTriggers',
                   'c_UpdateProximityTriggers', level=5, setup=prox(53, 0x0800, 0x0600, 1, (fired, far))))
    # tick 0 and 4 (SnowFrameTick & 3 == 0): the reveal step runs, with RedrawTileIfVisible in AX/DI form
    cs.append(case('UpdateProximityTriggers: fired triggers, reveal step (tick 0)', 'UpdateProximityTriggers',
                   'c_UpdateProximityTriggers', level=5, setup=prox(55, 0x0800, 0x0600, 0, (fired, far))))
    cs.append(case('UpdateProximityTriggers: fired, near and idle triggers, reveal step (tick 4)', 'UpdateProximityTriggers',
                   'c_UpdateProximityTriggers', level=5, setup=prox(56, 0x0800, 0x0600, 4, (fired, near, idle))))
    cs.append(case('UpdateProximityTriggers: fired triggers, tick 2 (no reveal step)', 'UpdateProximityTriggers',
                   'c_UpdateProximityTriggers', level=5, setup=prox(54, 0x0800, 0x0600, 2, (fired, idle))))
    for px, py in ((0x0000, 0x0000), (0x7FF0, 0x0FF0), (0xFFF0, 0x00F0), (0x0130, 0x0240)):
        cs.append(case(f'GetPlayerTile: X {px:04X} Y {py:04X}', 'GetPlayerTile', 'c_GetPlayerTile', level=4,
                       setup=(lambda x, y: lambda m: (ds(m, 'PlayerX', x, 2), ds(m, 'PlayerY', y, 2)))(px, py)))
    return cs

# -- tally panel, resets, scenery -----------------------------------------------------------
def tally_cases():
    cs = []
    def tally(total_main, total_decor, sum_a, sum_b):
        return lambda m: (pages(m), ds(m, 'CollectTotalMain', total_main, 2), ds(m, 'CollectTotalDecor', total_decor, 2),
                          ds(m, 'word_2A76', sum_a, 2), ds(m, 'word_2A7A', sum_b, 2))
    for tm, td, a, b, tag in ((0, 0, 0, 0, 'no items: 100'), (5, 5, 3, 2, '5 of 10: 50'),
                              (300, 0, 200, 100, 'wrap at 256 (sum 300)'),
                              (1, 0, 1, 0, 'one of one'), (200, 100, 0x1FF, 0, 'sum 511, low byte FF'),
                              (7, 3, 0x0100, 0, 'sum 256 wraps to 0')):
        cs.append(case(f'RenderTallyPercent: {tag}', 'RenderTallyPercent', 'c_RenderTallyPercent', level=4,
                       setup=tally(tm, td, a, b)))
    cs.append(case('RenderTallyPanel: score 123456, 50 of 100', 'RenderTallyPanel', 'c_RenderTallyPanel', level=4,
                   setup=lambda m: (pages(m), ds(m, 'ScoreLow', 123456 & 0xFFFF, 2), ds(m, 'ScoreHigh', 123456 >> 16, 2),
                                    ds(m, 'CollectTotalMain', 60, 2), ds(m, 'CollectTotalDecor', 40, 2),
                                    ds(m, 'word_2A76', 30, 2), ds(m, 'word_2A7A', 20, 2))))
    cs.append(case('DrawHudText: label with spaces', 'DrawHudText', 'c_DrawHudText', level=4,
                   setup=lambda m: pages(m), regs=dict(SI=0x7D8F, DI=0x2000 + 0x230)))
    def tally_counts(m):
        rng = random.Random(61)
        m.write(DATA_SEG, 0x6C12, bytes(rng.randrange(256) for _ in range(0x71)))
        ds(m, 'ItemTotal', 9, 2)
    cs.append(case('ResetItemTally: counts cleared', 'ResetItemTally', 'c_ResetItemTally', level=4,
                   setup=tally_counts))
    for v, tick in ((0x68, 0), (0x6D, 4), (0x6D, 3), (0x3F, 8), (0x6E, 0)):
        cs.append(case(f'AnimateTallyPotFlame: word {v:04X}, tick {tick}', 'AnimateTallyPotFlame',
                       'c_AnimateTallyPotFlame', level=4,
                       setup=(lambda vv, tk: lambda m: (ds(m, 'word_4F56', vv, 2), ds(m, 'SnowFrameTick', tk)))(v, tick)))
    cs.append(case('ClearDrawPage: page 2000h cleared, VGA planes compared', 'ClearDrawPage', 'c_ClearDrawPage',
                   level=4, setup=lambda m: pages(m, dp=0x2000, dd=0)))
    cs.append(case('ResetRenderSlots: slot block cleared, start position', 'ResetRenderSlots', 'c_ResetRenderSlots',
                   level=9, regs=dict(AX=0xFFFF)))
    return cs

def reset_cases():
    cs = []
    for lv in (0, 4, 9, 11):
        cs.append(case(f'ResetLevelState: level {lv} (script and tables)', 'ResetLevelState', 'c_ResetLevelState',
                       level=lv, ranges=R.FULL_RANGES, budget=BIG, setup=lambda m: pages(m)))
    cs.append(case('ResetLevelState: level 4 with a shake carried over', 'ResetLevelState', 'c_ResetLevelState',
                   level=4, budget=BIG, ranges=R.FULL_RANGES, setup=lambda m: (pages(m), ds(m, 'ShakeRows', 9, 2))))
    def patches(m):
        seg = 0x7000
        recs = bytearray()
        recs += (0x0140).to_bytes(2, 'little') + bytes([2]) + bytes([3]) + bytes(range(1, 7))
        recs += (0x0410).to_bytes(2, 'little') + bytes([1, 1]) + bytes([0xAA])
        recs += (0x2020).to_bytes(2, 'little') + bytes([3, 4]) + bytes(range(0x30, 0x3C))
        recs += (0xFFFF).to_bytes(2, 'little')
        m.write(seg, 0, bytes(recs))
        ds(m, 'NextFreeSegment', seg, 2)
        ds(m, 'LevelSegment', 0x6000, 2)
        m.write(0x6000, 0, bytes(0x100 * 0x40))
    cs.append(case('RestoreSceneryMap: custom patch records into a map', 'RestoreSceneryMap', 'c_RestoreSceneryMap',
                   level=4, setup=patches, ranges=[(0x6000, 0, 0x4000)]))
    cs.append(case('RestoreSceneryMap: level 9 shipped patch list', 'RestoreSceneryMap', 'c_RestoreSceneryMap',
                   level=9, ranges=R.FULL_RANGES, setup=lambda m: pages(m)))
    cs.append(case('RestoreSceneryMap: level 11 shipped patch list', 'RestoreSceneryMap', 'c_RestoreSceneryMap',
                   level=11, ranges=R.FULL_RANGES, setup=lambda m: pages(m)))
    def enter(px, py, flags, vs=0, hs=0, col=0, row=0):
        def f(m):
            pages(m)
            ds(m, 'PlayerX', px, 2); ds(m, 'PlayerY', py, 2)
            ds(m, 'CameraCol', col, 2); ds(m, 'CameraRow', row, 2)
            ds(m, 'LevelFlags', flags)
            ds(m, 'VScrollActive', vs); ds(m, 'HScrollDir', hs)
            ds(m, 'PlayerVelY', 0, 2)
        return f
    cs.append(case('EnterLevelView: pan right (player far right of the view)', 'EnterLevelView', 'c_EnterLevelView',
                   level=4, budget=BIG, setup=enter(0x1C00, 0x0A80, 0x00, col=0, row=0)))
    cs.append(case('EnterLevelView: LevelFlags bit 1 set, no pan', 'EnterLevelView', 'c_EnterLevelView',
                   level=9, budget=BIG, setup=enter(0x1C00, 0x0A80, 0x02, col=3, row=2)))
    cs.append(case('EnterLevelView: start with vertical scroll pending', 'EnterLevelView', 'c_EnterLevelView',
                   level=11, budget=BIG, setup=enter(0x0700, 0x0600, 0x00, vs=1, col=5, row=4)))
    def restart(seed):
        def f(m):
            pages(m)
            rng = random.Random(seed)
            for k in range(70):
                base = 0x8F1D + 7 * k
                m.ww(base + 4, rng.choice((0xFFFF, 0x0035 + rng.randrange(0, 0xB0), 0x40, 0x0E0 + 0x35 - 1)))
            for k in range(80):
                base = 0x8C8D + 5 * k
                m.ww(base + 3, rng.choice((0xFFFF, 0x0010)))
                m.wb(base, rng.randrange(256))
            ds(m, 'CheckpointX', 0x0900, 2); ds(m, 'CheckpointY', 0x0700, 2)
            ds(m, 'CameraCol', 4, 2); ds(m, 'CameraRow', 3, 2)
        return f
    cs.append(case('RestartFromCheckpoint: level 4, snapshot restored', 'RestartFromCheckpoint',
                   'c_RestartFromCheckpoint', level=4, budget=BIG, setup=restart(71)))
    cs.append(case('RestartFromCheckpoint: level 9, snapshot restored', 'RestartFromCheckpoint',
                   'c_RestartFromCheckpoint', level=9, budget=BIG, setup=restart(72)))
    return cs

# -- camera chain -------------------------------------------------------------------------
def cam(row=5, col=5, px=0x0900, py=0x0700, vy=0, vx=0, gate=0, flags=0, fixed=None, on_plat=0,
        hdir=0, vdir=0, vx_hi=0, vscroll=0, gdirty=0, lph=None, w6bf=0):
    def f(m):
        pages(m)
        ds(m, 'CameraRow', row, 2); ds(m, 'CameraCol', col, 2)
        ds(m, 'PlayerX', px, 2); ds(m, 'PlayerY', py, 2)
        ds(m, 'PlayerVelY', vy, 2); ds(m, 'PlayerVelX', vx, 2)
        ds(m, 'ScrollFollowGate', gate); ds(m, 'LevelFlags', flags)
        ds(m, 'PlayerOnPlatform', on_plat); ds(m, 'HScrollDir', hdir); ds(m, 'VScrollActive', vscroll)
        ds(m, 'PlayerVelXHigh', vx_hi); ds(m, 'GridDirtyFlag', gdirty)
        ds(m, 'word_6BF1', w6bf, 2)
        if lph is not None: ds(m, 'LevelPropHeader', lph, 2)
        if fixed is not None: cs_byte(m, 'CameraFixedStep', fixed)
    return f

def camera_cases():
    cs = []
    cs.append(case('UpdateCameraFollow: gate set, nothing moves', 'UpdateCameraFollow', 'c_UpdateCameraFollow',
                   level=4, setup=cam(gate=1, px=0x3000)))
    cs.append(case('UpdateCameraFollow: player far right, camera scrolls right', 'UpdateCameraFollow',
                   'c_UpdateCameraFollow', level=4, setup=cam(col=2, px=0x2400, py=0x0800)))
    cs.append(case('UpdateCameraFollow: player far left at the left edge', 'UpdateCameraFollow',
                   'c_UpdateCameraFollow', level=4, setup=cam(col=0, px=0x0040, py=0x0800)))
    cs.append(case('UpdateCameraFollow: falling player, vertical follow down', 'UpdateCameraFollow',
                   'c_UpdateCameraFollow', level=9, setup=cam(row=1, col=8, px=0x0900, py=0x1400, vy=0x20, lph=20)))
    cs.append(case('UpdateCameraFollow: rising player, vertical follow up', 'UpdateCameraFollow',
                   'c_UpdateCameraFollow', level=9, setup=cam(row=12, col=8, px=0x0900, py=0x0200, vy=0xFFE0, lph=5)))
    cs.append(case('UpdateCameraFixedStep: fixed step, down scroll', 'UpdateCameraFixedStep',
                   'c_UpdateCameraFixedStep', level=4, setup=cam(row=2, col=6, px=0x0900, py=0x1600, vy=0x40, lph=30)))
    cs.append(case('UpdateCameraFixedStep: fixed step, camera at the bottom edge', 'UpdateCameraFixedStep',
                   'c_UpdateCameraFixedStep', level=4, setup=cam(row=60, col=6, px=0x0900, py=0x0200, vy=0xFF00, lph=4)))
    cs.append(case('UpdateCameraFollow: LevelFlags bit 2 forces a down scroll', 'UpdateCameraFollow',
                   'c_UpdateCameraFollow', level=5, setup=cam(row=3, col=7, px=0x0800, py=0x0600, flags=0x04)))
    cs.append(case('UpdateCameraFollow: LevelFlags bit 1 skips the horizontal camera', 'UpdateCameraFollow',
                   'c_UpdateCameraFollow', level=5, setup=cam(col=1, px=0x3000, py=0x0700, flags=0x02)))
    cs.append(case('UpdateVerticalCamera: fixed step 0, bit 2, down-scroll call', 'UpdateVerticalCamera',
                   'c_UpdateVerticalCamera', level=5, setup=cam(row=3, col=7, flags=0x04, fixed=0)))
    cs.append(case('UpdateVerticalCamera: pending scroll inside the dead zone clears it', 'UpdateVerticalCamera',
                   'c_UpdateVerticalCamera', level=4, setup=cam(row=6, py=0x0700, vscroll=1, w6bf=5, vy=0, lph=10)))
    cs.append(case('UpdateVerticalCamera: pending scroll, row above the target, step table', 'UpdateVerticalCamera',
                   'c_UpdateVerticalCamera', level=4, setup=cam(row=6, py=0x0600, vscroll=1, w6bf=3, vy=0x10, lph=10,
                                                                 fixed=0, gdirty=1)))
    cs.append(case('UpdateHorizontalCamera: scroll right, player column 6', 'UpdateHorizontalCamera',
                   'c_UpdateHorizontalCamera', level=4, setup=cam(col=0, px=0x0600, hdir=1)))
    cs.append(case('UpdateHorizontalCamera: scroll left, player column 2', 'UpdateHorizontalCamera',
                   'c_UpdateHorizontalCamera', level=4, setup=cam(col=8, px=0x0200, hdir=2)))
    cs.append(case('UpdateHorizontalCamera: idle, start right push', 'UpdateHorizontalCamera',
                   'c_UpdateHorizontalCamera', level=4, setup=cam(col=2, px=0x0100 + 0x0200, vx=0x40, hdir=0)))
    cs.append(case('UpdateHorizontalCamera: idle, start left push with high bit', 'UpdateHorizontalCamera',
                   'c_UpdateHorizontalCamera', level=4, setup=cam(col=8, px=0x0900, vx=0xFFC0, vx_hi=0x80, hdir=0)))
    cs.append(case('UpdateHorizontalCamera: idle, column 14 starts no scroll', 'UpdateHorizontalCamera',
                   'c_UpdateHorizontalCamera', level=4, setup=cam(col=0, px=0x00E0, hdir=0)))
    cs.append(case('UpdateHorizontalCamera: idle, no velocity, column 3 starts left', 'UpdateHorizontalCamera',
                   'c_UpdateHorizontalCamera', level=4, setup=cam(col=0, px=0x0030, hdir=0)))
    return cs

# -- level state machine ------------------------------------------------------------------
def state(mode=0, level=4, respawn=0, endsig=0, demo=0, item_total=0, tally=None, budget=None):
    def f(m):
        pages(m)
        ds(m, 'LevelEndMode', mode)
        ds(m, 'CurrentLevel', level)
        ds(m, 'RespawnState', respawn)
        ds(m, 'EndSignal', endsig)
        ds(m, 'byte_2879', demo)
        ds(m, 'ItemTotal', item_total, 2)
        for k, v in (tally or {}).items():
            m.wb(m.data('ItemTallyCounts') + k, v)
        ds(m, 'SnowFrameTick', 0)
        ds(m, 'PlayerX', 0x0900, 2); ds(m, 'PlayerY', 0x0700, 2)
        ds(m, 'CheckpointX', 0x0900, 2); ds(m, 'CheckpointY', 0x0700, 2)
    return f

def level_cases():
    cs = []
    cs.append(case('UpdateLevelState: mode 0, idle, continue (AX 1)', 'UpdateLevelState', 'c_UpdateLevelState',
                   level=4, setup=state(), out_regs=('AX', 'CF')))
    cs.append(case('UpdateLevelState: mode 0, RespawnState 2, continue', 'UpdateLevelState', 'c_UpdateLevelState',
                   level=4, setup=state(respawn=2), out_regs=('AX', 'CF')))
    cs.append(case('UpdateLevelState: mode 0, EndSignal 0 continue, level 9', 'UpdateLevelState',
                   'c_UpdateLevelState', level=9, setup=state(level=9, endsig=0), out_regs=('AX', 'CF')))
    cs.append(case('UpdateLevelState: mode 1, level 0Ah, next level without tally', 'UpdateLevelState',
                   'c_UpdateLevelState', level=0x0A, setup=state(mode=1, level=0x0A), out_regs=('AX', 'CF'), budget=BIG))
    cs.append(case('UpdateLevelState: mode 1, level 0Bh, next level', 'UpdateLevelState', 'c_UpdateLevelState',
                   level=0x0B, setup=state(mode=1, level=0x0B), out_regs=('AX', 'CF'), budget=BIG))
    cs.append(case('UpdateLevelState: mode 2, warp level 1 (table 12)', 'UpdateLevelState', 'c_UpdateLevelState',
                   level=1, setup=state(mode=2, level=1), out_regs=('AX', 'CF'), budget=BIG))
    cs.append(case('UpdateLevelState: mode 2, warp level 6 (table 0, exit)', 'UpdateLevelState', 'c_UpdateLevelState',
                   level=6, setup=state(mode=2, level=6), out_regs=('AX', 'CF'), budget=BIG))
    # The level-end transition with its frame loops (tally flights, walk-off): runs with the
    # test tick model. ES = A000h at its ResetItemTally, so the DATA tally survives (ES audit).
    for lv, tally in ((4, {0: 3, 5: 2}), (2, {1: 1})):
        cs.append(case(f'UpdateLevelState: mode 1, level {lv}, tally exit with frame loops', 'UpdateLevelState',
                       'c_UpdateLevelState', level=lv, setup=state(mode=1, level=lv, item_total=sum(tally.values()),
                       tally=tally), out_regs=('AX', 'CF'), budget=BIG * 4, tick_model=True))
    def absent(m):
        state(mode=2, level=0x0D)(m)
        m.wb(m.data('WarpLevelTable') + 12, 0x0D)   # the first match past the 10-byte table (slot 12)
    cs.append(case('UpdateLevelState: mode 2, level 0Dh absent (scan runs past the table to slot 12)',
                   'UpdateLevelState', 'c_UpdateLevelState', level=0x0D, setup=absent, out_regs=('AX', 'CF'),
                   budget=BIG))
    return cs

# -- region suite -----------------------------------------------------------------------
CASES = (hud_cases() + pose_cases() + terrain_cases() + particle_cases() + shake_cases() +
         popup_cases() + firefly_cases() + trigger_cases() + tally_cases() + reset_cases() +
         camera_cases() + level_cases())
