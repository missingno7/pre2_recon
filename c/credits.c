/* Credits and intro screens (oracle: src/MODULE2.ASM ReadClockYear cs:23D4h, ShowLateYearCredits
 * cs:2409h, TryShowCredits cs:2479h, ShowCreditsTimed cs:24C0h (unused), ShowCreditsScreen
 * cs:24F8h, TryShowIntroKeyed cs:25C5h, ShowIntroPicture cs:25FEh, ShowIntroBody cs:2609h).
 *
 * Frame waits: the only loop with a frame wait is ShowCreditsTimed, which calls platform_poll()
 * per turn as the other screens do. The button waits (WaitForButtonPressRelease) are the
 * system.c polls. The keyboard gate tables (KeyDownTable, DATA:27F4h) are read as written.
 *
 * ReadClockYear quirk kept: the BCD year is decoded digit by digit into AX with an 8-bit add,
 * so a carry out of AL is lost (years 2048 and 2049 decode to 1792 and 1793). */
#include "pre2.h"
#include "ports.h"
#include "vga.h"
#include "platform.h"
#include "render.h"
#include "sqz.h"
#include "startup.h"
#include "system.h"
#include "credits.h"

/* Year gates: the late credits from 1996, the intro picture from 1994. */
#define CREDITS_YEAR_MIN 0x07CCu
#define INTRO_YEAR_MIN   0x07CAu
#define YEAR_LIMIT       0x0813u     /* 2067: years at or above are not stored */

/* ---- ReadClockYear (cs:23D4h) ----------------------------------------------------------- */

uint16_t ReadClockYear(int *carry)
{
    uint16_t ax, cx, dx;
    int err;
    platform_rtc_read_date(&ax, &cx, &dx, &err);     /* INT 1Ah AH=04h: CX century:year, DX month:day */
    if (err) {
        *carry = 1;
        return ax;
    }
    uint16_t year = 0;
    for (int digit = 0; digit < 4; digit++) {
        year = (uint16_t)(year * 10u);                /* mul bp (DX, the high word, is 0 here) */
        uint8_t nib = (uint8_t)(cx >> 12);            /* top nibble of CX, shifted into DH */
        cx = (uint16_t)(cx << 4);
        year = (uint16_t)((year & 0xFF00u) | (uint8_t)((uint8_t)year + nib));   /* add al, dh: carry lost */
    }
    *carry = year < YEAR_LIMIT;                      /* CF of the final cmp ax, 813h */
    if (year < YEAR_LIMIT) SET_DW(DS_ClockYear, year);
    return year;
}

/* ---- late-year credits (cs:2409h) ---------------------------------------------------------- */

void ShowLateYearCredits(void)
{
    if (DW(DS_ClockYear) < CREDITS_YEAR_MIN) return;
    ClearDisplayPage();
    SetStartupPalette();
    SET_DW(DS_TextByteColumn, 5);
    SET_DW(DS_TextRowOffset, 0x0960);
    PrintAsciiz(DATA_SEG, 0x28CA);
    SET_DW(DS_TextByteColumn, 0);
    SET_DW(DS_TextRowOffset, (uint16_t)(DW(DS_TextRowOffset) + 0x01E0));
    PrintAsciiz(DATA_SEG, 0x28D6);
    PrintDecimalWord(DW(DS_ClockYear));
    PrintAsciiz(DATA_SEG, 0x28F2);
    SET_DW(DS_TextByteColumn, 1);
    SET_DW(DS_TextRowOffset, 0x1680);
    PrintAsciiz(DATA_SEG, 0x28F6);
    SET_DW(DS_TextByteColumn, 3);
    SET_DW(DS_TextRowOffset, (uint16_t)(DW(DS_TextRowOffset) + 0x01E0));
    PrintAsciiz(DATA_SEG, 0x291B);
    WaitForButtonPressRelease();
    ClearDisplayPage();
}

