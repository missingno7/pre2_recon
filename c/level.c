/* HUD, terrain entities, effect particles, camera shake, level state machine and death bounce,
 * tally panel, level resets, triggers, fireflies, camera chain and popup ring (oracle:
 * src/MODULE3.ASM cs:454E..584F; EnterLevelView at src/MODULE2.ASM cs:3AF2).
 *
 * Game state lives in pre2_mem at its original addresses (DATA through DB/DW, code-segment
 * variables through CB/CW). Screen writes go through the VGA model (vga_read/vga_write and the
 * port writes that select map mask, write mode, set/reset and function), so the planar memory
 * and the register state match the original byte for byte.
 *
 * Quirks kept (each comment marks the site): the status-bar and BONUS letter copies, the
 * 25000-point extra-life division (quotient above FFFFh faults), the lives and energy pip
 * arithmetic, the percent wrap of the tally (low byte of the collected sum times 100), the
 * one-frame life of effect particles (freed after their plot), the unbounded warp search in
 * UpdateLevelState, the 65536-iteration terminator fill of UpdateTerrainEntities when all seven
 * slots are used, and the popup ring masking its free marker before the test.
 *
 * Callees of the other regions (player, objects, effects, screens, sprites) are called through
 * their owners' headers. Boundaries that are not translated return LEVEL_UNSUPPORTED (level.h). */
#include "pre2.h"
#include "vga.h"
#include "ports.h"
#include "render.h"
#include "sprites.h"
#include "credits.h"
#include "sqz.h"
#include "system.h"
#include "effects.h"
#include "objects.h"
#include "player.h"
#include "screens.h"
#include "level.h"

/* ---- constants without labels (offsets inside DATA or the HUD graphic) -------------------- */

#define HUD_STRIP_SRC     0x07B0u   /* status-bar strip in the HUD sheet (word_003D) */
#define HUD_BONUS_SRC     0x07D1u   /* BONUS letter row background in the HUD sheet */
#define HUD_GLYPH_BASE    0x1610u   /* 16x12 glyph sheet, 96 bytes per glyph */
#define HUD_DIGIT_BASE    0x1C70u   /* 16x11 digit font, 88 bytes per digit */
#define HUD_LETTER_AT     0x6F86u   /* DATA: five destination offsets of the BONUS letters */
#define HUD_DIGITS_AT     0x6F4Eu   /* FormatDecimal32 output: ten digits, then '$' */
#define HUD_POW10_TOP     0x6F82u   /* FormatDecimal32 power table, first (largest) entry */
#define TALLY_PERCENT_AT  0x6F55u   /* the percent digits inside the FormatDecimal32 output */
#define TALLY_SCORE_AT    0x6F52u   /* the six low score digits inside the output */
#define ITEM_TALLY_COUNT  0x71u     /* ItemTallyCounts entries */
#define PLAYER_REC        0x4F1Cu   /* player record base (PlayerX): frame word at +4 */
#define FLIGHT_TABLE      0x52E8u   /* tally flight records, 20 of 12h bytes */
#define LEVEL_END_TARGET_X 0x3Cu
#define LEVEL_END_TARGET_Y 0xAFu
#define MAP_STRIDE        0x100u

/* ---- helpers --------------------------------------------------------------------------- */

/* Sequencer map mask (index 2) to plane_mask. */
static void map_mask(uint8_t plane_mask) { port_out16(0x3C4, (uint16_t)((uint16_t)plane_mask << 8 | 0x02)); }

static void hud_dirty_inc(void) { SET_CB(CS_HudDirtyCount, (uint8_t)(CB(CS_HudDirtyCount) + 1)); }

/* ---- HUD (cs:454E..4803, 46EE, 47C0) --------------------------------------------------- */

/* Redraws the status-bar background: 23 rows of 40 bytes in each plane from the HUD sheet into
 * DrawPage+1B80h, then latch-copies the strip to DisplayPage+1B80h. Invalidates the HUD caches
 * so UpdateHud redraws every field on its next call. */
void DrawStatusBarBackground(void)
{
    SET_DW(DS_HudCacheScoreHigh, 0xFFFFu);
    SET_DB(DS_HudCacheLives, 0xFF);
    SET_DB(DS_HudCacheEnergy, 0xFF);
    SET_DB(DS_HudCacheBonus, 0xFF);
    SetCpuWriteMode();
    uint16_t display = DW(DS_DisplayPage);
    uint16_t sheet = DW(DS_word_003D);
    uint16_t src = HUD_STRIP_SRC;
    uint16_t dst = (uint16_t)(DW(DS_DrawPage) + 0x1B80u);
    for (unsigned p = 0; p < 4; p++) {
        map_mask((uint8_t)(1u << p));
        uint16_t d = dst;
        for (unsigned k = 0; k < 0x398u; k++) {          /* rep movsw cx=1CCh, then sub di,398h */
            vga_write(d, rd8(sheet, src));
            src++; d++;
        }
    }
    SetLatchCopyMode();
    uint16_t si = dst;
    uint16_t di = (uint16_t)(display + 0x1B80u);
    for (unsigned k = 0; k < 0x398u; k++) { vga_movsb(si, di); si++; di++; }
}

/* Restores the BONUS letter row background (6 bytes by 23 rows, four planes) from the HUD sheet
 * into DrawPage+1BA1h, and counts one HUD write. AX (the letter mask in AH) is preserved. */
void ClearBonusLetterRow(void)
{
    uint16_t sheet = DW(DS_word_003D);
    uint16_t dst = (uint16_t)(DW(DS_DrawPage) + 0x1BA1u);
    uint16_t s = HUD_BONUS_SRC;
    for (unsigned p = 0; p < 4; p++) {
        map_mask((uint8_t)(1u << p));
        uint16_t d = dst;
        for (unsigned row = 0; row < 0x17; row++) {
            for (unsigned k = 0; k < 6; k++) { vga_write(d, rd8(sheet, s)); s++; d++; }
            d = (uint16_t)(d + 0x22); s = (uint16_t)(s + 0x22);  /* row stride 40 */
        }
    }
    hud_dirty_inc();
}

/* Blits one 16x12 HUD glyph (four planes) from the glyph sheet at HUD_GLYPH_BASE + 96*glyph.
 * In: glyph 0..9 digits, 0Ah/0Bh energy pips, 0Ch..10h BONUS letters; di = destination. */
void BlitHudGlyph(uint8_t glyph, uint16_t di)
{
    uint16_t sheet = DW(DS_word_003D);
    uint16_t s = (uint16_t)(HUD_GLYPH_BASE + 0x60u * glyph);
    for (unsigned p = 0; p < 4; p++) {
        map_mask((uint8_t)(1u << p));
        uint16_t d = di;
        for (unsigned row = 0; row < 12; row++) {
            vga_write(d, rd8(sheet, s)); s++; d++;
            vga_write(d, rd8(sheet, s)); s++; d++;
            d = (uint16_t)(d + 0x26);
        }
    }
    hud_dirty_inc();
}

/* Blits one 16x11 digit (four planes) from the font at HUD_DIGIT_BASE + 88*digit. */
void BlitHudDigit(uint8_t digit, uint16_t di)
{
    uint16_t sheet = DW(DS_word_003D);
    uint16_t s = (uint16_t)(HUD_DIGIT_BASE + 0x58u * digit);
    for (unsigned p = 0; p < 4; p++) {
        map_mask((uint8_t)(1u << p));
        uint16_t d = di;
        for (unsigned row = 0; row < 0x0B; row++) {
            vga_write(d, rd8(sheet, s)); s++; d++;
            vga_write(d, rd8(sheet, s)); s++; d++;
            d = (uint16_t)(d + 0x26);
        }
    }
    hud_dirty_inc();
}

/* Blits one 16x11 glyph from sprite frame (0F1h + index) of the loaded sprite sheet, skipping its
 * 16h-byte frame header. Index is signed (CBW): letters are letter - 41h. It does not count a HUD
 * write, so a glyph drawn only through this routine does not trigger the strip copy (quirk). */
void BlitHudSpriteGlyph(uint8_t index, uint16_t di)
{
    uint16_t idx = (uint16_t)((((uint16_t)(int16_t)(int8_t)index) + 0xF1u) << 1);
    uint16_t seg = DW((uint16_t)(DS_FrameDataSegments + idx));
    uint16_t s = (uint16_t)(DW((uint16_t)(DS_FrameDataOffsets + idx)) + 0x16u);
    for (unsigned p = 0; p < 4; p++) {
        map_mask((uint8_t)(1u << p));
        uint16_t d = di;
        for (unsigned row = 0; row < 0x0B; row++) {
            vga_write(d, rd8(seg, s)); s++; d++;
            vga_write(d, rd8(seg, s)); s++; d++;
            d = (uint16_t)(d + 0x26);
        }
    }
}

/* Converts the unsigned 32-bit value CX:DX (CX high word) to ten ASCII digits at DATA
 * HUD_DIGITS_AT, terminated by '$'. Repeated subtraction of the descending powers of ten (the
 * ASM's compare chain is exactly value >= power). Leaves DS unchanged in C. */
void FormatDecimal32(uint16_t cx, uint16_t dx)
{
    uint32_t v = (uint32_t)cx << 16 | dx;
    uint16_t di = HUD_POW10_TOP;
    uint16_t si = HUD_DIGITS_AT;
    for (unsigned k = 0; k < 10; k++) {
        uint32_t p = (uint32_t)DW(di) << 16 | DW((uint16_t)(di + 2));
        uint8_t digit = '0';
        while (v >= p) { v -= p; digit++; }
        wr8(DATA_SEG, si, digit);
        si++;
        di = (uint16_t)(di - 4);
    }
    wr8(DATA_SEG, si, '$');
}

/* Per-frame HUD update: lives, energy, score digits and the BONUS letters, plus the 25000-point
 * extra-life check. Returns LEVEL_DIVIDE_FAULT before any write when the DIV of the score by 25000
 * faults (quotient above FFFFh; the 1638400000-point boundary). Lives are shown capped at 9 (the
 * cache holds the uncapped value). Returns at once while RespawnState is nonzero. */
