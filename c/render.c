/* Video page and tile-ring routines (oracle: src/MODULE2.ASM from EnableVideoOutput at
 * cs:3034 to the end of the module). Covers the video enable/disable, the page flip and
 * the vertical-retrace wait, the level-entry wipes (RevealLevelColumns, ClearStaggeredBlocks,
 * RunIrisClose and its BlackoutSpan), the tile ring and its scroll and redraw passes, and
 * the level loader (LoadLevel) with its helpers.
 *
 * VGA memory (A000h) is accessed as the original does it: a read loads the latches
 * (vga_read), a write goes through the write mode (vga_write), and XCHG is read then write.
 * Port I/O goes through ports.h, which routes 3C0h..3DAh to the VGA model.
 */
#include "pre2.h"
#include "vga.h"
#include "ports.h"
#include "sprites.h"
#include "sqz.h"
#include "system.h"
#include "platform.h"
#include "render.h"
#include "player.h"
#include "sound.h"
#include "effects.h"

/* ---- helpers ------------------------------------------------------------------------- */

/* OUT DX, AX: AL to port, AH to port+1 (port_out16 does the same, VGA or not). */
#define OUT_AX(port, ax) port_out16((port), (ax))

/* ---- video output enable / disable (cs:3034, cs:3042) ------------------------------- */

/* Re-enables video output: reads 3DAh (resets the attribute flip-flop), then 20h to 3C0h. */
void EnableVideoOutput(void)
{
    (void)port_in8(0x3DA);
    port_out8(0x3C0, 0x20);
}

/* Blanks video output: reads 3DAh, then 00h to 3C0h (PAS clear). */
void DisableVideoOutput(void)
{
    (void)port_in8(0x3DA);
    port_out8(0x3C0, 0x00);
}

/* ---- level-entry column wipe (cs:3054, cs:309B) -------------------------------------- */

/* Copies one 2-byte column of 176 scanlines from DrawPage to DisplayPage (DI = column
 * offset in the page). Latch-copy mode and map mask 0Fh are set by the caller. */
void CopyColumnStrip(uint16_t di)
{
    uint16_t si = (uint16_t)(di + DW(DS_DrawPage));
    di = (uint16_t)(di + DW(DS_DisplayPage));
    for (int line = 0; line < 0xB0; line++) {
        vga_movsb(si, di); si++; di++;
        vga_movsb(si, di); si++; di++;
        di = (uint16_t)(di + 0x26);
        si = (uint16_t)(si + 0x26);
    }
}

void RevealLevelColumns(void)
{
    SetLatchCopyMode();
    SET_CW(CS_WipeColumnBase, 0x14);
    SET_CW(CS_WipeColumnOffset, 0);
    do {
        CopyColumnStrip(CW(CS_WipeColumnBase));
        CopyColumnStrip((uint16_t)(CW(CS_WipeColumnBase) + CW(CS_WipeColumnOffset)));
        WaitVerticalRetrace();
        WaitVerticalRetrace();
        SET_CW(CS_WipeColumnBase, (uint16_t)(CW(CS_WipeColumnBase) - 2));
        SET_CW(CS_WipeColumnOffset, (uint16_t)(CW(CS_WipeColumnOffset) + 4));
    } while (CW(CS_WipeColumnOffset) < 0x28);      /* jb: unsigned */
    SetCpuWriteMode();
}

/* ---- staggered block blank (cs:30C6) ------------------------------------------------- */

/* Zeroes one 16x10 block at DI: ten stosw, each followed by add di, 26h (stride 40). */
void ClearBlock16x10(uint16_t di)
{
    for (int line = 0; line < 10; line++) {
        vga_write(di, 0);
        vga_write((uint16_t)(di + 1), 0);          /* stosw: low byte, then high byte */
        di = (uint16_t)(di + 2 + 0x26);            /* stosw advances DI by 2; add di, 26h */
    }
}

void ClearStaggeredBlocks(void)
{
    SetCpuWriteMode();
    OUT_AX(0x3C4, 0x0F02);
    SET_DB(DS_FineScroll, 0);
    SET_CW(CS_WipeColumnBase, DW(DS_DisplayPage));
    SET_CW(CS_WipeColumnOffset, 0x19F0);
    uint16_t off;
    do {
        uint16_t di = CW(CS_WipeColumnBase);
        for (int k = 0; k < 0x14; k++) { ClearBlock16x10(di); di = (uint16_t)(di + 2); }
        di = (uint16_t)(CW(CS_WipeColumnBase) + CW(CS_WipeColumnOffset));
        for (int k = 0; k < 0x14; k++) { ClearBlock16x10(di); di = (uint16_t)(di + 2); }
        WaitVerticalRetrace();
        WaitVerticalRetrace();
        SET_CW(CS_WipeColumnBase, (uint16_t)(CW(CS_WipeColumnBase) + 0x190));
        off = CW(CS_WipeColumnOffset);
        SET_CW(CS_WipeColumnOffset, (uint16_t)(off - 0x320));
    } while (off >= 0x320);                         /* jae after sub: no borrow */
    SetCpuWriteMode();
}

/* ---- ClearDisplayPage (cs:314A) ------------------------------------------------------- */

void ClearDisplayPage(void)
{
    SetCpuWriteMode();
    OUT_AX(0x3C4, 0x0F02);
    uint16_t di = DW(DS_DisplayPage);
    for (int k = 0; k < 0xFA0; k++) {               /* rep stosw, CX = 0FA0h */
        vga_write(di, 0);
        vga_write((uint16_t)(di + 1), 0);
        di = (uint16_t)(di + 2);
    }
}

/* ---- iris close (cs:316F, BlackoutSpan cs:32DE) -------------------------------------- */

/* Blacks out a horizontal run of DrawPage (colour 0 in all planes). In: BX = x, CX = width,
 * DX = y. Returns at once when x >= 320 (unsigned), width > 320 or y >= 200. The width
 * check is CX <= 320 only: x + width is not clipped, so a run crossing the right edge
 * wraps into the next scanline. Preserves all registers. */
void BlackoutSpan(uint16_t bx, uint16_t cx, uint16_t dx)
{
    if (bx >= 0x140 || cx > 0x140 || dx >= 0xC8) return;
    uint16_t ax = (uint16_t)(0x28u * (uint8_t)dx) + DW(DS_DrawPage);
    uint16_t di = (uint16_t)((bx >> 3) + ax);      /* shr x3 then add ax */
    uint16_t width = cx;
    if ((bx & 7) != 0 || cx >= 8) {                  /* jne 3312h, else jb 3349h */
        uint8_t cl = (uint8_t)(bx & 7);
        port_out16(0x3CE, 0x0803);                   /* function select: AND */
        uint8_t al = (uint8_t)~(uint8_t)(0xFFu >> cl);
        cx = width;                                  /* pop cx */
        (void)vga_xchg(di, al);
        di++;
        port_out16(0x3CE, 0x0005);                   /* write mode 0 */
        port_out16(0x3CE, 0x0003);                   /* replace */
        bx &= 7;
        cx = (uint16_t)(cx + bx);
    } else {
        cx = width;
    }
    /* loc_3349: cx is the pushed value (width, or width + (x & 7) on the head path). */
    uint16_t run = (uint16_t)(cx >> 3);
    if (run != 0) {
        run--;
        for (uint16_t k = 0; k < run; k++) { vga_write(di, 0); di++; }   /* rep stosb */
    }
    uint8_t tail = (uint8_t)(cx & 7);
    if (tail != 0) {
        port_out16(0x3CE, 0x0803);
        uint8_t al = (uint8_t)(0xFFu >> tail);
        (void)vga_xchg(di, al);
    }
}

