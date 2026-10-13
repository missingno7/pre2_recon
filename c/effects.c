/* Contact, effect, debris and cell-change routines (oracle: src/MODULE3.ASM cs:8295..94F3).
 *
 * Covered: the per-frame contact pass (ProcessPlayerContacts, with its item and effect
 * handlers as labelled blocks of that function), the debris and effect pools, follower
 * hits, the ambient draw list, the cell-change groups and timers, the hitbox test, sprite
 * animation steps, the backdrop and picture pointers and the SQZ-table helpers sub_93F6,
 * sub_93B2, sub_94DC and the password code. The front-end screens with their own frame loops,
 * the picture and backdrop copies (screens.c) and the routines of the other regions are not here.
 *
 * Shared tails are kept as the ASM has them: TickCellChangeTimer jumps into the apply body
 * of ApplyCellChangeRecord (loc_8B77) and ends through its CF exits; the countdown tail
 * (loc_8B61) is shared by the burst paths. Stack-based worklists of TriggerCellChangeGroup
 * are an explicit array (one pushed cell per applied record).
 *
 * Quirks preserved (see the comments at each site): SpawnEffectBurst with CX = 0 fills every
 * free record (DEC wraps), the sub_94DC table search has no bound (a missing key never
 * returns), SkipScriptToMarker (objects.c) has no bound, DestroyObjectWithDebris reads the table
 * count with a 16-bit DS index, and ApplyCellChangeRecord writes the level map at cell offset
 * row*256+col.
 */
#include "pre2.h"
#include "vga.h"
#include "system.h"
#include "render.h"
#include "player.h"
#include "objects.h"
#include "effects.h"

/* ---- SpawnDebrisElement (cs:8875) ------------------------------------------------------- */

/* In: AX = debris frame/code, SI = source record (slot or pool). Codes 4Ah..5Ah add the
 * score from table A353h (word per code). The new entry's lifetime is 2Ch; a pool source
 * (SI >= 50A8h) marks its owner record dead. Preserves all registers. */
void SpawnDebrisElement(uint16_t ax, uint16_t si)
{
    if (ax >= 0x4A && (uint16_t)(ax - 0x4A) <= 0x10) {
        uint16_t bx = (uint16_t)((ax - 0x4A) << 1);
        uint16_t pts = DW((uint16_t)(bx - 0x5CAD));          /* [bx-5CADh] = A353h */
        uint32_t sum = (uint32_t)DW(DS_ScoreLow) + pts;
        SET_DW(DS_ScoreLow, (uint16_t)sum);
        SET_DW(DS_ScoreHigh, (uint16_t)(DW(DS_ScoreHigh) + (uint16_t)(sum >> 16)));
    }
    uint16_t di = 0x5450;
    int found = 0;
    for (int n = 0; n < 16; n++, di = (uint16_t)(di + 0x12)) {
        if (DW((uint16_t)(di + 4)) == 0xFFFF) { found = 1; break; }
    }
    if (!found) return;
    SET_DW((uint16_t)(di + 4), ax);
    SET_DW(di, DW(si));
    SET_DW((uint16_t)(di + 2), DW((uint16_t)(si + 2)));
    SET_DW((uint16_t)(di + 0x0C), 0x2C);
    SET_DW(DS_LastDebrisSlot, di);
    if (si >= 0x50A8) {
        uint16_t owner = DW((uint16_t)(si + 9));
        if (owner != 0xFFFF) SET_DW((uint16_t)(owner + 4), 0xFFFF);
    }
}

/* ---- SpawnEffectBurst (cs:8D1B) --------------------------------------------------------- */

/* In: AX = velocity X seed, *DX = velocity Y seed, CX = particle count, EffectOriginX/Y,
 * EffectFrame. Fills free records of the 32-entry effect pool (50A8h..52E8h). Out: *DX as the original
 * leaves it (only the even particles after the first change it); AX, CX, BX, DI, BP are
 * preserved. CX = 0 wraps to FFFFh and fills every free record (DEC then JE). */
void SpawnEffectBurst(uint16_t ax, uint16_t *dxp, uint16_t cx)
{
    uint16_t dx = *dxp;
    uint16_t di = 0;
    uint16_t bx = 0x50A8;
    for (;;) {
        if (DW((uint16_t)(bx + 4)) == 0xFFFF) {
            SET_DW((uint16_t)(bx + 4), DW(DS_EffectFrame));
            SET_DB((uint16_t)(bx + 0x11), 0);
            SET_DW((uint16_t)(bx + 0x0C), 0xC6);
            SET_DW(bx, DW(DS_EffectOriginX));
            SET_DW((uint16_t)(bx + 2), DW(DS_EffectOriginY));
            SET_DW((uint16_t)(bx + 6), ax);
            SET_DW((uint16_t)(bx + 0x0E), dx);
            SET_DW((uint16_t)(bx + 9), 0xFFFF);
            ax = (uint16_t)-ax;
            di++;
            if ((di & 1) == 0) {                     /* even particles reduce the seeds */
                if (di > 0x0C) ax = 0;               /* unsigned: past 12 AX is zeroed first */
                ax = (uint16_t)(ax - 0x10);
                dx = (uint16_t)(dx - 0x10);
            }
            cx = (uint16_t)(cx - 1);
            if (cx == 0) break;
        }
        bx = (uint16_t)(bx + 0x12);
        if (bx >= 0x52E8) break;
    }
    *dxp = dx;
}

/* ---- SetEffectOriginFromRecord (cs:8BF6) ------------------------------------------------ */

/* EffectOriginX = column * 16 (low byte of the cell word), EffectOriginY = row * 16. */
void SetEffectOriginFromRecord(uint16_t di)
{
    uint16_t cell = DW((uint16_t)(di + 3));
    SET_DW(DS_EffectOriginX, (uint16_t)((cell & 0xFF) << 4));
    SET_DW(DS_EffectOriginY, (uint16_t)(((cell >> 8) & 0xFF) << 4));
}

/* ---- RandomDebrisFrame (cs:8C13) -------------------------------------------------------- */

/* AX = 2080h + r, r in 0..5Eh, drawn by rejection from NextWindRandom (7Fh mask). */
uint16_t RandomDebrisFrame(void)
{
    for (;;) {
        uint16_t ax = (uint16_t)(NextWindRandom() & 0x7F);
        if (ax >= 0x5F) continue;
        return (uint16_t)(ax + 0x2080);
    }
}

/* ---- DestroyObjectWithDebris (cs:8C72) -------------------------------------------------- */

/* In: DI = object record, BX = its sprite-definition record ([bx+8] debris class, [bx+4]
 * flags), SI = the caller's record (its +5 bit 80h picks the knockback direction).
 * The debris count is table A3F1h indexed by (object +10h >> 3) & 7. The object's X and Y
 * are restored after the debris loop; only the debris entries move. */