int UpdateHud(void)
{
    if (DB(DS_RespawnState) != 0) return 0;
    uint32_t score = (uint32_t)DW(DS_ScoreHigh) << 16 | DW(DS_ScoreLow);
    uint32_t q = score / 25000u;
    if (q > 0xFFFFu) return LEVEL_DIVIDE_FAULT;      /* quirk: DIV #DE, nothing is written */
    uint16_t diff = (uint16_t)((uint16_t)q - DW(DS_ExtraLifeBlocks));
    if (diff != 0) {
        SET_DW(DS_ExtraLifeBlocks, (uint16_t)(DW(DS_ExtraLifeBlocks) + diff));
        uint8_t al = (uint8_t)diff;
        for (;;) {                                  /* loc_45D3: dec al; jns loc_45D0 */
            al = (uint8_t)(al - 1);
            if (al & 0x80) break;
            IncrementPlayerLives();
        }
        SpawnDebrisElement(0x00E3u, PLAYER_REC);
    }
    SetCpuWriteMode();
    SET_CB(CS_HudDirtyCount, 0);

    uint8_t lives = DB(DS_Lives);
    if (DB(DS_HudCacheLives) != lives) {
        SET_DB(DS_HudCacheLives, lives);
        uint8_t shown = lives <= 9 ? lives : 9;     /* quirk: capped at 9 on the display */
        BlitHudGlyph(shown, (uint16_t)(DW(DS_DrawPage) + 0x1CEDu));
    }

    uint16_t lo = DW(DS_ScoreLow), hi = DW(DS_ScoreHigh);
    if (DW(DS_HudCacheScoreLow) != lo || DW(DS_HudCacheScoreHigh) != hi) {
        SET_DW(DS_HudCacheScoreLow, lo);
        SET_DW(DS_HudCacheScoreHigh, hi);
        FormatDecimal32(hi, lo);
        uint16_t di = (uint16_t)(DW(DS_DrawPage) + 0x1CF1u);
        uint16_t si = TALLY_SCORE_AT;
        for (;;) {                                  /* six digits, then '$' */
            uint8_t c = DB(si);
            si++;
            if (c == '$') break;
            BlitHudGlyph((uint8_t)(c - 0x30), di);
            di = (uint16_t)(di + 2);
        }
        BlitHudGlyph(0, di);                        /* the fixed 0 digit */
    }

    uint8_t energy = DB(DS_Energy);
    if (DB(DS_HudCacheEnergy) != energy) {
        SET_DB(DS_HudCacheEnergy, energy);
        uint8_t ah = energy;
        uint8_t dl = 3;
        uint16_t di = (uint16_t)(DW(DS_DrawPage) + 0x1D01u);
        if (ah != 0) {                              /* full pips (quirk: dl wraps above 3) */
            do {
                BlitHudGlyph(0x0A, di); di = (uint16_t)(di + 2);
                dl--; ah--;
            } while (ah != 0);
        }
        if (dl != 0) {                              /* empty pips */
            do {
                BlitHudGlyph(0x0B, di); di = (uint16_t)(di + 2);
                dl--;
            } while (dl != 0);
        }
    }

    uint8_t bits;
    if (DB(DS_BonusFlashTimer) != 0) {
        SET_DB(DS_HudCacheBonus, 0xFF);
        bits = (DB(DS_SnowFrameTick) & 1) ? 0x1F : 0x00;
    } else {
        bits = DB(DS_BonusLetterMask);
        if (DB(DS_HudCacheBonus) == bits) goto dirty_check;
        SET_DB(DS_HudCacheBonus, bits);
    }
    ClearBonusLetterRow();
    {
        uint8_t letter = 0x0C;
        uint16_t si = HUD_LETTER_AT;
        do {
            uint16_t di = (uint16_t)(DW(si) + DW(DS_DrawPage));
            si = (uint16_t)(si + 2);
            int lit = bits & 1;
            bits = (uint8_t)(bits >> 1);
            if (lit) BlitHudGlyph(letter, di);
            letter++;
        } while (letter <= 0x10);
    }

dirty_check:
    if (CB(CS_HudDirtyCount) != 0) {
        SetLatchCopyMode();
        uint16_t si = (uint16_t)(0x1B80u + DW(DS_DrawPage));
        uint16_t di = (uint16_t)(0x1B80u + DW(DS_DisplayPage));
        for (unsigned k = 0; k < 0x398u; k++) { vga_movsb(si, di); si++; di++; }
    }
    return 0;
}

/* ---- player render slot and terrain entities (cs:484E..4AF9) -------------------------- */

/* Chooses the player's pose frame and display position from the pose word PlayerFrameWord (pose
 * key in its low 13 bits) and the pose table at 7B29h (6-byte records: key, frame, dx, dy; key 0
 * ends). Writes render slot 0 (X), PlayerRenderY and PlayerSprite. Returns at once unless
 * PlayerAltModeFlag is set. */
void UpdatePlayerRenderSlot(void)
{
    if (DB(DS_PlayerAltModeFlag) == 0) return;
    if (DB(DS_PlayerAirFlags) != 0) {
        SET_DW(DS_PlayerFrameWord, DW(DS_PlayerFrameWord) & 0x8000u);
        uint16_t dx = 0x30;
        uint16_t ax = DW(DS_PlayerVelX);
        if ((int16_t)ax < 0) ax = (uint16_t)(0u - ax);
        if (ax <= 0x40) { dx++; if (ax <= 0x20) dx++; }
        uint8_t al = DB(DS_AltMomentumStep);
        if (al < 3) dx = (uint16_t)(0x33u - al);
        SET_DW(DS_PlayerFrameWord, DW(DS_PlayerFrameWord) | dx);
    }
    uint16_t pfw = DW(DS_PlayerFrameWord);
    uint16_t key = (uint16_t)(pfw & 0x1FFFu);
    uint8_t flagsHi = (uint8_t)(pfw >> 8);
    uint8_t step = (uint8_t)(DB(DS_byte_27EA) | DB(DS_byte_27EB));
    if (step == 0) {
        step = 1;
        if (DB(DS_AltMomentumStep) != 3) {
            if (DB(DS_AltMomentumStep) >= 3) step = (uint8_t)(0 - step);
            SET_DB(DS_AltMomentumStep, (uint8_t)(DB(DS_AltMomentumStep) + step));
        }
    }
    uint16_t si = 0x7B29;
    for (;;) {
        uint16_t k = DW(si);
        if (k == 0) return;
        if (key != k) { si = (uint16_t)(si + 6); continue; }
        int16_t cx = (int8_t)DB((uint16_t)(si + 4));
        uint16_t ax2 = DW((uint16_t)(si + 2));
        if (ax2 >= 0x79) ax2 = DW((uint16_t)(DS_word_7B1B + 2u * DB(DS_AltMomentumStep)));
        if (flagsHi & 0x80) { ax2 = (uint16_t)(ax2 | 0x8000u); cx = (int16_t)(0 - cx); }
        if (DB(DS_StrokeCounter) > 0x18 && (uint8_t)ax2 < 0x79) ax2++;
        SET_DW(DS_PlayerSprite, ax2);
        SET_DW(DS_RenderSlots, (uint16_t)(DW(DS_PlayerX) + (uint16_t)cx));
        int16_t dy = (int8_t)DB((uint16_t)(si + 5));
        SET_DW(DS_PlayerRenderY, (uint16_t)(DW(DS_PlayerY) + (uint16_t)dy));
        return;
    }
}

/* Lands the player on terrain entity DI when the player is above it and the probe at PlayerY +
 * PlatformDeltaY overlaps. Returns CF = 1 when landed (PlayerOnPlatform = 1, PlayerX += delta X,
 * PlayerY snapped to the top, PlayerVelY set); CF = 0 otherwise. */
int TryLandOnTerrainEntity(uint16_t di)
{
    if (DB(DS_PlayerOnPlatform) != 0) return 0;
    if ((int16_t)DW(di + 2) <= (int16_t)DW(DS_PlayerY)) return 0;
    uint16_t savedY = DW(DS_PlayerY);
    int16_t pdy = (int16_t)DW(DS_PlatformDeltaY);
    if (pdy >= 0) SET_DW(DS_PlayerY, (uint16_t)(DW(DS_PlayerY) + (uint16_t)pdy));
    uint16_t savedFrame = DW(DS_PlayerFrameWord);
    SET_DW(DS_PlayerFrameWord, 7);
    int overlap = TestHitboxOverlap(PLAYER_REC, di);
    SET_DW(DS_PlayerFrameWord, savedFrame);
    SET_DW(DS_PlayerY, savedY);
    if (!overlap) return 0;
    SET_DB(DS_PlayerOnPlatform, 1);
    SET_DB(DS_MotionShift, 0);
    SET_DW(DS_PlayerX, (uint16_t)(DW(DS_PlayerX) + DW(DS_PlatformDeltaX)));
    FinishPlayerLanding();
    uint16_t bx = (uint16_t)((DW(di + 4) & 0x1FFFu) << 1);     /* and bh,1Fh; shl bx,1 */
    uint16_t top = (uint16_t)(DW(di + 2) - DB((uint16_t)(DS_FrameHeights + bx)));
    if ((int16_t)DW(DS_PlayerY) > (int16_t)top) {
        SET_DW(DS_PlayerY, (uint16_t)(top + 1));
        SET_DW(DS_PlayerVelY, 1);
    } else {
        SET_DW(DS_PlayerVelY, (uint16_t)((uint16_t)pdy << 4));
    }
    return 1;
}

/* Moves the terrain entities (16 records at 9107h, stride 0Fh; sprite word FFFFh = free) and
 * writes the visible ones as render slots from 5570h (stride 12h, up to 7 plus a FFFFh
 * terminator). Falling entities (type 8) move under gravity; the others step in one of eight
 * directions. PlayerOnPlatform is cleared at entry. */