/* One iris frame of the RunIrisClose loop body (cs:31F4..32DD) is inlined below. */
void RunIrisClose(void)
{
    /* copy DrawPage to DisplayPage: rep movsb, CX = 1F40h, ds = es = A000h */
    uint16_t di = DW(DS_DisplayPage);
    uint16_t si = DW(DS_DrawPage);
    SetLatchCopyMode();
    SET_ES(0xA000);                                  /* ES stays A000h to the return */
    for (int k = 0; k < 0x1F40; k++) { vga_movsb(si, di); si++; di++; }

    /* render slots 2.. to empty (+4 = FFFFh). The second loop writes two entries more
     * than the first (0x73 against 0x71); it is not redundant. */
    uint16_t d = 0x4F1C;
    for (int k = 0; k < 0x71; k++) { d = (uint16_t)(d + 0x12); SET_DW((uint16_t)(d + 4), 0xFFFF); }
    d = 0x4F1C;
    for (int k = 0; k < 0x73; k++) { d = (uint16_t)(d + 0x12); SET_DW((uint16_t)(d + 4), 0xFFFF); }

    /* IrisCenterX = -(CameraCol*16 - PlayerX) */
    uint16_t cxv = (uint16_t)(0 - (uint16_t)((uint16_t)(DW(DS_CameraCol) << 4) - DW(DS_PlayerX)));
    SET_DW(DS_IrisCenterX, cxv);
    /* IrisCenterY = -(CameraRow*16 - PlayerY) - 10h - FineScroll (byte sub, then sbb ah) */
    uint16_t ay = (uint16_t)(0 - (uint16_t)((uint16_t)(DW(DS_CameraRow) << 4) - DW(DS_PlayerY)));
    ay = (uint16_t)(ay - 0x10);
    uint8_t lo = (uint8_t)ay, fs = DB(DS_FineScroll);
    int borrow = lo < fs;
    lo = (uint8_t)(lo - fs);
    uint8_t hi = (uint8_t)((ay >> 8) - borrow);
    ay = (uint16_t)(hi << 8 | lo);
    SET_DW(DS_IrisCenterY, ay);
    /* word_2DC4 = max(2*ay, 0F0h) unsigned */
    uint16_t top = (uint16_t)(ay << 1);
    if (top < 0xF0) top = 0xF0;
    SET_DW(DS_word_2DC4, top);

    SET_DW(DS_IrisRadius, 0xE6);
    SET_DW(DS_IrisSpeed, 4);
    SET_DW(DS_IrisSpeedCounter, 0);

    for (;;) {                                       /* loc_31F4: one frame */
        uint16_t bp = 0;
        uint16_t n = 0;                              /* si in the point tables */
        SET_DW(DS_word_2DCC, 0x7D0);
        SET_DW(DS_IrisAngleIndex, 0);
        for (;;) {                                   /* loc_3204: 65 boundary points */
            uint16_t bx = DW(DS_IrisAngleIndex);
            uint16_t r = DW(DS_IrisRadius);
            /* mul byte ptr IrisRadius: AX = AL * low byte; sar ax, 6 (signed) */
            uint16_t p = (uint16_t)(DB((uint16_t)(DS_byte_7090 + bx)) * (uint8_t)r);
            uint16_t y = (uint16_t)((int16_t)p >> 6);
            y = (uint16_t)(y + DW(DS_IrisCenterY));
            if ((int16_t)y < (int16_t)DW(DS_word_2DCC) && (int16_t)y < (int16_t)DW(DS_word_2DC4)) {
                SET_DW(DS_word_2DCC, y);
                SET_DW((uint16_t)(DS_word_6B14 + n), y);
                uint16_t q = (uint16_t)(DB((uint16_t)(DS_SineTable + bx)) * (uint8_t)r);
                uint16_t x = (uint16_t)((int16_t)q >> 6);
                x = (uint16_t)(x + DW(DS_IrisCenterX));
                SET_DW((uint16_t)(DS_word_6A88 + n), x);
                n = (uint16_t)(n + 2);
                bp++;
            }
            SET_DW(DS_IrisAngleIndex, (uint16_t)(DW(DS_IrisAngleIndex) + 1));
            if (DW(DS_IrisAngleIndex) > 0x40) break;   /* jbe loop: unsigned */
        }
        n = 0;
        SET_DW(DS_word_2DCA, DW(DS_IrisCenterX));
        SET_DW(DS_word_2DD2, DW(DS_word_2DC4));
        for (;;) {                                   /* loc_3259: one scanline per step */
            if (DW((uint16_t)(DS_word_6B14 + n)) == DW(DS_word_2DD2)) {
                bp = (uint16_t)(bp - 1);
                if ((int16_t)bp < 0) break;          /* js loc_32B0 */
                SET_DW(DS_word_2DCA, DW((uint16_t)(DS_word_6A88 + n)));
                n = (uint16_t)(n + 2);
            }
            SetCpuWriteMode();
            OUT_AX(0x3C4, 0x0F02);
            uint16_t dx = DW(DS_word_2DD2);
            uint16_t bxs = DW(DS_word_2DCA);
            uint16_t cx = (uint16_t)(0x140 - bxs);
            BlackoutSpan(bxs, cx, dx);
            uint16_t dy = (uint16_t)((DW(DS_IrisCenterY) << 1) - dx);
            BlackoutSpan(bxs, cx, dy);
            uint16_t cx2 = (uint16_t)((DW(DS_IrisCenterX) << 1) - bxs);
            BlackoutSpan(0, cx2, dy);
            BlackoutSpan(0, cx2, dx);
            SET_DW(DS_word_2DD2, (uint16_t)(DW(DS_word_2DD2) - 1));
            if (DW(DS_word_2DD2) == 0) break;        /* je loc_32B0 */
        }
        /* loc_32B0 */
        SET_DW(DS_IrisSpeedCounter, (uint16_t)(DW(DS_IrisSpeedCounter) + 1));
        if (DW(DS_IrisSpeedCounter) >= 0x14) {       /* jb loc_32C5 otherwise */
            SET_DW(DS_IrisSpeedCounter, 0);
            SET_DW(DS_IrisSpeed, (uint16_t)(DW(DS_IrisSpeed) + 1));
        }
        DrawSpriteObjects();
        FlipDisplayPage();
        WaitVerticalRetrace();
        StepPaletteFade();
        uint16_t speed = DW(DS_IrisSpeed);
        uint16_t old = DW(DS_IrisRadius);
        SET_DW(DS_IrisRadius, (uint16_t)(old - speed));
        /* sub leaves flags for jle: signed old <= speed */
        if ((int16_t)old <= (int16_t)speed) break;
    }
}

/* ---- tile cache, backdrop and tile blits (cs:3080..3327) ---------------------------- */

#define TILE_CACHE_VRAM   0x5E80u   /* VRAM slot of tile t: 5E80h + 32*t, same offset in each plane (no label) */
#define TILE_MASK_TABLE   0x2DF8u   /* 32-byte AND masks per class: 2DF8h + 32*(class-2) (no label) */
#define RING_BASE         0x3F40u   /* tile ring origin in VRAM */
#define RING_WRAP_AT      0x5D40u   /* ring offsets at or past this wrap back by RING_WRAP */
#define RING_WRAP         0x1E00u
#define BACKDROP_BASE     0x7E80u   /* backdrop ring origin in VRAM */

/* RestoreTileBackdrop (cs:3328): 2 bytes x 16 scanlines from the backdrop ring at
 * BackdropCursor - 40*FineScroll to ES:di (A000h). The source offset is not wrapped; the
 * destination wraps at 5D40h. Returns DI + 2 (the routine's push/pop of DI and its final
 * inc di twice). */
uint16_t RestoreTileBackdrop(uint16_t di)
{
    uint16_t di0 = di;
    uint16_t si = (uint16_t)(DW(DS_BackdropCursor) - (uint16_t)(0x28u * DB(DS_FineScroll)));
    for (int row = 0; row < 16; row++) {
        vga_movsb(si, di); si++; di++;
        vga_movsb(si, di); si++; di++;
        if (row < 15) {
            di = (uint16_t)(di + 0x26);
            si = (uint16_t)(si + 0x26);
            if (di >= RING_WRAP_AT) di = (uint16_t)(di - RING_WRAP);
        }
    }
    return (uint16_t)(di0 + 2);
}

/* OR of a word into A000h:off (or word ptr [di], ax): both bytes read, then both written. */
static void vmem_or16(uint16_t off, uint16_t w)
{
    uint8_t lo = vga_read(off), hi = vga_read((uint16_t)(off + 1));
    vga_write(off, (uint8_t)(lo | (w & 0xFF)));
    vga_write((uint16_t)(off + 1), (uint8_t)(hi | (w >> 8)));
}

/* lodsw from A000h:si. */
static uint16_t vmem_lodsw(uint16_t si)
{
    uint8_t lo = vga_read(si), hi = vga_read((uint16_t)(si + 1));
    return (uint16_t)(lo | hi << 8);
}

/* BlitTileCell (cs:3100): tile AL (AH ignored) to A000h:di. In latch-copy mode for class 0
 * and 1; class >= 2 switches to the masked path and back to latch-copy mode. Returns DI + 2.
 *   class 0: latch copy of the cache slot (16 rows of 2 bytes, read then write).
 *   class 1: backdrop only (RestoreTileBackdrop).
 *   class >= 2: backdrop, then the AND mask of the class, then each plane OR-ed in (the
 *   planes are read back from the cache slot with read map select = plane). */
uint16_t BlitTileCell(uint16_t ax, uint16_t di)
{
    uint16_t tile = (uint8_t)ax;                 /* xor ah, ah */
    uint8_t cls = DB(DS_TileClassTable + tile);
    if (cls == 0) {
        uint16_t si = (uint16_t)(tile * 32u + TILE_CACHE_VRAM);
        for (int row = 0; row < 16; row++) {
            vga_movsb(si, di); si++; di++;
            vga_movsb(si, di); si++; di++;
            if (row < 15) di = (uint16_t)(di + 0x26);    /* add di, bp (bp = 26h) */
        }
        return (uint16_t)(di - 0x258);
    }
    if (cls == 1) return RestoreTileBackdrop(di);

    uint16_t done = RestoreTileBackdrop(di);     /* DI = di + 2, pushed */
    uint16_t dd = (uint16_t)(done - 2);          /* dec di twice */
    uint16_t mask = (uint16_t)(TILE_MASK_TABLE + (uint16_t)((uint8_t)(cls - 2) * 32u));
    uint16_t slot = (uint16_t)(tile * 32u + TILE_CACHE_VRAM);

    SetCpuWriteMode();
    port_out16(0x3C4, 0x0F02);                   /* map mask 0Fh */
    port_out16(0x3CE, 0x0803);                   /* function AND */
    for (int row = 0; row < 16; row++) {
        uint16_t w = DW((uint16_t)(mask + 2 * row));   /* DATA, through DS */
        (void)vga_xchg((uint16_t)(dd + 40 * row), (uint8_t)w);
        (void)vga_xchg((uint16_t)(dd + 40 * row + 1), (uint8_t)(w >> 8));
    }
    port_out16(0x3CE, 0x0003);                   /* function replace */
    port_out8(0x3CE, 0x04);                      /* read map select index */
    port_out8(0x3C4, 0x02);                      /* map mask index */
    uint8_t plane = 0, bit = 1;
    do {
        port_out8(0x3C5, bit);                   /* map mask = plane bit */
        port_out8(0x3CF, plane);                 /* read map select = plane */
        for (int row = 0; row < 16; row++) {
            uint16_t w = vmem_lodsw((uint16_t)(slot + 2 * row));
            vmem_or16((uint16_t)(dd + 40 * row), w);
        }
        plane++;
        bit = (uint8_t)(bit << 1);
    } while (bit < 0x10);
    SetLatchCopyMode();
    return done;
}

/* BlitTileToVram (cs:3084): AL = tile, DI = VRAM destination. Returns DI + 2. */
uint16_t BlitTileToVram(uint16_t ax, uint16_t di) { return BlitTileCell(ax, di); }

/* BlitTileToRing (cs:3080): DI = ring offset, adds the ring origin, then BlitTileToVram. */
uint16_t BlitTileToRing(uint16_t ax, uint16_t di)
{
    return BlitTileToVram(ax, (uint16_t)(di + RING_BASE));
}

/* Tile class / flag lookups shared by the row and column draws: the three tables are OR-ed
 * into the frame flags for the level tile t (xlat through ES = DATA). */
static void or_tile_flags(uint8_t t)
{
    SET_DB(DS_AnimVisibleFlag, DB(DS_AnimVisibleFlag) | DB((uint16_t)(DS_AnimTileFlags + t)));
    SET_DB(DS_VisibleTileFlags, DB(DS_VisibleTileFlags) | DB((uint16_t)(DS_TileFlagsTable + t)));
    SET_DB(DS_GridDirtyFlag, DB(DS_GridDirtyFlag) | DB((uint16_t)(DS_TileClassTable + t)));
}

/* ComputeRingTileOffset (cs:3588): RingTileOffset = 3F40h + 2*col + 640*RowRing. */
void ComputeRingTileOffset(uint16_t col)
{
    uint16_t ax = (uint16_t)(col << 1);
    uint16_t dx = (uint16_t)(DB(DS_RowRing) << 8);   /* mov dh, RowRing; dl = 0 */
    uint16_t bx = (uint16_t)(dx << 1);
    ax = (uint16_t)(ax + (uint16_t)(dx >> 1) + bx + RING_BASE);
    SET_DW(DS_RingTileOffset, ax);
}

