/* Front-end and fixed screens (oracle: src/MODULE4.ASM cs:9520..9DFB and src/MODULE3.ASM
 * cs:8E45..9286).
 *
 *   ShowWorldMapScrollIn   stage marker stamp, map scroll-in, hold, fade (loc_9613 is its
 *                          frame loop; loc_96AE the hold; loc_96C1 the fade tail).
 *   RunCallbackMapScreen   map screen loop (loc_97A8 is the loop; loc_9881 the fade tail).
 *                          The frame callback is the CS offset in FrameCallback and is
 *                          dispatched by value: UpdateModeSelectFrame or UpdateCodeEntryFrame.
 *   RunGameOverScreen      GAME OVER screen (loc_9C62 is the frame loop and the fade tail).
 *   RunFrontEnd            MODE / ENTER CODE / level start flow (loc_8EFD: teardown).
 *   RunLevelIntroScene     level-start intro (loc_8F96: first walk loop, loc_9002: walk-off).
 *   ShowMode13PictureFadeIn, ShowPlanarPicture, ShowPresentTitle, ShowTitusScreen and
 *   FadeOutPaletteToBlack  picture and palette fades (loc_91CF: the fade-in passes).
 *
 * Waits poll platform_poll(). Frame callbacks and labels that the tests enter are C
 * functions named after their ASM labels. Quirks kept on purpose (each noted at its site):
 *   - the GAME OVER planes are cleared at 3F40h but copied to 5E80h (DI is not reset);
 *   - RunLevelIntroScene leaves DS = A000h after RestoreBackdropToDrawPage, so the PresentFrame
 *     that follows reads A000:6BE7h (a VRAM read that loads the latches);
 *   - the palette fades count only the +/-1 snaps of ShowPresentTitle's morph as passes;
 *   - loops with CX = 0 run 65536 times (LOOP semantics), as in the original.
 */
#include "pre2.h"
#include "vga.h"
#include "ports.h"
#include "platform.h"
#include "render.h"
#include "sprites.h"
#include "system.h"
#include "sqz.h"
#include "screens.h"
#include "player.h"
#include "effects.h"

#define SEG_VGA        0xA000u
#define ATTR_PAS_PEL   0x33u         /* attribute index 13h with PAS set: pel pan */
#define RS_TOP_PARAS   0x105Bu       /* paragraphs of a resource copied into ModuleSegment */
#define LEVEL_TABLE_X  0xB148u       /* DATA: per-level stage node, word x and word y (4 bytes) */
#define SCAN_TABLE     0xB068u       /* DATA: scan code -> character (128 bytes) */
#define CODE_BOX       0xB170u       /* DATA: the four ENTER CODE input characters */
#define ROW_PITCH      0x28u         /* VRAM bytes per row (plane) */
#define FONT_STRIDE    0x1080u       /* bytes per pixel-shifted font table (phase) */

/* ---- memory access ---------------------------------------------------------------------
 * The VGA window (A000h) goes through the VGA model; every other segment is plain memory. */
static uint8_t mb_rd(uint16_t seg, uint16_t off)
{
    return seg == SEG_VGA ? vga_read(off) : rd8(seg, off);
}

static void mb_wr(uint16_t seg, uint16_t off, uint8_t v)
{
    if (seg == SEG_VGA) vga_write(off, v);
    else wr8(seg, off, v);
}

static uint16_t mw_rd(uint16_t seg, uint16_t off)           /* low byte first */
{
    uint8_t lo = mb_rd(seg, off);
    uint8_t hi = mb_rd(seg, (uint16_t)(off + 1));
    return (uint16_t)(lo | hi << 8);
}

static void mw_wr(uint16_t seg, uint16_t off, uint16_t v)  /* low byte first */
{
    mb_wr(seg, off, (uint8_t)v);
    mb_wr(seg, (uint16_t)(off + 1), (uint8_t)(v >> 8));
}

static uint8_t  rol8(uint8_t v, unsigned n)  { return (uint8_t)((v << n) | (v >> (8 - n))); }
static uint16_t ror16(uint16_t v, unsigned n) { return (uint16_t)((v >> n) | (v << (16 - n))); }

/* A busy-wait on the retrace or the timer that the host may pump. */
static void wait_poll(void) { platform_poll(); }

/* Frame callbacks reached through FrameCallback (near call CS:offset). */
static int call_frame_callback(void)
{
    uint16_t cb = DW(DS_FrameCallback);
    if (cb == CS_UpdateModeSelectFrame) { UpdateModeSelectFrame(); return SCR_OK; }
    if (cb == CS_UpdateCodeEntryFrame) { UpdateCodeEntryFrame(); return SCR_OK; }
    return SCR_UNSUPPORTED;                  /* any other target: the original runs arbitrary code */
}

/* ---- resource paging -------------------------------------------------------------------
 * PageInResource leaves no result (its contract says AX is not defined), but on the disk
 * path the original's AX at return is the decoded segment (pinned, flags 08h) or that
 * segment + 105Bh (copied into ModuleSegment, loc_06B3 counts AX up). The front end stores
 * AX into LevelSegment, so the disk path is reproduced here with the same effects as
 * c/system.c's page_in_disk (heap top restored). Other locations go through PageInResource. */
static int page_in_seg(uint16_t slot, uint16_t *ax)
{
    uint8_t flags = DB((uint16_t)(DS_ResourceFlags + slot));
    if (((flags >> 1) & 3) != 0) { *ax = 0; return PageInResource(slot); }
    SET_DB(DS_byte_0B16, flags & 0x08);
    uint16_t saved = DW(DS_NextFreeSegment), seg = 0;
    int st = LoadSqzFile(ResourceNameAt(slot), &seg);
    if (st) return st;
    *ax = seg;
    if (DB(DS_byte_0B16) == 0) {                         /* copy 105Bh paragraphs to ModuleSegment */
        uint16_t dst = DW(DS_ModuleSegment), src = seg;
        for (uint16_t n = RS_TOP_PARAS; n != 0; n--) {
            for (uint16_t k = 0; k < 8; k++) wr16(dst, (uint16_t)(2 * k), rd16(src, (uint16_t)(2 * k)));
            dst = (uint16_t)(dst + 1);
            src = (uint16_t)(src + 1);
        }
        *ax = (uint16_t)(seg + RS_TOP_PARAS);
    }
    SET_DW(DS_NextFreeSegment, saved);
    return SCR_OK;
}

/* ---- ShowWorldMapScrollIn (cs:9520) ------------------------------------------------------ */

/* Frame loop (loc_9613): the column copy, the input sample and the retrace wait, then the
 * hold or fade exits. Ends with the fade tail loc_96C1. */
static void loc_96AE(void);
static void loc_96C1(void);

static void loc_9613(void);

static void loc_9613(void)
{
    for (;;) {
        uint16_t ax = (uint16_t)(0x28 * DW(DS_MapScrollY));
        uint16_t bx = (uint16_t)((DW(DS_MapScrollX) >> 3) + ax);
        bx &= 0x1FFF;                                    /* and bh, 1Fh */
        port_out16(0x3D4, (uint16_t)((bx & 0xFF00) | 0x0C));
        port_out16(0x3D4, (uint16_t)(((bx & 0xFF) << 8) | 0x0D));
        uint16_t old = DW(DS_ScreenStartOffset);
        SET_DW(DS_ScreenStartOffset, bx);
        SET_DW(DS_PrevScreenStartOffset, old);
        SetCpuWriteMode();
        PollInputState();
        WaitNextVerticalRetrace();
        port_out8(0x3C4, 2);
        port_out8(0x3C0, ATTR_PAS_PEL);
        port_out8(0x3C0, (uint8_t)(DB(DS_MapScrollX) & 7));
        uint16_t dx = DW(DS_MapScrollX);
        SET_DW(DS_MapScrollX, (uint16_t)(dx + 1));
        if ((dx & 7) == 0) {
            /* column copy: from the map buffer (DS = word_2875) to VRAM, four planes, 200 rows;
             * the destination masks to 8K each row, the source does not */
            uint16_t map = DW(DS_NextFreeSegment);
            uint16_t col = (uint16_t)((dx >> 3) - 1);
            uint16_t si = col, di = col;
            for (uint8_t al = 1; al < 0x10; al <<= 1) {
                port_out8(0x3C5, al);
                uint16_t cx = 0xC8;
                do {
                    di &= 0x1FFF;
                    mb_wr(SEG_VGA, di, rd8(map, si));
                    si = (uint16_t)(si + 1 + 0x4F);
                    di = (uint16_t)(di + 1 + 0x27);
                } while (--cx != 0);
                di = (uint16_t)(di - 0x1F40);
            }
        }
        if (DB(DS_byte_27E8) != 0) break;                 /* loc_96C1 */
        if (DW(DS_MapScrollX) >= 0x280) { loc_96AE(); return; }
    }
    loc_96C1();
}

