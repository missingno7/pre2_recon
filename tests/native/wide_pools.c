/* Constructed tests for the wide-only DATA pool tail. No original code is run. */
#include "c/pre2.h"
#include "c/view.h"
#include "c/pools.h"
#include "c/effects.h"
#include "c/level.h"
#include "native/level_regions.h"
#include <stdio.h>
#include <string.h>

extern int AllocChildRecord(uint16_t *di);
extern uint16_t AllocDebrisRecord(void);

static int fail(const char *what)
{
    fprintf(stderr, "wide_pools: %s\n", what);
    return 1;
}

static void occupy(Pre2PoolKind kind, unsigned count, unsigned field)
{
    for (unsigned i=0;i<count;i++) SET_DW(pre2_pool_slot(kind,i)+field,0x1234);
}

int main(void)
{
    memset(pre2_mem,0,sizeof pre2_mem);
    pre2_view.variant=1; pre2_view.configured=1; pre2_view.requested_width=640;
    pre2_view.width_px=640; pre2_view.height_px=176; pre2_view.cols=40; pre2_view.rows=11;
    pre2_view.summary_rows=12; pre2_view.region=PRE2_REGION_MAIN;
    pre2_view_region_policy=pre2_region_policy;
    SET_DW(DS_NextFreeSegment,DATA_SEG+(0xB1BC>>4)+1);
    pre2_pools_reserve();
    if (DW(DS_NextFreeSegment)!=DATA_SEG+0xDB9)
        return fail("heap does not start beyond the reserved DS tail and canary");
    pre2_pools_reset();
    const uint16_t guard=0x5AA5;
    SET_DW(0xDB80,guard);

    uint16_t original_child=pre2_pool_slot(PRE2_POOL_CHILD,0);
    occupy(PRE2_POOL_CHILD,pre2_pool_count(PRE2_POOL_CHILD),4);
    uint16_t child=0;
    if (AllocChildRecord(&child) || pre2_pool_count(PRE2_POOL_CHILD)!=48 ||
        child!=pre2_pool_slot(PRE2_POOL_CHILD,24) || pre2_pool_slot(PRE2_POOL_CHILD,0)!=original_child)
        return fail("child growth or stable original offset");

    occupy(PRE2_POOL_BURST,pre2_pool_count(PRE2_POOL_BURST),4);
    SET_DW(DS_EffectFrame,0x2080); SET_DW(DS_EffectOriginX,0x100); SET_DW(DS_EffectOriginY,0x80);
    uint16_t dy=0;
    SpawnEffectBurst(0x10,&dy,1);
    if (pre2_pool_count(PRE2_POOL_BURST)!=128 || DW(pre2_pool_slot(PRE2_POOL_BURST,64)+4)!=0x2080)
        return fail("burst growth after initial capacity");

    occupy(PRE2_POOL_DEBRIS,pre2_pool_count(PRE2_POOL_DEBRIS),4);
    SpawnDebrisElement(0x40,0x4F1C);
    if (pre2_pool_count(PRE2_POOL_DEBRIS)!=64 || DW(pre2_pool_slot(PRE2_POOL_DEBRIS,32)+4)!=0x40)
        return fail("debris sprite growth after initial capacity");
    while (pre2_pool_grow(PRE2_POOL_DEBRIS)) { }
    occupy(PRE2_POOL_DEBRIS,pre2_pool_count(PRE2_POOL_DEBRIS),4);
    SET_DW(DS_LastDebrisSlot,0x1234);
    SpawnDebrisElement(0x40,0x4F1C);
    if (DW(DS_LastDebrisSlot)!=0xFFFF) return fail("bounded debris exhaustion retained a stale slot");

    for (unsigned n=0;n<PRE2_POOL_ORIGINAL_POINT;n++) SET_DW(pre2_pool_slot(PRE2_POOL_POINT,n),0xFFFF);
    for (unsigned n=0;n<256;n++) {
        uint16_t point=AllocDebrisRecord();
        if (point==0xFFFF) return fail("point allocator stopped before its explicit bound");
        SET_DW(point,0x1111);
    }
    if (AllocDebrisRecord()!=0xFFFF || DW(0xDB80)!=guard)
        return fail("point pool exhausted without bounded failure or crossed its extent");

    for (unsigned n=0;n<70;n++) {
        uint16_t source=(uint16_t)(0x8F1D+n*7);
        SET_DW(source,0x40); SET_DW(source+2,0x40); SET_DW(source+4,0x2000+n); SET_DB(source+6,0);
    }
    SET_DW(DS_CameraCol,0); SET_DW(DS_CameraRow,0); SET_DW(DS_PlayerVelY,0xFFF0);
    for (unsigned n=0;n<16;n++) {
        uint16_t source=(uint16_t)(0x9107+n*0x0F);
        SET_DW(source,0x40); SET_DW(source+2,0x40); SET_DW(source+4,0x2200+n);
        SET_DB(source+6,0); SET_DB(source+7,0); SET_DB(source+0x0E,0);
    }
    for (unsigned active=7;active<=16;active+=9) {
        for (unsigned n=0;n<16;n++) SET_DW(0x9107+n*0x0F+4,n<active?(uint16_t)(0x2200+n):0xFFFF);
        UpdateTerrainEntities();
        for (unsigned n=0;n<active;n++) {
            uint16_t terrain=pre2_pool_slot(PRE2_POOL_TERRAIN,n);
            if (DW(terrain+4)!=(uint16_t)(0x2200+n) || DW(terrain+9)!=(uint16_t)(0x9107+n*0x0F))
                return fail("terrain projection count/order at seven or sixteen sources");
        }
        if (DW(0xDB80)!=guard) return fail("terrain producer crossed its reserved extent");
    }
    SET_DW(DS_CameraCol,0); SET_DW(DS_CameraRow,0); SET_DB(DS_SnowFrameTick,1);
    BuildEffectDrawList();
    for (unsigned n=0;n<70;n++) {
        uint16_t projection=pre2_pool_slot(PRE2_POOL_PROJECTION,n);
        if (DW(projection+4)!=(uint16_t)(0x2000+n) || DW(projection+9)!=(uint16_t)(0x8F1D+n*7))
            return fail("projection order or candidate retention");
    }

    if (pre2_render_count()!=29+pre2_pool_count(PRE2_POOL_CHILD)+pre2_pool_count(PRE2_POOL_BURST)+
        pre2_pool_count(PRE2_POOL_PROJECTION)+pre2_pool_count(PRE2_POOL_DEBRIS)+pre2_pool_count(PRE2_POOL_TERRAIN))
        return fail("logical sprite capacity");
    if (pre2_render_slot(0)!=0x4F0A || pre2_render_slot(11)!=pre2_pool_slot(PRE2_POOL_CHILD,0) ||
        pre2_render_slot(11+pre2_pool_count(PRE2_POOL_CHILD))!=pre2_pool_slot(PRE2_POOL_BURST,0))
        return fail("role-ordered logical sprite slots");
    if (DW(0xDB80)!=guard) return fail("extension wrote beyond its reserved extent");
    printf("wide_pools: PASS; high water child=%u burst=%u debris=%u point=%u projection=%u\n",
        pre2_pool_highwater(PRE2_POOL_CHILD),pre2_pool_highwater(PRE2_POOL_BURST),
        pre2_pool_highwater(PRE2_POOL_DEBRIS),pre2_pool_highwater(PRE2_POOL_POINT),
        pre2_pool_highwater(PRE2_POOL_PROJECTION));
    return 0;
}
