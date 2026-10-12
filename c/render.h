/* Video page, tile ring and level-loading routines (oracle: src/MODULE2.ASM, cs:3034..454D).
 * Routines keep their ASM names; return values follow the register contract of the adapter
 * (CF as the int result of the scroll routines: 1 = carry set, the edge was reached). */
#ifndef RENDER_H
#define RENDER_H
#include <stdint.h>

void EnableVideoOutput(void);
void DisableVideoOutput(void);
void RevealLevelColumns(void);
void CopyColumnStrip(uint16_t di);
void ClearStaggeredBlocks(void);
void ClearBlock16x10(uint16_t di);
void ClearDisplayPage(void);
void BlackoutSpan(uint16_t bx, uint16_t cx, uint16_t dx);
void RunIrisClose(void);

int  ScrollViewUp(uint8_t dl);
int  ScrollViewDown(uint8_t dl);
int  ScrollViewLeft(void);
int  ScrollViewRight(void);

uint16_t RestoreTileBackdrop(uint16_t di);
uint16_t BlitTileCell(uint16_t ax, uint16_t di);
uint16_t BlitTileToVram(uint16_t ax, uint16_t di);
uint16_t BlitTileToRing(uint16_t ax, uint16_t di);
void ComputeRingTileOffset(uint16_t col);
void DrawTileRow(uint16_t map, uint16_t di);
void DrawTileColumn(uint16_t map, uint16_t col);
void RedrawOverlayTiles(void);
void AnimateTiles(void);
void DrawForegroundTile(uint16_t ax, uint16_t bx);
void DrawForegroundOverSprites(void);

uint8_t NextWindRandom(void);
void UpdateWindAndFlakes(void);
void CopyRingWindowToPage(void);

void SetCpuWriteMode(void);
void SetLatchCopyMode(void);
void WaitVerticalRetrace(void);
void FlipDisplayPage(void);
void PresentFrame(void);
int  ProbeVideoMemory(void);
int  CheckVgaAdapter(void);
void SetLevelVideoMode(void);

uint16_t LoadLevelTileSet(void);
void UploadSpriteTiles(void);
void ClassifyTileCache(void);
void BuildAnimatedTileTables(void);
void ApplySecretTilePatches(void);
void RebaseEntitySpriteRefs(void);
void ScrollEffectSources(void);
void SavePristineTriggerRows(void);
void BuildDecorList(void);
int  LoadLevel(uint16_t ax);
void unused_443E(void);

#endif
