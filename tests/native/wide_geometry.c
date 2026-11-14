/* Exact inclusive/strict edges of the wide simulation's distinct predicates. */
#include "script.h"
#include "c/view.h"
#include "c/pools.h"
#include "c/level.h"
#include "c/objects.h"
#include "c/sprites.h"
#include "c/render.h"
#include "c/effects.h"
#include "native/level_regions.h"
#include <assert.h>
extern int IsInCameraWindow(uint16_t x,uint16_t y);
static void draw_at(int x,int baseline,int visible)
{
    SET_DW(DS_PlayerX,x); SET_DW(DS_PlayerY,baseline);
    SET_DW(DS_PlayerFrameWord,0);
    DrawSpriteObjects();
    assert(!!(DB(DS_PlayerFrameHigh)&0x20)==visible);
}
int main(int argc,char **argv)
{
    rt_boot(arg_assets(argc,argv),1995,6,1);
    pre2_view_region_policy=pre2_region_policy;
    assert(!pre2_view_configure(640));
    SET_DB(DS_byte_2879,0); SET_DB(DS_CurrentLevel,0);
    pre2_view_begin_level(); ResetRenderSlots();
    SET_DW(DS_CameraCol,10); SET_DW(DS_CameraRow,10);
    assert(!IsInCameraWindow(8*16,8*16));
    assert(!IsInCameraWindow(52*16,23*16));
    assert(IsInCameraWindow(7*16,10*16));
    assert(IsInCameraWindow(53*16,10*16));
    assert(IsInCameraWindow(10*16,24*16));
    SET_DW(DS_CameraCol,0); SET_DW(DS_CameraRow,0);
    SET_DB(DS_FineScroll,0); SET_DW(DS_ShakeRows,0);
    SET_DW(DS_FrameWidths,0x1010); /* constructed 16x16 frame */
    SET_DB(DS_FrameHotX,0); SET_DB(DS_FrameHotY,0);
    draw_at(639,16,1); draw_at(640,16,0);
    draw_at(-16,16,1); draw_at(-17,16,0);
    draw_at(100,0,0); draw_at(100,191,1); draw_at(100,192,0);
    draw_at(500,32,1);
    SET_DB(DS_CurrentLevel,1); pre2_view_begin_level();
    assert(pre2_view.camera_max_col==104 && pre2_view.logical_right_px==144*16);
    SET_DW(DS_CameraCol,104); assert(ScrollViewRight());
    SET_DW(DS_CameraCol,0); assert(ScrollViewLeft());
    assert(pre2_view_cave(0,236)==236 && pre2_view.width_px==320);
    SET_DW(DS_CameraCol,235); pre2_view_refresh();
    assert(pre2_view.camera_max_col==236 && pre2_view.logical_right_px==256*16);
    /* Exit contacts in an extension projection must remain part of the
     * ascending contact union, including authored level-number remaps. */
    ResetRenderSlots();
    SET_DW(DS_PlayerX,500); SET_DW(DS_PlayerY,96); SET_DW(DS_PlayerFrameWord,0);
    SET_DB(DS_PlayerHurtTimer,1);
    const unsigned exits[16]={0,1,12,3,4,5,14,7,8,9,10,11,12,2,14,6};
    uint16_t projected=pre2_pool_slot(PRE2_POOL_PROJECTION,40);
    for (unsigned level=0;level<16;level++) {
        SET_DB(DS_CurrentLevel,level); SET_DB(DS_LevelEndMode,0);
        SET_DW(projected,500); SET_DW(projected+2,96);
        SET_DW(projected+4,0x2117); SET_DW(projected+12,0);
        SET_DW(DS_FrameWidths+2*0x117,0x1010); SET_DB(DS_FrameHotX+2*0x117,0);
        ProcessPlayerContacts();
        assert(DB(DS_LevelEndMode)==1 && DB(DS_CurrentLevel)==exits[level]);
        assert(DW(projected+4)==0xFFFF);
    }
    SET_DB(DS_LevelEndMode,0); SET_DW(projected+4,0x2137);
    SET_DW(DS_FrameWidths+2*0x137,0x1010); SET_DB(DS_FrameHotX+2*0x137,0);
    ProcessPlayerContacts(); assert(DB(DS_LevelEndMode)==0xFF);
    /* A run's demo geometry is selected before level entry, independently of
     * its configured width. No host presentation option can activate it. */
    SET_DB(DS_byte_2879,1); pre2_view_begin_level();
    assert(!pre2_view.variant && pre2_view.width_px==320 && pre2_pool_count(PRE2_POOL_CHILD)==12);
    puts("wide_geometry: PASS activation/drawn edges, 16 extension exit contacts, warp contact, demo exclusion");
    return 0;
}
