/* Sprite pipeline: LoadSpriteSheet (src/MODULE2.ASM, cs:2DFA) and DrawSpriteObjects
 * (cs:26FA with DrawSpriteMirrored, DrawSpriteStraight and the blitter state in cs:26E0..26F7).
 *
 * The blitter is a line-by-line translation of the original passes. Its register pairs
 * (AX halves, CX counter/mask, DX row counter/carry mask) are kept as separate C values
 * with the same 8-bit wraps, and every VGA access is the same sequence of latch loads
 * (vga_read) and writes (vga_write) as the original's xchg/scasb/stosb, so the latches
 * and planes come out identical. Quirks kept: zero-count loops run 65536 times (loop with
 * CX = 0), row counters of 0 run 256 rows, the frame-309 path skips the camera, the
 * clipped mirror and straight paths clear even in flash mode (PlanePassMode 10h) and then
 * run four flash passes, the 8-bit clip sums, and the FrameDataOffsets normalisation that
 * subtracts 1000h only once per frame.
 */
#include "pre2.h"
#include "vga.h"
#include "sqz.h"
#include "sprites.h"

#define SPRITE_SQZ_NAME  0x2A2Eu   /* DATA offset of "SPRITES.SQZ" (anonymous string, MODULE1 DATA) */
#define BITREV_TABLE     0x2F34u   /* cs:2F34h: 256 bytes, byte i = reverse8(i); no label, read by XLAT */
#define SCREEN_ROW_BYTES 40u       /* the blitter's row stride constant (sub ax, 28h) */
#define FRAME_309_BX     0x026Au   /* frame 309 as a frame-table index (word, bx = frame * 2) */

#define LO(x) ((uint8_t)((x) & 0xFFu))
#define HI(x) ((uint8_t)((x) >> 8))

/* cbw: AL sign-extended to AX. */
static uint16_t cbw(uint8_t al) { return (uint16_t)(int16_t)(int8_t)al; }

/* XCHG with memory at A000h:off (a read that loads the latches, then the write); returns
 * the old byte. The scasb/stosb pairs of the original are the same read-then-write. */
static uint8_t vmem_xchg(uint16_t off, uint8_t v)
{
    uint8_t old = vga_read(off);
    vga_write(off, v);
    return old;
}

/* XLAT through cs:[BX] with BX = BITREV_TABLE. */
static uint8_t bitrev(uint8_t v) { return CB((uint16_t)(BITREV_TABLE + v)); }

/* SetCpuWriteMode (cs:452Bh, outside this region): GC 05h = 00h, GC 03h = 00h. */
static void set_cpu_write_mode(void)
{
    vga_out(0x3CE, 5); vga_out(0x3CF, 0);
    vga_out(0x3CE, 3); vga_out(0x3CF, 0);
}

static void seq_index(uint8_t index) { vga_out(0x3C4, index); }

/* The blit's register state at the entry of DrawSpriteMirrored / DrawSpriteStraight. */
typedef struct {
    uint16_t src;   /* DS: SpriteSrcSegment (frame data) */
    uint16_t si;    /* SI: source offset (advanced by the passes) */
    uint16_t di;    /* DI: destination offset in A000h (advanced by the passes) */
    uint8_t  ch;    /* CH: FFh << (X & 7); the complement is the carry mask (dh / bh) */
    uint8_t  xs;    /* X & 7: GC rotate count */
    uint8_t  vis;   /* SpriteVisibleBytes: bytes per visible row (cl) */
    uint8_t  rows;  /* SpriteVisibleRows (dl) */
} Blit;

/* BP = visible bytes - 40: the per-row step back to the next row's first byte. */
static uint16_t blit_bp(const Blit *b) { return (uint16_t)(cbw(b->vis) - SCREEN_ROW_BYTES); }

/* dec byte ptr cs:RowCounter; returns the new value (the passes loop on jne). */
static uint8_t row_dec(void)
{
    uint8_t v = (uint8_t)(CB(CS_RowCounter) - 1);
    SET_CB(CS_RowCounter, v);
    return v;
}

/* ---- DrawSpriteMirrored: frame read backwards (DF = 1), bytes bit-reversed ----------
 * Pointer convention: the mirrored rows are read from their last byte, so after each row
 * SI sits at the last byte of the next row (the next pass starts from there). */

/* loc_2943: clear pass, aligned (ch = FFh, no carry). NOT(reverse(src)) ANDed into A000h. */
static void mirror_clear_aligned(Blit *b)
{
    uint16_t bp = blit_bp(b);
    do {
        uint16_t cx = b->vis;               /* ch = 0 here: CX = vis */
        do {
            uint8_t v = bitrev(rd8(b->src, b->si));
            b->si--;
            (void)vmem_xchg(b->di, (uint8_t)~v);
            b->di++;
        } while (--cx != 0);
        b->si = (uint16_t)(b->si + 2u * cbw(b->vis));
        b->di = (uint16_t)(b->di - bp);
    } while (row_dec() != 0);
}

