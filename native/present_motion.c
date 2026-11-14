#include "presenter.h"
#include "level_regions.h"
#include <math.h>
#include <string.h>

static double clamp(double x, double lo, double hi) { return x < lo ? lo : x > hi ? hi : x; }
void presenter_interpolate(const FrameGameView *a, const FrameGameView *b, double t, PresenterPose *o)
{
    memset(o, 0, sizeof *o);
    t = clamp(t, 0, 1); o->alpha = t;
    o->view = t == 0 ? a : b;
    o->x = o->view->camera_x; o->y = o->view->camera_y;
    if (t == 0 || t == 1 || b->camera_cut || a->level_epoch != b->level_epoch || a->assets != b->assets) return;
    o->x = a->camera_x + (b->camera_x - a->camera_x) * t;
    o->y = a->camera_y + (b->camera_y - a->camera_y) * t;
    for (unsigned j = 0; j < b->sprite_count && j < FRAME_MAX_SPRITES; j++) {
        const FrameSprite *s = &b->sprites[j];
        if (s->screen_space) continue;
        for (unsigned i = 0; i < a->sprite_count && i < FRAME_MAX_SPRITES; i++) {
            const FrameSprite *old = &a->sprites[i];
            if (old->id != s->id || old->gen != s->gen || old->screen_space) continue;
            double dx = s->world_x - old->world_x, dy = s->world_y - old->world_y;
            if ((dx || dy) && dx*dx + dy*dy <= PRESENTER_TELEPORT_PX * PRESENTER_TELEPORT_PX) {
                EnhPosition *p = &o->positions[o->count++]; p->id = s->id;
                p->x = old->world_x + dx*t; p->y = old->world_y + dy*t;
            }
            break;
        }
    }
}
Pre2PresentWindow pre2_present_window(const FrameGameView *v, double x, double y, int w, int h)
{
    Pre2PresentWindow out;
    out.region=pre2_region_for_view(v);
    Pre2WorldRect b=out.region.bounds;
    /* An original pan may include void beside an authored band. At exactly
     * the unsmoothed 320x176 endpoint, parity wins over reframing that pan.
     * Wider windows still rest at the actual art edge below. */
    if (w==320 && h==176 && x==v->camera_x && y==v->camera_y &&
        (x<b.x0 || x+w>b.x1 || y<b.y0 || y+h>b.y1)) {
        out.x=x; out.y=y; out.visible=(Pre2WorldRect){v->camera_x,v->camera_x+320,v->camera_y,v->camera_y+176};
        out.region.proven_extent=0;
        return out;
    }
    int bw=b.x1-b.x0, bh=b.y1-b.y0;
    out.x=bw<w ? b.x0-(w-bw)/2.0 : clamp(x,b.x0,b.x1-w);
    out.y=bh<h ? b.y0-(h-bh)/2.0 : clamp(y,b.y0,b.y1-h);
    out.visible=(Pre2WorldRect){(int32_t)fmax(b.x0,floor(out.x)),
        (int32_t)fmin(b.x1,floor(out.x)+w),
        (int32_t)fmax(b.y0,floor(out.y)),(int32_t)fmin(b.y1,floor(out.y)+h)};
    return out;
}
int presenter_fixed_aperture(const FrameGameView *v, int width)
{
    Pre2AuthoredRegion r=pre2_region_for_view(v);
    return r.fixed_screen || r.bounds.x1-r.bounds.x0<width;
}
void presenter_camera(PresenterCamera *s, const PresenterPose *p, uint64_t t,
                      int w, double tau, double *x, double *y)
{
    const FrameGameView *v=p->view;
    const FrameSprite *player=NULL;
    for (unsigned i=0;i<v->sprite_count && i<FRAME_MAX_SPRITES;i++)
        if (v->sprites[i].slot==1 && !v->sprites[i].screen_space) { player=&v->sprites[i]; break; }
    Pre2AuthoredRegion region=pre2_region_for_view(v);
    int fixed=region.fixed_screen;
    double sw=v->view_w>320?v->view_w:320;
    double tx=p->x-(w-sw)/2.0, ty=p->y;
    /* Zero still resets the camera. A seeded tag retains region+1, separate
     * from the player ID: hashing those two identities could miss a reseed. */
    int region_seed=(int)region.id+1;
    uint32_t id=player?player->id:0;
    int seed=s->seeded!=region_seed || t<s->time || s->epoch!=v->level_epoch || s->player_id!=id ||
             (v->camera_cut && s->cut_frame!=v->sim_frame);
    if (seed) { s->x=tx; s->y=ty; }
    else if (player && !fixed) {
        double px=player->world_x, py=player->world_y;
        for (size_t i=0;i<p->count;i++) if (p->positions[i].id==player->id) {
            px=p->positions[i].x; py=p->positions[i].y; break;
        }
        double k=tau>0 ? -expm1(-(double)(t-s->time)/1e9/tau) : 1;
        s->x+=(px-w/2.0-s->x)*k;
        s->y+=(py-88.0-s->y)*k;
    } else { s->x=tx; s->y=ty; }
    if (v->level_flags&4) s->y=p->y;
    Pre2PresentWindow window=pre2_present_window(v,s->x,s->y,w,176);
    s->x=window.x; s->y=window.y;
    s->time=t; s->epoch=v->level_epoch; s->player_id=id; s->seeded=region_seed;
    if (v->camera_cut) s->cut_frame=v->sim_frame;
    *x=s->x; *y=s->y;
    /* Shake remains outside the camera integrator. */
}