/* DrawTileRow (cs:348D): 20 tiles of the map row at AX (AH = row, AL = column) into the ring
 * at DI (relative to RingTileOffset). Ends with DI moved back by 40 at each ColumnRing wrap. */
void DrawTileRow(uint16_t map, uint16_t di)
{
    SET_DW(DS_BackdropCursor, (uint16_t)(di + BACKDROP_BASE));
    SetLatchCopyMode();
    uint16_t si = map;
    di = (uint16_t)(di + DW(DS_RingTileOffset));
    uint16_t dx = DW(DS_ColumnRing);
    uint16_t lvl = DW(DS_LevelSegment);
    for (int k = 0; k < 0x14; k++) {
        if (di >= RING_WRAP_AT) di = (uint16_t)(di - RING_WRAP);
        uint8_t t = rd8(lvl, si); si++;
        or_tile_flags(t);
        di = BlitTileCell(t, di);
        SET_DW(DS_BackdropCursor, (uint16_t)(DW(DS_BackdropCursor) + 2));
        dx = (uint16_t)(dx + 1);
        if (dx >= 0x14) { di = (uint16_t)(di - 0x28); dx = 0; }
    }
}

/* DrawTileColumn (cs:350C): 12 tiles of the map column at AX (top tile) into ring column DI. */
void DrawTileColumn(uint16_t map, uint16_t col)
{
    SET_DW(DS_BackdropCursor, (uint16_t)(col + BACKDROP_BASE));
    SetLatchCopyMode();
    uint16_t si = map;
    uint16_t di = (uint16_t)(col << 1);
    di = (uint16_t)(di + DW(DS_RingTileOffset));
    uint16_t ax = (uint16_t)(col + DW(DS_ColumnRing));
    if (ax >= 0x14) di = (uint16_t)(di - 0x28);
    uint16_t lvl = DW(DS_LevelSegment);
    for (int k = 0; k < 0x0C; k++) {
        if (di >= RING_WRAP_AT) di = (uint16_t)(di - RING_WRAP);
        uint8_t t = rd8(lvl, si); si++;
        or_tile_flags(t);
        di = BlitTileCell(t, di);
        SET_DW(DS_BackdropCursor, (uint16_t)(DW(DS_BackdropCursor) + 0x40));
        si = (uint16_t)(si + 0xFF);
        di = (uint16_t)(di + 0x27E);
    }
}

/* ScrollViewUp (cs:3363): one scanline up. dl = FineScroll step. Returns CF. */
int ScrollViewUp(uint8_t dl)
{
    if (DW(DS_CameraRow) == 0) return 1;
    SET_DB(DS_ScrollAnimCounter, (uint8_t)(DB(DS_ScrollAnimCounter) + 1));
    uint8_t fs = (uint8_t)(DB(DS_FineScroll) - dl);
    SET_DB(DS_FineScroll, fs);
    if (fs & 0x80) {                             /* js not taken: wrapped below zero */
        SET_DB(DS_FineScroll, (uint8_t)(DB(DS_FineScroll) + 0x10));
        SET_DW(DS_CameraRow, (uint16_t)(DW(DS_CameraRow) - 1));
        uint16_t ax = (uint16_t)(DW(DS_RowRing) - 1);
        if ((int16_t)ax < 0) ax = 0x0B;
        SET_DW(DS_RowRing, ax);
        ComputeRingTileOffset(DW(DS_ColumnRing));
        uint16_t map = (uint16_t)((DB(DS_CameraRow) << 8) | DB(DS_CameraCol));
        DrawTileRow(map, 0);
    }
    return 0;
}

/* ScrollViewDown (cs:33AD): one scanline down. Returns CF. */
int ScrollViewDown(uint8_t dl)
{
    uint16_t lim = (uint16_t)(DB(DS_MapRows) - 0x0B);
    if (DW(DS_CameraRow) >= lim) return 1;
    SET_DB(DS_ScrollAnimCounter, (uint8_t)(DB(DS_ScrollAnimCounter) + 1));
    SET_DB(DS_FineScroll, (uint8_t)(DB(DS_FineScroll) + dl));
    if (DB(DS_FineScroll) < 0x10) return 0;
    SET_DB(DS_FineScroll, (uint8_t)(DB(DS_FineScroll) - 0x10));
    SET_DW(DS_CameraRow, (uint16_t)(DW(DS_CameraRow) + 1));
    if (DW(DS_CameraRow) >= lim) SET_DB(DS_FineScroll, 0);
    uint16_t ax = (uint16_t)(DW(DS_RowRing) + 1);
    if (ax >= 0x0C) ax = 0;
    SET_DW(DS_RowRing, ax);
    ComputeRingTileOffset(DW(DS_ColumnRing));
    uint16_t map = (uint16_t)((uint16_t)((DB(DS_CameraRow) << 8) | DB(DS_CameraCol)) + 0x0B00);
    DrawTileRow(map, 0x1B80);
    return 0;
}

/* ScrollViewLeft (cs:3145): one tile column left. Returns CF. */
int ScrollViewLeft(void)
{
    if (DW(DS_CameraCol) == 0) return 1;
    SET_DW(DS_CameraCol, (uint16_t)(DW(DS_CameraCol) - 1));
    uint16_t ax = (uint16_t)(DW(DS_ColumnRing) - 1);
    if ((int16_t)ax < 0) ax = 0x13;
    SET_DW(DS_ColumnRing, ax);
    ComputeRingTileOffset(ax);
    uint16_t map = (uint16_t)(((DB(DS_CameraRow) << 8) | DB(DS_CameraCol)) + 0);
    DrawTileColumn(map, 0);
    return 0;
}

/* ScrollViewRight (cs:166?): one tile column right. Returns CF. */
int ScrollViewRight(void)
{
    uint16_t ax = (uint16_t)((int16_t)DW(DS_PlayerX) >> 4);
    ax = (uint16_t)(ax - 0x14);
    if ((int16_t)DW(DS_CameraLeftLimit) >= (int16_t)ax) ax = DW(DS_CameraLeftLimit);
    else ax = 0xEC;
    if (DW(DS_CameraCol) >= ax) return 1;
    uint16_t di = 0x13;
    SET_DW(DS_CameraCol, (uint16_t)(DW(DS_CameraCol) + 1));
    ax = (uint16_t)(DW(DS_ColumnRing) + 1);
    if (ax >= 0x14) ax = 0;
    SET_DW(DS_ColumnRing, ax);
    ComputeRingTileOffset(ax);
    uint16_t map = (uint16_t)(((DB(DS_CameraRow) << 8) | DB(DS_CameraCol)) + di);
    DrawTileColumn(map, di);
    return 0;
}

/* RedrawOverlayTiles (cs:3588+ / 35A1): redraw the visible 20x12 window's overlay tiles
 * (class >= 1) when the scroll animation counter is set, or the camera cell changed and the
 * grid is dirty. PrevCameraRow is updated only when the column matched (the original's
 * compare sequence). */
void RedrawOverlayTiles(void)
{
    if (DB(DS_ScrollAnimCounter) == 0) {
        uint16_t cc = DW(DS_CameraCol);
        uint16_t pc = DW(DS_PrevCameraCol);
        SET_DW(DS_PrevCameraCol, cc);
        if (pc == cc) {
            uint16_t cr = DW(DS_CameraRow);
            uint16_t pr = DW(DS_PrevCameraRow);
            SET_DW(DS_PrevCameraRow, cr);
            if (pr == cr) return;
        }
    }
    /* loc_35D1: a dirty grid or an animation step redraws; otherwise nothing to do. */
    if (DB(DS_ScrollAnimCounter) == 0 && DB(DS_GridDirtyFlag) == 0) return;

    SetLatchCopyMode();
    SET_DB(DS_GridDirtyFlag, 0);
    SET_DB(DS_ScrollAnimCounter, 0);
    SET_DB(DS_VisibleTileFlags, 0);
    uint16_t si = (uint16_t)((DB(DS_CameraRow) << 8) | DB(DS_CameraCol));
    uint16_t di = DW(DS_RingTileOffset);
    SET_DW(DS_BackdropCursor, BACKDROP_BASE);
    uint16_t lvl = DW(DS_LevelSegment);
    for (int row = 0; row < 12; row++) {
        uint16_t dx = DW(DS_ColumnRing);
        for (int col = 0; col < 20; col++) {
            uint8_t t = rd8(lvl, si);
            SET_DB(DS_VisibleTileFlags, DB(DS_VisibleTileFlags) | DB((uint16_t)(DS_TileFlagsTable + t)));
            if (DB((uint16_t)(DS_TileClassTable + t)) >= 1) {
                SET_DB(DS_GridDirtyFlag, 1);
                if (di >= RING_WRAP_AT) di = (uint16_t)(di - RING_WRAP);
                (void)BlitTileToVram(t, di);     /* leaves DI + 2; the original decs twice */
            }
            dx = (uint16_t)(dx + 1);
            if (dx >= 0x14) { di = (uint16_t)(di - 0x28); dx = 0; }
            SET_DW(DS_BackdropCursor, (uint16_t)(DW(DS_BackdropCursor) + 2));
            di = (uint16_t)(di + 2);
            si = (uint16_t)(si + 1);
        }
        SET_DW(DS_BackdropCursor, (uint16_t)(DW(DS_BackdropCursor) + 0x258));
        di = (uint16_t)(di + 0x280);
        si = (uint16_t)(si + 0xEC);
    }
}

/* AnimateTiles (cs:3668): advance the tile animation phase (every 4 ticks, or 2 when
 * SnowWind >= 14h) and redraw the animated tiles of the window through AnimRemapPtr. */
