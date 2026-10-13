"""Video page, tile ring, wipes and level loader (src/MODULE2.ASM cs:3034..454D) vs c/render.c.

Level state: every case that needs a level gets it from the oracle's own LoadLevel run on
LEVELn.SQZ (built once per level in setup and restored from a snapshot afterwards), so the
tile cache, backdrop, class tables, level map and render slots are the shipped data. Camera,
fine scroll, rings and page variables are then set per case. VGA state (planes, latches,
registers, DAC) is compared on every case; the INT 10h requests of the oracle are served by
bios10 below with the same register effects as c/platform.c (mode set: BIOS data area
0040:0049h; palette and DAC block: attribute and DAC port writes through the VGA model).

Retrace: the VGA status model runs with a retrace period of 4 reads, so WaitVerticalRetrace
ends. The monochrome path (OptionVByte 0) is not used (it would wait on 3BAh, a non-VGA port
that reads 0 by default).
"""
import ctypes, random, zlib
from difftest import Case
import sys as _sys
from common import ROOT as _ROOT
_sys.path.insert(0, str(_ROOT/'tests'))
import _bios as bios
from common import ROOT
from emu import DATA_SEG, CODE_SEG

FREE = 0x2530                 # first paragraph after the loaded image
VGA_PLANE = 0x10000
LEVELS = (0, 4, 9, 11)        # 9 = LEVELA (A..), 11 = LEVELC (above the 0Ah split)

# -- VGA and BIOS helpers -------------------------------------------------------------
def vga_base(m):
    return ctypes.addressof(m.vga.pre2_vga_state().contents)

def vga_bytes(m):
    return bytes(m.vga.pre2_vga_state().contents)

def vga_restore(m, data):
    ctypes.memmove(vga_base(m), data, len(data))

def vga_plane(m, plane, off, data):
    data = bytes(data)
    ctypes.memmove(vga_base(m) + plane * VGA_PLANE + off, data, len(data))

def bios10(m):
    """INT 10h services used by the video routines (see c/platform.c)."""
    ax = m.reg('AX')
    ah, al = ax >> 8, ax & 0xFF
    if ah == 0x00:                                   # set video mode
        bios.set_mode(m, al)
        return None
    if ah == 0x10 and al == 0x02:                    # attribute palette + overscan
        table = m.read(m.reg('ES'), m.reg('DX'), 17)
        m.vga.vga_in(0x3DA)
        for i in range(16):
            m.vga.vga_out(0x3C0, i | 0x20)
            m.vga.vga_out(0x3C0, table[i])
        m.vga.vga_out(0x3C0, 0x11 | 0x20)
        m.vga.vga_out(0x3C0, table[16])
        return None
    if ah == 0x10 and al == 0x12:                    # DAC block
        first, count = m.reg('BX'), m.reg('CX')
        data = m.read(m.reg('ES'), m.reg('DX'), count * 3)
        m.vga.vga_out(0x3C8, first & 0xFF)
        for b in data:
            m.vga.vga_out(0x3C9, b)
        return None
    return False                                     # not modelled: stop the oracle

def base(m):
    m.ww(m.code('ConventionalTopSeg'), 0xA000, seg=CODE_SEG)
    m.ww(m.data('NextFreeSegment'), FREE)
    m.ints[0x10] = bios10
    m.vga.vga_set_retrace(4)

# -- level state (oracle LoadLevel, cached) ----------------------------------------------
_levels = {}

def level_state(m, level):
    if level not in _levels:
        base(m)
        m.call('LoadLevel', {'AX': level}, budget=200_000_000)
        dos = m.ints[0x21]
        dos.log.clear(); dos.next = 5
        m.ports.log.clear()
        _levels[level] = (m.snapshot(), vga_bytes(m))
    mem, vga = _levels[level]
    m.restore(mem)
    vga_restore(m, vga)
    m.vga.vga_set_retrace(4)

def tile_classes(m):
    """TileClassTable of the machine's current state (the oracle's ClassifyTileCache)."""
    return list(m.read(DATA_SEG, m.data('TileClassTable'), 256))

# -- case builder ---------------------------------------------------------------------
def case(title, routine, cfunc, setup=None, regs=None, out_regs=(), ranges=(), files=False,
         level=None, budget=50_000_000, port_inputs=None):
    def s(m):
        if level is not None:
            level_state(m, level)
        else:
            base(m)
        if setup:
            setup(m)
    return Case(title, routine, cfunc, regs=regs or {}, setup=s, out_regs=out_regs, ranges=ranges,
                files=files, budget=budget, port_inputs=port_inputs)

def set_cam(m, row=0, col=0, fine=0, rowring=0, colring=0, shake=0, dp=0x2000, dd=0):
    m.ww(m.data('CameraRow'), row)
    m.ww(m.data('CameraCol'), col)
    m.wb(m.data('FineScroll'), fine)
    m.ww(m.data('RowRing'), rowring)
    m.ww(m.data('ColumnRing'), colring)
    m.ww(m.data('ShakeRows'), shake)
    m.ww(m.data('DrawPage'), dp)
    m.ww(m.data('DisplayPage'), dd)
    m.ww(m.data('RingTileOffset'), (0x3F40 + 2 * colring + 640 * rowring) & 0xFFFF)

FULL_RANGES = [(FREE, 0, 0xA000 * 16 - FREE * 16)]

# -- simple and VGA-register routines ------------------------------------------------
def plain_cases():
    cs = []
    cs.append(case('EnableVideoOutput (flip-flop reset, PAS on)', 'EnableVideoOutput', 'c_EnableVideoOutput'))
    cs.append(case('DisableVideoOutput (PAS off)', 'DisableVideoOutput', 'c_DisableVideoOutput'))
    cs.append(case('SetCpuWriteMode after latch copy', 'SetCpuWriteMode', 'c_SetCpuWriteMode',
                   setup=lambda m: m.vga.vga_out(0x3CE, 5) or m.vga.vga_out(0x3CF, 1)))
    cs.append(case('SetLatchCopyMode', 'SetLatchCopyMode', 'c_SetLatchCopyMode'))
    cs.append(case('WaitVerticalRetrace: VGA model, retrace period 4', 'WaitVerticalRetrace',
                   'c_WaitVerticalRetrace', setup=lambda m: m.wb(m.code('OptionVByte'), 5, seg=CODE_SEG)))
    cs.append(case('unused_443E: bare RET', 'unused_443E', 'c_unused_443E'))
    cs.append(case('ProbeVideoMemory: both probes read back', 'ProbeVideoMemory', 'c_ProbeVideoMemory',
                   out_regs=('CF',)))
    cs.append(case('CheckVgaAdapter: probe passes (OptionVByte FFh), level mode set',
                   'CheckVgaAdapter', 'c_CheckVgaAdapter', out_regs=(),
                   setup=lambda m: m.wb(m.code('OptionVByte'), 0xFF, seg=CODE_SEG)))
    cs.append(case('CheckVgaAdapter: OptionVByte set, skips probe', 'CheckVgaAdapter', 'c_CheckVgaAdapter',
                   setup=lambda m: m.wb(m.code('OptionVByte'), 1, seg=CODE_SEG)))
    return cs

