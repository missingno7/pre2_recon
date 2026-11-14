/* Capture on/off replay: compare the game, every machine counter, PCM and the
 * complete faithful publication (including its regular semantic view). */
#include "script.h"
#include "c/vga.h"
#include "native/compositor.h"
#include "native/enh_margin.h"
#include "native/enh_render.h"
#include <assert.h>
#define MAXS 20000
static struct Trace { uint64_t samples[MAXS],pcm,frames,end;unsigned n;uint32_t game_frames;RtStats stats; } traces[2],*trace;
static Pre2Frame frame;
static unsigned last_frame,wide;
static const char *last_phase;
static void service(void)
{
    script_service();
    if (last_frame!=pre2_frames_run || last_phase!=pre2_phase) {
        last_frame=pre2_frames_run;last_phase=pre2_phase;assert(trace->n<MAXS);
        trace->samples[trace->n++]=fnv64(pre2_mem,0xa0000,FNV0);
    }
}
static void display(uint64_t t,uint16_t start)
{
    compositor_publish(t,start,&frame);
    trace->frames=fnv64(frame.index,(size_t)frame.w*frame.h,trace->frames);
    trace->frames=fnv64(frame.palette,sizeof frame.palette,trace->frames);
    trace->frames=fnv64(&frame.transition,sizeof frame.transition,trace->frames);
    if (frame.game.valid) {
        unsigned count=frame.game.sprite_count;
        while (count && frame.game.sprites[count-1].margin_only) count--;
        if (wide) assert(count==frame.game.sprite_count);
        frame.game.sprite_count=(uint16_t)count;
        memset(frame.game.sprites+count,0,(FRAME_MAX_SPRITES-count)*sizeof(FrameSprite));
        trace->frames=enh_view_hash(&frame.game,trace->frames);
    }
}
static void pcm(uint64_t t,const uint8_t *p,uint32_t n,uint32_t rate)
{
    trace->pcm=fnv64(&t,sizeof t,trace->pcm);trace->pcm=fnv64(&rate,sizeof rate,trace->pcm);trace->pcm=fnv64(p,n,trace->pcm);
}
static void run(const char *assets,const char *scenario,unsigned seconds,int enabled,struct Trace *out)
{
    memset(out,0,sizeof *out);trace=out;out->pcm=out->frames=FNV0;last_frame=0;last_phase=NULL;pre2_frames_run=0;
    memset(&frame,0,sizeof frame);script_reset(scenario);rt_boot(assets,1995,6,1);vga_memory_off=1;compositor_init();enh_margin_enable(enabled);
    rt_set_wide_sim_width(wide);
    RtConfig cfg={0};cfg.audio_native=1;rt_on_service=service;rt_on_display_frame=display;rt_on_pcm=pcm;assert(!rt_init(&cfg));
    for (uint64_t t=0;t<(uint64_t)seconds*RT_NS_PER_SEC;t+=RT_NS_PER_SEC/60) assert(rt_run_until(t)==RT_STEP_DONE);
    out->end=fnv64(pre2_mem,0xa0000,FNV0);out->stats=*rt_stats();out->game_frames=pre2_frames_run;
    pre2_scene_sink=NULL;vga_memory_off=0;
}
int main(int argc,char **argv)
{
    static const struct {const char *scenario;unsigned seconds,width;} cases[]={{"none",45,0},{"warp",60,0},{"gameover",75,0},{"none",45,640}};
    for (unsigned i=0;i<sizeof cases/sizeof *cases;i++) {
        wide=cases[i].width;run(arg_assets(argc,argv),cases[i].scenario,cases[i].seconds,0,&traces[0]);run(arg_assets(argc,argv),cases[i].scenario,cases[i].seconds,1,&traces[1]);
        assert(traces[0].n==traces[1].n && !memcmp(traces[0].samples,traces[1].samples,traces[0].n*sizeof(uint64_t)));
        assert(traces[0].pcm==traces[1].pcm && traces[0].frames==traces[1].frames && traces[0].end==traces[1].end);
        assert(!memcmp(&traces[0].stats,&traces[1].stats,sizeof(RtStats)) && traces[0].game_frames==traces[1].game_frames);
        printf("regions_purity: %s, wide-sim %u: %u state samples, PCM, faithful frames, regular views and every machine counter equal\n",cases[i].scenario,wide,traces[0].n);
    }
    rt_set_wide_sim_width(0);enh_margin_enable(1);return 0;
}