/* loc_2963 .. loc_29D4: colour passes (OR, GC 03h = 10h) for the aligned case. */
static void mirror_or_aligned(Blit *b)
{
    uint16_t bp = blit_bp(b);
    vga_out(0x3CE, 3); vga_out(0x3CF, 0x10);
    seq_index(2);
    SET_CB(CS_EdgeByteInvert, (uint8_t)~CB(CS_EdgeByteInvert));
    uint8_t al = CB(CS_PlanePassMode);
    if (al == 0) return;
    if (al == 0x10) { al = 0x0F; b->si = (uint16_t)(b->si - CW(CS_SpriteSourceSkip)); }
    for (;;) {                                  /* loc_2989 */
        SET_CB(CS_CurrentPlaneMask, al);
        vga_out(0x3C5, al);
        SET_CB(CS_RowCounter, CB(CS_SpriteVisibleRows));
        b->di = CW(CS_SpriteDestOffset);
        b->si = (uint16_t)(b->si + CW(CS_SpriteSourceSkip));
        do {                                    /* loc_29AC */
            uint16_t cx = b->vis;
            do {
                uint8_t v = bitrev(rd8(b->src, b->si));
                b->si--;
                (void)vmem_xchg(b->di, v);
                b->di++;
            } while (--cx != 0);
            b->si = (uint16_t)(b->si + 2u * cbw(b->vis));
            b->di = (uint16_t)(b->di - bp);
        } while (row_dec() != 0);
        SET_CB(CS_CurrentPlaneMask, (uint8_t)(CB(CS_CurrentPlaneMask) << 1));
        al = CB(CS_CurrentPlaneMask);
        if (al >= 0x10) break;
    }
}

/* loc_29DF: clear pass with a shifted byte. The carry (AH) is zero at each row start
 * (the row's CBW), DL keeps the byte, and the last carry is written past the row. */
static void mirror_clear_partial(Blit *b)
{
    uint16_t bp = blit_bp(b);
    uint8_t rmask = (uint8_t)~b->ch;
    do {
        uint8_t cl = b->vis, ah = 0, dl;
        do {
            uint8_t v = bitrev(rd8(b->src, b->si));
            b->si--;
            dl = v;
            (void)vmem_xchg(b->di, (uint8_t)~((v & b->ch) | ah));
            b->di++;
            dl &= rmask;
            ah = dl;
        } while (--cl != 0);
        ah = (uint8_t)~ah;
        (void)vmem_xchg(b->di, ah);
        b->si = (uint16_t)(b->si + 2u * cbw(b->vis));
        b->di = (uint16_t)(b->di - bp);
    } while (row_dec() != 0);
}

/* loc_2A10 .. loc_2A9E: colour passes for the shifted case (OR, carry written at row end). */
static void mirror_or_partial(Blit *b)
{
    uint16_t bp = blit_bp(b);
    uint8_t rmask = (uint8_t)~b->ch;
    vga_out(0x3CE, 3); vga_out(0x3CF, (uint8_t)(b->xs | 0x10));
    seq_index(2);
    SET_CB(CS_EdgeByteInvert, (uint8_t)~CB(CS_EdgeByteInvert));
    uint8_t al = CB(CS_PlanePassMode);
    if (al == 0) return;
    if (al == 0x10) { al = 0x0F; b->si = (uint16_t)(b->si - CW(CS_SpriteSourceSkip)); }
    for (;;) {                                  /* loc_2A42 */
        SET_CB(CS_CurrentPlaneMask, al);
        vga_out(0x3C5, al);
        SET_CB(CS_RowCounter, CB(CS_SpriteVisibleRows));
        b->di = CW(CS_SpriteDestOffset);
        b->si = (uint16_t)(b->si + CW(CS_SpriteSourceSkip));
        do {                                    /* loc_2A67 */
            uint8_t cl = b->vis, ah = 0, dl;
            do {
                uint8_t v = bitrev(rd8(b->src, b->si));
                b->si--;
                dl = v;
                (void)vmem_xchg(b->di, (uint8_t)((v & b->ch) | ah));
                b->di++;
                dl &= rmask;
                ah = dl;
            } while (--cl != 0);
            (void)vmem_xchg(b->di, ah);
            b->si = (uint16_t)(b->si + 2u * cbw(b->vis));
            b->di = (uint16_t)(b->di - bp);
        } while (row_dec() != 0);
        SET_CB(CS_CurrentPlaneMask, (uint8_t)(CB(CS_CurrentPlaneMask) << 1));
        al = CB(CS_CurrentPlaneMask);
        if (al >= 0x10) break;
    }
}

/* loc_2AA8: clear pass for a clipped row (left and/or right). The first byte of a row with
 * left clipping is consumed without a write (CX is bumped to compensate). */