def palette_cases():
    cs = []
    for lv in (0, 4, 9):
        cs.append(case(f'SetLevelVideoMode: level {lv} overscan + DAC block', 'SetLevelVideoMode',
                       'c_SetLevelVideoMode', level=lv, files=True))
    cs.append(case('StepPaletteFade: fade out, one step (level 4)', 'StepPaletteFade', 'c_StepPaletteFade',
                   level=4, files=True,
                   setup=lambda m: (m.wb(m.data('FadeOutActive'), 1), m.wb(m.data('FadeInActive'), 0),
                                    m.wb(m.data('PaletteFadeStep'), 3))))
    cs.append(case('StepPaletteFade: fade in, step 0x20 (large steps)', 'StepPaletteFade', 'c_StepPaletteFade',
                   level=4, files=True,
                   setup=lambda m: (m.wb(m.data('FadeOutActive'), 0), m.wb(m.data('FadeInActive'), 1),
                                    m.wb(m.data('PaletteFadeStep'), 0x20))))
    def done(m):
        # target equals the source palette: every component reaches it, both flags clear
        src = m.read(DATA_SEG, m.rw(m.data('LevelPaletteTable') + 2 * 4), 48)
        m.write(DATA_SEG, 0xACB7, src)                         # FADE target palette (no label)
        m.wb(m.data('FadeOutActive'), 1); m.wb(m.data('FadeInActive'), 0); m.wb(m.data('PaletteFadeStep'), 1)
    cs.append(case('StepPaletteFade: target reached, both flags clear', 'StepPaletteFade', 'c_StepPaletteFade',
                   level=4, files=True, setup=done))
    cs.append(case('StepPaletteFade: no fade active (returns at once)', 'StepPaletteFade', 'c_StepPaletteFade',
                   level=4, files=True,
                   setup=lambda m: (m.wb(m.data('FadeOutActive'), 0), m.wb(m.data('FadeInActive'), 0))))
    return cs

def timer_cases():
    def tick(m, last, now, step=0):
        m.ww(m.data('word_27EE'), last)
        m.ww(m.code('TimerTickCount'), now, seg=CODE_SEG)
        m.wb(m.data('byte_6BE7'), 0x77)
    cs = []
    cs.append(case('WaitSoundTicks: advance exactly 3', 'WaitSoundTicks', 'c_WaitSoundTicks',
                   setup=lambda m: tick(m, 0x0100, 0x0103)))
    cs.append(case('WaitSoundTicks: advance 0x100 (no count)', 'WaitSoundTicks', 'c_WaitSoundTicks',
                   setup=lambda m: tick(m, 0x0100, 0x0200)))
    cs.append(case('WaitSoundTicks: backwards 3 (negated, counts)', 'WaitSoundTicks', 'c_WaitSoundTicks',
                   setup=lambda m: tick(m, 0x0103, 0x0100)))
    cs.append(case('WaitSoundTicks: backwards 0x8000 wrap', 'WaitSoundTicks', 'c_WaitSoundTicks',
                   setup=lambda m: tick(m, 0x8000, 0x0000)))
    cs.append(case('PresentFrame: flip then wait (advance 4)', 'PresentFrame', 'c_PresentFrame',
                   setup=lambda m: (tick(m, 0x0010, 0x0014),
                                    m.ww(m.data('DrawPage'), 0x2000), m.ww(m.data('DisplayPage'), 0))))
    cs.append(case('FlipDisplayPage: swap pages (DrawPage 2000h)', 'FlipDisplayPage', 'c_FlipDisplayPage',
                   setup=lambda m: (m.ww(m.data('DrawPage'), 0x2000), m.ww(m.data('DisplayPage'), 0))))
    cs.append(case('FlipDisplayPage: DrawPage high byte 0Ah', 'FlipDisplayPage', 'c_FlipDisplayPage',
                   setup=lambda m: (m.ww(m.data('DrawPage'), 0x0A34), m.ww(m.data('DisplayPage'), 0x2000))))
    return cs

# -- VGA memory helpers for the wipes and the blits (state from a loaded level) ----------
def scratch_planes(seed):
    def f(m):
        rng = random.Random(seed)
        for p in range(4):
            vga_plane(m, p, 0x2000, bytes(rng.randrange(256) for _ in range(0x1900)))
            vga_plane(m, p, 0x0000, bytes(rng.randrange(256) for _ in range(0x2000)))
    return f

def wipe_cases():
    cs = []
    for lv in (0, 4):
        cs.append(case(f'RevealLevelColumns: level {lv} (ten columns, retraces)', 'RevealLevelColumns',
                       'c_RevealLevelColumns', level=lv, files=True,
                       setup=lambda m: (set_cam(m, row=3, col=7), scratch_planes(lv)(m))))
    cs.append(case('ClearStaggeredBlocks: resets FineScroll, blocks on DisplayPage', 'ClearStaggeredBlocks',
                   'c_ClearStaggeredBlocks', level=4, files=True,
                   setup=lambda m: (set_cam(m, fine=9, dp=0x2000, dd=0), scratch_planes(3)(m))))
    cs.append(case('ClearDisplayPage: DisplayPage 0 (8000 bytes)', 'ClearDisplayPage', 'c_ClearDisplayPage',
                   level=4, files=True, setup=lambda m: (set_cam(m, dd=0x2000), scratch_planes(4)(m))))
    return cs

def blackout_cases():
    cs = []
    def bo(title, bx, cx, dx, dp=0x2000, pl=5):
        cs.append(case(title, 'BlackoutSpan', 'c_BlackoutSpan', regs={'BX': bx, 'CX': cx, 'DX': dx, 'ES': 0xA000},
                       setup=lambda m: (m.ww(m.data('DrawPage'), dp), scratch_planes(pl)(m)), files=False))
    bo('BlackoutSpan: aligned run of 40 pixels', 0x40, 40, 10)
    bo('BlackoutSpan: unaligned start 3, width 13', 0x43, 13, 20)
    bo('BlackoutSpan: width below 8 with x&7 = 6', 0x46, 5, 33)
    bo('BlackoutSpan: run to the right edge (wraps to next line)', 0x130, 0x40, 50)
    bo('BlackoutSpan: x = 320 (out, no write)', 0x140, 10, 10)
    bo('BlackoutSpan: width 321 (out)', 0x10, 0x141, 10)
    bo('BlackoutSpan: y = 200 (out)', 0x10, 8, 0xC8)
    bo('BlackoutSpan: y = 199, full width 320', 0x00, 0x140, 0xC7)
    bo('BlackoutSpan: zero width, aligned', 0x20, 0, 60)
    return cs

def iris_cases():
    cs = []
    def iris(m, cc, cr, px, py, fine):
        set_cam(m, row=cr, col=cc, fine=fine, dp=0x2000, dd=0)
        m.ww(m.data('PlayerX'), px); m.ww(m.data('PlayerY'), py)
    cs.append(case('RunIrisClose: centre of the level (level 4)', 'RunIrisClose', 'c_RunIrisClose', level=4,
                   files=True, budget=400_000_000, setup=lambda m: (iris(m, 20, 10, 0x0A00, 0x0B00, 0), scratch_planes(7)(m))))
    cs.append(case('RunIrisClose: near the top edge, fine scroll 15 (level 9)', 'RunIrisClose', 'c_RunIrisClose',
                   level=9, files=True, budget=400_000_000, setup=lambda m: (iris(m, 3, 0, 0x0300, 0x0040, 15), scratch_planes(8)(m))))
    return cs