/* Hold (loc_96AE): up to 70 frames of retrace wait and input, ends on the action flag. */
static void loc_96AE(void)
{
    for (uint16_t cx = 0x46; cx != 0; cx--) {
        WaitNextVerticalRetrace();
        PollInputState();
        if (DB(DS_byte_27E8) != 0) break;
    }
    loc_96C1();
}

/* Fade tail (loc_96C1). */
static void loc_96C1(void)
{
    FadeOutPaletteToBlack();
    SET_DB(DS_LastScanCode, 0);
    SET_DB(DS_EndSignal, 0);
}

void ShowWorldMapScrollIn(void)
{
    DisableVideoOutput();
    ActivateResourceSlot(2);
    PageInResource(5);
    uint16_t map = DW(DS_NextFreeSegment);
    uint16_t bx = (uint16_t)(DB(DS_CurrentLevel) * 4);
    uint16_t dx = (uint16_t)(DW((uint16_t)(LEVEL_TABLE_X + bx)) >> 3);
    uint16_t yy = (uint16_t)(DW((uint16_t)(LEVEL_TABLE_X + 2 + bx)) - 0x20);
    uint16_t di = (uint16_t)((uint8_t)yy * 0x50 + dx);   /* mov ah,50h; mul ah: row * 80 */

    uint16_t size = DW(DS_word_7522);
    uint8_t w8 = (uint8_t)((uint8_t)size >> 3);          /* width in bytes */
    uint8_t rows = (uint8_t)(size >> 8);
    uint16_t si = DW(DS_word_62DA);
    uint16_t mseg = DW(DS_word_667A);
    uint16_t bp = (uint16_t)(0x50 - w8);
    uint16_t n = (uint16_t)(w8 * rows);                  /* bytes per marker plane */
    uint8_t dh = rows;
    /* stamp: rows (dec dh, jne: 256 rows when dh is 0), each w8 columns (LOOP: 65536 when 0) */
    do {
        uint16_t cx = w8;
        do {
            uint8_t inv = (uint8_t)~rd8(mseg, si);
            wr8(map, di, (uint8_t)(rd8(map, di) & inv));
            wr8(map, (uint16_t)(di + 0x3E80), (uint8_t)(rd8(map, (uint16_t)(di + 0x3E80)) & inv));
            wr8(map, (uint16_t)(di + 0x7D00), (uint8_t)(rd8(map, (uint16_t)(di + 0x7D00)) & inv));
            wr8(map, (uint16_t)(di - 0x4480), (uint8_t)(rd8(map, (uint16_t)(di - 0x4480)) & inv));
            uint8_t al = rd8(mseg, (uint16_t)(n + si));
            wr8(map, di, (uint8_t)(rd8(map, di) | al));
            si = (uint16_t)(si + n);
            al = rd8(mseg, (uint16_t)(n + si));
            wr8(map, (uint16_t)(di + 0x3E80), (uint8_t)(rd8(map, (uint16_t)(di + 0x3E80)) | al));
            si = (uint16_t)(si + n);
            al = rd8(mseg, (uint16_t)(n + si));
            wr8(map, (uint16_t)(di + 0x7D00), (uint8_t)(rd8(map, (uint16_t)(di + 0x7D00)) | al));
            si = (uint16_t)(si + n);
            al = rd8(mseg, (uint16_t)(n + si));
            wr8(map, (uint16_t)(di - 0x4480), (uint8_t)(rd8(map, (uint16_t)(di - 0x4480)) | al));
            si = (uint16_t)(si - 3 * n);
            si = (uint16_t)(si + 1);
            di = (uint16_t)(di + 1);
        } while (--cx != 0);
        di = (uint16_t)(di + bp);
        dh--;
    } while (dh != 0);

    SET_DW(DS_MapScrollY, 0);
    SET_DW(DS_PrevMapScrollX, 0);
    SET_DW(DS_PrevMapScrollY, 0);
    SET_DW(DS_MapScrollX, 8);
    bios_set_dac_block(0, 0x10, DATA_SEG, 0xB0E8);
    EnableVideoOutput();
    SetCpuWriteMode();
    port_out16(0x3D4, 0x2411);
    port_out16(0x3D4, 0xE217);
    port_out16(0x3D4, 0x2601);
    port_out8(0x3C4, 2);
    loc_9613();
}

/* ---- MODE and ENTER CODE map screen (cs:96D5) -------------------------------------------- */

/* Frame loop (loc_97A8): retrace start, CRTC start, pel pan, the callback, the column and
 * row copies, exit on ScreenExitRequested. Ends with the fade tail loc_9881. */
static int loc_97A8(void);
static void loc_9881(void);

int RunCallbackMapScreen(uint16_t callback)
{
    SET_DW(DS_FrameCallback, callback);
    int st = ActivateResourceSlot(1);
    if (st) return st;
    DisableVideoOutput();
    st = PageInResource(6);
    if (st) return st;
    uint16_t map = DW(DS_NextFreeSegment);

    /* planes 0 and 1 (map mask 1, 2; the loop stops after 2) from the map buffer to VRAM */
    SetCpuWriteMode();
    port_out16(0x3D4, 0x2411);
    port_out16(0x3D4, 0xE217);
    port_out16(0x3D4, 0x2601);
    port_out8(0x3C4, 2);
    uint16_t si = 0;
    for (uint8_t al = 1; al < 4; al <<= 1) {
        port_out8(0x3C5, al);
        uint16_t di = 0;
        for (uint16_t cx = 0xFA0; cx != 0; cx--) {
            mw_wr(SEG_VGA, di, rd16(map, si));
            si = (uint16_t)(si + 2);
            di = (uint16_t)(di + 2);
        }
    }

    /* eight pixel-shifted tables of the font (word_003D segment) into the map buffer: each
     * of 0x560 words is byte-swapped, shifted right by the phase, and stored with the bits
     * shifted out (one byte); 30h zero words follow each table */
    uint16_t src = DW(DS_word_003D);
    uint16_t di = 0;
    for (uint8_t dh = 0; dh < 8; dh++) {
        uint16_t sp = 0x1FE0;
        for (uint16_t cx = 0x560; cx != 0; cx--) {
            uint16_t ax = rd16(src, sp);
            sp = (uint16_t)(sp + 2);
            ax = (uint16_t)(ax << 8 | ax >> 8);                    /* xchg al, ah */
            uint8_t dl = 0;
            uint8_t bl = dh;
            while (bl != 0) {                                       /* shr ax,1 / rcr dl,1 */
                uint8_t cf = (uint8_t)(ax & 1);
                ax = (uint16_t)(ax >> 1);
                dl = (uint8_t)((dl >> 1) | (cf << 7));
                bl--;
            }
            ax = (uint16_t)(ax << 8 | ax >> 8);                    /* xchg al, ah */
            wr16(map, di, ax);
            di = (uint16_t)(di + 2);
            wr8(map, di, dl);
            di = (uint16_t)(di + 1);
        }
        for (uint16_t cx = 0x30; cx != 0; cx--) {
            wr16(map, di, 0);
            di = (uint16_t)(di + 2);
        }
    }

    SET_DW(DS_MapScrollX, 0);
    SET_DW(DS_MapScrollY, 0);
    SET_DW(DS_PrevMapScrollX, 0);
    SET_DW(DS_PrevMapScrollY, 0);
    SET_DW(DS_CodeInputCount, 0);
    SET_DB(DS_ScreenExitRequested, 0);
    SET_DB(DS_CodeEntryState, 0);
    SET_DB(DS_TextAdvance, 4);
    SET_DB(DS_WavePhase, 2);
    SET_DB(DS_ScreenFirstFrame, 1);
    bios_set_dac_block(0, 0x10, DATA_SEG, 0xB118);
    return loc_97A8();
}

