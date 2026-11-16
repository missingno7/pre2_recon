#include "pre2.h"
#include "view.h"
#include "pools.h"

Pre2View pre2_view = { .width_px=320, .height_px=176, .cols=20, .rows=11,
    .summary_rows=12, .activation_margin=2, .terrain_margin=32, .pickup_max_row=43,
    .child_retire_dy=300, .patrol_retire_dy=190, .ground_child_drop=176,
    .follow_center=10, .follow_start_left=4, .follow_start_right=16,
    .follow_stop_left=5, .follow_stop_right=15, .idle_look_left=2, .idle_look_right=17,
    .entry_pan_threshold=12, .entry_pan_tiles=10 };
Pre2RegionPolicy (*pre2_view_region_policy)(unsigned, Pre2RegionKind, unsigned, unsigned);

static void aperture(unsigned width)
{
    pre2_view.width_px=width; pre2_view.height_px=176;
    pre2_view.cols=width/16; pre2_view.rows=11; pre2_view.summary_rows=12;
    pre2_view.follow_center=pre2_view.cols/2;
    pre2_view.follow_start_left=4; pre2_view.follow_start_right=pre2_view.cols-4;
    pre2_view.follow_stop_left=5; pre2_view.follow_stop_right=pre2_view.cols-5;
    pre2_view.idle_look_left=2; pre2_view.idle_look_right=pre2_view.cols-3;
    pre2_view.entry_pan_threshold=pre2_view.cols/2+2; pre2_view.entry_pan_tiles=pre2_view.cols/2;
}
int pre2_view_configure(unsigned width)
{
    if (width && (width < 320 || width > 640 || width % 16)) return -1;
    pre2_view.requested_width=width;
    pre2_view.configured=width > 320;
    pre2_view.variant=0;
    pre2_view.region=PRE2_REGION_MAIN;
    aperture(320);
    return 0;
}
void pre2_view_refresh(void)
{
    if (!pre2_view.variant || !pre2_view_region_policy) return;
    Pre2RegionPolicy p=pre2_view_region_policy(DB(DS_CurrentLevel), pre2_view.region,
        pre2_view.requested_width, pre2_view.region==PRE2_REGION_MAIN ? DW(DS_CameraCol) : pre2_view.region_anchor_col);
    aperture(p.effective_width_px);
    pre2_view.camera_min_col=p.camera_min_col;
    pre2_view.camera_max_col=p.camera_max_col;
    /* World right stays authored even when the camera maximum moves left. */
    pre2_view.logical_right_px=p.world_right_column*16;
}
static unsigned main_door;
static int main_door_valid;
void pre2_view_begin_level(void)
{
    pre2_view.variant=pre2_view.configured && !DB(DS_byte_2879);
    pre2_view.region=PRE2_REGION_MAIN;
    main_door_valid=0;
    pre2_view.region_anchor_col=0;
    aperture(320);
    pre2_view_refresh();
}
void pre2_view_stage(void)
{
    if (pre2_view.variant) {
        for (unsigned n=2;n<pre2_render_count();n++)
            SET_DW(pre2_render_slot(n)+4,0xFFFF);
        /* Reframe the authored 320 stage around the player before its iris and
         * exit walk consume camera-relative coordinates. Configuration stays
         * latched; the next level chooses its region again at its boundary. */
        unsigned px=DW(DS_PlayerX), cx=DW(DS_CameraCol)*16;
        if (px>cx+312) SET_DW(DS_CameraCol,(px-160)/16);
    }
    pre2_view.variant=0;
    aperture(320);
}
/* The door the player left the main region through. A later free-follow trigger that
 * lands within DOOR_REACH tiles of it is that door's way back (the shipped return records
 * land 0..1 columns and 0..4 rows from their entry tile): the main region, at its full
 * width, is selected again. Anything else (another cave, a one-way passage, a locked
 * screen) keeps the conservative 320 arrival. */
#define DOOR_REACH 4
static int near_door(unsigned tile)
{
    int dc=(int)(tile&255)-(int)(main_door&255), dr=(int)(tile>>8)-(int)(main_door>>8);
    return main_door_valid && dc>=-DOOR_REACH && dc<=DOOR_REACH && dr>=-DOOR_REACH && dr<=DOOR_REACH;
}
unsigned pre2_view_cave(unsigned gate, unsigned target_col, unsigned source, unsigned destination)
{
    if (!pre2_view.variant) return target_col;
    int from_main=pre2_view.region==PRE2_REGION_MAIN;
    pre2_view.region=DB(DS_CurrentLevel)==5 ? PRE2_REGION_TOWER_LOWER :
        gate ? PRE2_REGION_CAVE_LOCKED :
        !from_main && near_door(destination) ? PRE2_REGION_MAIN : PRE2_REGION_CAVE_UNKNOWN;
    if (from_main) { main_door=source; main_door_valid=1; }
    else if (pre2_view.region==PRE2_REGION_MAIN) main_door_valid=0;
    pre2_view.region_anchor_col=target_col;
    /* Select the arrival region before the equality pan, rather than changing
     * its destination underneath ScrollViewRight's bound. */
    Pre2RegionPolicy p=pre2_view_region_policy(DB(DS_CurrentLevel), pre2_view.region,
                                               pre2_view.requested_width, target_col);
    aperture(p.effective_width_px);
    pre2_view.camera_min_col=p.camera_min_col;
    pre2_view.camera_max_col=p.camera_max_col;
    pre2_view.logical_right_px=p.world_right_column*16;
    if (target_col < p.camera_min_col) target_col=p.camera_min_col;
    if (target_col > p.camera_max_col) target_col=p.camera_max_col;
    return target_col;
}
int pre2_view_candidate(unsigned x, unsigned y)
{
    if (!pre2_view.variant) return 1;
    (void)y;
    Pre2RegionPolicy p=pre2_view_region_policy(DB(DS_CurrentLevel), pre2_view.region,
        pre2_view.requested_width, pre2_view.region==PRE2_REGION_MAIN ? DW(DS_CameraCol) : pre2_view.region_anchor_col);
    return x/16 >= p.first_column && x/16 < p.end_column && x/16 < p.world_right_column;
}