/* ---- credits screen (cs:24F8h) ------------------------------------------------------------- */

static void AddRow(uint16_t delta) { SET_DW(DS_TextRowOffset, (uint16_t)(DW(DS_TextRowOffset) + delta)); }

void ShowCreditsScreen(void)
{
    ClearDisplayPage();
    SetStartupPalette();
    SET_DW(DS_TextByteColumn, 1);
    SET_DW(DS_TextRowOffset, 0x0140);
    PrintAsciiz(DATA_SEG, 0x2930);
    AddRow(0x0230);
    SET_DW(DS_TextByteColumn, 0x0E);
    PrintAsciiz(DATA_SEG, 0x2955);
    AddRow(0x0460);
    SET_DW(DS_TextByteColumn, 4);
    PrintAsciiz(DATA_SEG, 0x2960);
    AddRow(0x0230);
    SET_DW(DS_TextByteColumn, 0x0B);
    PrintAsciiz(DATA_SEG, 0x297F);
    AddRow(0x0460);
    SET_DW(DS_TextByteColumn, 9);
    PrintAsciiz(DATA_SEG, 0x2990);
    AddRow(0x0230);
    SET_DW(DS_TextByteColumn, 0x0B);
    PrintAsciiz(DATA_SEG, 0x29A5);
    SET_DW(DS_TextByteColumn, 0x0F);
    SET_DW(DS_TextRowOffset, 0x1770);
    PrintAsciiz(DATA_SEG, 0x29B6);
    SET_DW(DS_TextByteColumn, 2);
    SET_DW(DS_TextRowOffset, 0x1A40);
    PrintAsciiz(DATA_SEG, 0x29C0);
    AddRow(0x01E0);
    SET_DW(DS_TextByteColumn, 0);
    PrintAsciiz(DATA_SEG, 0x29E4);
}

/* ---- ShowCreditsTimed (cs:24C0h, unused) ----------------------------------------------------- */

void ShowCreditsTimed(void)
{
    SetLevelVideoMode();
    ShowCreditsScreen();
    SET_DW(DS_word_27F0, 0);
    for (;;) {
        PollInputState();
        if (DB(DS_byte_27E8) != 0) break;
        if (DW(DS_word_27F0) >= 0x0168u) break;
        platform_poll();
    }
    SET_DW(DS_word_27F0, 0x0870);
}

/* ---- TryShowCredits (cs:2479h) ----------------------------------------------------------------- */

int TryShowCredits(void)
{
    uint8_t gate = (uint8_t)(DB(DS_byte_2811) & DB(DS_byte_282C) & DB(DS_byte_2805));
    if (gate == 0) return 1;
    for (unsigned sc = 1; sc < 0x7F; sc++) {         /* scancodes 01h..7Eh */
        if (sc == 0x11 || sc == 0x1D || sc == 0x38) continue;   /* W, Ctrl, Alt */
        if (DB((uint16_t)(DS_KeyDownTable + sc)) != 0) return 1;
    }
    ShowCreditsScreen();
    WaitForButtonPressRelease();
    ClearDisplayPage();
    SetLevelPalette();
    return 0;
}

/* ---- intro picture (cs:25C5h..26A7h) ---------------------------------------------------------- */

/* Plane copy: map mask for the plane, then rep movsw of 40D8h words from the decoded
 * picture at src_seg:0 to A000:0, as the original does for each of the four planes. */
static void CopyPlane(uint16_t src_seg, uint8_t mask)
{
    port_out16(0x3C4, (uint16_t)((mask << 8) | 0x02));   /* sequencer index 2 (map mask) */
    for (uint16_t k = 0; k < 0x40D8u; k++) {
        uint16_t off = (uint16_t)(2u * k);
        uint16_t w = rd16(src_seg, off);
        vga_write(off, (uint8_t)w);
        vga_write((uint16_t)(off + 1), (uint8_t)(w >> 8));
    }
}

