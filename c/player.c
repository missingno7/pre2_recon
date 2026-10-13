/* Player update, player physics and tile collision (oracle: src/MODULE3.ASM cs:5850..6821).
 *
 * Covered: UpdatePlayer (cs:5850) and its state handlers (StateHandlers / AltStateHandlers),
 * the collision and tile contact (UpdatePlayerCollision, UpdatePlayerTileContact), the side and
 * ground and ceiling tile handlers, the player physics helpers, the animation helpers, the
 * projectile, burst-particle and debris updates, and the unused routines of the range.
 * Callees outside the region are called through their owners' headers: UpdatePlayerRenderSlot
 * (level.h), SpawnEffectBurst (effects.h) and the TryShowCredits gate (sprites.h).
 * StepPaletteFade (cs:6772h) is in this region and is defined here.
 *
 * Dispatch: every `call word ptr [bx+Table]` reads the word from the DATA table in pre2_mem
 * and switches on it against the CS_<Label> offsets (invoke). The hurt state in alt mode
 * reads AltStateHandlers[8], the text 'LE' at ds:7D7Fh: code word 454Ch, the final `ret` of
 * SetLatchCopyMode, so the original calls a bare return (quirk; translated as a no-op). Any
 * other word returns PLAYER_UNSUPPORTED and stops the frame (none occurs with the shipped
 * levels: docs/completeness.md).
 *
 * Registers: each routine takes the registers its ASM reads on entry and returns the ones the
 * callers consume (SetPlayerAnim returns BX, FindFreeProjectileSlot returns SI). Quirks kept:
 *   - UpdatePlayer: `and byte PlayerAirFlags, 1` keeps bit 0 only (alt mode); the StateFrameCounter
 *     and StrokeCounter and the timers saturate (add 1, sbb 0 / sub 1, adc 0), they do not wrap.
 *   - UpdatePlayer X commit: PlayerX is written only when X' is in 8..0FF7h (signed) and X'
 *     is left of (CameraLeftLimit+20)*16 (signed).
 *   - UpdatePlayerStateCharge: the SetPlayerAnim call uses AL = low byte of |PlayerVelX| (the
 *     compare left AX holding the speed), not the dispatch AL 4.
 *   - UpdatePlayerStateAttack, the render path without a match and the FallCounter path: the
 *     frame writes PlayerRenderY from the BX and SI and CX that are live at the tail (CX is the
 *     value UpdatePlayer was entered with, SI is 4F1Ch, BX is 5*weapon; on the projectile path
 *     BX = b>>4, SI = slot, CX = cx>>4). The tail reads [BX+SI+6] with those values.
 *   - UpdatePlayerTileContact: the foot cell passed in BX has no edge term; the side scan from
 *     UpdatePlayerCollision adds the edge (+9 or -9) to PlayerX before the cell.
 *   - Column scan and tile contact read the tile tables with 16-bit offsets wrapping at 64 KiB.
 *   - ApplyPlayerAcceleration stores the clamped bound, not the sum, on the high side.
 *   - UpdatePlayerRenderSlot: alt-mode display uses AltMomentumStep as the frame index.
 *   - SlopeHeightOffset returns AX unchanged (high byte included) when bits 4 and 5 are clear.
 *   - ProjectileKindHandlers entries 2 and 3 are data words (0022h, 003Ah): boundary.
 *   - UpdateBurstParticles: the countdown byte +11h is never decremented here (kept frozen).
 *   - UpdatePlayerStateIdle: the anim table index is the dispatch BX (2*state, +40h in alt mode).
 *   - CeilingTileHeadBump: the position record pointer word_6BB1 is used as a DS pointer.
 *   - Output of ApplyWindDrift floors VelX at 0FFA0h (-96) and never clamps the top.
 *   - IncrementPlayerLives writes 0CFh (IRET) to cs:26FAh when the count is above 63h.
 */
#include <stdbool.h>
#include "pre2.h"
#include "platform.h"
#include "ports.h"
#include "render.h"
#include "effects.h"
#include "level.h"
#include "sprites.h"
#include "credits.h"
#include "system.h"
#include "player.h"

/* ---- helpers ---------------------------------------------------------------------- */

#define U16(v)            ((uint16_t)(v))
#define S16(v)            ((int16_t)(v))
#define SAR(v, n)         ((uint16_t)(S16(v) >> (n)))
#define LSEG()            DW(DS_LevelSegment)
#define ESB(off)          rd8(LSEG(), U16(off))          /* es:[off] with ES = LevelSegment */
#define SET_ESB(off, v)   wr8(LSEG(), U16(off), (uint8_t)(v))
#define CODE_SIGN_BIT_7   0x80u

/* sub byte ptr x, 1 / adc byte ptr x, 0 (saturating down) and add/sbb (saturating up) */
static void dec8_sat(uint16_t off) { uint8_t v = DB(off); SET_DB(off, v == 0 ? 0 : (uint8_t)(v - 1)); }
static void inc8_sat(uint16_t off) { uint8_t v = DB(off); SET_DB(off, v == 0xFF ? 0xFF : (uint8_t)(v + 1)); }
static void dec16_sat(uint16_t off) { uint16_t v = DW(off); SET_DW(off, v == 0 ? 0 : (uint16_t)(v - 1)); }
static void inc16_sat(uint16_t off) { uint16_t v = DW(off); SET_DW(off, v == 0xFFFF ? 0xFFFF : (uint16_t)(v + 1)); }

static void WaitFrameSyncFlag(void);

/* Forward declarations for the state and tile handlers. */
static void MaybeEmitPlayerTrail(void);
static void EmitPlayerTrail(void);
static uint16_t SetPlayerAnim(uint8_t al, uint16_t bx);
static uint16_t SetPlayerAnimLocked(uint8_t al, uint16_t bx);
static void ApplyPlayerAcceleration(uint16_t bp);
static void ApplyWindDrift(void);
static void ApplyPlayerGravity(uint16_t bp);
static void ApplyAirMotionAnim(void);
static void SetPlayerAirborne(void);
static void BlockPlayerHorizontal(void);
static void RecordWallImpact(void);
static void SideTileBlockStop(uint16_t di);
static void ApplyProjectileHeavy(uint16_t si);
static void ApplyProjectileLight(uint16_t si);
static void TriggerPlayerRespawn(void);
static uint16_t SlopeHeightOffset(uint16_t ax);
static void GroundTileLand(uint16_t di);
static void GroundTileReleaseLand(uint16_t di);
static void GroundTileLandSlope1(uint16_t di);
static void GroundTileLandSlope2(uint16_t di);
static void GroundTileLandSlope3(uint16_t di);
static void CeilingTileNone(uint16_t di);
static void UpdatePlayerStateIdle(uint8_t al, uint16_t bx, uint16_t cx);
static void UpdatePlayerStateAttack(uint8_t al, uint16_t bx, uint16_t cx, uint16_t si);
static void UpdatePlayerStateJump(uint8_t al, uint16_t bx, uint16_t cx);
static void UpdatePlayerStateRun(uint8_t al, uint16_t bx, uint16_t cx);
static void UpdatePlayerStateCrouch(uint8_t al, uint16_t bx, uint16_t cx);
static void UpdatePlayerStateCharge(uint8_t al, uint16_t bx, uint16_t cx);
static void UpdatePlayerStateHurt(uint8_t al, uint16_t bx, uint16_t cx);
static void UpdatePlayerAltStateBounce(uint8_t al, uint16_t bx, uint16_t cx);
static void IncrementChargeLevel(void);
static void RefreshChangedTile(uint16_t di, uint16_t tile);

/* Walks a DATA word table entry: the word at table+bx, zero-extended, read as a CS offset. */
static int invoke(uint16_t target, uint8_t al, uint16_t bx, uint16_t di, uint16_t si, uint16_t cx);
static int  DispatchSideTileHandler(uint16_t bx);
static int  DispatchSideTileClass24(uint16_t bx);
static void BridgeDipUpdate(uint16_t di);
static void AltMomentumBlock(void);
static void attack_tail(uint16_t sprite, uint16_t cx, uint16_t bx, uint16_t si);
static void attack_render(uint16_t bx, uint16_t cx);
static void JumpBody(uint8_t al, uint16_t bx, uint16_t cx);
static void IdleAnimFromDispatch(uint16_t bx);
static void ContinueAttackAnim(uint16_t bx, uint16_t cx, uint16_t si);
static void BurstPhysics(uint16_t si);
static int  unused_66D3(uint16_t si, uint16_t di);
static uint16_t unused_6760(uint16_t si);

/* SI at the state dispatch: UpdatePlayer loads it with PlayerX (4F1Ch) before the input wait, and
 * PollInputState preserves SI and CX, so the attack tail sees 4F1Ch on every path. */
#define PLAYER_SI_AT_DISPATCH  DS_PlayerX
#define PROJECTILE_SLOTS_BASE  DS_word_4F2E
#define PROJECTILE_SLOT_SIZE   0x12u
#define PARTICLE_BASE          0x50A8u
#define PARTICLE_END           0x52E8u
#define DEBRIS_BASE            0x5450u

/* ---- the state-table dispatch ----------------------------------------------------------- */

