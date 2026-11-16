#include "presenter.h"
#include "level_regions.h"
#include "enh_screens.h"
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
    uint8_t next_index[PRESENTER_MAX_W * PRESENTER_MAX_H];
    FrameScreenView pan;   /* CARTE pan endpoint with capped reveal */
    /* The status strip as last published outside an iris. The original iris blacks its
     * spans out of the 320 strip itself, which a wide layout cannot map back. */
    FrameGameView hud_view, curtain_view, leaving_view;
    EnhSurface curtain_surface;
    uint16_t hud_count; uint8_t hud_frame, hud_valid; uint32_t hud_epoch;
    FrameHudElement hud_elements[FRAME_MAX_HUD];
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
    PresenterOptions o = { PRESENT_FAITHFUL, 0, 0, 0, .12, 16.0/9.0, ENH_BACK_BLACK, ENH_HUD_ANCHORED };
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
/* Retained history owns a reference to each record's native screen assets
 * (enh_screens.h): capture may replace or reset them while a record is still
 * needed for interpolation, fades or transition endpoints. */
static void release_all(Presenter *p)
{
    for (unsigned i = 0; i < p->count; i++) enh_screen_view_release(&record(p,i)->f.screen_view);
}
void presenter_reset(Presenter *p)
{
    if (!p) return;
    release_all(p);
    p->head = p->count = 0; p->have_sim = 0; p->hud_valid = 0;
    memset(&p->camera, 0, sizeof p->camera);
}
void presenter_destroy(Presenter *p) { if (p) release_all(p); free(p); }
void presenter_push(Presenter *p, const Pre2Frame *f)
{
    if (!p || !f || !f->w || f->w > FRAME_MAX_W || !f->h || f->h > FRAME_MAX_H) return;
    if (p->count && f->t_ns <= record(p,p->count-1)->f.t_ns) return;
    if (p->count == HISTORY) {
        enh_screen_view_release(&record(p,0)->f.screen_view);
        p->head = (p->head+1)%HISTORY; p->count--;
    }
    Record *r = record(p,p->count++); r->f = *f; r->sim_time = f->t_ns;
    enh_screen_view_retain(&r->f.screen_view);
    if (f->game.valid && f->game.world_valid && f->transition.kind != 1 && f->game.hud_count <= FRAME_MAX_HUD) {
        p->hud_count = f->game.hud_count; p->hud_frame = f->game.hud_frame;
        p->hud_epoch = f->game.level_epoch; p->hud_valid = 1;
        memcpy(p->hud_elements, f->game.hud_elements, f->game.hud_count * sizeof p->hud_elements[0]);
    }
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
        (!(a->game.surface || b->game.surface) || a->game.surface == b->game.surface) &&
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
/* Native screens (docs/native-screens-wide.md): every non-gameplay publication with a
 * valid screen view is drawn by enh_screens_render at the output width; the faithful
 * image is not an input. A CARTE pan between two compatible publications samples the
 * interpolated published pan over the newer surface, with each channel's reveal
 * capped to the older endpoint so an incomplete plane pass is never promoted early.
 * High-resolution pictures keep their own size and are centred. */
static int native_screen(Presenter *p, const Pre2Frame *f, const Pre2Frame *next, double step_t,
                         int smooth, const Pre2Frame *pal_b, double pal_t, PresenterImage *o)
{
    const FrameScreenView *v=&f->screen_view;
    if (!v->valid) return -1;
    const FrameScreenView *use=v; double px=v->pan_x, py=v->pan_y;
    const FrameScreenView *n=next?&next->screen_view:NULL;
    if (smooth && n && step_t>0 && n->valid && v->kind==FRAME_SCREEN_WORLD_MAP && n->kind==v->kind &&
        n->epoch==v->epoch && n->master==v->master && next->w==f->w && next->h==f->h &&
        !memcmp(next->palette,f->palette,sizeof f->palette)) {
        p->pan=*n;
        for (int k=0;k<4;k++) if (v->revealed[k]<p->pan.revealed[k]) p->pan.revealed[k]=v->revealed[k];
        use=&p->pan; px=v->pan_x+(n->pan_x-v->pan_x)*step_t; py=v->pan_y+(n->pan_y-v->pan_y)*step_t;
    }
    unsigned rw=f->h>200?f->w:(unsigned)o->w, rh=f->h;
    if (rw>(unsigned)o->w || rh>(unsigned)o->h) return -1;
    EnhImage im={0}; im.index=p->index; im.capacity=sizeof p->index; im.stride=rw;
    if (enh_screens_render(use,f->palette,f->pel_mask,px,py,rw,rh,&im)) return -1;
    uint32_t pal[256];
    for (int i=0;i<256;i++) pal[i]=rgb(f->palette[i&f->pel_mask],(pal_b?pal_b:f)->palette[i&f->pel_mask],pal_t);
    for (int y=0;y<o->h;y++) for (int x=0;x<o->w;x++) o->pixels[y*o->w+x]=0xff000000u;
    int ox=(o->w-(int)rw)/2, oy=(o->h-(int)rh)/2;
    for (unsigned y=0;y<rh;y++) for (unsigned x=0;x<rw;x++)
        o->pixels[(y+oy)*o->w+x+ox]=pal[p->index[y*rw+x]];
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
    if (p->opt.mode != PRESENT_FAITHFUL) o->fallback |= 8;
    if (pair) {
        o->progress=f->transition.step+(next->transition.step-f->transition.step)*step_t;
        o->radius=f->transition.radius+(next->transition.radius-f->transition.radius)*step_t;
    }
    int ordinary=!f->transition.kind || f->transition.kind==4;
    int iris=p->opt.smooth_transitions && f->transition.kind==1 && pair && step_t>0;
    int gameplay_transition=f->transition.kind>=1 && f->transition.kind<=3;
    int world=f->game.valid && (f->game.world_valid || (f->game.surface && gameplay_transition)) && f->game.assets && f->w==320 && f->h==200 && !f->blank;
    int enhanced=world && (p->opt.mode!=PRESENT_FAITHFUL || p->opt.interpolate || iris) && (ordinary || gameplay_transition);
    if (!world) { p->camera.seeded=0; if (f->game.valid) o->fallback|=1; }
    int screen=0;
    if (!enhanced && (p->opt.mode!=PRESENT_FAITHFUL || (pair && f->transition.kind==5)) &&
        native_screen(p,f,next,step_t,pair,pal_b,pal_t,o)==0) { screen=1; o->fallback&=~9u; }
    if (enhanced) {
        PresenterPose pose;
        presenter_interpolate(&f->game,&f->game,1,&pose);
        if (p->opt.interpolate && ordinary) for (unsigned k=at+1;k<available;k++) {
            Record *b=record(p,k);
            if (!b->f.game.valid || !b->f.game.world_valid || b->f.transition.kind || b->f.game.level_epoch!=f->game.level_epoch) break;
            if (b->f.game.sim_frame==f->game.sim_frame) continue;
            presenter_interpolate(&f->game,&b->f.game,fraction(t,r->sim_time,b->sim_time),&pose); break;
        }
        /* The simulation camera moves in whole tiles and culls what that step puts off
         * screen. The newer view supplies the sprite set, so such a sprite would vanish at
         * the start of the slide with part of it still in view: keep each sprite of the
         * older view that the newer one lacks and that lies outside the newer view's own
         * window (it left; it was not removed), at its world position, for this interval. */
        if (pose.view!=&f->game && pose.view->assets && pose.view->assets->cels) {
            const FrameGameView *old=&f->game, *nv=pose.view;
            int vw=nv->view_w>320?nv->view_w:320, added=0;
            for (unsigned i=0;i<old->sprite_count && i<FRAME_MAX_SPRITES;i++) {
                const FrameSprite *s=&old->sprites[i];
                if (s->screen_space || s->margin_only || s->frame>=1024) continue;
                unsigned k; for (k=0;k<nv->sprite_count;k++) if (nv->sprites[k].id==s->id && nv->sprites[k].gen==s->gen) break;
                if (k<nv->sprite_count) continue;
                const EnhCel *cel=&nv->assets->cels[s->frame];
                int bx=s->screen_x+old->camera_x-nv->camera_x, by=s->screen_y+old->camera_y-nv->camera_y;
                if (!(bx>=vw || bx+cel->w<=0 || by>=176 || by+cel->h<=0)) continue;
                if (!added) { p->leaving_view=*nv; added=1; }
                if (p->leaving_view.sprite_count>=FRAME_MAX_SPRITES) break;
                FrameSprite *d=&p->leaving_view.sprites[p->leaving_view.sprite_count++];
                *d=*s; d->screen_x=bx; d->screen_y=by;
            }
            if (added) pose.view=&p->leaving_view;
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
        double ix=f->game.camera_x+f->transition.cx-cx;
        double iy=f->game.camera_y+f->transition.cy-cy;
        int rc = iris ? enh_render_iris(pose.view,f->palette,f->pel_mask,cx,cy,rw,200,&opt,ix,iy,o->radius,&im) :
            enh_render(pose.view,pose.view->assets,f->palette,f->pel_mask,cx,cy,rw,200,&opt,&im);
        /* The captured iris spans over the HUD rows are positions in the original 320
         * strip. A wide layout moves the HUD fields apart, so there the strip is redrawn
         * from its state before the iris and cut by the published circle, which continues
         * the world rows' outline. */
        if (rc==0 && f->transition.kind==1 && (rw!=320 || pose.view->view_w>320) &&
            p->hud_valid && p->hud_epoch==pose.view->level_epoch) {
            p->hud_view=*pose.view;
            p->hud_view.hud_count=p->hud_count; p->hud_view.hud_frame=p->hud_frame;
            memcpy(p->hud_view.hud_elements,p->hud_elements,p->hud_count*sizeof p->hud_elements[0]);
            if (!enh_render_hud(&p->hud_view,opt.hud,rw,176,&im))
                for (int y=176;y<200;y++) for (int x=0;x<rw;x++)
                    if ((x-ix)*(x-ix)+(y-iy)*(y-iy)>o->radius*o->radius) p->index[y*rw+x]=0;
        }
        if (rc==0) {
            o->fallback &= ~8u;
            /* Smooth shapes (the original's own timeline, drawn continuously).
             * Column reveal: one curtain opening from the centre over the whole width, its
             * half-width the original's right edge (16 pixels of 320 per step); the left
             * side mirrors it instead of trailing by a strip, so the far-left strip the
             * original never copies opens with the rest. The destination is the page the
             * original copies from, complete before the first column.
             * Block wipe: the two bands (ten rows per step, from the top and the bottom
             * edge) close towards the middle at a steady rate. */
            double advance = pair ? (next->transition.step-f->transition.step)*step_t : 0;
            int curtain = p->opt.smooth_transitions && f->transition.kind==2 &&
                pose.view->surface && pose.view->surface->reveal;
            if (curtain) {
                p->curtain_surface=*pose.view->surface; p->curtain_surface.columns=0xFFFFFu;
                p->curtain_view=*pose.view; p->curtain_view.surface=&p->curtain_surface;
                EnhImage ni={0}; ni.index=p->next_index; ni.capacity=sizeof p->next_index; ni.stride=rw;
                curtain = !enh_render(&p->curtain_view,p->curtain_view.assets,f->palette,f->pel_mask,cx,cy,rw,200,&opt,&ni);
            }
            double curtain_half = (f->transition.step+advance)*rw/20.0;
            int bands = p->opt.smooth_transitions && f->transition.kind==3 && pair && step_t>0;
            double band_rows = 10.0*(f->transition.step+advance);
            int ox=(o->w-rw)/2;
            uint32_t pal[256];
            for (int i=0;i<256;i++) pal[i]=rgb(f->palette[i&f->pel_mask],(pal_b?pal_b:f)->palette[i&f->pel_mask],pal_t);
            /* RunIrisClose's centre excludes shake; its retained source world
             * already contains the original shake displacement. */
            for (int y=0;y<200;y++) for (int x=0;x<rw;x++) {
                uint32_t c=pal[p->index[y*rw+x]];
                if (y<176 && (floor(cx)+x<window.visible.x0 || floor(cx)+x>=window.visible.x1 ||
                              floor(cy)+y<window.visible.y0 || floor(cy)+y>=window.visible.y1)) c=0xff000000u;
                if (curtain && y<176 && fabs(x+.5-rw/2.0)<curtain_half) c=pal[p->next_index[y*rw+x]];
                if (bands && (y<band_rows || y>=176-band_rows) && y<176) c=pal[0];
                o->pixels[y*o->w+x+ox]=c;
            }
            o->camera_x=cx; o->camera_y=cy;
        } else o->fallback|=1;
    }
    /* Native step endpoints retain the original span arithmetic and sprite
     * redraw. A circle is used only inside a compatible smoothing interval. */
    if (f->transition.kind==1 && !world) o->fallback|=2;
    if (pair && step_t>0 && (f->transition.kind==2 || f->transition.kind==3) && !enhanced && !screen) mask_steps(f,next,step_t,o);
    if (!screen && pair && step_t>0 && f->transition.kind==5 && scroll_steps(f,next,step_t,o)) o->fallback|=4;
    return 0;
}