static void mirror_clear_clip(Blit *b)
{
    uint16_t bp = blit_bp(b);
    uint8_t rmask = (uint8_t)~b->ch;
    do {
        uint8_t cl = b->vis, ah, dl, v;
        uint16_t lw = cbw(CB(CS_SpriteLeftClipBytes));
        ah = HI(lw);
        if (lw != 0) {
            b->si = (uint16_t)(b->si - (uint16_t)(lw - 1));
            dl = bitrev(rd8(b->src, b->si));
            b->si--;
            cl++;
        } else {
            v = bitrev(rd8(b->src, b->si));
            b->si--;
            dl = v;
            (void)vmem_xchg(b->di, (uint8_t)~((v & b->ch) | ah));
            b->di++;
        }
        for (;;) {                              /* loc_2ACB */
            dl &= rmask;
            ah = dl;
            if (--cl == 0) break;
            v = bitrev(rd8(b->src, b->si));
            b->si--;
            dl = v;
            (void)vmem_xchg(b->di, (uint8_t)~((v & b->ch) | ah));
            b->di++;
        }
        ah = (uint8_t)((uint8_t)~ah | CB(CS_EdgeByteInvert));
        (void)vmem_xchg(b->di, ah);
        uint8_t rb = CB(CS_SpriteRowBytes);
        b->si = (uint16_t)(b->si + cbw(rb));
        b->si = (uint16_t)(b->si + (uint8_t)(rb - CB(CS_SpriteRightClipBytes)));
        b->di = (uint16_t)(b->di - bp);
    } while (row_dec() != 0);
}

/* loc_2B23 .. loc_2BC0: clipped colour passes (OR). Flash mode runs four 0Fh passes here. */
static void mirror_or_clip(Blit *b)
{
    uint16_t bp = blit_bp(b);
    uint8_t rmask = (uint8_t)~b->ch;
    vga_out(0x3CE, 3); vga_out(0x3CF, (uint8_t)(b->xs | 0x10));
    seq_index(2);
    SET_CB(CS_EdgeByteInvert, (uint8_t)~CB(CS_EdgeByteInvert));
    uint8_t al = CB(CS_PlanePassMode);
    if (al == 0) return;
    if (al == 0x10) { SET_CB(CS_CurrentPlaneMask, 1); al = 0x0F; b->si = (uint16_t)(b->si - CW(CS_SpriteSourceSkip)); }
    else SET_CB(CS_CurrentPlaneMask, al);
    for (;;) {                                  /* loc_2B3A */
        vga_out(0x3C5, al);
        SET_CB(CS_RowCounter, CB(CS_SpriteVisibleRows));
        b->di = CW(CS_SpriteDestOffset);
        b->si = (uint16_t)(b->si + CW(CS_SpriteSourceSkip));
        do {                                    /* loc_2B59 */
            uint8_t cl = b->vis, ah = 0, dl, v;
            uint16_t lw = cbw(CB(CS_SpriteLeftClipBytes));
            ah = HI(lw);
            if (lw != 0) {
                b->si = (uint16_t)(b->si - (uint16_t)(lw - 1));
                dl = bitrev(rd8(b->src, b->si));
                b->si--;
                cl++;
            } else {
                v = bitrev(rd8(b->src, b->si));
                b->si--;
                dl = v;
                (void)vmem_xchg(b->di, (uint8_t)((v & b->ch) | ah));
                b->di++;
            }
            for (;;) {                          /* loc_2B7A */
                dl &= rmask;
                ah = dl;
                if (--cl == 0) break;
                v = bitrev(rd8(b->src, b->si));
                b->si--;
                dl = v;
                (void)vmem_xchg(b->di, (uint8_t)((v & b->ch) | ah));
                b->di++;
            }
            ah &= CB(CS_EdgeByteInvert);
            (void)vmem_xchg(b->di, ah);
            uint8_t rb = CB(CS_SpriteRowBytes);
            b->si = (uint16_t)(b->si + cbw(rb));
            b->si = (uint16_t)(b->si + (uint8_t)(rb - CB(CS_SpriteRightClipBytes)));
            b->di = (uint16_t)(b->di - bp);
        } while (row_dec() != 0);
        SET_CB(CS_CurrentPlaneMask, (uint8_t)(CB(CS_CurrentPlaneMask) << 1));
        al = CB(CS_CurrentPlaneMask);
        if (al >= 0x10) return;
        if (CB(CS_PlanePassMode) == 0x10) al = 0x0F;
    }
}

static void DrawSpriteMirrored(Blit *b)
{
    SET_CB(CS_RowCounter, b->rows);
    b->si = (uint16_t)(b->si - 1);           /* std; dec si */
    if (CB(CS_ClipXFlag) != 0) {             /* loc_2AA1: first row starts a row later */
        b->si = (uint16_t)(b->si + cbw(CB(CS_SpriteRowBytes)));
        mirror_clear_clip(b);
        mirror_or_clip(b);
        return;
    }
    b->si = (uint16_t)(b->si + cbw(b->vis));  /* loc_292C */
    if (b->ch == 0xFF) {                     /* loc_2939: aligned */
        if (CB(CS_PlanePassMode) != 0x10) mirror_clear_aligned(b);
        mirror_or_aligned(b);
    } else {
        if (CB(CS_PlanePassMode) != 0x10) mirror_clear_partial(b);
        mirror_or_partial(b);
    }
}