static int invoke(uint16_t target, uint8_t al, uint16_t bx, uint16_t di, uint16_t si, uint16_t cx)
{
    switch (target) {
    case CS_UpdatePlayerStateIdle:        UpdatePlayerStateIdle(al, bx, cx); return PLAYER_OK;
    case CS_UpdatePlayerStateRun:         UpdatePlayerStateRun(al, bx, cx); return PLAYER_OK;
    case CS_UpdatePlayerStateJump:        UpdatePlayerStateJump(al, bx, cx); return PLAYER_OK;
    case CS_UpdatePlayerStateAttack:      UpdatePlayerStateAttack(al, bx, cx, si); return PLAYER_OK;
    case CS_UpdatePlayerStateCharge:      UpdatePlayerStateCharge(al, bx, cx); return PLAYER_OK;
    case CS_UpdatePlayerStateCrouch:      UpdatePlayerStateCrouch(al, bx, cx); return PLAYER_OK;
    case CS_UpdatePlayerStateHurt:        UpdatePlayerStateHurt(al, bx, cx); return PLAYER_OK;
    case CS_UpdatePlayerAltStateBounce:   UpdatePlayerAltStateBounce(al, bx, cx); return PLAYER_OK;
    case CS_SideTileWallImpactHandler:    SideTileWallImpactHandler(di); return PLAYER_OK;
    case CS_SideTileBlockStop:            SideTileBlockStop(di); return PLAYER_OK;
    case CS_TriggerPlayerRespawn:         TriggerPlayerRespawn(); return PLAYER_OK;
    case CS_GroundTileSnapOrFall:         GroundTileSnapOrFall(di); return PLAYER_OK;
    case CS_GroundTileLand:               GroundTileLand(di); return PLAYER_OK;
    case CS_GroundTileLandSlope1:         GroundTileLandSlope1(di); return PLAYER_OK;
    case CS_GroundTileLandSlope2:         GroundTileLandSlope2(di); return PLAYER_OK;
    case CS_GroundTileLandSlope3:         GroundTileLandSlope3(di); return PLAYER_OK;
    case CS_GroundTileReleaseLand:        GroundTileReleaseLand(di); return PLAYER_OK;
    case CS_CeilingTileNone:              CeilingTileNone(di); return PLAYER_OK;
    case CS_CeilingTileHeadBump:          CeilingTileHeadBump(di); return PLAYER_OK;
    case CS_ApplyProjectileHeavy:         ApplyProjectileHeavy(si); return PLAYER_OK;
    case CS_ApplyProjectileLight:         ApplyProjectileLight(si); return PLAYER_OK;
    case 0x454Cu:                         return PLAYER_OK;   /* 'LE': the `ret` at cs:454Ch (see above) */
    default:                              return PLAYER_UNSUPPORTED;
    }
}

/* Alt-mode momentum block (UpdatePlayer, loc_5977): runs instead of the handler while
 * PlayerAirFlags bit 0 is set. Up adds to AltMomentumStep (unless it is 6, where it may set
 * PlayerAirFlags bit 1) and spends MomentumCounter; down steps AltMomentumStep back and adds
 * |VelX>>4| to MomentumCounter when the step is at most 1. */
static void AltMomentumBlock(void)
{
    if (DB(DS_byte_27EA) != 0) {
        if (DB(DS_AltMomentumStep) != 6) SET_DB(DS_AltMomentumStep, U16(DB(DS_AltMomentumStep) + 1));
        else if (S16(DW(DS_PlayerVelY)) > 0x10) SET_DB(DS_PlayerAirFlags, DB(DS_PlayerAirFlags) | 2);
        if (DB(DS_MomentumCounter) != 0) {
            SET_DB(DS_MomentumCounter, U16(DB(DS_MomentumCounter) - 1));
            if (DB(DS_AltMomentumStep) >= 4) SET_DW(DS_PlayerVelY, 0xFFC0);
            return;
        }
    }
    if (DB(DS_byte_27EB) == 0) { dec8_sat(DS_MomentumCounter); return; }
    if (DB(DS_AltMomentumStep) != 0) SET_DB(DS_AltMomentumStep, U16(DB(DS_AltMomentumStep) - 1));
    SET_DB(DS_PlayerAirFlags, DB(DS_PlayerAirFlags) | 2);
    if (DB(DS_AltMomentumStep) > 1) return;
    int16_t v = S16(DW(DS_PlayerVelX)) >> 4;                    /* sar 4 x4, then abs */
    if (v < 0) v = (int16_t)-v;
    uint8_t lo = (uint8_t)v;
    uint8_t prod = (uint8_t)(lo * 5);                           /* mul ah: AL = low byte of lo*5 */
    if (DB(DS_MomentumCounter) < prod) SET_DB(DS_MomentumCounter, (uint8_t)(DB(DS_MomentumCounter) + lo));
}

/* ---- UpdatePlayer (cs:5850) -------------------------------------------------------------- */

/* Per-frame update. Order: respawn request, end signal, credits gate, input sampling, facing,
 * state index and handler (or the alt-mode momentum block), X and Y integrate, collision,
 * render slot, timers. */
static int UpdatePlayerBody(uint16_t cx_in)
{
    SET_DW(DS_PlayerSprite, 0xFFFF);
    if (DB(DS_RespawnState) == 0 && DB(DS_RespawnRequestFlag) != 0) {
        TriggerPlayerRespawn();
        SET_DB(DS_RespawnState, U16(DB(DS_RespawnState) - 1));
        return PLAYER_OK;                                  /* jmp loc_5A8C */
    }
    if (DB(DS_byte_2830) != 0) {                           /* loc_587C: F2 down: EndSignal 1 (give up) */
        SET_DB(DS_EndSignal, 1);
        /* loc_5888 then spins while byte_2830 is zero: only if F2 was released between the two
         * tests, which needs a keyboard interrupt there; none is delivered between them (no
         * service point), so the spin always falls through, as on the original in practice */
        return PLAYER_OK;
    }
    if (!TryShowCredits()) DrawStatusBarBackground();     /* call; jb loc_589A: CF = 1 skips it */
    WaitFrameSyncFlag();
    PollInputState();                                      /* preserves CX and SI */

    uint8_t dl = U16(DB(DS_byte_27ED) | DB(DS_byte_27EC));
    SET_DB(DS_HorizontalInputHeld, dl);
    dl = U16(DB(DS_byte_27EA) | DB(DS_byte_27EB));
    SET_DB(DS_VerticalInputHeld, dl);

    if (DB(DS_byte_27EC) != 0) {
        if (DB(DS_byte_27ED) == 0 && DW(DS_PlayerFacing) != 1) {
            SET_DW(DS_PlayerFacing, 1);
            SET_DW(DS_StateFrameCounter, 0);
        }
    } else {
        if (DB(DS_byte_27ED) != 0 && DW(DS_PlayerFacing) != 0xFFFF) {
            SET_DW(DS_PlayerFacing, 0xFFFF);
            SET_DW(DS_StateFrameCounter, 0);
        }
    }

    /* state index: 27EC<<4 | 27ED<<3 | 27EA<<2 | 27EB<<1 | 27E8 (bit 0 of each byte) */
    uint16_t ah = 0;
    ah = U16((ah << 1) | (DB(DS_byte_27EC) & 1));
    ah = U16((ah << 1) | (DB(DS_byte_27ED) & 1));
    ah = U16((ah << 1) | (DB(DS_byte_27EA) & 1));
    ah = U16((ah << 1) | (DB(DS_byte_27EB) & 1));
    ah = U16((ah << 1) | (DB(DS_byte_27E8) & 1));
    if (DB(DS_AttackRecoveryTimer) != 0) ah = 0;
    uint16_t bx = ah & 0xFF;
    uint8_t al = DB(U16(DS_InputAnimTable + bx));
    uint8_t hurt = DB(DS_PlayerHurtTimer);
    SET_DB(DS_byte_4F1B, hurt);
    if (hurt >= 0x16) al = 8;

    if (DB(DS_CurrentAnimId) != al) {
        SET_DW(DS_StateFrameCounter, 0);
        SET_DB(DS_SubAnimKey, 0);
    }
    inc16_sat(DS_StateFrameCounter);

    bx = U16((uint16_t)al << 1);
    int st = PLAYER_OK;
    if (DB(DS_PlayerAltModeFlag) == 0) {
        st = invoke(DW(U16(DS_StateHandlers + bx)), al, bx, 0, PLAYER_SI_AT_DISPATCH, cx_in);
    } else {
        bx = U16(bx + 0x40);
        SET_DB(DS_PlayerAirFlags, DB(DS_PlayerAirFlags) & 1);
        if (DB(DS_PlayerAirFlags) != 0) {
            AltMomentumBlock();                              /* the handler call is skipped */
        } else {
            if (S16(DW(DS_PlayerVelY)) > 0xA0) SET_DB(DS_PlayerAirFlags, 1);
            st = invoke(DW(U16(DS_StateHandlers + bx)), al, bx, 0, PLAYER_SI_AT_DISPATCH, cx_in);
        }
    }
    if (st != PLAYER_OK) return st;

    /* X integrate: committed only inside the window (signed compares) */
    uint16_t ax = U16(SAR(DW(DS_PlayerVelX), 4) + DW(DS_PlayerX));
    uint16_t lim = U16((DW(DS_CameraLeftLimit) + 0x14) << 4);
    if (S16(lim) > S16(ax) && S16(ax) >= 8 && S16(ax) < 0x0FF8) SET_DW(DS_PlayerX, ax);
    /* Y integrate, unconditional */
    SET_DW(DS_PlayerY, U16(DW(DS_PlayerY) + SAR(DW(DS_PlayerVelY), 4)));
    st = UpdatePlayerCollision();
    if (st != PLAYER_OK) return st;
    UpdatePlayerRenderSlot();
    dec8_sat(DS_ChargeLevel);
    dec8_sat(DS_AttackRecoveryTimer);
    dec8_sat(DS_ShakeMagnitude);
    dec8_sat(DS_byte_6BE8);
    dec8_sat(DS_RespawnState);
    dec8_sat(DS_LandingBlockTimer);
    dec8_sat(DS_BonusFlashTimer);
    dec16_sat(DS_word_6BE2);
    return PLAYER_OK;
}

