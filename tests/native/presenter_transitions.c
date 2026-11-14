#include "native/presenter.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* A failed CRT assert opens a modal dialog on some Windows runners. Keep a
 * bounded command-line test bounded and put the failed expression in stderr. */
#undef assert
#define assert(expr) do { if (!(expr)) { fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#expr); exit(1); } } while (0)

static Pre2Frame a,b;
static uint32_t pixels[640*480], expected[640*480];
static EnhLevel level;
static EnhMap map;
static uint8_t row[256];
static PresenterImage im;
static void sample(Presenter *p,uint64_t t)
{
    im.pixels=pixels; im.capacity=sizeof pixels/sizeof *pixels;
    assert(!presenter_render(p,t+PRESENTER_DELAY_NS,1.6,&im));
    assert(im.time==t);
}
static void endpoint(Presenter *p,Pre2Frame *f)
{
    sample(p,f->t_ns); frame_to_xrgb(f,expected);
    assert(im.w==f->w && im.h==f->h && !memcmp(pixels,expected,(size_t)im.w*im.h*4));
}
int main(void)
{
    PresenterOptions opt=presenter_defaults(); opt.smooth_transitions=1;
    a.w=b.w=320; a.h=b.h=200; a.pel_mask=b.pel_mask=255;
    a.t_ns=1000000000; b.t_ns=a.t_ns+30000000;
    memset(a.index,1,64000); memset(b.index,1,64000);
    a.palette[1][0]=10; b.palette[1][0]=50;
    for (unsigned kind=1;kind<=5;kind++) {
        a.transition=(FrameTransition){.kind=(uint8_t)kind,.phase=kind==2?2:0,.step=1,.steps=10,.cx=160,.cy=88,.radius=100};
        b.transition=a.transition; b.transition.step=2; b.transition.radius=90;
        if (kind==2 || kind==3) { memset(b.index,2,64000); b.palette[2][0]=63; }
        else memcpy(b.index,a.index,64000);
        Presenter *p=presenter_create(&opt); assert(p); presenter_push(p,&a); presenter_push(p,&b);
        endpoint(p,&a); endpoint(p,&b);
        double last=1,last_r=100;
        unsigned last_red=0;
        for (unsigned k=1;k<30;k++) {
            sample(p,a.t_ns+k*1000000);
            assert(im.progress>=last && im.progress<=2); last=im.progress;
            assert(im.radius<=last_r && im.radius>=90); last_r=im.radius;
            if (kind==4) { unsigned red=(pixels[0]>>16)&255; assert(red>=last_red && red<=203); last_red=red; }
        }
        presenter_destroy(p);
    }
    /* A validated map translation uses bilinear colour samples. A stationary
     * overlay or missing edge repair makes it an explicit faithful fallback. */
    memcpy(b.palette,a.palette,sizeof a.palette); a.palette[2][0]=b.palette[2][0]=63;
    for (int y=0;y<200;y++) for (int x=0;x<320;x++) a.index[y*320+x]=(uint8_t)(1+x%2);
    for (int y=0;y<200;y++) for (int x=0;x<320;x++) b.index[y*320+x]=a.index[y*320+(x+1)%320];
    a.transition.kind=b.transition.kind=5; a.transition.cx=0; b.transition.cx=1;
    Presenter *p=presenter_create(&opt); presenter_push(p,&a); presenter_push(p,&b);
    sample(p,a.t_ns+15000000); assert(!(im.fallback&4)); assert(pixels[0]!=0xff280000u);
    endpoint(p,&a); endpoint(p,&b); presenter_destroy(p);
    b.index[100]=0; p=presenter_create(&opt); presenter_push(p,&a); presenter_push(p,&b);
    sample(p,a.t_ns+15000000); assert(im.fallback&4); presenter_destroy(p);
    /* A real enhanced-world circle, with duplicate CRT starts between distinct
     * original steps. The progress clock must still begin at the FIRST start. */
    level.rows=100; level.policy.end_column=256;
    memset(level.tiles[0],1,256);
    for (unsigned i=0;i<256;i++) map.rows[i]=row;
    a.game.valid=a.game.world_valid=1; a.game.assets=&level; a.game.map=&map;
    a.game.camera_limit_column=236; a.game.camera_limit_row=89;
    memset(a.game.hud,1,sizeof a.game.hud);
    b.game=a.game;
    a.transition=(FrameTransition){.kind=1,.step=1,.steps=10,.cx=160,.cy=88,.radius=100};
    b.transition=a.transition; b.transition.step=2; b.transition.radius=90;
    memset(a.index,2,64000); memset(b.index,2,64000);
    a.palette[1][0]=b.palette[1][0]=63;
    opt.mode=PRESENT_ENHANCED;
    p=presenter_create(&opt); presenter_push(p,&a);
    Pre2Frame *duplicate=malloc(sizeof *duplicate); assert(duplicate); *duplicate=a; duplicate->t_ns+=10000000;
    presenter_push(p,duplicate); presenter_push(p,&b);
    endpoint(p,&a); endpoint(p,&b);
    sample(p,a.t_ns+15000000);
    assert(im.radius==95 && !(im.fallback&2));
    assert(pixels[88*320+160]==0xffff0000u && pixels[0]==0xff000000u);
    assert(pixels[199*320+160]!=0xff000000u); /* HUD not covered by circle */
    free(duplicate); presenter_destroy(p);
    /* Even a preloaded history must not use an endpoint beyond the host's
     * logical bound. This also covers a producer racing ahead of its mailbox. */
    a.game.valid=b.game.valid=0; a.transition.kind=b.transition.kind=4;
    a.palette[1][0]=10; b.palette[1][0]=50;
    memset(a.index,1,64000); memset(b.index,1,64000); b.t_ns=a.t_ns+100000000;
    p=presenter_create(&opt); presenter_push(p,&a); presenter_push(p,&b);
    sample(p,a.t_ns+15000000); assert(im.progress==a.transition.step && pixels[0]==0xff280000u);
    presenter_destroy(p);
    puts("presenter_transitions: exact published endpoints, bounded monotonic progress/radius, per-entry RGB fades, masks, validated map translation/fallback pass");
    return 0;
}