static int loc_97A8(void)
{
    for (;;) {
        uint16_t ax = (uint16_t)(0x28 * DW(DS_MapScrollY));
        uint16_t bx = (uint16_t)((DW(DS_MapScrollX) >> 3) + ax);
        bx &= 0x1FFF;
        port_out16(0x3D4, (uint16_t)((bx & 0xFF00) | 0x0C));
        port_out16(0x3D4, (uint16_t)(((bx & 0xFF) << 8) | 0x0D));
        uint16_t old = DW(DS_ScreenStartOffset);
        SET_DW(DS_ScreenStartOffset, bx);
        SET_DW(DS_PrevScreenStartOffset, old);
        SetCpuWriteMode();
        WaitVerticalRetraceStart();
        port_out8(0x3C4, 2);
        uint8_t first = DB(DS_ScreenFirstFrame);
        SET_DB(DS_ScreenFirstFrame, 0);
        if (first == 0) {                                 /* the pel pan is skipped once */
            port_out8(0x3C0, ATTR_PAS_PEL);
            port_out8(0x3C0, (uint8_t)(DB(DS_MapScrollX) & 7));
        }
        int st = call_frame_callback();                   /* BP = 1FFFh from AdvanceMapFrame */
        if (st) return st;
        const uint16_t bp = 0x1FFF;
        SetLatchCopyMode();
        if (((DB(DS_PrevMapScrollX) & 8)) != ((DB(DS_MapScrollX) & 8))) {
            uint16_t si = (uint16_t)(DW(DS_ScreenStartOffset) + 0x27);
            uint16_t di = (uint16_t)(si - 0x28);
            uint16_t cx = 0xC8;
            do {                                           /* column copy VRAM -> VRAM */
                si &= bp;
                di &= bp;
                mb_wr(SEG_VGA, di, mb_rd(SEG_VGA, si));
                si = (uint16_t)(si + 1 + 0x27);
                di = (uint16_t)(di + 1 + 0x27);
            } while (--cx != 0);
        }
        uint16_t dx = (uint16_t)(DW(DS_MapScrollY) - DW(DS_PrevMapScrollY));
        if (dx != 0) {
            uint16_t si, di, cnt;
            if (!(dx & 0x8000)) {                          /* positive: copy 199 rows down */
                si = (uint16_t)(DW(DS_ScreenStartOffset) - 1);
                di = (uint16_t)(si + 0x1F18);
            } else {                                       /* negative: copy up */
                dx = (uint16_t)(-dx);
                si = (uint16_t)(DW(DS_ScreenStartOffset) - 1);
                si = (uint16_t)(si - (uint16_t)(ROW_PITCH * (uint8_t)dx));
                di = si;
                si = (uint16_t)(si + 0x1F18);
            }
            cnt = (uint16_t)(0x29 * (uint8_t)dx);      /* LOOP with CX = 0 runs 65536 times */
            do {
                si &= bp;
                di &= bp;
                mb_wr(SEG_VGA, di, mb_rd(SEG_VGA, si));
                si = (uint16_t)(si + 1);
                di = (uint16_t)(di + 1);
            } while (--cnt != 0);
        }
        if (DB(DS_ScreenExitRequested) != 0) break;
    }
    loc_9881();
    return SCR_OK;
}

/* Exit tail (loc_9881): fade the palette out. */
static void loc_9881(void)
{
    FadeOutPaletteToBlack();
}

/* Shared by the callbacks: the map scroll step, then the wave term (cs:cvar_0005 test). */
void AdvanceMapFrame(void)
{
    uint16_t old = DW(DS_MapScrollX);
    SET_DW(DS_MapScrollX, (uint16_t)(old - 4));
    SET_DW(DS_PrevMapScrollX, old);
    uint8_t phase = (uint8_t)(DB(DS_WavePhase) + 1);
    SET_DB(DS_WavePhase, phase);
    if (CB(CS_CpuClass) != 0) {
        int16_t d = (int8_t)rd8(DATA_SEG, (uint16_t)(0x6F90 + phase));   /* xlat; cbw */
        d = (int16_t)(d >> 4);                                             /* sar ax,4 */
        uint16_t oldy = DW(DS_MapScrollY);
        SET_DW(DS_MapScrollY, (uint16_t)(d + oldy));
        SET_DW(DS_PrevMapScrollY, oldy);
    }
    SET_DW(DS_TextFontBank, (uint16_t)((old & 7) * FONT_STRIDE));
}

/* MODE callback: draws "MODE" and the choice; the arrows toggle ExpertFlag, fire or confirm
 * exits. Break codes are ignored; other make codes are consumed before the arrow test. */
void UpdateModeSelectFrame(void)
{
    AdvanceMapFrame();
    SET_DB(DS_TextAdvance, 4);
    SET_DW(DS_TextCursorOffset, 0x0C38);
    DrawFontString(0xB180, 0x1FFF);
    SET_DW(DS_TextCursorOffset, 0x1185);
    SET_DB(DS_TextAdvance, 3);
    DrawFontString(DB(DS_ExpertFlag) == 0 ? 0xB185 : 0xB18E, 0x1FFF);
    uint8_t bl = DB(DS_LastScanCode);
    if (bl & 0x80) return;
    if ((DB(DS_byte_282D) | DB(DS_byte_2810)) != 0) {
        SET_DB(DS_ScreenExitRequested, (uint8_t)(DB(DS_ScreenExitRequested) + 1));
        return;
    }
    SET_DB(DS_LastScanCode, 0);
    if (bl == 0x48 || bl == 0x4B || bl == 0x4D || bl == 0x50)
        SET_DB(DS_ExpertFlag, DB(DS_ExpertFlag) ^ 1);
}

/* ENTER CODE callback. The scan table gives the character of a make code; each digit adds
 * its nibble to CodeValue (the four nibbles of the code). The table bytes are followed: the
 * ASM comment above UpdateCodeEntryFrame names 2Ch 'C' and 2Eh 'B', but the table holds 2Ch
 * '-' (ignored), 2Eh 'C' and 30h 'B'. */
static void loc_9AB4(void);
static void loc_9AC4(void);
static void loc_9AC8(void);
static void loc_9A27(void);
static void loc_9A9B(uint16_t dx);

void UpdateCodeEntryFrame(void)
{
    AdvanceMapFrame();
    SET_DB(DS_TextAdvance, 3);
    SET_DW(DS_TextCursorOffset, 0x0AF2);
    DrawFontString(0xB175, 0x1FFF);
    SET_DB(DS_TextAdvance, 4);
    SET_DW(DS_TextCursorOffset, 0x12C9);
    DrawFontString(CODE_BOX, 0x1FFF);

    uint8_t st = DB(DS_CodeEntryState);
    if (st == 1) { loc_9AB4(); return; }
    if (st == 2) { loc_9AC4(); return; }
    uint8_t bl = DB(DS_LastScanCode);
    if (bl & 0x80) return;
    if ((DB(DS_byte_282D) | DB(DS_byte_2810)) != 0) { loc_9AC4(); return; }
    SET_DB(DS_LastScanCode, 0);
    uint8_t al = DB((uint16_t)(SCAN_TABLE + bl));
    if (al == 0x2D) return;                                  /* '-': ignored key */

    uint8_t ah = (uint8_t)(al - 0x30);
    if (ah > 9) ah = (uint8_t)(ah - 7);
    uint16_t dx = DW(DS_CodeValue);
    ah = (uint8_t)(ah << 4);                                 /* shl ah, cl (cl = 4) */
    for (int k = 0; k < 4; k++) {                            /* shl ah,1 / rcl dx,1 */
        uint8_t cf = (uint8_t)(ah >> 7);
        ah = (uint8_t)(ah << 1);
        dx = (uint16_t)((dx << 1) | cf);
    }
    SET_DW(DS_CodeValue, dx);
    uint16_t cnt = DW(DS_CodeInputCount);
    SET_DB((uint16_t)(CODE_BOX + cnt), al);
    SET_DW(DS_CodeInputCount, (uint16_t)(cnt + 1));
    if ((uint8_t)(cnt + 1) < 4) return;
    loc_9A27();
}

/* Timer for a wrong code: after 8Ch frames the boxes reset (state 1). */
static void loc_9AB4(void)
{
    uint16_t t = (uint16_t)(DW(DS_CodeErrorTimer) + 1);
    SET_DW(DS_CodeErrorTimer, t);
    if (t < 0x8C) return;
    loc_9AC8();
}

/* Cancel or accepted code (state 2): one more exit request, then the boxes reset. */
static void loc_9AC4(void)
{
    SET_DB(DS_ScreenExitRequested, (uint8_t)(DB(DS_ScreenExitRequested) + 1));
    loc_9AC8();
}

