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
static void setup(const uint8_t *data)
{
    memset(pre2_mem,0,0x100000);SET_DW(DS_LevelSegment,0x4000);SET_DW(DS_NextFreeSegment,0x5000);
    SET_DB(DS_CurrentLevel,13);SET_DW(DS_CameraCol,220);SET_DW(DS_CameraRow,0);SET_DB(DS_SnowFrameTick,0);
    for (unsigned a=0;a<38*256;a++) wr8(0x4000,(uint16_t)a,data[a]);
    for (unsigned a=0;a<150;a++) SET_DB(0x83f3+a,data[0x8000+0x595+a]);
    SavePristineTriggerRows();
}
static void snapshot(void)
{
    for (unsigned a=0;a<38*256;a++) current[a]=rd8(0x4000,(uint16_t)a);
    for (unsigned y=0;y<38;y++) map.rows[y]=current+y*256;
}
static void visibility(unsigned index,unsigned phase,const Pre2RevealRegion *r)
{
    Pre2RevealVisibility v=pre2_region_reveal_visibility(&view,index);
    assert(v.phase==(int)phase);
    unsigned col=r->source&255, row=r->source>>8, bottom=row+r->rows-phase;
    assert(v.visible_copy.x0==(int)col*16 && v.visible_copy.x1==(int)(col+r->columns)*16);
    assert(v.visible_copy.y0==(int)(row-phase)*16 && v.visible_copy.y1==336);
    assert(v.never_copy.y0==336 && v.never_copy.y1==(int)bottom*16);
    assert(v.never_restored.y0==(int)bottom*16 && v.never_restored.y1==(int)(row+r->rows)*16);
    Pre2WorldRect bank=pre2_region_staging(&view,index);
    assert(v.never_copy.y0==bank.y0 && v.never_restored.y1==bank.y1);
    /* Compare both visible and hidden portions of the moving copy to its
     * original rows; the rows behind it must be restored at their OLD sites. */
    for (unsigned y=row-phase;y<bottom;y++) for (unsigned x=col;x<col+r->columns;x++)
        assert(current[y*256+x]==assets.map[(y+phase)*256+x]);
    for (unsigned y=bottom;y<row+r->rows;y++) for (unsigned x=col;x<col+r->columns;x++)
        assert(current[y*256+x]==assets.map[y*256+x]);
}
int main(int argc,char **argv)
{
    asset_set_dir(arg_assets(argc,argv));
    uint8_t *in,*data;size_t n,size;assert(!asset_read("LEVELE.SQZ",&in,&n) && !sqz_decode(in,n,&data,&size));free(in);
    assert(size>=0x8000+0x13a5);
    assets.level=view.level=13;assets.rows=38;memcpy(assets.map,data,38*256);view.assets=&assets;view.map=&map;view.camera_y=160;
    assets.columns=256;view.view_w=320;view.view_h=176;view.camera_limit_column=(uint16_t)word(data+0x8000+0x306);
    unsigned count;const Pre2RevealRegion *regions=pre2_region_reveals(&count);assert(count==3);
    for (unsigned i=0;i<count;i++) {
        const uint8_t *p=data+0x8000+0x595+(12+i)*10;
        assert(word(p)==regions[i].source && word(p+4)==regions[i].trigger);
        assert(p[2]==regions[i].columns && p[3]==regions[i].rows && p[8]==regions[i].steps);
    }
    unsigned combinations=0,checks=0,broken_floor_states=0;
    /* Stagger trigger starts so each combination is reached without pausing
     * or rewriting an in-progress countdown. The original routine owns every
     * map shift, bank read, countdown and completion transition. */
    for (unsigned a=0;a<=4;a++) for (unsigned b=0;b<=4;b++) for (unsigned c=0;c<=6;c++) {
        const unsigned phase[]={a,b,c};unsigned ticks=a>b?a:b;if(c>ticks) ticks=c;
        setup(data);
        for (unsigned t=0;t<ticks;t++) {
            for (unsigned i=0;i<count;i++) if (phase[i] && t==ticks-phase[i]) SET_DW(0x83f3+(12+i)*10+4,0xfffe);
            UpdateProximityTriggers();
        }
        snapshot();
        int broken_floor=0;
        for(unsigned x=0;x<256;x++) broken_floor|=data[0x8000+0x100+current[19*256+x]]!=1;
        broken_floor_states+=broken_floor;
        uint64_t before=fnv64(pre2_mem,0x100000,FNV0);
        for (unsigned i=0;i<count;i++) {
            visibility(i,phase[i],&regions[i]);checks++;
            unsigned rec=0x83f3+(12+i)*10;
            assert(DW(rec)==regions[i].source-phase[i]*256 && DB(rec+8)==regions[i].steps-phase[i]);
            assert(DW(rec+4)==(phase[i]==regions[i].steps?0xffff:phase[i]?0xfffe:regions[i].trigger));
        }
        assert(before==fnv64(pre2_mem,0x100000,FNV0));
        Pre2PresentWindow w=pre2_present_window(&view,0,220,640,300);
        assert(w.visible.y1==336 && w.y==36);
        combinations++;
    }
    setup(data);snapshot();unsigned cell=regions[0].source;current[cell]^=255;
    Pre2RevealVisibility unknown=pre2_region_reveal_visibility(&view,0);
    Pre2WorldRect bank=pre2_region_staging(&view,0);
    assert(unknown.phase==-1 && unknown.visible_copy.y0==unknown.visible_copy.y1);
    assert(!memcmp(&unknown.never_copy,&bank,sizeof bank) && unknown.never_restored.y0==unknown.never_restored.y1);
    assert(broken_floor_states==167);
    printf("regions_reveal: %u independently staggered joint phases; %u exact moving/restored silhouettes; %u states invalidate the phase-blind class-1 floor; pure map-only phase reads; malformed phase stays hidden\n",combinations,checks,broken_floor_states);
    free(data);return 0;
}