def scroll_cases():
    cs = []
    def sv(title, routine, cfunc, dx=None, regs=None, pre=None, **camkw):
        r = dict(regs or {})
        if dx is not None: r['DX'] = dx
        def s(m):
            set_cam(m, **camkw)
            if pre: pre(m)
        cs.append(case(title, routine, cfunc, level=4, files=True, regs=r, out_regs=('CF',), setup=s))
    def bottom(m):
        m.ww(m.data('CameraRow'), m.rb(m.data('MapRows')) - 11)          # CameraRow = MapRows - 0Bh
    def right_limit(m):
        m.ww(m.data('PlayerX'), 0x0400)
        m.ww(m.data('CameraLeftLimit'), 0x70)                          # CameraCol 70h: at the limit
    sv('ScrollViewUp: top edge (CameraRow 0): CF set', 'ScrollViewUp', 'c_ScrollViewUp', dx=1, row=0, col=5)
    sv('ScrollViewUp: fine 0 wraps to 15, ring row 0 -> 11', 'ScrollViewUp', 'c_ScrollViewUp', dx=1,
       row=6, col=5, fine=0, rowring=0, colring=3)
    sv('ScrollViewUp: fine 5 minus 3 stays', 'ScrollViewUp', 'c_ScrollViewUp', dx=3, row=6, col=5, fine=5, rowring=4)
    sv('ScrollViewUp: dl 0FFh wraps FineScroll from 3', 'ScrollViewUp', 'c_ScrollViewUp', dx=0xFF,
       row=20, col=0, fine=3, rowring=7, colring=19)
    sv('ScrollViewDown: fine 15 + 1 wraps, row advances', 'ScrollViewDown', 'c_ScrollViewDown', dx=1,
       row=6, col=5, fine=15, rowring=11, colring=2)
    sv('ScrollViewDown: fine 0 + 4 stays', 'ScrollViewDown', 'c_ScrollViewDown', dx=4, row=6, col=5, fine=0, rowring=3)
    sv('ScrollViewDown: at the bottom edge (CameraRow = MapRows - 11): CF set', 'ScrollViewDown',
       'c_ScrollViewDown', dx=1, pre=bottom, col=5)
    sv('ScrollViewDown: one row below the bottom edge', 'ScrollViewDown', 'c_ScrollViewDown', dx=15,
       row=0x00FE, col=17, fine=2, rowring=11, colring=19)
    for f, d in ((14, 1), (14, 2), (13, 2), (0, 15), (1, 15), (8, 8)):   # boundary: fine + dl = 15 / 16 / 17
        sv(f'ScrollViewDown: fine {f} + {d} (sum {f + d})', 'ScrollViewDown', 'c_ScrollViewDown', dx=d,
           row=6, col=5, fine=f, rowring=4, colring=2)
    sv('ScrollViewLeft: CameraCol 0 edge: CF set', 'ScrollViewLeft', 'c_ScrollViewLeft', col=0, row=8)
    sv('ScrollViewLeft: ring column 0 wraps to 19', 'ScrollViewLeft', 'c_ScrollViewLeft', col=40, row=8,
       colring=0, rowring=5)
    sv('ScrollViewLeft: ring column 7', 'ScrollViewLeft', 'c_ScrollViewLeft', col=40, row=8, colring=7, rowring=0)
    sv('ScrollViewRight: at the camera limit: CF set', 'ScrollViewRight', 'c_ScrollViewRight', col=0x70, row=8, pre=right_limit)
    sv('ScrollViewRight: ring column 19 wraps to 0', 'ScrollViewRight', 'c_ScrollViewRight', col=2, row=8,
       colring=19, rowring=11)
    sv('ScrollViewRight: ring column 4', 'ScrollViewRight', 'c_ScrollViewRight', col=2, row=8, colring=4, rowring=6)
    return cs

def ring_cases():
    cs = []
    def dtr(title, routine, cfunc, regs, **camkw):
        cs.append(case(title, routine, cfunc, level=4, files=True, regs=regs,
                       setup=lambda m: set_cam(m, **camkw)))
    dtr('ComputeRingTileOffset: ring column 0, RowRing 0', 'ComputeRingTileOffset', 'c_ComputeRingTileOffset',
        {'AX': 0}, rowring=0)
    dtr('ComputeRingTileOffset: column 19, RowRing 11', 'ComputeRingTileOffset', 'c_ComputeRingTileOffset',
        {'AX': 19}, rowring=11)
    dtr('DrawTileRow: map row 3 col 9, ring 0 (no wrap)', 'DrawTileRow', 'c_DrawTileRow',
        {'AX': (3 << 8) | 9, 'DI': 0}, row=3, col=9, colring=0, rowring=2)
    dtr('DrawTileRow: ring offset past 5D40h wraps', 'DrawTileRow', 'c_DrawTileRow',
        {'AX': (20 << 8) | 0xF0, 'DI': 0x1B80}, row=20, col=0xF0, colring=14, rowring=11)
    dtr('DrawTileRow: column wrap at ColumnRing 19', 'DrawTileRow', 'c_DrawTileRow',
        {'AX': (100 << 8) | 200, 'DI': 0x0040}, row=100, col=200, colring=19, rowring=0)
    dtr('DrawTileColumn: column 13h, ring column 19', 'DrawTileColumn', 'c_DrawTileColumn',
        {'AX': (40 << 8) | 90, 'DI': 0x13}, row=40, col=90, colring=7, rowring=4)
    dtr('DrawTileColumn: column 0 at ColumnRing 0 (no wrap)', 'DrawTileColumn', 'c_DrawTileColumn',
        {'AX': (7 << 8) | 1, 'DI': 0}, row=7, col=1, colring=0, rowring=9)
    dtr('DrawTileColumn: ring column wraps past 14h', 'DrawTileColumn', 'c_DrawTileColumn',
        {'AX': (60 << 8) | 61, 'DI': 0x0E}, row=60, col=61, colring=15, rowring=11)
    return cs

def tile_cases():
    cs = []
    def blit(title, routine, cfunc, want, di, fine=0, dp=0x2000, nth=0, clear=False):
        """want: class 0, 1, or 2 (any masked class); the tile is the nth one of that class.
        clear: the destination planes are zero first, so the OR of a masked tile shows."""
        def s(m):
            set_cam(m, fine=fine, dp=dp)
            scratch_planes(0x51 + nth)(m)
            if clear:
                for p in range(4):
                    vga_plane(m, p, di - 0x80, bytes(0x300))
            cls = tile_classes(m)
            hits = [t for t in range(256) if (cls[t] == want if want < 2 else cls[t] >= 2)]
            c.regs['AX'] = hits[nth]
        c = case(title, routine, cfunc, level=4, files=True, regs={'AX': 0, 'DI': di, 'ES': 0xA000}, setup=s)
        cs.append(c)
    blit('BlitTileCell: class 0 tile (opaque latch copy)', 'BlitTileCell', 'c_BlitTileCell', 0, 0x2000)
    blit('BlitTileCell: class 0, second tile', 'BlitTileCell', 'c_BlitTileCell', 0, 0x2A00, nth=1)
    blit('BlitTileCell: class 1 (backdrop only)', 'BlitTileCell', 'c_BlitTileCell', 1, 0x3F42)
    blit('BlitTileCell: masked class tile', 'BlitTileCell', 'c_BlitTileCell', 2, 0x3F50 + 40)
    blit('BlitTileCell: masked class tile, second', 'BlitTileCell', 'c_BlitTileCell', 2, 0x3F60, nth=1)
    blit('BlitTileCell: masked class tile over cleared planes (OR of the cache)', 'BlitTileCell',
         'c_BlitTileCell', 2, 0x3F70, clear=True)
    blit('BlitTileCell: masked class tile over cleared planes, second', 'BlitTileCell', 'c_BlitTileCell', 2,
         0x3FA0, nth=1, clear=True)
    blit('BlitTileCell: class 0 with FineScroll 15 and destination wrap', 'BlitTileCell', 'c_BlitTileCell',
         0, 0x5D3E, fine=15)
    blit('BlitTileToVram: masked tile at the 5D40h wrap', 'BlitTileToVram', 'c_BlitTileToVram', 2, 0x5D30)
    blit('BlitTileToRing: class 0 into ring offset 0', 'BlitTileToRing', 'c_BlitTileToRing', 0, 0)
    blit('BlitTileToRing: masked tile into ring 1B80h', 'BlitTileToRing', 'c_BlitTileToRing', 2, 0x1B80, nth=2)
    cs.append(case('RestoreTileBackdrop: DI 3F40h, FineScroll 7', 'RestoreTileBackdrop', 'c_RestoreTileBackdrop',
                   level=4, files=True, regs={'DI': 0x3F40, 'ES': 0xA000},
                   setup=lambda m: (set_cam(m, fine=7), m.ww(m.data('BackdropCursor'), 0x7E80 + 0x40),
                                    scratch_planes(0x52)(m))))
    return cs