void DestroyObjectWithDebris(uint16_t di, uint16_t bx, uint16_t si)
{
    uint16_t ax = (uint16_t)((int16_t)(int8_t)DB((uint16_t)(bx + 8)) + 0x4A);
    uint16_t dx = DB((uint16_t)((((DB((uint16_t)(di + 0x10)) >> 3) & 7)) - 0x5C0F));  /* [bx-5C0Fh] = A3F1h */
    uint16_t ox = DW(di), oy = DW((uint16_t)(di + 2));
    do {
        SpawnDebrisElement(ax, di);
        uint16_t lds = DW(DS_LastDebrisSlot);
        SET_DW((uint16_t)(lds + 0x0C), (uint16_t)(DW((uint16_t)(lds + 0x0C)) - (uint16_t)(dx << 2)));
        SET_DW((uint16_t)(di + 2), (uint16_t)(DW((uint16_t)(di + 2)) + 7));
        SET_DW(di, (uint16_t)(DW(di) + 9));
        dx = (uint16_t)(dx - 1);
    } while (dx != 0);
    SET_DW(di, ox);
    SET_DW((uint16_t)(di + 2), oy);
    SET_DB((uint16_t)(di + 0x0E), 0xFF);

    if (!(DB((uint16_t)(bx + 4)) & 1)) {
        SET_DW(DS_EffectOriginX, DW(di));
        SET_DW(DS_EffectOriginY, DW((uint16_t)(di + 2)));
        uint16_t dxv = 0xFF80;
        SET_DW(DS_EffectFrame, 0x2046);
        SpawnEffectBurst(0x30, &dxv, 6);
        return;
    }
    /* loc_8CE1: knockback record */
    SkipScriptToMarker(di);
    uint8_t b4 = DB((uint16_t)(bx + 4));
    uint8_t keep_ah = (uint8_t)(b4 & 0xF7);
    if ((uint8_t)(b4 & 0xC8) != 0x88) SET_DB((uint16_t)(bx + 4), keep_ah);
    uint16_t pa = DB(DS_PlayerAttackPower);
    if (pa > 0x19) pa = 0x19;
    uint16_t v = (uint16_t)((uint16_t)(-pa) << 3);
    SET_DW((uint16_t)(di + 0x0A), v);
    uint16_t half = (uint16_t)((int16_t)v >> 1);
    if (!(DB((uint16_t)(si + 5)) & 0x80)) half = (uint16_t)-half;
    SET_DW((uint16_t)(di + 8), half);
}

/* ---- ApplyFollowerHitToObjects (cs:8C21) ------------------------------------------------ */

/* In: SI = follower slot. Tests the 12 render slots at 4FD0h; the first contactable hit
 * takes damage PlayerAttackPower; the follower dies ([si+4] = FFFFh). Out: CF = 1 on a hit. */
int ApplyFollowerHitToObjects(uint16_t si)
{
    uint16_t di = 0x4FD0;
    for (int n = 0; n < 12; n++, di = (uint16_t)(di + 0x12)) {
        if (DW((uint16_t)(di + 4)) == 0xFFFF) continue;
        if (DB((uint16_t)(di + 0x0E)) == 0xFF) continue;
        uint16_t bx = DW((uint16_t)(di + 6));
        if (DB((uint16_t)(bx + 4)) & 0x10) continue;
        if (!TestHitboxOverlap(si, di)) continue;
        SET_DB((uint16_t)(di + 5), DB((uint16_t)(di + 5)) | 0x40);
        uint8_t pa = DB(DS_PlayerAttackPower);
        uint8_t hp = DB((uint16_t)(di + 0x0F));
        SET_DB((uint16_t)(di + 0x0F), (uint8_t)(hp - pa));
        if (hp >= pa) {                              /* no borrow: knock back by vel/4 */
            uint16_t ax = (uint16_t)((int16_t)DW((uint16_t)(di + 8)) >> 2);
            SET_DW(di, (uint16_t)(DW(di) - ax));
        } else {
            StartSoundEffect(2);
            DestroyObjectWithDebris(di, bx, si);
        }
        SET_DW((uint16_t)(si + 4), 0xFFFF);
        return 1;
    }
    return 0;
}

/* ---- TestHitboxOverlap (cs:8D7B) -------------------------------------------------------- */

/* In: SI, DI = records (+0 X, +2 Y, +4 frame word; the frame index (frame & 1FFFh) * 2 selects
 * FrameWidths, FrameHeights and FrameHotX). Out: CF = 1 when the boxes overlap. Side effects:
 * LandingFlag = 0 on entry; with FollowerOverlapMode 0 and SI standing on DI the landing
 * test sets LandingFlag and LandingDepth. Registers are preserved. FollowerOverlapMode 1
 * uses the full width (the player test halves it). The upper record is the one with the
 * greater Y (both records are swapped for the Y test; the X test uses the un-swapped
 * pointers by picking the record with the smaller left edge). */
int TestHitboxOverlap(uint16_t si, uint16_t di)
{
    SET_DB(DS_LandingFlag, 0);
    uint16_t ax = (uint16_t)(DW(si) - DW(di));
    if ((int16_t)ax < 0) ax = (uint16_t)-ax;
    if (ax >= 0x40) return 0;
    ax = (uint16_t)(DW((uint16_t)(si + 2)) - DW((uint16_t)(di + 2)));
    if ((int16_t)ax < 0) ax = (uint16_t)-ax;
    if (ax >= 0x46) return 0;

    uint16_t s = si, d = di;
    uint16_t ys = DW((uint16_t)(si + 2)), yd = DW((uint16_t)(di + 2));
    uint16_t fbx = DW((uint16_t)(si + 4));
    if ((int16_t)ys < (int16_t)yd) {                 /* xchg dx,ax; xchg si,di */
        fbx = DW((uint16_t)(di + 4));
        uint16_t t = ys; ys = yd; yd = t;
        s = di; d = si;
    }
    uint16_t hidx = (uint16_t)((fbx & 0x1FFF) << 1);      /* and bh,1Fh; shl bx,1 */
    uint16_t h = DB((uint16_t)(DS_FrameHeights + hidx));
    ax = (uint16_t)(ys - h);
    if ((int16_t)ax >= (int16_t)yd) return 0;

    if (DB(DS_FollowerOverlapMode) == 0) {
        uint16_t depth = (uint16_t)(yd - ax);
        int land = 0;
        if ((int16_t)DW(DS_PlayerVelY) >= 0x80) land = 1;
        else if (depth <= (uint16_t)(h >> 1) && s != 0x4F1C) land = 1;   /* ja: too deep; je: player */
        if (land) {
            SET_DB(DS_LandingFlag, (uint8_t)(DB(DS_LandingFlag) + 1));
            SET_DW(DS_LandingDepth, depth);
        }
    }

    uint16_t sidx = (uint16_t)((DW((uint16_t)(s + 4)) & 0x1FFF) << 1);     /* bp */
    uint16_t sx = (uint16_t)(DW(s) - DB((uint16_t)(DS_FrameHotX + sidx)));
    uint16_t didx = (uint16_t)((DW((uint16_t)(d + 4)) & 0x1FFF) << 1);
    uint16_t dxx = (uint16_t)(DW(d) - DB((uint16_t)(DS_FrameHotX + didx)));  /* sub al; sbb ah */
    uint16_t wd = DB((uint16_t)(DS_FrameWidths + didx));
    if ((int16_t)dxx >= (int16_t)sx) {               /* swap: the smaller left edge is SI's */
        uint16_t t = dxx; dxx = sx; sx = t;
        wd = DB((uint16_t)(DS_FrameWidths + sidx));
    }
    uint16_t w = wd;
    if (DB(DS_FollowerOverlapMode) == 0) w = (uint16_t)((int16_t)w >> 1);
    dxx = (uint16_t)(dxx + w);
    if ((int16_t)dxx <= (int16_t)sx) return 0;
    return 1;
}

/* ---- UpdateFollowerSlots (cs:88D7) ------------------------------------------------------ */

