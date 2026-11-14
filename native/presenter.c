#include "presenter.h"
#include "level_regions.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* Bounded history: 32 CRT starts, >10 simulation views at the original cadence.
 * A stalled consumer drops old presentation records, never asks for game work. */
#define HISTORY 32
typedef struct Record { Pre2Frame f; uint64_t sim_time; } Record;
struct Presenter {
    PresenterOptions opt;
    Record records[HISTORY];
    unsigned head, count;
    uint64_t first_sim_time;
    uint32_t last_sim, last_epoch;
    int have_sim;
    PresenterCamera camera;
    uint8_t index[PRESENTER_MAX_W * PRESENTER_MAX_H];
};
static Record *record(Presenter *p, unsigned i) { return &p->records[(p->head + i) % HISTORY]; }
static double fraction(uint64_t t, uint64_t a, uint64_t b)
{
    return t <= a || b <= a ? 0 : t >= b ? 1 : (double)(t-a)/(double)(b-a);
}
static int expand(unsigned c) { return (int)((c << 2) | (c >> 4)); }
static uint32_t rgb(const uint8_t a[3], const uint8_t b[3], double t)
{
    unsigned c[3];
    for (int k=0;k<3;k++) c[k]=(unsigned)floor(expand(a[k]) + (expand(b[k])-expand(a[k]))*t + .5);
    return 0xff000000u | c[0]<<16 | c[1]<<8 | c[2];
}
PresenterOptions presenter_defaults(void)
{
    PresenterOptions o = { PRESENT_FAITHFUL, 0, 0, 0, .12, 16.0/9.0, ENH_BACK_BLACK, ENH_HUD_CENTRE };
    return o;
}
Presenter *presenter_create(const PresenterOptions *o)
{
    Presenter *p = calloc(1, sizeof *p);
    if (p) {
        p->opt = o ? *o : presenter_defaults();
        if (p->opt.smooth_camera) p->opt.interpolate = 1;
        if (!isfinite(p->opt.camera_tau) || p->opt.camera_tau < 0) p->opt.camera_tau = .12;
        if (!isfinite(p->opt.aspect) || p->opt.aspect < 0) p->opt.aspect = 16.0/9.0;
    }
    return p;
}
void presenter_reset(Presenter *p)
{
    if (!p) return;
    p->head = p->count = 0; p->have_sim = 0;
    memset(&p->camera, 0, sizeof p->camera);
}
void presenter_destroy(Presenter *p) { free(p); }
void presenter_push(Presenter *p, const Pre2Frame *f)
{
    if (!p || !f || !f->w || f->w > FRAME_MAX_W || !f->h || f->h > FRAME_MAX_H) return;
    if (p->count && f->t_ns <= record(p,p->count-1)->f.t_ns) return;
    if (p->count == HISTORY) { p->head = (p->head+1)%HISTORY; p->count--; }
    Record *r = record(p,p->count++); r->f = *f; r->sim_time = f->t_ns;
    if (f->game.valid) {
        if (!p->have_sim || p->last_sim != f->game.sim_frame || p->last_epoch != f->game.level_epoch) {
            p->first_sim_time = f->t_ns; p->last_sim = f->game.sim_frame;
            p->last_epoch = f->game.level_epoch; p->have_sim = 1;
        }
        r->sim_time = p->first_sim_time;
    } else p->have_sim = 0;
}
static int same_transition(const Pre2Frame *a, const Pre2Frame *b)
{
    return a && b && a->w == b->w && a->h == b->h && a->blank == b->blank &&
        a->pel_mask == b->pel_mask && a->transition.kind && a->transition.kind == b->transition.kind &&
        a->transition.phase == b->transition.phase && a->transition.steps == b->transition.steps &&
        b->transition.step >= a->transition.step &&
        a->game.level_epoch == b->game.level_epoch;
}
static int same_step(const Pre2Frame *a, const Pre2Frame *b)
{
    return same_transition(a,b) && !memcmp(&a->transition,&b->transition,sizeof a->transition) &&
        !memcmp(a->palette,b->palette,sizeof a->palette) && !memcmp(a->index,b->index,(size_t)a->w*a->h);
}
static void faithful(const Pre2Frame *f, const Pre2Frame *pal_b, double pal_t, PresenterImage *o)
{
    uint32_t pal[256];
    for (int i=0;i<256;i++) pal[i]=rgb(f->palette[i & f->pel_mask],
        (pal_b ? pal_b : f)->palette[i & f->pel_mask],pal_t);
    for (int y=0;y<o->h;y++) for (int x=0;x<o->w;x++) o->pixels[y*o->w+x]=0xff000000u;
    int ox=(o->w-f->w)/2, oy=(o->h-f->h)/2;
    for (int y=0;y<f->h;y++) for (int x=0;x<f->w;x++)
        o->pixels[(y+oy)*o->w+x+ox]=pal[f->index[y*f->w+x]];
}
/* Smooth masks operate ONLY on two already published images. A partially copied
 * page is a legitimate endpoint. No unrevealed source page is read or invented. */
