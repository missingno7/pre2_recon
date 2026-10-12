/* Sprite pipeline (oracle: LoadSpriteSheet cs:2DFA and DrawSpriteObjects cs:26FA in src/MODULE2.ASM). */
#ifndef SPRITES_H
#define SPRITES_H
#include <stdint.h>

/* Loads SPRITES.SQZ and builds the frame tables; returns 0 or the SQZ error code. */
int LoadSpriteSheet(void);

/* Draws the object list into the draw page (see the contract above DrawSpriteObjects). */
void DrawSpriteObjects(void);
#endif