def overlay_cases():
    cs = []
    cs.append(case('RedrawOverlayTiles: camera moved, dirty grid (level 4)', 'RedrawOverlayTiles',
                   'c_RedrawOverlayTiles', level=4, files=True,
                   setup=lambda m: (set_cam(m, row=12, col=30, rowring=3, colring=5),
                                    m.ww(m.data('PrevCameraCol'), 0), m.ww(m.data('PrevCameraRow'), 0),
                                    m.wb(m.data('GridDirtyFlag'), 1), m.wb(m.data('ScrollAnimCounter'), 0))))
    cs.append(case('RedrawOverlayTiles: scroll animation counter set (level 9)', 'RedrawOverlayTiles',
                   'c_RedrawOverlayTiles', level=9, files=True,
                   setup=lambda m: (set_cam(m, row=4, col=2, rowring=11, colring=19),
                                    m.wb(m.data('ScrollAnimCounter'), 3), m.wb(m.data('GridDirtyFlag'), 0))))
    cs.append(case('RedrawOverlayTiles: same cell, clean grid (no drawing)', 'RedrawOverlayTiles',
                   'c_RedrawOverlayTiles', level=4, files=True,
                   setup=lambda m: (set_cam(m, row=4, col=2), m.ww(m.data('PrevCameraCol'), 2),
                                    m.ww(m.data('PrevCameraRow'), 4), m.wb(m.data('GridDirtyFlag'), 0),
                                    m.wb(m.data('ScrollAnimCounter'), 0))))
    cs.append(case('AnimateTiles: phase advances (snow wind 0 -> gate 3)', 'AnimateTiles', 'c_AnimateTiles',
                   level=4, files=True,
                   setup=lambda m: (set_cam(m, row=10, col=20, rowring=2, colring=6),
                                    m.wb(m.data('AnimVisibleFlag'), 1), m.wb(m.data('AnimTickCounter'), 2),
                                    m.ww(m.data('SnowWind'), 0), m.ww(m.data('AnimRemapPtr'), 0x6688))))
    cs.append(case('AnimateTiles: wind >= 14h, gate 1, phase wraps 6988h -> 6688h', 'AnimateTiles', 'c_AnimateTiles',
                   level=9, files=True,
                   setup=lambda m: (set_cam(m, row=30, col=60, rowring=8, colring=9),
                                    m.wb(m.data('AnimVisibleFlag'), 1), m.wb(m.data('AnimTickCounter'), 0),
                                    m.ww(m.data('SnowWind'), 0x20), m.ww(m.data('AnimRemapPtr'), 0x6888))))
    cs.append(case('AnimateTiles: gated off (tick not due)', 'AnimateTiles', 'c_AnimateTiles', level=4, files=True,
                   setup=lambda m: (set_cam(m, row=1, col=1), m.wb(m.data('AnimVisibleFlag'), 1),
                                    m.wb(m.data('AnimTickCounter'), 0))))
    cs.append(case('DrawForegroundTile: map cell at the window (tile 0x40 flagged)', 'DrawForegroundTile',
                   'c_DrawForegroundTile', level=4, files=True, regs={'AX': (6 << 8) | 5, 'BX': 0x40},
                   setup=lambda m: (set_cam(m, row=2, col=2, fine=4), m.ww(m.data('word_003B'), m.rw(m.data('SpriteSheetSegment'))),
                                    scratch_planes(0x61)(m))))
    def fg(m):
        set_cam(m, row=2, col=2, fine=4)
        m.wb(m.code('OptionBits'), 2, seg=CODE_SEG)
        m.wb(m.data('VisibleTileFlags'), 0x40)
        m.ww(m.data('word_003B'), m.rw(m.data('SpriteSheetSegment')))
        for slot in range(115):                             # render slots 4F0Ah.. 5731h, 12h apart
            if slot % 4 == 0:
                addr = m.data('RenderSlots') + 0x12 * slot
                m.ww(addr, (slot * 37) & 0x1FF)
                m.ww(addr + 2, 0x3000 + slot * 64)
                m.ww(addr + 4, 0x2000 | (slot & 0x0F))
        scratch_planes(0x62)(m)
    cs.append(case('DrawForegroundOverSprites: flagged slots over the window (level 4)', 'DrawForegroundOverSprites',
                   'c_DrawForegroundOverSprites', level=4, files=True, setup=fg))
    return cs

def wind_cases():
    cs = []
    def wind(title, wind_v, frame, tick, lim, rnd):
        def s(m):
            set_cam(m, row=5, col=5, dp=0x2000)
            m.ww(m.data('SnowWind'), wind_v)
            m.ww(m.data('ScriptFrameCounter'), frame)
            m.wb(m.data('SnowFrameTick'), tick)
            m.ww(m.data('WindRandWord'), rnd)
            for k in range(0x30):
                m.ww(m.data('FlakeTable') + 2 * k, (k * 0x1F3) & 0xFFFF)
            m.wb(m.data('WindRandA'), rnd & 0xFF); m.wb(m.data('WindRandB'), (rnd >> 3) & 0xFF)
            m.wb(m.data('WindRandC'), 0x5A)
            scratch_planes(0x70 + frame)(m)
        cs.append(case(title, 'UpdateWindAndFlakes', 'c_UpdateWindAndFlakes', level=4, files=True, setup=s))
    wind('UpdateWindAndFlakes: no wind, no flakes', 0, 3, 1, 0, 0x1234)
    wind('UpdateWindAndFlakes: wind 9, script step due', 9, 4, 0, 0x7FF, 0x1234)
    wind('UpdateWindAndFlakes: wind 0x30, script not due', 0x30, 1, 0, 0, 0xBEEF)
    cs.append(case('NextWindRandom: generator step', 'NextWindRandom', 'c_NextWindRandom', regs={'AX': 0x1200},
                   level=4, files=True,
                   setup=lambda m: (m.ww(m.data('WindRandWord'), 0xFFF0), m.wb(m.data('WindRandA'), 0xFE),
                                    m.wb(m.data('WindRandB'), 0x11), m.wb(m.data('WindRandC'), 0xF3))))
    return cs

def ring_copy_cases():
    cs = []
    def cp(title, cam):
        cs.append(case(title, 'CopyRingWindowToPage', 'c_CopyRingWindowToPage', level=4, files=True,
                       setup=lambda m: (set_cam(m, **cam), scratch_planes(0x90)(m))))
    cp('CopyRingWindowToPage: no shake, fine 0', dict(row=6, col=6, fine=0, rowring=0, colring=0))
    cp('CopyRingWindowToPage: fine 15, RowRing 11', dict(row=6, col=6, fine=15, rowring=11, colring=19))
    cp('CopyRingWindowToPage: shake 3 rows, fine 7', dict(row=6, col=6, fine=7, rowring=5, colring=4, shake=3))
    cp('CopyRingWindowToPage: shake 0x10 rows (first part short)', dict(row=6, col=6, fine=0, rowring=11,
                                                                       colring=19, shake=0x10))
    cp('CopyRingWindowToPage: shake 0 with the first part at 0 (swap path)', dict(row=6, col=6, fine=0xF0,
                                                                                  rowring=11, colring=0))
    for f in (1, 2, 3, 6, 9, 12, 14):
        cp(f'CopyRingWindowToPage: fine {f}, RowRing 3', dict(row=6, col=6, fine=f, rowring=3, colring=8))
    return cs