/* ---- DrawSpriteStraight: frame read forwards, no bit reversal --------------------- */

/* loc_2BDD: clear pass, aligned: NOT(src) ANDed into A000h. */
static void straight_clear_aligned(Blit *b)
{
    uint16_t bp = blit_bp(b);
    uint8_t dl = b->rows;
    do {
        uint16_t cx = b->vis;
        do {
            uint8_t v = (uint8_t)~rd8(b->src, b->si);
            b->si++;
            (void)vmem_xchg(b->di, v);
            b->di++;
        } while (--cx != 0);
        b->di = (uint16_t)(b->di - bp);
    } while (--dl != 0);
}

/* loc_2BED .. loc_2C42: colour passes, aligned (OR of the source bytes as they are). */
static void straight_or_aligned(Blit *b)
{
    uint16_t bp = blit_bp(b);
    vga_out(0x3CE, 3); vga_out(0x3CF, 0x10);
    seq_index(2);
    uint8_t al = CB(CS_PlanePassMode);
    if (al == 0) return;
    if (al == 0x10) { al = 0x0F; b->si = (uint16_t)(b->si - CW(CS_SpriteSourceSkip)); }
    for (;;) {                                  /* loc_2C0C */
        SET_CB(CS_CurrentPlaneMask, al);
        vga_out(0x3C5, al);
        uint8_t dl = CB(CS_SpriteVisibleRows);
        b->di = CW(CS_SpriteDestOffset);
        b->si = (uint16_t)(b->si + CW(CS_SpriteSourceSkip));
        do {                                    /* loc_2C2A */
            uint16_t cx = b->vis;
            do {
                uint8_t v = rd8(b->src, b->si);
                b->si++;
                (void)vmem_xchg(b->di, v);
                b->di++;
            } while (--cx != 0);
            b->di = (uint16_t)(b->di - bp);
        } while (--dl != 0);
        SET_CB(CS_CurrentPlaneMask, (uint8_t)(CB(CS_CurrentPlaneMask) << 1));
        al = CB(CS_CurrentPlaneMask);
        if (al >= 0x10) break;
    }
}

/* loc_2C45: clear pass with a shifted byte (carry in AH, reset at each row). */
static void straight_clear_partial(Blit *b)
{
    uint16_t bp = blit_bp(b);
    uint8_t rmask = (uint8_t)~b->ch;
    uint8_t dl = b->rows;
    do {
        uint8_t cl = b->vis, ah = 0, bl;
        do {
            uint8_t v = rd8(b->src, b->si);
            b->si++;
            bl = v;
            (void)vmem_xchg(b->di, (uint8_t)~((v & b->ch) | ah));
            b->di++;
            bl &= rmask;
            ah = bl;
        } while (--cl != 0);
        ah = (uint8_t)~ah;
        (void)vmem_xchg(b->di, ah);
        b->di = (uint16_t)(b->di - bp);
    } while (--dl != 0);
}

/* loc_2C70 .. loc_2CE7: colour passes, shifted (OR, carry written at row end). */
static void straight_or_partial(Blit *b)
{
    uint16_t bp = blit_bp(b);
    uint8_t rmask = (uint8_t)~b->ch;
    vga_out(0x3CE, 3); vga_out(0x3CF, (uint8_t)(b->xs | 0x10));
    seq_index(2);
    SET_CB(CS_EdgeByteInvert, (uint8_t)~CB(CS_EdgeByteInvert));
    uint8_t al = CB(CS_PlanePassMode);
    if (al == 0) return;
    if (al == 0x10) { al = 0x0F; b->si = (uint16_t)(b->si - CW(CS_SpriteSourceSkip)); }
    for (;;) {                                  /* loc_2CA0 */
        SET_CB(CS_CurrentPlaneMask, al);
        vga_out(0x3C5, al);
        uint8_t dl = CB(CS_SpriteVisibleRows);
        b->di = CW(CS_SpriteDestOffset);
        b->si = (uint16_t)(b->si + CW(CS_SpriteSourceSkip));
        do {                                    /* loc_2CBE */
            uint8_t cl = b->vis, ah = 0, bl;
            do {
                uint8_t v = rd8(b->src, b->si);
                b->si++;
                bl = v;
                (void)vmem_xchg(b->di, (uint8_t)((v & b->ch) | ah));
                b->di++;
                bl &= rmask;
                ah = bl;
            } while (--cl != 0);
            (void)vmem_xchg(b->di, ah);
            b->di = (uint16_t)(b->di - bp);
        } while (--dl != 0);
        SET_CB(CS_CurrentPlaneMask, (uint8_t)(CB(CS_CurrentPlaneMask) << 1));
        al = CB(CS_CurrentPlaneMask);
        if (al >= 0x10) break;
    }
}

/* loc_2CEA: clear pass for a clipped row. The row's source bytes are: left clipped bytes
 * (skipped, the first one still feeds the carry), visible bytes, right clipped bytes. */