static void mask_steps(const Pre2Frame *a, const Pre2Frame *b, double t, PresenterImage *o)
{
    int ox=(o->w-a->w)/2, oy=(o->h-a->h)/2;
    for (int y=0;y<a->h;y++) for (int x=0;x<a->w;x++) {
        double edge = a->transition.kind == 2 ? ((x < 160 ? 15-x%16 : x%16)+.5)/16.0 : (y%10+.5)/10.0;
        const Pre2Frame *f = t >= edge ? b : a;
        unsigned idx=f->index[y*f->w+x]&f->pel_mask;
        o->pixels[(y+oy)*o->w+x+ox]=rgb(f->palette[idx],f->palette[idx],0);
    }
}
/* Validate the whole overlapping image before translating a map frame. Markers,
 * text, ring repairs or a capture/CRTC phase mismatch invalidate this projection.
 * These cases keep the faithful indexed frame instead of smearing the overlay. */
static int scroll_steps(const Pre2Frame *a, const Pre2Frame *b, double t, PresenterImage *o)
{
    int dx=b->transition.cx-a->transition.cx, dy=b->transition.cy-a->transition.cy;
    if ((!dx && !dy) || abs(dx)>=a->w || abs(dy)>=a->h || memcmp(a->palette,b->palette,sizeof a->palette)) return -1;
    for (int y=0;y<a->h;y++) for (int x=0;x<a->w;x++) {
        int sx=x+dx, sy=y+dy;
        if (sx>=0 && sx<a->w && sy>=0 && sy<a->h && a->index[sy*a->w+sx]!=b->index[y*b->w+x]) return -1;
    }
    int ox=(o->w-a->w)/2, oy=(o->h-a->h)/2;
    for (int y=0;y<a->h;y++) for (int x=0;x<a->w;x++) {
        double sx=x+dx*t, sy=y+dy*t;
        int ix=(int)floor(sx), iy=(int)floor(sy);
        double fx=sx-ix, fy=sy-iy;
        double channels[3]={0};
        for (int yy=0;yy<2;yy++) for (int xx=0;xx<2;xx++) {
            int ax=ix+xx, ay=iy+yy; const Pre2Frame *f=a;
            if (ax<0 || ax>=a->w || ay<0 || ay>=a->h) { ax-=dx; ay-=dy; f=b; }
            if (ax<0) ax=0;
            if (ay<0) ay=0;
            if (ax>=f->w) ax=f->w-1;
            if (ay>=f->h) ay=f->h-1;
            const uint8_t *c=f->palette[f->index[ay*f->w+ax]&f->pel_mask];
            double k=(xx?fx:1-fx)*(yy?fy:1-fy);
            for (int ch=0;ch<3;ch++) channels[ch]+=expand(c[ch])*k;
        }
        o->pixels[(y+oy)*o->w+x+ox]=0xff000000u | (uint32_t)(channels[0]+.5)<<16 |
            (uint32_t)(channels[1]+.5)<<8 | (uint32_t)(channels[2]+.5);
    }
    return 0;
}
int presenter_render(Presenter *p, uint64_t bound, double aspect, PresenterImage *o)
{
    if (!p || !o || !o->pixels || !p->count) return -1;
    uint64_t t=bound;
    if (p->opt.interpolate || p->opt.smooth_transitions) t=bound>PRESENTER_DELAY_NS?bound-PRESENTER_DELAY_NS:0;
    if (t<record(p,0)->f.t_ns) return -1; /* never expose a frame ahead of the requested time */
    unsigned available=p->count;
    while (available && record(p,available-1)->f.t_ns>bound) available--;
    if (!available) return -1;
    unsigned at=0;
    while (at+1<available && record(p,at+1)->f.t_ns<=t) at++;
    Record *r=record(p,at); const Pre2Frame *f=&r->f;
    const Pre2Frame *step=f, *next=at+1<available?&record(p,at+1)->f:NULL;
    /* Duplicate CRT publications do not restart a transition's clock. The
     * first display of each distinct progress/image/palette is its endpoint.
     * Palette writes can expose intermediate DAC states within one fade pass;
     * those are endpoints too, rather than guessed whole-pass palettes. */
    if (p->opt.smooth_transitions && f->transition.kind) {
        unsigned begin=at, end=at+1;
        while (begin && same_step(f,&record(p,begin-1)->f)) begin--;
        while (end<available && same_step(f,&record(p,end)->f)) end++;
        step=&record(p,begin)->f;
        next=end<available?&record(p,end)->f:NULL;
    }
    double step_t=next?fraction(t,step->t_ns,next->t_ns):0;
    o->time=t; o->alpha=1; o->fallback=0; o->camera_x=f->game.camera_x; o->camera_y=f->game.camera_y;
    o->progress=f->transition.step; o->radius=f->transition.radius;
    o->w=f->w; o->h=f->h;
    if (p->opt.mode==PRESENT_WIDESCREEN) {
        if (p->opt.aspect>0) aspect=p->opt.aspect;
        if (!isfinite(aspect) || aspect<=0) aspect=16.0/9.0;
        double want=ceil(f->h*aspect/2)*2;
        if (want>PRESENTER_MAX_W) want=PRESENTER_MAX_W;
        if (want>o->w) o->w=(int)want;
        /* A wider simulated view (true widescreen) is never cropped: the image is at least
         * as wide as the view; the host letterboxes it. */
        if (f->game.valid && f->game.view_w>o->w) o->w=f->game.view_w;
    }
    if (o->capacity<(size_t)o->w*o->h) return -1;
    int pair=p->opt.smooth_transitions && same_transition(f,next);
    const Pre2Frame *pal_b=NULL; double pal_t=0;
    if (pair && f->transition.kind==4 && !memcmp(f->index,next->index,(size_t)f->w*f->h)) { pal_b=next; pal_t=step_t; }
    faithful(f,pal_b,pal_t,o);
    if (pair) {
        o->progress=f->transition.step+(next->transition.step-f->transition.step)*step_t;
        o->radius=f->transition.radius+(next->transition.radius-f->transition.radius)*step_t;
    }
    int ordinary=!f->transition.kind || f->transition.kind==4;
    int iris=p->opt.smooth_transitions && f->transition.kind==1 && pair && step_t>0;
    int world=f->game.valid && f->game.world_valid && f->game.assets && f->w==320 && f->h==200 && !f->blank;
    int enhanced=world && (p->opt.mode!=PRESENT_FAITHFUL || p->opt.interpolate || iris) && (ordinary || iris);
    if (!world) { p->camera.seeded=0; if (f->game.valid) o->fallback|=1; }
    if (enhanced) {
        PresenterPose pose;
        presenter_interpolate(&f->game,&f->game,1,&pose);
        if (p->opt.interpolate && ordinary) for (unsigned k=at+1;k<available;k++) {
            Record *b=record(p,k);
            if (!b->f.game.valid || !b->f.game.world_valid || b->f.transition.kind || b->f.game.level_epoch!=f->game.level_epoch) break;
            if (b->f.game.sim_frame==f->game.sim_frame) continue;
            presenter_interpolate(&f->game,&b->f.game,fraction(t,r->sim_time,b->sim_time),&pose); break;
        }
        o->alpha=pose.alpha;
        int rw=o->w;
        double sw=pose.view->view_w>320?pose.view->view_w:320;
        double cx=pose.x-(rw-sw)/2.0, cy=pose.y;
        if (p->opt.smooth_camera) presenter_camera(&p->camera,&pose,t,rw,p->opt.camera_tau,&cx,&cy);
        Pre2PresentWindow window=pre2_present_window(pose.view,cx,cy,rw,176);
        cx=window.x; cy=window.y;
        EnhImage im={0}; im.index=p->index; im.capacity=sizeof p->index; im.stride=rw;
        EnhOptions opt={p->opt.backdrop,p->opt.hud,pose.positions,pose.count};
        if (window.region.bounds.x1-window.region.bounds.x0<rw) opt.hud=ENH_HUD_CENTRE;
        if (enh_render(pose.view,pose.view->assets,f->palette,f->pel_mask,cx,cy,rw,200,&opt,&im)==0) {
            int ox=(o->w-rw)/2;
            uint32_t pal[256];
            for (int i=0;i<256;i++) pal[i]=rgb(f->palette[i&f->pel_mask],(pal_b?pal_b:f)->palette[i&f->pel_mask],pal_t);
            /* Iris centre is frozen in world space from the source view. Shake
             * displaces the world after smoothing, just as for sprites/tiles. */
            double ix=f->game.camera_x+f->transition.cx-cx;
            double iy=f->game.camera_y+f->transition.cy-cy+pose.view->shake_rows;
            for (int y=0;y<200;y++) for (int x=0;x<rw;x++) {
                uint32_t c=pal[p->index[y*rw+x]];
                if (y<176 && (floor(cx)+x<window.visible.x0 || floor(cx)+x>=window.visible.x1 ||
                              floor(cy)+y<window.visible.y0 || floor(cy)+y>=window.visible.y1)) c=0xff000000u;
                if (iris && y<176 && (x-ix)*(x-ix)+(y-iy)*(y-iy)>o->radius*o->radius) c=0xff000000u;
                o->pixels[y*o->w+x+ox]=c;
            }
            o->camera_x=cx; o->camera_y=cy;
        } else o->fallback|=1;
    }
    /* Exact distinct-step endpoints use the original iris raster, including its unusual
     * span arithmetic and sprites drawn after blackout. Circle interiors are a
     * presentation choice; an exact endpoint is not replaced by a guessed mask. */
    if (f->transition.kind==1 && (!iris || !world)) o->fallback|=2;
    if (pair && step_t>0 && (f->transition.kind==2 || f->transition.kind==3)) mask_steps(f,next,step_t,o);
    if (pair && step_t>0 && f->transition.kind==5 && scroll_steps(f,next,step_t,o)) o->fallback|=4;
    return 0;
}