int ShowIntroBody(void)
{
    SET_DW(DS_NextFreeSegment, DW(DS_word_0039));
    uint16_t seg = 0;
    LoadSqzFile(0x2A0Cu, &seg);                        /* first load is overwritten (see notes) */
    SET_DW(DS_NextFreeSegment, seg);
    SET_DW(DS_PictureSegment, seg);
    bios_set_mode(0x12);                               /* INT 10h AX=12h: 640x480x16 */
    bios_set_palette(DATA_SEG, 0x28B0u);               /* attribute palette, identity */
    port_out8(0x3C8, 0x00);                            /* DAC: 16 grey steps, v = 0, 4 .. 60 */
    for (unsigned i = 0; i < 0x10; i++) {
        uint8_t v = (uint8_t)(4 * i);
        port_out8(0x3C9, v);
        port_out8(0x3C9, v);
        port_out8(0x3C9, v);
    }
    LoadSqzFile(0x2A17u, &seg);
    SET_DW(DS_NextFreeSegment, seg);
    SET_DW(DS_PictureSegment, seg);
    uint16_t bx = DW(DS_PictureSegment);
    CopyPlane(bx, 0x01);
    CopyPlane((uint16_t)(bx + 0x081Bu), 0x02);

    LoadSqzFile(0x2A22u, &seg);
    SET_DW(DS_NextFreeSegment, seg);
    SET_DW(DS_PictureSegment, seg);
    bx = DW(DS_PictureSegment);
    CopyPlane(bx, 0x04);
    CopyPlane((uint16_t)(bx + 0x081Bu), 0x08);

    WaitForButtonPressRelease();
    SetLevelVideoMode();
    return 0;
}

int ShowIntroPicture(void)
{
    if (DW(DS_ClockYear) < INTRO_YEAR_MIN) return 1;
    return ShowIntroBody();
}

int TryShowIntroKeyed(void)
{
    uint8_t gate = (uint8_t)(DB(DS_byte_2811) & DB(DS_byte_282C) & DB(DS_byte_2806));
    if (gate == 0) return 1;
    uint8_t acc = 0;
    for (unsigned sc = 1; sc < 0x7F; sc++) {         /* scancodes 01h..7Eh */
        if (sc == 0x12 || sc == 0x1D || sc == 0x38) continue;   /* E, Ctrl, Alt */
        acc |= DB((uint16_t)(DS_KeyDownTable + sc));
        if (acc != 0) return 1;
    }
    return ShowIntroBody();
}

/* ---- difftest adapters (tests/credits.py) ----------------------------------------------------- */

EXPORT int c_ReadClockYear(Regs *r)
{
    int cf;
    r->ax = ReadClockYear(&cf);
    r->cx = 0;                                       /* the digit loop leaves CX = 0 */
    r->ds = DATA_SEG;
    set_cf(r, cf);
    return 0;
}

EXPORT int c_ShowLateYearCredits(Regs *r) { ShowLateYearCredits(); r->ds = DATA_SEG; return 0; }

EXPORT int c_TryShowCredits(Regs *r) { set_cf(r, TryShowCredits()); r->ds = DATA_SEG; return 0; }

EXPORT int c_ShowCreditsTimed(Regs *r) { ShowCreditsTimed(); r->ds = DATA_SEG; return 0; }

EXPORT int c_ShowCreditsScreen(Regs *r) { ShowCreditsScreen(); r->ds = DATA_SEG; return 0; }

EXPORT int c_TryShowIntroKeyed(Regs *r) { set_cf(r, TryShowIntroKeyed()); r->ds = DATA_SEG; return 0; }

EXPORT int c_ShowIntroPicture(Regs *r) { set_cf(r, ShowIntroPicture()); r->ds = DATA_SEG; return 0; }

EXPORT int c_ShowIntroBody(Regs *r) { set_cf(r, ShowIntroBody()); r->ds = DATA_SEG; return 0; }