static void straight_clear_clip(Blit *b)
{
    uint16_t bp = blit_bp(b);
    uint8_t rmask = (uint8_t)~b->ch;
    uint8_t dl = b->rows;
    do {
        uint8_t cl = b->vis, ah = 0, bl, v;
        uint16_t lw = cbw(CB(CS_SpriteLeftClipBytes));
        if (lw != 0) {
            b->si = (uint16_t)(b->si + (uint16_t)(lw - 1));
            cl++;
            bl = rd8(b->src, b->si);
            b->si++;
        } else {
            v = rd8(b->src, b->si);
            b->si++;
            bl = v;
            (void)vmem_xchg(b->di, (uint8_t)~((v & b->ch) | ah));
            b->di++;
        }
        for (;;) {                              /* loc_2D08 */
            bl &= rmask;
            ah = bl;
            if (--cl == 0) break;
            v = rd8(b->src, b->si);
            b->si++;
            bl = v;
            (void)vmem_xchg(b->di, (uint8_t)~((v & b->ch) | ah));
            b->di++;
        }
        ah = (uint8_t)((uint8_t)~ah | CB(CS_EdgeByteInvert));
        (void)vmem_xchg(b->di, ah);
        b->si = (uint16_t)(b->si + cbw(CB(CS_SpriteRightClipBytes)));
        b->di = (uint16_t)(b->di - bp);
    } while (--dl != 0);
}

/* loc_2D54 .. loc_2D9F: clipped colour passes (OR). */
static void straight_or_clip(Blit *b)
{
    uint16_t bp = blit_bp(b);
    uint8_t rmask = (uint8_t)~b->ch;
    vga_out(0x3CE, 3); vga_out(0x3CF, (uint8_t)(b->xs | 0x10));
    seq_index(2);
    SET_CB(CS_EdgeByteInvert, (uint8_t)~CB(CS_EdgeByteInvert));
    uint8_t al = CB(CS_PlanePassMode);
    if (al == 0) return;
    if (al == 0x10) { SET_CB(CS_CurrentPlaneMask, 1); al = 0x0F; b->si = (uint16_t)(b->si - CW(CS_SpriteSourceSkip)); }
    else SET_CB(CS_CurrentPlaneMask, al);
    for (;;) {                                  /* loc_2D6B */
        vga_out(0x3C5, al);
        uint8_t dl = CB(CS_SpriteVisibleRows);
        b->di = CW(CS_SpriteDestOffset);
        b->si = (uint16_t)(b->si + CW(CS_SpriteSourceSkip));
        do {                                    /* loc_2D83 */
            uint8_t cl = b->vis, ah = 0, bl, v;
            uint16_t lw = cbw(CB(CS_SpriteLeftClipBytes));
            if (lw != 0) {
                b->si = (uint16_t)(b->si + (uint16_t)(lw - 1));
                cl++;
                bl = rd8(b->src, b->si);
                b->si++;
            } else {
                v = rd8(b->src, b->si);
                b->si++;
                bl = v;
                (void)vmem_xchg(b->di, (uint8_t)((v & b->ch) | ah));
                b->di++;
            }
            for (;;) {                          /* loc_2D9F */
                bl &= rmask;
                ah = bl;
                if (--cl == 0) break;
                v = rd8(b->src, b->si);
                b->si++;
                bl = v;
                (void)vmem_xchg(b->di, (uint8_t)((v & b->ch) | ah));
                b->di++;
            }
            ah &= CB(CS_EdgeByteInvert);
            (void)vmem_xchg(b->di, ah);
            b->si = (uint16_t)(b->si + cbw(CB(CS_SpriteRightClipBytes)));
            b->di = (uint16_t)(b->di - bp);
        } while (--dl != 0);
        SET_CB(CS_CurrentPlaneMask, (uint8_t)(CB(CS_CurrentPlaneMask) << 1));
        al = CB(CS_CurrentPlaneMask);
        if (al >= 0x10) return;
        if (CB(CS_PlanePassMode) == 0x10) al = 0x0F;
    }
}

static void DrawSpriteStraight(Blit *b)
{
    if (CB(CS_ClipXFlag) != 0) {
        straight_clear_clip(b);
        straight_or_clip(b);
        return;
    }
    if (b->ch == 0xFF) {                         /* loc_2BCE: aligned */
        if (CB(CS_PlanePassMode) != 0x10) straight_clear_aligned(b);
        straight_or_aligned(b);
    } else {
        if (CB(CS_PlanePassMode) != 0x10) straight_clear_partial(b);
        straight_or_partial(b);
    }
}

/* ---- DrawSpriteObjects ----------------------------------------------------------------- */

/* One object's draw: the body of SpriteObjectLoop from the empty check to NextSpriteObject.
 * si is the record (DATA). Record: +0 X, +2 Y, +4 frame word, +11h blink countdown. */