def level_cases():
    cs = []
    for lv in LEVELS:
        cs.append(case(f'LoadLevel: level {lv} (SQZ, tile set, palette, decor)', 'LoadLevel', 'c_LoadLevel',
                       regs={'AX': lv}, files=True, budget=400_000_000, ranges=FULL_RANGES,
                       setup=lambda m: (base(m), m.ww(m.data('CollectTotalDecor'), 0x0123),
                                        m.ww(m.data('CollectTotalMain'), 0x0456))))
    for lv in (4, 9):
        cs.append(case(f'LoadLevelTileSet: level {lv} (cache slots, TileIndexTable)', 'LoadLevelTileSet',
                       'c_LoadLevelTileSet', level=lv, files=True, ranges=FULL_RANGES,
                       regs={'BX': 0}))          # contract: BH = 0 (mov bh, [bx+LevelMapRowsTable])
        cs.append(case(f'BuildAnimatedTileTables: level {lv}', 'BuildAnimatedTileTables', 'c_BuildAnimatedTileTables',
                       level=lv, files=True))
        cs.append(case(f'ClassifyTileCache: level {lv}', 'ClassifyTileCache', 'c_ClassifyTileCache',
                       level=lv, files=True))
        cs.append(case(f'UploadSpriteTiles: level {lv}', 'UploadSpriteTiles', 'c_UploadSpriteTiles',
                       level=lv, files=True))
        cs.append(case(f'ScrollEffectSources: level {lv}', 'ScrollEffectSources', 'c_ScrollEffectSources',
                       level=lv, files=True))
        cs.append(case(f'RebaseEntitySpriteRefs: level {lv} (banks 30h/120h)', 'RebaseEntitySpriteRefs',
                       'c_RebaseEntitySpriteRefs', level=lv, files=True,
                       setup=lambda m: (m.ww(m.data('SpriteBankA'), 0x30), m.ww(m.data('SpriteBankB'), 0x120))))
        cs.append(case(f'RebaseEntitySpriteRefs: level {lv}, bank A = FFFFh (early exit)', 'RebaseEntitySpriteRefs',
                       'c_RebaseEntitySpriteRefs', level=lv, files=True,
                       setup=lambda m: m.ww(m.data('SpriteBankA'), 0xFFFF)))
        cs.append(case(f'ApplySecretTilePatches: level {lv}', 'ApplySecretTilePatches', 'c_ApplySecretTilePatches',
                       level=lv, files=True, setup=patch_records))
        cs.append(case(f'SavePristineTriggerRows: level {lv}', 'SavePristineTriggerRows', 'c_SavePristineTriggerRows',
                       level=lv, files=True, ranges=[(0x5000, 0, 0x800)], setup=trigger_records))
        cs.append(case(f'BuildDecorList: level {lv} (password code from the BIOS seed)', 'BuildDecorList',
                       'c_BuildDecorList', level=lv, files=True, setup=decor_records))
    cs.append(case('ComputePasswordCode: seed from ROM bytes (level 0 state), CpuClass 0', 'ComputePasswordCode',
                   'c_ComputePasswordCode', regs={'AX': 0x0004}, setup=rom_bytes(0)))
    cs.append(case('ComputePasswordCode: ready seed, CpuClass 3, AX 0FFFFh', 'ComputePasswordCode',
                   'c_ComputePasswordCode', regs={'AX': 0xFFFF}, setup=ready_seed(3)))
    cs.append(case('ComputePasswordCode: ready seed 0 (becomes 20h), CpuClass 2, AX -10', 'ComputePasswordCode',
                   'c_ComputePasswordCode', regs={'AX': 0xFFF6}, setup=ready_seed(2)))
    return cs

def patch_records(m):
    rng = random.Random(0x77)
    for k in range(0x50):
        rec = 0x8C8D + 5 * k                                   # SecretPatchTable (no label)
        if k % 3 == 2:
            m.ww(rec + 3, 0xFFFF)
        else:
            m.ww(rec + 3, 0x0100 + 37 * k)
        m.wb(rec, rng.randrange(256))
        m.wb(rec + 1, 0)

def trigger_records(m):
    lseg = m.rw(m.data('LevelSegment'))
    m.ww(m.data('NextFreeSegment'), 0x5000)
    for k in range(15):
        rec = 0x83F3 + 0x0A * k
        if k in (3, 9):
            m.ww(rec, 0xFFFF)
            continue
        m.ww(rec, 0x0400 + 0x30 * k)            # word map offset (si)
        m.wb(rec + 2, 3 + k % 4)                 # width (dh)
        m.wb(rec + 3, 2 + k % 3)                 # rows base (dl)
        m.wb(rec + 8, k % 2)                     # extra rows / start shift
        m.ww(rec + 6, 0)

def decor_records(m):
    for k in range(0x46):
        rec = 0x8F1D + 7 * k
        tgt = 0x0200 + 9 * k
        m.ww(rec + 4, 0x011B + (k % 0x10) if k % 3 else 0x0000 + 0x0F00 + k)
        m.ww(rec, tgt)
        m.ww(rec + 2, 0)

def rom_bytes(cls):
    def f(m):
        rng = random.Random(0xBEEF)
        m.write(0xF000, 0xFFF0, bytes(rng.randrange(256) for _ in range(16)))
        m.write(0xC000, 0, bytes([0x55, 0xAA, 0x40]) + bytes(rng.randrange(256) for _ in range(0x7D)))
        m.write(0xC400, 0, bytes([0x55, 0xAA, 0x20]) + bytes(rng.randrange(256) for _ in range(0x7D)))
        m.wb(m.code('CpuClass'), cls, seg=CODE_SEG)
        m.wb(m.data('BiosSeedReady'), 0)
    return f

def ready_seed(cls):
    def f(m):
        m.ww(m.data('BiosSeed'), 0x3D1B)
        m.wb(m.data('BiosSeedReady'), 1)
        m.wb(m.code('CpuClass'), cls, seg=CODE_SEG)
    return f

# -- gap-closing cases: foreground over sprites, wind script, ring copy, backdrop wrap ---------
def flag_table(m, mode, seed):
    """TileFlagsTable bit 40h (foreground) of every tile: 'all' set, 'half' seeded 50%, 'real' kept."""
    if mode == 'real':
        return
    rng = random.Random(seed)
    for t in range(256):
        a = m.data('TileFlagsTable') + t
        v = m.rb(a) & ~0x40
        if mode == 'all' or rng.random() < 0.5:
            v |= 0x40
        m.wb(a, v)

def fg_slots(m, rng, row, col, count=115):
    """Render slots (x, y, flags at +0, +2, +4) around the camera window: x over the tile
    columns -3..22 of the window (sar 4, so negative tile columns too), y over map rows -2..13
    below the window top, half of them off a tile boundary (the extra row), some free (FFFFh) or
    without the 2000h bit."""
    for slot in range(count):
        a = m.data('RenderSlots') + 0x12 * slot
        kind = rng.random()
        if kind < 0.12:
            flags = 0xFFFF
        elif kind < 0.25:
            flags = rng.choice((0x0003, 0x1FFF, 0x0000, 0xDFFF))
        else:
            flags = 0x2000 | rng.randrange(0x2000)
        x = (col + rng.randrange(-3, 23)) * 16 + rng.randrange(16)
        y = (row + 1 + rng.randrange(-2, 14)) * 16 + rng.choice((0, 0, rng.randrange(16)))
        m.ww(a, x & 0xFFFF)
        m.ww(a + 2, y & 0xFFFF)
        m.ww(a + 4, flags)

def fg_scene(seed, flags, row, col, fine=0, option=2, visible=0x40, count=115, dp=0x2000):
    def s(m):
        rng = random.Random(seed)
        set_cam(m, row=row, col=col, fine=fine, dp=dp)
        m.ww(m.code('OptionBits'), option, seg=CODE_SEG)
        m.wb(m.data('VisibleTileFlags'), visible)
        m.ww(m.data('word_003B'), m.rw(m.data('SpriteSheetSegment')))
        flag_table(m, flags, seed)
        fg_slots(m, rng, row, col, count)
        scratch_planes(0x100 + seed)(m)
    return s