/* Runs the four follower slots at 4F2Eh (stride 12h), then the slot at 4F0Ah while the
 * player is not in alt mode. Each live follower is tested against the objects, and when it
 * does not hit it is tested against the cell-change records. A hit or a cell change on the
 * 4F0Ah slot lowers PlayerVelY to FFB0h while it is nonzero. FollowerOverlapMode is 1 during
 * the scan and 0 at exit. */
void UpdateFollowerSlots(void)
{
    SET_DB(DS_FollowerOverlapMode, 1);
    uint16_t si = 0x4F2E;
    for (int n = 0; n < 4; n++, si = (uint16_t)(si + 0x12)) {
        if (DW((uint16_t)(si + 4)) == 0xFFFF) continue;
        if (!ApplyFollowerHitToObjects(si)) TriggerCellChangeGroup(si);
    }
    if (DB(DS_PlayerAltModeFlag) == 0) {
        si = 0x4F0A;
        if (DW((uint16_t)(si + 4)) != 0xFFFF) {
            int bump = ApplyFollowerHitToObjects(si);
            if (!bump) bump = TriggerCellChangeGroup(si);
            if (bump && DW(DS_PlayerVelY) != 0) SET_DW(DS_PlayerVelY, 0xFFB0);
        }
    }
    SET_DB(DS_FollowerOverlapMode, 0);
}

/* ---- BuildEffectDrawList (cs:8922) ------------------------------------------------------ */

/* Builds the ambient draw list at 52E8h (18-byte records, FFFFh frame = end) from the 70
 * effect sources at 8F1Dh (7 bytes: +0 X, +2 Y, +4 frame or FFFFh free, +6 sway). A source
 * inside the camera window (column offset 0..16h, row offset 0..2Bh) is copied, at most 20.
 * The sway byte steps on even SnowFrameTick values; Y advances by the sway. Unused records
 * up to 20 get FFFFh terminators, except when the 20th record was filled (then no terminator). */
void BuildEffectDrawList(void)
{
    uint16_t di = 0x52E8, si = 0x8F1D;
    uint16_t bx = 0x14;
    for (int n = 0; n < 0x46; n++, si = (uint16_t)(si + 7)) {
        if (DW((uint16_t)(si + 4)) == 0xFFFF) continue;
        uint16_t cam_col = DW(DS_CameraCol);
        uint16_t u = (uint16_t)((int16_t)DW(si) >> 4);
        if (u < cam_col) continue;                                   /* jb: unsigned borrow */
        if ((int16_t)(uint16_t)(u - cam_col) > 0x16) continue;
        uint16_t v = (uint16_t)((int16_t)DW((uint16_t)(si + 2)) >> 4);
        uint16_t cam_row = DW(DS_CameraRow);
        if (v < cam_row) continue;
        if ((int16_t)(uint16_t)(v - cam_row) > 0x2B) continue;

        SET_DW(di, DW(si));
        uint16_t sway = 0;
        if (!(DB(DS_SnowFrameTick) & 1)) {
            uint16_t a = (uint16_t)(int16_t)(int8_t)DB((uint16_t)(si + 6));   /* cbw; inc ax */
            a = (uint16_t)(a + 1);
            SET_DB((uint16_t)(si + 6), (uint8_t)a);
            sway = a;
            if ((int8_t)(uint8_t)a >= 4) {                                  /* jl: keep */
                sway = (uint16_t)(-a + 1);
                SET_DB((uint16_t)(si + 6), (uint8_t)sway);
            }
        }
        uint16_t y = (uint16_t)(sway + DW((uint16_t)(si + 2)));
        SET_DW((uint16_t)(si + 2), y);
        SET_DW((uint16_t)(di + 2), y);
        SET_DW((uint16_t)(di + 4), DW((uint16_t)(si + 4)));
        SET_DW((uint16_t)(di + 9), si);
        di = (uint16_t)(di + 0x12);
        bx = (uint16_t)(bx - 1);
        if (bx == 0) return;
    }
    do {
        SET_DW((uint16_t)(di + 4), 0xFFFF);
        di = (uint16_t)(di + 0x12);
        bx = (uint16_t)(bx - 1);
    } while (bx != 0);
}

/* ---- SpawnFourDebrisParticles (cs:94F3) ------------------------------------------------- */

/* Four single-particle bursts at EffectOriginX/Y with random frames and a fan of seeds. The
 * AX and DX seeds are restored at exit (the ASM pushes and pops them). */
void SpawnFourDebrisParticles(void)
{
    uint16_t ax = 0x20, dx = 0xFF60;
    for (int n = 0; n < 4; n++) {
        uint16_t frame = RandomDebrisFrame();
        SET_DW(DS_EffectFrame, frame);
        SpawnEffectBurst(ax, &dx, 1);
        ax = (uint16_t)-ax;
        if ((int16_t)ax >= 0) {                      /* js: skip the reduction when negative */
            ax = (uint16_t)(ax - 0x10);
            dx = (uint16_t)(dx - 0x10);
        }
    }
}

/* ---- Cell-change records (cs:899E..8B6D, apply at cs:8B77) ------------------------------ */

/* loc_8B77: writes the record's tile into the level map at its cell and, when the cell is
 * inside the camera window (12 rows, 20 columns), blits it to the ring. The cell word is
 * row*256 + column. Sets CF = 1 (the caller treats the record as applied). */
static void cell_change_apply(uint16_t di)
{
    SET_DW(DS_word_2A76, (uint16_t)(DW(DS_word_2A76) + 1));
    uint16_t cell = DW((uint16_t)(di + 3));
    SET_DW((uint16_t)(di + 3), 0xFFFF);
    uint8_t tile = DB((uint16_t)(di + 1));
    wr8(DW(DS_LevelSegment), cell, tile);
    uint8_t col = (uint8_t)cell, row = (uint8_t)(cell >> 8);
    uint8_t cam_col = DB(DS_CameraCol), cam_row = DB(DS_CameraRow);
    if (col < cam_col) return;
    if ((uint8_t)(col - cam_col) >= 0x14) return;
    if (row < cam_row) return;
    if ((uint8_t)(row - cam_row) >= 0x0C) return;
    SetLatchCopyMode();
    /* ring offset: 640 * (row mod 12) + 2 * (column mod 20) (the div/mul chain of the ASM) */
    uint16_t off = (uint16_t)(640 * (row % 12) + 2 * (col % 20));
    BlitTileToRing(tile, off);                       /* AH of the pushed BX is ignored */
    SET_DB(DS_AnimVisibleFlag, 1);
    SET_DB(DS_GridDirtyFlag, 1);
    SET_DW(DS_PrevCameraCol, 0x55AA);
}

int ApplyCellChangeRecord(uint16_t di)
{
    cell_change_apply(di);
    return 1;
}

/* Burst frame for the cell change: 132h on level 3, 12Ch on levels 6 and 7, else 134h. */
static uint16_t cell_change_frame(void)
{
    uint8_t lv = DB(DS_CurrentLevel);
    if (lv == 3) return 0x132;
    if (lv == 6 || lv == 7) return 0x12C;
    return 0x134;
}

/* Tail after a countdown burst (loc_8B61): the countdown byte [di+2] drops by one; CF = 1
 * when it borrowed (was 0), and then the record is applied. */
static int cell_change_countdown(uint16_t di)
{
    uint8_t b = DB((uint16_t)(di + 2));
    SET_DB((uint16_t)(di + 2), (uint8_t)(b - 1));
    if (b < 1) { cell_change_apply(di); return 1; }
    return 0;
}