static void draw_object(uint16_t si)
{
    SET_CB(CS_ClipXFlag, 0);
    SET_CB(CS_SpriteLeftClipBytes, 0);
    SET_CB(CS_SpriteRightClipBytes, 0);
    SET_CB(CS_EdgeByteInvert, 0);
    SET_CW(CS_SpriteSourceSkip, 0);
    SET_DB(si + 5, DB(si + 5) & 0xDF);                  /* clear the drawn flag */

    uint16_t fw = DW(si + 4);
    /* shl bx,1 then rcl MirrorBits: the mirror bit (15) enters bit 0. MirrorBits keeps the
     * older bits above it, as the original does (never reset). */
    SET_CB(CS_MirrorBits, (uint8_t)((CB(CS_MirrorBits) << 1) | (fw >> 15)));
    uint16_t bx = (uint16_t)(fw << 1);

    /* Blink countdown, saturating at 0 (sub byte [..],1; adc byte [..],0). The record is
     * drawn when the countdown is zero or SnowFrameTick (low bits) is 0. */
    uint8_t blink = DB(si + 0x11);
    uint8_t cnt = (uint8_t)(blink - 1);
    if (blink == 0) cnt = (uint8_t)(cnt + 1);
    SET_DB(si + 0x11, cnt);
    uint8_t pass;
    if (cnt == 0 || (DB(DS_SnowFrameTick) & 3) == 0) {
        pass = (bx & 0x8000u) ? 0x10 : 1;               /* flash: all planes */
    } else {
        pass = 0;                                       /* blinking off: clear only */
    }
    bx &= 0x7FFF;                                       /* and bh, 7Fh */
    SET_CB(CS_PlanePassMode, pass);

    uint16_t dx = DW(DS_FrameWidths + bx);              /* dl = width, dh = height */
    uint8_t w = LO(dx), h = HI(dx);
    uint8_t hx = DB(DS_FrameHotX + bx);
    if (CB(CS_MirrorBits) & 1) hx = (uint8_t)(w - hx);  /* sub al,dl; neg al */
    uint16_t ax = (uint16_t)(0u - cbw(hx)) + DW(si);    /* cbw; neg ax; add ax,[si] */

    uint16_t rel;
    if (bx == FRAME_309_BX) {
        /* Frame 309: no camera, no left test, no rel > 0 test (quirk). */
        SET_CW(CS_SpriteScreenX, ax);
        rel = (uint16_t)(cbw(DB(DS_FrameHotY + bx)) + DW(si + 2));
    } else {
        ax = (uint16_t)(ax - (uint16_t)(DW(DS_CameraCol) << 4));
        SET_CW(CS_SpriteScreenX, ax);
        if ((int16_t)ax >= 0x140) return;               /* jl: left < 320 */
        uint16_t cx = (uint16_t)(ax + w);              /* add cl,dl; adc ch,0 */
        if (cx & 0x8000u) return;                       /* right edge left of column 0 */
        ax = (uint16_t)(cbw(DB(DS_FrameHotY + bx)) + DW(si + 2) + DW(DS_ShakeRows));
        cx = (uint16_t)((DW(DS_CameraRow) << 4) + DB(DS_FineScroll));
        ax = (uint16_t)(ax - cx);
        if ((int16_t)ax <= 0) return;                   /* jg: rel > 0 */
        rel = ax;
    }

    /* loc_27DE: vertical extent. */
    SET_DW(DS_SpriteRelY, rel);
    SET_CB(CS_SpriteVisibleRows, h);
    SET_CB(CS_RowCounter, h);
    uint16_t top = (uint16_t)(rel - h);                 /* sub al,dh; sbb ah,0 */
    if ((int16_t)top >= 0xB0) return;                   /* jl: top < 176 */
    SET_CW(CS_SpriteTopRow, top);

    uint8_t sh = CB(CS_cvar_0000);                      /* 3: pixels to bytes */
    uint8_t vb = sh < 8 ? (uint8_t)(w >> sh) : 0;       /* shr dl, cl */
    SET_CB(CS_SpriteVisibleBytes, vb);
    SET_CB(CS_SpriteRowBytes, vb);

    uint16_t di, cx = 0;
    if ((int16_t)top >= 0) {                            /* loc_2828 */
        di = (uint16_t)(LO(top) * DB(DS_ScreenRowBytes)); /* mul byte ptr ScreenRowBytes */
    } else {                                            /* top clipped above row 0 */
        uint16_t neg = (uint16_t)(0u - top);
        SET_CB(CS_SpriteVisibleRows, (uint8_t)(CB(CS_SpriteVisibleRows) - LO(neg)));
        cx = (uint16_t)(LO(neg) * vb);                  /* mul dl */
        SET_CW(CS_SpriteSourceSkip, cx);
        di = 0;
    }
    di = (uint16_t)(di + DW(DS_DrawPage));
    SET_CW(CS_SpriteDestOffset, di);
    SET_CW(CS_SpriteSrcSegment, DW(DS_FrameDataSegments + bx));
    SET_CW(CS_SpriteSrcOffset, (uint16_t)(DW(DS_FrameDataOffsets + bx) + cx));

    /* Rows below row 176 (bottom clip): the byte count is the low byte of rel - 176. */
    if ((int16_t)DW(DS_SpriteRelY) > 0xB0) {            /* jle skips when rel <= 176 */
        uint8_t t = LO(DW(DS_SpriteRelY) - 0xB0u);
        SET_CB(CS_SpriteVisibleRows, (uint8_t)(CB(CS_SpriteVisibleRows) - t));
        SET_CW(CS_SpriteSourceSkip, (uint16_t)(CW(CS_SpriteSourceSkip) + (uint16_t)(t * vb)));
    }

    /* loc_285D: horizontal extent. Negative X clips bytes on the left; X >= 0 may clip
     * on the right when the visible bytes pass column 40. */
    int16_t sx = (int16_t)CW(CS_SpriteScreenX);
    if (sx < 0) {
        uint8_t t = LO((uint16_t)(sx >> 3));            /* sar ax, cl */
        SET_CB(CS_SpriteVisibleBytes, (uint8_t)(CB(CS_SpriteVisibleBytes) + t));
        SET_CB(CS_SpriteLeftClipBytes, (uint8_t)(0u - t));
        SET_CB(CS_ClipXFlag, (uint8_t)(CB(CS_ClipXFlag) + 1));
    } else {
        SET_CW(CS_SpriteDestOffset, (uint16_t)(CW(CS_SpriteDestOffset) + (uint16_t)(sx >> 3)));
        uint16_t sum = (uint16_t)((uint16_t)(sx >> 3) + CB(CS_SpriteVisibleBytes));
        if (sum >= DW(DS_ScreenRowBytes)) {             /* sub ax, ScreenRowBytes; jb skips */
            uint8_t over = LO(sum - DW(DS_ScreenRowBytes));
            SET_CB(CS_SpriteVisibleBytes, (uint8_t)(CB(CS_SpriteVisibleBytes) - over));
            SET_CB(CS_ClipXFlag, (uint8_t)(CB(CS_ClipXFlag) + 1));
            SET_CB(CS_SpriteRightClipBytes, over);
            SET_CB(CS_EdgeByteInvert, (uint8_t)~CB(CS_EdgeByteInvert));
        }
    }

    /* loc_28B6: the record is marked drawn, the flash request is consumed. */
    SET_DB(si + 5, (uint8_t)((DB(si + 5) | 0x20) & 0xBF));
    set_cpu_write_mode();
    vga_out(0x3C4, 0x02); vga_out(0x3C5, 0x0F);        /* map mask: all four planes (for the clear) */

    Blit b;
    uint8_t xs = (uint8_t)(LO(CW(CS_SpriteScreenX)) & 7);
    b.xs = xs;
    b.ch = (uint8_t)(0xFFu << xs);
    b.di = CW(CS_SpriteDestOffset);
    b.si = CW(CS_SpriteSrcOffset);
    b.src = CW(CS_SpriteSrcSegment);
    b.vis = CB(CS_SpriteVisibleBytes);
    b.rows = CB(CS_SpriteVisibleRows);
    vga_out(0x3CE, 3); vga_out(0x3CF, (uint8_t)(xs | 0x08));   /* GC 03h = (X & 7) | AND */

    if (CB(CS_MirrorBits) & 1) DrawSpriteMirrored(&b);
    else DrawSpriteStraight(&b);
}