/* loc_492D..loc_49EA: falling entity (type 8). */
static void terrain_falling(uint16_t si)
{
    SET_DW(DS_PlatformDeltaX, 0);
    uint16_t v = DW((uint16_t)(si + 0x0B));
    SET_DW((uint16_t)(si + 2), (uint16_t)(DW((uint16_t)(si + 2)) - v));
    if (DB((uint16_t)(si + 0x0A)) == 0) {
        uint16_t ax = (uint16_t)(v - 8);
        if ((int16_t)ax < 0) { ax = 0; SET_DB((uint16_t)(si + 7), 0); }
        SET_DW((uint16_t)(si + 0x0B), ax);
        if (DB((uint16_t)(si + 6)) & 0x40) {
            int stay = 0;
            if (DB((uint16_t)(si + 7)) == 0) {
                SET_DB((uint16_t)(si + 0x0D), (uint8_t)(DB((uint16_t)(si + 0x0D)) - 1));
                stay = DB((uint16_t)(si + 0x0D)) != 0;
            }
            if (!stay) { SET_DB((uint16_t)(si + 0x0A), 1); SET_DB((uint16_t)(si + 7), 0); }
        }
    } else if (DB((uint16_t)(si + 0x0A)) == 1) {
        uint16_t ax = DB((uint16_t)(si + 7));
        if (ax < 0xC0) SET_DB((uint16_t)(si + 7), (uint8_t)(DB((uint16_t)(si + 7)) + 8));
        int16_t d = (int16_t)ax >> 4;
        SET_DW(DS_PlatformDeltaY, (uint16_t)d);
        SET_DW((uint16_t)(si + 0x0B), (uint16_t)(DW((uint16_t)(si + 0x0B)) + (uint16_t)d));
        int16_t col = (int16_t)DW(si) >> 4;
        int16_t row = (int16_t)(DW((uint16_t)(si + 2)) + DW((uint16_t)(si + 0x0B))) >> 4;
        uint16_t lim = (uint16_t)((uint16_t)DB(DS_MapRows) - 1u);
        int16_t hit2 = 0;
        if ((uint16_t)row <= lim) {                       /* jae loc_49AE: inside the map */
            uint16_t off = (uint16_t)((((uint16_t)row & 0xFFu) << 8) | ((uint16_t)col & 0xFFu));
            uint8_t tile = rd8(DW(DS_LevelSegment), off);
            uint8_t gt = DB((uint16_t)(DS_GroundTypeTable + tile));
            hit2 = gt != 0 && gt != 6;
        } else {
            uint16_t t = (uint16_t)(0u - (uint16_t)(lim - (uint16_t)row));
            hit2 = t >= 3;
        }
        if (hit2) { SET_DB((uint16_t)(si + 0x0A), 2); SET_DB((uint16_t)(si + 0x0D), 0x16); }
    } else if (DB((uint16_t)(si + 0x0A)) == 2) {
        if (!(DB((uint16_t)(si + 6)) & 0x40)) {
            SET_DB((uint16_t)(si + 0x0D), (uint8_t)(DB((uint16_t)(si + 0x0D)) - 1));
            if (DB((uint16_t)(si + 0x0D)) == 0) {
                SET_DB((uint16_t)(si + 0x0A), 0);
                SET_DB((uint16_t)(si + 0x0D), DB((uint16_t)(si + 9)));
            }
        }
    }
    SET_DW((uint16_t)(si + 2), (uint16_t)(DW((uint16_t)(si + 2)) + DW((uint16_t)(si + 0x0B))));
}

/* loc_49F3..loc_4A8B: directional entity (types other than 8). Returns with the entity moved. */
static void terrain_directional(uint16_t si)
{
    SET_DW(DS_PlatformDeltaX, 0);
    SET_DW(DS_PlatformDeltaY, 0);
    if (DB((uint16_t)(si + 0x0E)) == 0 && (int8_t)DB((uint16_t)(si + 7)) >= 0 &&
        (DB((uint16_t)(si + 6)) & 0xC0) == 0)
        return;
    uint8_t al = DB((uint16_t)(si + 0x0E));
    if (DB((uint16_t)(si + 7)) != al) {
        int8_t step = (int8_t)DB((uint16_t)(si + 7)) >= (int8_t)al ? 1 : -1;
        al = (uint8_t)(al + (uint8_t)step);
        SET_DB((uint16_t)(si + 0x0E), al);
    }
    uint8_t ah = al;
    uint8_t dir = DB((uint16_t)(si + 6)) & 7;
    if (dir == 0) { al = 0; ah = (uint8_t)(0 - ah); }
    if (dir == 1) ah = (uint8_t)(0 - ah);
    if (dir == 2) ah = 0;
    if (dir == 4) al = 0;
    if (dir == 5) al = (uint8_t)(0 - al);
    if (dir == 6) { al = (uint8_t)(0 - al); ah = 0; }
    if (dir == 7) { al = (uint8_t)(0 - al); ah = (uint8_t)(0 - ah); }
    if (dir != 3) {
        int16_t dxv = (int8_t)al, dyv = (int8_t)ah;
        SET_DW(DS_PlatformDeltaX, (uint16_t)dxv);
        SET_DW((uint16_t)(si), (uint16_t)(DW((uint16_t)(si)) + (uint16_t)dxv));
        SET_DW(DS_PlatformDeltaY, (uint16_t)dyv);
        SET_DW((uint16_t)(si + 2), (uint16_t)(DW((uint16_t)(si + 2)) + (uint16_t)dyv));
    }
    if (DB((uint16_t)(si + 7)) == DB((uint16_t)(si + 0x0E))) {
        uint16_t ax = (uint16_t)(DW((uint16_t)(si + 0x0C)) + 1);
        if (DW((uint16_t)(si + 0x0A)) != ax) {
            SET_DW((uint16_t)(si + 0x0C), ax);
        } else {
            SET_DW((uint16_t)(si + 7), (uint16_t)(0u - DW((uint16_t)(si + 7))));
            SET_DW((uint16_t)(si + 0x0C), 0);
        }
    }
}

void UpdateTerrainEntities(void)
{
    uint16_t si = 0x9107, di = 0x5570;
    uint16_t bx = 7;
    SET_DB(DS_PlayerOnPlatform, 0);
    for (unsigned cx = 0x10; cx != 0; cx--, si = (uint16_t)(si + 0x0F)) {
        if (DW((uint16_t)(si + 4)) == 0xFFFF) continue;
        if ((DB((uint16_t)(si + 6)) & 0x0F) == 8) terrain_falling(si);
        else terrain_directional(si);
        int16_t rx = (int16_t)(DW(si) - (uint16_t)(DW(DS_CameraCol) << 4));
        if (rx >= 0x160 || rx <= -0x20) continue;
        wr16(DATA_SEG, di, DW(si));                /* X is written before the Y test (as the ASM) */
        int16_t ry = (int16_t)(DW((uint16_t)(si + 2)) - (uint16_t)(DW(DS_CameraRow) << 4));
        if (ry >= 0xD0 || ry <= -0x20) continue;
        wr16(DATA_SEG, (uint16_t)(di + 2), DW((uint16_t)(si + 2)));
        wr16(DATA_SEG, (uint16_t)(di + 4), DW((uint16_t)(si + 4)));
        wr16(DATA_SEG, (uint16_t)(di + 9), si);
        if ((int16_t)DW(DS_PlayerVelY) > -0x10) {
            SET_DB((uint16_t)(si + 6), DB((uint16_t)(si + 6)) | 0x40);
            if (TryLandOnTerrainEntity(di)) {
                di = (uint16_t)(di + 0x12);
                bx--;
                continue;
            }
        }
        SET_DB((uint16_t)(si + 6), DB((uint16_t)(si + 6)) & 0xBF);
        SET_DW((uint16_t)(di + 2), (uint16_t)(DW((uint16_t)(di + 2)) - 2));
        di = (uint16_t)(di + 0x12);
        bx--;
    }
    /* loc_4AF9: terminators. The loop body runs at least once: with all seven slots used,
     * bx is 0 and the loop runs 65536 times (quirk, kept). */
    do {
        wr16(DATA_SEG, (uint16_t)(di + 4), 0xFFFF);
        di = (uint16_t)(di + 0x12);
        bx--;
    } while (bx != 0);
}

/* ---- effect particles (cs:4B8E..4C2F) -------------------------------------------------- */

/* Moves the 20 effect particles (DebrisRecord, 6-byte records: X, Y, angle index, speed; X = FFFFh
 * is free) by the sine tables and plots each one inside the view as one pixel with the VGA XOR
 * function. Quirk: every processed record is freed after its plot, so a particle lives one frame.
 * Leaves GC index 3 = XOR and index 1 = 0 (the caller does not restore them). */
void UpdateEffectParticles(void)
{
    SetCpuWriteMode();
    port_out16(0x3C4, 0x0F02);
    port_out16(0x3CE, 0x1003);
    uint16_t si = DS_DebrisRecord;
    for (unsigned n = 0; n < 0x14; n++, si = (uint16_t)(si + 6)) {
        if (DW(si) == 0xFFFF) continue;
        uint8_t ang = DB((uint16_t)(si + 4));
        int8_t tx = (int8_t)DB((uint16_t)(DS_SineTable + ang)) >> 2;
        int16_t px = (int16_t)tx * (int8_t)DB((uint16_t)(si + 5));
        SET_DW(si, (uint16_t)(DW(si) + (uint16_t)(px >> 4)));
        int8_t ty = (int8_t)DB((uint16_t)(DS_byte_7090 + ang)) >> 2;
        int16_t py = (int16_t)ty * (int8_t)DB((uint16_t)(si + 5));
        SET_DW((uint16_t)(si + 2), (uint16_t)(DW((uint16_t)(si + 2)) + (uint16_t)(py >> 4)));

        uint16_t ay = (uint16_t)((uint16_t)(0u - (uint16_t)(int16_t)(int8_t)DB(DS_FineScroll)) +
                                 DW((uint16_t)(si + 2)));
        ay = (uint16_t)(ay - (uint16_t)(DW(DS_CameraRow) << 4));
        if (ay < 0xB0) {
            uint16_t di = (uint16_t)((uint8_t)ay * 0x28u);
            uint16_t ax = (uint16_t)(DW(si) - (uint16_t)(DW(DS_CameraCol) << 4));
            if (ax < 0x140) {
                uint8_t mask = (uint8_t)(0x80u >> (ax & 7));
                di = (uint16_t)(di + (ax >> 3) + DW(DS_DrawPage));
                vga_xchg(di, mask);
            }
        }
        SET_DW(si, 0xFFFF);                        /* loc_4C1D: freed after the plot (quirk) */
    }
    port_out16(0x3CE, 0x0001);
}

/* ---- camera shake (cs:4C30) ---------------------------------------------------------- */

/* Per-frame vertical screen shake. ShakeMagnitude 0 = idle; 1 = clear ShakeRows; 2 and above =
 * active: on each odd SnowFrameTick PlayerY moves up 3 (unless CurrentAnimId is 5 or 20h) and the
 * magnitude is incremented; ShakeRows = the magnitude on odd ticks, else 0. */
void UpdateCameraShake(void)
{
    uint8_t m = DB(DS_ShakeMagnitude);
    if (m < 1) return;
    if (m == 1) { SET_DW(DS_ShakeRows, 0); return; }
    uint16_t ax = 0;
    if (DB(DS_SnowFrameTick) & 1) {
        uint8_t anim = DB(DS_CurrentAnimId);
        if (anim != 5 && anim != 0x20) SET_DW(DS_PlayerY, (uint16_t)(DW(DS_PlayerY) - 3));
        SET_DB(DS_ShakeMagnitude, (uint8_t)(DB(DS_ShakeMagnitude) + 1));
        ax = DB(DS_ShakeMagnitude);
    }
    SET_DW(DS_ShakeRows, ax);
}

/* ---- level state machine (cs:4C69..509C) ------------------------------------------- */

