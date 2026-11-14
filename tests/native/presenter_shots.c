/* Bounded native schedules plus one explicitly constructed, audit-established
 * cave arrival. No original executable, interactive play or simulation widening. */
#include "script.h"
#include "c/vga.h"
#include "native/compositor.h"
#include "native/presenter.h"
#include <assert.h>
#undef assert
#define assert(expr) do { if (!(expr)) { fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#expr); exit(1); } } while (0)
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#endif

#define W 468
#define H 200
static Presenter *presenter;
static Pre2Frame f,cave;
static uint32_t pixels[PRESENTER_MAX_W*PRESENTER_MAX_H], sheet[W*3*(H+8)*3];
static uint32_t faithful[FRAME_MAX_W*FRAME_MAX_H];
static unsigned seen[7],target,selected,parity,interiors,iris_count;
static uint64_t first_game;
static const char *names[]={"level1","cave_constructed","gorilla","levelF","iris","fade","map_scroll"};
static void bmp(const char *path,const uint32_t *p,int w,int h)
{
    FILE *file=fopen(path,"wb"); assert(file);
    uint8_t hd[54]={'B','M'}; uint32_t size=54+(uint32_t)w*h*4; int32_t height=-h;
    memcpy(hd+2,&size,4); hd[10]=54; hd[14]=40; memcpy(hd+18,&w,4); memcpy(hd+22,&height,4); hd[26]=1; hd[28]=32;
    assert(fwrite(hd,1,54,file)==54); assert(fwrite(p,4,(size_t)w*h,file)==(size_t)w*h); fclose(file);
}
static void save(unsigned n,const PresenterImage *im)
{
    if (seen[n]) return;
    char path[256]; snprintf(path,sizeof path,"build/presenter_shots/%s.bmp",names[n]); bmp(path,im->pixels,im->w,im->h);
    int ox=(int)(n%3)*W, oy=(int)(n/3)*(H+8)+8;
    for (int y=0;y<H;y++) for (int x=0;x<W;x++)
        sheet[(oy+y)*W*3+ox+x]=im->pixels[(y*im->h/H)*im->w+x*im->w/W];
    /* Seven binary index marks above the cells identify the manifest entries. */
    for (unsigned bit=0;bit<3;bit++) for (int y=1;y<6;y++) for(int x=0;x<5;x++)
        sheet[(oy-8+y)*W*3+ox+2+(int)bit*7+x]=((n+1)&(1u<<bit))?0xffffffffu:0xff444444u;
    seen[n]=1;
}
static void service(void)
{
    script_service();
    if (!selected && !strcmp(pre2_phase,"ShowWorldMapScrollIn()")) {
        SET_DB(DS_CurrentLevel,target<10?target:0); SET_DB(DS_byte_B198,1); selected=1;
    }
    if (pre2_frames_run && !first_game) first_game=rt_now();
    if (target>=10 && selected==1 && first_game && rt_now()-first_game>2000000000ull) {
        SET_DB(DS_WarpLevelTable,target); SET_DB(DS_LevelEndMode,2); selected=2;
    }
}
static void make_cave(void)
{
    /* Audit: level 00h cave camera 00DCh, arrival 07E5h, gate 1.
     * Reproject only the captured player into that constructed immutable view.
     * This is a source-data contact view, not a claim to have walked the cave. */
    cave=f; cave.t_ns+=1000000000; cave.game.sim_frame+=10000;
    FrameGameView *v=&cave.game;
    FrameSprite player={0};
    for (unsigned i=0;i<v->sprite_count;i++) if(v->sprites[i].slot==1) player=v->sprites[i];
    int dx=player.screen_x-player.world_x+v->camera_x,dy=player.screen_y-player.world_y+v->camera_y;
    v->camera_x=220*16; v->camera_y=0; v->fine_scroll=0; v->room=1; v->camera_cut=1; v->shake_rows=0;
    player.world_x=229*16; player.world_y=7*16;
    player.screen_x=(int16_t)(player.world_x-v->camera_x+dx); player.screen_y=(int16_t)(player.world_y+dy);
    v->sprites[0]=player; v->sprite_count=1; v->dot_count=v->foreground_count=0;
    memset(v->paint,0,sizeof v->paint); memset(&cave.transition,0,sizeof cave.transition);
    PresenterOptions opt=presenter_defaults(); opt.mode=PRESENT_WIDESCREEN; opt.aspect=21.0/9;
    opt.interpolate=opt.smooth_camera=1;
    Presenter *p=presenter_create(&opt); assert(p); presenter_push(p,&cave);
    PresenterImage im={0}; im.pixels=pixels; im.capacity=sizeof pixels/4;
    assert(!presenter_render(p,cave.t_ns+PRESENTER_DELAY_NS,1.6,&im)); save(1,&im); presenter_destroy(p);
}
static void display(uint64_t t,uint16_t start)
{
    compositor_publish(t,start,&f); presenter_push(presenter,&f);
    PresenterImage im={0}; im.pixels=pixels; im.capacity=sizeof pixels/4;
    if (presenter_render(presenter,t+1000000,1.6,&im)) return;
    assert(im.w==468 || f.h!=200);
    if (im.alpha>0 && im.alpha<1) interiors++;
    if (f.transition.kind==1) iris_count++;
    if (!target && f.game.valid && f.game.level==0 && f.game.world_valid && !f.transition.kind && f.game.sim_frame>20) {
        if (im.alpha>0 && im.alpha<1) save(0,&im);
        if (!seen[1]) make_cave();
    }
    if (target==9 && f.game.valid && f.game.level==9 && !f.transition.kind && f.game.sim_frame>20) save(2,&im);
    if (target==14 && f.game.valid && f.game.level==14 && !f.transition.kind && f.game.sim_frame>20) save(3,&im);
    if (f.transition.kind==1 && im.radius<180 && im.radius>30 && !(im.fallback&2)) save(4,&im);
    if (f.transition.kind==4 && !seen[5]) {
        unsigned lit=0;
        for (int i=0;i<im.w*im.h;i++) if ((pixels[i]&0xffffff)>0x303030) lit++;
        if (lit>2000) save(5,&im);
    }
    if (f.transition.kind==5 && f.transition.step>350 && f.transition.step<500) save(6,&im);

    /* 320-wide enhancement parity is exercised for every eligible publication
     * in all three level schedules, independent of the contact image selection. */
    if (f.game.valid && f.game.world_valid && (!f.transition.kind || f.transition.kind==4)) {
        PresenterOptions opt=presenter_defaults(); opt.mode=PRESENT_ENHANCED;
        /* A persistent unsmoothed presenter is tested by presenter_purity. Here
         * compare the whole enhanced frame/HUD directly at real level 9/F too. */
        static uint8_t index[64000]; EnhImage ei={0}; ei.index=index; ei.capacity=sizeof index; ei.stride=320;
        EnhOptions eo={opt.backdrop,opt.hud,NULL,0};
        assert(!enh_render(&f.game,f.game.assets,f.palette,f.pel_mask,f.game.camera_x,f.game.camera_y,320,200,&eo,&ei));
        assert(!memcmp(index,f.index,64000));
        parity++;
    }
}
int main(int argc,char **argv)
{
    (void)faithful;
#ifdef _WIN32
    _mkdir("build/presenter_shots");
#else
    mkdir("build/presenter_shots",0777);
#endif
    for(unsigned i=0;i<sizeof sheet/sizeof *sheet;i++) sheet[i]=0xff202020;
    const unsigned levels[]={0,9,14};
    for(unsigned k=0;k<3;k++) {
        target=levels[k]; selected=0; first_game=0; pre2_frames_run=0;
        script_reset(k==0?"castle":"none"); rt_boot(arg_assets(argc,argv),1995,6,1); vga_memory_off=1; compositor_init();
        PresenterOptions opt=presenter_defaults(); opt.mode=PRESENT_WIDESCREEN; opt.aspect=21.0/9;
        opt.interpolate=opt.smooth_camera=opt.smooth_transitions=1;
        presenter=presenter_create(&opt); assert(presenter);
        RtConfig cfg={0}; cfg.audio_native=1; rt_on_service=service; rt_on_display_frame=display; rt_on_pcm=NULL;
        assert(!rt_init(&cfg));
        for(uint64_t t=0;t<60ull*RT_NS_PER_SEC;t+=RT_NS_PER_SEC/60)
            if(rt_run_until(t)!=RT_STEP_DONE || (first_game && t>first_game && t-first_game>15000000000ull)) break;
        presenter_destroy(presenter); pre2_scene_sink=NULL; vga_memory_off=0;
    }
    bmp("build/presenter_shots/contact.bmp",sheet,W*3,(H+8)*3);
    FILE *manifest=fopen("build/presenter_shots/manifest.txt","w"); assert(manifest);
    for(unsigned i=0;i<7;i++) { fprintf(manifest,"%u: %s%s\n",i+1,names[i],seen[i]?"":" MISSING"); printf("shot %s: %s\n",names[i],seen[i]?"written":"MISSING"); }
    fclose(manifest);
    printf("presenter_shots: %u exact 320x200 gameplay/HUD frames; %u interpolation interiors; %u iris publications\n",parity,interiors,iris_count);
    for(unsigned i=0;i<7;i++) if(!seen[i]) return 1;
    return !parity || !interiors;
}