/* UpdatePlayer saves ES on entry and restores it on the way out (push es ... pop es), so the
 * ES changes of the tile handlers (CeilingTileHeadBump loads LevelSegment) do not leak. */
int UpdatePlayer(uint16_t cx_in)
{
    uint16_t es_saved = pre2_es;
    int st = UpdatePlayerBody(cx_in);
    if (st == PLAYER_OK) pre2_es = es_saved;
    return st;
}

/* ---- player collision (cs:5A96) ------------------------------------------------------------ */

/* Foot cell range check, tile contact at the foot, the airborne tail, and the side-wall column
 * scan. The foot cell for the tile contact is (PlayerX sar 4) + row; the column scan starts one
 * cell over with the edge term (+9 / -9 / 0 by the sign of PlayerVelX). */
int UpdatePlayerCollision(void)
{
    int16_t row1 = (int16_t)(S16(DW(DS_PlayerY)) >> 4) - 1;      /* sar ax,4; dec ax */
    uint16_t ax = U16(row1);
    ax = U16(((ax & 0xFF) << 8) | ((ax >> 8) & 0xFF));           /* xchg ah, al */
    uint16_t fi = U16((DW(DS_PlayerFrameWord) & 0x1FFF) << 1);   /* frame index * 2 */
    uint8_t dh = DB(U16(DS_FrameHeights + fi));
    int16_t vx = S16(DW(DS_PlayerVelX));
    uint16_t edge = (vx > 0) ? 9 : (vx < 0) ? U16(-9) : 0;

    /* range: |row - CameraRow| <= 11 and |col - CameraCol| <= 20; PlayerY < 0 skips the
     * level-bound test; LevelFlags bit 2 also rejects PlayerY < CameraRow*16 */
    bool out = false;
    uint16_t d = U16(SAR(DW(DS_PlayerY), 4) - DW(DS_CameraRow));
    if (S16(d) < 0) d = U16(-d);
    if (d > 0x0B) out = true;
    if (!out) {
        d = U16(SAR(DW(DS_PlayerX), 4) - DW(DS_CameraCol));
        if (S16(d) < 0) d = U16(-d);
        if (d > 0x14) out = true;
    }
    if (!out && (DB(DS_LevelFlags) & 4) != 0) {
        if (S16(DW(DS_PlayerY)) < S16(U16(DW(DS_CameraRow) << 4))) out = true;
    }
    if (!out && S16(DW(DS_PlayerY)) >= 0) {
        if (S16(DW(DS_PlayerY)) > S16(U16((DB(DS_MapRows) + 1) << 4))) out = true;
    }
    if (out) {
        if (DB(DS_CurrentLevel) == 0x0E) SET_DB(DS_EndSignal, 0xFF);
        else TriggerPlayerRespawn();
    }

    SET_DB(DS_AirborneFlag, 0);
    uint16_t foot = U16(SAR(DW(DS_PlayerX), 4) + ax);
    int st = UpdatePlayerTileContact(foot);
    if (st != PLAYER_OK) return st;

    if (DB(DS_AirborneFlag) == 1) {
        if (DB(DS_PlayerOnPlatform) == 0) {
            ApplyAirMotionAnim();
            if (S16(DW(DS_PlayerVelY)) > 0) SET_DB(DS_FallCounter, U16(DB(DS_FallCounter) + 1));
        } else {
            FinishPlayerLanding();
            SET_DB(DS_FallCounter, 0);
        }
    } else {
        SET_DB(DS_FallCounter, 0);
    }

    if (S16(DW(DS_PlayerY)) <= 0) return PLAYER_OK;
    uint16_t cell = U16(SAR(U16(edge + DW(DS_PlayerX)), 4) + ax);
    st = DispatchSideTileHandler(cell);
    if (st != PLAYER_OK) return st;
    uint16_t bx = cell;
    for (;;) {                                                  /* loc_5B72 */
        if (bx < 0x100) break;                                  /* sub bx,100h; jb */
        bx = U16(bx - 0x100);
        if (dh <= 0x10) break;                                  /* sub dh,10h; ja */
        dh = (uint8_t)(dh - 0x10);
        st = DispatchSideTileClass24(bx);
        if (st != PLAYER_OK) return st;
    }
    return PLAYER_OK;
}

/* Side dispatch (DispatchSideTileHandler): class of the tile at cell, handler SideTileHandlers[2*class]
 * with DI = cell. Entries 3..8 alias the ground handlers (GroundTileHandlers = SideTileHandlers+6). */
static int DispatchSideTileHandler(uint16_t bx)
{
    uint16_t di = bx;
    uint8_t cls = DB(U16(DS_SideTileClassTable + ESB(bx)));
    uint16_t target = DW(U16(DS_SideTileHandlers + 2u * cls));
    return invoke(target, 0, 2u * cls, di, PLAYER_SI_AT_DISPATCH, 0);
}

/* Column-scan variant: only classes 2 and 4 call the side handler. */
static int DispatchSideTileClass24(uint16_t bx)
{
    uint16_t di = bx;
    uint8_t cls = DB(U16(DS_SideTileClassTable + ESB(bx)));
    if (cls != 4 && cls != 2) return PLAYER_OK;
    uint16_t target = DW(U16(DS_SideTileHandlers + 2u * cls));
    return invoke(target, 0, 2u * cls, di, PLAYER_SI_AT_DISPATCH, 0);
}

/* Refresh a changed tile: class >= 1 marks the grid dirty; class 0 redraws it. */
static void RefreshChangedTile(uint16_t di, uint16_t tile)
{
    if (DB(U16(DS_TileClassTable + tile)) >= 1) {
        SET_DB(DS_GridDirtyFlag, 1);
        SET_DW(DS_PrevCameraCol, 0x55AA);
        return;
    }
    RedrawTileIfVisible(tile & 0xFF, di);
}

/* Bridge dip (UpdatePlayerTileContact, loc_5B96..5C01): when the foot tile di is not the dipping
 * tile, the previous dipping tiles are walked back while the next lower id has TileFlagsTable
 * bit 20h; the new tile steps up one id when it has the bit, and becomes the dipping tile. */
static void BridgeDipUpdate(uint16_t di)
{
    if (DW(DS_BridgeDipTile) != 0x55AA) {
        uint16_t old = DW(DS_BridgeDipTile);
        uint16_t bx = ESB(old);
        for (;;) {
            bx = U16(bx - 1);
            if ((DB(U16(DS_TileFlagsTable + bx)) & 0x20) == 0) {
                SET_DW(DS_BridgeDipTile, 0x55AA);
                break;
            }
            SET_ESB(old, (uint8_t)bx);
            RefreshChangedTile(old, bx);
        }
    }
    /* loc_5BE8: the new foot tile */
    uint16_t bx = ESB(di);
    if (DB(U16(DS_TileFlagsTable + bx)) & 0x20) {
        SET_DW(DS_BridgeDipTile, di);
        bx = U16(bx + 1);
        SET_ESB(di, (uint8_t)bx);
        RefreshChangedTile(di, bx);
    }
}

/* Tile interaction for the foot cell (bx). The ground cell is the foot cell + 100h (DI for the
 * ground handlers). Ceiling handler from the tile above when moving up or still. */
int UpdatePlayerTileContact(uint16_t bx)
{
    if (S16(DW(DS_PlayerY)) <= -1) {
        ApplyAirMotionAnim();
        SET_DB(DS_AirborneFlag, 0xFF);
        return PLAYER_OK;
    }
    uint16_t di = U16(bx + DS_word_0100);                    /* ground cell */
    uint8_t tile = (DB(DS_MapRows) > (uint8_t)(di >> 8)) ? ESB(di) : 0;   /* rows >= MapRows: tile 0 */
    uint8_t type = DB(U16(DS_GroundTypeTable + tile));
    uint16_t tbx = U16(type * 2u);
    if (DW(DS_BridgeDipTile) != di) BridgeDipUpdate(di);
    int st = invoke(DW(U16(DS_GroundTileHandlers + tbx)), 0, tbx, di, PLAYER_SI_AT_DISPATCH, 0);
    if (st != PLAYER_OK) return st;

    /* ceiling: the tile above the foot (cell - 100h) */
    if (bx < 0x100) return PLAYER_OK;
    uint16_t cb = U16(bx - 0x100);
    if (S16(DW(DS_PlayerVelY)) > 0) return PLAYER_OK;
    uint8_t above = ESB(cb);
    uint8_t foot = ESB(U16(bx));                                 /* the cell at bx = foot tile */
    uint8_t side_foot = DB(U16(DS_SideTileClassTable + foot));
    uint8_t flags = DB(U16(DS_TileFlagsTable + above));
    uint16_t n = U16((flags & 0x0F) * 2u);
    st = invoke(DW(U16(DS_CeilingTileHandlers + n)), 0, n, cb, PLAYER_SI_AT_DISPATCH, 0);
    if (st != PLAYER_OK) return st;
    if ((side_foot & 1) == 0) return PLAYER_OK;                  /* pop ax; and ah,1; je */
    if (S16(DW(DS_PlayerY)) <= 0) return PLAYER_OK;
    int16_t dx = (S16(DW(DS_PlayerVelX)) > 0) ? -1 : 1;
    uint16_t d1 = U16(cb + dx);
    if (DB(U16(DS_SideTileClassTable + ESB(U16(d1 + 0x100)))) == 0) {
        SET_DW(DS_PlayerX, U16(DW(DS_PlayerX) + 2 * dx));
    } else {
        int16_t dn = (int16_t)-dx;
        uint16_t d2 = U16(d1 + dn + dn);
        if (DB(U16(DS_SideTileClassTable + ESB(U16(d2 + 0x100)))) == 0)
            SET_DW(DS_PlayerX, U16(DW(DS_PlayerX) + 2 * dn));
    }
    return PLAYER_OK;
}