void DrawSpriteObjects(void)
{
    SET_DW(DS_SnowFrameTick, (uint16_t)(DW(DS_SnowFrameTick) + 1));
    uint16_t si = 0x5720;                               /* walked down to 4F0Ah */
    for (;;) {
        SET_DW(DS_CurrentObject, si);
        if (DW(si + 4) != 0xFFFF) draw_object(si);     /* FFFFh: empty record */
        si = (uint16_t)(DW(DS_CurrentObject) - 0x12);
        if (si < 0x4F0A) break;
    }
}

/* ---- LoadSpriteSheet ------------------------------------------------------------------- */

/* Width in bytes of a frame row: ceil(w / 8), computed as the original's ror 3 sequence. */
static uint8_t width_bytes(uint8_t w)
{
    uint8_t r = (uint8_t)((w >> 3) | (w << 5));         /* ror al, 3 */
    if (r & 0xE0) return (uint8_t)((r & 0x1F) + 1);
    return r;
}

/* MoveSpriteDataUp: copies `paras` paragraphs of the raw data up, from the top paragraph
 * down (top_src -> top_dst, one paragraph per step, 16 bytes each, DF = 1). Paras = 0 runs
 * 65536 steps, as the original's jne after dec does. */
static void MoveSpriteDataUp(uint16_t paras, uint16_t top_src, uint16_t top_dst)
{
    uint16_t ax = top_src, bx = top_dst, bp = paras;
    do {
        for (uint16_t k = 0; k < 16; k++) {             /* rep movsb, DF = 1: 16 bytes backwards */
            wr8(bx, (uint16_t)(15 - k), rd8(ax, (uint16_t)(15 - k)));
        }
        ax--;
        bx--;
    } while (--bp != 0);
}

