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

#endif