def fg_cases():
    cs = []
    for tag, opt, vis in (('OptionBits bit 2 clear', 0x0001, 0x40), ('OptionBits 0', 0x0000, 0x40),
                          ('VisibleTileFlags bit 40h clear', 0x0002, 0xBF)):
        cs.append(case(f'DrawForegroundOverSprites: gate closed ({tag})', 'DrawForegroundOverSprites',
                       'c_DrawForegroundOverSprites', level=4, files=True,
                       setup=fg_scene(1, 'all', 6, 10, option=opt, visible=vis, count=20)))
    # every tile flagged: all four cells of every slot are drawn (cell -2, -1, 0, +1), with
    # slots over the window edges (cl from -3 to 22) and below / above it (ch out of range)
    cs.append(case('DrawForegroundOverSprites: all tiles foreground, slots around the window (level 4)',
                   'DrawForegroundOverSprites', 'c_DrawForegroundOverSprites', level=4, files=True,
                   setup=fg_scene(2, 'all', 6, 10, fine=3), budget=200_000_000))
    cs.append(case('DrawForegroundOverSprites: half the tiles foreground, FineScroll 15 (level 9)',
                   'DrawForegroundOverSprites', 'c_DrawForegroundOverSprites', level=9, files=True,
                   setup=fg_scene(3, 'half', 20, 30, fine=15), budget=200_000_000))
    cs.append(case('DrawForegroundOverSprites: camera at the map origin, half flagged (level 11)',
                   'DrawForegroundOverSprites', 'c_DrawForegroundOverSprites', level=11, files=True,
                   setup=fg_scene(4, 'half', 0, 0, fine=0), budget=200_000_000))
    cs.append(case('DrawForegroundOverSprites: camera column 0F0h (byte arithmetic wraps), all flagged',
                   'DrawForegroundOverSprites', 'c_DrawForegroundOverSprites', level=4, files=True,
                   setup=fg_scene(5, 'all', 2, 0xF0, fine=9, dp=0x0000), budget=200_000_000))
    def tile(title, row, col, fine, tile_id=0x40, dp=0x2000):
        cs.append(case(title, 'DrawForegroundTile', 'c_DrawForegroundTile', level=4, files=True,
                       regs={'AX': (row << 8) | col, 'BX': tile_id},
                       setup=lambda m: (set_cam(m, row=5, col=7, fine=fine, dp=dp),
                                        m.ww(m.data('word_003B'), m.rw(m.data('SpriteSheetSegment'))),
                                        scratch_planes(0x120 + fine)(m))))
    # destination window DrawPage .. DrawPage + 1900h: below it (FineScroll 40 bytes per unit),
    # at its first byte, just inside the end, and at / past the end
    tile('DrawForegroundTile: destination below DrawPage (FineScroll 15, top row)', 5, 7, 15)
    tile('DrawForegroundTile: first byte of the window is drawn (FineScroll 0, top left)', 5, 7, 0)
    tile('DrawForegroundTile: row 9 of the window, last column', 14, 26, 0)
    tile('DrawForegroundTile: row 10, FineScroll 1 (destination 28h bytes inside the end)', 15, 7, 1)
    tile('DrawForegroundTile: row 10, FineScroll 0 (destination at the end of the window)', 15, 7, 0)
    tile('DrawForegroundTile: row 11 past the end', 16, 12, 0)
    tile('DrawForegroundTile: column left of the camera (byte 0FFh scaled)', 6, 6, 2)
    return cs

# ScriptPointer entries (6 bytes): frame, wind step, limit; the next entry's frame at +6. LevelScriptTable
# entry 15 (the snow level): (1B8h, 1, 28h) (370h, 1, 64h)... see the data; FFFFh ends the script.
SNOW_SCRIPT = 0x2D76

def wind_script_cases():
    cs = []
    def w(title, frame, wind, ptr=SNOW_SCRIPT, tick=0, rnd=0x1234):
        def s(m):
            set_cam(m, row=5, col=5, dp=0x2000)
            m.ww(m.data('SnowWind'), wind)
            m.ww(m.data('ScriptFrameCounter'), frame)
            m.ww(m.data('ScriptPointer'), ptr)
            m.wb(m.data('SnowFrameTick'), tick)
            m.ww(m.data('WindRandWord'), rnd)
            for k in range(0x30):
                m.ww(m.data('FlakeTable') + 2 * k, (k * 0x2F5 + frame) & 0xFFFF)
            m.wb(m.data('WindRandA'), rnd & 0xFF); m.wb(m.data('WindRandB'), (rnd >> 3) & 0xFF)
            m.wb(m.data('WindRandC'), 0x5A)
            scratch_planes(0x140 + frame)(m)
        cs.append(case(title, 'UpdateWindAndFlakes', 'c_UpdateWindAndFlakes', level=4, files=True, setup=s))
    w('UpdateWindAndFlakes: script entry due (counter past 1B8h), wind steps by 1, entry kept', 0x1B8, 0x10)
    w('UpdateWindAndFlakes: script entry not due (counter before 1B8h), wind only clamped', 5, 0x10)
    w('UpdateWindAndFlakes: counter reaches the entry frame exactly (1B8h): not yet due', 0x1B7, 0x10)
    w('UpdateWindAndFlakes: counter at the next frame: entry kept (equal compare)', 0x36F, 0x20)
    w('UpdateWindAndFlakes: counter past the next frame: pointer advances 6, old limit clamps', 0x370, 0x27)
    w('UpdateWindAndFlakes: wind above the entry limit is clamped to it', 0x1B8, 0x60)
    w('UpdateWindAndFlakes: negative wind clamps to 0 (no flakes drawn)', 0x1B8, 0xFFF0)
    w('UpdateWindAndFlakes: wind exactly the limit, step lands on it', 0x1B8, 0x27)
    w('UpdateWindAndFlakes: second entry (step 6, limit 64h)', 0x371, 0x30, ptr=SNOW_SCRIPT + 6)
    w('UpdateWindAndFlakes: third entry, advance to the end marker, limit FFh', 0x529, 0x70, ptr=SNOW_SCRIPT + 12)
    w('UpdateWindAndFlakes: end marker reached (FFFFh): wind not touched', 0x700, 0x44, ptr=SNOW_SCRIPT + 18)
    w('UpdateWindAndFlakes: script step skipped on tick 1 (not every fourth frame)', 0x1B8, 0x10, tick=1)
    return cs

def scratch_wide(seed, end=0xA000):
    """Seeded bytes over all four planes from 0 to END: the window, the tile ring (3F40h..5D3Fh) and
    the backdrop ring, so copies from the ring and wraps of the destination have data to move."""
    def f(m):
        rng = random.Random(seed)
        for p in range(4):
            vga_plane(m, p, 0, bytes(rng.randrange(256) for _ in range(end)))
    return f

def ring_copy_gap_cases():
    cs = []
    def cp(title, cam):
        cs.append(case(title, 'CopyRingWindowToPage', 'c_CopyRingWindowToPage', level=4, files=True,
                       setup=lambda m: (set_cam(m, **cam), scratch_wide(0x160)(m))))
    # first part short: bp = C0h - fine - 10h * RowRing < B0h; shake larger than the second part
    # leaves bx negative, which folds into the first part (add bp, bx; xor bx, bx)
    cp('CopyRingWindowToPage: shake 5 over a 2-row second part: second count negative, folded',
       dict(row=6, col=6, fine=2, rowring=1, colring=4, shake=5))
    cp('CopyRingWindowToPage: shake equal to the second part (count 0, no fold)',
       dict(row=6, col=6, fine=3, rowring=1, colring=9, shake=3))
    cp('CopyRingWindowToPage: shake 25 over fine 1, RowRing 2: fold, small first part',
       dict(row=6, col=6, fine=1, rowring=2, colring=0, shake=9 + 16))
    # first part empty: bp = 0 needs FineScroll + 10h * RowRing = C0h (outside the normal 0..15
    # FineScroll range): the loop counts swap and the second part is copied first
    cp('CopyRingWindowToPage: first-part count 0 swaps with the second (FineScroll 70h, RowRing 5)',
       dict(row=6, col=6, fine=0x70, rowring=5, colring=7, shake=0))
    cp('CopyRingWindowToPage: first-part count 0 swapped, shake 4',
       dict(row=6, col=6, fine=0x70, rowring=5, colring=19, shake=4))
    return cs