/* loc_8B43: sets EffectFrame, spawns the directed burst (count CX) and runs the countdown.
 * The direction is 30h when the reference value (PlayerFacing, or [si+6] for followers
 * at or above 4F1Ch) is negative, else -30h. */
static int cell_change_tail_b43(uint16_t di, uint16_t si, uint16_t ax, uint16_t cx)
{
    SET_DW(DS_EffectFrame, ax);
    int negative;
    if (si >= 0x4F1C) negative = (int16_t)DW((uint16_t)(si + 6)) < 0;
    else negative = (int16_t)DW(DS_PlayerFacing) < 0;
    ax = negative ? 0x30 : (uint16_t)-0x30;
    uint16_t dx = 0xFF90;
    SpawnEffectBurst(ax, &dx, cx);
    return cell_change_countdown(di);
}

/* TickCellChangeTimer: advances the cell-change record at DI (+1 tile, +2 timer byte, +3
 * cell word) and reports whether its change is due (CF). SI is the caller's follower slot.
 * Armed records (bit 80h) count down and burst sparks; unarmed records advance at most once
 * per 6 SnowFrameTick steps (LastEffectTick). */
int TickCellChangeTimer(uint16_t di, uint16_t si)
{
    uint16_t cell = DW((uint16_t)(di + 3));
    EmitPlayerTrailAt((uint16_t)(((cell & 0xFF) << 4) + 8), (uint16_t)(((cell >> 8) << 4) + 0x0C));

    if (DB((uint16_t)(di + 2)) & 0x80) {
        uint8_t b = (uint8_t)(DB((uint16_t)(di + 2)) - 1);
        SET_DB((uint16_t)(di + 2), b);
        if (b & 0x80) return 1;                      /* loc_8BE8: due, not applied */
        SetEffectOriginFromRecord(di);
        uint16_t r;
        do { r = (uint16_t)(NextWindRandom() & 7); } while (r == 0);
        SET_DW(DS_EffectFrame, (uint16_t)(r + 0x6E));
        SET_DW(DS_EffectOriginY, (uint16_t)(DW(DS_EffectOriginY) - 0x70));
        uint16_t dxv = 0;
        SpawnEffectBurst(0, &dxv, 1);
        cell_change_apply(di);
        return 1;
    }

    uint16_t ticks = DW(DS_SnowFrameTick);
    uint16_t diff = (uint16_t)(ticks - DW(DS_LastEffectTick));
    if ((int16_t)diff < 0) diff = (uint16_t)-diff;
    if (diff < 6) return 0;
    SET_DW(DS_LastEffectTick, ticks);
    SetEffectOriginFromRecord(di);

    uint8_t f = DB((uint16_t)(di + 2));
    if (!(f & 0x40)) {                               /* loc_8B40: random debris frame */
        uint16_t frame = RandomDebrisFrame();
        return cell_change_tail_b43(di, si, frame, 1);
    }
    if (f == 0x40) {                                 /* loc_8AF7: the level burst, then 136h/E5h */
        SET_DB((uint16_t)(di + 2), 0);
        SET_DW(DS_EffectFrame, cell_change_frame());
        uint16_t dxv = 0xFFD0;
        SpawnEffectBurst(0x20, &dxv, 2);
        uint16_t ax = 0x136, cx = 2;
        if (DB(DS_CurrentLevel) != 8) { ax = 0xE5; cx = 4; }
        return cell_change_tail_b43(di, si, ax, cx);
    }
    /* loc_8B19: other 40h-flagged records burst with the level frame, count 4 */
    SET_DW(DS_EffectFrame, cell_change_frame());
    uint16_t dxv = 0xFFA0;
    SpawnEffectBurst(0x30, &dxv, 4);
    return cell_change_countdown(di);
}

/* TriggerCellChangeGroup (cs:899E): the follower at SI tests the 80 records at 8C8Dh. A
 * record within one column and one row of the follower and within 16 pixels vertically kills
 * the follower and ticks its record; when that reports due, every not-yet-applied record in
 * the 3x3 neighbourhood of the record is applied, and each applied record becomes the next
 * pivot (the ASM pushes its cell word and rescans from the top until the stack is empty).
 * Out: CF = 1 when a cluster was applied. */
int TriggerCellChangeGroup(uint16_t si)
{
    uint8_t al = (uint8_t)((int16_t)DW(si) >> 4);
    uint16_t bp = (uint16_t)(DW((uint16_t)(si + 2)) - 0x10);
    uint16_t di = 0x8C8D;
    for (int k = 0; k < 0x50; k++, di = (uint16_t)(di + 5)) {
        uint16_t dx = DW((uint16_t)(di + 3));
        if (dx == 0xFFFF) continue;
        uint8_t bl = (uint8_t)((uint8_t)dx - al);
        if (bl & 0x80) bl = (uint8_t)-bl;
        if (bl > 1) continue;
        uint16_t bx = (uint16_t)((dx >> 8) << 4);
        bx = (uint16_t)(bx - bp);
        if ((int16_t)bx < 0) bx = (uint16_t)-bx;
        if (bx >= 0x10) continue;
        SET_DW((uint16_t)(si + 4), 0xFFFF);
        if (!TickCellChangeTimer(di, si)) continue;

        /* cluster: applied flags at 0A2A8h (80 bytes), worklist of pushed cells */
        SET_ES(DATA_SEG);                 /* mov ax, ds / mov es, ax: ES = DATA stays to the return (UpdateFollowerSlots does not restore it) */
        for (int j = 0; j < 0x50; j++) SET_DB((uint16_t)(DS_byte_A2A8 + j), 0);
        uint16_t stack[0x51];
        int depth = 0;
        uint16_t pivot = dx;
        for (;;) {
            uint16_t sj = 0x8C8D;
            for (int j = 0; j < 0x50; j++, sj = (uint16_t)(sj + 5)) {
                uint16_t cell = DW((uint16_t)(sj + 3));
                if (cell == 0xFFFF || cell == pivot) continue;
                uint8_t c = (uint8_t)((uint8_t)cell - (uint8_t)pivot);
                if (c & 0x80) c = (uint8_t)-c;
                if (c > 1) continue;
                uint8_t r = (uint8_t)((cell >> 8) - (pivot >> 8));
                if (r & 0x80) r = (uint8_t)-r;
                if (r > 1) continue;
                if (DB((uint16_t)(DS_byte_A2A8 + j)) != 0) continue;
                SET_DB((uint16_t)(DS_byte_A2A8 + j), (uint8_t)(DB((uint16_t)(DS_byte_A2A8 + j)) + 1));
                stack[depth++] = cell;
                ApplyCellChangeRecord(sj);
            }
            if (depth == 0) break;
            pivot = stack[--depth];
        }
        return 1;
    }
    return 0;
}

/* ---- Contact pass (cs:8295, handlers in the same block) -------------------------------- */

/* ProcessPlayerContacts: once per frame. While PlayerHurtTimer is nonzero only the pool is
 * tested. The pool loop runs 52 records from 50A8h: the 32 effect records, then the 20 draw-list
 * records at 52E8h (as the original, so a draw-list record can be contacted). Labels follow the
 * ASM; loc_8618 etc. are the handler blocks. */