/* ---- redraw, wall impact, respawn and lives ---------------------------------------------- */

/* Draws the tile at map cell DI into the ring when it is inside the view window (column offset
 * below 14h, row offset below 0Ch). Returns CF (0 drawn, 1 outside). AX is the tile id that the
 * caller leaves (BlitTileToRing reads it). */
int RedrawTileIfVisible(uint16_t ax, uint16_t di)
{
    uint8_t dl = (uint8_t)((di & 0xFF) - DB(DS_CameraCol));
    uint8_t dh = (uint8_t)((di >> 8) - DB(DS_CameraRow));
    if (dl >= 0x14 || dh >= 0x0C) return 1;
    uint8_t bl = 0x50, bh = 2;
    if (CB(CS_OptionVByte) >= 3) {
        SetLatchCopyMode();
        bl = 0x28;
        bh = 1;
    }
    uint16_t row = (uint16_t)(di >> 8);
    uint16_t off = U16((bl * (row % 12)) << 4);                  /* mul ah; shl ax,4 */
    uint16_t col = (uint16_t)((di & 0xFF) % 20);
    off = U16(off + (col << bh));
    BlitTileToRing(ax, off);
    SET_DB(DS_AnimVisibleFlag, 1);
    return 0;
}

/* Records a wall impact in the first free of the 20 slots at 6EA9h (free = first word 55AAh). */
static void RecordWallImpact(void)
{
    for (uint16_t si = 0x6EA9; si < 0x6F49; si = U16(si + 8)) {
        if (DW(si) != 0x55AA) continue;
        SET_DW(si, U16(DW(DS_PlayerX) << 3));
        SET_DW(U16(si + 2), U16(DW(DS_PlayerY) << 3));
        SET_DB(U16(si + 4), 0);
        SET_DB(U16(si + 5), 0);
        SET_DB(U16(si + 7), 0);
        return;
    }
}

void SideTileWallImpactHandler(uint16_t di)
{
    if (DB(U16(DS_TileFlagsTable + ESB(di))) & 0x10) RecordWallImpact();
}

static void BlockPlayerHorizontal(void)
{
    SET_DW(DS_PlayerX, U16(DW(DS_PlayerX) - SAR(DW(DS_PlayerVelX), 4)));
    SET_DW(DS_PlayerVelX, 0);
}

static void SideTileBlockStop(uint16_t di)
{
    (void)di;
    BlockPlayerHorizontal();
}

static void SetPlayerAirborne(void)
{
    SET_DB(DS_AirborneFlag, DB(DS_AirborneFlag) | 1);
}

/* Respawn request: TriggerPlayerRespawnCore (the state and lives check). */
static void TriggerPlayerRespawn(void)
{
    TriggerPlayerRespawnCore();
}

void TriggerPlayerRespawnCore(void)
{
    if (DB(DS_RespawnState) != 0) return;
    if (DB(DS_Lives) == 0) {
        SET_DB(DS_EndSignal, 1);
        return;
    }
    SET_DB(DS_Lives, U16(DB(DS_Lives) - 1));
    SET_DB(DS_Energy, 0);
    SET_DB(DS_RespawnState, 2);
}

/* Adds one life unless the count is 63h. Above 63h it writes IRET (0CFh) to cs:26FAh, the first
 * byte of DrawSpriteObjects (self-modifying in the original). */
void IncrementPlayerLives(void)
{
    uint16_t addr = U16(0x2172 + DS_byte_0666);
    uint8_t v = DB(addr);
    if (v == 0x63) return;
    if (v > 0x63) SET_CB(CS_DrawSpriteObjects, 0xCF);
    SET_DB(addr, U16(v + 1));
}

/* ---- ground and side handlers (DI = ground cell, or the cell for side handlers) ---------- */

/* SlopeHeightOffset: AX = property byte. When bits 4 or 5 are set: quotient (PlayerX & 0Fh)/3,
 * then height (low nibble) + quotient (bit 4) or height - quotient. Otherwise AX is unchanged. */
static uint16_t SlopeHeightOffset(uint16_t ax)
{
    uint8_t prop = (uint8_t)ax;
    if ((prop & 0x30) == 0) return ax;
    uint8_t q = (uint8_t)((DW(DS_PlayerX) & 0x0F) / 3);
    uint8_t h = prop & 0x0F;
    uint8_t al = (prop & 0x10) ? (uint8_t)(q + h) : (uint8_t)(h - q);
    return U16((int8_t)al);                                       /* cbw */
}

/* Lands the player on the ground cell DI (LandPlayerOnGround). */
void LandPlayerOnGround(uint16_t di)
{
    SET_DB(DS_MotionShift, 0);
    if (S16(DW(DS_PlayerVelY)) < 0) {
        SetPlayerAirborne();
        return;
    }
    SET_DB(DS_PlayerAirFlags, 0);
    SET_DB(DS_PlayerY, DB(DS_PlayerY) & 0xF0);                     /* and byte ptr PlayerY, 0F0h */
    uint16_t ax = U16((int8_t)DB(U16(DS_TileSolidProps + ESB(di))));  /* cbw */
    if (ax != 0) {
        ax = SlopeHeightOffset(ax);
        int16_t bx = SAR(DW(DS_PlayerVelY), 4);
        if (bx > 0 && S16(ax) >= bx) ax = U16(bx);
        SET_DW(DS_PlayerY, U16(DW(DS_PlayerY) + ax));
    } else {
        uint8_t p2 = DB(U16(DS_TileSolidProps + ESB(U16(di - 0x100))));
        if (p2 != 0) {
            ax = SlopeHeightOffset(p2);
            if (S16(ax) < 0x10) {
                ax = U16(ax - 0x10);
                SET_DW(DS_PlayerY, U16(DW(DS_PlayerY) + ax));
            }
        }
    }
    /* loc_647C: landing impact */
    if (DB(DS_FallCounter) > 4) {
        EmitPlayerTrail();
        int16_t d = S16(U16(DW(DS_PlayerY) - DW(DS_LastLandY)));
        if (d >= 0x20 && S16(DW(DS_PlayerVelY)) >= 0x50) {
            SET_DW(DS_LastLandY, DW(DS_PlayerY));
            if (DB(DS_FallCounter) >= 0x14 && S16(DW(DS_PlayerVelY)) > 0xA0) SET_DB(DS_ShakeMagnitude, 8);
            if (DB(DS_FallCounter) > 0x0A) {                       /* loc_64B3: bounce */
                if ((DB(DS_LevelFlags) & 1) == 0) SET_DW(DS_PlayerVelY, 0xFFE0);
                SET_DW(DS_PlayerFrameWord, U16((DW(DS_PlayerFrameWord) & 0xE000) | 0x0C));
                SET_DB(DS_FallCounter, 0);
                return;
            }
        }
    }
    SET_DW(DS_PlayerVelY, 0);                                      /* loc_64D9: soft landing */
    FinishPlayerLanding();
}

void FinishPlayerLanding(void)
{
    dec8_sat(DS_JumpCooldown);
    SET_DB(DS_JumpFrame, 0);
    SET_DB(DS_AirborneFlag, 2);
    SET_DW(DS_LastLandY, DW(DS_PlayerY));
}

/* Ground class 0: snaps onto the tile one row below when at rest and it is solid, else airborne. */
void GroundTileSnapOrFall(uint16_t di)
{
    if (DW(DS_PlayerVelY) == 0) {
        uint8_t prop = DB(U16(DS_TileSolidProps + ESB(U16(di + 0x100))));
        if (prop != 0) {
            uint16_t off = SlopeHeightOffset(prop);
            if (S16(off) < 0x10) {
                di = U16(di + 0x100);
                SET_DW(DS_PlayerY, U16(DW(DS_PlayerY) + 0x10));
                LandPlayerOnGround(di);
                return;
            }
        }
    }
    SetPlayerAirborne();
}

static void GroundTileLand(uint16_t di) { LandPlayerOnGround(di); }

/* Ground class 5: MotionShift 0; airborne while LandingBlockTimer is set, else lands. */
static void GroundTileReleaseLand(uint16_t di)
{
    SET_DB(DS_MotionShift, 0);
    if (DB(DS_LandingBlockTimer) != 0) SetPlayerAirborne();
    else LandPlayerOnGround(di);
}

static void GroundTileLandSlope1(uint16_t di) { LandPlayerOnGround(di); SET_DB(DS_MotionShift, 1); }
static void GroundTileLandSlope2(uint16_t di) { LandPlayerOnGround(di); SET_DB(DS_MotionShift, 2); }
static void GroundTileLandSlope3(uint16_t di) { LandPlayerOnGround(di); SET_DB(DS_MotionShift, 3); }

static void CeilingTileNone(uint16_t di) { (void)di; }

/* Ceiling class 1: with PlayerVelY != 0 the player stops and sits one row below the ceiling row.
 * With PlayerVelY == 0 and a frame other than 0Ah / 15h, the wall record at word_6BB1 is nudged
 * by 10h to the side that is free of solid class tiles. */
void CeilingTileHeadBump(uint16_t di)
{
    (void)di;
    if (DW(DS_PlayerVelY) != 0) {
        SET_DW(DS_PlayerVelY, 0);
        SET_DW(DS_PlayerY, U16((DW(DS_PlayerY) & 0xFFF0) + 0x10));
        return;
    }
    uint8_t al = DB(DS_PlayerFrameWord);
    if (al == 0x0A || al == 0x15) return;
    uint16_t p = DW(DS_word_6BB1);
    uint16_t cell = U16((((SAR(DW(U16(p + 2)), 4)) & 0xFF) << 8) | (SAR(DW(p), 4) & 0xFF));
    SET_ES(LSEG());                  /* mov es, LevelSegment: ES stays LevelSegment past the return (callers' ES) */
    if ((DB(U16(DS_SideTileClassTable + ESB(cell))) & 1) == 0) return;
    int16_t dx = 0x10;
    if ((DB(U16(DS_SideTileClassTable + ESB(U16(cell - 1)))) & 1) == 0) dx = -0x10;
    SET_DW(p, U16(DW(p) + dx));
}