static void loc_9AC8(void)
{
    SET_DW(CODE_BOX, 0x5B5B);
    SET_DW((uint16_t)(CODE_BOX + 2), 0x5B5B);
    SET_DB(DS_CodeEntryState, 0);
    SET_DW(DS_CodeInputCount, 0);
}

/* Check of a complete code: the history hash (accepts levels 0..12h through CodeValue-1
 * when CodeHistory0..2 hash to the constants), else the table of ComputePasswordCode(0..12h).
 * A miss shifts the history and sets state 1. */
static void loc_9A27(void)
{
    SET_DW(DS_CodeErrorTimer, 0);
    uint16_t di = DW(DS_CodeHistory0), si = DW(DS_CodeHistory1), bp = DW(DS_CodeHistory2);
    uint16_t ax = rol16(di, 2);
    ax = (uint16_t)((uint8_t)ax * (uint8_t)(ax >> 8));       /* mul ah: AX = AL * AH */
    ax ^= si;
    uint32_t prod = (uint32_t)ax * bp;                        /* mul bp: DX:AX */
    ax = (uint16_t)prod;
    uint16_t dx = (uint16_t)(prod >> 16);
    uint16_t bx = ror16((uint16_t)(ax + si), 1);
    uint8_t bh = (uint8_t)(bx >> 8);
    uint8_t bl = (uint8_t)(rol8((uint8_t)bx, 1) ^ bh);      /* rol bl,1; xor bl,bh */
    bx = (uint16_t)(bh << 8 | bl);
    bx ^= bp;
    if (ax == 0x36C8 && bx == 0x8BD1 && dx == 0x8E71) {
        dx = (uint16_t)(DW(DS_CodeValue) - 1);
        if (dx <= 0x13) { loc_9A9B(dx); return; }
    }
    for (dx = 0; dx <= 0x12; dx++) {
        if (ComputePasswordCode(dx) == DW(DS_CodeValue)) { loc_9A9B(dx); return; }
    }
    SET_DW(DS_CodeHistory0, DW(DS_CodeHistory1));
    SET_DW(DS_CodeHistory1, DW(DS_CodeHistory2));
    SET_DW(DS_CodeHistory2, DW(DS_CodeValue));
    SET_DB(DS_CodeEntryState, 1);
}

/* Accepted level DX: CurrentLevel = DX mod 10, ExpertFlag = DX >= 10 (state 2). */
static void loc_9A9B(uint16_t dx)
{
    uint8_t dl = (uint8_t)dx, expert = 0;
    if (dl >= 0x0A) { dl = (uint8_t)(dl - 0x0A); expert = 1; }
    SET_DB(DS_CurrentLevel, dl);
    SET_DB(DS_ExpertFlag, expert);
    SET_DB(DS_CodeEntryState, 2);
}

/* DrawFontString (cs:9886): each character of the string at DS:bx is drawn with the glyph
 * record (60h bytes) of font segment word_2875, erasing the previous cell first (planes 2 and
 * 3, three bytes x 12 rows) and drawing planes 2 then 3 from the shifted font. bp is the
 * VRAM offset mask (1FFFh). Returns BX after the terminator. */
uint16_t DrawFontString(uint16_t bx, uint16_t bp)
{
    for (;;) {
        uint8_t c = rd8(DATA_SEG, bx);
        bx = (uint16_t)(bx + 1);
        uint8_t al;
        if (c == 0x20) {
            al = 0x2B;                                       /* space: glyph 2Bh */
        } else {
            if (c < 0x30) return bx;                         /* terminator: NUL, '-' ... */
            al = (uint8_t)(c - 0x30);
            if (al > 9) al = (uint8_t)(al - 7);
        }
        uint16_t dx = DB(DS_TextAdvance);
        uint16_t si = (uint16_t)((uint16_t)al * 0x60 + DW(DS_TextFontBank) + 6);
        uint16_t cursor = (uint16_t)(DW(DS_TextCursorOffset) + dx);
        SET_DW(DS_TextCursorOffset, cursor);
        uint16_t di = (uint16_t)(cursor + 0x50);
        uint16_t draw = (uint16_t)(di + DW(DS_ScreenStartOffset));
        di = (uint16_t)(di + DW(DS_PrevScreenStartOffset));
        uint16_t fseg = DW(DS_NextFreeSegment);
        port_out8(0x3C5, 0x0C);
        for (int r = 0; r < 12; r++) {                       /* erase: 3 bytes per row */
            di &= bp;
            mw_wr(SEG_VGA, di, 0);
            mb_wr(SEG_VGA, (uint16_t)(di + 2), 0);
            di = (uint16_t)(di + 3 + 0x25);
        }
        di = draw;
        for (uint8_t plane = 4; plane < 0x10; plane <<= 1) {
            port_out8(0x3C5, plane);
            for (int r = 0; r < 12; r++) {                   /* draw: movsw then movsb per row */
                di &= bp;
                uint16_t w = rd16(fseg, si);
                si = (uint16_t)(si + 2);
                mw_wr(SEG_VGA, di, w);
                di = (uint16_t)(di + 2);
                mb_wr(SEG_VGA, di, rd8(fseg, si));
                si = (uint16_t)(si + 1);
                di = (uint16_t)(di + 1 + 0x25);
            }
            si = (uint16_t)(si + 0x0C);
            di = (uint16_t)(di - 0x1E0);
        }
    }
}

void WaitVerticalRetraceStart(void)
{
    while (!(port_in8(0x3DA) & 0x08)) wait_poll();
}

void WaitNextVerticalRetrace(void)
{
    while (port_in8(0x3DA) & 0x08) wait_poll();          /* first read clear: retrace ended */
    if (!(port_in8(0x3DA) & 0x08)) WaitVerticalRetraceStart();
}

/* ---- GAME OVER screen (cs:9B23) ---------------------------------------------------------- */

static void loc_9C62(void);

void RunGameOverScreen(void)
{
    if (DB(DS_byte_2879) != 0) return;                   /* gameplay: nothing to show */
    SET_DW(DS_CameraCol, 0);
    SET_DW(DS_CameraRow, 0);
    SET_DW(DS_ShakeRows, 0);
    SET_DB(DS_FineScroll, 0);
    PageInResource(0x0B);
    uint16_t map = DW(DS_NextFreeSegment);
    /* clear 8000 bytes at 3F40h in all planes, then copy the four planes; DI is not reset
     * after the clear, so the planes land at 5E80h (the 3F40h backdrop stays clear) */
    port_out8(0x3C4, 2);
    port_out8(0x3C5, 0x0F);
    uint16_t di = 0x3F40, si = 0;
    for (uint16_t cx = 0xFA0; cx != 0; cx--) {
        mw_wr(SEG_VGA, di, 0);
        di = (uint16_t)(di + 2);
    }
    for (uint8_t al = 1; al < 0x10; al <<= 1) {
        port_out8(0x3C5, al);
        for (uint16_t cx = 0xFA0; cx != 0; cx--) {
            mw_wr(SEG_VGA, di, rd16(map, si));
            si = (uint16_t)(si + 2);
            di = (uint16_t)(di + 2);
        }
        di = (uint16_t)(di - 0x1F40);
    }
    bios_set_dac_block(0, 0x10, DATA_SEG, 0xAFE8);

    /* object records: 116 inactive, 8 letters, 3 items, 2 archive records */
    for (uint16_t k = 0; k < 0x74; k++) SET_DW((uint16_t)(0x4F0A + 0x12 * k + 4), 0xFFFF);
    uint16_t p = 0x4F1C;
    uint16_t sx = 0x2C, sy = 0xE0;
    uint16_t text = 0xB018;
    for (int k = 0; k < 4; k++) {                        /* "GAMEOVER" letters, first four */
        uint8_t ch = rd8(DATA_SEG, text++);
        SET_DW((uint16_t)(p + 4), (uint16_t)(ch + 0xB0));
        SET_DW(p, sx);
        SET_DW((uint16_t)(p + 2), sy);
        uint16_t r = NextWindRandom();
        r &= 7;
        if (r == 0) r = 1;
        SET_DW((uint16_t)(p + 0x0E), r);
        sx = (uint16_t)(sx + 0x18);
        p = (uint16_t)(p + 0x12);
    }
    sx = (uint16_t)(sx + 0x30);
    for (int k = 0; k < 4; k++) {                        /* the other four letters */
        uint8_t ch = rd8(DATA_SEG, text++);
        SET_DW((uint16_t)(p + 4), (uint16_t)(ch + 0xB0));
        SET_DW(p, sx);
        SET_DW((uint16_t)(p + 2), sy);
        uint16_t r = NextWindRandom() & 7;
        SET_DW((uint16_t)(p + 0x0E), r);
        sx = (uint16_t)(sx + 0x18);
        p = (uint16_t)(p + 0x12);
    }
    p = (uint16_t)(p + 0xB4);
    SET_DW(p, 0x96);
    SET_DW((uint16_t)(p + 2), 0x135);
    SET_DW((uint16_t)(p + 4), 0x64);
    p = (uint16_t)(p + 0x12);
    SET_DW(p, 0x97);
    SET_DW((uint16_t)(p + 2), 0x108);
    SET_DW((uint16_t)(p + 4), 0x1C4);
    p = (uint16_t)(p + 0x12);
    SET_DW(p, 0x96);
    SET_DW((uint16_t)(p + 2), 0x121);
    SET_DW((uint16_t)(p + 4), 0x68);
    p = (uint16_t)(p + 0x12);
    SET_DW(p, 0x96);
    SET_DW((uint16_t)(p + 2), 0x119);
    SET_DW((uint16_t)(p + 4), 0x62);
    p = 0x5138;
    SET_DB((uint16_t)(p + 8), 0);
    SET_DW((uint16_t)(p + 0x0C), 0xB020);
    p = (uint16_t)(p + 0x12);
    SET_DB((uint16_t)(p + 8), 0x55);
    SET_DW((uint16_t)(p + 0x0C), 0xB032);
    p = (uint16_t)(p + 0x12);
    SET_DB((uint16_t)(p + 8), 0xAA);
    SET_DW((uint16_t)(p + 0x0C), 0xB054);
    SET_DW(DS_word_27F0, 0);
    loc_9C62();
}

