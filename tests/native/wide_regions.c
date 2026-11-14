#include "../../native/level_regions.h"
#include <assert.h>

int main(void)
{
    Pre2RegionPolicy p;
    p=pre2_region_backing_policy(5);
    assert(p.end_column==256); /* lower encounter is not the tower's asset mask */
    p = pre2_region_policy(0, PRE2_REGION_MAIN, 640, 0);
    assert(p.effective_width_px == 640 && p.camera_max_col == 216);
    assert(p.world_right_column == 256);

    p = pre2_region_policy(5, PRE2_REGION_MAIN, 640, 0);
    assert(p.effective_width_px == 320 && p.first_column == 0 && p.end_column == 20);
    assert(p.camera_max_col == 0 && p.world_right_column == 20);
    p = pre2_region_policy(5, PRE2_REGION_TOWER_LOWER, 640, 35);
    assert(p.effective_width_px == 320 && p.first_column == 35 && p.end_column == 66);
    assert(p.camera_min_col == 35 && p.camera_max_col == 46 && p.world_right_column == 66);

    p = pre2_region_policy(9, PRE2_REGION_MAIN, 640, 0);
    assert(p.effective_width_px == 320 && p.end_column == 20 && p.graphics_columns);
    p = pre2_region_policy(14, PRE2_REGION_MAIN, 480, 0);
    assert(p.effective_width_px == 320 && p.end_column == 20);
    p=pre2_region_policy(9,PRE2_REGION_CAVE_UNKNOWN,640,236);
    assert(p.camera_max_col==0 && p.end_column==20 && p.world_right_column==20);
    p=pre2_region_policy(14,PRE2_REGION_CAVE_LOCKED,640,236);
    assert(p.camera_max_col==0 && p.end_column==20 && p.world_right_column==20);

    p = pre2_region_policy(0, PRE2_REGION_CAVE_LOCKED, 640, 103);
    assert(p.effective_width_px == 320 && p.camera_min_col == 103 && p.camera_max_col == 103);
    assert(p.first_column == 103 && p.end_column == 123);
    p = pre2_region_policy(0, PRE2_REGION_CAVE_UNKNOWN, 640, 103);
    assert(p.effective_width_px == 320 && p.camera_max_col == 236);
    p = pre2_region_policy(1, PRE2_REGION_CAVE_UNKNOWN, 640, 236);
    assert(p.effective_width_px==320 && p.camera_max_col==236 && p.world_right_column==256);
    return 0;
}
