#ifndef PRE2_OVERLAY_H
#define PRE2_OVERLAY_H
#include "settings.h"
#include <stdint.h>
typedef enum OverlayKey { OVERLAY_TOGGLE, OVERLAY_ESCAPE, OVERLAY_UP, OVERLAY_DOWN,
    OVERLAY_LEFT, OVERLAY_RIGHT, OVERLAY_ENTER, OVERLAY_TAB, OVERLAY_BACKTAB } OverlayKey;
typedef struct Pre2Overlay { int open,tab,row; } Pre2Overlay;
/* draw_ms: composing one presented image (presenter); show_ms: scaling, upload and the
 * swap (includes any wait for vertical sync). */
typedef struct OverlayPerf { double fps,tps,drift_ms; int paused; double draw_ms,show_ms; } OverlayPerf;
int overlay_rows(int tab);
/* SET_COUNT denotes an informational row. Navigation always skips it. */
Pre2Setting overlay_item(int tab,int row);
Pre2Setting overlay_key(Pre2Overlay *o,Pre2Settings *s,OverlayKey key);
/* Draw over a fresh copy of the frozen presented image, never over prior UI.
 * Pixel coordinates, clipping and integral font scale are resolution dependent. */
void overlay_render(const Pre2Overlay *o,const Pre2Settings *s,const OverlayPerf *perf,
                    uint32_t *pixels,int w,int h);
void overlay_performance(const OverlayPerf *perf,uint32_t *pixels,int w,int h);
/* The integral font scale for a drawable, and the performance line drawn at that scale
 * into a strip of overlay_performance_rows(scale) rows (the host composites the strip). */
int overlay_scale(int w,int h);
int overlay_performance_rows(int scale);
void overlay_performance_strip(const OverlayPerf *perf,uint32_t *pixels,int w,int rows,int scale);
#endif