/* Frame loop (loc_9C62) and the fade tail (loc_9C83). */
static void loc_9C62(void)
{
    for (;;) {
        UpdateGameOverSprites();
        CopyBackdropStrip();
        DrawSpriteObjects();
        PresentFrame();
        PollInputState();
        WaitNextVerticalRetrace();
        if (DW(DS_word_27F0) >= 0x276) break;
        if (DB(DS_byte_27E8) != 0) break;
    }
    FadeOutPaletteToBlack();
}

/* Copies the 176-row backdrop window (7040 bytes) from 3F40h + 28h * FineScroll to DrawPage,
 * all four planes at once (latch copy). */
void CopyBackdropStrip(void)
{
    SetLatchCopyMode();
    uint16_t di = DW(DS_DrawPage);
    port_out16(0x3C4, 0x0F02);
    uint16_t si = (uint16_t)(ROW_PITCH * DB(DS_FineScroll) + 0x3F40);
    for (uint16_t cx = 0x1B80; cx != 0; cx--) {
        mb_wr(SEG_VGA, di, mb_rd(SEG_VGA, si));
        si = (uint16_t)(si + 1);
        di = (uint16_t)(di + 1);
    }
}

/* One animation step: FineScroll, the eight letters (sway velocity), the three items on
 * sine tables with path scripts, a bubble sort of the items by y, and the archive copy of
 * the items below 135h... (see the note on the archive records in screens.c). */
void UpdateGameOverSprites(void)
{
    if (DB(DS_FineScroll) < 0xB9) SET_DB(DS_FineScroll, (uint8_t)(DB(DS_FineScroll) + 1));
    if ((DB(DS_SnowFrameTick) & 3) == 0) {
        uint16_t ax = (uint16_t)((DW(DS_word_5088) & 0x1FFF) + 1);
        if (ax >= 0x6E) ax = 0x68;
        SET_DW(DS_word_5088, ax);
    }
    uint16_t si = 0x4F1C;
    for (int k = 0; k < 8; k++, si = (uint16_t)(si + 0x12)) {
        uint16_t ax = (uint16_t)(DW((uint16_t)(si + 0x0E)) + 1);
        if ((int8_t)(uint8_t)ax >= 8) ax = (uint16_t)(-ax + 1);
        SET_DW((uint16_t)(si + 0x0E), ax);
        SET_DW((uint16_t)(si + 2), (uint16_t)(DW((uint16_t)(si + 2)) + ax));
    }
    si = 0x5138;
    for (int k = 0; k < 3; k++, si = (uint16_t)(si + 0x12)) {
        SET_DW((uint16_t)(si - 0x164), 0xFFFF);
        int16_t v = (int8_t)rd8(DATA_SEG, (uint16_t)(0x6F90 + DB((uint16_t)(si + 8))));
        int32_t p = (int32_t)v * 0x41;                      /* imul dx: DX:AX */
        p >>= 6;                                            /* six sar/rcr pairs */
        SET_DW(si, (uint16_t)((uint16_t)p + 0x96));
        v = (int8_t)rd8(DATA_SEG, (uint16_t)(0x7090 + DB((uint16_t)(si + 8))));
        p = (int32_t)v * 0x0A;
        p >>= 6;
        uint16_t ay = (uint16_t)p;
        SET_DW((uint16_t)(si + 6), ay);
        SET_DW((uint16_t)(si + 2), (uint16_t)(ay + 0x135));
        SET_DB((uint16_t)(si + 8), (uint8_t)(DB((uint16_t)(si + 8)) + 2));
        uint16_t bx = DW((uint16_t)(si + 0x0C));
        uint16_t ax;
        for (;;) {                                          /* negative word: relative jump */
            ax = DW(bx);
            if (!(ax & 0x8000)) break;
            bx = (uint16_t)(bx + ax);
        }
        ax = (uint16_t)(ax + 0x138);
        ax |= (uint16_t)(DW((uint16_t)(si + 6)) & 0x8000);
        SET_DW((uint16_t)(si + 4), ax);
        bx = (uint16_t)(bx + 2);
        SET_DW((uint16_t)(si + 0x0C), bx);
    }
    /* bubble until sorted: the item after is swapped in when its y is not below */
    for (;;) {
        uint16_t s = 0x5138;
        int swapped = 0;
        for (int cx = 2; cx != 0; cx--, s = (uint16_t)(s + 0x12)) {
            uint16_t d = (uint16_t)(s + 0x12);
            if ((int16_t)DW((uint16_t)(d + 2)) >= (int16_t)DW((uint16_t)(s + 2))) {
                SwapSpriteRecords(s, d);
                swapped = 1;
                break;
            }
        }
        if (!swapped) break;
    }
    /* items with y >= 135h are copied to the archive records and deactivated */
    uint16_t dst = 0x4FD0;
    si = 0x5138;
    for (int cx = 2; cx != 0; cx--) {
        if ((int16_t)DW((uint16_t)(si + 2)) >= 0x135) {
            CopySpriteRecord(si, dst);
            SET_DW((uint16_t)(si + 4), 0xFFFF);
        }
        si = (uint16_t)(si + 0x12);
        dst = (uint16_t)(dst + 0x12);
    }
}

void SwapSpriteRecords(uint16_t si, uint16_t di)
{
    for (int k = 0; k < 0x12; k++) {
        uint8_t a = rd8(DATA_SEG, (uint16_t)(si + k));
        uint8_t b = rd8(DATA_SEG, (uint16_t)(di + k));
        wr8(DATA_SEG, (uint16_t)(si + k), b);
        wr8(DATA_SEG, (uint16_t)(di + k), a);
    }
}

void CopySpriteRecord(uint16_t si, uint16_t di)
{
    for (int k = 0; k < 0x12; k++) wr8(DATA_SEG, (uint16_t)(di + k), rd8(DATA_SEG, (uint16_t)(si + k)));
}

/* ---- front-end flow (cs:8E45) ----------------------------------------------------------- */

/* Fade-in passes (loc_91CF) over the palette at segment `pal` (entries 0..count-1, the
 * target is the 6-bit palette at pal:0), with OptionBits bit 0 clear the instant 160-entry
 * write (loc_925E). Returns BX: BH:BL as left by the last entry read. */
static uint16_t fade_in_passes(uint16_t pal, uint16_t bx);

uint16_t SetPictureCopyPointers(void)
{
    return (uint16_t)(DW(DS_LevelSegment) + 0x30);      /* DS: picture bytes; SI = 0, ES = A000h, DI = 0 */
}