/* ---- dead code in the range (no caller in the source tree) ------------------------------ */

/* Overlap test of two records at SI and DI, CF = 1 when they overlap. Not called. */
static int unused_66D3(uint16_t si, uint16_t di)
{
    uint16_t ax = U16(DW(si) - DW(di));
    if (S16(ax) < 0) ax = U16(-ax);
    if (ax >= 0x40) return 0;
    ax = U16(DW(U16(si + 2)) - DW(U16(di + 2)));
    if (S16(ax) < 0) ax = U16(-ax);
    if (ax >= 0x46) return 0;
    ax = DW(U16(si + 2));
    uint16_t dx = DW(U16(di + 2));
    uint16_t bx = DW(U16(si + 4));
    if (S16(ax) < S16(dx)) {
        uint16_t t = ax; ax = dx; dx = t;
        bx = DW(U16(di + 4));
    }
    uint16_t bi = U16((bx & 0x1FFF) << 1);
    uint16_t t3 = U16(DB(U16(DS_FrameHeights + bi)) - 3);                 /* [bx+7191h] - 3 */
    ax = U16(ax - t3);
    if (S16(ax) >= S16(dx)) return 0;
    uint16_t bp = U16((DW(U16(si + 4)) & 0x1FFF) << 1);
    uint8_t b = DB(U16(bp + DS_FrameHotX));
    dx = U16(DW(si) - b);
    uint16_t di_idx = U16((DW(U16(di + 4)) & 0x1FFF) << 1);
    ax = U16(DW(di) - DB(U16(di_idx + DS_FrameHotX)));
    uint16_t bl = (uint8_t)(DB(U16(di_idx + DS_FrameWidths)) >> 1);
    if (S16(ax) >= S16(dx)) {
        uint16_t t = ax; ax = dx; dx = t;
        bl = (uint8_t)(DB(U16(bp + DS_FrameWidths)) >> 1);
    }
    ax = U16(ax + bl);
    if (S16(ax) <= S16(dx)) return 0;
    return 1;
}

/* Returns X + 0Ch, or X - 0Ah when bit 7 of byte +5 is set. Not called. */
static uint16_t unused_6760(uint16_t si)
{
    uint16_t dx = 0x0C;
    if (DB(U16(si + 5)) & 0x80) dx = 0xFFF6;
    return U16(DW(si) + dx);
}

/* Placeholder (sub_67D7): when BonusLetterMask is 1Fh, spawns the bonus burst at the player and
 * sets BonusFlashTimer; otherwise clears the 38h bits of byte 6CA8h and sets word_6BE2. */
void sub_67D7(void)
{
    if (DB(DS_BonusLetterMask) == 0x1F) {
        SET_DW(DS_EffectOriginX, DW(DS_PlayerX));
        SET_DW(DS_EffectOriginY, U16(DW(DS_PlayerY) - 0x70));
        SET_DW(DS_EffectFrame, 0x6E);
        uint16_t dx = 0;
        SpawnEffectBurst(0, &dx, 1);
        SET_DB(DS_BonusLetterMask, 0);
        SET_DB(DS_byte_6BFF, 1);
        SET_DB(DS_BonusFlashTimer, 0x2C);
        return;
    }
    if ((DB(DS_byte_6CA8) & 0x38) == 0x38) {
        SET_DB(DS_byte_6CA8, DB(DS_byte_6CA8) & 0xC7);
        SET_DW(DS_word_6BE2, 0x294);
    }
}

/* StepPaletteFade (cs:6772h): one step of the 16-colour DAC fade. Moves each of the 48 DAC
 * components of the level palette one step toward the target block at ds:ACB7h (or back, when
 * FadeInActive is set), writing through the port model; clears both fade flags when no
 * component moved (moved here from render.c: the region of cs:6772h). */
#define FADE_TARGET_PALETTE  0xACB7u   /* 48-byte target DAC block (no label) */

void StepPaletteFade(void)
{
    if ((DB(DS_FadeOutActive) | DB(DS_FadeInActive)) == 0) return;
    SET_DB(DS_PaletteFadeStep, (uint8_t)(DB(DS_PaletteFadeStep) + 1));
    uint16_t si = DW((uint16_t)(DS_LevelPaletteTable + 2 * DB(DS_CurrentLevel)));
    port_out8(0x3C8, 0);
    uint16_t bx = FADE_TARGET_PALETTE;
    uint16_t bp = 0;
    if (DB(DS_FadeInActive) != 0) { uint16_t t = si; si = bx; bx = t; }
    uint8_t step = DB(DS_PaletteFadeStep);
    for (int k = 0; k < 0x30; k++) {
        uint8_t src = DB(si);
        si = (uint16_t)(si + 1);
        uint8_t tgt = DB(bx);
        uint8_t dl = step, dh = step;
        uint8_t al = (uint8_t)(src - tgt);
        if (al & 0x80) { al = (uint8_t)(-al); dh = (uint8_t)(-dh); }
        uint8_t out;
        if (al > dl) { out = (uint8_t)(src - dh); bp++; }
        else out = tgt;
        port_out8(0x3C9, out);
        bx = (uint16_t)(bx + 1);
    }
    if (bp == 0) { SET_DB(DS_FadeOutActive, 0); SET_DB(DS_FadeInActive, 0); }
}

/* The pause: byte 280Dh is KeyDownTable[19h], the P key (KeyboardIsr writes it). With P down
 * the game waits for its release, the next press and that release; the keyboard interrupt
 * ends each busy-wait, so every turn is a service point (platform_poll). */
static void WaitFrameSyncFlag(void)
{
    if (DB(DS_byte_280D) == 0) return;
    while (DB(DS_byte_280D) != 0) platform_poll();
    while (DB(DS_byte_280D) == 0) platform_poll();
    while (DB(DS_byte_280D) != 0) platform_poll();
}

/* ---- physics helpers ------------------------------------------------------------------- */

/* PlayerVelX += (facing * 16) sar MotionShift while HorizontalInputHeld; clamp to +/-bp. */
static void ApplyPlayerAcceleration(uint16_t bp)
{
    uint16_t ax = 0;
    if (DB(DS_HorizontalInputHeld) != 0) ax = U16(DW(DS_PlayerFacing) << 4);
    unsigned sh = DB(DS_MotionShift);
    if (sh > 15) sh = 15;
    ax = U16(S16(ax) >> sh);
    uint16_t dx = U16(DW(DS_PlayerVelX) + ax);
    uint16_t out = bp;
    if (S16(dx) < S16(bp)) {
        uint16_t nb = U16(-S16(bp));
        out = (S16(dx) <= S16(nb)) ? nb : dx;
    }
    SET_DW(DS_PlayerVelX, out);
}

/* PlayerVelX -= SnowWind >> 3 (unsigned shift); floors PlayerVelX at -60h (signed). */
static void ApplyWindDrift(void)
{
    uint16_t ax = U16(DW(DS_SnowWind) >> 3);
    uint16_t v = U16(DW(DS_PlayerVelX) - ax);
    SET_DW(DS_PlayerVelX, v);
    if (S16(DW(DS_PlayerVelX)) < -0x60) SET_DW(DS_PlayerVelX, 0xFFA0);
}

/* PlayerVelY = min(PlayerVelY + g, bp) (signed); g = 10h, or 4 with cap bp/8 in AirFlags 1. */
static void ApplyPlayerGravity(uint16_t bp)
{
    uint16_t dx = DW(DS_PlayerVelY);
    uint16_t ax = 0x10;
    if (DB(DS_PlayerAirFlags) == 1) {
        ax = 4;
        bp = U16(S16(bp) >> 3);
    }
    dx = U16(dx + ax);
    if (S16(dx) < S16(bp)) bp = dx;
    SET_DW(DS_PlayerVelY, bp);
}

/* Moves |PlayerVelX| toward zero by 0Ch >> MotionShift, floored at 0, sign kept. The shift count
 * is CL masked to 5 bits (the oracle's 386 shift; the game writes MotionShift 0..3 only). */
void ApplyPlayerFriction(void)
{
    uint8_t ch = 0;
    uint16_t ax = DW(DS_PlayerVelX);
    if (S16(ax) < 0) {
        ax = U16(-ax);
        ch = 1;
    }
    unsigned cl = DB(DS_MotionShift) & 0x1Fu;          /* the 386 shift count is masked to 5 bits */
    uint16_t dx = (cl >= 16) ? 0 : U16(0x0C >> cl);
    if (ax >= dx) ax = U16(ax - dx);
    else ax = 0;
    if (ch) ax = U16(-ax);
    SET_DW(DS_PlayerVelX, ax);
}

/* SetPlayerAnimLocked (keyed by SubAnimKey): AL = key, BX = 2*anim. Returns BX = PlayerAnimPtr. */
static uint16_t SetPlayerAnimLocked(uint8_t al, uint16_t bx)
{
    if (DB(DS_SubAnimKey) == al) return DW(DS_PlayerAnimPtr);
    SET_DB(DS_SubAnimKey, al);
    bx = DW(U16(DS_AnimSeqPtrTable + bx));
    SET_DW(DS_PlayerAnimPtr, bx);
    return bx;
}

