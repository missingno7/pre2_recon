"""Sprite pipeline vs c/sprites.c: LoadSpriteSheet (cs:2DFA) and DrawSpriteObjects (cs:26FA).

Load: the real SPRITES.SQZ through the original LoadSpriteSheet (DOS reads logged on both
sides); DATA, the code-segment blitter variables and the whole sprite area (raw data, the
expansion and everything the routine leaves above it) are compared.

Draw: the 116-record object list at ds:4F0Ah (18 bytes each, drawn from 5720h down, so the
lowest record is on top), camera and scroll state, and the frame tables. The tables come
from one run of the original LoadSpriteSheet, copied into each case (sheet()). The VGA
planes are seeded random so that every read-modify-write of the blitter is observable,
and VGA latches, planes and registers are compared after each case.
"""
import ctypes, random
from difftest import Case
from emu import DosFiles, DATA_SEG, STACK_SEG

NAME = 0x2A2E          # DATA offset of "SPRITES.SQZ" (anonymous string in MODULE1 DATA)
REC0 = 0x4F0A          # object records: 116 x 18 bytes, ds:4F0Ah..5720h
NREC = 116
SHEET_BYTES = 0xA0000  # everything below the VGA window
FREE = 0x2530          # first free paragraph after the program image (as in tests/sqz.py)
BUDGET = 200_000_000
REGS = dict(AX=0x1111, BX=0x2222, CX=0x3333, DX=0x4444, SI=0x5555, DI=0x6666, BP=0x7777)
REG_OUT = ('AX', 'BX', 'CX', 'DX', 'SI', 'DI', 'BP', 'DS', 'ES')

# Entry registers of the expansion: it is entered at the label after MoveSpriteDataUp, so
# BX = FrameWidths offset (7190h), SI = DI = 0. The pops in BuildFrameOffsets restore the
# values staged on the stack (EXP_REGS), so AX..BP are compared as well.
EXP_REGS = dict(AX=0, BX=0x7190, CX=0, DX=0, SI=0, DI=0, BP=0)
_sheet = {}


def load_setup(m, free):
    m.ww(m.code('ConventionalTopSeg'), 0xA000, seg=0x1030)
    m.ww(m.data('NextFreeSegment'), free)
    m.write(DATA_SEG, NAME, b'SPRITES.SQZ\0')


def sheet(m):
    """Memory after one original LoadSpriteSheet (cached); copied into the machine."""
    if 'mem' not in _sheet:
        m.reset_memory()
        load_setup(m, FREE)
        m.ints = {0x21: DosFiles()}
        m.call('LoadSpriteSheet', budget=BUDGET)
        _sheet['mem'] = m.read(0, 0, SHEET_BYTES)
    m.write(0, 0, _sheet['mem'])
    m.ints = {}


def load_case(title, free):
    def setup(m):
        load_setup(m, free)
    return Case(title, 'LoadSpriteSheet', 'c_LoadSpriteSheet', regs=REGS, setup=setup, out_regs=REG_OUT,
                ranges=[(free, 0, (SHEET_BYTES - free * 16))], files=True, budget=BUDGET)


def obj(k, x, y, frame, mirror=False, flash=False, blink=0):
    """A record for slot k (0..115): position anchor, frame, flags, blink countdown."""
    return (k, x, y, frame, mirror, flash, blink)


def scene_setup(objs, cam=(0, 0), fine=0, shake=0, drawpage=0, tick=0, seed=1, widths=None):
    def setup(m):
        sheet(m)
        for f, w in (widths or {}).items():     # frame widths other than the shipped ones
            m.wb(m.data('FrameWidths') + 2 * f, w)
        vga = m.vga
        # explicit VGA state the blitter relies on (reset values, written through the ports)
        vga.vga_out(0x3C4, 4); vga.vga_out(0x3C5, 0x06)    # chain-4 off, planar
        vga.vga_out(0x3CE, 1); vga.vga_out(0x3CF, 0x00)    # set/reset disabled
        vga.vga_out(0x3CE, 8); vga.vga_out(0x3CF, 0xFF)    # bit mask: all bits
        planes = random.Random(seed).randbytes(0x40000)     # observable read-modify-writes
        ctypes.memmove(m.vga.pre2_vga_state(), planes, 0x40000)
        for k in range(NREC):
            b = REC0 + 18 * k
            m.ww(b + 4, 0xFFFF)                             # empty record
            m.wb(b + 0x11, 0)
        for k, x, y, frame, mirror, flash, blink in objs:
            b = REC0 + 18 * k
            m.ww(b, x)
            m.ww(b + 2, y)
            m.ww(b + 4, frame | (0x8000 if mirror else 0) | (0x4000 if flash else 0))
            m.wb(b + 0x11, blink)
        m.ww(m.data('CameraCol'), cam[0])
        m.ww(m.data('CameraRow'), cam[1])
        m.wb(m.data('FineScroll'), fine)
        m.ww(m.data('ShakeRows'), shake)
        m.ww(m.data('DrawPage'), drawpage)
        m.ww(m.data('SnowFrameTick'), tick)
    return setup