void AnimateTiles(void)
{
    SetLatchCopyMode();
    if (DB(DS_AnimVisibleFlag) == 0) return;
    uint8_t gate = 3;
    if (DW(DS_SnowWind) >= 0x14) gate = 1;          /* cmp SnowWind, 14h; jb (unsigned) */
    SET_DB(DS_AnimTickCounter, (uint8_t)(DB(DS_AnimTickCounter) + 1));
    if (DB(DS_AnimTickCounter) & gate) return;
    uint16_t ax = DW(DS_AnimRemapPtr);
    ax = (uint16_t)((((ax >> 8) + 1) & 0xFF) << 8 | (ax & 0xFF));   /* inc ah */
    if (ax == 0x6988) ax = (uint16_t)(ax - 0x300);                  /* sub ah, 3 */
    SET_DW(DS_AnimRemapPtr, ax);
    SET_DB(DS_VisibleTileFlags, 0);
    SET_DB(DS_AnimVisibleFlag, 0);
    uint16_t si = (uint16_t)((DB(DS_CameraRow) << 8) | DB(DS_CameraCol));
    uint16_t di = DW(DS_RingTileOffset);
    uint16_t lvl = DW(DS_LevelSegment);
    for (int row = 0; row < 11; row++) {
        uint16_t dx = DW(DS_ColumnRing);
        for (int col = 0; col < 20; col++) {
            uint8_t t = rd8(lvl, si);
            SET_DB(DS_VisibleTileFlags, DB(DS_VisibleTileFlags) | DB((uint16_t)(DS_TileFlagsTable + t)));
            if (DB((uint16_t)(DS_AnimTileFlags + t)) != 0) {
                SET_DB(DS_AnimVisibleFlag, 1);
                /* xlat with BX = AnimRemapPtr and AL = tile */
                uint8_t remapped = DB((uint16_t)(DW(DS_AnimRemapPtr) + t));
                if (di >= RING_WRAP_AT) di = (uint16_t)(di - RING_WRAP);
                (void)BlitTileToVram(remapped, di);
            }
            dx = (uint16_t)(dx + 1);
            if (dx >= 0x14) { di = (uint16_t)(di - 0x28); dx = 0; }
            di = (uint16_t)(di + 2);
            si = (uint16_t)(si + 1);
        }
        di = (uint16_t)(di + 0x280);
        si = (uint16_t)(si + 0xEC);
    }
}

/* DrawForegroundTile (cs:37F7): one foreground tile on DrawPage, colour 0 transparent.
 * In: AX = map cell (row in AH, column in AL), BX = tile id. The graphic is the 128-byte
 * entry of the word table at 8167h + 2*tile in the segment held at DS:003Bh. The AND pass
 * writes the inverted OR of the four planes, then each plane is OR-ed in with its map
 * mask. Preserves all registers. */
#define FG_TILE_TABLE 0x8167u   /* word table of graphic offsets 8167h + 2*tile (no label) */

void DrawForegroundTile(uint16_t ax, uint16_t bx)
{
    uint16_t seg = DW(DS_word_003B);
    uint16_t si = (uint16_t)(rd16(DATA_SEG, (uint16_t)(FG_TILE_TABLE + (bx << 1))) << 7);
    uint8_t row = (uint8_t)((ax >> 8) - DB(DS_CameraRow));
    uint8_t col = (uint8_t)(ax - DB(DS_CameraCol));
    uint16_t di = (uint16_t)(((uint16_t)row * 4 * 0xA0) + (uint16_t)col * 2 + DW(DS_DrawPage));
    di = (uint16_t)(di - (uint16_t)(0x28u * DB(DS_FineScroll)));
    uint16_t dp = DW(DS_DrawPage);
    if (di < dp) return;
    if (di >= (uint16_t)(dp + 0x1900)) return;

    port_out16(0x3C4, 0x0F02);
    port_out16(0x3CE, 0x0803);                  /* function AND */
    uint16_t sp = si;
    for (int k = 0; k < 16; k++) {
        uint16_t b = sp;
        uint16_t w = (uint16_t)(rd16(seg, b) | rd16(seg, (uint16_t)(b + 0x20)) |
                                rd16(seg, (uint16_t)(b + 0x40)) | rd16(seg, (uint16_t)(b + 0x60)));
        w = (uint16_t)~w;
        sp = (uint16_t)(b + 2);
        (void)vga_xchg((uint16_t)(di + 40 * k), (uint8_t)w);
        (void)vga_xchg((uint16_t)(di + 40 * k + 1), (uint8_t)(w >> 8));
    }
    sp = si;                                    /* sub si, 20h: back to plane 0 */
    port_out16(0x3CE, 0x0003);                  /* function replace */
    port_out8(0x3C4, 0x02);
    port_out8(0x3CE, 0x04);
    uint8_t bl = 1, bh = 0;
    uint16_t top = di;                           /* the original's `sub di, 280h`: each plane starts here */
    do {
        port_out8(0x3C5, bl);
        port_out8(0x3CF, bh);
        di = top;
        for (int k = 0; k < 16; k++) {
            uint16_t w = rd16(seg, sp); sp = (uint16_t)(sp + 2);
            vmem_or16(di, w);
            di = (uint16_t)(di + 0x28);
        }
        bh++;
        bl = (uint8_t)(bl << 1);
    } while (bl < 0x10);
}

/* DrawForegroundOverSprites (cs:3721): foreground tiles (TileFlagsTable bit 40h) next to each
 * render slot with bit 2000h set in +4 (slot loop from 4F0Ah to 5732h, 12h per slot). Gated
 * by OptionBits bit 2 and VisibleTileFlags bit 40h. */
void DrawForegroundOverSprites(void)
{
    if (!(CW(CS_OptionBits) & 2)) return;
    if (!(DB(DS_VisibleTileFlags) & 0x40)) return;
    uint16_t lvl = DW(DS_LevelSegment);
    SET_ES(lvl);                                /* ES = LevelSegment on every return past the gates */
    SetCpuWriteMode();
    for (uint16_t si = DS_RenderSlots; si < 0x5732; si = (uint16_t)(si + 0x12)) {
        uint16_t bx = DW((uint16_t)(si + 4));
        if (bx == 0xFFFF) continue;
        if (!(bx & 0x2000)) continue;
        uint16_t a = (uint16_t)((int16_t)DW((uint16_t)(si + 2)) >> 4);
        a = (uint16_t)(a - 1);
        a = (uint16_t)((a << 8) | (a >> 8));    /* xchg al, ah */
        uint16_t di = (uint16_t)((int16_t)DW(si) >> 4);
        di = (uint16_t)(di + a);
        uint8_t ch = (uint8_t)((di >> 8) - DB(DS_CameraRow));
        uint8_t cl = (uint8_t)(di - DB(DS_CameraCol));
        int dx = 3;
        if (DB((uint16_t)(si + 2)) & 0x0F) { di = (uint16_t)(di + 0x100); ch++; dx++; }
        do {
            if (ch < 0x0B) {                     /* cmp ch, 0Bh; jae skip body (unsigned) */
                ch--;
                if ((int8_t)cl >= 2) {
                    uint8_t t = rd8(lvl, (uint16_t)(di - 2));
                    if (DB((uint16_t)(DS_TileFlagsTable + t)) & 0x40) DrawForegroundTile((uint16_t)(di - 2), t);
                }
                if ((int8_t)cl >= 1) {
                    uint8_t t = rd8(lvl, (uint16_t)(di - 1));
                    if (DB((uint16_t)(DS_TileFlagsTable + t)) & 0x40) DrawForegroundTile((uint16_t)(di - 1), t);
                }
                if ((int8_t)cl >= 0 && (int8_t)cl < 0x14) {
                    uint8_t t = rd8(lvl, di);
                    if (DB((uint16_t)(DS_TileFlagsTable + t)) & 0x40) DrawForegroundTile(di, t);
                }
                if ((int8_t)cl < 0x13 && (int8_t)cl >= -1) {
                    uint8_t t = rd8(lvl, (uint16_t)(di + 1));
                    if (DB((uint16_t)(DS_TileFlagsTable + t)) & 0x40) DrawForegroundTile((uint16_t)(di + 1), t);
                }
            }
            di = (uint16_t)(di - 0x100);
            dx--;
        } while (dx != 0);
    }
}

/* NextWindRandom (cs:39DF): four-byte mixing generator; returns the new WindRandB. */
uint8_t NextWindRandom(void)
{
    uint16_t dx = (uint16_t)(DW(DS_WindRandWord) + DB(DS_WindRandA));
    SET_DW(DS_WindRandWord, dx);
    uint8_t al = (uint8_t)(DB(DS_WindRandA) + 3 + (uint8_t)(dx >> 8));
    SET_DB(DS_WindRandA, al);
    uint8_t bl = (uint8_t)(DB(DS_WindRandB) + DB(DS_WindRandC));
    bl = (uint8_t)(bl + bl);
    bl = (uint8_t)(bl + al);
    SET_DB(DS_WindRandB, bl);
    uint8_t cl = (uint8_t)(DB(DS_WindRandC) ^ al);
    cl = (uint8_t)(cl ^ bl);
    SET_DB(DS_WindRandC, cl);
    return DB(DS_WindRandB);
}

/* UpdateWindAndFlakes (cs:3922): steps the wind script every 4th frame, then moves and plots
 * the flakes while SnowWind != 0. Per flake: the word gets +4Fh, the x is wrapped (AND 1Fh in
 * the high byte, -1B58h past it), and a 2-pixel dot at the rotated position is OR-ed in. */
void UpdateWindAndFlakes(void)
{
    SET_DW(DS_ScriptFrameCounter, (uint16_t)(DW(DS_ScriptFrameCounter) + 1));
    if ((DB(DS_SnowFrameTick) & 3) == 0) {
        uint16_t ax = DW(DS_ScriptFrameCounter);
        uint16_t bx = DW(DS_ScriptPointer);
        if (DW(bx) != 0xFFFF) {
            if (DW(bx) < ax) {                  /* jae skips the wind step */
                SET_DW(DS_SnowWind, (uint16_t)(DW(DS_SnowWind) + DW((uint16_t)(bx + 2))));
                if (DW((uint16_t)(bx + 6)) < ax)
                    SET_DW(DS_ScriptPointer, (uint16_t)(DW(DS_ScriptPointer) + 6));
            }
            /* loc_394E: clamp to 0 and to the entry's limit (signed compares) */
            ax = DW(DS_SnowWind);
            if ((int16_t)ax < 0) ax = 0;
            if ((int16_t)DW((uint16_t)(bx + 4)) < (int16_t)ax) ax = DW((uint16_t)(bx + 4));
            SET_DW(DS_SnowWind, ax);
        }
    }
    if (DW(DS_SnowWind) == 0) return;

    SET_ES(0xA000);                             /* ES = A000h on the flake path; the early return keeps it */
    port_out16(0x3CE, 0x1003);                  /* function OR */
    port_out16(0x3CE, 0x0005);                  /* write mode 0 */
    port_out16(0x3C4, 0x0F02);
    uint16_t dx = DW(DS_SnowWind);
    uint16_t bp = (uint16_t)(dx >> 1);
    uint16_t si = DS_FlakeTable;
    do {
        SET_DW(si, (uint16_t)(DW(si) + 0x4F));
        uint16_t ax = DW(si);
        si = (uint16_t)(si + 2);
        uint16_t bx = ax;
        uint8_t cl = (uint8_t)ax;
        cl = (uint8_t)((cl << 1) | (cl >> 7));  /* rol cl, 1 */
        cl &= 3;
        bx = (uint16_t)(bx - DW(DS_CameraCol));
        bx &= 0x1FFF;                           /* and bh, 1Fh */
        if (bx >= 0x1B58) {
            bx = (uint16_t)(bx - 0x1B58);
            SET_DW((uint16_t)(si - 2), bx);
        }
        bx = (uint16_t)(bx + DW(DS_DrawPage));
        uint16_t shifted = (uint16_t)(0x302u << cl);
        (void)vga_xchg(bx, (uint8_t)shifted);
        if (dx > bp) (void)vga_xchg((uint16_t)(bx + 0x28), (uint8_t)(shifted >> 8));
        dx = (uint16_t)(dx - 1);
    } while (dx != 0);
    uint16_t bx = (uint16_t)(NextWindRandom() << 1);
    uint8_t dh = NextWindRandom();
    uint8_t r3 = NextWindRandom();
    SET_DW((uint16_t)(DS_FlakeTable + bx), (uint16_t)(dh << 8 | r3));
}