/* SetPlayerAnim (keyed by CurrentAnimId). Returns BX = PlayerAnimPtr. */
static uint16_t SetPlayerAnim(uint8_t al, uint16_t bx)
{
    if (DB(DS_CurrentAnimId) == al) return DW(DS_PlayerAnimPtr);
    SET_DB(DS_CurrentAnimId, al);
    bx = DW(U16(DS_AnimSeqPtrTable + bx));
    SET_DW(DS_PlayerAnimPtr, bx);
    return bx;
}

/* Steps the animation script at BX: a negative word is a relative jump. */
void AdvancePlayerAnim(uint16_t bx)
{
    uint16_t ax = DW(bx);
    if (S16(ax) < 0) {
        bx = U16(bx + ax);
        ax = DW(bx);
    }
    SET_DB(DS_FrameFlagsRaw, (uint8_t)(ax >> 8));
    uint8_t ah = (uint8_t)((ax >> 8) & 0x1F) | (DB(DS_PlayerFacing) & 0x80);
    SET_DW(DS_PlayerFrameWord, U16((ah << 8) | (ax & 0xFF)));
    bx = U16(bx + 2);
    SET_DW(DS_PlayerAnimPtr, bx);
}

/* Air step: acceleration 50h, gravity cap 0C0h, then the air frame (alt: 2Dh/2Eh; normal: 0Ch/0Dh
 * while falling and not attacking, with JumpCooldown = 6). */
static void ApplyAirMotionAnim(void)
{
    ApplyPlayerAcceleration(0x50);
    ApplyPlayerGravity(0xC0);
    uint8_t al;
    if (DB(DS_PlayerAltModeFlag) != 0) {
        al = 0x2D;
        if (S16(DW(DS_PlayerVelY)) >= 0) al = 0x2E;
    } else {
        if (S16(DW(DS_PlayerVelY)) <= 0) return;
        SET_DB(DS_JumpCooldown, 6);
        if (DB(DS_AttackAnimLock) != 0) return;
        al = 0x0C;
        if (DB(DS_FallCounter) >= 0x0C) al = 0x0D;
    }
    SET_DW(DS_PlayerFrameWord, U16(((DB(DS_PlayerFacing) & 0x80) << 8) | al));
}

/* MaybeEmitPlayerTrail: a trail record every fourth frame (SnowFrameTick low bits zero). */
static void MaybeEmitPlayerTrail(void)
{
    if ((DB(DS_SnowFrameTick) & 3) != 0) return;
    EmitPlayerTrail();
}

/* EmitPlayerTrail: record (X, Y, 35h) at PopupRingHead; the ring of five slots 4F76h..4FBEh
 * (stride 12h, descending, a step below 4F76h wraps to 4FBEh). */
static void EmitPlayerTrail(void)
{
    EmitPlayerTrailAt(DW(DS_PlayerX), DW(DS_PlayerY));
}

void EmitPlayerTrailAt(uint16_t ax, uint16_t dx)
{
    uint16_t bx = DW(DS_PopupRingHead);
    SET_DW(bx, ax);
    SET_DW(U16(bx + 2), dx);
    SET_DW(U16(bx + 4), 0x35);
    bx = U16(bx - 0x12);
    if (bx < 0x4F76) bx = 0x4FBE;
    SET_DW(DS_PopupRingHead, bx);
}

static void IncrementChargeLevel(void)
{
    if (DB(DS_ChargeLevel) <= 0x30) SET_DB(DS_ChargeLevel, U16(DB(DS_ChargeLevel) + 2));
}

/* ---- the state handlers ---------------------------------------------------------------- */

/* Attack lock set: the other states hand over to the attack body with AL = CurrentAnimId. */
static void ContinueAttackAnim(uint16_t bx, uint16_t cx, uint16_t si)
{
    UpdatePlayerStateAttack(DB(DS_CurrentAnimId), bx, cx, si);
}

/* Anim id from the dispatch BX (loc_5DED): PlayerAnimPtr, FrameWord, SubAnimKey. */
static void IdleAnimFromDispatch(uint16_t bx)
{
    bx = DW(U16(DS_AnimSeqPtrTable + bx));
    SET_DW(DS_PlayerAnimPtr, bx);
    uint16_t ax = DW(bx);
    uint8_t dh = DB(DS_PlayerFacing) & 0x80;
    ax = U16((ax & 0x00FF) | (((ax >> 8) | dh) << 8));
    SET_DW(DS_PlayerFrameWord, ax);
    SET_DB(DS_SubAnimKey, 0);
}

/* State 0 (idle): stand, look-around and the falling/airborne case with no input. */
static void UpdatePlayerStateIdle(uint8_t al, uint16_t bx, uint16_t cx)
{
    (void)al;
    if (DB(DS_AttackAnimLock) != 0) {
        ContinueAttackAnim(bx, cx, PLAYER_SI_AT_DISPATCH);
        return;
    }
    SET_DB(DS_StrokeCounter, 0);
    ApplyWindDrift();
    ApplyPlayerFriction();
    if (DB(DS_PlayerOnPlatform) == 0 && DW(DS_PlayerVelY) != 0) {
        if (DB(DS_JumpFrame) > 4) ApplyWindDrift();
        return;                                          /* loc_5E0D: CurrentAnimId kept */
    }
    uint16_t ax = DW(DS_PlayerVelX);
    if (S16(ax) < 0) ax = U16(-ax);
    if (ax >= 8) {
        uint8_t fh = DB(DS_PlayerFrameHigh), vh = DB(DS_PlayerVelXHigh);
        uint8_t fb = (uint8_t)(((fh << 1) | (fh >> 7)) & 1);
        uint8_t vb = (uint8_t)(((vh << 1) | (vh >> 7)) & 1);
        if (fb == vb) {
            bx = SetPlayerAnimLocked(0x12, 0x24);
            AdvancePlayerAnim(bx);
            MaybeEmitPlayerTrail();
        } else {
            IdleAnimFromDispatch(bx);
        }
        SET_DB(DS_CurrentAnimId, 0);
        return;
    }
    if (ax != 0) {
        IdleAnimFromDispatch(bx);
        SET_DB(DS_CurrentAnimId, 0);
        return;
    }
    /* |VelX| == 0 */
    if (DB(DS_PlayerActionTicks) >= 0x1E) {
        if (DB(DS_byte_27E9) == 0 && (DB(DS_byte_27EC) & DB(DS_byte_27ED)) == 0) {
            SET_DB(DS_PlayerActionTicks, U16(DB(DS_PlayerActionTicks) - 3));
            bx = SetPlayerAnimLocked(0x10, 0x20);
            AdvancePlayerAnim(bx);
            SET_DB(DS_CurrentAnimId, 0);
            return;
        }
    } else if ((DB(DS_byte_27EC) & DB(DS_byte_27ED)) == 0 && DB(DS_byte_27E9) == 0) {
        goto table_search;
    }
    /* loc_5D83: look-around (13h) when unpanned */
    if (DB(DS_PlayerOnPlatform) != 0) goto table_search;
    bx = SetPlayerAnimLocked(0x13, 0x26);
    AdvancePlayerAnim(bx);
    if ((DB(DS_LevelFlags) & 2) == 0 && DB(DS_ScrollFollowGate) == 0) {
        uint16_t d = U16(SAR(DW(DS_PlayerX), 4) - DW(DS_CameraCol));
        if ((DB(DS_PlayerFrameHigh) & 0x80) == 0) {
            if (d > 2) ScrollViewRight();
        } else {
            if (d < 0x11) ScrollViewLeft();
        }
    }
    SET_DB(DS_CurrentAnimId, 0);
    return;
table_search:
    {
        uint16_t key = U16(DW(DS_word_27F0) & 0x1FF);
        uint16_t si = 0x79E0;
        for (;;) {
            if (key < DW(si)) {                              /* loc_5DED */
                IdleAnimFromDispatch(bx);
                SET_DB(DS_CurrentAnimId, 0);
                return;
            }
            if (key < DW(U16(si + 2))) {                     /* loc_5DE0 */
                bx = SetPlayerAnimLocked(0x11, 0x22);
                AdvancePlayerAnim(bx);
                SET_DB(DS_CurrentAnimId, 0);
                return;
            }
            si = U16(si + 4);
        }
    }
}

/* State 2 jump body (UpdatePlayerStateJump at loc_5F41 and the bounce fall-through). */
static void JumpBody(uint8_t al, uint16_t bx, uint16_t cx)
{
    (void)al; (void)bx; (void)cx;
    SET_DB(DS_PlayerOnPlatform, 0);
    uint8_t bl = DB(DS_JumpFrame);
    SET_DB(DS_JumpFrame, U16(bl + 1));
    if (bl < 9) {
        int16_t v = S16(DW(U16(DS_JumpArcVelTable + 2u * bl)));
        if (DB(DS_PlayerAltModeFlag) != 0) v = (int16_t)(v >> 1);
        SET_DW(DS_PlayerVelY, U16(DW(DS_PlayerVelY) + U16(v)));
    } else {
        ApplyPlayerGravity(0xC0);
    }
    if (DW(DS_PlayerVelX) < 0x30) ApplyPlayerAcceleration(0x30);
    else ApplyPlayerFriction();
    bx = SetPlayerAnim(2, 4);
    AdvancePlayerAnim(bx);
    ApplyWindDrift();
    ApplyWindDrift();
}

/* State 2 (jump). */
static void UpdatePlayerStateJump(uint8_t al, uint16_t bx, uint16_t cx)
{
    if (DB(DS_AttackAnimLock) != 0) {
        ContinueAttackAnim(bx, cx, PLAYER_SI_AT_DISPATCH);
        return;
    }
    if (DB(DS_JumpCooldown) != 0) {
        UpdatePlayerStateIdle(al, bx, cx);
        return;
    }
    JumpBody(al, bx, cx);
}