static int level_finish(uint16_t *ax_out);
static int LevelTransitionStart(uint16_t *ax_out);
static int RespawnPath(uint16_t *ax_out);
static int LifeLostPath(uint16_t *ax_out);
static int GameOverPath(uint16_t *ax_out);
static int RestartFromCheckpoint(uint16_t *ax_out);
static int ShowEndingPicture(uint16_t *ax_out);

/* loc_4F65: ResetLevelState, then return AX = 0, CF = 1 (a new level begins). */
static int level_finish(uint16_t *ax_out)
{
    ResetLevelState();
    *ax_out = 0;
    return 1;
}

/* Death bounce: 60 frames of a hop. Hides the player, moves PlayerX by +/-5 (away from the view
 * centre) and PlayerY along a PlayerVelY arc (15 down to -16 units per frame), redrawing the scene
 * each frame, and plays sound 7. Callers: UpdateLevelState (respawn, life lost, demo end). */
void PlayDeathBounce(void)
{
    StartSoundEffect(7);
    SET_DB(DS_PlayerHurtTimer, 0);
    SET_DW(DS_PlayerFrameWord, 0x21);
    SET_DW(DS_PlayerVelY, 0x0F);
    uint16_t limit = (uint16_t)((DW(DS_CameraCol) + 0x0Au) << 4);
    uint16_t vx = DW(DS_PlayerX) < limit ? 5 : (uint16_t)(0u - 5u);
    SET_DW(DS_PlayerVelX, vx);
    for (unsigned n = 0; n < 0x3C; n++) {
        SET_DW(DS_PlayerSprite, 0xFFFF);
        UpdatePopupRing();
        UpdateObjects();
        UpdatePlayerProjectiles();
        UpdateBurstParticles();
        TickDebrisSlots();
        AnimateTiles();
        RedrawOverlayTiles();
        CopyRingWindowToPage();
        UpdateEffectParticles();
        DrawSpriteObjects();
        DrawForegroundOverSprites();
        UpdateFireflies();
        UpdateWindAndFlakes();
        PresentFrame();
        SET_DW(DS_PlayerX, (uint16_t)(DW(DS_PlayerX) + DW(DS_PlayerVelX)));
        int16_t vy = (int16_t)DW(DS_PlayerVelY) - 1;
        if (vy < (int16_t)0xFFF0) vy = (int16_t)0xFFF0;
        SET_DW(DS_PlayerVelY, (uint16_t)vy);
        SET_DW(DS_PlayerY, (uint16_t)(DW(DS_PlayerY) - (uint16_t)vy));
    }
    ClearStaggeredBlocks();
}

/* Level state machine, called once per frame by MainFrameLoop. Priority: LevelEndMode (nonzero)
 * first; with it zero, RespawnState, then EndSignal. Returns CF = 0 to continue the frame (AX = 1
 * on that path); CF = 1 to start the level again with AX = 0 (new level, warp or checkpoint
 * restart) or AX = 1 (game over, ending, demo end). */
int UpdateLevelState(uint16_t *ax_out)
{
    uint8_t mode = DB(DS_LevelEndMode);
    if (mode == 0) {                                   /* loc_4C99 */
        *ax_out = 1;
        if (DB(DS_RespawnState) == 1) return RespawnPath(ax_out);
        if (DB(DS_EndSignal) == 1) return LifeLostPath(ax_out);
        if (DB(DS_EndSignal) == 0xFF) return ShowEndingPicture(ax_out);
        return 0;
    }
    if (mode >= 2) {
        uint8_t lvl = DB(DS_CurrentLevel);
        if (lvl < 0x0A) {                              /* loc_4C8F: warp table, no exit */
            SET_DB(DS_CurrentLevel, DB((uint16_t)(DS_WarpLevelTable + lvl)));
            ClearStaggeredBlocks();
            return level_finish(ax_out);
        }
        /* quirk: unbounded equality scan of the warp table; an id absent from the table runs past
         * it (16-bit wrap) until a byte matches. */
        uint16_t bx = DS_WarpLevelTable;
        while (DB(bx) != lvl) bx = (uint16_t)(bx + 1);
        SET_DB(DS_CurrentLevel, (uint8_t)(bx - DS_WarpLevelTable));
    }
    /* loc_4CBA: mode 1, or a warp id that has just been mapped */
    *ax_out = 0;
    if (DB(DS_CurrentLevel) >= 0x0A) {
        ClearStaggeredBlocks();
        SET_DB(DS_CurrentLevel, (uint8_t)(DB(DS_CurrentLevel) + 1));
        return level_finish(ax_out);
    }
    return LevelTransitionStart(ax_out);
}

/* Exit sequence (level below 0Ah): iris close, the player walks to the exit, the tally flights
 * run when ItemTotal is nonzero, then the level counter advances. */
static int LevelTransitionStart(uint16_t *ax_out)
{
    if (DB(DS_byte_6C04) != 0) {
        SET_DB(DS_FadeOutActive, 0);
        SET_DB(DS_FadeInActive, 1);
        SET_DB(DS_PaletteFadeStep, 0);
    }
    RunIrisClose();
    ActivateResourceSlot(0x0F);
    SET_DW(DS_PlayerY, (uint16_t)(DW(DS_PlayerY) - (DW(DS_CameraRow) << 4)));
    SET_DW(DS_PlayerX, (uint16_t)(DW(DS_PlayerX) - (DW(DS_CameraCol) << 4)));
    SET_DW(DS_CameraCol, 0);
    SET_DW(DS_CameraRow, 0);
    SET_DW(DS_PlayerFacing, 0);
    SET_DW(DS_PlayerVelX, 0);
    SetCpuWriteMode();
    RenderTallyPanel();
    SET_DB(DS_PlayerAltModeFlag, 0);
    SET_DW(DS_PlayerSprite, 0xFFFF);
    SET_DW(DS_PlayerAnimPtr, 0x7BA7);

    /* walk the player to the exit point (LEVEL_END_TARGET_X/Y) */
    for (;;) {
        AdvancePlayerAnim(DW(DS_PlayerAnimPtr));
        unsigned moving = 0;
        int16_t bxstep = 2;
        uint16_t ax = (uint16_t)(DW(DS_PlayerX) - LEVEL_END_TARGET_X);
        if ((int16_t)ax < 0) bxstep = -2;
        uint16_t nx = LEVEL_END_TARGET_X;
        if (ax >= 2) { nx = (uint16_t)(DW(DS_PlayerX) - (uint16_t)bxstep); moving++; }
        SET_DW(DS_PlayerX, nx);
        ax = (uint16_t)(DW(DS_PlayerY) - LEVEL_END_TARGET_Y);
        bxstep = (int16_t)ax < 0 ? -2 : 2;
        uint16_t ny = LEVEL_END_TARGET_Y;
        if (ax >= 2) { ny = (uint16_t)(DW(DS_PlayerY) - (uint16_t)bxstep); moving++; }
        SET_DW(DS_PlayerY, ny);
        ClearDrawPage();
        RenderTallyPanel();
        DrawSpriteObjects();
        PresentFrame();
        if (moving == 0) break;
    }

    SET_DW(DS_word_4F2E, 0x168);
    SET_DW(DS_word_4F30, 0x0AF);
    SET_DW(DS_word_4F32, 0x64);
    SET_DW(DS_word_4F40, 0x168);
    SET_DW(DS_word_4F42, 0x94);
    SET_DW(DS_word_4F44, 0x62);
    SET_DW(DS_word_4F52, 0x168);
    SET_DW(DS_word_4F54, 0x9B);
    SET_DW(DS_word_4F56, 0x68);
    do {                                               /* the pot and flame slide up */
        AdvancePlayerAnim(DW(DS_PlayerAnimPtr));
        ClearDrawPage();
        AnimateTallyPotFlame();
        RenderTallyPanel();
        DrawSpriteObjects();
        PresentFrame();
        SET_DW(DS_word_4F2E, (uint16_t)(DW(DS_word_4F2E) - 3));
        SET_DW(DS_word_4F40, (uint16_t)(DW(DS_word_4F40) - 3));
        SET_DW(DS_word_4F52, (uint16_t)(DW(DS_word_4F52) - 3));
    } while (DW(DS_word_4F2E) > 0x9B);

    if (DW(DS_ItemTotal) != 0) {
        SET_DW(DS_PlayerAnimPtr, 0x7C6B);
        SET_DW(DS_PlayerVelX, 0x40);
        SET_DB(DS_MotionShift, 2);
        do {
            AdvancePlayerAnim(DW(DS_PlayerAnimPtr));
            ClearDrawPage();
            AnimateTallyPotFlame();
            RenderTallyPanel();
            DrawSpriteObjects();
            PresentFrame();
            ApplyPlayerFriction();
            int16_t vx = (int16_t)DW(DS_PlayerVelX) >> 4;
            SET_DW(DS_PlayerX, (uint16_t)(DW(DS_PlayerX) + (uint16_t)vx));
            if (vx == 0) break;
        } while (1);
        SET_DW(DS_PlayerAnimPtr, 0x7CAF);

        /* tally flights: each frame, step the flights and, when none is due, hand out one item */
        uint16_t di = 0;
        for (;;) {                                    /* loc_4E3A */
            AdvancePlayerAnim(DW(DS_PlayerAnimPtr));
            ClearDrawPage();
            AnimateTallyPotFlame();
            RenderTallyPanel();
            DrawSpriteObjects();
            FlipDisplayPage();
            WaitVerticalRetrace();
            uint16_t active = 0;
            uint16_t si = FLIGHT_TABLE;
            for (unsigned k = 0; k < 0x14; k++, si = (uint16_t)(si + 0x12)) {
                if (DW((uint16_t)(si + 4)) == 0xFFFF) continue;
                uint16_t ax = DW((uint16_t)(si + 0x0E));
                if (ax < 0x80) { ax = (uint16_t)(ax + 8); SET_DW((uint16_t)(si + 0x0E), ax); }
                SET_DW((uint16_t)(si + 2), (uint16_t)(DW((uint16_t)(si + 2)) + (uint16_t)((int16_t)ax >> 4)));
                active++;
                if (DW((uint16_t)(si + 2)) < 0x91) continue;
                uint16_t bx = (uint16_t)((DW((uint16_t)(si + 4)) & 0x1FFFu) - 0x6E);
                uint8_t idx = DB((uint16_t)(bx - 0x5C8Bu));
                uint16_t add = DW((uint16_t)(2u * idx - 0x5CADu));
                uint32_t score = ((uint32_t)DW(DS_ScoreHigh) << 16 | DW(DS_ScoreLow)) + add;
                SET_DW(DS_ScoreLow, (uint16_t)score);
                SET_DW(DS_ScoreHigh, (uint16_t)(score >> 16));
                SET_DW((uint16_t)(si + 4), 0xFFFF);
                StartSoundEffect(8);
            }
            if (DW(DS_SnowFrameTick) & 7) continue;   /* tick & 7 != 0: the flight block repeats */

            /* loc_4EBA: find the next item count to spend */
            unsigned found = 0;
            uint8_t al = 0;
            for (;;) {
                if (di >= ITEM_TALLY_COUNT) di = 0;
                if (DB((uint16_t)(DS_ItemTallyCounts + di)) != 0) { found = 1; break; }
                al++;
                if (al >= ITEM_TALLY_COUNT) break;
                di++;
            }
            if (found) {
                uint16_t ax = (uint16_t)(di + 0x6E);
                uint16_t si = FLIGHT_TABLE;
                unsigned placed = 0;
                for (unsigned k = 0; k < 0x14; k++, si = (uint16_t)(si + 0x12)) {
                    if (DW((uint16_t)(si + 4)) == 0xFFFF) { placed = 1; break; }
                }
                if (placed) {
                    SET_DW((uint16_t)(si + 4), ax);
                    SET_DW(si, 0x9B);
                    SET_DW((uint16_t)(si + 2), 0);
                    SET_DW((uint16_t)(si + 0x0E), 0);
                    SET_DB((uint16_t)(DS_ItemTallyCounts + di), (uint8_t)(DB((uint16_t)(DS_ItemTallyCounts + di)) - 1));
                    di++;
                }
                continue;
            }
            if (active == 0) break;                    /* loc_4F07: no flight left: exit */
        }
    }

    /* LevelExitNoTally: wait for the walk-off, then advance the level */
    WaitVerticalRetrace();
    WaitVerticalRetrace();
    SET_DW(DS_PlayerAnimPtr, 0x7BA7);
    do {
        AdvancePlayerAnim(DW(DS_PlayerAnimPtr));
        ClearDrawPage();
        AnimateTallyPotFlame();
        RenderTallyPanel();
        DrawSpriteObjects();
        PresentFrame();
        SET_DW(DS_word_4F2E, (uint16_t)(DW(DS_word_4F2E) - 2));
        SET_DW(DS_word_4F40, (uint16_t)(DW(DS_word_4F40) - 2));
        SET_DW(DS_word_4F52, (uint16_t)(DW(DS_word_4F52) - 2));
        uint16_t step = (int16_t)DW(DS_word_4F2E) < 0 ? 3 : 2;
        SET_DW(DS_PlayerX, (uint16_t)(DW(DS_PlayerX) + step));
    } while ((int16_t)DW(DS_word_4F2E) > -0x34);

    if (DB(DS_byte_2879) != 0) return GameOverPath(ax_out);
    SET_DB(DS_CurrentLevel, (uint8_t)(DB(DS_CurrentLevel) + 1));
    /* quirk: ES is still A000h from RunIrisClose here, so the tally clear goes to video
     * memory (A000:6C12h..) and the DATA tally survives the transition (ES audit) */
    ResetItemTallyEs(0xA000u);
    return level_finish(ax_out);
}