uint16_t ShowMode13PictureFadeIn(uint8_t count, uint16_t bx)
{
    SET_CB(CS_FadeEntryCount, count);
    bios_set_mode(0x13);
    WaitVerticalRetrace();
    port_out8(0x3C8, 0);
    for (int k = 0; k < 0x300; k++) port_out8(0x3C9, 0);
    uint16_t src = SetPictureCopyPointers();
    for (uint32_t k = 0; k < 0x7D00; k++) {             /* rep movsw: 0FA00h bytes */
        uint16_t off = (uint16_t)(2 * k);
        mw_wr(SEG_VGA, off, rd16(src, off));
    }
    WaitVerticalRetrace();
    EnableVideoOutput();
    return fade_in_passes(DW(DS_LevelSegment), bx);
}

static uint16_t fade_in_passes(uint16_t pal, uint16_t bx)
{
    uint8_t bl = (uint8_t)bx, bh = (uint8_t)(bx >> 8);
    for (;;) {                                           /* loc_91CF: one pass */
        uint16_t si = 0, bp = 0;
        SET_CB(CS_FadeEntryIndex, 0);
        for (;;) {                                       /* loc_91D9: one entry */
            if ((CB(CS_FadeEntryIndex) & 0x1F) == 0) WaitVerticalRetrace();
            if (!(CW(CS_OptionBits) & 1)) {              /* loc_925E: instant write */
                port_out8(0x3C8, 0);
                for (int k = 0; k < 0x1E0; k++) port_out8(0x3C9, rd8(pal, (uint16_t)(si + k)));
                return (uint16_t)(bh << 8 | bl);
            }
            uint8_t idx = CB(CS_FadeEntryIndex);
            port_out8(0x3C7, idx);
            uint8_t red = (uint8_t)(port_in8(0x3C9) & 0x3F);
            uint8_t green = (uint8_t)(port_in8(0x3C9) & 0x3F);
            uint8_t blue = (uint8_t)(port_in8(0x3C9) & 0x3F);
            bl = green;
            bh = blue;
            port_out8(0x3C8, idx);
            uint8_t comp[3] = { red, green, blue };
            for (int c = 0; c < 3; c++) {
                uint8_t v = comp[c];
                if (rd8(pal, (uint16_t)(si + c)) > v && v <= 0x3D) {   /* step +2 toward target */
                    v = (uint8_t)(v + 2);
                    bp++;
                }
                port_out8(0x3C9, v);
            }
            si = (uint16_t)(si + 3);
            SET_CB(CS_FadeEntryIndex, (uint8_t)(CB(CS_FadeEntryIndex) + 1));
            if (CB(CS_FadeEntryIndex) >= CB(CS_FadeEntryCount)) break;
        }
        if (bp == 0) return (uint16_t)(bh << 8 | bl);
    }
}

void ShowPlanarPicture(uint16_t pic)
{
    SetLevelVideoMode();
    WaitVerticalRetrace();
    port_out8(0x3C8, 0);
    for (int k = 0; k < 0x300; k++) port_out8(0x3C9, 0);
    port_out8(0x3C4, 2);
    uint16_t si = 0;
    for (uint8_t al = 1; al < 0x10; al <<= 1) {        /* four planes, 8000 bytes each */
        port_out8(0x3C5, al);
        uint16_t di = 0;
        for (uint16_t cx = 0xFA0; cx != 0; cx--) {
            mw_wr(SEG_VGA, di, rd16(pic, si));
            si = (uint16_t)(si + 2);
            di = (uint16_t)(di + 2);
        }
    }
    for (uint16_t k = 0; k < 0x2D0; k++) wr8(pic, (uint16_t)(0x7D30 + k), 0);
    fade_in_passes((uint16_t)(pic + 0x7D0), 0);
}

/* Shows the PRESENT title (name at DATA:A2F8h): morph of the palette to the target at
 * DATA:ACE7h, by 2 steps per component, a retrace every 82 writes (BL counter continuous
 * across passes), passes while a component snapped. */
int ShowPresentTitle(void)
{
    uint16_t seg = 0;
    int st = LoadSqzFile(0xA2F8, &seg);
    if (st) return st;
    SET_DW(DS_NextFreeSegment, seg);
    SET_DW(DS_LevelSegment, seg);
    uint16_t bx = ShowMode13PictureFadeIn(0xFF, 0);
    uint16_t logo = (uint16_t)(seg + 0x1030);            /* 3A20h words: 320 x 93 strip */
    for (uint32_t k = 0; k < 0x3A20; k++) {
        uint16_t off = (uint16_t)(2 * k);
        mw_wr(SEG_VGA, off, rd16(logo, off));
    }
    uint16_t cur = seg;
    for (int k = 0; k < 0x300; k++) {                    /* 6-bit palette of the screen */
        if (k == 0) port_out8(0x3C7, 0);
        wr8(cur, (uint16_t)k, (uint8_t)(port_in8(0x3C9) & 0x3F));
    }
    uint8_t bl = (uint8_t)bx;
    uint16_t bp;
    do {                                                 /* loc_90DE: one pass */
        port_out8(0x3C8, 0);
        bp = 0;
        for (uint16_t k = 0; k < 0x300; k++) {
            uint8_t target = rd8(DATA_SEG, (uint16_t)(0xACE7 + k));
            uint8_t curv = rd8(cur, k);
            uint8_t d = (uint8_t)(target - curv);
            uint8_t out = curv;
            if (d != 0) {
                uint8_t ad = (d & 0x80) ? (uint8_t)(-d) : d;
                if (ad < 2) {                            /* one step: snap to the target */
                    bp++;
                    out = target;
                } else {
                    out = (uint8_t)(target > curv ? curv + 2 : curv - 2);
                }
            }
            wr8(cur, k, out);
            port_out8(0x3C9, out);
            bl++;
            if (bl >= 0x52) {
                WaitVerticalRetrace();
                bl = 0;
            }
        }
    } while (bp != 0);
    return SCR_OK;
}

/* TITUS title (name at DATA:A304h): 16-entry fade-in, 70 retraces, fade out. */
int ShowTitusScreen(void)
{
    uint16_t seg = 0;
    int st = LoadSqzFile(0xA304, &seg);
    if (st) return st;
    SET_DW(DS_NextFreeSegment, seg);
    SET_DW(DS_LevelSegment, seg);
    ShowMode13PictureFadeIn(0x10, 0);
    for (int k = 0; k < 0x46; k++) WaitVerticalRetrace();
    FadeOutPaletteToBlack();
    return SCR_OK;
}

/* Fades every component of the DAC toward zero by four per entry; the pass repeats while
 * any entry still had red nonzero, or green or blue above 4 (BP accumulates AH and AL). */
void FadeOutPaletteToBlack(void)
{
    if (!(CW(CS_OptionBits) & 1)) return;
    if (CB(CS_OptionVByte) != 5) return;
    for (;;) {
        uint16_t bp = 0;
        SET_CB(CS_FadeEntryIndex, 0);
        for (;;) {
            if ((CB(CS_FadeEntryIndex) & 0x1F) == 0) WaitVerticalRetrace();
            uint8_t idx = CB(CS_FadeEntryIndex);
            port_out8(0x3C7, idx);
            uint8_t red = (uint8_t)(port_in8(0x3C9) & 0x3F);
            uint8_t green = (uint8_t)(port_in8(0x3C9) & 0x3F);
            uint8_t blue = (uint8_t)(port_in8(0x3C9) & 0x3F);
            port_out8(0x3C8, idx);
            uint8_t comp[3] = { red, green, blue };
            for (int c = 0; c < 3; c++) {
                uint8_t res = comp[c] > 4 ? (uint8_t)(comp[c] - 4) : 0;
                bp |= (uint16_t)((uint16_t)red << 8) | res;
                port_out8(0x3C9, res);
            }
            SET_CB(CS_FadeEntryIndex, (uint8_t)(CB(CS_FadeEntryIndex) + 1));
            if (CB(CS_FadeEntryIndex) >= CB(CS_FadeEntryCount)) break;
        }
        if (bp == 0) return;
    }
}

/* Front-end flow. A MODE request runs the MODE map screen once and leaves through the exit
 * tail (loc_8EFD). An ENTER CODE request restarts the whole routine unless the code was
 * accepted (CurrentLevel >= 0); a key or timer then starts the level. */
static void loc_8EFD(void);
static void loc_8F29(void);
static int front_end_loop(int stop_before_intro);
static void level_start_tail(void);

#define FE_RESTART 0x7FFF        /* internal: the ENTER path jumps back to RunFrontEnd */