/* Alt-mode state 2: bounce when StrokeCounter-like byte 6BC8h reached 18h, else the jump body. */
static void UpdatePlayerAltStateBounce(uint8_t al, uint16_t bx, uint16_t cx)
{
    if (DB(DS_StrokeCounter) < 0x18) {
        JumpBody(al, bx, cx);
        return;
    }
    SET_DB(DS_PlayerAirFlags, 1);
    SET_DB(DS_MomentumCounter, 0x18);
    SET_DW(DS_PlayerY, U16(DW(DS_PlayerY) - 3));
    SET_DB(DS_StrokeCounter, 0);
    ApplyWindDrift();
    ApplyWindDrift();
}

/* State 4 (charge): accelerate to 20h, else fall through to the idle handler. */
static void UpdatePlayerStateCharge(uint8_t al, uint16_t bx, uint16_t cx)
{
    if (DB(DS_AttackAnimLock) != 0) {
        ContinueAttackAnim(bx, cx, PLAYER_SI_AT_DISPATCH);
        return;
    }
    SET_DB(DS_PlayerActionTicks, 0);
    SET_DB(DS_LandingBlockTimer, 4);
    IncrementChargeLevel();
    uint16_t ax = DW(DS_PlayerVelX);
    if (S16(ax) < 0) ax = U16(-ax);
    if (ax > 0x20) {
        UpdatePlayerStateIdle(al, bx, cx);
        return;
    }
    ApplyPlayerAcceleration(0x20);
    /* AL is the low byte of |PlayerVelX| here (the compare left AX as the speed) */
    bx = SetPlayerAnim((uint8_t)ax, bx);
    AdvancePlayerAnim(bx);
}

/* State 5 (crouch, charge): stroke reset, anim, friction, charge. */
static void UpdatePlayerStateCrouch(uint8_t al, uint16_t bx, uint16_t cx)
{
    if (DB(DS_AttackAnimLock) != 0) {
        ContinueAttackAnim(bx, cx, PLAYER_SI_AT_DISPATCH);
        return;
    }
    SET_DB(DS_StrokeCounter, 0);
    SET_DB(DS_LandingBlockTimer, 4);
    bx = SetPlayerAnim(al, bx);
    AdvancePlayerAnim(bx);
    ApplyPlayerFriction();
    IncrementChargeLevel();
}

/* State 1 (run): accelerate to 50h, wind drift, anim; alt mode counts strokes and emits a trail. */
static void UpdatePlayerStateRun(uint8_t al, uint16_t bx, uint16_t cx)
{
    if (DB(DS_AttackAnimLock) != 0) {
        ContinueAttackAnim(bx, cx, PLAYER_SI_AT_DISPATCH);
        return;
    }
    uint16_t ax = DW(DS_PlayerVelX);
    if (S16(ax) < 0) ax = U16(-ax);
    if (S16(ax) >= 0x40 && DB(DS_PlayerAltModeFlag) != 0) {
        inc8_sat(DS_StrokeCounter);
        if (DB(DS_StrokeCounter) == 0x17) EmitPlayerTrail();
    }
    inc8_sat(DS_PlayerActionTicks);
    ApplyPlayerAcceleration(0x50);
    ApplyWindDrift();
    bx = SetPlayerAnim(al, bx);
    AdvancePlayerAnim(bx);
}

/* Attack render path (6081): picks the frame and the display from the table of the weapon
 * (key = frame word, 8-byte rows). With no key match nothing is written. */
static void attack_render(uint16_t bx, uint16_t cx)
{
    (void)cx;
    uint16_t s2 = DW(U16(DS_word_7B04 + bx));
    uint16_t ax = DW(DS_PlayerFrameWord);
    uint8_t dh = (uint8_t)(ax >> 8);
    ax = U16((ax & 0x00FF) | (ax & 0x1F00u));
    uint16_t b = 0xFFF8;
    for (;;) {
        b = U16(b + 8);
        uint16_t key = DW(U16(b + s2));
        if (key == 0x55AA) return;                          /* loc_60DE */
        if (key != ax) continue;
        uint16_t sp = DW(U16(b + s2 + 2));
        uint16_t cxm = DW(U16(b + s2 + 4));
        if (dh & 0x80) {
            sp = U16((sp & 0x00FF) | ((((sp >> 8) | 0x80) & 0xFF) << 8));
            cxm = U16(-cxm);
        }
        attack_tail(sp, cxm, b, s2);
        return;
    }
}

/* PlayerSprite, RenderSlots and PlayerRenderY of the attack tail (60AE). BX and SI are the
 * values live at the jump (see the header note); CX is the offset subtracted from X. */
static void attack_tail(uint16_t sprite, uint16_t cx, uint16_t bx, uint16_t si)
{
    SET_DW(DS_PlayerSprite, sprite);
    SET_DW(DS_RenderSlots, U16(DW(DS_PlayerX) + SAR(DW(DS_PlayerVelX), 4) - cx));
    SET_DW(DS_PlayerRenderY, U16(DW(DS_PlayerY) + SAR(DW(DS_PlayerVelY), 4) - DW(U16(bx + si + 6))));
}

/* States 3, 6 and 7 (attack). Also entered from the attack lock of the other states. */
static void UpdatePlayerStateAttack(uint8_t al, uint16_t bx, uint16_t cx, uint16_t si)
{
    bx = SetPlayerAnim(al, bx);
    AdvancePlayerAnim(bx);
    ApplyPlayerFriction();
    inc8_sat(DS_PlayerActionTicks);
    bx = U16(5u * DB(DS_PlayerWeaponType));
    uint8_t pw = DB(U16(DS_byte_7B07 + bx));
    if (DB(DS_ChargeLevel) != 0) pw = (uint8_t)(pw << 2);
    SET_DB(DS_PlayerAttackPower, pw);
    uint8_t lock = (uint8_t)((DB(DS_FrameFlagsRaw) & 0x40) ^ 0x40);
    SET_DB(DS_AttackAnimLock, lock);
    if (lock != 0) {
        attack_render(bx, cx);
        return;
    }
    SET_DB(DS_AttackRecoveryTimer, DB(U16(DS_byte_7B06 + bx)));
    uint16_t dx = 5;
    uint8_t wt = DB(DS_PlayerWeaponType);
    if (wt != 0) {
        dx = 0;
        if (wt != 1) dx = 0x0A;
    }
    StartSoundEffect(dx);
    uint16_t dy = 0;
    uint8_t ca = DB(DS_CurrentAnimId);
    if (ca != 6) {
        dy = 0xFFE0;
        if (ca != 3) {
            dy = 0xFFD0;
            MaybeEmitPlayerTrail();
        }
    }
    if (DB(DS_PlayerOnPlatform) == 0) SET_DW(DS_PlayerVelY, U16(DW(DS_PlayerVelY) + dy));

    uint8_t fl = DB(U16(DS_byte_7B08 + bx));
    if (fl & 1) {
        uint8_t kind = (uint8_t)((fl >> 1) & 3);
        uint16_t slot;
        if (FindFreeProjectileSlot(&slot) == 0) {
            si = slot;
            SET_DB(U16(si + 8), kind);
            uint16_t bxp = DW(U16(DS_word_7B04 + bx));
            do { bxp = U16(bxp + 2); } while (DW(bxp) != 0x55AA);
            bxp = U16(bxp + 6);
            SET_DW(U16(si + 0x0C), bxp);
            uint16_t ax2 = DW(bxp);
            uint16_t cxp = DW(U16(bxp - 4));
            uint16_t b2 = DW(U16(bxp - 2));
            SET_DW(U16(si + 0x0E), b2);
            uint8_t dhf = DB(DS_PlayerFacing) & 0x80;
            if (dhf) {
                ax2 = U16((ax2 & 0x00FF) | ((((ax2 >> 8) | dhf) & 0xFF) << 8));
                cxp = U16(-cxp);
            }
            SET_DW(U16(si + 4), ax2);
            SET_DW(U16(si + 6), cxp);
            SET_DW(U16(si + 0), U16(DW(DS_RenderSlots) + SAR(cxp, 4)));
            SET_DW(U16(si + 2), U16(DW(DS_PlayerRenderY) + SAR(b2, 4)));
            /* the shifts leave BX = b2>>4 and CX = cxp>>4 at the tail */
            attack_tail(0xFFFF, SAR(cxp, 4), SAR(b2, 4), si);
            return;
        }
        si = slot;                                          /* failed search leaves SI = 4F76h */
    }
    if (DB(DS_FallCounter) != 0) {
        attack_tail(0xFFFF, cx, bx, si);
        return;
    }
    attack_render(bx, cx);
}

/* State 8 (hurt): wind, friction, anim. In alt mode the table entry is data (see the header). */
static void UpdatePlayerStateHurt(uint8_t al, uint16_t bx, uint16_t cx)
{
    (void)al; (void)cx;
    ApplyWindDrift();
    ApplyPlayerFriction();
    /* ApplyPlayerFriction leaves AX = the new PlayerVelX (no push ax), so SetPlayerAnim gets
     * AL = its low byte, not the dispatch AL 8 */
    al = (uint8_t)DW(DS_PlayerVelX);
    bx = SetPlayerAnim(al, bx);
    AdvancePlayerAnim(bx);
}

/* ---- projectiles, particles and debris ------------------------------------------------- */

/* Projectile kind 0 and 1 per-frame steps (VelY at +0Eh). */
static void ApplyProjectileHeavy(uint16_t si) { SET_DW(U16(si + 0x0E), U16(DW(U16(si + 0x0E)) + 0x20)); }
static void ApplyProjectileLight(uint16_t si) { SET_DW(U16(si + 0x0E), U16(DW(U16(si + 0x0E)) - 0x10)); }

