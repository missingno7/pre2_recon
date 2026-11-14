/* Opt-in simulation geometry smoke and determinism test.
 * Runs each authored level at 480 and 640 pixels, twice, with scripted movement,
 * jump and attack input. It checks captured identities on every display frame and
 * saves eight enhanced gameplay views to build/wide_shots/wide_sim.bmp. */
#include "script.h"
#include "c/scene_game.h"
#include "c/view.h"
#include "c/pools.h"
#include "native/compositor.h"
#include "native/enh_render.h"
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#define MKDIR(p) _mkdir(p)
#else
#define MKDIR(p) mkdir(p, 0777)
#endif

enum { LEVELS=16, SHOT_LEVELS=4, SHOTS=8, SHOT_W=640, SHOT_H=176 };
static Pre2Frame frame;
static uint8_t enhanced[640*176];
static uint8_t sheet[SHOT_W*SHOT_H*2*4*4]; /* RGBA, 4 columns by 2 rows */
static const unsigned shot_levels[SHOT_LEVELS]={1,2,6,8};
static unsigned target_level, selected, reached, alive_seen, dead_seen;
static unsigned duplicate_ids, invalid_geometry, pool_write_bad, display_count, shot_written, margin_enemy_frames;
static unsigned margin_enemy_total;
static unsigned margin_child_frames, margin_child_sprites, margin_level_mask;
static unsigned nonempty_views, max_view_sprites;
static unsigned highwater[6];
static unsigned aggregate_highwater[6];
static uint64_t digest=FNV0;
static uint64_t first_game_ns, last_input_ns;
static unsigned input_phase;
static int guard_ready;

static void check_pool_extents(void)
{
    static const unsigned recsz[6]={18,18,18,18,6,18};
    for(unsigned k=0;k<6;k++) {
        unsigned n=pre2_pool_count((Pre2PoolKind)k);
        uint16_t last=pre2_pool_slot((Pre2PoolKind)k,n-1);
        if(last==0xFFFF || (unsigned)last+recsz[k]>0xDB80) pool_write_bad++;
    }
    if(!guard_ready) {
        for(unsigned i=0;i<16;i++) SET_DB((uint16_t)(0xDB80+i),0xD7);
        guard_ready=1;
    } else for(unsigned i=0;i<16;i++) if(DB((uint16_t)(0xDB80+i))!=0xD7) pool_write_bad++;
}

static void hash_bytes(const void *p,size_t n)
{ digest=fnv64(p,n,digest); }

static void hash_view(const FrameGameView *v)
{
    hash_bytes(&v->level,sizeof v->level); hash_bytes(&v->sim_frame,sizeof v->sim_frame);
    hash_bytes(&v->camera_x,sizeof v->camera_x); hash_bytes(&v->camera_y,sizeof v->camera_y);
    hash_bytes(&v->view_w,sizeof v->view_w); hash_bytes(&v->view_h,sizeof v->view_h);
    hash_bytes(&v->sprite_count,sizeof v->sprite_count); hash_bytes(&v->dot_count,sizeof v->dot_count);
    for (unsigned i=0;i<v->sprite_count;i++) {
        const FrameSprite *s=&v->sprites[i]; hash_bytes(&s->id,sizeof s->id);
        hash_bytes(&s->gen,sizeof s->gen); hash_bytes(&s->frame,sizeof s->frame);
        hash_bytes(&s->world_x,sizeof s->world_x); hash_bytes(&s->world_y,sizeof s->world_y);
    }
    for (unsigned i=0;i<v->dot_count;i++) hash_bytes(&v->dots[i],sizeof v->dots[i]);
}

static void check_ids(const FrameGameView *v)
{
    if (v->sprite_count>FRAME_MAX_SPRITES || v->dot_count>FRAME_MAX_DOTS) { invalid_geometry++; return; }
    for (unsigned i=0;i<v->sprite_count;i++) {
        if (!v->sprites[i].id) { invalid_geometry++; continue; }
        for (unsigned j=0;j<i;j++) if(v->sprites[i].id==v->sprites[j].id) duplicate_ids++;
    }
    for (unsigned i=0;i<v->dot_count;i++) {
        if (!v->dots[i].id || !v->dots[i].gen) invalid_geometry++;
        for (unsigned j=0;j<i;j++) if (v->dots[i].id==v->dots[j].id) duplicate_ids++;
    }
    if (v->level==target_level) {
        reached++;
        if (DB(DS_Energy)) alive_seen++;
        else {
            dead_seen++;
            if(!DB(DS_RespawnState) && !DB(DS_EndSignal) && !DB(DS_PlayerHurtTimer)) invalid_geometry++;
        }
        for(unsigned k=0;k<6;k++) {
            unsigned n=pre2_pool_count((Pre2PoolKind)k), used=0;
            unsigned freeoff=k==PRE2_POOL_POINT?0:4;
            for(unsigned i=0;i<n;i++) if(DW(pre2_pool_slot((Pre2PoolKind)k,i)+freeoff)!=0xFFFF) used++;
            if(used>highwater[k]) highwater[k]=used;
        }
        unsigned expected=pre2_view.variant ? (target_level==5 || target_level==9 || target_level==14 ? 320 : pre2_view.requested_width) : 320;
        if(!pre2_view.variant || v->view_w!=expected || v->view_h!=176) invalid_geometry++;
    }
    if(v->level==target_level && v->view_w>320) {
        unsigned n=0;
        for(unsigned i=0;i<v->sprite_count;i++) {
            const FrameSprite *s=&v->sprites[i];
            int child=(s->slot>=11 && s->slot<23) || (s->slot>=116 && s->slot<372);
            if(child && s->screen_x>=320 && s->screen_x<(int32_t)v->view_w) n++;
        }
        if(n) { margin_child_frames++; margin_child_sprites+=n; margin_level_mask|=1u<<target_level; }
    }
}

