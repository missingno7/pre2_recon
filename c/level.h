/* HUD, terrain entities, effect particles, camera shake, level state machine, death bounce,
 * tally panel, level resets, triggers, fireflies, camera chain, popup ring and EnterLevelView
 * (oracle: src/MODULE3.ASM cs:454E..584F, plus EnterLevelView at cs:3AF2 in src/MODULE2.ASM).
 *
 * Contracts follow the ASM comments above each label. Routines that return CF return it as
 * an int (1 = carry set). UpdateLevelState also returns AX through *ax.
 *
 * Region edge: the routines below are owned by other regions (player 5850..6821, objects
 * 6822..8294, effects 8295..951F, MODULE2 screens, MODULE4). They are declared here with the
 * contracts the ASM gives and defined in level.c as weak stubs that abort when reached, so the
 * DLL links while they are not translated; a strong definition from their C file replaces the
 * stub. Calls to them are direct C calls. */
#ifndef LEVEL_H
#define LEVEL_H
#include <stdint.h>

/* UpdateHud: the DIV of the score by 25000 faults (quotient above FFFFh, #DE in the original).
 * Returned before any write; the fault handler is not modelled. */
#define LEVEL_DIVIDE_FAULT 1

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

/* ---- region edge: owned by other regions, weak stubs in level.c -------------------------- */
int  TestHitboxOverlap(uint16_t si, uint16_t di);     /* CF = boxes overlap */
int  RedrawTileIfVisible(uint16_t di);                /* CF = 1 when outside the view window */
void IncrementPlayerLives(void);
void SpawnDebrisElement(uint16_t ax, uint16_t si);
void TickDebrisSlots(void);
void UpdateBurstParticles(void);
void UpdatePlayerProjectiles(void);
void UpdateObjects(void);
void BuildEffectDrawList(void);
void ShowMode13PictureFadeIn(uint16_t seg, uint16_t si, uint8_t count);
void FadeOutPaletteToBlack(void);
int  TryShowIntroKeyed(void);                         /* CF = 1 when the gate fails */
int  ShowIntroPicture(void);                          /* CF = 1 when the clock year is below 1994 */
void RunGameOverScreen(void);

#endif
