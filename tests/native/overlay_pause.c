/* Virtual wall time drives the same pause mapper used under the host lock.
 * The translated game runs only outside modal intervals; no real sleeps/SDL. */
#include "script.h"
#include "c/vga.h"
#include "native/compositor.h"
#include "native/overlay.h"
#include "native/host_clock.h"
#include <assert.h>
#undef assert
#define assert(c) do { if(!(c)) {fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#c); exit(1);} } while(0)


typedef struct Trace {uint64_t memory,pcm,frames,samples,end; RtStats stats; uint32_t game_frames;} Trace;
static Trace *cur;
static uint32_t last_frame;
static const char *last_phase;
static uint64_t seq_base;
static int seq_seeded;
static Pre2Frame frame;
static uint32_t px[320*200];
static void service(void)
{
    script_service();
    if(pre2_frames_run!=last_frame || pre2_phase!=last_phase) {
        last_frame=pre2_frames_run; last_phase=pre2_phase;
        cur->memory=fnv64(pre2_mem,0xa0000,cur->memory); cur->samples++;
    }
}
static void pcm(uint64_t t,const uint8_t *data,uint32_t n,uint32_t rate)
{
    cur->pcm=fnv64(&t,sizeof t,cur->pcm); cur->pcm=fnv64(&rate,sizeof rate,cur->pcm); cur->pcm=fnv64(data,n,cur->pcm);
}
static void display(uint64_t t,uint16_t start)
{
    compositor_publish(t,start,&frame);
    cur->frames=fnv64(&t,sizeof t,cur->frames);
    /* Compositor's serial is process-global, including across rt_boot calls.
     * Compare the per-run sequence as well as every logical publication time. */
    if(!seq_seeded) {seq_base=frame.seq-1; seq_seeded=1;}
    uint64_t sequence=frame.seq-seq_base;
    cur->frames=fnv64(&sequence,sizeof sequence,cur->frames);
    cur->frames=fnv64(&frame.w,sizeof frame.w,cur->frames);
    cur->frames=fnv64(&frame.h,sizeof frame.h,cur->frames);
    cur->frames=fnv64(&frame.blank,sizeof frame.blank,cur->frames);
    cur->frames=fnv64(&frame.pel_mask,sizeof frame.pel_mask,cur->frames);
    if(frame.screen) cur->frames=fnv64(frame.screen,strlen(frame.screen),cur->frames);
    cur->frames=fnv64(frame.index,(size_t)frame.w*frame.h,cur->frames);
    cur->frames=fnv64(frame.palette,sizeof frame.palette,cur->frames);
    cur->frames=fnv64(&frame.transition,sizeof frame.transition,cur->frames);
    if(frame.game.valid) cur->frames=enh_view_hash(&frame.game,cur->frames);
}
static Trace run(const char *assets,const char *scenario,int pause)
{
    Trace result={0}; cur=&result; result.memory=result.pcm=result.frames=FNV0;
    last_frame=0; last_phase=NULL; seq_seeded=0; pre2_frames_run=0; script_reset(scenario);
    rt_boot(assets,1995,6,1); vga_memory_off=1; compositor_init();
    rt_on_service=service; rt_on_display_frame=display; rt_on_pcm=pcm;
    RtConfig cfg={0}; cfg.audio_native=1; assert(!rt_init(&cfg));
    HostClock clock; uint64_t wall=1000000000ull,bound=0; host_clock_init(&clock,wall);
    Pre2Settings settings=settings_defaults(); Pre2Overlay overlay={0};
    const uint64_t step=1000000000ull/60,end=45000000000ull;
    unsigned pauses=0;
    while(bound<end) {
        wall+=step; bound=host_clock_bound(&clock,wall); if(bound>end) bound=end;
        assert(rt_run_until(bound)==RT_STEP_DONE);
        if(pause && ((pauses==0 && bound>=5000000000ull) || (pauses==1 && bound>=38000000000ull))) {
            uint64_t t=rt_now(),mem=fnv64(pre2_mem,0xa0000,FNV0),pcm0=result.pcm,frames0=result.frames;
            RtStats st=*rt_stats();
            overlay_key(&overlay,&settings,OVERLAY_TOGGLE); host_clock_pause(&clock,1,wall);
            for(unsigned refresh=0;refresh<300;refresh++) {
                wall+=step; assert(host_clock_bound(&clock,wall)==bound);
                if(refresh%50==0) overlay_key(&overlay,&settings,OVERLAY_TAB);
                if(refresh%30==0) overlay_key(&overlay,&settings,OVERLAY_RIGHT);
                memset(px,0,sizeof px); overlay_render(&overlay,&settings,NULL,px,320,200);
                assert(rt_now()==t && !memcmp(&st,rt_stats(),sizeof st));
            }
            assert(mem==fnv64(pre2_mem,0xa0000,FNV0) && result.pcm==pcm0 && result.frames==frames0);
            overlay_key(&overlay,&settings,OVERLAY_ESCAPE); host_clock_pause(&clock,0,wall);
            assert(host_clock_bound(&clock,wall)==bound); pauses++;
        }
    }
    result.end=fnv64(pre2_mem,0xa0000,FNV0); result.stats=*rt_stats(); result.game_frames=pre2_frames_run;
    pre2_scene_sink=NULL; vga_memory_off=0;
    return result;
}
int main(int argc,char **argv)
{
    const char *assets=arg_assets(argc,argv),*cases[]={"none","warp","attract"};
    HostClock c; host_clock_init(&c,100); assert(host_clock_bound(&c,110)==10);
    assert(host_clock_target(&c,120)==20 && c.logical==10);
    host_clock_pause(&c,1,120); assert(host_clock_bound(&c,100000)==10);
    host_clock_pause(&c,0,100100); assert(host_clock_bound(&c,100110)==20);
    for(unsigned i=0;i<3;i++) {
        Trace a=run(assets,cases[i],0),b=run(assets,cases[i],1);
        assert(!memcmp(&a,&b,sizeof a));
        printf("%s: two 5s virtual wall pauses at T=5s/38s; %llu memory samples, all counters/PCM/publications/end memory equal\n",cases[i],(unsigned long long)a.samples);
    }
    puts("overlay_pause: pause boundaries freeze logical time; resume has no catch-up jump");
    return 0;
}