/* Finds a free projectile slot (frame word FFFFh) among the four slots at 4F2Eh. Returns 0 (CF
 * clear) with *si_out = the slot, or 1 (CF set) with *si_out = 4F76h (the end of the search). */
int FindFreeProjectileSlot(uint16_t *si_out)
{
    uint16_t s = PROJECTILE_SLOTS_BASE;
    for (int n = 0; n < 4; n++) {
        if (DW(U16(s + 4)) == 0xFFFF) {
            *si_out = s;
            return 0;
        }
        s = U16(s + PROJECTILE_SLOT_SIZE);
    }
    *si_out = s;
    return 1;
}

/* Updates the four projectile slots (frame words, positions, frame script, kind handler).
 * Slots with frame word FFFFh are skipped; a slot without bit 5 of its flags is freed. */
int UpdatePlayerProjectiles(void)
{
    for (int n = 0; n < 4; n++) {
        uint16_t s = U16(PROJECTILE_SLOTS_BASE + n * PROJECTILE_SLOT_SIZE);
        if (DW(U16(s + 4)) == 0xFFFF) continue;
        if ((DB(U16(s + 5)) & 0x20) == 0) {
            SET_DW(U16(s + 4), 0xFFFF);
            continue;
        }
        SET_DW(s, U16(DW(s) + SAR(DW(U16(s + 6)), 4)));
        SET_DW(U16(s + 2), U16(DW(U16(s + 2)) + SAR(DW(U16(s + 0x0E)), 4)));
        uint16_t bx = DW(U16(s + 0x0C));
        uint16_t ax = DW(bx);
        if (S16(ax) < 0) {
            bx = U16(bx + ax);
            ax = DW(bx);
        }
        bx = U16(bx + 2);
        SET_DW(U16(s + 0x0C), bx);
        uint8_t hi = (uint8_t)((ax >> 8) | (DB(U16(s + 7)) & 0x80));
        ax = U16((ax & 0x00FF) | ((uint16_t)hi << 8));
        SET_DW(U16(s + 4), ax);
        uint8_t kind = DB(U16(s + 8));
        int st = invoke(DW(U16(DS_ProjectileKindHandlers + 2u * kind)), 0, 0, 0, s, 0);
        if (st != PLAYER_OK) return st;
    }
    return PLAYER_OK;
}

/* Burst particle physics (UpdateBurstParticles, loc_6139..61E3), slot at si, timer positive. */
static void BurstPhysics(uint16_t si)
{
    uint16_t ax = U16(SAR(DW(U16(si + 6)), 4));
    SET_DW(si, U16(DW(si) + ax));
    if (S16(DW(si)) < 0) {
        SET_DW(si, 0);
        SET_DW(U16(si + 6), U16(-DW(U16(si + 6))));
    }
    if (DW(si) >= 0x1000) SET_DW(U16(si + 6), U16(-DW(U16(si + 6))));
    uint16_t vy = DW(U16(si + 0x0E));
    int16_t vy4 = (int16_t)(S16(vy) >> 4);
    SET_DW(U16(si + 2), U16(DW(U16(si + 2)) + U16(vy4)));
    uint16_t dxv = U16(vy + 9);
    if (S16(dxv) < 0x100) SET_DW(U16(si + 0x0E), dxv);
    uint16_t key = DW(U16(si + 4)) & 0x1FFF;
    if (key == 0x136 || key == 0x134 || key == 0x12C || key == 0x132) {
        if (DW(U16(si + 0x0C)) > 0x32) SET_DW(U16(si + 0x0C), 0x32);
        return;
    }
    if (key == 0x0E5) return;
    uint16_t cell = U16((((SAR(DW(U16(si + 2)), 4)) & 0xFF) << 8) | (SAR(DW(si), 4) & 0xFF));
    if (vy4 > 0) {
        if (DB(U16(DS_GroundTypeTable + ESB(cell))) != 0) {
            SET_DW(U16(si + 0x0E), U16(S16(U16(-S16(DW(U16(si + 0x0E))))) >> 1));
            uint16_t vx = DW(U16(si + 6));
            int16_t step = (S16(vx) < 0) ? -8 : 8;
            uint16_t nv = U16(vx - U16(step));
            if (((nv >> 8) ^ DB(U16(si + 7))) != 0) nv = 0;
            SET_DW(U16(si + 6), nv);
        }
    } else {
        if (DB(U16(DS_SideTileClassTable + ESB(U16(cell - 0x100)))) != 0) {
            SET_DW(U16(si + 6), U16(-DW(U16(si + 6))));
            SET_DW(si, U16(DW(si) + SAR(DW(U16(si + 6)), 4)));
        }
    }
    /* frame animation: every second frame, keys 46h..49h cycle */
    if ((DB(DS_SnowFrameTick) & 1) != 0) return;
    if (DW(U16(si + 6)) == 0) return;
    uint16_t f = DW(U16(si + 4));
    uint16_t nf = U16(f + 1);
    uint16_t masked = f & 0x1FFF;
    if (masked < 0x49) SET_DW(U16(si + 4), nf);
    else if (masked == 0x49) SET_DW(U16(si + 4), 0x46);
}

/* Updates the 32 burst/particle slots at 50A8h (stride 12h): timer, motion, tile bounce, frames. */
void UpdateBurstParticles(void)
{
    SET_ES(LSEG());                     /* ES = LevelSegment for the tile probes; it stays so at the return */
    for (int n = 0; n < 32; n++) {
        uint16_t si = U16(PARTICLE_BASE + n * 0x12);
        if (DW(U16(si + 4)) == 0xFFFF) continue;
        uint16_t t = U16(DW(U16(si + 0x0C)) - 1);
        SET_DW(U16(si + 0x0C), t);
        if (S16(t) > 0) {
            BurstPhysics(si);
        } else if (t == 0) {
            SET_DB(U16(si + 0x11), 0x0F);
        } else if (DB(U16(si + 0x11)) == 0) {
            SET_DW(U16(si + 4), 0xFFFF);
        }
    }
}

/* Ticks the 16 debris slots at 5450h (stride 12h): Y and lifetime down, freed at lifetime 0. */
void TickDebrisSlots(void)
{
    for (int n = 0; n < 16; n++) {
        uint16_t si = U16(DEBRIS_BASE + n * 0x12);
        if (DW(U16(si + 4)) == 0xFFFF) continue;
        SET_DW(U16(si + 2), U16(DW(U16(si + 2)) - 1));
        uint16_t t = U16(DW(U16(si + 0x0C)) - 1);
        SET_DW(U16(si + 0x0C), t);
        if (t == 0) SET_DW(U16(si + 4), 0xFFFF);
    }
}

/* ---- adapters (differential tests) ------------------------------------------------------ */

EXPORT int c_UpdatePlayer(Regs *r) { int st = UpdatePlayer(r->cx); r->ds = DATA_SEG; return st; }
EXPORT int c_UpdatePlayerCollision(Regs *r) { int st = UpdatePlayerCollision(); r->ds = DATA_SEG; return st; }
EXPORT int c_UpdatePlayerTileContact(Regs *r) { int st = UpdatePlayerTileContact(r->bx); r->ds = DATA_SEG; return st; }
EXPORT int c_UpdatePlayerProjectiles(Regs *r) { int st = UpdatePlayerProjectiles(); r->ds = DATA_SEG; return st; }
EXPORT int c_UpdateBurstParticles(Regs *r) { UpdateBurstParticles(); r->ds = DATA_SEG; return 0; }
EXPORT int c_TickDebrisSlots(Regs *r) { TickDebrisSlots(); r->ds = DATA_SEG; return 0; }
EXPORT int c_FindFreeProjectileSlot(Regs *r)
{
    uint16_t si = 0;
    int cf = FindFreeProjectileSlot(&si);
    r->si = si;
    set_cf(r, cf);
    r->ds = DATA_SEG;
    return 0;
}
EXPORT int c_LandPlayerOnGround(Regs *r) { LandPlayerOnGround(r->di); r->ds = DATA_SEG; return 0; }
EXPORT int c_GroundTileSnapOrFall(Regs *r) { GroundTileSnapOrFall(r->di); r->ds = DATA_SEG; return 0; }
EXPORT int c_CeilingTileHeadBump(Regs *r) { CeilingTileHeadBump(r->di); r->ds = DATA_SEG; return 0; }
EXPORT int c_SideTileWallImpactHandler(Regs *r) { SideTileWallImpactHandler(r->di); r->ds = DATA_SEG; return 0; }
EXPORT int c_RedrawTileIfVisible(Regs *r)
{
    set_cf(r, RedrawTileIfVisible(r->ax, r->di));
    r->ds = DATA_SEG;
    return 0;
}
EXPORT int c_TriggerPlayerRespawnCore(Regs *r) { TriggerPlayerRespawnCore(); r->ds = DATA_SEG; return 0; }
EXPORT int c_IncrementPlayerLives(Regs *r) { IncrementPlayerLives(); r->ds = DATA_SEG; return 0; }
EXPORT int c_ApplyPlayerFriction(Regs *r) { ApplyPlayerFriction(); r->ds = DATA_SEG; return 0; }
EXPORT int c_sub_67D7(Regs *r) { sub_67D7(); r->ds = DATA_SEG; return 0; }
EXPORT int c_unused_66D3(Regs *r) { set_cf(r, unused_66D3(r->si, r->di)); r->ds = DATA_SEG; return 0; }
EXPORT int c_unused_6760(Regs *r) { r->ax = unused_6760(r->si); r->ds = DATA_SEG; return 0; }
EXPORT int c_StepPaletteFade(Regs *r) { StepPaletteFade(); r->ds = DATA_SEG; return 0; }