/* Respawn (RespawnState 1): death bounce, then a game over in demo or on EndSignal, else a
 * checkpoint restart. */
static int RespawnPath(uint16_t *ax_out)
{
    PlayDeathBounce();
    if (DB(DS_byte_2879) != 0) return GameOverPath(ax_out);
    if (DB(DS_EndSignal) == 0) return RestartFromCheckpoint(ax_out);
    return GameOverPath(ax_out);
}

/* Life lost (EndSignal 1): death bounce, then the game over. */
static int LifeLostPath(uint16_t *ax_out)
{
    SET_DW(DS_NextFreeSegment, DW(DS_LevelSegment));
    PlayDeathBounce();
    return GameOverPath(ax_out);
}

/* Game over: the intro gate, the game-over screen, then score, level and lives are reset. */
static int GameOverPath(uint16_t *ax_out)
{
    (void)TryShowIntroKeyed();                /* CF is not tested by the ASM caller */
    ActivateResourceSlot(0x11);
    RunGameOverScreen();
    SET_DW(DS_PlayerFrameWord, 0x0D);
    SET_DB(DS_CurrentLevel, 0);
    SET_DW(DS_ScoreLow, 0);
    SET_DW(DS_ScoreHigh, 0);
    SET_DW(DS_ExtraLifeBlocks, 0);
    ResetItemTally();
    *ax_out = 1;
    return 1;
}

/* Checkpoint restart (after RespawnPath): the effect-source snapshot and the object snapshot are
 * kept across the reset, with the sources of certain kinds dropped (see the ASM notes). */
static int RestartFromCheckpoint(uint16_t *ax_out)
{
    /* death snapshot of the effect-source table (8F1Dh, 70 records of 7 bytes, frame word at +4)
     * into 6C12h..6C9Fh, then 55AAh. This overwrites ItemTallyCounts and ItemTotal: quirk, the
     * tally is cleared again below. */
    uint16_t di = 0x6C12, si = 0x8F1D;
    for (unsigned k = 0; k < 0x46; k++, si = (uint16_t)(si + 7), di = (uint16_t)(di + 2))
        wr16(DATA_SEG, di, DW((uint16_t)(si + 4)));
    wr16(DATA_SEG, di, 0x55AA);
    di = 0xA2A8; si = 0x8C8D;
    for (unsigned k = 0; k < 0x50; k++, si = (uint16_t)(si + 5), di = (uint16_t)(di + 1)) {
        uint8_t al = 0;
        if (DW((uint16_t)(si + 3)) != 0xFFFF) al = 1;
        wr8(DATA_SEG, di, al);
    }
    ResetLevelState();
    SET_DW(DS_PlayerX, DW(DS_CheckpointX));
    SET_DW(DS_PlayerY, DW(DS_CheckpointY));
    EnterLevelView();
    di = 0x8F1D; si = 0x6C12;
    for (unsigned k = 0; k < 0x46; k++, si = (uint16_t)(si + 2), di = (uint16_t)(di + 7)) {
        uint16_t ax = DW(si);
        uint8_t dl = (uint8_t)(DW((uint16_t)(di + 4)) - 0x35u);
        if (dl == 0x0D || dl == 0x2C || dl == 0x41 || dl == 0xA9 || dl == 0xAA || dl == 0xB6 || dl == 0xE0)
            continue;
        SET_DW((uint16_t)(di + 4), ax);
    }
    di = 0x8C8D; si = 0xA2A8;
    for (unsigned k = 0; k < 0x50; k++, si = (uint16_t)(si + 1), di = (uint16_t)(di + 5)) {
        if (DB(si) == 0) wr16(DATA_SEG, (uint16_t)(di + 3), 0xFFFF);
    }
    ResetItemTally();
    *ax_out = 0;
    return 0;
}

/* Ending picture (EndSignal 0FFh): the title picture from LEVEL, fade-in, then the intro. */
static int ShowEndingPicture(uint16_t *ax_out)
{
    SetLevelVideoMode();
    SET_DW(DS_NextFreeSegment, DW(DS_word_0039));
    uint16_t seg = 0;
    LoadSqzFile(0x6BA0, &seg);
    SET_DW(DS_LevelSegment, seg);
    ShowMode13PictureFadeIn(0x10, 0);       /* SI = 0 and CL = 10h; BX is not an input of the ASM */
    WaitForButtonPressRelease();
    FadeOutPaletteToBlack();
    (void)ShowIntroPicture();                 /* CF is not tested by the ASM caller */
    *ax_out = 1;
    return 1;
}

/* ---- tally panel (cs:5139..520B) ------------------------------------------------------- */

/* Draws the percent-collected line of the tally panel: label at DrawPage+410h, then the percent at
 * DrawPage+430h and a percent sign. Percent = 100 * (word_2A76 + word_2A7A) / (total); 100 when the
 * total is zero. Quirk: the numerator is the LOW BYTE of the sum times 100, so the percent wraps
 * once the collected count reaches 256. */
void RenderTallyPercent(void)
{
    SetCpuWriteMode();
    DrawHudText(0x7D7F, (uint16_t)(DW(DS_DrawPage) + 0x410u));
    uint16_t pct = 0x64;
    uint16_t total = (uint16_t)(DW(DS_CollectTotalMain) + DW(DS_CollectTotalDecor));
    if (total != 0) {
        uint16_t sum = (uint16_t)(DW(DS_word_2A76) + DW(DS_word_2A7A));
        uint16_t prod = (uint16_t)((sum & 0xFFu) * 100u);
        pct = (uint16_t)(prod / total);
    }
    FormatDecimal32(0, pct);
    uint16_t si = TALLY_PERCENT_AT;
    uint16_t di = (uint16_t)(DW(DS_DrawPage) + 0x430u);
    if (DB(si) == 0x30) si++;                         /* a leading zero is not drawn */
    for (;;) {
        uint8_t c = DB(si);
        si++;
        if (c == '$') break;
        BlitHudDigit((uint8_t)(c - 0x30), di);
        di = (uint16_t)(di + 2);
    }
    BlitHudSpriteGlyph(0x1A, di);
}

/* Draws a glyph string at DI: '$' ends it, a space advances one glyph cell (2 bytes), any other
 * byte is a letter drawn by BlitHudSpriteGlyph with index (byte - 41h). */
void DrawHudText(uint16_t si, uint16_t di)
{
    for (;;) {
        uint8_t c = DB(si);
        si++;
        if (c == '$') return;
        if (c != 0x20) BlitHudSpriteGlyph((uint8_t)(c - 0x41), di);
        di = (uint16_t)(di + 2);
    }
}

/* Renders the tally score line: the text at DrawPage+230h, the six low score digits and a fixed
 * 0 digit at DrawPage+23Ch, then the percent line. */
void RenderTallyPanel(void)
{
    SetCpuWriteMode();
    DrawHudText(0x7D8F, (uint16_t)(DW(DS_DrawPage) + 0x230u));
    FormatDecimal32(DW(DS_ScoreHigh), DW(DS_ScoreLow));
    uint16_t si = TALLY_SCORE_AT;
    uint16_t di = (uint16_t)(DW(DS_DrawPage) + 0x23Cu);
    for (;;) {
        uint8_t c = DB(si);
        si++;
        if (c == '$') break;
        BlitHudDigit((uint8_t)(c - 0x30), di);
        di = (uint16_t)(di + 2);
    }
    BlitHudDigit(0, di);
    RenderTallyPercent();
}

/* Clears the item tally: ItemTallyCounts (71h bytes) and ItemTotal. */
/* rep stosb at ES:ItemTallyCounts with the caller's ES; ItemTotal through DS. */
void ResetItemTallyEs(uint16_t es)
{
    for (unsigned k = 0; k < ITEM_TALLY_COUNT; k++) {
        uint16_t off = (uint16_t)(DS_ItemTallyCounts + k);
        if (es == 0xA000u) vga_write(off, 0); else wr8(es, off, 0);
    }
    SET_DW(DS_ItemTotal, 0);
}