void ProcessPlayerContacts(void)
{
    uint16_t si, di, cx, ax, bx, dx;
    uint8_t dl;

    if (DB(DS_PlayerHurtTimer) != 0) goto loc_83D7;
    si = 0x4F1C;
    di = 0x4FD0;
    cx = 12;
loc_82A8:
    if (DW((uint16_t)(di + 4)) == 0xFFFF) goto loc_83CE;
    if (!(DB((uint16_t)(di + 5)) & 0x20)) goto loc_83CE;
    bx = DW((uint16_t)(di + 6));
    if (DB((uint16_t)(bx + 4)) & 0x10) goto loc_83CE;
    if (DB((uint16_t)(di + 0x0E)) == 0xFF) goto loc_83CE;
    if (!TestHitboxOverlap(si, di)) goto loc_83CE;
    if (DW(DS_word_6BE2) != 0) {
        DestroyObjectWithDebris(di, bx, si);
        goto loc_83CE;
    }
    if (DB(DS_LandingFlag) == 0) goto loc_838A;
    if ((int8_t)DB(DS_byte_4F2B) < 0) goto loc_838A;
    if (DB(DS_PlayerAirFlags) == 0) goto loc_8348;
    if ((int16_t)DW(DS_PlayerVelY) <= 0x20) goto loc_8343;

    /* stomp: debris by the low bits of the record's +10h counter */
    dl = DB((uint16_t)(di + 0x10)) & 3;
    SpawnDebrisElement((uint16_t)(dl * 2 + 0x52), si);
    SET_DB((uint16_t)(di + 5), DB((uint16_t)(di + 5)) | 0x40);
    if (dl != 2) {
        SET_DB((uint16_t)(di + 0x10), (uint8_t)(DB((uint16_t)(di + 0x10)) + 1));   /* loc_8340 */
        goto loc_8343;
    }
    SET_DB((uint16_t)(di + 0x0E), 0xFF);
    SkipScriptToMarker(di);
    bx = DW((uint16_t)(di + 6));
    SET_DB((uint16_t)(bx + 4), DB((uint16_t)(bx + 4)) & 0xF7);
    SET_DW((uint16_t)(di + 0x0A), 0xFF38);
    ax = DW(DS_PlayerVelY);
    if ((int16_t)ax < 0) ax = (uint16_t)-ax;
    dx = DW(DS_PlayerX);
    if (!((int16_t)DW(di) > (int16_t)dx)) ax = (uint16_t)-ax;
    dx = ax;
    ax = (uint16_t)(ax + (uint16_t)(dx << 1));
    SET_DW((uint16_t)(di + 8), ax);
    return;

loc_8343:
    ax = 0xFFA0;
    goto loc_837A;

loc_8348:
    StartSoundEffect(3);
    {
        uint8_t al = (uint8_t)(DB((uint16_t)(di + 0x10)) >> 2);
        if (al != 0x0B) {
            SET_DB((uint16_t)(di + 0x10), (uint8_t)(DB((uint16_t)(di + 0x10)) + 4));
            uint16_t bxi = (uint16_t)(al + 1);
            if (!(bxi & 1)) {                            /* shr al,1 leaves CF clear */
                ax = DW((uint16_t)(bxi - 0x5C1B));      /* [bx-5C1Bh] = A3E5h */
                SpawnDebrisElement(ax, si);
            }
        }
    }
    ax = 0xFFC0;
    if (DB(DS_byte_27EA) != 0) ax = 0xFF20;
loc_837A:
    SET_DW(DS_PlayerVelY, ax);
    SET_DB(DS_FallCounter, 0);
    ax = DW(DS_LandingDepth);
    SET_DW(DS_PlayerY, (uint16_t)(DW(DS_PlayerY) - ax));
    return;

loc_838A:
    StartSoundEffect(9);
    di = DW((uint16_t)(di + 6));
    SET_DB((uint16_t)(di + 4), DB((uint16_t)(di + 4)) & 0xFE);
    SET_DB(DS_PlayerHurtTimer, 0x2C);
    SET_DB(DS_AttackAnimLock, 0);
    SET_DW(DS_PlayerVelY, 0xFF80);
    ax = (uint16_t)(DW(DS_PlayerVelX) << 2);
    ax = (uint16_t)-ax;
    SET_DW(DS_PlayerVelX, ax);
    SET_DB(DS_PlayerAirFlags, 0);
    {
        uint8_t alt = DB(DS_PlayerAltModeFlag);
        SET_DB(DS_PlayerAltModeFlag, 0);
        if (alt != 0) return;
        uint8_t en = (uint8_t)(DB(DS_Energy) - 1);
        SET_DB(DS_Energy, en);
        if (!(en & 0x80)) return;
        TriggerPlayerRespawnCore();
    }
    return;

loc_83CE:
    di = (uint16_t)(di + 0x12);
    cx = (uint16_t)(cx - 1);
    if (cx == 0) goto loc_83D7;
    goto loc_82A8;

loc_83D7:
    si = 0x50A8;
    di = 0x4F1C;
    cx = 0x34;
loc_83E0:
    if (DW((uint16_t)(si + 4)) == 0xFFFF) goto loc_860E;
    if ((int16_t)DW((uint16_t)(si + 0x0C)) > 0xBC) goto loc_860E;
    if (!(DB((uint16_t)(si + 5)) & 0x20)) goto loc_860E;
    if (!TestHitboxOverlap(si, di)) goto loc_860E;
    ax = DW((uint16_t)(si + 4)) & 0x1FFF;
    if (ax == 0xE5) goto loc_860E;
    if (ax == 0x12C || ax == 0x132 || ax == 0x134 || ax == 0x136) goto loc_860E;
    ax = DW((uint16_t)(si + 4));                     /* xchg with FFFFh: the record is consumed */
    SET_DW((uint16_t)(si + 4), 0xFFFF);
    SET_DW(DS_EffectFrame, ax);
    ax &= 0x1FFF;
    if (ax == 0x1CA) goto loc_8618;
    if (ax == 0x1CB) goto loc_8618;
    ax = (uint16_t)(ax - 0x35);
    if (ax == 0x91) goto loc_885F;
    if ((int16_t)DW(DS_PlayerY) > 0x20 && ax == 0xE2) goto loc_882A;
    if (ax == 0x102) goto loc_8859;
    if (ax == 0xE4) goto loc_87FD;
    if (ax == 0xAE) goto loc_87E6;
    if (ax == 0x0D) goto loc_87AE;
    if (ax == 0xB6) goto loc_87BC;
    if (ax == 0x2C) goto loc_87CA;
    if (ax == 0xE0) goto loc_87D8;
    if (ax <= 0x14) goto loc_85DA;
    if (ax <= 0x2C) goto loc_8524;
    if (ax <= 0x32) goto loc_854F;
    if (ax <= 0x40) goto loc_8582;
    if (ax <= 0x4A) goto loc_850A;
    if (ax <= 0xA6) goto loc_85B0;
    if (ax == 0xAD) goto loc_84F6;
    if (ax == 0xA7 || ax == 0xA8) goto loc_864F;
    if (ax == 0xA9) goto loc_86B7;
    if (ax == 0xAA) goto loc_870A;
    if (ax == 0xB5) goto loc_876C;
    if (ax == 0xB4) goto loc_8790;
    goto loc_860E;

loc_8618:
    /* loc_8618: fatal contact (codes 1CAh, 1CBh): respawn when dead, else lose energy */
    if (DB(DS_Energy) < 1) {
        TriggerPlayerRespawnCore();
        goto loc_860E;
    }
    StartSoundEffect(1);
    SET_DB(DS_PlayerHurtTimer, 0x2C);
    SET_DB(DS_SubAnimKey, 0);
    SET_DB(DS_CurrentAnimId, 8);
    SET_DB(DS_Energy, (uint8_t)(DB(DS_Energy) - 1));
    {
        uint8_t saved = DB(DS_Energy);
        SET_DB(DS_Energy, 1);
        SpawnPlayerBurst();
        SET_DB(DS_Energy, saved);
    }
    goto loc_860E;

loc_864F:
    StartSoundEffect(1);
    SET_DB(DS_PlayerHurtTimer, 0x2C);
    SET_DB(DS_SubAnimKey, 0);
    SET_DB(DS_CurrentAnimId, 8);
    SpawnPlayerBurst();
    SET_DB(DS_ShakeMagnitude, 7);
    bx = DW((uint16_t)(si + 9));
    if (bx != 0xFFFF) SET_DW((uint16_t)(bx + 4), 0xFFFF);
    ax = 0xE4;
    goto loc_860B;

loc_86B7:
    /* code A9h: destroy every live object that is not a background piece */
    StartSoundEffect(0);
    {
        uint16_t d = 0x4FD0;
        for (int n = 0; n < 12; n++, d = (uint16_t)(d + 0x12)) {
            if (DW((uint16_t)(d + 4)) == 0xFFFF) continue;
            bx = DW((uint16_t)(d + 6));
            if (DB((uint16_t)(bx + 4)) & 0x10) continue;
            if (!(DB((uint16_t)(d + 5)) & 0x20)) continue;
            DestroyObjectWithDebris(d, bx, si);
        }
    }
    SET_DB(DS_ShakeMagnitude, 9);
    bx = DW((uint16_t)(si + 9));
    if (bx != 0xFFFF) SET_DW((uint16_t)(bx + 4), 0xFFFF);
    ax = 0xE6;
    goto loc_860B;

loc_870A:
    /* code AAh: four debris particles at every live object */
    StartSoundEffect(0);
    {
        uint16_t d = 0x4FD0;
        for (int n = 0; n < 12; n++, d = (uint16_t)(d + 0x12)) {
            if (DW((uint16_t)(d + 4)) == 0xFFFF) continue;
            bx = DW((uint16_t)(d + 6));
            if (DB((uint16_t)(bx + 4)) & 0x10) continue;
            if (!(DB((uint16_t)(d + 5)) & 0x20)) continue;
            SET_DW(DS_EffectOriginX, DW(d));
            SET_DW(DS_EffectOriginY, DW((uint16_t)(d + 2)));
            SET_DB((uint16_t)(d + 0x0E), 0xFF);
            SET_DW((uint16_t)(d + 4), 0xFFFF);
            SpawnFourDebrisParticles();
        }
    }
    bx = DW((uint16_t)(si + 9));
    if (bx != 0xFFFF) SET_DW((uint16_t)(bx + 4), 0xFFFF);
    ax = 0xE7;
    goto loc_860B;

loc_876C:
    if (DB(DS_byte_6C04) != 1) {
        StartSoundEffect(1);
        SET_DB(DS_FadeInActive, 0);
        SET_DB(DS_FadeOutActive, 1);
        SET_DB(DS_PaletteFadeStep, 0);
        SET_DB(DS_byte_6C04, 1);
    }
    goto loc_853F;

loc_8790:
    if (DB(DS_byte_6C04) != 0) {
        SET_DB(DS_FadeOutActive, 0);
        SET_DB(DS_FadeInActive, 1);
        SET_DB(DS_PaletteFadeStep, 0);
        SET_DB(DS_byte_6C04, 0);
    }
    goto loc_853F;

loc_87AE:
    StartSoundEffect(8);
    SET_DB(DS_PlayerWeaponType, 0);
    goto loc_853F;

loc_87BC:
    StartSoundEffect(8);
    SET_DB(DS_PlayerWeaponType, 1);
    goto loc_853F;

loc_87CA:
    StartSoundEffect(8);
    SET_DB(DS_PlayerWeaponType, 2);
    goto loc_853F;

loc_87D8:
    StartSoundEffect(8);
    SET_DB(DS_PlayerWeaponType, 3);
    goto loc_853F;

loc_87E6:
    StartSoundEffect(4);
    IncrementPlayerLives();
    SpawnDebrisElement(0xE3, 0x4F1C);
    goto loc_853F;

loc_87FD:
    /* code E4h: checkpoint, then the 70 effect sources at 118h advance one step */
    SET_DW(DS_CheckpointX, DW(DS_PlayerX));
    SET_DW(DS_CheckpointY, DW(DS_PlayerY));
    {
        uint16_t d = 0x8F1D;
        for (int n = 0; n < 0x46; n++, d = (uint16_t)(d + 7)) {
            if (DW((uint16_t)(d + 4)) == 0x118) SET_DW((uint16_t)(d + 4), 0x119);
        }
    }
    bx = DW((uint16_t)(si + 9));
    if (bx != 0xFFFF) SET_DW((uint16_t)(bx + 4), (uint16_t)(DW((uint16_t)(bx + 4)) - 1));
    return;

loc_882A:
    /* code E2h below the player's Y limit: level transitions, then LevelEndMode = 1 */
    {
        uint8_t al = DB(DS_CurrentLevel);
        if (al == 2) SET_DB(DS_CurrentLevel, 0x0C);
        else if (al == 0x0D) SET_DB(DS_CurrentLevel, 2);
        if (al == 6) SET_DB(DS_CurrentLevel, 0x0E);
        if (al == 0x0F) SET_DB(DS_CurrentLevel, 6);
    }
    SET_DB(DS_LevelEndMode, 1);
    return;

loc_8859:
    SET_DB(DS_LevelEndMode, 0xFF);
    return;

loc_885F:
    /* code 91h + 35h: clears the 20 entries at 6EA9h (byte +7 = 7), then the item path */
    {
        uint16_t p = 0x6EA9;
        for (int n = 0; n < 0x14; n++, p = (uint16_t)(p + 8)) SET_DB((uint16_t)(p + 7), 7);
    }
    goto loc_85B0;

loc_850A:
    StartSoundEffect(8);
    if (DB(DS_PlayerAltModeFlag) != 0) goto loc_860E;
    SET_DB(DS_PlayerAltModeFlag, 1);
    ax = 0xFFFF;
    SET_DB(DS_CurrentAnimId, 0xFF);
    goto loc_853F;

loc_84F6:
    if (DB(DS_Energy) >= 3) return;
    SET_DB(DS_Energy, (uint8_t)(DB(DS_Energy) + 1));
    StartSoundEffect(4);
    goto loc_853F;

loc_8524:
    StartSoundEffect(8);
    if (ax < 0x27) goto loc_860E;
    ax = (uint16_t)(ax - 0x27);
    if ((uint8_t)ax > 4) goto loc_860E;
    SET_DB(DS_BonusLetterMask, DB(DS_BonusLetterMask) | (uint8_t)(1u << (uint8_t)ax));
    goto loc_853F;

loc_854F:
    StartSoundEffect(8);
    ax = (uint16_t)(ax - 0x2D);
    SET_DB(DS_byte_6CA8, DB(DS_byte_6CA8) | (uint8_t)(1u << (uint8_t)ax));
    if ((uint8_t)ax == 1) {
        uint16_t d = 0x8F1D;
        for (int n = 0; n < 0x46; n++, d = (uint16_t)(d + 7)) {
            if (DW((uint16_t)(d + 4)) == 0x116) SET_DW((uint16_t)(d + 4), 0x117);
        }
    }
    goto loc_853F;

loc_8582:
    StartSoundEffect(4);
    bx = DW((uint16_t)(si + 0x0E));
    if ((int16_t)bx < 0x80) goto loc_85B6;
    SET_DW((uint16_t)(si + 0x0E), (uint16_t)-DW((uint16_t)(si + 0x0E)));
    {
        uint8_t r = NextWindRandom();
        uint16_t dir = 0x20;
        if (r & 1) {
            dir = (uint16_t)-0x20;
            SET_DB(DS_ShakeMagnitude, 7);
        }
        SET_DW((uint16_t)(si + 6), dir);
        SET_DW((uint16_t)(si + 4), DW(DS_EffectFrame));
    }
    goto loc_860E;

loc_85B0:
    StartSoundEffect(8);
loc_85B6:
    /* item pickup: tally [code-39h], count the item, popup code = table byte + 4Ah */
    ax = (uint16_t)(ax - 0x39);
    bx = ax;
    SET_DB((uint16_t)(DS_ItemTallyCounts + bx), (uint8_t)(DB((uint16_t)(DS_ItemTallyCounts + bx)) + 1));
    SET_DW(DS_ItemTotal, (uint16_t)(DW(DS_ItemTotal) + 1));
    ax = (uint16_t)(DB((uint16_t)(bx - 0x5C8B)) + 0x4A);      /* [bx-5C8Bh] = A375h */
    bx = DW((uint16_t)(si + 9));
    if (bx != 0xFFFF) SET_DW(DS_word_2A7A, (uint16_t)(DW(DS_word_2A7A) + 1));
    goto loc_860B;

loc_85DA:
    StartSoundEffect(8);
    bx = DW((uint16_t)(si + 9));
    if (bx != 0xFFFF) SET_DW((uint16_t)(bx + 4), 0xFFFF);
    SET_DB(DS_byte_6BC9, (uint8_t)(DB(DS_byte_6BC9) + 1));
    if (DB(DS_byte_6BC9) < 6) goto loc_860E;
    if (DB(DS_Energy) == 3) goto loc_860E;
    SET_DB(DS_Energy, (uint8_t)(DB(DS_Energy) + 1));
    SET_DB(DS_byte_6BC9, 0);
    ax = 0xE2;
    goto loc_860B;

loc_853F:
    bx = DW((uint16_t)(si + 9));
    if (bx != 0xFFFF) SET_DW((uint16_t)(bx + 4), 0xFFFF);
    goto loc_860E;

loc_860B:
    SpawnDebrisElement(ax, si);
loc_860E:
    si = (uint16_t)(si + 0x12);
    cx = (uint16_t)(cx - 1);
    if (cx == 0) return;
    goto loc_83E0;
}