/* ---- mode, palette and page routines (cs:3034 region tail: cs:43D2..452B) -------------- */

#define PALETTE_PROBE_TABLE  0x2A7Cu   /* 30 DAC bytes written by CheckVgaAdapter (no label) */
#define OVERSCAN_TABLE       0x2A63u   /* 17-byte attribute/overscan table for INT 10h 1002h (no label) */
#define VGA_FAIL_TEXT        0x2A3Au   /* '$' string printed on the failed probe (no label) */
#define CS_VRAM_CLEAR_AT     0x26FAu   /* CS words zeroed on the failed probe (no label) */
#define CHECK_VGA_HALT       1         /* failed probe: platform HaltWithMessage (not translated) */

void SetCpuWriteMode(void)
{
    port_out16(0x3CE, 0x0005);                 /* GC 05h = 00h: write mode 0 */
    port_out16(0x3CE, 0x0003);                 /* GC 03h = 00h: replace */
}

void SetLatchCopyMode(void)
{
    port_out16(0x3CE, 0x0105);                 /* GC 05h = 01h: latch copy */
    port_out16(0x3C4, 0x0F02);                 /* map mask 0Fh */
}

/* WaitVerticalRetrace (cs:44CD): bit 3 of 3DAh (or bit 0 of 3BAh when OptionVByte is 0). */
void WaitVerticalRetrace(void)
{
    if (CB(CS_OptionVByte) != 0) {
        while (port_in8(0x3DA) & 0x08) { }     /* wait for the end of a retrace */
        while (!(port_in8(0x3DA) & 0x08)) { }  /* then for its start */
    } else {
        while (port_in8(0x3BA) & 0x01) { }
        while (!(port_in8(0x3BA) & 0x01)) { }
    }
}

/* FlipDisplayPage (cs:4509): CRTC start high byte from DrawPage; the pages swap. */
void FlipDisplayPage(void)
{
    uint16_t bx = DW(DS_DrawPage);
    port_out16(0x3D4, (uint16_t)((bx >> 8) << 8 | 0x0C));     /* CRTC 0Ch = high byte */
    uint16_t old = DW(DS_DisplayPage);
    SET_DW(DS_DisplayPage, bx);
    SET_DW(DS_DrawPage, old);
}

/* PresentFrame (cs:44FB): the byte_6BE7 test has identical branches (no effect). */
void PresentFrame(void)
{
    FlipDisplayPage();
    WaitSoundTicks();
}

/* ProbeVideoMemory (cs:443F): CF = 0 when both probes of A000:2000h read back. */
int ProbeVideoMemory(void)
{
    bios_set_mode(0x0D);
    SET_ES(0xA000);                            /* ES = A000h on both exits */
    port_out16(0x3C4, 0x0102);
    port_out16(0x3CE, 0x0005);
    port_out16(0x3CE, 0x0004);
    vga_write(0x2000, 0x01);
    if (vga_read(0x2000) != 0x01) return 1;
    port_out16(0x3CE, 0x1103);                 /* rotate 1, function OR */
    vga_write(0x2000, 0xA0);
    if (vga_read(0x2000) != 0x51) return 1;
    return 0;
}

/* CheckVgaAdapter (cs:43D2): probes the adapter once (OptionVByte FFh); then SetLevelVideoMode
 * runs in every case. Returns CHECK_VGA_HALT when the probe fails (the original prints the
 * message and jumps to HaltWithMessage, a platform path outside this translation). */
int CheckVgaAdapter(void)
{
    if (CB(CS_OptionVByte) != 0xFF) { SetLevelVideoMode(); return 0; }
    bios_set_mode(0x0D);
    port_out8(0x3C8, 0x40);
    for (uint16_t k = 0; k < 0x1E; k++) port_out8(0x3C9, (uint8_t)(DB(PALETTE_PROBE_TABLE + k) & 0x3F));
    port_out8(0x3C7, 0x40);
    int ok = 1;
    for (uint16_t k = 0; k < 0x1E; k++) {
        uint8_t want = (uint8_t)(DB(PALETTE_PROBE_TABLE + k) & 0x3F);
        if (port_in8(0x3C9) != want) ok = 0;   /* loop runs to the end: the original jumps at the first miss */
        if (!ok) break;
    }
    if (ok && !ProbeVideoMemory()) {
        SET_CB(CS_OptionVByte, 5);
        return 0;
    }
    SET_DW(DS_NextFreeSegment, 0);
    SET_ES(CODE_SEG);                          /* push cs / pop es before the clear; ES stays CS at the halt */
    for (uint16_t k = 0; k < 0x3E8; k++) wr16(CODE_SEG, (uint16_t)(CS_VRAM_CLEAR_AT + 2 * k), 0);
    bios_set_mode(0x03);
    dos_print_dollar(DATA_SEG, VGA_FAIL_TEXT);
    return CHECK_VGA_HALT;
}

/* SetLevelVideoMode (cs:447D): mode 0Dh, CRTC 11h = 24h, pages 0 / 2000h, the overscan table
 * and the level's 16-entry DAC block. */
void SetLevelVideoMode(void)
{
    bios_set_mode(0x0D);
    port_out16(0x3D4, 0x2411);
    SET_DW(DS_DisplayPage, 0);
    SET_DW(DS_DrawPage, 0x2000);
    bios_set_palette(DATA_SEG, OVERSCAN_TABLE);
    uint16_t dx = DW((uint16_t)(DS_LevelPaletteTable + 2 * DB(DS_CurrentLevel)));
    bios_set_dac_block(0, 0x10, DATA_SEG, dx);
}

/* ---- level tile set and cache (cs:4316, cs:4389, cs:4232, cs:42AF) ------------------- */

#define TILE_INDEX_TABLE  0x25CEu       /* 256 words: the level's first 512 bytes (no label) */

/* LoadLevelTileSet (cs:4316): MapRows, NextFreeSegment, TileIndexTable, and the tile cache
 * slots of the entries below 100h. Returns SI = the variant offset after the used tile data. */
uint16_t LoadLevelTileSet(void)
{
    uint16_t lseg = DW(DS_LevelSegment);
    uint8_t rows = DB((uint16_t)(DS_LevelMapRowsTable + DB(DS_CurrentLevel)));
    SET_DB(DS_MapRows, rows);
    /* the variant block follows the map: DS = LevelSegment + MapRows*16 for the rest of the
     * routine (and after it: LoadLevel copies the state block from there) */
    uint16_t var = (uint16_t)(lseg + ((uint16_t)rows << 4));   /* bx = rows << 8, shr 4 */
    SET_DW(DS_NextFreeSegment, var);
    for (uint16_t k = 0; k < 0x100; k++)                       /* rep movsw: variant:0 -> TileIndexTable */
        SET_DW((uint16_t)(TILE_INDEX_TABLE + 2 * k), rd16(var, (uint16_t)(2 * k)));
    SET_ES(0xA000);                                            /* ES = A000h to the return */
    port_out8(0x3C4, 0x02);
    uint16_t bp = 0x200;
    uint16_t di = 0x5E80;
    for (uint16_t bx = 0; bx < 0x200; bx = (uint16_t)(bx + 2)) {
        uint16_t si = rd16(var, bx);                           /* [bx] through DS = variant block */
        if (si < 0x100) {
            bp = (uint16_t)(bp + 0x80);
            si = (uint16_t)((si << 7) + 0x200);
            uint8_t al = 1;
            do {
                port_out8(0x3C5, al);
                for (int w = 0; w < 0x10; w++) {                /* rep movsw: LevelSeg:si -> A000:di */
                    vga_write(di, rd8(var, si));
                    vga_write((uint16_t)(di + 1), rd8(var, (uint16_t)(si + 1)));
                    si = (uint16_t)(si + 2); di = (uint16_t)(di + 2);
                }
                di = (uint16_t)(di - 0x20);
                al = (uint8_t)(al << 1);
            } while (al < 0x10);
        }
        di = (uint16_t)(di + 0x20);
    }
    return bp;
}

/* UploadSpriteTiles (cs:4389): TileIndexTable words >= 100h take the sprite-sheet graphic
 * (w - 100h) * 8 paragraphs past SpriteSheetSegment, four planes, into the cache slot. */
void UploadSpriteTiles(void)
{
    uint16_t di = 0x5E80;
    SET_ES(0xA000);                                         /* ES = A000h to the return */
    port_out8(0x3C4, 0x02);
    for (uint16_t bx = TILE_INDEX_TABLE; bx < 0x27CE; bx = (uint16_t)(bx + 2)) {
        uint16_t w = DW(bx);
        if (w >= 0x100) {
            uint16_t sseg = (uint16_t)(((uint16_t)(w - 0x100) << 3) + DW(DS_SpriteSheetSegment));
            uint16_t si = 0;
            uint8_t al = 1;
            do {
                port_out8(0x3C5, al);
                for (int k = 0; k < 0x10; k++) {                /* rep movsw from sseg:si */
                    vga_write(di, rd8(sseg, si));
                    vga_write((uint16_t)(di + 1), rd8(sseg, (uint16_t)(si + 1)));
                    si = (uint16_t)(si + 2); di = (uint16_t)(di + 2);
                }
                di = (uint16_t)(di - 0x20);
                al = (uint8_t)(al << 1);
            } while (al < 0x10);
        }
        di = (uint16_t)(di + 0x20);
    }
}

