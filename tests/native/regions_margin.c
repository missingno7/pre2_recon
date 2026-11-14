/* Constructed source observation: only proven sleeping terrain, exact margin
 * clipping, rear ordering, source identity and rejection of moving/float types. */
#include "script.h"
#include "c/view.h"
#include "c/level.h"
#include "native/enh_margin.h"
#include "native/enh_render.h"
#include <assert.h>
static EnhLevel assets;
static EnhCel cels[1024];
static EnhMap map;
static FrameGameView view;
static uint8_t blocks[2][65536],cells[256],image[468*200],memory[0x100000];
static int overrides;
static void source(int x)
{
    SET_DW(0x9107,x);SET_DW(0x9109,100);SET_DW(0x910b,0);SET_DB(0x910d,2);SET_DB(0x910e,2);SET_DB(0x9115,0);
    view.sprite_count=0;
}
static void render(void)
{
    uint8_t palette[256][3]={{0}};EnhImage im={0};im.index=image;im.capacity=sizeof image;im.stride=468;
    EnhPosition unused={.id=12345,.x=0,.y=0};
    EnhOptions options={ENH_BACK_BLACK,ENH_HUD_CENTRE,overrides?&unused:NULL,overrides?1:0};
    assert(!enh_render(&view,&assets,palette,255,0,0,468,200,&options,&im));
}
int main(void)
{
    memset(pre2_mem,0,0x100000);assert(!pre2_view_configure(0));
    for (unsigned i=0;i<16;i++) SET_DW(0x9107+i*15+4,0xffff);
    for (unsigned i=0;i<70;i++) SET_DW(0x8f1d+i*7+4,0xffff);
    for (unsigned i=0;i<2;i++) {
        cels[i]=(EnhCel){16,8,2,16,blocks[i],NULL,NULL};memset(blocks[i],255,16);memset(blocks[i]+(i+1)*16,255,16);
        SET_DW(DS_FrameWidths+i*2,0x0810);SET_DB(DS_FrameHotX+i*2,0);SET_DB(DS_FrameHotY+i*2,0);
    }
    for (unsigned y=0;y<256;y++) map.rows[y]=cells;
    assets.rows=49;assets.cels=cels;assets.policy=enh_level_policy(0);
    view.valid=view.world_valid=1;view.assets=&assets;view.map=&map;view.level_epoch=7;view.view_w=320;view.view_h=176;
    source(316);memcpy(memory,pre2_mem,sizeof memory);enh_margin_capture(&view);
    assert(!memcmp(memory,pre2_mem,sizeof memory));assert(view.sprite_count==1 && view.sprites[0].margin_only);
    assert(view.sprites[0].slot==0xf046 && view.sprites[0].id==0x80000046 && view.sprites[0].gen==7);
    assert(view.sprites[0].screen_y==90);render();
    for (int y=90;y<98;y++) for (int x=316;x<332;x++) assert(image[y*468+x]==(x<320?0:1));
    /* Interpolation regenerates foreground for the regular sprites. Margin
     * neighbours must still exclude the centre even with position overrides. */
    view.foreground_enabled=1;cells[17]=1;assets.flags[1]=0x40;memset(assets.front[1],9,256);overrides=1;
    render();assert(image[48*468+272]==0);
    overrides=0;view.foreground_enabled=0;cells[17]=0;assets.flags[1]=0;
    /* Appended extra is rearward even though array order follows regulars. */
    source(400);FrameSprite regular={.slot=1,.id=129,.frame=1,.mode=1,.screen_x=400,.screen_y=90};
    view.sprites[0]=regular;view.sprite_count=1;enh_margin_capture(&view);assert(view.sprite_count==2);render();
    assert(image[90*468+400]==2);
    /* The actual projector: constructed camera admission does not move or
     * animate a sleeping source. Keep the player below it to avoid riding. */
    source(400);SET_DW(DS_PlayerY,200);SET_DW(DS_CameraCol,0);UpdateTerrainEntities();
    assert(DW(0x9107)==400 && DW(0x9109)==100 && DW(0x910b)==0);
    SET_DW(DS_CameraCol,10);UpdateTerrainEntities();
    assert(DW(0x9107)==400 && DW(0x9109)==100 && DW(0x910b)==0);
    assert(DW(0x5570+9)==0x9107 && DW(0x5570+2)==98);SET_DW(DS_CameraCol,0);
    /* A regular generation may eventually enter the source-ID namespace. */
    source(400);regular.id=0x80000046;view.sprites[0]=regular;view.sprite_count=1;enh_margin_capture(&view);assert(view.sprite_count==1);
    source(400);SET_DB(0x9115,1);enh_margin_capture(&view);assert(!view.sprite_count);
    source(400);SET_DB(0x910d,8);enh_margin_capture(&view);assert(!view.sprite_count);
    source(400);SET_DW(0x910b,0xffff);SET_DW(0x8f1d,400);SET_DW(0x8f1f,100);SET_DW(0x8f21,0);enh_margin_capture(&view);assert(!view.sprite_count);
    source(400);assert(!pre2_view_configure(640));enh_margin_capture(&view);assert(!view.sprite_count);assert(!pre2_view_configure(0));
    puts("regions_margin: full-memory purity, 320 exclusion incl. edge carry, rear order, source generation, collision guard, sleeping-only allowlist, pickup rejection and wide-sim exclusion pass");
    return 0;
}
