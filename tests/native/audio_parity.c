/* Full-flow audio and non-audio simulation parity, at each main-loop frame. */
#include "script.h"
#include "c/audio_events.h"
#include "c/vga.h"

typedef struct { uint64_t t; uint8_t pcm[168]; } Block;
typedef struct { uint64_t t; AudioCmd c; } Command;
typedef struct { uint64_t t,h; uint32_t frame; } Frame;
typedef struct {
    Block *blocks; Command *commands; Frame *frames;
    size_t nb,nc,nf,cb,cc,cf;
    uint64_t end;
} Trace;
static Trace *trace;
static uint32_t last_frame;
/* Audit section 4: only the reference IRQ/mixer advance these private DATA fields.
 * ModuleSegment, resource flags/allocation state, PIT, SoundState, and the decoded
 * allocations are deliberately INCLUDED. No module/sample byte is written by IRQ.
 * SbIrqCount's sole non-audio ASM consumer was ESC's completion wait; C exits instead.
 * ChannelEnd's wrong-index stores stay inside the instrument range below. */
static const struct { uint16_t lo,hi; } excluded[]={
    {0x0b5d,0x0b5e}, /* completion count */
    {0x0b60,0x0b64}, /* saved SP and channel-index scratch */
    {0x0b84,0x0dc2}, /* sequencer, channels, aliased instrument records */
    {0x1004,0x1008}, /* replaceable SFX cursor and remaining count */
    {0x10b9,0x10bb}, /* saved volume row */
    {0x10c1,0x12c1}, /* buffer selectors, PCM A/B/C, overrun bytes */
};
static uint64_t state_hash(void) {
    uint64_t h=FNV0; size_t begin=0;
    for (size_t i=0;i<sizeof excluded/sizeof excluded[0];i++) {
        size_t lo=lin(DATA_SEG,excluded[i].lo), hi=lin(DATA_SEG,excluded[i].hi);
        h=fnv64(pre2_mem+begin,lo-begin,h); begin=hi;
    }
    return fnv64(pre2_mem+begin,0xa0000-begin,h);
}
static void *grow(void *p,size_t n,size_t *cap,size_t stride) {
    if (n<*cap) return p;
    *cap=*cap ? *cap*2 : 2048;
    p=realloc(p,*cap*stride); if (!p) exit(1); return p;
}
static void on_service(void) {
    script_service();
    if (pre2_frames_run==last_frame) return;
    last_frame=pre2_frames_run;
    trace->frames=grow(trace->frames,trace->nf,&trace->cf,sizeof *trace->frames);
    trace->frames[trace->nf++]=(Frame){rt_now(),state_hash(),last_frame};
}
static void on_pcm(uint64_t t,const uint8_t *p,uint32_t n,uint32_t rate) {
    if (n!=168 || rate!=8403) exit(1);
    trace->blocks=grow(trace->blocks,trace->nb,&trace->cb,sizeof *trace->blocks);
    Block *b=&trace->blocks[trace->nb++]; b->t=t; memcpy(b->pcm,p,168);
}
static void on_command(uint64_t t,const AudioCmd *c) {
    trace->commands=grow(trace->commands,trace->nc,&trace->cc,sizeof *trace->commands);
    trace->commands[trace->nc++]=(Command){t,*c};
}
static void run(const char *assets,const char *scenario,unsigned secs,int native,Trace *t) {
    memset(t,0,sizeof *t); trace=t; last_frame=0; pre2_frames_run=0;
    script_reset(scenario); rt_boot(assets,1995,6,1);
    RtConfig cfg={0,0,native};
    rt_on_service=on_service; rt_on_pcm=on_pcm; rt_on_audio_command=on_command;
    if (rt_init(&cfg)) exit(1);
    uint64_t end=(uint64_t)secs*RT_NS_PER_SEC;
    for (uint64_t b=0;b<end;) { b+=RT_NS_PER_SEC/60; if (b>end) b=end; if (rt_run_until(b)!=RT_STEP_DONE) break; }
    t->end=state_hash();
}
static int compare(const Trace *a,const Trace *b) {
    int bad=0;
    if (a->nb!=b->nb || a->nc!=b->nc || a->nf!=b->nf || a->end!=b->end) {
        printf(" counts reference/native: blocks %zu/%zu commands %zu/%zu frames %zu/%zu; end %s\n",
            a->nb,b->nb,a->nc,b->nc,a->nf,b->nf,a->end==b->end?"equal":"DIFFERENT"); bad=1;
    }
    for (size_t i=0;i<a->nb && i<b->nb;i++) if (a->blocks[i].t!=b->blocks[i].t || memcmp(a->blocks[i].pcm,b->blocks[i].pcm,168)) {
        size_t k=0; while (k<168 && a->blocks[i].pcm[k]==b->blocks[i].pcm[k]) k++;
        printf(" first PCM mismatch block %zu time %llu/%llu byte %zu",i,(unsigned long long)a->blocks[i].t,(unsigned long long)b->blocks[i].t,k);
        if (k<168) printf(" value %02x/%02x",a->blocks[i].pcm[k],b->blocks[i].pcm[k]);
        puts(""); bad=1; break;
    }
    for (size_t i=0;i<a->nc && i<b->nc;i++) {
        const Command *x=&a->commands[i], *y=&b->commands[i];
        if (x->t!=y->t || (x->c.kind!=y->c.kind || x->c.id!=y->c.id || x->c.src_space!=y->c.src_space || x->c.src_kind!=y->c.src_kind || x->c.src_ref!=y->c.src_ref || x->c.x!=y->c.x || x->c.y!=y->c.y || x->c.game_frame!=y->c.game_frame || x->c.tail_offset!=y->c.tail_offset || memcmp(x->c.tail,y->c.tail,16))) {
            printf(" first command mismatch %zu time %llu/%llu kind %u/%u id %u/%u\n",i,
                (unsigned long long)x->t,(unsigned long long)y->t,x->c.kind,y->c.kind,x->c.id,y->c.id); bad=1; break;
        }
    }
    for (size_t i=0;i<a->nf && i<b->nf;i++) if (a->frames[i].h!=b->frames[i].h || a->frames[i].t!=b->frames[i].t || a->frames[i].frame!=b->frames[i].frame) {
        printf(" first state mismatch frame %u time %llu/%llu\n",a->frames[i].frame,
            (unsigned long long)a->frames[i].t,(unsigned long long)b->frames[i].t); bad=1; break;
    }
    return bad;
}
static void release(Trace *t) { free(t->blocks); free(t->commands); free(t->frames); }
int main(int argc,char **argv) {
    static const struct { const char *name; unsigned secs; } cases[]={
        {"none",45},{"warp",60},{"ending",75},{"gameover",75},{"castle",75},{"attract",110}
    };
    int bad=0; size_t total=0;
    for (size_t i=0;i<sizeof cases/sizeof cases[0];i++) {
        Trace a,b;
        run(arg_assets(argc,argv),cases[i].name,cases[i].secs,0,&a);
        run(arg_assets(argc,argv),cases[i].name,cases[i].secs,1,&b);
        int fail=compare(&a,&b); bad+=fail; total+=a.nb;
        printf("%-9s %s: %zu blocks, %zu commands, %zu frame hashes\n",cases[i].name,fail?"DIFFERENT":"equal",a.nb,a.nc,a.nf);
        release(&a); release(&b);
    }
    printf("audio_parity: %zu blocks across six scenarios; %s\n",total,bad?"MISMATCH":"byte/time/state equal");
    return bad!=0;
}