def latch_copy(m):
    """Map mask Fh and write mode 1 (the latches go to all planes): a copy moves data in every plane."""
    m.vga.vga_out(0x3C4, 2); m.vga.vga_out(0x3C5, 0x0F)
    m.vga.vga_out(0x3CE, 5); m.vga.vga_out(0x3CF, 0x01)

def backdrop_wrap_cases():
    """RestoreTileBackdrop: the destination wraps at 5D40h after each of the 15 row steps."""
    cs = []
    for r in range(15):
        di = 0x5D40 - 0x28 * (r + 1)
        cs.append(case(f'RestoreTileBackdrop: destination reaches 5D40h after row {r} (DI {di:04X})',
                       'RestoreTileBackdrop', 'c_RestoreTileBackdrop', level=4, files=True,
                       regs={'DI': di, 'ES': 0xA000},
                       setup=lambda m, r=r: (set_cam(m, fine=r % 16), m.ww(m.data('BackdropCursor'), 0x7E80 + 0x40 * r),
                                             scratch_wide(0x180 + r)(m), latch_copy(m))))
    cs.append(case('RestoreTileBackdrop: DI 5E00h (first step already past the wrap)', 'RestoreTileBackdrop',
                   'c_RestoreTileBackdrop', level=4, files=True, regs={'DI': 0x5E00, 'ES': 0xA000},
                   setup=lambda m: (set_cam(m, fine=2), m.ww(m.data('BackdropCursor'), 0x7E80), scratch_wide(0x190)(m),
                                    latch_copy(m))))
    return cs

def decor_n(count, shuffle):
    """Effect-source records (7 bytes from 8F1Dh): COUNT records carry a decor word (+4 in 11Bh..12Ah,
    with a flag bit above), the rest are free (FFFFh). Their targets (+0) point at distinct DATA words
    in a seeded order, so the sort has swaps to make."""
    def f(m):
        rng = random.Random(shuffle)
        targets = [0x0300 + 6 * k for k in range(count)]
        rng.shuffle(targets)
        for k in range(0x46):
            rec = 0x8F1D + 7 * k
            if k < count:
                m.ww(rec, targets[k])
                m.ww(rec + 4, (0x011B + rng.randrange(16)) | (0x2000 if k % 3 == 1 else 0))
                m.ww(targets[k], rng.randrange(0x10000))
            else:
                m.ww(rec + 4, 0xFFFF)
    return f

def decor_gap_cases():
    cs = []
    for lv, count, b198, tag in ((4, 4, 0, 'one group of four'), (4, 8, 1, 'two groups, byte B198h set'),
                                 (9, 12, 1, 'three groups, byte B198h set (+0Ah on the password index)'),
                                 (9, 16, 0, 'four groups'), (4, 6, 1, 'six: not a multiple of four, nothing changes'),
                                 (4, 0, 1, 'no decor records')):
        cs.append(case(f'BuildDecorList: level {lv}, {count} records, {tag}', 'BuildDecorList',
                       'c_BuildDecorList', level=lv, files=True,
                       setup=lambda m, c=count, b=b198, lv=lv: (decor_n(c, 100 + c + lv)(m), m.wb(m.data('byte_B198'), b))))
    return cs

def entity_list(m):
    """Entity records at 8489h: size byte at +0 (signed step), X word at +2; the list ends at a size above 32h."""
    out, si = [], 0x8489
    while m.rb(si) <= 0x32:
        out.append(si)
        step = m.rb(si)
        si = (si + (step if step < 0x80 else step - 0x100)) & 0xFFFF
        if len(out) > 400:
            break
    return out

def rebase_gap_cases():
    cs = []
    def reb(title, lv, a, b, spread=None):
        def s(m):
            if spread:
                for k, si in enumerate(entity_list(m)):          # a spread of X values over the bank windows
                    m.ww(si + 2, spread[k % len(spread)])
            m.ww(m.data('SpriteBankA'), a)
            m.ww(m.data('SpriteBankB'), b)
        cs.append(case(title, 'RebaseEntitySpriteRefs', 'c_RebaseEntitySpriteRefs', level=lv, files=True, setup=s))
    # X at or above bank B: rebased by B; X below B and at or above A: by A; below both: kept; FFFFh: kept
    spread = (0x0010, 0x0035, 0x0040, 0x0100, 0x0200, 0xFFFF, 0x0120, 0x011F, 0x0030, 0x002F)
    reb('RebaseEntitySpriteRefs: X between the banks, window A 30h / B 120h', 4, 0x30, 0x120, spread)
    reb('RebaseEntitySpriteRefs: bank B above every X (all rebased by A)', 4, 0x00, 0x7FFF, spread)
    reb('RebaseEntitySpriteRefs: bank A above every X below B (all kept)', 9, 0x7FFE, 0x7FFF, spread)
    reb('RebaseEntitySpriteRefs: the shipped X values, window 30h / 8000h', 9, 0x30, 0x8000)
    return cs

# -- HUD extra life, fireflies and the horizontal camera follow (c/level.c routines, driven here with
# the same level states; tests/level.py holds the main matrix) ----------------------------------
HUD_SEG = 0x9800

def hud_sheet(m, seed=81):
    rng = random.Random(seed)
    m.write(HUD_SEG, 0, bytes(rng.randrange(256) for _ in range(0x4000)))
    m.ww(m.data('word_003D'), HUD_SEG)

def hud_extra_life(score, elb, lives=3, energy=3):
    def s(m):
        hud_sheet(m)
        set_cam(m, dp=0x2000, dd=0)
        m.wb(m.data('Lives'), lives); m.wb(m.data('Energy'), energy)
        m.ww(m.data('ScoreLow'), score & 0xFFFF); m.ww(m.data('ScoreHigh'), score >> 16)
        m.ww(m.data('ExtraLifeBlocks'), elb)
        m.wb(m.data('RespawnState'), 0); m.wb(m.data('BonusFlashTimer'), 0)
        m.wb(m.data('BonusLetterMask'), 0x15); m.wb(m.data('SnowFrameTick'), 0)
        m.ww(m.data('HudCacheScoreLow'), 0x1111); m.ww(m.data('HudCacheScoreHigh'), 0x2222)
        m.wb(m.data('HudCacheLives'), 0xEE); m.wb(m.data('HudCacheEnergy'), 0xEE); m.wb(m.data('HudCacheBonus'), 0xEE)
    return s

def hud_gap_cases():
    cs = []
    for score, elb, lives, tag in ((25000, 0, 3, 'one block earned: one life, debris spawned'),
                                   (99999, 0, 5, 'three blocks earned at once'),
                                   (75000, 2, 9, 'one block earned on top of nine lives'),
                                   (25000, 3, 4, 'blocks cache above the score (negative difference)'),
                                   (1000000, 0, 2, 'forty blocks earned at once')):
        cs.append(case(f'UpdateHud: extra life check, {tag}', 'UpdateHud', 'c_UpdateHud', level=4, files=True,
                       setup=hud_extra_life(score, elb, lives)))
    for energy, flash, tick in ((1, 0, 0), (0, 2, 1), (2, 5, 2)):
        def s(m, energy=energy, flash=flash, tick=tick):
            hud_extra_life(30000, 1, 3, energy)(m)
            m.wb(m.data('BonusFlashTimer'), flash); m.wb(m.data('SnowFrameTick'), tick)
        cs.append(case(f'UpdateHud: energy {energy} (empty pips), bonus flash {flash}, tick {tick}', 'UpdateHud',
                       'c_UpdateHud', level=4, files=True, setup=s))
    return cs

