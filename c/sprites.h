/* Sprite pipeline (oracle: LoadSpriteSheet cs:2DFA and DrawSpriteObjects cs:26FA in src/MODULE2.ASM). */
#ifndef SPRITES_H
#define SPRITES_H
#include <stdint.h>

/* Loads SPRITES.SQZ and builds the frame tables; returns 0 or the SQZ error code. */
int LoadSpriteSheet(void);

/* Draws the object list into the draw page (see the contract above DrawSpriteObjects). */
void DrawSpriteObjects(void);

enum SpriteStatus {
    SPR_OK = 0,
    SPR_UNSUPPORTED = 404      /* a boundary of this region that is not translated (intro) */
};

/* RandomSeed = ror3(RandomSeed + 9248h); returns the new seed (AX). */
uint16_t StepRandomSeed(void);

#endif
