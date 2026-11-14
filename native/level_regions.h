#ifndef PRE2_LEVEL_REGIONS_H
#define PRE2_LEVEL_REGIONS_H

#include <stdint.h>

/* Geometry constraints for authored rooms. Bounds are camera columns and are
 * deliberately distinct from the 256-column backing-map storage extent. */
typedef enum Pre2RegionKind {
    PRE2_REGION_MAIN,
    PRE2_REGION_CAVE_LOCKED,
    PRE2_REGION_CAVE_UNKNOWN,
    PRE2_REGION_TOWER_LOWER
} Pre2RegionKind;

typedef struct Pre2RegionPolicy {
    uint16_t effective_width_px;
    uint16_t first_column, end_column;
    uint16_t world_right_column; /* exclusive authored world edge, not map storage */
    uint16_t camera_min_col, camera_max_col;
    uint8_t fixed_aperture, graphics_columns;
} Pre2RegionPolicy;

/* `camera_col` is the mapped destination camera column for a cave arrival.
 * Requested widths are pixel widths and must be a multiple of 16. */
Pre2RegionPolicy pre2_region_policy(unsigned level, Pre2RegionKind kind,
                                    unsigned requested_width_px,
                                    unsigned camera_col);
/* Level-lifetime backing policy for immutable asset snapshots; room-local
 * aperture/camera bounds are selected separately by pre2_region_policy. */
Pre2RegionPolicy pre2_region_backing_policy(unsigned level);

/* Presentation rectangles are half-open WORLD PIXELS. They do not change the
 * simulation policy above. Evidence names original routines/decoded records;
 * proven_extent=0 explicitly identifies an arrival whose outer walls are still
 * unknown (its original aperture is the only established safe rectangle). */
typedef struct Pre2WorldRect { int32_t x0, x1, y0, y1; } Pre2WorldRect;
typedef struct Pre2AuthoredRegion {
    Pre2WorldRect bounds;
    uint16_t id;
    uint8_t fixed_screen, proven_extent;
    const char *evidence;
} Pre2AuthoredRegion;
typedef struct Pre2CaveRegion {
    uint8_t level, gate;
    uint16_t source, camera, destination;
    Pre2AuthoredRegion region; /* with proven_extent=0, bounds are ONLY the arrival aperture */
} Pre2CaveRegion;
struct FrameGameView;
Pre2AuthoredRegion pre2_region_for_view(const struct FrameGameView *view);
const Pre2CaveRegion *pre2_region_caves(unsigned *count);

/* Proximity-trigger staging banks are map art, not extra playable rows.
 * Phase is reconstructed from the immutable map: trigger +8 is a per-record
 * countdown, not a level-wide earthquake state. -1 means no matching phase. */
typedef struct Pre2RevealRegion {
    uint16_t source, trigger;
    uint8_t columns, rows, steps;
    const char *evidence;
} Pre2RevealRegion;
const Pre2RevealRegion *pre2_region_reveals(unsigned *count);
int pre2_region_reveal_phase(const struct FrameGameView *view, unsigned index);
/* Tile-aligned silhouette of one reveal. The moving copy grows into the main
 * view; its tail and the original rows restored behind it are both forbidden.
 * Their union is invariant, but their boundary changes on every reveal step.
 * phase=-1 is conservative: no visible-copy claim, whole bank forbidden. */
typedef struct Pre2RevealVisibility {
    int phase;
    Pre2WorldRect visible_copy, never_copy, never_restored;
} Pre2RevealVisibility;
Pre2RevealVisibility pre2_region_reveal_visibility(const struct FrameGameView *view, unsigned index);
/* Original below-ground bank, half-open. UpdateProximityTriggers restores its
 * old bottom rows while moving a copy upward, so it stays hidden in ALL phases. */
Pre2WorldRect pre2_region_staging(const struct FrameGameView *view, unsigned index);
/* Up to seven forbidden column/row rectangles in the backing map: the four
 * exterior ranges plus the three restored earthquake staging banks. */
unsigned pre2_region_never_visible(const struct FrameGameView *view, Pre2WorldRect out[7]);

/* Camera plus the visible world intersection. A narrow region is centred;
 * otherwise the window rests at its edge. An original 320 endpoint panning into
 * void retains its aperture for parity. Called by both smooth and unsmoothed
 * presentation. Height is playfield height, excluding the HUD. */
typedef struct Pre2PresentWindow {
    double x, y;
    Pre2WorldRect visible;
    Pre2AuthoredRegion region;
} Pre2PresentWindow;
Pre2PresentWindow pre2_present_window(const struct FrameGameView *view,
                                     double x, double y, int width, int height);

#endif