/* ClassifyTileCache (cs:4232): GC colour compare over each cache slot. TileClassTable: 0
 * opaque, 1 fully transparent, else a masked class numbered from MixedTileCount; the mask
 * is copied to 2DF8h + 32*(class-2) (the copy is undone for classes 0 and 1). */
void ClassifyTileCache(void)
{
    port_out16(0x3CE, 0x0F07);                 /* colour don't care: all planes */
    port_out16(0x3CE, 0x0002);                 /* colour compare: colour 0 */
    port_out16(0x3CE, 0x0805);                 /* read mode 1 (colour compare) */
    uint16_t si = 0x5E80, di = TILE_MASK_TABLE;
    SET_DB(DS_MixedTileCount, 1);
    for (uint16_t bx = 0; bx != 0x100; bx = (uint16_t)(bx + 1)) {
        uint8_t dl = 0xFF, dh = 0;
        for (int k = 0; k < 0x20; k++) {
            uint8_t al = vga_read(si);
            si = (uint16_t)(si + 1);
            wr8(DATA_SEG, di, al);            /* the mask copy (DS = DATA) */
            di = (uint16_t)(di + 1);
            dl &= al;
            dh |= al;
        }
        if (dh == 0) {
            SET_DB((uint16_t)(DS_TileClassTable + bx), dh);
            di = (uint16_t)(di - 0x20);
        } else if (dl == 0xFF) {
            SET_DB((uint16_t)(DS_TileClassTable + bx), 1);
            SET_DB(DS_GridDirtyFlag, 1);
            di = (uint16_t)(di - 0x20);
        } else {
            SET_DB(DS_MixedTileCount, (uint8_t)(DB(DS_MixedTileCount) + 1));   /* inc byte ptr */
            SET_DB((uint16_t)(DS_TileClassTable + bx), DB(DS_MixedTileCount));
            SET_DB(DS_GridDirtyFlag, 1);
        }
    }
    SET_DW(DS_PrevCameraCol, 0xAA55);
}

/* BuildAnimatedTileTables (cs:42AF): phase tables at 6688h, 6788h and 6888h. An animated triple
 * (t, t+1, t+2) starts at a tile with TileFlagsTable bit 80h; the loop then steps 3 tiles.
 * The loop ends when BX reaches 100h (the original tests BH after the increment). */
void BuildAnimatedTileTables(void)
{
    uint8_t dl = 0;
    uint16_t bx = 0;
    do {
        if (!(DB((uint16_t)(DS_TileFlagsTable + bx)) & 0x80)) {
            SET_DB((uint16_t)(DS_byte_6688 + bx), (uint8_t)bx);
            SET_DB((uint16_t)(DS_byte_6788 + bx), (uint8_t)bx);
            SET_DB((uint16_t)(DS_byte_6888 + bx), (uint8_t)bx);
            SET_DB((uint16_t)(DS_AnimTileFlags + bx), 0);
        } else {
            dl = 1;
            uint8_t t = (uint8_t)bx, t1 = (uint8_t)(t + 1), t2 = (uint8_t)(t + 2);
            SET_DB((uint16_t)(DS_byte_6688 + bx), t);
            SET_DB((uint16_t)(DS_byte_678A + bx), t);
            SET_DB((uint16_t)(DS_byte_6889 + bx), t);
            SET_DB((uint16_t)(DS_byte_6689 + bx), t1);
            SET_DB((uint16_t)(DS_byte_6788 + bx), t1);
            SET_DB((uint16_t)(DS_byte_688A + bx), t1);
            SET_DB((uint16_t)(DS_byte_668A + bx), t2);
            SET_DB((uint16_t)(DS_byte_6789 + bx), t2);
            SET_DB((uint16_t)(DS_byte_6888 + bx), t2);
            SET_DB((uint16_t)(DS_AnimTileFlags + bx), 1);
            SET_DW((uint16_t)(DS_word_6989 + bx), 0x101);
            bx = (uint16_t)(bx + 2);
        }
        bx = (uint16_t)(bx + 1);
    } while ((bx >> 8) == 0);
    SET_DB(DS_AnimVisibleFlag, dl);
}

/* ---- ring to page copy and shake (cs:3A27) ----------------------------------------- */

/* CopyRingWindowToPage: DrawPage = the ring window, scrolled by FineScroll and RowRing and
 * shifted down by ShakeRows. The first ring part runs from RingTileOffset, the rest from 3F40h.
 * The 0 count of the first part is swapped with the second (loc_3A89..3AA3). */
void CopyRingWindowToPage(void)
{
    uint16_t shake = DW(DS_ShakeRows);
    uint16_t shake_bytes = (uint16_t)(0x28u * DB(DS_ShakeRows));     /* mul byte ptr ShakeRows */
    SET_CW(CS_ShakeOffsetBytes, shake_bytes);
    uint16_t si = DW(DS_RingTileOffset);
    uint16_t di = (uint16_t)(DW(DS_DrawPage) + shake_bytes);
    uint8_t dh = DB(DS_ColumnRing);
    uint8_t dl = (uint8_t)(0x14 - dh);
    dl = (uint8_t)(dl << 1);
    dh = (uint8_t)(dh << 1);
    SetLatchCopyMode();
    uint16_t bp = (uint16_t)(0xC0 - (uint16_t)(int16_t)(int8_t)DB(DS_FineScroll));
    bp = (uint16_t)(bp - (uint16_t)(DW(DS_RowRing) << 4));
    uint16_t bx;
    if (bp < 0xB0) {
        bx = (uint16_t)(0xB0 - bp);
        bx = (uint16_t)(bx - shake);
        if ((int16_t)bx < 0) { bp = (uint16_t)(bp + bx); bx = 0; }
    } else {
        bp = (uint16_t)(0xB0 - shake);
        bx = 0;
    }
    si = (uint16_t)(si + 0x28u * DB(DS_FineScroll));
    if (bp == 0) { uint16_t t = bp; bp = bx; bx = t; }     /* or bp, bp; jne; xchg bx, bp */
    do {                                                   /* loc_3AA3: first part rows */
        for (int k = 0; k < dl; k++) { vga_movsb(si, di); si++; di++; }
        si = (uint16_t)(si - 0x28);
        for (int k = 0; k < dh; k++) { vga_movsb(si, di); si++; di++; }
        si = (uint16_t)(si + 0x28);
        bp = (uint16_t)(bp - 1);
    } while (bp != 0);
    if (bx != 0) {                                         /* second part from the ring top */
        si = (uint16_t)(0x3F40 + dh);
        do {                                               /* loc_3ABD */
            for (int k = 0; k < dl; k++) { vga_movsb(si, di); si++; di++; }
            si = (uint16_t)(si - 0x28);
            for (int k = 0; k < dh; k++) { vga_movsb(si, di); si++; di++; }
            si = (uint16_t)(si + 0x28);
            bx = (uint16_t)(bx - 1);
        } while (bx != 0);
    }
    uint16_t words = (uint16_t)(CW(CS_ShakeOffsetBytes) >> 1);
    if (words != 0) {                                      /* rep stosw from DrawPage */
        uint16_t p = DW(DS_DrawPage);
        SetCpuWriteMode();
        port_out16(0x3C4, 0x0F02);
        for (uint16_t k = 0; k < words; k++) {
            vga_write(p, 0);
            vga_write((uint16_t)(p + 1), 0);
            p = (uint16_t)(p + 2);
        }
    }
}

/* ---- level fix-ups (cs:3EAD..40BB) -------------------------------------------------- */

/* ApplySecretTilePatches (cs:3EAD): 80 records of 5 bytes at 8C8Dh; a record with bank
 * offset != FFFFh swaps its new tile into LevelSegment:offset and saves the old one. */
void ApplySecretTilePatches(void)
{
    uint16_t lseg = DW(DS_LevelSegment);
    uint16_t si = 0x8C8D, dx = 0;
    for (int cx = 0x50; cx != 0; cx--, si = (uint16_t)(si + 5)) {
        uint16_t bx = DW((uint16_t)(si + 3));
        if (bx == 0xFFFF) continue;
        dx++;
        uint8_t al = DB(si);
        uint8_t old = rd8(lseg, bx);
        wr8(lseg, bx, al);
        SET_DB((uint16_t)(si + 1), old);
    }
    SET_DW(DS_CollectTotalMain, (uint16_t)(DW(DS_CollectTotalMain) + dx));
}

/* RebaseEntitySpriteRefs (cs:4182): entity X values move between the sprite bank windows
 * (see the routine's contract); then SpriteBankA = 35h and SpriteBankB = 138h. */
void RebaseEntitySpriteRefs(void)
{
    uint16_t dx = DW(DS_SpriteBankA), bx = DW(DS_SpriteBankB);
    if (dx != 0xFFFF) {
        uint16_t si = 0x8489;
        while (DB(si) <= 0x32) {                          /* ja: unsigned stop */
            uint16_t ax = DW((uint16_t)(si + 2));
            if (ax != 0xFFFF) {
                if (ax >= bx) { ax = (uint16_t)(ax - bx + 0x138); SET_DW((uint16_t)(si + 2), ax); }
                else if (ax >= dx) { ax = (uint16_t)(ax - dx + 0x35); SET_DW((uint16_t)(si + 2), ax); }
            }
            si = (uint16_t)(si + (uint16_t)(int16_t)(int8_t)DB(si));   /* cbw; add si, ax */
        }
    }
    SET_DW(DS_SpriteBankA, 0x35);
    SET_DW(DS_SpriteBankB, 0x138);
}

/* ScrollEffectSources (cs:4157 and cs:416F): effect sources (70 records, stride 7, from
 * 8F1Dh) and terrain entities (16 records, stride 0Fh, from 9107h) move by the bank rebase. */
void ScrollEffectSources(void)
{
    uint16_t dx = DW(DS_SpriteBankA);
    uint16_t si = 0x8F1D;
    for (int cx = 0x46; cx != 0; cx--, si = (uint16_t)(si + 7)) {
        if (DW((uint16_t)(si + 4)) != 0xFFFF) {
            SET_DW((uint16_t)(si + 4), (uint16_t)(DW((uint16_t)(si + 4)) - dx + 0x35));
        }
    }
    si = 0x9107;
    for (int cx = 0x10; cx != 0; cx--, si = (uint16_t)(si + 0x0F)) {
        if (DW((uint16_t)(si + 4)) != 0xFFFF) {
            SET_DW((uint16_t)(si + 4), (uint16_t)(DW((uint16_t)(si + 4)) - dx + 0x35));
        }
    }
}