def draw_case(title, objs, cam=(0, 0), fine=0, shake=0, drawpage=0, tick=0, seed=1, widths=None):
    return Case(title, 'DrawSpriteObjects', 'c_DrawSpriteObjects', regs=REGS,
                setup=scene_setup(objs, cam, fine, shake, drawpage, tick, seed, widths), out_regs=REG_OUT,
                budget=BUDGET)


def expand_case(title, frames, dst=0x3000, src=0x5000, seed=7):
    """ExpandSpriteFrames + BuildFrameOffsets on a synthetic frame table (widths and heights
    are bytes at ds:FrameWidths/FrameHeights, the terminator is a zero width). The raw planes
    are seeded random at src; the output is written from dst, as after MoveSpriteDataUp."""
    def setup(m):
        # BuildFrameOffsets ends with the caller's pops (es, ds, si, di, dx, cx, bx, ax) and ret.
        # The machine's call writes its return word at SP - 2 (popped by pop bp), so the saved
        # values sit at SP.. and the sentinel return word (0) at SP + 16.
        for off, v in zip((0x100, 0x102, 0x104, 0x106, 0x108, 0x10A, 0x10C, 0x10E),
                          (DATA_SEG, DATA_SEG, EXP_REGS['SI'], EXP_REGS['DI'], EXP_REGS['DX'],
                           EXP_REGS['CX'], EXP_REGS['BX'], EXP_REGS['AX'])):
            m.ww(off, v, seg=STACK_SEG)
        m.ww(0x110, 0, seg=STACK_SEG)
        m.ww(m.data('SpriteDataSegment'), dst)
        m.ww(m.data('ExpandDstSegment'), dst)
        m.ww(m.data('ExpandSrcSegment'), src)
        for i, (w, h) in enumerate(frames):
            m.wb(m.data('FrameWidths') + 2 * i, w)
            m.wb(m.data('FrameHeights') + 2 * i, h)
        m.wb(m.data('FrameWidths') + 2 * len(frames), 0)
        raw = random.Random(seed).randbytes(0x10000)
        m.write(src, 0, raw)
    return Case(title, 'ExpandSpriteFrames', 'c_ExpandSpriteFrames', regs=dict(EXP_REGS, SP=0x0100), setup=setup,
                out_regs=REG_OUT, ranges=[(dst, 0, 0x20000)], budget=BUDGET)


CASES = []

# Load: the real SPRITES.SQZ at two placements (the expansion moves with the free segment).
CASES.append(load_case('SPRITES.SQZ at 2530h', FREE))
CASES.append(load_case('SPRITES.SQZ at 2800h', 0x2800))

# Geometry of one large frame (frame 0: 40x36, hot 20,0). Straight and mirrored.
for mirror in (False, True):
    tag = 'mirror' if mirror else 'straight'
    for x in (100,):
        CASES.append(draw_case(f'{tag}: visible X={x}', [obj(0, x, 60, 0, mirror)]))
    for x in (0, 3, 7, 8, 11, 13, 16, 19, 22, 27):          # left clip (left = X - 20 < 0)
        CASES.append(draw_case(f'{tag}: left clip X={x}', [obj(0, x, 60, 0, mirror)]))
    for x in (301, 304, 307, 311, 316, 319, 320, 321, 327):  # right clip / edge of the window
        CASES.append(draw_case(f'{tag}: right clip X={x}', [obj(0, x, 60, 0, mirror)]))
    for x in (350, -60, -30, -20, -19, -1):                   # off-screen both sides (cull tests)
        CASES.append(draw_case(f'{tag}: horizontal X={x}', [obj(0, x, 60, 0, mirror)]))
    for y in (-40, 0, 1, 10, 30, 36, 37):                     # rel <= 0, top clip, top = 0
        CASES.append(draw_case(f'{tag}: top Y={y}', [obj(0, 100, y, 0, mirror)]))
    for y in (150, 170, 175, 176, 177, 190, 200, 211, 212, 213):  # bottom clip and cull
        CASES.append(draw_case(f'{tag}: bottom Y={y}', [obj(0, 100, y, 0, mirror)]))

