#include "enh_margin.h"
#include "enh_render.h"
#include "../c/pre2.h"
#include "../c/view.h"

enum { FLOATS=70, TERRAIN=16 };
static int enabled=1;

void enh_margin_enable(int on) { enabled=!!on; }

static int published_source(const FrameGameView *v, uint16_t source, unsigned first, unsigned n)
{
    /* Ownership, not a re-evaluation of the projector's earlier camera test.
     * Only slots actually present in this page's draw list suppress a source:
     * a live but sprite-culled projection is precisely a margin candidate. */
    for (unsigned j=0;j<v->sprite_count;j++) {
        unsigned slot=v->sprites[j].slot;
        if (slot>=first && slot<first+n && DW(DS_RenderSlots+slot*18+9)==source) return 1;
    }
    return 0;
}
static void append(FrameGameView *v, unsigned ordinal, uint16_t source)
{
    uint16_t fw=DW(source+4);
    if (fw==0xffff) return;
    /* UpdateTerrainEntities runs BEFORE projection. Only its sleeping
     * directional early return is static: speed accumulator +Eh=0,
     * nonnegative target speed +7, no contact/start flags C0h. Falling
     * type 8 and every moving/awakened record are intentionally excluded. */
    if ((DB(source+6)&15)==8 || DB(source+14) || (int8_t)DB(source+7)<0 || (DB(source+6)&0xc0)) return;
    if (published_source(v,source,91,7)) return;
    unsigned frame=fw&0x1fff;
    if (frame>=1024 || frame==309 || !v->assets->cels || !v->assets->cels[frame].blocks) return;
    int x=(int16_t)DW(source), y=(int16_t)(uint16_t)(DW(source+2)-2);
    unsigned at=frame*2;
    int width=DB(DS_FrameWidths+at), height=DB(DS_FrameWidths+at+1);
    uint8_t hx=DB(DS_FrameHotX+at);
    if (fw&0x8000) hx=(uint8_t)(width-hx);
    int sx=x-(int8_t)hx-v->camera_x;
    int sy=y+(int8_t)DB(DS_FrameHotY+at)+v->shake_rows-v->camera_y-height;
    /* A capacity-dropped central source must not acquire new centre pixels.
     * Keep only records with at least some coverage outside the old aperture. */
    if (sx>=0 && sx+width<=320 && sy>=0 && sy+height<=176) return;
    if (v->sprite_count==FRAME_MAX_SPRITES) return;
    FrameSprite s={0};
    s.slot=(uint16_t)(0xf000u+ordinal);
    /* Sources are authored, not dynamically allocated. LoadLevel creates a
     * generation; ResetLevelState restores the same sources and EnterLevelView
     * publishes a camera cut. No observer-frequency-dependent lifetime state. */
    s.gen=v->level_epoch; s.id=0x80000000u | ordinal;
    /* Ordinary slot generations are unbounded. Even after millions of reuses,
     * never collide with an existing publication's ID. */
    for (unsigned i=0;i<v->sprite_count;i++) if (v->sprites[i].id==s.id) return;
    s.frame=(uint16_t)frame; s.mirror=!!(fw&0x8000); s.mode=(fw&0x4000)?2:1;
    s.world_x=x; s.world_y=y; s.screen_x=sx; s.screen_y=sy; s.margin_only=1;
    v->sprites[v->sprite_count++]=s;
}
void enh_margin_capture(FrameGameView *v)
{
    if (!enabled || !v->valid || !v->world_valid || !v->assets || pre2_view.configured ||
        v->view_w>320 || v->level!=DB(DS_CurrentLevel)) return;
    /* Float/pickup sources freeze while omitted, but BuildEffectDrawList resumes
     * their sway on activation (59 nonzero Y handoffs in the scripted sweep).
     * They fail the unchanged-activation requirement and are NOT allowlisted.
     * Reserve their ordinal range for a separately justified future policy. */
    for (unsigned i=0;i<TERRAIN;i++) append(v,FLOATS+i,(uint16_t)(0x9107+i*15));
}