/* SavePristineTriggerRows (cs:41CA): the original cells of the proximity triggers (15 records
 * from 83F3h, stride 0Ah) go to the pristine bank at NextFreeSegment. The +6 word receives
 * di - rows (as the original computes it; the value is kept as is). */
void SavePristineTriggerRows(void)
{
    uint16_t bank = DW(DS_NextFreeSegment);
    uint16_t di = 0;
    SET_ES(bank);                                        /* ES = NextFreeSegment, reloaded per record */
    wr16(bank, 0, 0xFFFF);
    uint16_t bx = 0x83F3;
    for (int cx = 0x0F; cx != 0; cx--, bx = (uint16_t)(bx + 0x0A)) {
        if (DW(bx) == 0xFFFF) continue;
        uint16_t si = DW(bx);
        uint8_t ah = DB((uint16_t)(bx + 8));
        si = (uint16_t)(si - ((uint16_t)ah << 8));       /* al = 0 */
        uint8_t dl = (uint8_t)(DB((uint16_t)(bx + 3)) + DB((uint16_t)(bx + 8)));
        uint8_t dh = DB((uint16_t)(bx + 2));
        bank = DW(DS_NextFreeSegment);
        SET_ES(bank);
        uint16_t lseg = DW(DS_LevelSegment);
        wr16(bank, di, si);
        wr16(bank, (uint16_t)(di + 2), (uint16_t)(dh << 8 | dl));
        di = (uint16_t)(di + 4);
        uint8_t rows = dl;
        do {                                             /* outer: dl rows */
            uint8_t al = dh;
            do {                                         /* inner: dh bytes, movsb */
                wr8(bank, di, rd8(lseg, si));
                si = (uint16_t)(si + 1); di = (uint16_t)(di + 1);
                al = (uint8_t)(al - 1);
            } while (al != 0);
            si = (uint16_t)(si - dh + 0x100);
            rows = (uint8_t)(rows - 1);
        } while (rows != 0);
        SET_DW((uint16_t)(bx + 6), (uint16_t)(di - dh));
        wr16(bank, di, 0xFFFF);
    }
}

/* BuildDecorList (cs:40BD): the decor pointers (effect sources with word +4 & 1FFFh in
 * 11Bh..12Ah) at 6A88h, bubble-sorted by each target's word +0, then word +4 of each group of
 * four is 11Bh + a nibble of ComputePasswordCode(CurrentLevel [+ 0Ah with byte B198]). Does
 * nothing when the count is 0 or not a multiple of four. */
void BuildDecorList(void)
{
    uint16_t si = 0x8F1D, di = 0x6A88, bp = 0;
    for (int cx = 0x46; cx != 0; cx--, si = (uint16_t)(si + 7)) {
        uint16_t ax = (uint16_t)((DW((uint16_t)(si + 4)) & 0x1FFF) - 0x11B);
        if (ax <= 0x0F) {
            SET_DW(di, si);
            di = (uint16_t)(di + 2);
            bp++;
        }
    }
    if (bp == 0 || (bp & 3) != 0) return;
    uint16_t dx;
    do {                                                 /* loc_40ED: bubble passes */
        dx = 0;
        uint16_t p = 0x6A88;
        /* mov cx, bp / jmp loc_4109: the pass enters at its LOOP, so it compares the bp - 1
         * adjacent pairs of the list (never the stale word after its end) */
        for (uint16_t cx = (uint16_t)(bp - 1); cx != 0; cx--, p = (uint16_t)(p + 2)) {   /* loc_40F6 */
            uint16_t a = DW(p);
            uint16_t av = DW(a);
            uint16_t b = DW((uint16_t)(p + 2));
            if (DW(b) < av) {                            /* jae skips the swap */
                SET_DW(p, b);
                SET_DW((uint16_t)(p + 2), a);
                dx++;
            }
        }
    } while (dx != 0);
    uint8_t al = DB(DS_CurrentLevel);
    if (DB(DS_byte_B198) != 0) al = (uint8_t)(al + 0x0A);
    uint16_t code = ComputePasswordCode((uint16_t)(int16_t)(int8_t)al);
    si = 0x6A88;
    while (bp != 0) {                                    /* loc_4122: one group of four */
        uint16_t dx2 = code;
        for (int k = 0; k < 4; k++) {
            uint16_t target = DW(si);
            si = (uint16_t)(si + 2);
            uint16_t nib = (uint16_t)((dx2 >> 12) & 0xF);
            SET_DW((uint16_t)(target + 4), (uint16_t)(0x11B + nib));
            dx2 = (uint16_t)(dx2 << 4);
        }
        bp = (uint16_t)(bp - 4);
    }
}

/* ---- LoadLevel (cs:3ED6) -------------------------------------------------------------- */

#define LEVEL_NAME_AT     0x2D8Bu   /* "LEVEL?.SQZ" template in DATA (no label) */
#define BACK_NAME_AT      0x2DA6u   /* "BACK?.SQZ" template in DATA (no label) */
#define LEVEL_VRAM_STASH  0x9DC0u   /* VRAM offset of the stash (no label) */
#define TILE_FLAG_CLEAR_AT 0x7E5Eu  /* 13A5h bytes set to FFh before the load (no label) */
#define RENDER_SLOT_1      0x4F1Cu  /* first render slot entry (+4 is the flag word); no label */

/* Copies n words (or bytes) from lseg:*si to DATA:*di (movsw / movsb with DS = src, ES = DATA). */
static void copy_words(uint16_t src_seg, uint16_t *si, uint16_t *di, uint16_t n)
{
    for (uint16_t k = 0; k < n; k++) {
        uint8_t lo = rd8(src_seg, *si), hi = rd8(src_seg, (uint16_t)(*si + 1));
        wr8(DATA_SEG, *di, lo);
        wr8(DATA_SEG, (uint16_t)(*di + 1), hi);
        *si = (uint16_t)(*si + 2); *di = (uint16_t)(*di + 2);
    }
}

static void copy_bytes(uint16_t src_seg, uint16_t *si, uint16_t *di, uint16_t n)
{
    for (uint16_t k = 0; k < n; k++) {
        wr8(DATA_SEG, *di, rd8(src_seg, *si));
        *si = (uint16_t)(*si + 1); *di = (uint16_t)(*di + 1);
    }
}

/* LoadLevel: AX on entry (AL = level 0.., AH is kept: see the render-slot fill). Returns 0 or the
 * SQZ error code of the failed load (the original enters its fatal path there). */
int LoadLevel(uint16_t ax)
{
    uint8_t level = (uint8_t)ax;
    SET_DB(DS_CurrentLevel, level);
    if (level > 0x0A) {
        SET_DW(DS_CollectTotalDecor, (uint16_t)(DW(DS_CollectTotalDecor) >> 1));
        SET_DW(DS_CollectTotalMain, (uint16_t)(DW(DS_CollectTotalMain) >> 1));
    } else {
        SET_DW(DS_CollectTotalDecor, 0);
        SET_DW(DS_CollectTotalMain, 0);
        SET_DW(DS_word_2A7A, 0);
        SET_DW(DS_word_2A76, 0);
    }
    SET_DB(DS_BackFileChar, DB((uint16_t)(DS_byte_2D96 + level)));
    uint8_t c = (uint8_t)(level + 0x31);
    if (c > 0x39) c = (uint8_t)(c + 7);                  /* '9' then 'A' */
    SET_DB(DS_LevelFileChar, c);
    for (uint16_t k = 0; k < 0x13A5; k++) SET_DB((uint16_t)(TILE_FLAG_CLEAR_AT + k), 0xFF);
    /* stosw of AX: AL = FFh from the fill above, AH = the caller's AH (the original does not
     * load AX here), so the slot words are 00FFh when the caller left AH zero. */
    for (uint16_t k = 0; k < 0x40B; k++) SET_DW((uint16_t)(RENDER_SLOT_1 + 2 * k), (uint16_t)((ax & 0xFF00) | 0xFF));
    SetCpuWriteMode();
    uint16_t lseg = 0;
    int e = LoadSqzFile(LEVEL_NAME_AT, &lseg);
    if (e) return e;
    SET_DW(DS_LevelSegment, lseg);
    uint16_t si = LoadLevelTileSet();
    uint16_t var = DW(DS_NextFreeSegment);      /* DS after LoadLevelTileSet: the variant block */

    /* variant block -> the 7E5Eh.. state block (DS = variant block, ES = DATA) */
    uint16_t di = 0x7E5E;
    copy_words(var, &si, &di, 0x180);
    copy_words(var, &si, &di, 4);
    copy_bytes(var, &si, &di, 1);
    copy_words(var, &si, &di, 0x100);
    copy_words(var, &si, &di, 0x46);
    copy_words(var, &si, &di, 0x4B);
    copy_words(var, &si, &di, 0x400);
    copy_words(var, &si, &di, 2);
    copy_bytes(var, &si, &di, 0x190);
    copy_bytes(var, &si, &di, 0x100);
    copy_bytes(var, &si, &di, 0x1EA);
    copy_bytes(var, &si, &di, 0xF0);
    copy_words(var, &si, &di, 2);
    copy_words(var, &si, &di, 4);

    /* stash the level segment into VRAM 9DC0h: four map-mask passes, one source pointer */
    uint16_t src = 0;
    SetCpuWriteMode();
    for (uint16_t pl = 1; pl != 0x10; pl = (uint16_t)(pl << 1)) {
        port_out16(0x3C4, (uint16_t)(pl << 8 | 0x02));
        uint16_t d = LEVEL_VRAM_STASH;
        for (uint16_t k = 0; k < 0x3120; k++) {
            vga_write(d, rd8(lseg, src));
            vga_write((uint16_t)(d + 1), rd8(lseg, (uint16_t)(src + 1)));
            src = (uint16_t)(src + 2); d = (uint16_t)(d + 2);
        }
    }
    uint16_t nfs = DW(DS_NextFreeSegment);
    SET_DW(DS_NextFreeSegment, lseg);
    /* PageInResource (system.c) returns only its status. Slot 7 is pinned (ResourceFlags 08h),
     * so the original's AX on return is LoadSqzFile's output segment: the free pointer, which
     * is LevelSegment here. An unpinned slot would leave AX past its 105Bh-paragraph copy. */
    e = PageInResource(7);
    if (e) return e;
    uint16_t sprites = (DB((uint16_t)(DS_ResourceFlags + 7)) & 0x08) ? lseg
                                                                  : (uint16_t)(lseg + 0x105B);
    SET_DW(DS_SpriteSheetSegment, sprites);
    SET_DW(DS_NextFreeSegment, sprites);
    UploadSpriteTiles();
    SET_DW(DS_NextFreeSegment, nfs);

    /* copy back: read map 0..3, all into LevelSegment from offset 0 (the offset is not reset
     * between the passes, so the four blocks follow each other, wrapping at 64 KB) */
    uint16_t dst = 0;
    for (uint16_t pl = 0; pl <= 3; pl++) {
        port_out16(0x3CE, (uint16_t)(pl << 8 | 0x04));
        uint16_t s = LEVEL_VRAM_STASH;
        for (uint16_t k = 0; k < 0x3120; k++) {
            uint8_t lo = vga_read(s), hi = vga_read((uint16_t)(s + 1));
            wr8(lseg, dst, lo);
            wr8(lseg, (uint16_t)(dst + 1), hi);
            s = (uint16_t)(s + 2); dst = (uint16_t)(dst + 2);
        }
    }
    SetCpuWriteMode();

    /* backdrop: four planes of 8000 bytes into VRAM 7E80h */
    uint16_t back = 0;
    /* push word ptr ds:[2875h] reads A000:2875h (DS is A000 here); the pop at the end writes
     * it to ds:[2875h] with DS = the BACK segment, so NextFreeSegment keeps the BACK allocation. */
    uint16_t popped = (uint16_t)(vga_read(0x2875) | vga_read(0x2876) << 8);
    e = LoadSqzFile(BACK_NAME_AT, &back);
    if (e) return e;
    uint16_t bsrc = 0;
    for (uint16_t pl = 1; pl != 0x10; pl = (uint16_t)(pl << 1)) {
        port_out16(0x3C4, (uint16_t)(pl << 8 | 0x02));
        uint16_t d = 0x7E80;
        for (uint16_t k = 0; k < 0xFA0; k++) {
            vga_write(d, rd8(back, bsrc));
            vga_write((uint16_t)(d + 1), rd8(back, (uint16_t)(bsrc + 1)));
            bsrc = (uint16_t)(bsrc + 2); d = (uint16_t)(d + 2);
        }
    }
    wr16(back, 0x2875, popped);

    BuildAnimatedTileTables();
    ClassifyTileCache();
    uint8_t lv = DB(DS_CurrentLevel);
    SET_DW(DS_ScriptPointer, DW((uint16_t)(DS_LevelScriptTable + 2 * lv)));
    SET_DW(DS_ScriptFrameCounter, 0);
    SET_DW(DS_CheckpointX, DW(DS_LevelStartX));
    SET_DW(DS_CheckpointY, DW(DS_LevelStartY));
    ScrollEffectSources();
    RebaseEntitySpriteRefs();
    ApplySecretTilePatches();
    BuildDecorList();
        for (uint16_t k = 0; k < 0x10A5; k++)                 /* rep movsb: 815Eh -> 9203h */
        wr8(DATA_SEG, (uint16_t)(0x9203 + k), rd8(DATA_SEG, (uint16_t)(0x815E + k)));
    SavePristineTriggerRows();

    uint16_t decor = 0;
    uint16_t p = 0x8F1D;
    for (int cx = 0x46; cx != 0; cx--, p = (uint16_t)(p + 7)) {
        uint16_t w = DW((uint16_t)(p + 4));
        if (w == 0xFFFF) continue;
        if (w < 0x6E) continue;
        if (w > 0xDB) continue;
        if (w >= 0x80) { decor++; continue; }
        if (w > 0x75) continue;
        decor++;
    }
    SET_DW(DS_CollectTotalDecor, (uint16_t)(DW(DS_CollectTotalDecor) + decor));
    uint8_t lv2 = DB(DS_CurrentLevel);
    if (lv2 < 0x0A && DB((uint16_t)(DS_WarpLevelTable + lv2)) != 0xFF) {
        SET_DW(DS_CollectTotalDecor, (uint16_t)(DW(DS_CollectTotalDecor) << 1));
        SET_DW(DS_CollectTotalMain, (uint16_t)(DW(DS_CollectTotalMain) << 1));
    }
    return 0;
}