def firefly_records(seed, px, py, row, col, tick):
    """20 firefly records: x, y (1/8 pixel), vx, vy, timer, state. Timers expired (re-seed) or
    running; the player left of / right of and above / below the flies; two records near the
    signed overflow of x and one off screen."""
    def s(m):
        rng = random.Random(seed)
        set_cam(m, row=row, col=col, dp=0x2000)
        m.wb(m.data('SnowFrameTick'), tick)
        m.ww(m.data('PlayerX'), px); m.ww(m.data('PlayerY'), py)
        m.ww(m.data('RandomSeed'), 0x1234 + seed)
        m.wb(m.data('WindRandA'), seed); m.wb(m.data('WindRandB'), 7 * seed); m.wb(m.data('WindRandC'), 0x5A)
        m.ww(m.data('WindRandWord'), 0x1357 + seed)
        for k in range(20):
            a = 0x6EA9 + 8 * k
            if k == 1:
                m.ww(a, 0x55AA); continue                           # free record
            if k == 3:                                                # signed overflow of x going up
                m.ww(a, 0x7FFE); m.ww(a + 2, row * 128 + 0x200)
                m.wb(a + 4, 0x20); m.wb(a + 5, 0x03); m.wb(a + 6, 5); m.wb(a + 7, 0); continue
            if k == 4:                                                # signed overflow of x going down
                m.ww(a, 0x8003); m.ww(a + 2, row * 128 + 0x100)
                m.wb(a + 4, 0xE0); m.wb(a + 5, 0xFD); m.wb(a + 6, 4); m.wb(a + 7, 0); continue
            if k == 5:                                                # off screen below / right (kept running)
                m.ww(a, col * 128 + 0x4000); m.ww(a + 2, row * 128 + 0x4000)
                m.wb(a + 4, 1); m.wb(a + 5, 2); m.wb(a + 6, 6); m.wb(a + 7, 0); continue
            m.ww(a, (col * 128 + rng.randrange(0, 0x600)) & 0xFFFF)
            m.ww(a + 2, (row * 128 + rng.randrange(0, 0x500)) & 0xFFFF)
            m.wb(a + 4, rng.randrange(256)); m.wb(a + 5, rng.randrange(256))
            m.wb(a + 6, rng.choice((0, 0xFF, 0, 3, 0x80, 1, 0)))
            m.wb(a + 7, rng.choice((0, 1, 2, 7, 8, 0x13, 0xFF)))
    return s

def firefly_cases():
    cs = []
    for seed, px, py, row, col, tick, lv, tag in (
            (51, 0x0000, 0x0000, 8, 6, 0, 4, 'player at the top left: flies right of and below it, tick 0'),
            (52, 0x7000, 0x7000, 8, 6, 16, 9, 'player at the bottom right: flies left of and above it, tick 16'),
            (53, 0x0300, 0x0900, 4, 3, 5, 4, 'player left of the flies, below them'),
            (54, 0x0600, 0x0100, 4, 3, 8, 11, 'player right of the flies, above them'),
            (55, 0x0000, 0x0000, 8, 6, 0, 4, 'steep re-seeds: |vy| * 8 / |vx| reaches 20h and is capped'),
            (67, 0x0000, 0x0000, 8, 6, 0, 9, 'two capped speeds in one pass')):
        cs.append(case(f'UpdateFireflies: {tag}', 'UpdateFireflies', 'c_UpdateFireflies', level=lv, files=True,
                       setup=firefly_records(seed, px, py, row, col, tick)))
    return cs

def hcam_cases():
    cs = []
    def cam(title, col, px, hdir, vx=0, platform=0, rowring=0, vxhigh=0):
        def s(m):
            set_cam(m, row=8, col=col, fine=0, rowring=rowring, colring=5)
            m.ww(m.data('PlayerX'), px)
            m.wb(m.data('HScrollDir'), hdir)
            m.ww(m.data('PlayerVelX'), vx)
            m.wb(m.data('PlayerVelXHigh'), vxhigh)
            m.wb(m.data('PlayerOnPlatform'), platform)
        cs.append(case(title, 'UpdateHorizontalCamera', 'c_UpdateHorizontalCamera', level=4, files=True, setup=s))
    # HScrollDir 2 (scroll left) with the player column below 0Fh and a push in progress: the view
    # scrolls (CF clear) or hits the left map edge (CF set, direction cleared)
    cam('UpdateHorizontalCamera: scrolling left, column 5 of the view, push in progress', 20, (20 + 5) * 16, 2, vx=0x40)
    cam('UpdateHorizontalCamera: scrolling left on a platform, column 0Eh', 20, (20 + 14) * 16, 2, platform=1)
    cam('UpdateHorizontalCamera: scrolling left at the map edge (CameraCol 0): direction cleared',
        0, 5 * 16, 2, vx=0x40)
    cam('UpdateHorizontalCamera: scrolling right, column 10 of the view, push in progress', 20, (20 + 10) * 16, 1, vx=0x40)
    # idle (HScrollDir 0): the direction starts from the push direction or, without a push, the column
    cam('UpdateHorizontalCamera: idle, no push, platform, column 8: below 0Ah picks left, but 8 > 4', 20, (20 + 8) * 16, 0,
        platform=1)
    cam('UpdateHorizontalCamera: idle, no push, platform, column 3: starts the left scroll', 20, (20 + 3) * 16, 0,
        platform=1)
    cam('UpdateHorizontalCamera: idle, no push, column 15h: starts the right scroll', 20, (20 + 0x15) * 16, 0)
    cam('UpdateHorizontalCamera: idle, no push, column 12h: starts right (above 0Ah, 10h reached)', 20, (20 + 0x12) * 16, 0,
        platform=1)
    cam('UpdateHorizontalCamera: idle, rightward push, column 0Bh: below 10h, nothing starts', 20, (20 + 11) * 16, 0, vx=0x30)
    cam('UpdateHorizontalCamera: idle, rightward push, column 11h: starts right', 20, (20 + 0x11) * 16, 0, vx=0x30)
    cam('UpdateHorizontalCamera: idle, leftward push (high byte 80h), column 4: starts left', 20, (20 + 4) * 16, 0,
        vx=0xFFC0, vxhigh=0x80)
    cam('UpdateHorizontalCamera: idle, leftward push, column 9: above 4, nothing starts', 20, (20 + 9) * 16, 0,
        vx=0xFFC0, vxhigh=0x80)
    cam('UpdateHorizontalCamera: scrolling right, column 5 of the view: direction cleared', 20, (20 + 5) * 16, 1, vx=0x40)
    cam('UpdateHorizontalCamera: scrolling left, column 0Fh or more: direction cleared', 20, (20 + 0x0F) * 16, 2, vx=0x40)
    cam('UpdateHorizontalCamera: centre of the view, no push, not on a platform: idle', 20, (20 + 8) * 16, 2)
    return cs

# -- the per-run ranges and the assembled suite ---------------------------------------
CASES = (plain_cases() + palette_cases() + timer_cases() + wipe_cases() + blackout_cases() + iris_cases()
         + scroll_cases() + ring_cases() + tile_cases() + overlay_cases() + wind_cases()
         + ring_copy_cases() + level_cases() + fg_cases() + wind_script_cases() + ring_copy_gap_cases()
         + backdrop_wrap_cases() + decor_gap_cases() + rebase_gap_cases() + hud_gap_cases()
         + firefly_cases() + hcam_cases())