static void put_shot(const FrameGameView *v,const Pre2Frame *f)
{
    if (v->level!=target_level || !v->world_valid || !v->assets || !v->map || !v->sprite_count) return;
    unsigned li=0; while(li<SHOT_LEVELS && shot_levels[li]!=target_level) li++;
    if(li==SHOT_LEVELS) return;
    unsigned wi=pre2_view.requested_width==640;
    unsigned at=li*2+wi;
    unsigned sx=(at%4)*SHOT_W, sy=(at/4)*SHOT_H;
    if (shot_written & (1u<<at)) return;
    unsigned margin_enemy=0;
    unsigned has_player=0;
    for(unsigned i=0;i<v->sprite_count;i++) {
        const FrameSprite *s=&v->sprites[i];
        if(s->slot==1) has_player=1;
        if(((s->slot>=11 && s->slot<23) || (s->slot>=116 && s->slot<372)) &&
           s->screen_x>=320 && s->screen_x<(int32_t)v->view_w) margin_enemy++;
    }
    if(!has_player) return;
    if(margin_enemy) { margin_enemy_frames++; margin_enemy_total++; }
    /* Prefer a frame with an active child in the added aperture. For known
     * fixed-room policies, save the final gameplay view to show the letterbox. */
    if(!margin_enemy && target_level!=5 && target_level!=9 && target_level!=14 &&
       rt_now()<first_game_ns+7000000000ull) return;
    EnhImage out={0}; out.index=enhanced; out.capacity=sizeof enhanced; out.stride=pre2_view.requested_width;
    if(enh_render(v,v->assets,f->palette,f->pel_mask,v->camera_x,v->camera_y,
                  (int)pre2_view.requested_width,SHOT_H,NULL,&out)) return;
    for(unsigned y=0;y<SHOT_H;y++) for(unsigned x=0;x<pre2_view.requested_width;x++) {
        const uint8_t *c=f->palette[enhanced[y*pre2_view.requested_width+x]&f->pel_mask];
        uint8_t *p=sheet+((size_t)(sy+y)*(SHOT_W*SHOT_LEVELS*4)+(sx+x)*4);
        p[0]=(uint8_t)(c[2]<<2); p[1]=(uint8_t)(c[1]<<2); p[2]=(uint8_t)(c[0]<<2); p[3]=255;
    }
    shot_written |= 1u<<at;
}

static void on_display(uint64_t t,uint16_t start)
{
    compositor_publish(t,start,&frame);
    if(!frame.game.valid || strcmp(pre2_phase,"MainFrameLoop")) return;
    display_count++;
    if(frame.game.sprite_count) { nonempty_views++; if(frame.game.sprite_count>max_view_sprites) max_view_sprites=frame.game.sprite_count; }
    check_pool_extents(); check_ids(&frame.game); hash_view(&frame.game); put_shot(&frame.game,&frame);
}

static void input_service(void)
{
    script_service(); /* drives start-up, menu and map exactly like game_levels.c */
    uint64_t now=rt_now();
    if(!selected && !strcmp(pre2_phase,"ShowWorldMapScrollIn()")) {
        SET_DB(DS_CurrentLevel,target_level<10?target_level:0);
        SET_DB(DS_byte_B198,1); selected=1;
    }
    if(!strcmp(pre2_phase,"MainFrameLoop") && !first_game_ns) first_game_ns=now;
    if(target_level>=10 && selected==1 && first_game_ns && now-first_game_ns>1800000000ull) {
        SET_DB(DS_WarpLevelTable,target_level); SET_DB(DS_LevelEndMode,2); selected=2;
    }
    if(!first_game_ns || now-first_game_ns<100000000ull) return;
    /* script_service keeps right held continuously like game_levels.c. This
     * schedule adds jump and Ctrl/attack without releasing that walk input. */
    if(now-last_input_ns<300000000ull) return;
    last_input_ns=now; input_phase++;
    switch(input_phase%4) {
    case 0: rt_key_at(0x48,now); break;       /* up / jump */
    case 1: rt_key_at(0xC8,now); break;
    case 2: rt_key_at(0x1D,now); break;       /* Ctrl / attack */
    default: rt_key_at(0x9D,now); break;
    }
}

