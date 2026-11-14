#ifndef PRE2_ENH_MARGIN_H
#define PRE2_ENH_MARGIN_H
#include "frame.h"

/* Read-only source observation at CRT publication. Never calls the simulation's
 * projectors, allocators, script handlers or sprite planner. Off in wide-sim. */
void enh_margin_capture(FrameGameView *view);
void enh_margin_enable(int enabled); /* diagnostic on/off purity comparison */
#endif