/* SpawnPlayerBurst (cs:867E, the body that the contact handlers share): the player's
 * charge burst, 6 * Energy + byte_6BC9 particles from (PlayerX, PlayerY - 30h). Does
 * nothing when the total's low byte is zero. */
void SpawnPlayerBurst(void)
{
    SET_DW(DS_EffectOriginX, DW(DS_PlayerX));
    SET_DW(DS_EffectOriginY, (uint16_t)(DW(DS_PlayerY) - 0x30));
    uint16_t cx = (uint16_t)(6 * DB(DS_Energy));
    uint8_t cl = (uint8_t)((uint8_t)cx + DB(DS_byte_6BC9));
    cx = (uint16_t)((cx & 0xFF00) | cl);
    if (cl == 0) return;
    uint16_t dx = 0xFF80;
    SET_DB(DS_Energy, 0);
    SET_DB(DS_byte_6BC9, 0);
    SET_DW(DS_EffectFrame, 0x2046);
    SpawnEffectBurst(0x30, &dx, cx);
}

/* ---- sub_94DC, sub_93F6, sub_93B2 (SQZ-table helpers at cs:93B2..94DB) ------------------ */

/* sub_94DC: searches the 6-byte records at A6EDh for (DX, AX); returns the record's third
 * word. No bound: a key that is not in the table never returns. */