/* ExpandSpriteFrames: expands each frame from the low end while the output stays below
 * the input. Block 0 = OR of the four planes; blocks 1..4 = the planes. */
static void ExpandSpriteFrames(void)
{
    uint16_t si = 0, di = 0, bx = DS_FrameWidths;
    for (;;) {
        SET_DW(DS_ExpandSrcSegment, (uint16_t)(DW(DS_ExpandSrcSegment) + (si >> 4)));
        si &= 0x0F;
        SET_DW(DS_ExpandDstSegment, (uint16_t)(DW(DS_ExpandDstSegment) + (di >> 4)));
        di &= 0x0F;
        uint16_t w = DW(bx);
        if (LO(w) == 0) break;                          /* zero width: end of the table */
        uint8_t bytes = width_bytes(LO(w));
        bx = (uint16_t)(bx + 2);
        uint16_t bp = (uint16_t)(bytes * HI(w));        /* mul ah */
        uint16_t es = DW(DS_ExpandDstSegment), ds = DW(DS_ExpandSrcSegment);
        uint16_t cx = bp, s = si;
        do {                                            /* loc_2EBA */
            uint8_t al = rd8(ds, s);
            al |= rd8(ds, (uint16_t)(s + bp));
            al |= rd8(ds, (uint16_t)(s + 2u * bp));
            al |= rd8(ds, (uint16_t)(s + 3u * bp));
            wr8(es, di, al);
            di++;
            s++;
        } while (--cx != 0);
        /* rep movsw: 2*bp words (the four planes) from ds:si to es:di */
        uint16_t words = (uint16_t)(bp << 1);
        for (uint16_t k = 0; k < words; k++) {
            wr16(es, di, rd16(ds, si));
            si = (uint16_t)(si + 2);
            di = (uint16_t)(di + 2);
        }
    }
    /* loc_2EF1 onwards: BuildFrameOffsets (the expansion's loop exit falls through here) */
}

/* BuildFrameOffsets: start offset of each frame's 5 blocks in the expanded data. */
static void BuildFrameOffsets(uint16_t seg)
{
    uint16_t bx = seg, si = DS_FrameWidths, di = 0, dx = 0;
    for (;;) {
        uint16_t ax = DW(si);
        si = (uint16_t)(si + 2);
        if (LO(ax) == 0) break;
        uint8_t bytes = width_bytes(LO(ax));
        uint16_t bp = (uint16_t)(bytes * HI(ax));       /* mul ah */
        ax = (uint16_t)(bp * 5u);                       /* shl 2; add bp: 5 blocks */
        SET_DW((uint16_t)(DS_FrameDataOffsets + di), dx);
        SET_DW((uint16_t)(DS_FrameDataSegments + di), bx);
        if (dx >= 0x1000) {                             /* subtracted once per frame (quirk) */
            dx = (uint16_t)(dx - 0x1000);
            bx = (uint16_t)(bx + 0x100);
        }
        di = (uint16_t)(di + 2);
        dx = (uint16_t)(dx + ax);
    }
}

int LoadSpriteSheet(void)
{
    uint16_t seg = 0;
    int e = LoadSqzFile(SPRITE_SQZ_NAME, &seg);
    if (e) return e;
    SET_DW(DS_SpriteDataSegment, seg);

    /* Hot Y becomes (height - hot) up to the first zero height. */
    for (uint16_t bx = 0; DB(DS_FrameHeights + bx) != 0; bx = (uint16_t)(bx + 2)) {
        uint8_t h = DB(DS_FrameHeights + bx);
        uint8_t t = (uint8_t)(DB(DS_FrameHotY + bx) - h);
        SET_DB(DS_FrameHotY + bx, (uint8_t)(0u - t));
    }

    /* The raw data (paras paragraphs at seg) moves up by dx = paras/4 + 1 paragraphs, and
     * NextFreeSegment becomes the new end. The expansion writes from seg and reads from
     * seg + dx. */
    uint16_t paras = (uint16_t)(DW(DS_NextFreeSegment) - seg);
    uint16_t dx = (uint16_t)((paras >> 2) + 1);
    uint16_t end = (uint16_t)(paras + dx + seg);
    SET_DW(DS_NextFreeSegment, end);
    uint16_t top_dst = (uint16_t)(end - 1);
    uint16_t top_src = (uint16_t)(top_dst - dx);
    MoveSpriteDataUp(paras, top_src, top_dst);
    SET_DW(DS_ExpandDstSegment, (uint16_t)(top_src - paras + 1));
    SET_DW(DS_ExpandSrcSegment, (uint16_t)(top_dst - paras + 1));

    ExpandSpriteFrames();
    BuildFrameOffsets(seg);
    return 0;
}

/* ---- differential-test adapters ---------------------------------------------------------- */

EXPORT int c_LoadSpriteSheet(Regs *r)
{
    int e = LoadSpriteSheet();
    r->ds = DATA_SEG;
    return e;
}

EXPORT int c_DrawSpriteObjects(Regs *r)
{
    DrawSpriteObjects();
    r->ds = DATA_SEG;
    return 0;
}
