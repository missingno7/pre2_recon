/* Constructed native capture check for stable wide sprite/dot identities. */
#include "c/pre2.h"
#include "c/game.h"
#include "c/view.h"
#include "c/pools.h"
#include "c/scene_game.h"
#include "native/compositor.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static FrameGameView view;
static void event(uint16_t kind, unsigned count, const int32_t *args)
{
    SceneEvent e; memset(&e, 0, sizeof e); e.kind = kind; e.n = (uint16_t)count;
    for (unsigned i = 0; i < count; i++) e.a[i] = args[i];
    assert(pre2_scene_sink); pre2_scene_sink(&e);
}
static void ring(void)
{
    event(SCENE_GAME_RING, 0, NULL);
    event(SCENE_GAME_SPRITES_BEGIN, 0, NULL);
    memset(&view, 0, sizeof view);
    compositor_game_view(&view, 0x2000);
    assert(view.valid && view.world_valid);
}
static void dot(unsigned kind, uint16_t source, uint32_t bits, unsigned x, unsigned y)
{
    int32_t a[8] = { 0x3000, (int32_t)bits, 15, (int32_t)kind, 0, (int32_t)(x & ~7u), (int32_t)y, source };
    event(SCENE_GAME_DOT, 8, a);
}
int main(void)
{
    pre2_phase = "MainFrameLoop";
    pre2_view.variant = 1; pre2_view.configured = 1;
    pre2_view.requested_width = pre2_view.width_px = 480; pre2_view.height_px = 176;
    pre2_view.cols = 30; pre2_view.rows = 11; pre2_view.summary_rows = 12;
    SET_DB(DS_CurrentLevel, 1); SET_DB(DS_MapRows, 32); SET_DW(DS_DrawPage, 0x2000);
    compositor_init(); pre2_pools_reset();
    event(SCENE_GAME_LEVEL, 0, NULL); ring();

    uint16_t burst=pre2_pool_slot(PRE2_POOL_BURST,40);
    SET_DW(0x4F1C+4,0); SET_DW(burst+4,0);
    int32_t sprite[12]={0x4F1C,0,0,1,400,32,0,0,0,0,0,0};
    event(SCENE_GAME_SPRITE,12,sprite);
    sprite[0]=burst; event(SCENE_GAME_SPRITE,12,sprite);
    compositor_game_view(&view,0x2000);
    assert(view.sprite_count==2 && view.sprites[0].slot==1 && view.sprites[1].slot>=128);
    assert(view.sprites[0].id!=view.sprites[1].id);
    uint32_t sprite_id=view.sprites[1].id;
    assert(pre2_pool_grow(PRE2_POOL_CHILD));
    event(SCENE_GAME_SPRITES_BEGIN,0,NULL);
    event(SCENE_GAME_SPRITE,12,sprite);
    compositor_game_view(&view,0x2000);
    assert(view.sprite_count==1 && view.sprites[0].id==sprite_id);
    ring();

    uint16_t point = pre2_pool_slot(PRE2_POOL_POINT, PRE2_POOL_ORIGINAL_POINT + 5);
    dot(0, point, 0x80, 24, 32); compositor_game_view(&view, 0x2000);
    assert(view.dot_count == 1 && view.dots[0].id && view.dots[0].gen == 1);
    uint64_t point_first = view.dots[0].id;
    ring(); dot(0, point, 0x80, 24, 32); compositor_game_view(&view, 0x2000);
    assert(view.dot_count == 1 && view.dots[0].gen == 2 && view.dots[0].id != point_first);

    uint16_t fly = 0x6EA9;
    SET_DW(fly, 100); ring(); dot(1, fly, 0x80, 40, 48); compositor_game_view(&view, 0x2000);
    assert(view.dot_count == 1 && view.dots[0].gen == 1);
    uint64_t fly_first = view.dots[0].id;
    ring(); dot(1, fly, 0x80, 40, 48); compositor_game_view(&view, 0x2000);
    assert(view.dots[0].id == fly_first && view.dots[0].gen == 1);
    SET_DW(fly, 0x55AA); ring(); SET_DW(fly, 100); dot(1, fly, 0x80, 40, 48);
    compositor_game_view(&view, 0x2000);
    assert(view.dot_count == 1 && view.dots[0].gen == 2 && view.dots[0].id != fly_first);

    ring(); dot(2, 0x9000, 0x0302, 64, 80); compositor_game_view(&view, 0x2000);
    assert(view.dot_count == 3);
    for (unsigned i = 0; i < view.dot_count; i++) {
        assert(view.dots[i].id && view.dots[i].gen != 0);
        for (unsigned j = 0; j < i; j++) assert(view.dots[i].id != view.dots[j].id);
    }
    printf("wide_capture: point reuse generations, firefly free/reuse generations, three unique snow pieces\n");
    return 0;
}
