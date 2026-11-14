/* All-level native schedules: region coverage, source measurements, capture
 * purity and 320 parity. Contact views are labelled constructions, not routes. */
#include "script.h"
#include "c/vga.h"
#include "c/scene_game.h"
#include "native/compositor.h"
#include "native/enh_margin.h"
#include "native/level_regions.h"
#include "native/presenter.h"
#include "native/assetfs.h"
#include "native/sqz.h"
#include <assert.h>
#include <math.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#endif
#define W 468
#define H 200
static Pre2Frame frame;
static FrameGameView original,trimmed;
static uint8_t memory[0x100000],index[640*200];
static uint32_t pixels[W*H],sheet[W*4*(H+8)*6],map_pixels[1024*692];
static unsigned target,selected,reached,checked,last_sim,shot_score,shot_taken;
static uint64_t first_game;
static unsigned pure,parity,covered,unresolved,source_checks,activation_y_steps,float_checks,terrain_checks,terrain_activations;
static unsigned pop_before[3],pop_after[3];
static uint8_t old_regular[86],old_visible[86][3],old_margin[86][3];
static const int widths[]={356,468,640};
static const unsigned rows[]={49,104,49,45,128,128,128,86,110,12,24,51,51,38,173,84};
static const unsigned state_offsets[]={0x4180,0x9600,0x6900,0x8f00,0xdd80,0xd880,0x9300,0xd200,0xc780,0x7080,0x1d00,0x3680,0x3500,0x8000,0xc180,0x5600};
static void (*real_sink)(const SceneEvent *);
typedef struct Sample { uint16_t x,y,frame; uint8_t valid,projected,asleep; } Sample;
static Sample previous[86];
static unsigned sample_sim;
static int projected(unsigned source,unsigned first,unsigned count)
{
    for (unsigned i=0;i<count;i++) { unsigned a=DS_RenderSlots+(first+i)*18; if (DW(a+4)!=0xffff && DW(a+9)==source) return 1; }
    return 0;
}
static void watch(const SceneEvent *e)
{
    real_sink(e);
    if (e->kind==SCENE_GAME_LEVEL || e->kind==SCENE_GAME_CUT) { memset(previous,0,sizeof previous); sample_sim=~0u; }
    if (e->kind!=SCENE_GAME_SPRITES_BEGIN || sample_sim==pre2_frames_run || DB(DS_CurrentLevel)!=target) return;
    sample_sim=pre2_frames_run;
    for (unsigned i=0;i<86;i++) {
        unsigned a=i<70 ? 0x8f1d+i*7 : 0x9107+(i-70)*15;
        Sample s={DW(a),DW(a+2),DW(a+4),0,0,0}; s.valid=s.frame!=0xffff;
        s.projected=(uint8_t)projected(a,i<70?55:91,i<70?20:7);
        s.asleep=i<70 || ((DB(a+6)&15)!=8 && !DB(a+14) && (int8_t)DB(a+7)>=0 && !(DB(a+6)&0xc0));
        Sample p=previous[i];
        if (s.valid && p.valid && s.asleep && p.asleep && s.frame==p.frame) {
            if (!s.projected && !p.projected) { assert(s.x==p.x && s.y==p.y); source_checks++; if (i<70) float_checks++; else terrain_checks++; }
            if (s.projected && !p.projected) {
                assert(s.x==p.x);
                if (i<70) { int delta=(int16_t)(s.y-p.y); assert(delta>=-3 && delta<=3); activation_y_steps+=delta!=0; }
                else { assert(s.y==p.y); terrain_activations++; }
            }
        }
        previous[i]=s;
    }
}
static void bmp(const char *path,const uint32_t *p,int w,int h)
{
    FILE *file=fopen(path,"wb"); assert(file); uint8_t hd[54]={'B','M'};
    uint32_t size=54+(uint32_t)w*h*4; int32_t height=-h;
    memcpy(hd+2,&size,4); hd[10]=54; hd[14]=40; memcpy(hd+18,&w,4); memcpy(hd+22,&height,4); hd[26]=1; hd[28]=32;
    assert(fwrite(hd,1,54,file)==54 && fwrite(p,4,(size_t)w*h,file)==(size_t)w*h); fclose(file);
}
static uint32_t rgb(const uint8_t *c)
{
    return 0xff000000u | ((uint32_t)((c[0]<<2)|(c[0]>>4))<<16) | ((uint32_t)((c[1]<<2)|(c[1]>>4))<<8) | (c[2]<<2) | (c[2]>>4);
}
static void map_shot(void)
{
    const EnhLevel *a=frame.game.assets;
    int height=a->rows*4;
    for (int y=0;y<height;y++) for (int x=0;x<1024;x++) {
        unsigned t=frame.game.map->rows[y/4][x/4];
        unsigned pixel=a->tiles[t][(y%4*4+2)*16+x%4*4+2];
        if (a->tile_class[t]==1) pixel=0;
        map_pixels[y*1024+x]=rgb(frame.palette[pixel&frame.pel_mask]);
    }
    char path[128]; snprintf(path,sizeof path,"build/regions_shots/map_%02x.bmp",target); bmp(path,map_pixels,1024,height);
}
static void save_view(unsigned cell,const FrameGameView *v)
{
    Pre2PresentWindow win=pre2_present_window(v,v->camera_x-(W-320)/2.0,v->camera_y,W,176);
    EnhImage im={0}; im.index=index; im.capacity=sizeof index; im.stride=W;
    EnhOptions opt={ENH_BACK_BLACK,ENH_HUD_CENTRE,NULL,0};
    assert(!enh_render(v,v->assets,frame.palette,frame.pel_mask,win.x,win.y,W,H,&opt,&im));
    for (int y=0;y<H;y++) for (int x=0;x<W;x++) {
        uint32_t c=rgb(frame.palette[index[y*W+x]&frame.pel_mask]);
        if (y<176 && (x+(int)floor(win.x)<win.visible.x0 || x+(int)floor(win.x)>=win.visible.x1 ||
                      y+(int)floor(win.y)<win.visible.y0 || y+(int)floor(win.y)>=win.visible.y1)) c=0xff000000;
        pixels[y*W+x]=c;
        sheet[((cell/4)*(H+8)+8+y)*W*4+(cell%4)*W+x]=c;
    }
    for (unsigned b=0;b<5;b++) for (int y=1;y<6;y++) for (int x=1;x<5;x++)
        sheet[((cell/4)*(H+8)+y)*W*4+(cell%4)*W+b*7+x]=((cell+1)&(1u<<b))?0xffffffff:0xff444444;
    char path[128]; snprintf(path,sizeof path,"build/regions_shots/view_%02u.bmp",cell); bmp(path,pixels,W,H);
}
static void margin_contact(void)
{
    FrameGameView v=frame.game;
    for (unsigned i=0;i<16;i++) {
        unsigned source=0x9107+i*15;
        if (DW(source+4)==0xffff || (DB(source+6)&15)==8 || DB(source+14) || (int8_t)DB(source+7)<0 || (DB(source+6)&0xc0)) continue;
        v.camera_x=(int16_t)DW(source)-350; if (v.camera_x<0) continue;
        v.camera_y=(int16_t)DW(source+2)-100; if (v.camera_y<0) v.camera_y=0;
        v.sprite_count=v.dot_count=v.foreground_count=0;memset(v.paint,0,sizeof v.paint);
        enh_margin_capture(&v);assert(v.sprite_count);
        save_view(23,&v);
        static uint8_t with_margin[W*H];memcpy(with_margin,index,sizeof with_margin);
        v.sprite_count=0;
        Pre2PresentWindow win=pre2_present_window(&v,v.camera_x-(W-320)/2.0,v.camera_y,W,176);
        EnhImage im={0};im.index=index;im.capacity=sizeof index;im.stride=W;
        EnhOptions opt={ENH_BACK_BLACK,ENH_HUD_CENTRE,NULL,0};
        assert(!enh_render(&v,v.assets,frame.palette,frame.pel_mask,win.x,win.y,W,H,&opt,&im));
        unsigned changed=0;for (unsigned p=0;p<W*176;p++) changed+=with_margin[p]!=index[p];
        assert(changed);printf("constructed margin contact: shipped sleeping source %u contributes %u pixels\n",i,changed);
        return;
    }
    assert(!"no sleeping source for constructed margin contact");
}
static void reveal_contacts(void)
{
    static uint8_t cells[38][256]; static EnhMap map;
    memcpy(cells,frame.game.assets->map,sizeof cells);
    for (unsigned y=0;y<38;y++) map.rows[y]=cells[y];
    FrameGameView v=frame.game; v.map=&map;v.camera_x=40*16;v.camera_y=144;
    v.sprite_count=v.dot_count=v.foreground_count=0;memset(v.paint,0,sizeof v.paint);
    unsigned count;const Pre2RevealRegion *regions=pre2_region_reveals(&count);
    for (unsigned phase=0;phase<=6;phase++) {
        if (phase==0 || phase==1 || phase==4 || phase==6) save_view(phase==0?19:phase==1?20:phase==4?21:22,&v);
        for (unsigned k=0;k<count;k++) {
            const Pre2RevealRegion *r=&regions[k];
            unsigned col=r->source&255, row=r->source>>8;
            if (phase>=r->steps) continue;
            unsigned top=row-phase;
            for (unsigned y=0;y<r->rows;y++) for (unsigned x=0;x<r->columns;x++) cells[top-1+y][col+x]=cells[top+y][col+x];
            for (unsigned x=0;x<r->columns;x++) cells[top+r->rows-1][col+x]=frame.game.assets->map[(row+r->rows-1-phase)*256+col+x];
            assert(pre2_region_reveal_phase(&v,k)==(int)phase+1);
        }
    }
}
static void data_sweep(void)
{
    char name[32]; snprintf(name,sizeof name,"LEVEL%c.SQZ",target<9?'1'+target:'A'+target-9);
    uint8_t *compressed,*data; size_t n,size;
    assert(!asset_read(name,&compressed,&n) && !sqz_decode(compressed,n,&data,&size)); free(compressed);
    unsigned state=state_offsets[target]; assert(size>=state+0x13a5);
    unsigned count; const Pre2CaveRegion *c=pre2_region_caves(&count);
    unsigned found=0;
    for (unsigned k=0;k<20;k++) {
        const uint8_t *p=data+state+0x509+k*7;
        unsigned source=p[0]|p[1]<<8, camera=p[2]|p[3]<<8, destination=p[4]|p[5]<<8;
        if (source==0xffff) continue;
        unsigned matches=0;
        for (unsigned i=0;i<count;i++) if (c[i].level==target && c[i].source==source && c[i].camera==camera && c[i].destination==destination && c[i].gate==p[6]) matches++;
        assert(matches==1); found++;
    }
    unsigned authored=0; for (unsigned i=0;i<count;i++) authored+=c[i].level==target;
    assert(authored==found);
    /* Sweep every map cell: no restricted presentation rectangle may admit the
     * graphics bank, tower gap, or columns beside the vertical level-F band. */
    FrameGameView v={0}; v.level=(uint8_t)target; v.level_flags=data[state+0x308]; v.camera_limit_column=236;
    Pre2WorldRect b=pre2_region_for_view(&v).bounds;
    unsigned right=((data[state+0x306]|data[state+0x307]<<8)+20)*16;
    assert(b.x1==(int)(target==5?320:right));
    assert(b.y1==(int)(target==13?336:rows[target]*16));
    assert(frame.game.assets->rows==rows[target]);
    if (target==5) {
        FrameGameView lower=v;lower.level_flags=0;lower.camera_x=35*16;
        Pre2WorldRect band=pre2_region_for_view(&lower).bounds;
        assert(band.x0==35*16 && band.x1==(int)right && band.y0==0 && band.y1==128*16);
    }
    Pre2WorldRect never[7]; unsigned forbidden=pre2_region_never_visible(&v,never);
    unsigned cells=0,excluded=0;
    for (unsigned y=0;y<rows[target];y++) for (unsigned x=0;x<256;x++) {
        int wx=(int)x*16+8,wy=(int)y*16+8; cells++;
        int hidden=0;
        for (unsigned i=0;i<forbidden;i++) hidden|=wx>=never[i].x0 && wx<never[i].x1 && wy>=never[i].y0 && wy<never[i].y1;
        if (((target==9 || target==14 || target==5) && x>=20) || (target==13 && y>=21)) assert(hidden);
        if (hidden) {
            excluded++;
            /* For each forbidden map cell, sweep representative requested
             * cameras, all three wider windows and both ordinary/tall views. */
            for (unsigned k=0;k<3;k++) for (unsigned tall=0;tall<2;tall++) {
                Pre2PresentWindow win=pre2_present_window(&v,wx-widths[k]/2,wy-88,widths[k],tall?300:176);
                assert(wx<win.visible.x0 || wx>=win.visible.x1 || wy<win.visible.y0 || wy>=win.visible.y1);
            }
        }
    }
    assert(b.x0==0 && b.x1<=4096);
    printf("level %02x exclusions: %u forbidden cells tested against normal/tall windows\n",target,excluded);
    printf("level %02x data: %u cells, %u cave records matched\n",target,cells,found);
    free(data);
}
static void service(void)
{
    script_service();
    if (!selected && !strcmp(pre2_phase,"ShowWorldMapScrollIn()")) { SET_DB(DS_CurrentLevel,target<10?target:0); SET_DB(DS_byte_B198,1); selected=1; }
    if (pre2_frames_run && !first_game) first_game=rt_now();
    if (target>=10 && selected==1 && first_game && rt_now()-first_game>2000000000ull) { SET_DB(DS_WarpLevelTable,target); SET_DB(DS_LevelEndMode,2); selected=2; }
    if (!target && first_game && selected==1 && rt_now()-first_game>2000000000ull) { SET_DB(DS_RespawnState,1); SET_DB(DS_Energy,0); selected=2; }
}
static void display(uint64_t t,uint16_t start)
{
    enh_margin_enable(0); compositor_publish(t,start,&frame);
    if (!frame.game.valid || !frame.game.world_valid || frame.game.level!=target) return;
    reached++;
    original=frame.game;
    memcpy(memory,pre2_mem,sizeof memory); RtStats stats=*rt_stats(); uint64_t ma=vga_mem_accesses,io=vga_io_accesses;
    enh_margin_enable(1); enh_margin_capture(&frame.game);
    assert(!memcmp(memory,pre2_mem,sizeof memory) && !memcmp(&stats,rt_stats(),sizeof stats));
    assert(ma==vga_mem_accesses && io==vga_io_accesses);
    trimmed=frame.game; trimmed.sprite_count=original.sprite_count;
    memset(trimmed.sprites+original.sprite_count,0,(FRAME_MAX_SPRITES-original.sprite_count)*sizeof(FrameSprite));
    memset(original.sprites+original.sprite_count,0,(FRAME_MAX_SPRITES-original.sprite_count)*sizeof(FrameSprite));
    assert(!memcmp(&trimmed,&original,sizeof trimmed)); pure++;
    for (unsigned i=0;i<frame.game.sprite_count;i++) for (unsigned j=0;j<i;j++) assert(frame.game.sprites[i].id!=frame.game.sprites[j].id);
    if (frame.game.sim_frame==last_sim || frame.transition.kind) return;
    last_sim=frame.game.sim_frame;
    if (!checked++) { map_shot(); data_sweep(); }
    Pre2AuthoredRegion region=pre2_region_for_view(&frame.game);
    if (!region.proven_extent) unresolved++;
    assert(region.bounds.x0<=frame.game.camera_x && region.bounds.x1>=frame.game.camera_x+320);
    assert(region.bounds.y0<=frame.game.camera_y && region.bounds.y1>=frame.game.camera_y+176); covered++;
    EnhOptions opt={ENH_BACK_BLACK,ENH_HUD_CENTRE,NULL,0}; EnhImage im={0}; im.index=index; im.capacity=sizeof index; im.stride=320;
    assert(!enh_render(&frame.game,frame.game.assets,frame.palette,frame.pel_mask,frame.game.camera_x,frame.game.camera_y,320,200,&opt,&im));
    assert(!memcmp(index,frame.index,64000)); parity++;
    uint8_t regular[86]={0},margin[86]={0},visible[86][3]={{0}};
    for (unsigned i=0;i<frame.game.sprite_count;i++) {
        const FrameSprite *s=&frame.game.sprites[i]; unsigned ordinal=86;
        if (s->margin_only) ordinal=s->slot-0xf000u;
        else if ((s->slot>=55 && s->slot<75) || (s->slot>=91 && s->slot<98)) {
            unsigned source=DW(DS_RenderSlots+s->slot*18+9);
            if (source>=0x8f1d && source<0x9107 && !((source-0x8f1d)%7)) ordinal=(source-0x8f1d)/7;
            else if (source>=0x9107 && source<0x91f7 && !((source-0x9107)%15)) ordinal=70+(source-0x9107)/15;
        }
        if (ordinal>=86) continue;
        regular[ordinal]|=!s->margin_only; margin[ordinal]|=s->margin_only;
    }
    unsigned score=0;
    for (unsigned o=0;o<86;o++) {
        unsigned a=o<70?0x8f1d+o*7:0x9107+(o-70)*15;
        for (unsigned k=0;k<3;k++) {
            Pre2PresentWindow win=pre2_present_window(&frame.game,frame.game.camera_x-(widths[k]-320)/2.0,frame.game.camera_y,widths[k],176);
            int x=(int16_t)DW(a), y=(int16_t)DW(a+2);
            visible[o][k]=DW(a+4)!=0xffff && x>=win.visible.x0 && x<win.visible.x1 && y>=win.visible.y0 && y<win.visible.y1;
            if (!frame.game.camera_cut && regular[o] && !old_regular[o] && old_visible[o][k]) {
                pop_before[k]++; if (!old_margin[o][k]) pop_after[k]++;
            }
            if (k==1 && visible[o][k] && margin[o]) score++;
            old_visible[o][k]=visible[o][k]; old_margin[o][k]=visible[o][k]&&margin[o];
        }
        old_regular[o]=regular[o];
    }
    if ((!shot_taken || score>shot_score) && frame.game.sim_frame>20) { save_view(target,&frame.game); shot_score=score; shot_taken=1; }
    if (!target && checked==2) {
        margin_contact();
        FrameGameView v=frame.game; v.camera_x=220*16;v.camera_y=0;v.room=1;v.sprite_count=v.dot_count=v.foreground_count=0;memset(v.paint,0,sizeof v.paint);save_view(16,&v);
    }
    if (target==13 && checked==2) reveal_contacts();
    if (target==5 && checked==2) {
        FrameGameView v=frame.game;v.camera_x=35*16;v.camera_y=114*16;v.level_flags=0;v.sprite_count=v.dot_count=v.foreground_count=0;memset(v.paint,0,sizeof v.paint);save_view(17,&v);
        v.camera_x=46*16;save_view(18,&v);
    }
}
int main(int argc,char **argv)
{
#ifdef _WIN32
    _mkdir("build/regions_shots");
#else
    mkdir("build/regions_shots",0777);
#endif
    for (unsigned i=0;i<sizeof sheet/sizeof *sheet;i++) sheet[i]=0xff202020;
    for (target=0;target<16;target++) {
        reached=checked=selected=shot_score=shot_taken=0;last_sim=sample_sim=~0u;first_game=0;pre2_frames_run=0;
        memset(previous,0,sizeof previous);memset(old_regular,0,sizeof old_regular);memset(old_visible,0,sizeof old_visible);memset(old_margin,0,sizeof old_margin);
        memset(pop_before,0,sizeof pop_before);memset(pop_after,0,sizeof pop_after);
        script_reset("none");rt_boot(arg_assets(argc,argv),1995,6,1);vga_memory_off=1;compositor_init();real_sink=pre2_scene_sink;pre2_scene_sink=watch;
        RtConfig cfg={0};cfg.audio_native=1;rt_on_service=service;rt_on_display_frame=display;rt_on_pcm=NULL;assert(!rt_init(&cfg));
        for (uint64_t t=0;t<50ull*RT_NS_PER_SEC;t+=RT_NS_PER_SEC/60)
            if (rt_run_until(t)!=RT_STEP_DONE || (first_game && t>first_game && t-first_game>12000000000ull)) break;
        assert(reached && checked && shot_taken);
        printf("level %02x: %u views; source pop-ins 356 %u->%u, 468 %u->%u, 640 %u->%u\n",target,checked,pop_before[0],pop_after[0],pop_before[1],pop_after[1],pop_before[2],pop_after[2]);
        pre2_scene_sink=NULL;vga_memory_off=0;
    }
    bmp("build/regions_shots/contact.bmp",sheet,W*4,(H+8)*6);
    FILE *manifest=fopen("build/regions_shots/manifest.txt","w");assert(manifest);
    fprintf(manifest,"Cells 1..16: levels 00h..0Fh, scripted gameplay at 468px; maximum observed eligible margin sources.\n17: constructed level 00h locked cave 00DC.\n18,19: constructed level 05h lower encounter at cameras 35 and 46.\n20..23: constructed level 0Dh reveal map phases 0/1/4/6, all triggers advanced together.\n24: constructed camera beside a shipped sleeping terrain source, margin capture enabled.\nMap images: full backing maps at quarter scale, NOT playable bounds.\n");fclose(manifest);
    printf("regions_sweep: %u pure captures (full memory, machine/bus counters, regular semantic view); %u exact 320 frames; %u covered camera views; %u unresolved outer extents; %u frozen-source measurements; %u float activation bounce steps\n",pure,parity,covered,unresolved,source_checks,activation_y_steps);
    printf("allowlist measurements: %u omitted floats, %u sleeping terrain samples, %u unchanged sleeping-terrain activations; floats rejected after %u Y-changing activations\n",float_checks,terrain_checks,terrain_activations,activation_y_steps);
    return 0;
}