uint16_t sub_94DC(uint16_t dx, uint16_t ax)
{
    uint16_t bx = 0xA6ED;
    while (!(DW(bx) == dx && DW((uint16_t)(bx + 2)) == ax)) bx = (uint16_t)(bx + 6);
    return DW((uint16_t)(bx + 4));
}

/* cbw of the low byte of R, negated when byte_A3FA is set (the X path). */
static uint16_t lo_signed(uint16_t r, int negate)
{
    uint16_t v = (uint16_t)(int16_t)(int8_t)(uint8_t)r;
    if (negate && DB(DS_byte_A3FA) != 0) v = (uint16_t)-v;
    return v;
}

/* cbw of the high byte of R (the Y path). */
static uint16_t hi_signed(uint16_t r)
{
    return (uint16_t)(int16_t)(int8_t)(uint8_t)(r >> 8);
}

/* sub_93F6: reads five words at DS:SI (advancing SI by 10), derives the eight-word frame
 * placement table at A407h..A41Fh from four sub_94DC lookups and stores the sign-flipped
 * variant (A3FAh) and the bit-6 adjustment (A3F9h). Out: SI after the five words. */
uint16_t sub_93F6(uint16_t si)
{
    uint16_t w0 = DW(si); si = (uint16_t)(si + 2);
    SET_DW(DS_word_A407, w0);
    uint16_t dx = w0;
    uint16_t w1 = DW(si); si = (uint16_t)(si + 2);
    SET_DW(DS_word_A40D, w1);
    uint16_t r = sub_94DC(dx, w1);
    SET_DW(DS_word_A409, (uint16_t)(lo_signed(r, 1) + DW(DS_word_A403)));
    SET_DW(DS_word_A40B, (uint16_t)(hi_signed(r) + DW(DS_word_A405)));

    dx = DW(DS_word_A40D);
    uint16_t w2 = DW(si); si = (uint16_t)(si + 2);
    SET_DW(DS_word_A413, w2);
    r = sub_94DC(dx, w2);
    SET_DW(DS_word_A40F, (uint16_t)(lo_signed(r, 1) + DW(DS_word_A409)));
    SET_DW(DS_word_A411, (uint16_t)(hi_signed(r) + DW(DS_word_A40B)));

    dx = DW(DS_word_A40D);
    uint16_t w3 = DW(si); si = (uint16_t)(si + 2);
    SET_DW(DS_word_A419, w3);
    r = sub_94DC(dx, w3);
    SET_DW(DS_word_A415, (uint16_t)(lo_signed(r, 1) + DW(DS_word_A409)));
    SET_DW(DS_word_A417, (uint16_t)(hi_signed(r) + DW(DS_word_A40B)));

    dx = DW(DS_word_A40D);
    uint16_t w4 = DW(si); si = (uint16_t)(si + 2);
    SET_DW(DS_word_A41F, w4);
    r = sub_94DC(dx, w4);
    SET_DW(DS_word_A41B, (uint16_t)(lo_signed(r, 1) + DW(DS_word_A409)));
    SET_DW(DS_word_A41D, (uint16_t)(hi_signed(r) + DW(DS_word_A40B)));

    if (DB(DS_byte_A3F9) & 0x40) {
        SET_DW(DS_word_A40B, (uint16_t)(DW(DS_word_A40B) + 2));
        SET_DW(DS_word_A411, (uint16_t)(DW(DS_word_A411) + 1));
        SET_DW(DS_word_A41D, (uint16_t)(DW(DS_word_A41D) + 1));
        SET_DW(DS_word_A417, (uint16_t)(DW(DS_word_A417) + 1));
    }
    if (DB(DS_byte_A3FA) != 0) {
        SET_DW(DS_word_A408, DW(DS_word_A408) ^ 0x80);
        SET_DW(DS_word_A40E, DW(DS_word_A40E) ^ 0x80);
        SET_DW(DS_word_A414, DW(DS_word_A414) ^ 0x80);
        SET_DW(DS_word_A41A, DW(DS_word_A41A) ^ 0x80);
        SET_DW(DS_word_A420, DW(DS_word_A420) ^ 0x80);
    }
    return si;
}