/* unused_443E (cs:443E): a bare RET with no callers. */
void unused_443E(void) { }


/* ---- differential-test adapters (register contract -> C arguments) -------------------- */


EXPORT int c_EnableVideoOutput(Regs *r) { EnableVideoOutput(); r->ds = DATA_SEG; return 0; }
EXPORT int c_DisableVideoOutput(Regs *r) { DisableVideoOutput(); r->ds = DATA_SEG; return 0; }
EXPORT int c_RevealLevelColumns(Regs *r) { RevealLevelColumns(); r->ds = DATA_SEG; return 0; }
EXPORT int c_CopyColumnStrip(Regs *r) { CopyColumnStrip(r->di); return 0; }
EXPORT int c_ClearStaggeredBlocks(Regs *r) { ClearStaggeredBlocks(); r->ds = DATA_SEG; return 0; }
EXPORT int c_ClearBlock16x10(Regs *r) { ClearBlock16x10(r->di); return 0; }
EXPORT int c_ClearDisplayPage(Regs *r) { ClearDisplayPage(); r->ds = DATA_SEG; return 0; }
EXPORT int c_BlackoutSpan(Regs *r) { BlackoutSpan(r->bx, r->cx, r->dx); return 0; }
EXPORT int c_RunIrisClose(Regs *r) { RunIrisClose(); r->ds = DATA_SEG; return 0; }
EXPORT int c_ScrollViewUp(Regs *r) { set_cf(r, ScrollViewUp((uint8_t)r->dx)); r->ds = DATA_SEG; return 0; }
EXPORT int c_ScrollViewDown(Regs *r) { set_cf(r, ScrollViewDown((uint8_t)r->dx)); r->ds = DATA_SEG; return 0; }
EXPORT int c_ScrollViewLeft(Regs *r) { set_cf(r, ScrollViewLeft()); r->ds = DATA_SEG; return 0; }
EXPORT int c_ScrollViewRight(Regs *r) { set_cf(r, ScrollViewRight()); r->ds = DATA_SEG; return 0; }
EXPORT int c_RestoreTileBackdrop(Regs *r) { r->di = RestoreTileBackdrop(r->di); return 0; }
EXPORT int c_BlitTileCell(Regs *r) { r->di = BlitTileCell(r->ax, r->di); return 0; }
EXPORT int c_BlitTileToVram(Regs *r) { r->di = BlitTileToVram(r->ax, r->di); return 0; }
EXPORT int c_BlitTileToRing(Regs *r) { r->di = BlitTileToRing(r->ax, r->di); return 0; }
EXPORT int c_ComputeRingTileOffset(Regs *r) { ComputeRingTileOffset(r->ax); r->ds = DATA_SEG; return 0; }
EXPORT int c_DrawTileRow(Regs *r) { DrawTileRow(r->ax, r->di); r->ds = DATA_SEG; return 0; }
EXPORT int c_DrawTileColumn(Regs *r) { DrawTileColumn(r->ax, r->di); r->ds = DATA_SEG; return 0; }
EXPORT int c_RedrawOverlayTiles(Regs *r) { RedrawOverlayTiles(); r->ds = DATA_SEG; return 0; }
EXPORT int c_AnimateTiles(Regs *r) { AnimateTiles(); r->ds = DATA_SEG; return 0; }
EXPORT int c_DrawForegroundTile(Regs *r) { DrawForegroundTile(r->ax, r->bx); return 0; }
EXPORT int c_DrawForegroundOverSprites(Regs *r) { DrawForegroundOverSprites(); r->ds = DATA_SEG; return 0; }
EXPORT int c_NextWindRandom(Regs *r) { r->ax = (uint16_t)((r->ax & 0xFF00) | NextWindRandom()); r->ds = DATA_SEG; return 0; }
EXPORT int c_UpdateWindAndFlakes(Regs *r) { UpdateWindAndFlakes(); r->ds = DATA_SEG; return 0; }
EXPORT int c_CopyRingWindowToPage(Regs *r) { CopyRingWindowToPage(); r->ds = DATA_SEG; return 0; }
EXPORT int c_SetCpuWriteMode(Regs *r) { (void)r; SetCpuWriteMode(); return 0; }
EXPORT int c_SetLatchCopyMode(Regs *r) { (void)r; SetLatchCopyMode(); return 0; }
EXPORT int c_WaitVerticalRetrace(Regs *r) { (void)r; WaitVerticalRetrace(); return 0; }
EXPORT int c_FlipDisplayPage(Regs *r) { FlipDisplayPage(); r->ds = DATA_SEG; return 0; }
EXPORT int c_PresentFrame(Regs *r) { PresentFrame(); r->ds = DATA_SEG; return 0; }
EXPORT int c_ProbeVideoMemory(Regs *r) { set_cf(r, ProbeVideoMemory()); r->ds = DATA_SEG; return 0; }
EXPORT int c_CheckVgaAdapter(Regs *r) { int st = CheckVgaAdapter(); r->ds = DATA_SEG; return st; }
EXPORT int c_SetLevelVideoMode(Regs *r) { SetLevelVideoMode(); r->ds = DATA_SEG; return 0; }
EXPORT int c_LoadLevelTileSet(Regs *r) { r->si = LoadLevelTileSet(); r->ds = DATA_SEG; return 0; }
EXPORT int c_UploadSpriteTiles(Regs *r) { UploadSpriteTiles(); r->ds = DATA_SEG; return 0; }
EXPORT int c_ClassifyTileCache(Regs *r) { ClassifyTileCache(); r->ds = DATA_SEG; return 0; }
EXPORT int c_BuildAnimatedTileTables(Regs *r) { BuildAnimatedTileTables(); r->ds = DATA_SEG; return 0; }
EXPORT int c_ApplySecretTilePatches(Regs *r) { ApplySecretTilePatches(); r->ds = DATA_SEG; return 0; }
EXPORT int c_RebaseEntitySpriteRefs(Regs *r) { RebaseEntitySpriteRefs(); r->ds = DATA_SEG; return 0; }
EXPORT int c_ScrollEffectSources(Regs *r) { ScrollEffectSources(); r->ds = DATA_SEG; return 0; }
EXPORT int c_SavePristineTriggerRows(Regs *r) { SavePristineTriggerRows(); r->ds = DATA_SEG; return 0; }
EXPORT int c_BuildDecorList(Regs *r) { BuildDecorList(); r->ds = DATA_SEG; return 0; }
EXPORT int c_LoadLevel(Regs *r) { int st = LoadLevel(r->ax); r->ds = DATA_SEG; return st; }
EXPORT int c_unused_443E(Regs *r) { (void)r; unused_443E(); return 0; }
