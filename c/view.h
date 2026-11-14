/* Immutable run choice; effective aperture follows explicit authored regions.
 * This is variant configuration, never part of the original DATA image. */
#ifndef PRE2_VIEW_H
#define PRE2_VIEW_H
#include "../native/level_regions.h"
typedef struct Pre2View {
    unsigned requested_width, width_px, height_px, cols, rows, summary_rows;
    unsigned activation_margin, terrain_margin, pickup_max_row;
    unsigned child_retire_dy, patrol_retire_dy, ground_child_drop;
    unsigned follow_center, follow_start_left, follow_start_right;
    unsigned follow_stop_left, follow_stop_right, idle_look_left, idle_look_right;
    unsigned entry_pan_threshold, entry_pan_tiles;
    unsigned camera_min_col, camera_max_col, logical_right_px;
    unsigned region_anchor_col;
    int variant, configured;
    Pre2RegionKind region;
} Pre2View;
extern Pre2View pre2_view;
/* Native supplies the single authored region table; the differential DLL has no
 * native dependency and always starts with the original geometry. */
extern Pre2RegionPolicy (*pre2_view_region_policy)(unsigned, Pre2RegionKind, unsigned, unsigned);
int pre2_view_configure(unsigned width);
void pre2_view_begin_level(void);
void pre2_view_refresh(void);
void pre2_view_stage(void);
unsigned pre2_view_cave(unsigned gate, unsigned target_col);
int pre2_view_candidate(unsigned x, unsigned y);
#endif