/* sub_93B2: sub_93F6 on the words at SI, then five 18-byte entries written at ES:5648h
 * (ES = DS = DATA), each copying three words from the pointer of the table entry at SI+10.
 * The pointer words A415h, A41Bh and A40Fh record their destination in A423h, A421h, A425h.
 * Out: SI after the table (SI + 12), DI = 5648h + 5 * 18h through *di_out. */
uint16_t sub_93B2(uint16_t si, uint16_t *di_out)
{
    SET_ES(DATA_SEG);                           /* the caller's ES is replaced by DATA */
    si = sub_93F6(si);
    uint16_t di = 0x5648;
    for (int n = 0; n < 5; n++) {
        uint16_t bx = DW(si);
        if (bx == 0xA415) SET_DW(DS_word_A423, di);
        if (bx == 0xA41B) SET_DW(DS_word_A421, di);
        if (bx == 0xA40F) SET_DW(DS_word_A425, di);
        si = (uint16_t)(si + 2);
        SET_DW(di, DW(bx)); di = (uint16_t)(di + 2);
        SET_DW(di, DW((uint16_t)(bx + 2))); di = (uint16_t)(di + 2);
        SET_DW(di, DW((uint16_t)(bx + 4))); di = (uint16_t)(di + 2);
        di = (uint16_t)(di + 0x0C);
    }
    *di_out = di;
    return si;
}

/* ---- password code (moved from render.c: MODULE3 cs:932F ComputePasswordCode) ------------- */

/* ComputePasswordCode: depends on the BIOS seed (F000:FFF0..FFFF folded, option ROMs at
 * C000 and up, each 80h bytes, folded in 16-byte steps) and on CpuClass. A ROM with length
 * byte 0 never advances the segment (the original loops forever there); a non-ROM segment
 * advances 400h. AX in, AX out. */
uint16_t ComputePasswordCode(uint16_t ax_in)
{
    if (DB(DS_BiosSeedReady) == 0) {
        uint8_t dl = 0, dh = 0;
        for (uint16_t si = 0xFFF0; ; si++) {              /* add dl / sbb dh share the carry */
            uint8_t b = rd8(0xF000, si);
            uint16_t t = (uint16_t)dl + b;
            uint8_t carry = (uint8_t)(t >> 8);
            dl = (uint8_t)t;
            dh = (uint8_t)(dh - b - carry);
            if ((uint16_t)(si + 1) == 0) break;
        }
        uint16_t seg = 0xC000;
        for (;;) {
            if (rd16(seg, 0) == 0xAA55) {
                for (uint16_t si = 0; si < 0x80; si++) {
                    dl = (uint8_t)(dl + rd8(seg, si));
                    dh = (uint8_t)(dh ^ rd8(seg, si));
                }
                uint16_t bx = (uint16_t)((uint16_t)rd8(seg, 2) << 8) >> 3;   /* bh * 32 */
                seg = (uint16_t)(seg + bx);                                  /* ROM: no 400h step */
            } else {
                seg = (uint16_t)(seg + 0x400);
            }
            if ((uint8_t)(seg >> 8) >= 0xF0) break;
        }
        uint16_t dx = (uint16_t)(dh << 8 | dl);
        if (dx == 0) dx = 0x20;
        SET_DW(DS_BiosSeed, dx);
        SET_DB(DS_BiosSeedReady, 1);
    }
    uint16_t ax = (uint16_t)((ax_in ^ 0x55A3u) * DW(DS_BiosSeed));
    return rol16(ax, CB(CS_CpuClass));
}

/* ---- Differential-test adapters ---------------------------------------------------------- */

EXPORT int c_ProcessPlayerContacts(Regs *r) { ProcessPlayerContacts(); r->ds = DATA_SEG; return 0; }
EXPORT int c_SpawnPlayerBurst(Regs *r) { SpawnPlayerBurst(); r->ds = DATA_SEG; return 0; }
EXPORT int c_SpawnDebrisElement(Regs *r) { SpawnDebrisElement(r->ax, r->si); return 0; }
EXPORT int c_UpdateFollowerSlots(Regs *r) { UpdateFollowerSlots(); r->ds = DATA_SEG; return 0; }
EXPORT int c_BuildEffectDrawList(Regs *r) { BuildEffectDrawList(); r->ds = DATA_SEG; return 0; }

EXPORT int c_TriggerCellChangeGroup(Regs *r) { set_cf(r, TriggerCellChangeGroup(r->si)); return 0; }
EXPORT int c_TickCellChangeTimer(Regs *r) { set_cf(r, TickCellChangeTimer(r->di, r->si)); return 0; }
EXPORT int c_ApplyCellChangeRecord(Regs *r) { set_cf(r, ApplyCellChangeRecord(r->di)); return 0; }

/* SetEffectOriginFromRecord leaves AX = the cell word, DX = row * 16, CX = 1. */
EXPORT int c_SetEffectOriginFromRecord(Regs *r)
{
    uint16_t cell = DW((uint16_t)(r->di + 3));
    SetEffectOriginFromRecord(r->di);
    r->ax = cell;
    r->dx = (uint16_t)((cell >> 8) << 4);
    r->cx = 1;
    return 0;
}

EXPORT int c_RandomDebrisFrame(Regs *r) { r->ax = RandomDebrisFrame(); return 0; }
EXPORT int c_ApplyFollowerHitToObjects(Regs *r) { set_cf(r, ApplyFollowerHitToObjects(r->si)); return 0; }
EXPORT int c_DestroyObjectWithDebris(Regs *r)
{
    DestroyObjectWithDebris(r->di, r->bx, r->si);
    return 0;
}

EXPORT int c_SpawnEffectBurst(Regs *r)
{
    uint16_t dx = r->dx;
    SpawnEffectBurst(r->ax, &dx, r->cx);
    r->dx = dx;
    return 0;
}

EXPORT int c_TestHitboxOverlap(Regs *r) { set_cf(r, TestHitboxOverlap(r->si, r->di)); return 0; }

EXPORT int c_sub_93F6(Regs *r) { r->si = sub_93F6(r->si); r->ds = DATA_SEG; r->es = DATA_SEG; return 0; }

EXPORT int c_sub_93B2(Regs *r)
{
    uint16_t di = 0;
    r->si = sub_93B2(r->si, &di);
    r->di = di;
    r->ds = DATA_SEG;
    r->es = DATA_SEG;
    return 0;
}

EXPORT int c_sub_94DC(Regs *r) { r->ax = sub_94DC(r->dx, r->ax); return 0; }
EXPORT int c_SpawnFourDebrisParticles(Regs *r) { SpawnFourDebrisParticles(); r->ds = DATA_SEG; return 0; }
EXPORT int c_ComputePasswordCode(Regs *r) { r->ax = ComputePasswordCode(r->ax); r->ds = DATA_SEG; return 0; }