void ResetItemTally(void) { ResetItemTallyEs(DATA_SEG); }

/* Every 4th frame advances the tally pot flame sprite id word_4F56 through 68h..6Dh (high byte
 * masked with 1Fh before the step). */
void AnimateTallyPotFlame(void)
{
    if (DB(DS_SnowFrameTick) & 3) return;
    uint16_t ax = DW(DS_word_4F56);
    ax = (uint16_t)((ax & 0x1FFFu) + 1);
    if (ax >= 0x6E) ax = 0x68;
    SET_DW(DS_word_4F56, ax);
}

/* Clears the current draw page: 8000 bytes in each of the four planes, to zero. Leaves GC write
 * mode 0 and rotate 0 set. */
void ClearDrawPage(void)
{
    port_out16(0x3C4, 0x0F02);
    port_out16(0x3CE, 0x0005);
    port_out16(0x3CE, 0x0003);
    uint16_t di = DW(DS_DrawPage);
    for (unsigned k = 0; k < 0xFA0; k++) { vga_write(di, 0); vga_write((uint16_t)(di + 1), 0); di = (uint16_t)(di + 2); }
}

/* Level setup (level start, exit and respawn). Clears the transition-state block 6BC4h..6C0Bh, copies
 * the level's mutable object tables from the template at 9203h to 815Eh (10A5h bytes), sets
 * Energy = 3, the palette, the wind and flake tables, the particles and the popup ring, and
 * ScriptPointer from LevelScriptTable[CurrentLevel]. ShakeRows is not reset (quirk). */
void ResetLevelState(void)
{
    for (unsigned k = 0; k < 0x48; k++) SET_DB((uint16_t)(0x6BC4u + k), 0);
    for (unsigned k = 0; k < 0x10A5; k++) SET_DB((uint16_t)(0x815Eu + k), DB((uint16_t)(0x9203u + k)));
    SET_DB(DS_SubAnimKey, 0);
    SET_DW(DS_PrevCameraCol, 0x55AA);
    SET_DW(DS_PopupRingHead, 0x4F76);
    ResetRenderSlots();
    SetLevelPalette();
    for (unsigned k = 0; k < 0x100; k++) {
        (void)NextWindRandom();                       /* the first draw is discarded (quirk) */
        uint8_t r2 = NextWindRandom();
        SET_DW((uint16_t)(DS_FlakeTable + 2 * k), (uint16_t)(r2 * 0x101u));
    }
    for (unsigned k = 0; k < 0x50; k++) SET_DW((uint16_t)(0x6EA9u + 2 * k), 0x55AA);
    RestoreSceneryMap();
    for (unsigned k = 0; k < 0x78; k++) SET_DB((uint16_t)(DS_DebrisRecord + k), 0xFF);
    for (unsigned k = 0; k < 0x37; k++) SET_DB((uint16_t)(DS_L5Projectile + k), 0xFF);
    SET_DB(DS_Energy, 3);
    SET_DW(DS_Level9ScriptPtr, 0xFFFF);
    uint16_t bx = DB(DS_CurrentLevel);
    SET_DW(DS_ScriptPointer, DW((uint16_t)(DS_LevelScriptTable + 2 * bx)));
    SET_DW(DS_ScriptFrameCounter, 0);
}

/* Applies the inline patch records from NextFreeSegment to the level map at LevelSegment. Each record:
 * word dest (FFFFh ends the list), word rows-and-width (low byte = rows, high byte = width), then
 * width * rows data bytes. Rows are written MAP_STRIDE apart. The byte count is CL = width with CH
 * as the caller left it: ResetLevelState's rep stosw leaves CX = 0, so CH = 0 here. */
void RestoreSceneryMap(void)
{
    uint16_t es = DW(DS_LevelSegment);
    uint16_t ds = DW(DS_NextFreeSegment);
    uint16_t si = 0;
    for (;;) {
        uint16_t di = rd16(ds, si);
        if (di == 0xFFFF) break;
        uint16_t aw = rd16(ds, (uint16_t)(si + 2));
        si = (uint16_t)(si + 4);
        uint8_t rows = (uint8_t)aw;
        uint16_t count = (uint8_t)(aw >> 8);
        do {                                          /* loc_52EB: dec al; jne */
            uint16_t d = di;
            for (uint16_t k = 0; k < count; k++) {
                wr8(es, d, rd8(ds, si));
                si++; d++;
            }
            di = (uint16_t)(di + MAP_STRIDE);
            rows = (uint8_t)(rows - 1);
        } while (rows != 0);
    }
}

/* ---- triggers, player tile, fireflies, render slots (cs:52FE..55FB) --------------------- */

/* Cave trigger: when the player's tile equals a record of the 20-entry table at 8367h (stride 7:
 * source tile, camera target, destination tile, flag) and LandingBlockTimer and PlayerAltModeFlag
 * allow it, the view scrolls to the target, the player moves to the destination tile and
 * ScrollFollowGate takes the flag. CurrentLevel 5 gets a fixed setup of its state and projectile
 * table. Then the view is redrawn. */
void TryCaveTrigger(void)
{
    if (DB(DS_LandingBlockTimer) == 0) return;
    if (DB(DS_PlayerAltModeFlag) != 0) return;
    uint16_t dx = GetPlayerTile();
    uint16_t si = 0x8367;
    int found = 0;
    for (unsigned k = 0; k < 0x14; k++, si = (uint16_t)(si + 7)) {
        if (DW(si) == dx) { found = 1; break; }
    }
    if (!found) return;
    SET_DW(DS_PlayerVelX, 0);
    SET_DW(DS_PlayerVelY, 0);
    ClearStaggeredBlocks();
    SET_DB(DS_FineScroll, 0);
    uint16_t savedLimit = DW(DS_CameraLeftLimit);
    SET_DW(DS_CameraLeftLimit, 0xEC);
    uint16_t w4 = DW((uint16_t)(si + 4));
    SET_DW(DS_PlayerX, (uint16_t)((w4 & 0xFFu) << 4));
    SET_DW(DS_PlayerY, (uint16_t)((w4 >> 8) << 4));
    uint16_t w2 = DW((uint16_t)(si + 2));
    uint8_t tcol = (uint8_t)w2, trow = (uint8_t)(w2 >> 8);
    while (DB(DS_CameraRow) != trow) {
        if (DB(DS_CameraRow) < trow) ScrollViewDown(0x10);
        else ScrollViewUp(0x10);
    }
    while (DB(DS_CameraCol) != tcol) {
        if (DB(DS_CameraCol) < tcol) ScrollViewRight();
        else ScrollViewLeft();
    }
    SET_DW(DS_CameraLeftLimit, savedLimit);
    SET_DB(DS_ScrollFollowGate, DB((uint16_t)(si + 6)));
    SET_DB(DS_LandingBlockTimer, 0);
    if (DB(DS_CurrentLevel) == 5) {
        SET_DB(DS_LevelFlags, DB(DS_LevelFlags) & 1);
        SET_DB(DS_byte_A324, 0);
        SET_DB(DS_byte_A325, 5);
        SET_DB(DS_Level5Phase, 0);
        SET_DB(DS_byte_A328, 0);
        SET_DB(DS_byte_A32A, 1);
        SET_DB(DS_byte_A329, 0);
        SET_DB(DS_byte_A32B, 0x6E);
        SET_DB(DS_byte_A32C, 8);
        for (unsigned k = 0; k < 0x69; k++) SET_DB((uint16_t)(0x5570u + k), 0xFF);
    }
    RedrawOverlayTiles();
    CopyRingWindowToPage();
    UpdateTerrainEntities();
    BuildEffectDrawList();
    UpdateObjects();
    DrawSpriteObjects();
    DrawForegroundOverSprites();
    UpdateFireflies();
    UpdateWindAndFlakes();
    RevealLevelColumns();
}

/* Proximity triggers: the 15 records at 83F3h (stride 0Ah; word at +4: FFFFh = idle, FFFEh = fired,
 * else a packed tile). A trigger whose tile is within 8 of the packed player tile fires and sets
 * ShakeMagnitude = 7. A fired trigger on every 4th frame copies one step of its reveal block from
 * the trigger bank into the level map; it counts down its byte at +8 and then goes idle. */
void UpdateProximityTriggers(void)
{
    uint16_t dx = GetPlayerTile();
    uint16_t si = 0x83F3;
    for (unsigned k = 0; k < 0x0F; k++, si = (uint16_t)(si + 0x0A)) {
        uint16_t w = DW((uint16_t)(si + 4));
        if (w == 0xFFFF) continue;
        if (w == 0xFFFE) {
            SET_DB(DS_ShakeMagnitude, 7);
            if (DB(DS_SnowFrameTick) & 3) continue;
            uint16_t es = DW(DS_LevelSegment);
            uint8_t cols = DB((uint16_t)(si + 2));
            uint8_t rows = DB((uint16_t)(si + 3));
            uint16_t di = (uint16_t)(DW(si) - 0x100u);
            do {                                      /* rows of the reveal block */
                uint8_t n = cols;
                do {
                    uint8_t t = rd8(es, (uint16_t)(di + 0x100u));
                    wr8(es, di, t);
                    RedrawTileIfVisible((uint16_t)(((uint16_t)n << 8) | t), di);
                    di++;
                    n--;
                } while (n != 0);
                di = (uint16_t)(di - cols + 0x100u);
                rows--;
            } while (rows != 0);
            SET_DW(si, (uint16_t)(DW(si) - 0x100u));
            uint16_t bx = DW((uint16_t)(si + 6));
            uint8_t n = cols;
            do {                                      /* one step of the trigger bank */
                uint8_t v = rd8(DW(DS_NextFreeSegment), bx);
                wr8(es, di, v);
                RedrawTileIfVisible(v, di);                 /* AH = 0 after the row loop */
                di++; bx++;
                n--;
            } while (n != 0);
            SET_DW((uint16_t)(si + 6), (uint16_t)(DW((uint16_t)(si + 6)) - (uint16_t)(int16_t)(int8_t)cols));
            SET_DB((uint16_t)(si + 8), (uint8_t)(DB((uint16_t)(si + 8)) - 1));
            if (DB((uint16_t)(si + 8)) == 0) SET_DW((uint16_t)(si + 4), 0xFFFF);
            continue;
        }
        uint16_t dist = (uint16_t)(0u - (uint16_t)(w - dx));
        if (dist <= 8) {
            SET_DW((uint16_t)(si + 4), 0xFFFE);
            SET_DB(DS_ShakeMagnitude, 7);
        }
    }
}

