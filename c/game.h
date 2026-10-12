/* Program entry, level restart, main frame loop (see game.c). */
#ifndef GAME_H
#define GAME_H
#include <stdint.h>

enum { GAME_UNSUPPORTED = 600 };    /* a path whose register state is not established */

int ProgramEntry(void);
int LevelRestart(uint16_t ax);
int LevelStart(uint32_t es);
int MainFrameStep(uint16_t *ax_out);
int MainFrameLoop(uint16_t *ax_out);   /* returns 0 when UpdateLevelState requests a restart */
#endif