int RunFrontEnd(void)
{
    for (;;) {                                           /* restart point of the ENTER path */
        SET_DB(DS_byte_2879, 0);
        SET_DW(DS_word_287A, 0);
        SET_DB(DS_byte_287C, 0);
        SET_DB(DS_byte_287D, 0);
        uint16_t seg = 0;
        int st = page_in_seg(8, &seg);
        if (st) return st;
        SET_DW(DS_LevelSegment, seg);
        ShowMode13PictureFadeIn(0xA0, 0);
        SET_DW(DS_word_27F0, 0);
        SET_DW(DS_word_27F2, 0);
        st = front_end_loop(0);
        if (st != FE_RESTART) return st;
    }
}

/* The wait loop (loc_8E7B). With stop_before_intro the level start stops before the intro
 * scene (the tests enter at loc_8E7B and stop at the call of RunLevelIntroScene). */
static int front_end_loop(int stop_before_intro)
{
    for (;;) {                                           /* loc_8E7B */
        if (DB(DS_byte_27F6) != 0) {                     /* MODE request */
            SET_DB(DS_CurrentLevel, 0);
            SetLevelVideoMode();
            int st = RunCallbackMapScreen(CS_UpdateModeSelectFrame);
            if (st) return st;
            SET_DB(DS_CurrentLevel, 0);
            loc_8EFD();
            return SCR_OK;
        }
        if (DB(DS_byte_27F7) != 0) {                     /* ENTER CODE request */
            SET_DB(DS_CurrentLevel, 0);
            SetLevelVideoMode();
            SET_DB(DS_CurrentLevel, 0xFF);
            int st = RunCallbackMapScreen(CS_UpdateCodeEntryFrame);
            if (st) return st;
            if ((int8_t)DB(DS_CurrentLevel) < 0) return FE_RESTART;
            loc_8EFD();
            return SCR_OK;
        }
        if ((DB(DS_byte_282D) | DB(DS_byte_2810)) != 0) {  /* a key starts the MODE screen */
            SET_DB(DS_CurrentLevel, 0);
            SetLevelVideoMode();
            int st = RunCallbackMapScreen(CS_UpdateModeSelectFrame);
            if (st) return st;
            SET_DB(DS_CurrentLevel, 0);
            loc_8EFD();
            return SCR_OK;
        }
        if (DW(DS_word_27F0) >= 0x10E) break;            /* the level starts */
        platform_poll();
    }
    /* level start head: fade, picture 0Ch, planar picture */
    FadeOutPaletteToBlack();
    uint16_t lseg = 0;
    int st = page_in_seg(0x0C, &lseg);
    if (st) return st;
    SET_DW(DS_LevelSegment, lseg);
    ShowPlanarPicture(lseg);
    if (stop_before_intro) return SCR_OK;
    RunLevelIntroScene();
    level_start_tail();
    loc_8F29();
    return SCR_OK;
}

/* Level start after the intro (the instructions before loc_8F29): fade, the level picked on
 * the screen (083Eh, 083Dh) and byte_2879 = 1 for the demo log. */
static void level_start_tail(void)
{
    FadeOutPaletteToBlack();
    SET_DB(DS_byte_2879, 1);
    SET_DB(DS_CurrentLevel, DB(DS_byte_083E));
    SET_DB(DS_byte_B198, DB(DS_byte_083D));
}

/* Exit tail (loc_8EFD): 55AAh words over 003Fh..083Dh, then the level (083Eh) and the expert
 * flag (083Dh), and the CpuClass >= 3 word_78C4 setting. */
static void loc_8EFD(void)
{
    for (uint16_t k = 0; k < 0x400; k++) SET_DW((uint16_t)(0x3F + 2 * k), 0x55AA);
    SET_DB(DS_byte_083E, DB(DS_CurrentLevel));
    SET_DB(DS_byte_B198, DB(DS_ExpertFlag));
    SET_DB(DS_byte_083D, DB(DS_ExpertFlag));
    if (CB(CS_CpuClass) >= 3) SET_DW(DS_word_78C4, 0x78C6);
    FadeOutPaletteToBlack();
}

static void loc_8F29(void)
{
    FadeOutPaletteToBlack();
}

/* Level-start intro (cs:8F2D): backdrop save, walk-in (loc_8F96), walk-off (loc_9002). The
 * walk-in repeats while bit 20h of PlayerFrameHigh is set: DrawSpriteObjects sets it while
 * the player record is drawn, so the walk-in runs until the player leaves the screen. The
 * walk-off ends when PlayerX goes negative. */
static void loc_8F96(void);
static void loc_9002(void);
static void intro_setup(void);
static void intro_group_setup(void);

/* Scene set-up: backdrop save and the player and follower state (before the walk-in). */
static void intro_setup(void)
{
    SetLatchCopyMode();
    for (uint16_t k = 0; k < 0x1F40; k++) {             /* movsb: VRAM 0..1F3Fh -> 3F40h.. */
        uint8_t b = mb_rd(SEG_VGA, k);
        mb_wr(SEG_VGA, (uint16_t)(0x3F40 + k), b);
    }
    SET_DB(DS_FadeOutActive, 0);
    SET_DB(DS_FadeInActive, 1);
    SET_DB(DS_PaletteFadeStep, 0);
    SET_DB(DS_FineScroll, 0);
    SET_DW(DS_CameraCol, 0);
    SET_DW(DS_CameraRow, 0);
    SET_DW(DS_PlayerX, 0);
    SET_DW(DS_PlayerY, 0xA9);
    SET_DW(DS_PlayerFacing, 0);
    SET_DW(DS_PlayerVelX, 0);
    SET_DW(DS_PlayerAnimPtr, 0x7BA7);
    SET_DW(DS_word_4F2E, 0x30);
    SET_DW(DS_word_4F30, 0xA9);
    SET_DW(DS_word_4F37, 0);
    SET_DW(DS_word_4F34, 0);
    SET_DW(DS_word_4F3A, 0xAB4B);
    SET_DB(DS_PlayerAltModeFlag, 0);
}

void RunLevelIntroScene(void)
{
    intro_setup();
    loc_8F96();
    intro_group_setup();
    loc_9002();
}

/* Walk-in (loc_8F96): one frame per pass; the scene's backdrop and sprites are drawn and the
 * player walks 5 px (4 on every fourth frame) while the sprite group follows. */
/* One walk-in frame (the loc_8F96 body) with frame counter BP; returns the new BP. The
 * drawn flag of the player record (bit 20h of PlayerFrameHigh, set by DrawSpriteObjects while
 * the player is on screen) keeps the walk-in going: it runs until the player leaves. */
static uint16_t walk_in_frame(uint16_t bp)
{
    bp++;
    AdvancePlayerAnim(DW(DS_PlayerAnimPtr));
    StepSpriteAnimation(0x4F2E);
    RestoreBackdropToDrawPage();                        /* leaves DS = A000h (see RestoreBackdrop) */
    DrawSpriteObjects();
    vga_read(DS_byte_6BE7);                              /* PresentFrame's cmp ds:byte_6BE7 under DS = A000h */
    PresentFrame();
    uint16_t ax = 5;
    SET_DW(DS_PlayerX, (uint16_t)(DW(DS_PlayerX) + ax));
    if ((bp & 3) == 0) ax = 4;
    SET_DW(DS_word_4F2E, (uint16_t)(DW(DS_word_4F2E) + ax));
    return bp;
}

static void loc_8F96(void)
{
    uint16_t bp = 0;
    do {
        bp = walk_in_frame(bp);
    } while (DB(DS_PlayerFrameHigh) & 0x20);
}

/* Group set-up after the walk-in, up to the walk-off head loc_9002. */
static void intro_group_setup(void)
{
    /* rep movsw: 63 words forward from 4F2Eh to 4F30h (overlapping, so the 9-word group repeats) */
    SET_DW(DS_word_4F37, 0xFFFF);
    SET_DW(DS_word_4F3C, 0);
    SET_DW(DS_PlayerFacing, 0xFFFF);
    for (uint16_t k = 0; k < 0x3F; k++) {
        uint16_t src = (uint16_t)(0x4F2E + 2 * k);
        SET_DW((uint16_t)(src + 0x12), DW(src));
    }
    uint16_t ax = DW(DS_PlayerX), dx = DW(DS_PlayerY);
    uint16_t si = 0x4F2E;
    for (int k = 0; k < 3; k++, si = (uint16_t)(si + 0x12)) {
        ax = (uint16_t)(ax + 0x1E);
        dx = (uint16_t)(dx - 3);
        SET_DW(si, ax);
        SET_DW((uint16_t)(si + 2), dx);
    }
}