# Every X & 7 residue, in the middle, at the left edge and at the right edge.
for mirror in (False, True):
    tag = 'mirror' if mirror else 'straight'
    for x0 in (96, 4, 300):
        for x in range(x0, x0 + 8):
            CASES.append(draw_case(f'{tag}: fractional X={x}', [obj(0, x, 60, 0, mirror)]))

# Flash silhouettes (bit 14) with each kind of clipping.
for mirror in (False, True):
    tag = 'mirror' if mirror else 'straight'
    for x, y, label in ((100, 60, 'visible'), (7, 60, 'left clip'), (305, 60, 'right clip'),
                        (101, 60, 'fractional X=101'), (100, 10, 'top clip'), (100, 190, 'bottom clip')):
        CASES.append(draw_case(f'{tag} flash: {label}', [obj(0, x, y, 0, mirror, flash=True)]))

# Blink countdown and SnowFrameTick (the routine adds 1 to the tick before the test).
BLINK = [(1, 0, 'blink=1 tick=0'), (1, 1, 'blink=1 tick=1'), (1, 2, 'blink=1 tick=2'),
         (1, 3, 'blink=1 tick=3'), (2, 4, 'blink=2 tick=4'), (255, 1, 'blink=255 tick=1'),
         (0, 1, 'blink=0 tick=1'), (1, 0xFFFF, 'blink=1 tick=FFFFh')]
for mirror in (False, True):
    tag = 'mirror' if mirror else 'straight'
    for blink, tick, label in BLINK:
        CASES.append(draw_case(f'{tag}: {label}', [obj(0, 100, 60, 0, mirror, blink=blink)], tick=tick))
    CASES.append(draw_case(f'{tag} flash+blink: blink=1 tick=0',
                           [obj(0, 100, 60, 0, mirror, flash=True, blink=1)], tick=0))
    CASES.append(draw_case(f'{tag} flash+blink: blink=1 tick=1',
                           [obj(0, 100, 60, 0, mirror, flash=True, blink=1)], tick=1))

# blink=2: the countdown is 1 after this frame, so the tick decides (blink-off = clear only).
# X=120 is aligned (X & 7 = 0), X=100 is shifted.
for mirror in (False, True):
    tag = 'mirror' if mirror else 'straight'
    for x in (100, 120):
        for tick in (0, 1, 2, 3):
            CASES.append(draw_case(f'{tag}: blink=2 X={x} tick={tick}',
                                   [obj(0, x, 60, 0, mirror, blink=2)], tick=tick))
    for x in (7, 305):                                   # clipped: blink-off in the clip paths
        CASES.append(draw_case(f'{tag}: blink=2 clipped X={x} tick=1', [obj(0, x, 60, 0, mirror, blink=2)], tick=1))
    CASES.append(draw_case(f'{tag} flash+blink=2 X=120 tick=1',
                           [obj(0, 120, 60, 0, mirror, flash=True, blink=2)], tick=1))

# Frame 309 (8x12): camera and frame-tests bypass; rel > 0 and left tests not applied.
for mirror, flash in ((False, False), (True, False), (False, True)):
    tag = 'mirror' if mirror else ('flash' if flash else 'straight')
    CASES.append(draw_case(f'frame 309 {tag}: X=100 Y=60', [obj(3, 100, 60, 309, mirror, flash)]))
CASES.append(draw_case('frame 309: X=-20 Y=60', [obj(3, -20, 60, 309)]))
CASES.append(draw_case('frame 309: X=-100 Y=60 (negative visible bytes)', [obj(3, -100, 60, 309)]))
CASES.append(draw_case('frame 309: X=340 Y=60 (no right test)', [obj(3, 340, 60, 309)]))
CASES.append(draw_case('frame 309: X=100 Y=-10 (rel <= 0)', [obj(3, 100, -10, 309)]))
CASES.append(draw_case('frame 309: X=100 Y=170 (bottom clip)', [obj(3, 100, 170, 309)]))
CASES.append(draw_case('frame 309: X=100 Y=200 (top >= 176 cull)', [obj(3, 100, 200, 309)]))

