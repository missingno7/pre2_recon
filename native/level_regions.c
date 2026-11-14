#include "level_regions.h"

typedef struct LevelRegion {
    uint16_t authored_right_camera;
    uint16_t first_column, end_column;
    uint8_t fixed_aperture, graphics_columns;
} LevelRegion;

/* LevelXLimit values and restricted geometry are decoded from the shipped
 * level headers and documented in build/audit/view_geometry.md. They are
 * camera limits, not claims that the complete backing map is playable. */
static const LevelRegion levels[16] = {
    {236,0,256,0,0}, {124,0,256,0,0}, {236,0,256,0,0}, {140,0,256,0,0},
    {236,0,256,0,0}, { 46,0,256,1,0}, {236,0,256,0,0}, {236,0,256,0,0},
    {236,0,256,0,0}, {  0,0, 20,1,1}, {140,0,256,0,0}, {140,0,256,0,0},
    {140,0,256,0,0}, {236,0,256,0,0}, {  0,0, 20,1,0}, {236,0,256,0,0}
};

Pre2RegionPolicy pre2_region_backing_policy(unsigned level)
{
    if (level >= 16) level = 0;
    const LevelRegion *r = &levels[level];
    Pre2RegionPolicy p = { 320, r->first_column,
                           r->end_column, (uint16_t)(r->authored_right_camera + 20),
                           0, r->authored_right_camera,
                           r->fixed_aperture, r->graphics_columns };
    return p;
}

Pre2RegionPolicy pre2_region_policy(unsigned level, Pre2RegionKind kind,
                                    unsigned requested_width_px,
                                    unsigned camera_col)
{
    if (level >= 16) level = 0;
    if (requested_width_px < 320 || requested_width_px > 640 || (requested_width_px & 15))
        requested_width_px = 320;
    const LevelRegion *r = &levels[level];
    Pre2RegionPolicy p=pre2_region_backing_policy(level);
    p.effective_width_px=(uint16_t)requested_width_px;

    /* The tower's columns 0..19 are the climbing room. The lower encounter
     * starts at camera column 35 and retains its authored moving 320 aperture. */
    if (level == 5 && kind == PRE2_REGION_MAIN) {
        p.effective_width_px = 320; p.fixed_aperture = 1;
        p.first_column = 0; p.end_column = 256;
        p.camera_min_col = camera_col < 35 ? 0 : 35;
        p.camera_max_col = camera_col < 35 ? 0 : 46;
        p.world_right_column = camera_col < 35 ? 20 : 66;
        if (camera_col < 35) p.end_column = 20;
        return p;
    }
    if (kind == PRE2_REGION_TOWER_LOWER && level == 5) {
        p.effective_width_px = 320; p.fixed_aperture = 1;
        p.first_column = 35; p.end_column = 66;
        p.camera_min_col = 35; p.camera_max_col = 46; p.world_right_column = 66;
        return p;
    }
    /* A single authored band wins over a generic cave descriptor too. In
     * particular no arrival target can expose the gorilla graphics bank or
     * move level F sideways out of its ending/forced-scroll band. */
    if (r->fixed_aperture && r->end_column-r->first_column==20) {
        p.effective_width_px=320;
        return p;
    }

    /* A nonzero cave gate locks the original two-axis scroll. Until an
     * unannotated arrival is mapped, keep its aperture at 320 but preserve
     * the caller's original follow limits. */
    if (kind == PRE2_REGION_CAVE_UNKNOWN) {
        p.effective_width_px = 320; p.fixed_aperture = 1;
        /* The record proves its original 20-column arrival aperture even when
         * it lies beyond the main header's conditional camera threshold. Do
         * not clamp a canonical column-236 arrival to the main world's edge. */
        if (camera_col > 236) camera_col=236;
        if (p.camera_max_col < camera_col) p.camera_max_col=(uint16_t)camera_col;
        if (p.world_right_column < camera_col+20) p.world_right_column=(uint16_t)(camera_col+20);
        return p;
    }
    if (kind == PRE2_REGION_CAVE_LOCKED) {
        unsigned left = camera_col>236 ? 236 : camera_col;
        p.effective_width_px = 320; p.fixed_aperture = 1;
        p.first_column = (uint16_t)left;
        p.end_column = (uint16_t)(left + 20);
        p.world_right_column = (uint16_t)(left + 20);
        p.camera_min_col = p.camera_max_col = (uint16_t)left;
        return p;
    }

    if (r->fixed_aperture) p.effective_width_px = 320;
    /* Keep the same authored world-right limit when the aperture grows. */
    unsigned cols = p.effective_width_px / 16;
    unsigned shrink = cols > 20 ? cols - 20 : 0;
    p.camera_max_col = r->authored_right_camera > shrink
                     ? (uint16_t)(r->authored_right_camera - shrink) : 0;
    return p;
}
