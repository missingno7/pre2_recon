#include "level_regions.h"
#include "frame.h"
#include "enh_render.h"

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

/* The presentation table uses the decoded headers, not backing-map width. In
 * particular LEVEL2 is 108, LEVEL4 is 236 and LEVELD is 236: the earlier
 * simulation table's 124/140/140 values are not their shipped headers.
 * LoadLevel/LoadLevelTileSet and ScrollViewDown establish the vertical extent.
 * BossMinX/MaxX constrain boss motion, not the camera; they do not cut arenas.
 */
static const uint16_t presentation_right[16] = {
    256,128,256,256,256,66,256,256,256,20,160,160,256,256,20,256
};
static const uint16_t presentation_rows[16] = {
    49,104,49,45,128,128,128,86,110,12,24,51,51,38,173,84
};
#define CAVE(l,g,s,c,d,n) {l,g,s,c,d,{{((c)&255)*16,(((c)&255)+20)*16,((c)>>8)*16,((c)>>8)*16+176},n,g,g, \
    "TryCaveTrigger / shipped cave record: " #l ": " #s " -> " #c " -> " #d}}
static const Pre2CaveRegion cave_regions[] = {
    CAVE(0x00,0,0x2725,0x2440,0x2e4c,16),
    CAVE(0x00,0,0x07e5,0x19dc,0x20e4,17),
    CAVE(0x00,1,0x21e4,0x00dc,0x07e5,18),
    CAVE(0x00,0,0x0836,0x2672,0x2c7e,19),
    CAVE(0x00,0,0x2e7e,0x0032,0x0636,20),
    CAVE(0x00,0,0x076e,0x1ec7,0x22d1,21),
    CAVE(0x00,1,0x24d1,0x0065,0x056e,22),
    CAVE(0x00,0,0x220f,0x1bdb,0x1fe4,23),
    CAVE(0x01,0,0x0526,0x5767,0x5b6f,24),
    CAVE(0x01,0,0x0875,0x5965,0x5e6f,25),
    CAVE(0x01,0,0x3537,0x4e19,0x5223,26),
    CAVE(0x01,0,0x0874,0x5767,0x5e6f,27),
    CAVE(0x01,1,0x5f6f,0x006b,0x0874,28),
    CAVE(0x01,0,0x253c,0x3a01,0x410c,29),
    CAVE(0x01,0,0x1533,0x0200,0x0703,30),
    CAVE(0x02,0,0x1f0e,0x0257,0x045e,31),
    CAVE(0x02,0,0x2a38,0x2677,0x2e7f,32),
    CAVE(0x04,0,0x6b12,0x651f,0x6c27,33),
    CAVE(0x04,0,0x7bf0,0x00e7,0x07f0,34),
    CAVE(0x04,0,0x5adb,0x42d2,0x43db,35),
    CAVE(0x05,0,0x7e0f,0x7223,0x7925,36),
    CAVE(0x06,0,0x2725,0x2440,0x2e4c,37),
    CAVE(0x06,0,0x07e5,0x19dc,0x20e4,38),
    CAVE(0x06,1,0x21e4,0x00dc,0x07e5,39),
    CAVE(0x06,0,0x0836,0x2672,0x2c7e,40),
    CAVE(0x06,0,0x2e7e,0x0032,0x0636,41),
    CAVE(0x06,0,0x076e,0x1ec7,0x22d1,42),
    CAVE(0x06,1,0x24d1,0x0065,0x056e,43),
    CAVE(0x06,0,0x220f,0x1bdb,0x1fe4,44),
    CAVE(0x07,0,0x2709,0x4169,0x456f,45),
    CAVE(0x07,0,0x456f,0x1a04,0x1d07,46),
    CAVE(0x07,0,0x0e09,0x2b74,0x3178,47),
    CAVE(0x07,0,0x3477,0x030a,0x0813,48),
    CAVE(0x08,0,0x55cd,0x5cec,0x63f2,49),
    CAVE(0x08,0,0x07f5,0x0310,0x0918,50),
    CAVE(0x08,0,0x2c59,0x5ed0,0x61d9,51),
    CAVE(0x08,0,0x55cc,0x5eec,0x62f2,52),
    CAVE(0x0d,0,0x1305,0x09c1,0x12cc,53),
    CAVE(0x0d,0,0x2433,0x0ac2,0x11cb,54),
    CAVE(0x0d,0,0x2419,0x0a7f,0x1189,55),
    CAVE(0x0d,0,0x2400,0x0914,0x121e,56),
    CAVE(0x0d,1,0x1389,0x1b19,0x2319,57),
    CAVE(0x0d,1,0x131e,0x1b00,0x2201,58),
    CAVE(0x0d,1,0x13cc,0x1b33,0x2234,59),
};
#undef CAVE
const Pre2CaveRegion *pre2_region_caves(unsigned *count)
{
    if (count) *count = sizeof cave_regions / sizeof *cave_regions;
    return cave_regions;
}
static const Pre2RevealRegion reveals[] = {
    {0x133a,0x1340,4,6,4,"LEVELE proximity record 12; UpdateProximityTriggers +8 countdown"},
    {0x132c,0x1332,4,6,4,"LEVELE proximity record 13; UpdateProximityTriggers +8 countdown"},
    {0x13b0,0x0fb9,4,8,6,"LEVELE proximity record 14; UpdateProximityTriggers +8 countdown"}
};
const Pre2RevealRegion *pre2_region_reveals(unsigned *count)
{
    if (count) *count=sizeof reveals/sizeof *reveals;
    return reveals;
}
int pre2_region_reveal_phase(const FrameGameView *v, unsigned index)
{
    if (v->level!=13 || !v->map || !v->assets || index>=sizeof reveals/sizeof *reveals) return -1;
    const Pre2RevealRegion *r=&reveals[index];
    unsigned col=r->source&255, row=r->source>>8;
    if (v->assets->rows<row+r->rows) return -1;
    for (unsigned phase=0;phase<=r->steps;phase++) {
        int equal=1;
        for (unsigned y=0;y<r->rows;y++) for (unsigned x=0;x<r->columns;x++)
            if (v->map->rows[row-phase+y][col+x]!=v->assets->map[(row+y)*256+col+x]) equal=0;
        if (equal) return (int)phase;
    }
    return -1;
}
Pre2WorldRect pre2_region_staging(const FrameGameView *v, unsigned index)
{
    Pre2WorldRect empty={0,0,0,0};
    if (v->level!=13 || index>=sizeof reveals/sizeof *reveals) return empty;
    const Pre2RevealRegion *r=&reveals[index];
    int col=r->source&255, row=r->source>>8;
    Pre2WorldRect out={col*16,(col+r->columns)*16,(row+r->rows-r->steps)*16,(row+r->rows)*16};
    return out;
}
Pre2AuthoredRegion pre2_region_for_view(const FrameGameView *v)
{
    unsigned level = v->level < 16 ? v->level : 0;
    Pre2AuthoredRegion r = {{0,presentation_right[level]*16,0,presentation_rows[level]*16},
        (uint16_t)level,0,1,"LoadLevel header CameraLeftLimit; ScrollViewDown MapRows-11"};
    if (v->assets) r.bounds.y1=v->assets->rows*16;
    if (level == 5) {
        if (v->level_flags & 2) {
            r.bounds.x1=320;
            r.evidence="LEVEL6 flags 06h; tower columns 0..19; horizontal follow disabled";
        } else {
            /* The arrival row 114 is not a room-top clamp. Its band has no
             * other art above the encounter; retain the real MapRows extent.
             * The cleared horizontal-lock flag identifies it even if the
             * original camera pans left of its initial column 35. */
            r.bounds=(Pre2WorldRect){35*16,66*16,0,128*16};
            r.id=80;
            r.evidence="TryCaveTrigger clears flags 02h/04h; 7E0F->7223->7925; band columns 35..65; MapRows 128";
        }
        return r;
    }
    if (level == 9) {
        r.bounds.x1=320;
        r.evidence="DrawLevel9Panel copies graphics columns 20..49 into playable columns 12..17";
        return r;
    }
    if (level == 14) {
        r.bounds.x1=320;
        r.evidence="LEVELF CameraLeftLimit=0; 20-column vertical band; ScrollViewDown";
        return r;
    }
    if (level == 13 && !v->room) {
        /* All three authored blocks finish at row 21: 19+6-4 and
         * 19+8-6. Below that are their hidden tails, then the bonus rooms
         * starting at row 27. This is a world boundary, not a 149px camera
         * cap. The window helper separately preserves original 320 overshoots. */
        r.bounds.y1=21*16;
        r.evidence="LEVELE reveal records 12..14 final base row 21; bonus rooms start row 27";
    }
    for (unsigned i=0;i<sizeof cave_regions/sizeof *cave_regions;i++) {
        const Pre2CaveRegion *c=&cave_regions[i];
        if (c->level != level || !!c->gate != !!v->room) continue;
        if (v->camera_x == (c->camera&255)*16 && v->camera_y == (c->camera>>8)*16)
            return c->region;
    }
    if (v->room) {
        r.bounds=(Pre2WorldRect){v->camera_x,v->camera_x+320,v->camera_y,v->camera_y+176};
        r.id=81; r.fixed_screen=1; r.proven_extent=0;
        r.evidence="Unmapped full follow lock: only published aperture established";
    } else if (v->level_flags&2) {
        r.bounds.x0=v->camera_x; r.bounds.x1=v->camera_x+320;
        r.id=83; r.proven_extent=0;
        r.evidence="UpdateCameraScroll mask 02h locks X only; retain vertical region extent";
    } else {
        /* Preserve the original's conditional 236 fallback. A published camera
         * beyond the header cannot be moved back into another room. Its outer
         * walls remain unresolved, so only that published aperture is safe. */
        int sw=v->view_w>320?v->view_w:320;
        int header=(v->camera_limit_column+20)*16;
        if (v->camera_x+sw <= header && header<r.bounds.x1) r.bounds.x1=header;
        if (v->camera_x+sw > r.bounds.x1) {
            r.bounds=(Pre2WorldRect){v->camera_x,v->camera_x+sw,v->camera_y,v->camera_y+176};
            r.id=82; r.proven_extent=0;
            r.evidence="ScrollViewRight conditional 236 fallback; outer room extent unresolved";
        }
    }
    return r;
}
unsigned pre2_region_never_visible(const FrameGameView *v, Pre2WorldRect out[7])
{
    Pre2WorldRect b=pre2_region_for_view(v).bounds;
    int bottom=(v->assets?v->assets->rows:presentation_rows[v->level<16?v->level:0])*16;
    unsigned n=0;
    if (b.x0>0) out[n++]=(Pre2WorldRect){0,b.x0,0,bottom};
    if (b.x1<4096) out[n++]=(Pre2WorldRect){b.x1,4096,0,bottom};
    if (b.y0>0) out[n++]=(Pre2WorldRect){b.x0,b.x1,0,b.y0};
    if (b.y1<bottom) out[n++]=(Pre2WorldRect){b.x0,b.x1,b.y1,bottom};
    if (v->level==13 && !v->room) for (unsigned i=0;i<sizeof reveals/sizeof *reveals;i++) {
        Pre2WorldRect tail=pre2_region_staging(v,i);
        if (tail.y1>tail.y0) out[n++]=tail;
    }
    return n;
}
