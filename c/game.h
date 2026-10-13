/* Program entry, level restart, main frame loop (see game.c). */
#ifndef GAME_H
#define GAME_H
#include <stdint.h>

int ProgramEntry(void);
int LevelRestart(uint16_t ax);
int LevelStart(void);   /* the fill at loc_01C4 uses pre2_es */
int MainFrameStep(uint16_t *ax_out);
int MainFrameLoop(uint16_t *ax_out);
extern uint32_t pre2_frames_run;   /* MainFrameStep calls so far (host instrumentation) */
extern const char *pre2_phase;     /* top-level step running now (host instrumentation) */   /* returns 0 when UpdateLevelState requests a restart */
#endif
