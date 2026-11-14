/* Exercise only the translated reveal routine with constructed state. No game
 * boot/play: compare each map phase to the immutable source-region annotation. */
#include "script.h"
#include "c/level.h"
#include "c/render.h"
#include "native/assetfs.h"
#include "native/sqz.h"
#include "native/level_regions.h"
#include "native/enh_render.h"
#include <assert.h>
static EnhLevel assets;
static EnhMap map;
static FrameGameView view;
static uint8_t current[38*256];
static unsigned word(const uint8_t *p) { return p[0]|p[1]<<8; }
int main(int argc,char **argv)
{
    asset_set_dir(arg_assets(argc,argv));
    uint8_t *in,*data;size_t n,size;assert(!asset_read("LEVELE.SQZ",&in,&n) && !sqz_decode(in,n,&data,&size));free(in);
    assert(size>=0x8000+0x13a5);
    assets.level=view.level=13;assets.rows=38;memcpy(assets.map,data,38*256);view.assets=&assets;view.map=&map;view.camera_y=160;
    unsigned count;const Pre2RevealRegion *regions=pre2_region_reveals(&count);assert(count==3);
    unsigned checks=0;
    for (unsigned selected=0;selected<count;selected++) {
        memset(pre2_mem,0,0x100000);SET_DW(DS_LevelSegment,0x4000);SET_DW(DS_NextFreeSegment,0x5000);
        SET_DB(DS_CurrentLevel,13);SET_DW(DS_CameraCol,220);SET_DW(DS_CameraRow,0);SET_DB(DS_SnowFrameTick,0);
        for (unsigned a=0;a<38*256;a++) wr8(0x4000,(uint16_t)a,data[a]);
        for (unsigned a=0;a<150;a++) SET_DB(0x83f3+a,data[0x8000+0x595+a]);
        for (unsigned k=0;k<15;k++) if (word(data+0x8000+0x595+k*10)!=0xffff) {
            unsigned i=k-12;assert(i<count);
            assert(DW(0x83f3+k*10)==regions[i].source && DW(0x83f3+k*10+4)==regions[i].trigger);
            assert(DB(0x83f3+k*10+2)==regions[i].columns && DB(0x83f3+k*10+3)==regions[i].rows && DB(0x83f3+k*10+8)==regions[i].steps);
        }
        SavePristineTriggerRows();
        for (unsigned k=0;k<15;k++) SET_DW(0x83f3+k*10+4,0xffff);
        unsigned record=0x83f3+(12+selected)*10;SET_DW(record+4,0xfffe);
        for (unsigned phase=0;phase<=regions[selected].steps;phase++) {
            for (unsigned a=0;a<38*256;a++) current[a]=rd8(0x4000,(uint16_t)a);
            for (unsigned y=0;y<38;y++) map.rows[y]=current+y*256;
            assert(pre2_region_reveal_phase(&view,selected)==(int)phase);
            Pre2WorldRect tail=pre2_region_staging(&view,selected);
            assert(tail.y0==336 && tail.y1==(19+regions[selected].rows)*16);
            Pre2PresentWindow w=pre2_present_window(&view,0,220,640,300);
            assert(w.visible.y1==336 && w.y==36); /* taller view, no hidden pillar tails */
            checks++;
            if (phase<regions[selected].steps) UpdateProximityTriggers();
        }
        assert(DW(record+4)==0xffff && DB(record+8)==0);
    }
    printf("regions_reveal: %u exact map phases from translated UpdateProximityTriggers; all three final bases at world y336; no 149px camera cap\n",checks);
    free(data);return 0;
}
