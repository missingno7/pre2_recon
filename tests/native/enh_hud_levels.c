/* Extend the established sixteen-level/death schedules with whole HUD and
 * destructive-page parity. Poisoning the strip proves it is reference-only. */
#include "script.h"
#include "native/compositor.h"
#include "native/presenter.h"
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#endif
static unsigned hud_checks, surface_checks, native_checks, hud_field_checks;
static unsigned hud_iris_margin_checks;
static Pre2Frame hud_poisoned;
static uint8_t hud_image[64000];
static uint32_t hud_rgb[640*200];
static Presenter *hud_presenters[6];
static uint64_t hud_last_time;
static unsigned backdrop_seen[16];
static int hud_previous_eligible;
static unsigned hud_shots[5];
static unsigned hud_world_shots[16];
static uint8_t hud_curtain_index[640*200],hud_curtain_level;
static uint32_t hud_curtain_rgb[640*200];
static int hud_curtain_pending;
static void hud_write_bmp(const char *name,const uint32_t *pixels,int w,int h)
{
    char path[160]; snprintf(path,sizeof path,"build/gamehud_shots/%s.bmp",name);
    FILE *file=fopen(path,"wb"); if(!file) exit(1);
    uint8_t hd[54]={'B','M'}; uint32_t size=54+(uint32_t)w*h*4; int32_t height=-h;
    memcpy(hd+2,&size,4); hd[10]=54; hd[14]=40; memcpy(hd+18,&w,4); memcpy(hd+22,&height,4); hd[26]=1; hd[28]=32;
    if(fwrite(hd,1,54,file)!=54 || fwrite(pixels,4,(size_t)w*h,file)!=(size_t)w*h) exit(1);
    fclose(file);
}
static void hud_publish(uint64_t t, uint16_t start, Pre2Frame *f)
{
    compositor_publish(t,start,f);
    int ready=f->game.valid && f->game.assets && !f->blank &&
        (f->game.world_valid || (f->game.surface && f->transition.kind>=1 && f->transition.kind<=3)) && f->transition.kind<=4;
    hud_poisoned=*f;
    if(ready) { memset(hud_poisoned.index,0xa5,sizeof hud_poisoned.index); memset(hud_poisoned.game.hud,0x5a,sizeof hud_poisoned.game.hud); }
    if(t<=hud_last_time) {
        hud_curtain_pending=0;
        for(unsigned i=0;i<6;i++) { presenter_destroy(hud_presenters[i]); hud_presenters[i]=NULL; }
    }
    hud_last_time=t;
    const int widths[]={320,356,640};
    for(unsigned i=0;i<6;i++) {
        if(!hud_presenters[i]) {
            PresenterOptions o=presenter_defaults(); o.mode=PRESENT_WIDESCREEN;
            o.aspect=widths[i%3]/200.0; o.smooth_transitions=i>=3;
            hud_presenters[i]=presenter_create(&o); if(!hud_presenters[i]) exit(1);
        }
        presenter_push(hud_presenters[i],&hud_poisoned);
    }
    int previous=hud_previous_eligible;
    hud_previous_eligible=ready;
    if(!hud_previous_eligible) return;
    FrameGameView v=f->game; memset(v.hud,0xa5,sizeof v.hud);
    EnhOptions opt={ENH_BACK_BLACK,ENH_HUD_ANCHORED,NULL,0};
    EnhImage im={0}; im.index=hud_image; im.capacity=sizeof hud_image; im.stride=320;
    if(enh_render(&v,v.assets,f->palette,f->pel_mask,v.camera_x,v.camera_y,320,200,&opt,&im)) { puts("HUD render rejected"); exit(1); }
    unsigned top=f->game.surface?0:176, diff=0;
    for(unsigned y=top;y<200;y++) for(unsigned x=0;x<320;x++) if(hud_image[y*320+x]!=f->index[y*320+x]) {
        if(diff++<4) fprintf(stderr,"native level %u sim %u kind %u step %u surface %u pixel %u,%u got %u expected %u\n",v.level,v.sim_frame,f->transition.kind,f->transition.step,!!v.surface,x,y,hud_image[y*320+x],f->index[y*320+x]);
    }
    if(diff) { fprintf(stderr,"%u differences, HUD elements %u\n",diff,v.hud_count); exit(1); }
    hud_checks++; if(v.surface) surface_checks++;
    /* Anchored at 640, every field (with its digits, pips, letters and label art) is the
     * 320 field moved as one piece, and the columns between fields are the plain bar
     * pattern (column 1 of the 320 strip). The score's seven cells must never split. */
    if(!v.surface) {
        static uint8_t wide[640*200];
        EnhImage wi={0}; wi.index=wide; wi.capacity=sizeof wide; wi.stride=640;
        if(enh_render(&v,v.assets,f->palette,f->pel_mask,v.camera_x-160,v.camera_y,640,200,&opt,&wi)) { puts("wide HUD render rejected"); exit(1); }
        const int off[HUD_FIELDS]={0,160,320,320};
        static uint8_t owner[640];
        memset(owner,0xff,sizeof owner);
        for(unsigned fl=0;fl<HUD_FIELDS;fl++) for(int x=frame_hud_field_x[fl];x<frame_hud_field_x[fl+1];x++) {
            owner[x+off[fl]]=(uint8_t)fl;
            for(unsigned y=176;y<200;y++) if(wide[y*640+x+off[fl]]!=hud_image[y*320+x]) {
                fprintf(stderr,"HUD field %u torn at x %d y %u (level %u)\n",fl,x,y,v.level); exit(1);
            }
        }
        for(int x=0;x<640;x++) if(owner[x]==0xff) for(unsigned y=176;y<199;y++) if(wide[y*640+x]!=hud_image[y*320+1]) {
            fprintf(stderr,"HUD gap column %d row %u is not the plain bar (level %u)\n",x,y,v.level); exit(1);
        }
        hud_field_checks++;
    }
    if(v.assets->level==v.level && !backdrop_seen[v.level]) {
        unsigned seam=0, seam8=0;
        for(unsigned y=0;y<200;y++) {
            seam+=v.assets->backdrop[y*320]!=v.assets->backdrop[y*320+319];
            for(unsigned x=0;x<8;x++) seam8+=v.assets->backdrop[y*320+x]!=v.assets->backdrop[y*320+312+x];
        }
        printf("backdrop %X period %u seam rows %u seam8 pixels %u\n",v.level,v.assets->backdrop_period,seam,seam8); backdrop_seen[v.level]=1;
        for(unsigned k=0;k<64000;k++) {
            const uint8_t *c=f->palette[v.assets->backdrop[k]&f->pel_mask];
            hud_rgb[k]=0xff000000u | (uint32_t)((c[0]<<2)|(c[0]>>4))<<16 | (uint32_t)((c[1]<<2)|(c[1]>>4))<<8 | ((c[2]<<2)|(c[2]>>4));
        }
        char name[32]; snprintf(name,sizeof name,"backdrop_%X",v.level); hud_write_bmp(name,hud_rgb,320,200);
    }
    for(unsigned i=0;i<6;i++) {
        PresenterImage pi={0}; pi.pixels=hud_rgb; pi.capacity=sizeof hud_rgb/4;
        uint64_t bound=t+(i>=3?PRESENTER_DELAY_NS:0);
        if(presenter_render(hud_presenters[i],bound,1.6,&pi) || pi.fallback) {
            fprintf(stderr,"presenter fallback %u width %d smooth %u kind %u level %u\n",pi.fallback,widths[i%3],i>=3,f->transition.kind,v.level); exit(1);
        }
        if(i%3==0) for(unsigned pixel=0;pixel<64000;pixel++) {
            const uint8_t *c=f->palette[f->index[pixel]&f->pel_mask];
            uint32_t rgb=0xff000000u | (uint32_t)((c[0]<<2)|(c[0]>>4))<<16 | (uint32_t)((c[1]<<2)|(c[1]>>4))<<8 | ((c[2]<<2)|(c[2]>>4));
            if(pi.pixels[pixel]!=rgb) { fprintf(stderr,"poisoned presenter differs at level %u kind %u step %u pixel %u\n",v.level,f->transition.kind,f->transition.step,pixel); exit(1); }
        }
        if(i==2 && f->transition.kind==1 && v.surface && f->transition.cy>=0 && f->transition.cy<176) {
            unsigned y=(unsigned)f->transition.cy;
            double edge=v.surface->outside_right[y]+v.camera_x-pi.camera_x;
            if((v.surface->outside_valid[y]&2) && edge+8<640) {
                const uint8_t *c=f->palette[0];
                uint32_t black=0xff000000u | (uint32_t)((c[0]<<2)|(c[0]>>4))<<16 | (uint32_t)((c[1]<<2)|(c[1]>>4))<<8 | ((c[2]<<2)|(c[2]>>4));
                if(pi.pixels[y*640+639]!=black) { fputs("iris leaked into the far margin\n",stderr); exit(1); }
                hud_iris_margin_checks++;
            }
        }
        unsigned lit=0;
        if(i==1 || i==2) for(int pixel=0;pixel<pi.w*176;pixel++) lit+=(pi.pixels[pixel]&0xffffff)>0x080808;
        int shot=-1;
        if(i==1 && !f->transition.kind && v.sim_frame>32 && v.assets->level==0) shot=0;
        if(i==2 && !f->transition.kind && v.sim_frame>32 && v.assets->level==0) shot=1;
        if(i==2 && f->transition.kind==1 && f->transition.radius<200 && f->transition.radius>40) shot=2;
        if(i==2 && f->transition.kind==2 && f->transition.step>=3 && f->transition.step<=8) shot=3;
        if(i==2 && f->transition.kind==3 && f->transition.step>=3 && f->transition.step<=7 && v.sim_frame>20) shot=4;
        if(lit<1000) shot=-1;
        if(shot>=0 && !hud_shots[shot]) {
            static const char *names[]={"hud356","hud640","iris640","curtain640","cave_wipe640"};
            hud_write_bmp(names[shot],pi.pixels,pi.w,pi.h); hud_shots[shot]=1;
        }
        /* The scripted entry curtains run while their DAC is black. Retain
         * native indices, then make an explicitly palette-normalized review
         * image only after that level's visible gameplay palette is observed. */
        if(i==2 && !hud_shots[3] && !hud_curtain_pending && f->transition.kind==2 && f->transition.step>=3 && f->transition.step<=8) {
            EnhImage review={0}; review.index=hud_curtain_index; review.stride=640; review.capacity=sizeof hud_curtain_index;
            if(!enh_render(&v,v.assets,f->palette,f->pel_mask,pi.camera_x,pi.camera_y,640,200,&opt,&review)) {
                unsigned ink=0; for(unsigned pixel=0;pixel<640*176;pixel++) ink+=hud_curtain_index[pixel]!=0;
                if(ink>1000) { hud_curtain_pending=1; hud_curtain_level=v.assets->level; }
            }
        }
        if(i==2 && hud_curtain_pending && !f->transition.kind && lit>=1000 && v.assets->level==hud_curtain_level) {
            for(unsigned pixel=0;pixel<640*200;pixel++) {
                const uint8_t *c=f->palette[hud_curtain_index[pixel]&f->pel_mask];
                hud_curtain_rgb[pixel]=0xff000000u | (uint32_t)((c[0]<<2)|(c[0]>>4))<<16 | (uint32_t)((c[1]<<2)|(c[1]>>4))<<8 | ((c[2]<<2)|(c[2]>>4));
            }
            hud_write_bmp("curtain640",hud_curtain_rgb,640,200); hud_shots[3]=1; hud_curtain_pending=0;
        }
        if(i==2 && lit>=1000 && !f->transition.kind && v.sim_frame>32 && v.assets->level==v.level && v.assets->backdrop_period && !hud_world_shots[v.level]) {
            char name[40]; snprintf(name,sizeof name,"backdrop_world_%X",v.level);
            hud_write_bmp(name,pi.pixels,pi.w,pi.h); hud_world_shots[v.level]=1;
        }
        if(i>=3 && previous && t>20000000) {
            if(!presenter_render(hud_presenters[i],bound-7000000,1.6,&pi) && pi.fallback) {
                fprintf(stderr,"interior fallback %u kind %u\n",pi.fallback,f->transition.kind); exit(1);
            }
        }
        native_checks++;
    }
}
#define compositor_publish hud_publish
#define main hud_level_main
#ifdef HUD_SCENARIOS
#include "enh_scenarios.c"
#else
#include "enh_levels.c"
#endif
#undef main
int main(int argc,char **argv)
{
#ifdef _WIN32
    _mkdir("build/gamehud_shots");
#else
    mkdir("build/gamehud_shots",0777);
#endif
    int rc=hud_level_main(argc,argv);
    for(unsigned i=0;i<6;i++) presenter_destroy(hud_presenters[i]);
    printf("enh_hud_levels: %u exact HUD frames (poisoned reference); %u exact native transition pages; %u zero-fallback presenter width/smoothing samples; %u anchored 640 HUDs with whole fields\n",hud_checks,surface_checks,native_checks,hud_field_checks);
#ifdef HUD_SCENARIOS
    printf("iris margin regression: %u original out-of-aperture boundaries close the wide edge\n",hud_iris_margin_checks);
    if(!hud_iris_margin_checks) return 1;
#endif
    return rc || !hud_checks || !surface_checks || !hud_field_checks;
}
