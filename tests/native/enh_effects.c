/* Constructed draw boundary: maximum scripted wind, consumed particles, firefly
 * channel operations, the foreground request bound and immutable map edits. */
#include "script.h"
#include "c/scene_game.h"
#include "c/render.h"
#include "c/level.h"
#include "c/sprites.h"
#include "native/compositor.h"
#include "native/enh_render.h"
#include <assert.h>

static FrameGameView view, old;
static uint8_t output[320*176], palette[256][3];
int main(int argc, char **argv)
{
    rt_boot(arg_assets(argc,argv),1995,6,1); compositor_init(); pre2_phase="MainFrameLoop";
    SET_DW(DS_LevelSegment,0x3000); SET_DB(DS_MapRows,16);
    memset(pre2_mem+lin(0x3000,0),0,65536);
    for(int i=0;i<256;i++) SET_DB(DS_TileClassTable+i,1);
    SET_DW(DS_AnimRemapPtr,0x6688);
    for(int i=0;i<256;i++) SET_DB(0x6688+i,i);
    for(int p=0;p<4;p++) for(int i=0;i<8000;i++) wr8(0x5000,(uint16_t)(p*8000+i),(p==0 || p==2)?255:0);
    scene_emit(SCENE_GAME_BACKDROP,1,0x5000); scene_emit(SCENE_GAME_LEVEL,0);
    for(int i=0;i<7680;i++) memset(comp_px((uint16_t)(0x3f40+i),0),5,8);
    SET_DW(DS_DrawPage,0x2000);SET_DW(DS_CameraCol,0);SET_DW(DS_CameraRow,0);
    SET_DW(DS_ColumnRing,0);SET_DW(DS_RowRing,0);SET_DW(DS_RingTileOffset,0x3f40);
    SET_DB(DS_FineScroll,0);SET_DW(DS_ShakeRows,0);
    scene_emit(SCENE_GAME_RING,0);
    for(int i=0;i<20;i++) {
        SET_DW(DS_DebrisRecord+i*6,8+i);SET_DW(DS_DebrisRecord+i*6+2,32);
        SET_DB(DS_DebrisRecord+i*6+4,0);SET_DB(DS_DebrisRecord+i*6+5,0);
        uint16_t f=(uint16_t)(0x6ea9+i*8);
        SET_DW(f,(40+i)*8);SET_DW(f+2,64*8);SET_DW(f+4,0);SET_DB(f+6,5);
    }
    UpdateEffectParticles();
    for(int i=0;i<116;i++) SET_DW(DS_RenderSlots+i*18+4,0xffff);
    SET_DW(DS_SnowFrameTick,1);DrawSpriteObjects();
    UpdateFireflies();
    SET_DW(DS_ScriptPointer,0xf000);SET_DW(0xf000,0xffff);SET_DW(DS_SnowWind,255);
    for(int i=0;i<256;i++) SET_DW(DS_FlakeTable+i*2,i*20);
    UpdateWindAndFlakes();
    compositor_game_view(&view,0x2000);
    assert(view.dot_count==551);
    for(int i=20;i<40;i++) assert(view.dots[i].kind==1 && view.dots[i].or_colour==14);
    EnhImage im={0};im.index=output;im.capacity=sizeof output;im.stride=320;
    assert(!enh_render(&view,view.assets,palette,15,0,0,320,176,NULL,&im));
    unsigned diff=0;
    for(int y=0;y<176;y++) for(int x=0;x<320;x++) diff+=output[y*320+x]!=*comp_px((uint16_t)(0x2000+y*40+x/8),x%8);
    assert(!diff);
    for(int i=0;i<FRAME_MAX_FOREGROUND;i++) scene_emit(SCENE_GAME_FOREGROUND,2,0,0);
    compositor_game_view(&view,0x2000);assert(view.foreground_count==FRAME_MAX_FOREGROUND);
    old=view;uint64_t h=enh_view_hash(&old,FNV0);
    wr8(0x3000,200,1);scene_emit(SCENE_GAME_RING,0);compositor_game_view(&view,0x2000);
    assert(old.map->rows[0][200]==0 && view.map->rows[0][200]==1);
    assert(old.map!=view.map && old.map->rows[1]==view.map->rows[1]);
    assert(enh_view_hash(&old,FNV0)==h);
    puts("enh_effects: 551 ordered dots pixel-equal, 1856 foreground requests retained, map rows copy on change");
    return 0;
}
