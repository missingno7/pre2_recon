/* HUD, terrain entities, effect particles, camera shake, level state machine, death bounce,
 * tally panel, level resets, triggers, fireflies, camera chain, popup ring and EnterLevelView
 * (oracle: src/MODULE3.ASM cs:454E..584F, plus EnterLevelView at cs:3AF2 in src/MODULE2.ASM).
 *
 * Contracts follow the ASM comments above each label. Routines that return CF return it as
 * an int (1 = carry set). UpdateLevelState also returns AX through *ax.
 *
 * Routines of the other regions are not declared here: callers include the owner's header
 * (player.h, objects.h, effects.h, screens.h, sprites.h). */
#ifndef LEVEL_H
#define LEVEL_H
#include <stdint.h>

/* UpdateHud: the DIV of the score by 25000 faults (quotient above FFFFh, #DE in the original).
 * Returned before any write; the fault handler is not modelled. */
#define LEVEL_DIVIDE_FAULT 1

/* UpdateLevelState stopped at a boundary that is not translated (the intro gate or picture of
 * sprites.c, cs:25C7/25F6): the status is returned instead of running on. */
#define LEVEL_UNSUPPORTED  405

/* ---- HUD (cs:454E..4803, 46EE, 47C0) -------------------------------------------------- */
void DrawStatusBarBackground(void);
int  UpdateHud(void);                       /* 0, or LEVEL_DIVIDE_FAULT */
void ClearBonusLetterRow(void);
void BlitHudGlyph(uint8_t glyph, uint16_t di);
void BlitHudDigit(uint8_t digit, uint16_t di);
void BlitHudSpriteGlyph(uint8_t index, uint16_t di);
void FormatDecimal32(uint16_t cx, uint16_t dx);

/* ---- player render slot, terrain entities, effect particles (cs:484E..4C2F) -------------- */
void UpdatePlayerRenderSlot(void);
void UpdateTerrainEntities(void);
int  TryLandOnTerrainEntity(uint16_t di);   /* CF */
void UpdateEffectParticles(void);

/* ---- camera shake and level state (cs:4C30..4C68, 4C69..509C) ------------------------------ */
void UpdateCameraShake(void);
int  UpdateLevelState(uint16_t *ax);        /* returns CF; *ax receives AX */
void PlayDeathBounce(void);

/* ---- tally panel, item tally, level clears (cs:5139..52D1) -------------------------------- */
void RenderTallyPercent(void);
void DrawHudText(uint16_t si, uint16_t di);
void RenderTallyPanel(void);
void ResetItemTally(void);
void ResetItemTallyEs(uint16_t es);   /* the clear goes to ES:ItemTallyCounts */
void AnimateTallyPotFlame(void);
void ClearDrawPage(void);
void ResetLevelState(void);
void RestoreSceneryMap(void);

/* ---- triggers, player tile, fireflies, render slots (cs:52FE..55FB) ----------------------- */
void TryCaveTrigger(void);
void UpdateProximityTriggers(void);
uint16_t GetPlayerTile(void);               /* DX: DH = row (PlayerY >> 4), DL = column */
void UpdateFireflies(void);
void ResetRenderSlots(void);

/* ---- camera chain and popup ring (cs:5634..581D, 581E..584D) ------------------------------ */
void UpdateCameraFixedStep(void);
void UpdateCameraFollow(void);
void UpdateCameraScroll(void);
void UpdateVerticalCamera(void);
void UpdateHorizontalCamera(void);
void UpdatePopupRing(void);

/* ---- level view entry (MODULE2 cs:3AF2) ---------------------------------------------------- */
void EnterLevelView(void);

#endif