# Frame widths that are not multiples of 8 (the shipped table has none): the rows are
# w >> 3 bytes wide and the mirrored hot point is w - hot.
for w in (1, 7, 20, 31):
    for mirror in (False, True):
        tag = 'mirror' if mirror else 'straight'
        CASES.append(draw_case(f'{tag}: frame 0 width {w} X=100', [obj(0, 100, 60, 0, mirror)],
                               widths={0: w}))
        CASES.append(draw_case(f'{tag}: frame 0 width {w} left clip X=7', [obj(0, 7, 60, 0, mirror)],
                               widths={0: w}))

# Empty slots, draw order and overlaps.
CASES.append(draw_case('empty slots around three objects', [
    obj(1, 100, 60, 0), obj(4, 110, 70, 4, mirror=True), obj(9, 140, 90, 1, flash=True)]))
CASES.append(draw_case('six overlapping objects, record order', [
    obj(0, 120, 60, 0), obj(1, 124, 64, 1, mirror=True), obj(2, 118, 58, 4),
    obj(5, 130, 66, 2, flash=True), obj(7, 112, 62, 3, mirror=True), obj(8, 126, 70, 5)]))
CASES.append(draw_case('first and last records (5720h slot 115 and 4F0Ah slot 0)', [
    obj(115, 90, 60, 0), obj(0, 100, 62, 1, mirror=True)]))
CASES.append(draw_case('overlap with blinking and flash', [
    obj(3, 100, 60, 0, blink=1), obj(2, 104, 62, 1, flash=True), obj(1, 96, 58, 4, mirror=True, blink=2)],
    tick=2))

# Camera position, FineScroll and ShakeRows.
row = [obj(k, 60 + 24 * (k % 6), 40 + 20 * (k // 6), (0, 1, 2, 4, 5, 6)[k % 6], mirror=bool(k & 1),
           flash=(k % 5 == 0)) for k in range(12)]
CASES.append(draw_case('camera (3,2) fine 0', row, cam=(3, 2)))
CASES.append(draw_case('camera (0,5) fine 255', row, cam=(0, 5), fine=255))
CASES.append(draw_case('camera (7,0) fine 9 shake 7', row, cam=(7, 0), fine=9, shake=7))
CASES.append(draw_case('camera (10,10) fine 128 shake -3', row, cam=(10, 10), fine=128, shake=0xFFFD))
CASES.append(draw_case('shake 200 fine 0', row, shake=200))

# DrawPage: other destination offsets, including an odd one.
CASES.append(draw_case('draw page 1000h', row, drawpage=0x1000))
CASES.append(draw_case('draw page 2F00h', row, drawpage=0x2F00))
CASES.append(draw_case('draw page 0005h', row, drawpage=0x0005))

# ExpandSpriteFrames and BuildFrameOffsets: widths 1..248 (ceil to bytes), a frame that
# has a width but no height (CX = 0 loop), and big frames that cross 1000h offsets.
CASES.append(expand_case('expand mixed widths', [(1, 1), (7, 3), (8, 8), (9, 4), (20, 6), (31, 5),
                                               (33, 2), (64, 8), (248, 40), (5, 9)]))
CASES.append(expand_case('expand zero-height frame (CX = 0 loop)', [(8, 4), (8, 0), (12, 3)]))
CASES.append(expand_case('expand big frames (offsets past 1000h)', [(248, 255), (248, 255), (200, 120), (8, 10)]))
CASES.append(expand_case('expand 1 px and 7 px rows', [(1, 200), (7, 180), (2, 2)], seed=9))

# Seeded random scenes: random frames, positions (on and off screen), flags, camera.
FRAMES = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 110, 278, 279, 403, 404, 428, 431, 438]
for seed in range(40):
    rnd = random.Random(1000 + seed)
    slots = rnd.sample(range(NREC), rnd.randint(2, 6))
    objs = []
    for k in slots:
        objs.append(obj(k, rnd.randint(-120, 440), rnd.randint(-60, 260), rnd.choice(FRAMES),
                        mirror=rnd.random() < 0.5, flash=rnd.random() < 0.15,
                        blink=rnd.choice((0, 0, 0, 1, 2, 3))))
    cam = (rnd.randint(0, 12), rnd.randint(0, 10))
    CASES.append(draw_case(f'random scene {seed}', objs, cam=cam, fine=rnd.randint(0, 255),
                           shake=rnd.randint(-20, 20) & 0xFFFF, drawpage=rnd.choice((0, 0x1000, 0x2F00)),
                           tick=rnd.randint(0, 0xFFFF), seed=seed + 1))
