/* Front-end and fixed screens (oracle: src/MODULE3.ASM cs:8E45..9286 and src/MODULE4.ASM
 * cs:9520..9DFB, see screens.c). Routines keep their ASM names. Status results are 0 or
 * SCR_UNSUPPORTED: a boundary path (an unknown frame callback, or a resource location that
 * the platform services do not model) that the translation stops at. */
#ifndef SCREENS_H
#define SCREENS_H
#include <stdint.h>

enum ScreenStatus {
    SCR_OK = 0,
    SCR_UNSUPPORTED = 400     /* boundary: not translated, see screens.c */
};

/* MODULE4: world map scroll-in (stage marker stamp, scroll, hold, fade). */
void ShowWorldMapScrollIn(void);

/* MODULE4: MODE / ENTER CODE map screen. `callback` is the frame callback offset (CS):
 * UpdateModeSelectFrame or UpdateCodeEntryFrame. */
int RunCallbackMapScreen(uint16_t callback);
void UpdateModeSelectFrame(void);
void UpdateCodeEntryFrame(void);
void AdvanceMapFrame(void);

/* Draws the glyph string at DS:bx with the plane mask bp (1FFFh); returns BX after the
 * terminator (see screens.c). */
uint16_t DrawFontString(uint16_t bx, uint16_t bp);

void WaitVerticalRetraceStart(void);
void WaitNextVerticalRetrace(void);

/* MODULE4: GAME OVER screen (returns at once when byte_2879 is set). */
void RunGameOverScreen(void);
void CopyBackdropStrip(void);
void UpdateGameOverSprites(void);
void SwapSpriteRecords(uint16_t si, uint16_t di);
void CopySpriteRecord(uint16_t si, uint16_t di);

/* MODULE3: front-end flow (picture, MODE / ENTER CODE, exit). */
int RunFrontEnd(void);
/* Level-start intro: backdrop save, player and follower walk-in and walk-off. */
void RunLevelIntroScene(void);
int ShowPresentTitle(void);
int ShowTitusScreen(void);
void ShowPlanarPicture(uint16_t picseg);
/* Fades in the picture at LevelSegment (count = CL), BX in and out (BH:BL as the fade
 * leaves them: green/blue of the last entry read when OptionBits bit 0 is set). */
uint16_t ShowMode13PictureFadeIn(uint8_t count, uint16_t bx);
uint16_t SetPictureCopyPointers(void);
void FadeOutPaletteToBlack(void);

/* Advances a render slot's animation script and writes its frame (si = render slot; cs:9047). */
void StepSpriteAnimation(uint16_t si);

/* Copies 1F40h bytes of each VRAM plane from the saved backdrop (3F40h) to the draw page (cs:9071). */
void RestoreBackdropToDrawPage(void);

#endif