/* Player tile: DH = PlayerY >> 4 (row), DL = PlayerX >> 4 (column). */
uint16_t GetPlayerTile(void)
{
    uint8_t row = (uint8_t)((int16_t)DW(DS_PlayerY) >> 4);
    uint8_t col = (uint8_t)((int16_t)DW(DS_PlayerX) >> 4);
    return (uint16_t)((uint16_t)row << 8 | col);
}

/* Moves and draws the 20 firefly records at 6EA9h (8 bytes: x, y, direction, speed, timer; x =
 * 55AAh is free). Motion comes from the wind (byte_6BC0 / byte_6BC1) and the random seed; pixels
 * are plotted with the VGA XOR function. Leaves GC index 1 = 0 on return. */
void UpdateFireflies(void)
{
    SetCpuWriteMode();
    port_out16(0x3C4, 0x0F02);
    port_out16(0x3CE, 0x1003);
    for (uint16_t si = 0x6EA9; si < 0x6F49; si = (uint16_t)(si + 8)) {
        if (DW(si) == 0x55AA) continue;
        uint8_t t = (uint8_t)(DB((uint16_t)(si + 6)) - 1);
        SET_DB((uint16_t)(si + 6), t);
        if (t & 0x80) {                               /* js loc_54D2: re-seed the firefly */
            uint16_t seed = StepRandomSeed();
            SET_DB((uint16_t)(si + 6), (uint8_t)((seed & 7) + 3));
            uint8_t w0 = NextWindRandom() & 0x0F;
            if ((int16_t)DW(DS_PlayerX) < ((int16_t)DW(si) >> 3)) w0 = (uint8_t)(0 - w0);
            if (DB((uint16_t)(si + 7)) & 7) w0 = (uint8_t)(0 - w0);
            SET_DB(DS_byte_6BC0, w0);
            uint8_t w1 = NextWindRandom() & 0x0F;
            if ((int16_t)DW(DS_PlayerY) < (int16_t)(((int16_t)DW((uint16_t)(si + 2)) >> 3) + 0x20)) w1 = (uint8_t)(0 - w1);
            if (DB((uint16_t)(si + 7)) & 7) w1 = (uint8_t)(0 - w1);
            SET_DB(DS_byte_6BC1, w1);
            uint8_t a = (uint8_t)(StepRandomSeed() & 0x1F);
            if ((int8_t)DB(DS_byte_6BC0) < 0) a = (uint8_t)(0 - a);
            SET_DB((uint16_t)(si + 4), a);
            uint8_t spd;
            if (DB(DS_byte_6BC0) == 0) {
                spd = 2;
            } else {
                uint8_t bl = DB(DS_byte_6BC0);
                if ((int8_t)bl < 0) bl = (uint8_t)(0 - bl);
                int16_t ax = (int8_t)DB(DS_byte_6BC1);
                if (ax < 0) ax = (int16_t)(0 - ax);
                ax = (int16_t)(ax << 3);
                spd = (uint8_t)((uint16_t)ax / bl);   /* |w1| <= 15 keeps the quotient in a byte */
            }
            if (spd >= 0x20) spd = 0x20;
            if ((int8_t)DB(DS_byte_6BC1) < 0) spd = (uint8_t)(0 - spd);
            SET_DB((uint16_t)(si + 5), spd);
            if ((DB(DS_SnowFrameTick) & 0x0F) == 0 && (DB((uint16_t)(si + 7)) & 7) != 0)
                SET_DB((uint16_t)(si + 7), (uint8_t)(DB((uint16_t)(si + 7)) - 1));
        }
        /* loc_5573: move by the velocities (a signed overflow keeps the old X) */
        int32_t nx = (int32_t)(int16_t)DW(si) + (int8_t)DB((uint16_t)(si + 4));
        if (nx >= -0x8000 && nx <= 0x7FFF) SET_DW(si, (uint16_t)nx);
        SET_DW((uint16_t)(si + 2), (uint16_t)(DW((uint16_t)(si + 2)) + (uint16_t)(int8_t)DB((uint16_t)(si + 5))));
        if (DB((uint16_t)(si + 6)) & 1) {
            port_out16(0x3CE, 0x0001);
        } else {
            port_out16(0x3CE, 0x0000);
            port_out16(0x3CE, 0x0101);
        }
        uint16_t ay = (uint16_t)(((int16_t)DW((uint16_t)(si + 2)) >> 3) - (int16_t)(DW(DS_CameraRow) << 4));
        if (ay >= 0xB0) continue;
        uint16_t di = (uint16_t)((uint8_t)ay * 0x28u);
        uint16_t ax = (uint16_t)(((int16_t)DW(si) >> 3) - (int16_t)(DW(DS_CameraCol) << 4));
        if (ax >= 0x140) continue;
        uint8_t mask = (uint8_t)(0x80u >> (ax & 7));
        di = (uint16_t)(di + (ax >> 3) + DW(DS_DrawPage));
        vga_xchg(di, mask);
    }
    port_out16(0x3CE, 0x0001);
}

/* Resets the per-level render state: clears words 4F1Ch..5732h, marks the 116 render slots from
 * 4F0Ah free (sprite FFFFh, life 0), and sets PlayerX/PlayerY to LevelStartX/Y. */
void ResetRenderSlots(void)
{
    for (unsigned k = 0; k < 0x40B; k++) SET_DW((uint16_t)(PLAYER_REC + 2 * k), 0);
    uint16_t di = 0x4F0A;
    for (unsigned k = 0; k < 0x74; k++, di = (uint16_t)(di + 0x12)) {
        SET_DW((uint16_t)(di + 4), 0xFFFF);
        SET_DB((uint16_t)(di + 0x11), 0);
    }
    SET_DW(DS_PlayerX, DW(DS_LevelStartX));
    SET_DW(DS_PlayerY, DW(DS_LevelStartY));
}

/* ---- camera chain (cs:5634..581D) ------------------------------------------------------ */

/* Camera follow with CameraFixedStep = 1: vertical steps are a fixed 10h and the forced
 * down-scroll is not used. Then UpdateCameraScroll. */
void UpdateCameraFixedStep(void)
{
    SET_CB(CS_CameraFixedStep, 1);
    UpdateCameraScroll();
    SET_CB(CS_CameraFixedStep, (uint8_t)(CB(CS_CameraFixedStep) - 1));
}

/* Camera follow with CameraFixedStep = 0 (table-driven vertical steps). Falls into
 * UpdateCameraScroll. */
void UpdateCameraFollow(void)
{
    SET_CB(CS_CameraFixedStep, 0);
    UpdateCameraScroll();
}

/* Camera follow body. Returns at once while ScrollFollowGate is nonzero. Otherwise calls
 * UpdateHorizontalCamera (unless LevelFlags bit 1 is set) and UpdateVerticalCamera. */
void UpdateCameraScroll(void)
{
    if (DB(DS_ScrollFollowGate) != 0) return;
    if (!(DB(DS_LevelFlags) & 2)) UpdateHorizontalCamera();
    UpdateVerticalCamera();
}

/* Vertical camera follow. VScrollActive is set when the player's screen row leaves the dead zone
 * (threshold from PlayerVelY and LevelFlags bit 0); the camera then scrolls one step per frame
 * until the player is back inside it. LevelFlags bit 2 forces a down-scroll when CameraFixedStep
 * is 0. Steps come from the tile table (GridDirtyFlag selects the table) or 10h. */
void UpdateVerticalCamera(void)
{
    if (CB(CS_CameraFixedStep) == 0 && (DB(DS_LevelFlags) & 4)) {
        ScrollViewDown(1);
        return;
    }
    if (DW(DS_PlayerVelY) == 0) SET_DB(DS_VScrollActive, 0);
    uint16_t dx = (uint16_t)(((int16_t)DW(DS_PlayerY) >> 4) - (int16_t)DW(DS_CameraRow));
    int set = 0;
    uint16_t ax = 0;
    if (DW(DS_PlayerVelY) != 0) {
        ax = 3;
        if ((int16_t)dx >= 9) set = 1;
        else { ax = 8; set = (int16_t)dx <= 2; }
    } else if (!(DB(DS_LevelFlags) & 1)) {
        ax = 9;
        if ((int16_t)dx >= 0x0A) set = 1;
        else { ax = 8; set = (int16_t)dx <= 3; }
    } else {
        ax = 7;
        if ((int16_t)dx >= 8) set = 1;
        else { ax = 6; set = (int16_t)dx <= 5; }
    }
    if (set) {
        SET_DW(DS_word_6BF1, ax);
        SET_DB(DS_VScrollActive, (uint8_t)(DB(DS_VScrollActive) + 1));
    }
    if (DB(DS_VScrollActive) == 0) return;
    int up = 0;
    uint16_t lph = (uint16_t)((DW(DS_LevelPropHeader) + 0x0Bu) << 4);
    if (DW(DS_PlayerY) <= lph) {
        up = (int16_t)DW(DS_LevelPropHeader) < (int16_t)(uint16_t)(DW(DS_CameraRow) - 1u);
    }
    if (!up) {
        if (DB(DS_VScrollActive) == 0) return;
        int16_t w = (int16_t)DW(DS_word_6BF1);
        if (w == (int16_t)dx) { SET_DB(DS_VScrollActive, 0); return; }
        if (w > (int16_t)dx) {
            up = 1;
        } else {
            uint16_t lp = DW(DS_LevelPropHeader);
            uint16_t lim = (uint16_t)((lp + 0x0Bu) << 4);
            if (!(DW(DS_PlayerY) > lim) && DW(DS_CameraRow) > lp) { SET_DB(DS_VScrollActive, 0); return; }
            uint8_t step = 0x10;
            if (CB(CS_CameraFixedStep) == 0) {
                uint16_t y = (uint16_t)(DW(DS_PlayerY) - DB(DS_FineScroll));
                uint16_t row = (uint16_t)((DW(DS_CameraRow) + DW(DS_word_6BF1)) << 4);
                uint16_t bx = (uint16_t)(y - row);
                if (DB(DS_GridDirtyFlag) == 0) step = DB((uint16_t)(DS_byte_78C6 + bx));
                else step = DB((uint16_t)(bx + DW(DS_word_78C4)));
            }
            if (ScrollViewDown(step)) SET_DB(DS_VScrollActive, 0);
            return;
        }
    }
    /* loc_576A: the player is above the dead zone */
    uint8_t step = 0x10;
    if (CB(CS_CameraFixedStep) == 0) {
        uint16_t y = (uint16_t)(DW(DS_PlayerY) - DB(DS_FineScroll));
        uint16_t bx = (uint16_t)(((DW(DS_CameraRow) + DW(DS_word_6BF1)) << 4) - y);
        if (DB(DS_GridDirtyFlag) == 0) step = DB((uint16_t)(DS_byte_78C6 + bx));
        else step = DB((uint16_t)(bx + DW(DS_word_78C4)));
    }
    if (ScrollViewUp(step)) SET_DB(DS_VScrollActive, 0);
}