static void write_bmp(const char *path)
{
    FILE *f=fopen(path,"wb"); if(!f) return;
    uint8_t h[54]={ 'B','M' }; uint32_t size=54+sizeof sheet;
    memcpy(h+2,&size,4); h[10]=54; h[14]=40;
    int32_t w=SHOT_W*SHOT_LEVELS,hh=-(SHOT_H*2); memcpy(h+18,&w,4); memcpy(h+22,&hh,4);
    h[26]=1; h[28]=32;
    fwrite(h,1,sizeof h,f);
    for(unsigned y=0;y<SHOT_H*2;y++) fwrite(sheet+y*(SHOT_W*SHOT_LEVELS*4),1,SHOT_W*SHOT_LEVELS*4,f);
    fclose(f);
}

static uint64_t run_once(const char *assets,unsigned level,unsigned width,unsigned pass)
{
    memset(&frame,0,sizeof frame); memset(highwater,0,sizeof highwater);
    selected=reached=alive_seen=dead_seen=duplicate_ids=invalid_geometry=pool_write_bad=display_count=margin_enemy_frames=0;
    first_game_ns=last_input_ns=0; input_phase=0; digest=FNV0;
    guard_ready=0;
    nonempty_views=max_view_sprites=0;
    pre2_frames_run=0;
    target_level=level; script_reset("none");
    rt_boot(assets,1995,6,1); compositor_init();
    RtConfig cfg={0}; cfg.audio_native=1;
    if(rt_set_wide_sim_width(width)) return 0;
    rt_on_service=input_service; rt_on_display_frame=on_display; rt_on_pcm=0;
    if(rt_init(&cfg)) return 0;
    uint64_t end=38ull*RT_NS_PER_SEC;
    for(uint64_t t=0;t<end;t+=RT_NS_PER_SEC/60) {
        if(rt_run_until(t)!=RT_STEP_DONE) break;
        if(first_game_ns && t>first_game_ns+9000000000ull) break;
    }
    pre2_scene_sink=0;
    /* Include the translated game's full address space in repeat-run equality;
     * semantic frame equality alone would miss divergent hidden simulation state. */
    hash_bytes(pre2_mem,MEM_SIZE);
    for(unsigned k=0;k<6;k++) if(highwater[k]>aggregate_highwater[k]) aggregate_highwater[k]=highwater[k];
    printf("wide level %02X width %u pass %u: frames %u reached %u alive %u dead %u captured %u maxsprites %u duplicates %u bad %u extent %u margin %u pools C%u B%u P%u D%u X%u T%u digest %016llX\n",
        level,width,pass,display_count,reached,alive_seen,dead_seen,nonempty_views,max_view_sprites,duplicate_ids,invalid_geometry,pool_write_bad,margin_enemy_frames,
        highwater[0],highwater[1],highwater[2],highwater[3],highwater[4],highwater[5],
        (unsigned long long)digest);
    return digest;
}

int main(int argc,char **argv)
{
    const char *assets=arg_assets(argc,argv); memset(sheet,0,sizeof sheet); shot_written=0; margin_enemy_total=0;
    memset(aggregate_highwater,0,sizeof aggregate_highwater);
    margin_child_frames=margin_child_sprites=margin_level_mask=0;
    unsigned failed=0;
    for(unsigned widx=0;widx<2;widx++) {
        unsigned width=widx?640:480;
        int only_level=-1,only_width=0;
        for(int ai=1;ai+1<argc;ai++) {
            if(!strcmp(argv[ai],"--level")) only_level=atoi(argv[ai+1]);
            if(!strcmp(argv[ai],"--width")) only_width=atoi(argv[ai+1]);
        }
        if(only_width && only_width!=(int)width) continue;
        for(unsigned level=0;level<LEVELS;level++) {
            if(only_level>=0 && level!=(unsigned)only_level) continue;
            uint64_t a=run_once(assets,level,width,1);
            unsigned r1=reached, i1=invalid_geometry, d1=duplicate_ids, p1=pool_write_bad;
            uint64_t b=run_once(assets,level,width,2);
            if(!a || a!=b || !r1 || !reached || i1 || invalid_geometry || d1 || duplicate_ids ||
               p1 || pool_write_bad || !display_count || !nonempty_views || (!alive_seen&&!dead_seen)) failed++;
        }
    }
    MKDIR("build"); MKDIR("build/wide_shots");
    write_bmp("build/wide_shots/wide_sim.bmp");
    printf("wide high-water: child %u burst %u projections %u debris %u points %u terrain %u\n",
           aggregate_highwater[0],aggregate_highwater[1],aggregate_highwater[2],aggregate_highwater[3],aggregate_highwater[4],aggregate_highwater[5]);
    printf("wide child margins: %u frames, %u sprites across levels %04Xh; %u rendered samples\n",
           margin_child_frames,margin_child_sprites,margin_level_mask,margin_enemy_total);
    unsigned bad=failed || shot_written!=255u || margin_enemy_total==0 || margin_child_frames==0;
    printf("wide_sim: %s, captured %u/8 contact-sheet frames at build/wide_shots/wide_sim.bmp\n",
           bad?"FAIL":"PASS",(unsigned)__builtin_popcount(shot_written));
    return bad;
}