/* Walk-off (loc_9002): PlayerX -= 7 per frame until it is negative. */
static void loc_9002(void)
{
    do {
        AdvancePlayerAnim(DW(DS_PlayerAnimPtr));
        uint16_t si = 0x4F2E;
        for (int k = 0; k < 3; k++, si = (uint16_t)(si + 0x12)) {
            SET_DW(si, (uint16_t)(DW(si) - 5));
            StepSpriteAnimation(si);
            if (DW((uint16_t)(si + 2)) > 0xA9) SET_DW((uint16_t)(si + 0x0E), (uint16_t)(-DW((uint16_t)(si + 0x0E))));
            SET_DW((uint16_t)(si + 0x0E), (uint16_t)(DW((uint16_t)(si + 0x0E)) + 8));
            int16_t ay = (int16_t)DW((uint16_t)(si + 0x0E)) >> 4;
            SET_DW((uint16_t)(si + 2), (uint16_t)(DW((uint16_t)(si + 2)) + (uint16_t)ay));
        }
        RestoreBackdropToDrawPage();
        DrawSpriteObjects();
        vga_read(DS_byte_6BE7);                          /* the DS = A000h read, as in loc_8F96 */
        PresentFrame();
        SET_DW(DS_PlayerX, (uint16_t)(DW(DS_PlayerX) - 7));
    } while ((int16_t)DW(DS_PlayerX) >= 0);
}

/* Advances a render slot's animation script and writes its frame (si = render slot). */
void StepSpriteAnimation(uint16_t si)
{
    uint16_t bx = DW((uint16_t)(si + 0x0C));
    uint16_t ax = DW(bx);
    if (ax & 0x8000) {
        bx = (uint16_t)(bx + ax);
        ax = DW(bx);
    }
    uint8_t ah = (uint8_t)((ax >> 8) & 0x1F);
    ax = (uint16_t)((ah << 8) | (uint8_t)ax);
    ax = (uint16_t)(ax + 0x138);
    uint8_t dh = (uint8_t)(DB((uint16_t)(si + 9)) & 0x80);
    ax = (uint16_t)(((ax >> 8) | dh) << 8 | (uint8_t)ax);
    SET_DW((uint16_t)(si + 4), ax);
    bx = (uint16_t)(bx + 2);
    SET_DW((uint16_t)(si + 0x0C), bx);
}

/* Copies 1F40h bytes of each VRAM plane from the saved backdrop (3F40h) to the draw page.
 * Leaves DS = A000h, as the original does (the caller reads DS-relative data only after
 * restoring DS, see RunLevelIntroScene). */
void RestoreBackdropToDrawPage(void)
{
    SetLatchCopyMode();
    uint16_t di = DW(DS_DrawPage);
    for (uint16_t k = 0; k < 0x1F40; k++) {
        uint8_t b = mb_rd(SEG_VGA, (uint16_t)(0x3F40 + k));
        mb_wr(SEG_VGA, (uint16_t)(di + k), b);
    }
}

/* ---- exported adapters (differential tests) ---------------------------------------------- */
EXPORT int c_ShowWorldMapScrollIn(Regs *r) { (void)r; ShowWorldMapScrollIn(); return 0; }
EXPORT int c_loc_9613(Regs *r) { (void)r; loc_9613(); return 0; }
EXPORT int c_loc_96AE(Regs *r) { (void)r; loc_96AE(); return 0; }
EXPORT int c_loc_96C1(Regs *r) { (void)r; loc_96C1(); return 0; }
EXPORT int c_RunCallbackMapScreen(Regs *r) { int st = RunCallbackMapScreen(r->dx); return st; }
EXPORT int c_loc_97A8(Regs *r) { (void)r; return loc_97A8(); }
EXPORT int c_loc_9881(Regs *r) { (void)r; loc_9881(); return 0; }
EXPORT int c_UpdateModeSelectFrame(Regs *r) { (void)r; UpdateModeSelectFrame(); return 0; }
EXPORT int c_UpdateCodeEntryFrame(Regs *r) { (void)r; UpdateCodeEntryFrame(); return 0; }
EXPORT int c_loc_9AB4(Regs *r) { (void)r; loc_9AB4(); return 0; }
EXPORT int c_loc_9AC4(Regs *r) { (void)r; loc_9AC4(); return 0; }
EXPORT int c_loc_9A27(Regs *r) { (void)r; loc_9A27(); return 0; }
EXPORT int c_loc_9A9B(Regs *r) { loc_9A9B(r->dx); return 0; }
EXPORT int c_AdvanceMapFrame(Regs *r) { (void)r; AdvanceMapFrame(); return 0; }
EXPORT int c_DrawFontString(Regs *r) { r->bx = DrawFontString(r->bx, r->bp); return 0; }
EXPORT int c_WaitVerticalRetraceStart(Regs *r) { (void)r; WaitVerticalRetraceStart(); return 0; }
EXPORT int c_WaitNextVerticalRetrace(Regs *r) { (void)r; WaitNextVerticalRetrace(); return 0; }
EXPORT int c_RunGameOverScreen(Regs *r) { (void)r; RunGameOverScreen(); return 0; }
EXPORT int c_loc_9C62(Regs *r) { (void)r; loc_9C62(); return 0; }
EXPORT int c_CopyBackdropStrip(Regs *r) { (void)r; CopyBackdropStrip(); return 0; }
EXPORT int c_UpdateGameOverSprites(Regs *r) { (void)r; UpdateGameOverSprites(); return 0; }
EXPORT int c_SwapSpriteRecords(Regs *r) { SwapSpriteRecords(r->si, r->di); return 0; }
EXPORT int c_CopySpriteRecord(Regs *r) { CopySpriteRecord(r->si, r->di); return 0; }
EXPORT int c_RunFrontEnd(Regs *r) { (void)r; return RunFrontEnd(); }
EXPORT int c_loc_8EFD(Regs *r) { (void)r; loc_8EFD(); return 0; }
EXPORT int c_loc_8F29(Regs *r) { (void)r; loc_8F29(); return 0; }
EXPORT int c_loc_8E7B(Regs *r) { r->ax = (uint16_t)front_end_loop(1); return 0; }
/* ENTER CODE request entered at its call (loc_8EDE); tests stop at the restart jump. */
EXPORT int c_loc_8EDE(Regs *r) { r->ax = (uint16_t)front_end_loop(1); return 0; }
EXPORT int c_level_start_tail(Regs *r) { (void)r; level_start_tail(); return 0; }
EXPORT int c_RunLevelIntroScene(Regs *r) { (void)r; RunLevelIntroScene(); return 0; }
EXPORT int c_RunLevelIntroScene_once(Regs *r) { (void)r; intro_setup(); r->bp = walk_in_frame(0); return 0; }
EXPORT int c_loc_8F96_frame(Regs *r) { r->bp = walk_in_frame(r->bp); return 0; }
EXPORT int c_intro_group_setup(Regs *r) { (void)r; intro_group_setup(); return 0; }
EXPORT int c_loc_8F96(Regs *r) { (void)r; loc_8F96(); return 0; }
EXPORT int c_loc_9002(Regs *r) { (void)r; loc_9002(); return 0; }
EXPORT int c_ShowPresentTitle(Regs *r) { (void)r; return ShowPresentTitle(); }
EXPORT int c_ShowTitusScreen(Regs *r) { (void)r; return ShowTitusScreen(); }
EXPORT int c_ShowPlanarPicture(Regs *r) { ShowPlanarPicture(r->ds); return 0; }
EXPORT int c_ShowMode13PictureFadeIn(Regs *r)
{
    r->bx = ShowMode13PictureFadeIn((uint8_t)r->cx, r->bx);
    return 0;
}
EXPORT int c_loc_91CF(Regs *r) { r->bx = fade_in_passes(r->ds, r->bx); return 0; }
EXPORT int c_SetPictureCopyPointers(Regs *r)
{
    r->ds = SetPictureCopyPointers();
    r->si = 0; r->es = SEG_VGA; r->di = 0;
    return 0;
}
EXPORT int c_StepSpriteAnimation(Regs *r) { StepSpriteAnimation(r->si); return 0; }

/* DS and ES end at A000h; SI, DI, CX and AX are restored by the original's pops. */
EXPORT int c_RestoreBackdropToDrawPage(Regs *r)
{
    RestoreBackdropToDrawPage();
    r->ds = 0xA000;
    r->es = 0xA000;
    return 0;
}

EXPORT int c_FadeOutPaletteToBlack(Regs *r) { (void)r; FadeOutPaletteToBlack(); return 0; }