/* Horizontal camera follow. HScrollDir: 0 = idle; 1 = view scrolls right until the player's screen
 * column is 5 or less; 2 = view scrolls left until it is 0Fh or more. A direction starts when the
 * column is outside a central band, or when a sideways push is in progress (PlayerVelX nonzero). */
void UpdateHorizontalCamera(void)
{
    uint16_t dx = (uint16_t)(((int16_t)DW(DS_PlayerX) >> 4) - (int16_t)DW(DS_CameraCol));
    int16_t sdx = (int16_t)dx;
    if (!(dx >= 0x14 || DB(DS_PlayerOnPlatform) != 0 || DW(DS_PlayerVelX) != 0)) {
        SET_DB(DS_HScrollDir, 0);
        return;
    }
    uint8_t dir = DB(DS_HScrollDir);
    if (dir == 0) {                                   /* loc_57F6: start a direction? */
        int alpha1;                                   /* 1 = loc_5813, 2 = loc_580B */
        if (DW(DS_PlayerVelX) == 0) alpha1 = sdx >= 0x0A ? 1 : 2;
        else alpha1 = (DB(DS_PlayerVelXHigh) & 0x80) ? 2 : 1;
        if (alpha1 == 2) {
            if (sdx <= 4) SET_DB(DS_HScrollDir, 2);
        } else {
            if (sdx >= 0x10) SET_DB(DS_HScrollDir, 1);
        }
        return;
    }
    if (dir == 1) {
        if (sdx <= 5) { SET_DB(DS_HScrollDir, 0); return; }
        if (ScrollViewRight()) SET_DB(DS_HScrollDir, 0);
        return;
    }
    if (sdx >= 0x0F) { SET_DB(DS_HScrollDir, 0); return; }
    if (ScrollViewLeft()) SET_DB(DS_HScrollDir, 0);
}

/* Ticks the popup/trail ring of 5 records (stride 12h, 4F76h..4FBEh), walking backwards from
 * PopupRingHead. Each record's frame word (masked with 1FFFh; the mask is applied before the free
 * test, so a free record is also updated: quirk) advances by one, its Y drops by one, and at
 * frame 3Ah the record is freed (FFFFh). */
void UpdatePopupRing(void)
{
    uint16_t si = DW(DS_PopupRingHead);
    for (unsigned k = 0; k < 5; k++) {
        uint16_t ax = (uint16_t)(DW((uint16_t)(si + 4)) & 0x1FFFu);
        if (ax != 0xFFFF) {
            ax = (uint16_t)(ax + 1);
            SET_DW((uint16_t)(si + 2), (uint16_t)(DW((uint16_t)(si + 2)) - 1));
            if (ax >= 0x3A) ax = 0xFFFF;
            SET_DW((uint16_t)(si + 4), ax);
        }
        si = (uint16_t)(si - 0x12);
        if (si >= 0x4F76) continue;
        si = 0x4FBE;
    }
}

/* ---- level view entry (MODULE2 cs:3AF2) ------------------------------------------------ */

/* Enters the level view: zeroes the camera and scroll state (FineScroll too, via
 * ClearStaggeredBlocks), draws the 20x12 ring from the map, then reveals it. The view waits until
 * the camera chain has finished its pan (VScrollActive or HScrollDir set), and pans right up to
 * ten tiles when the player is far to the right of the view. */
void EnterLevelView(void)
{
    ClearStaggeredBlocks();
    SET_DW(DS_CameraCol, 0);
    SET_DW(DS_CameraRow, 0);
    SET_DW(DS_ColumnRing, 0);
    SET_DW(DS_RowRing, 0);
    ComputeRingTileOffset(0);
    uint16_t di = DW(DS_RingTileOffset);
    uint16_t es = DW(DS_LevelSegment);
    uint16_t si = 0;
    SetLatchCopyMode();
    for (unsigned row = 0; row < 0x0C; row++) {
        for (unsigned col = 0; col < 0x14; col++) {
            di = BlitTileToVram(rd8(es, si), di);
            si++;
        }
        si = (uint16_t)(si + 0xEC);
        di = (uint16_t)(di + 0x258);
    }
    do {
        UpdateCameraFixedStep();
    } while ((DB(DS_VScrollActive) | DB(DS_HScrollDir)) != 0);
    if (!(DB(DS_LevelFlags) & 2)) {
        uint16_t ax = (uint16_t)(((int16_t)DW(DS_PlayerX) >> 4) - DW(DS_CameraCol));
        if (ax >= 0x0C) {
            for (unsigned k = 0; k < 10; k++) ScrollViewRight();
        }
    }
    SET_DB(DS_GridDirtyFlag, 1);
    SET_DW(DS_PrevCameraCol, 0x55AA);
    RedrawOverlayTiles();
    CopyRingWindowToPage();
    RevealLevelColumns();
}

/* ---- adapters for the differential tests (tests/level.py) ------------------------------- */

EXPORT int c_DrawStatusBarBackground(Regs *r) { DrawStatusBarBackground(); r->ds = DATA_SEG; return 0; }
EXPORT int c_UpdateHud(Regs *r) { int st = UpdateHud(); r->ds = DATA_SEG; return st; }
EXPORT int c_ClearBonusLetterRow(Regs *r) { ClearBonusLetterRow(); r->ds = DATA_SEG; return 0; }
EXPORT int c_BlitHudGlyph(Regs *r) { BlitHudGlyph((uint8_t)r->ax, r->di); r->ds = DATA_SEG; return 0; }
EXPORT int c_BlitHudDigit(Regs *r) { BlitHudDigit((uint8_t)r->ax, r->di); r->ds = DATA_SEG; return 0; }
EXPORT int c_BlitHudSpriteGlyph(Regs *r) { BlitHudSpriteGlyph((uint8_t)r->ax, r->di); r->ds = DATA_SEG; return 0; }
EXPORT int c_FormatDecimal32(Regs *r) { FormatDecimal32(r->cx, r->dx); r->ds = DATA_SEG; return 0; }
EXPORT int c_UpdatePlayerRenderSlot(Regs *r) { UpdatePlayerRenderSlot(); r->ds = DATA_SEG; return 0; }
EXPORT int c_UpdateTerrainEntities(Regs *r) { UpdateTerrainEntities(); r->ds = DATA_SEG; return 0; }
EXPORT int c_TryLandOnTerrainEntity(Regs *r) { int cf = TryLandOnTerrainEntity(r->di); r->ds = DATA_SEG; set_cf(r, cf); return 0; }
EXPORT int c_UpdateEffectParticles(Regs *r) { UpdateEffectParticles(); r->ds = DATA_SEG; return 0; }
EXPORT int c_UpdateCameraShake(Regs *r) { UpdateCameraShake(); r->ds = DATA_SEG; return 0; }
EXPORT int c_UpdateLevelState(Regs *r)
{
    uint16_t ax = 0;
    int cf = UpdateLevelState(&ax);
    if (cf == LEVEL_UNSUPPORTED) return cf;               /* boundary: the status is the result */
    r->ax = ax; r->ds = DATA_SEG;
    set_cf(r, cf);
    return 0;
}
EXPORT int c_PlayDeathBounce(Regs *r) { PlayDeathBounce(); r->ds = DATA_SEG; return 0; }
EXPORT int c_RenderTallyPercent(Regs *r) { RenderTallyPercent(); r->ds = DATA_SEG; return 0; }
EXPORT int c_DrawHudText(Regs *r) { DrawHudText(r->si, r->di); r->ds = DATA_SEG; return 0; }
EXPORT int c_RenderTallyPanel(Regs *r) { RenderTallyPanel(); r->ds = DATA_SEG; return 0; }
EXPORT int c_ResetItemTally(Regs *r) { ResetItemTally(); r->ds = DATA_SEG; return 0; }
EXPORT int c_AnimateTallyPotFlame(Regs *r) { AnimateTallyPotFlame(); r->ds = DATA_SEG; return 0; }
EXPORT int c_ClearDrawPage(Regs *r) { ClearDrawPage(); r->ds = DATA_SEG; return 0; }
EXPORT int c_ResetLevelState(Regs *r) { ResetLevelState(); r->ds = DATA_SEG; return 0; }
EXPORT int c_RestoreSceneryMap(Regs *r) { RestoreSceneryMap(); r->ds = DATA_SEG; return 0; }
EXPORT int c_TryCaveTrigger(Regs *r) { TryCaveTrigger(); r->ds = DATA_SEG; return 0; }
EXPORT int c_UpdateProximityTriggers(Regs *r) { UpdateProximityTriggers(); r->ds = DATA_SEG; return 0; }
EXPORT int c_GetPlayerTile(Regs *r) { r->dx = GetPlayerTile(); r->ds = DATA_SEG; return 0; }
EXPORT int c_UpdateFireflies(Regs *r) { UpdateFireflies(); r->ds = DATA_SEG; return 0; }
EXPORT int c_ResetRenderSlots(Regs *r) { ResetRenderSlots(); r->ds = DATA_SEG; return 0; }
EXPORT int c_UpdateCameraFixedStep(Regs *r) { UpdateCameraFixedStep(); r->ds = DATA_SEG; return 0; }
EXPORT int c_UpdateCameraFollow(Regs *r) { UpdateCameraFollow(); r->ds = DATA_SEG; return 0; }
EXPORT int c_UpdateCameraScroll(Regs *r) { UpdateCameraScroll(); r->ds = DATA_SEG; return 0; }
EXPORT int c_UpdateVerticalCamera(Regs *r) { UpdateVerticalCamera(); r->ds = DATA_SEG; return 0; }
EXPORT int c_UpdateHorizontalCamera(Regs *r) { UpdateHorizontalCamera(); r->ds = DATA_SEG; return 0; }
EXPORT int c_UpdatePopupRing(Regs *r) { UpdatePopupRing(); r->ds = DATA_SEG; return 0; }
EXPORT int c_EnterLevelView(Regs *r) { EnterLevelView(); r->ds = DATA_SEG; return 0; }
EXPORT int c_RestartFromCheckpoint(Regs *r)
{
    uint16_t ax = 0;
    int cf = RestartFromCheckpoint(&ax);
    r->ax = ax; r->ds = DATA_SEG;
    set_cf(r, cf);
    return 0;
}
