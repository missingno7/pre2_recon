/* The existing present_purity schedules, now calling the real presenter at
 * monitor refresh rates. All modes have all three presentation options enabled;
 * faithful/enhanced with every option off are also checked at every publication. */
#include "script.h"
#include "c/vga.h"
#include "native/compositor.h"
#include "native/presenter.h"

#define MAXS 20000
typedef struct Trace {
    uint64_t h[MAXS], frames, pcm, end;
    unsigned n;
    RtStats stats;
    uint32_t game_frames;
} Trace;
static Trace baseline,trial,*cur;
static Presenter *present[3],*plain[2];
static Pre2Frame frame;
static uint32_t pixels[PRESENTER_MAX_W*PRESENTER_MAX_H],expected[FRAME_MAX_W*FRAME_MAX_H];
static unsigned last_frame,checks,draws;
static const char *last_phase;
static void service(void)
{
    script_service();
    if (pre2_frames_run!=last_frame || pre2_phase!=last_phase) {
        last_frame=pre2_frames_run; last_phase=pre2_phase;
        if (cur->n==MAXS) exit(1);
        cur->h[cur->n++]=fnv64(pre2_mem,0xa0000,FNV0);
    }
}
static void pcm(uint64_t t,const uint8_t *data,uint32_t n,uint32_t rate)
{
    cur->pcm=fnv64(&rate,sizeof rate,cur->pcm); cur->pcm=fnv64(&t,sizeof t,cur->pcm); cur->pcm=fnv64(data,n,cur->pcm);
}
static void display(uint64_t t,uint16_t start)
{
    compositor_publish(t,start,&frame);
    cur->frames=fnv64(frame.index,(size_t)frame.w*frame.h,cur->frames);
    cur->frames=fnv64(frame.palette,sizeof frame.palette,cur->frames);
    cur->frames=fnv64(&frame.transition,sizeof frame.transition,cur->frames);
    cur->frames=fnv64(&frame.blank,sizeof frame.blank,cur->frames);
    if (frame.game.valid) cur->frames=enh_view_hash(&frame.game,cur->frames);
    for (unsigned i=0;i<3;i++) if (present[i]) presenter_push(present[i],&frame);
    if (plain[0]) {
        frame_to_xrgb(&frame,expected);
        for (unsigned i=0;i<2;i++) {
            presenter_push(plain[i],&frame);
            PresenterImage im={0}; im.pixels=pixels; im.capacity=sizeof pixels/4;
            if (presenter_render(plain[i],t,1.6,&im) || im.w!=frame.w || im.h!=frame.h ||
                memcmp(pixels,expected,(size_t)im.w*im.h*4)) {
                fprintf(stderr,"unsmoothed mode %u differs at %.6f level %u kind %u world %u\n",i,t/1e9,frame.game.level,frame.transition.kind,frame.game.world_valid); exit(1);
            }
        }
        checks++;
    }
}
static void run(const char *assets,const char *scenario,double seconds,int hz,Trace *tr)
{
    memset(tr,0,sizeof *tr); cur=tr; tr->frames=tr->pcm=FNV0;
    last_frame=0; last_phase=NULL; pre2_frames_run=0; script_reset(scenario);
    rt_boot(assets,1995,6,1); vga_memory_off=1; compositor_init();
    if (hz) for (unsigned i=0;i<3;i++) {
        PresenterOptions opt=presenter_defaults(); opt.mode=(PresenterMode)i;
        opt.interpolate=opt.smooth_camera=opt.smooth_transitions=1; opt.aspect=21.0/9;
        present[i]=presenter_create(&opt); if (!present[i]) exit(1);
    }
    if (hz==60) for (unsigned i=0;i<2;i++) {
        PresenterOptions opt=presenter_defaults(); opt.mode=(PresenterMode)i;
        plain[i]=presenter_create(&opt); if (!plain[i]) exit(1);
    }
    RtConfig cfg={0}; cfg.audio_native=1;
    rt_on_service=service; rt_on_display_frame=display; rt_on_pcm=pcm;
    if (rt_init(&cfg)) exit(1);
    uint64_t end=(uint64_t)(seconds*1e9), step=hz?1000000000ull/(unsigned)hz:1000000000ull/60;
    for (uint64_t bound=step;;bound+=step) {
        if (bound>end) bound=end;
        if (rt_run_until(bound)!=RT_STEP_DONE) exit(1);
        if (hz) for (unsigned i=0;i<3;i++) {
            PresenterImage im={0}; im.pixels=pixels; im.capacity=sizeof pixels/4;
            if (!presenter_render(present[i],bound,1.6,&im)) draws++;
        }
        if (bound==end) break;
    }
    tr->end=fnv64(pre2_mem,0xa0000,FNV0); tr->stats=*rt_stats(); tr->game_frames=pre2_frames_run;
    for (unsigned i=0;i<3;i++) { presenter_destroy(present[i]); present[i]=NULL; }
    for (unsigned i=0;i<2;i++) { presenter_destroy(plain[i]); plain[i]=NULL; }
    pre2_scene_sink=NULL; vga_memory_off=0;
}
int main(int argc,char **argv)
{
    const char *assets=arg_assets(argc,argv);
    static const struct { const char *name; double seconds; } cases[]={
        {"none",45},{"warp",60},{"gameover",75},{"attract",110}
    };
    const int rates[]={60,144,240};
    for (unsigned k=0;k<4;k++) {
        run(assets,cases[k].name,cases[k].seconds,0,&baseline);
        for (unsigned j=0;j<3;j++) {
            run(assets,cases[k].name,cases[k].seconds,rates[j],&trial);
            int same=baseline.n==trial.n && !memcmp(baseline.h,trial.h,baseline.n*sizeof *baseline.h) &&
                baseline.end==trial.end && baseline.frames==trial.frames && baseline.pcm==trial.pcm &&
                !memcmp(&baseline.stats,&trial.stats,sizeof baseline.stats) && baseline.game_frames==trial.game_frames;
            printf("%s %d Hz, three modes + all options: %s (%u state samples)\n",cases[k].name,rates[j],same?"equal":"DIFFERENT",trial.n);
            if (!same) return 1;
        }
    }
    printf("presenter_purity: %u renders; %u exact unsmoothed faithful/enhanced publication pairs; memory, every machine counter, PCM and published frames unchanged\n",draws,checks);
    return 0;
}
