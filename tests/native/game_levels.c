/* Gameplay-only parity over all sixteen level assets, plus a constructed death/respawn.
 * Native coroutine, deterministic input; the DOS executable is never booted. */
#include "script.h"
#include "c/scene_game.h"
#include "native/compositor.h"
#include "native/scanout.h"
#include <stdio.h>

static Pre2Frame frame;
static uint8_t reference[SCAN_MAX_W * SCAN_MAX_H];
static unsigned target_level, selected, displayed, bad, store_bad, max_dots, max_sprites;
static unsigned death, last_epoch, page_bad, cut_count, reached;
typedef struct { uint32_t sim; int32_t x, y; int16_t shake; uint8_t valid; } Stamp;
static Stamp stamps[2];
static void stamp_page(Stamp *s)
{
    s->sim = pre2_frames_run; s->x = DW(DS_CameraCol) * 16;
    s->y = DW(DS_CameraRow) * 16 + DB(DS_FineScroll); s->shake = (int16_t)DW(DS_ShakeRows); s->valid = 1;
}
static uint64_t first_game;
static uint16_t last_kind;
static void (*real_sink)(const SceneEvent *);
static void watch(const SceneEvent *e)
{
    last_kind = e->kind;
    if (e->kind == SCENE_GAME_RING || (e->kind == SCENE_GAME_CLEAR && e->a[1] == 8000)) stamp_page(&stamps[!!(DW(DS_DrawPage) & 0x2000)]);
    if (e->kind == SCENE_GAME_COPY && ((e->a[2] == 2 && e->a[3] == 176) || e->a[2] == 8000)) stamps[!!(e->a[1] & 0x2000)] = stamps[!!(e->a[0] & 0x2000)];
    if (e->kind == SCENE_GAME_LEVEL || e->kind == SCENE_GAME_HIDE) memset(stamps, 0, sizeof stamps);
    if (e->kind == SCENE_GAME_CUT) cut_count++;
    real_sink(e);
}

static void service(void)
{
    script_service();
    if (!selected && !strcmp(pre2_phase, "ShowWorldMapScrollIn()")) {
        SET_DB(DS_CurrentLevel, target_level < 10 ? target_level : 0); SET_DB(DS_byte_B198, 1); selected = 1;
    }
    if (pre2_frames_run && !first_game) first_game = rt_now();
    if (target_level >= 10 && selected == 1 && first_game && rt_now() - first_game > 2000000000ull) {
        SET_DB(DS_WarpLevelTable, target_level); SET_DB(DS_LevelEndMode, 2); selected = 2;
    }
    if (target_level == 0 && first_game && !death && rt_now() - first_game > 2000000000ull) {
        SET_DB(DS_RespawnState, 1); SET_DB(DS_Energy, 0); death = 1;
    }
}
static void display(uint64_t t, uint16_t start)
{
    ScanInfo si;
    scan_indices(&scan_vga, start, reference, &si); compositor_publish(t, start, &frame);
    if (strcmp(pre2_phase, "MainFrameLoop") && strcmp(pre2_phase, "SetLevelVideoMode()")) return;
    if (!pre2_frames_run && !selected) return;
    displayed++;
    const Stamp *expected = &stamps[!!(start & 0x2000)];
    if (frame.game.valid && expected->valid && (frame.game.sim_frame != expected->sim || frame.game.camera_x != expected->x ||
            frame.game.camera_y != expected->y || frame.game.shake_rows != expected->shake) && page_bad++ < 3)
        printf("semantic page mismatch: start %04X; actual sim %u, expected sim %u\n", start, frame.game.sim_frame, expected->sim);
    unsigned diff = 0;
    for (int i = 0; i < si.w * si.h; i++) diff += reference[i] != frame.index[i];
    if (diff && bad++ < 3) printf("level %u t %.3f step %s: %u differing pixels, event %u\n", target_level, t / 1e9, pre2_phase, diff, last_kind);
    if (diff && bad <= 3) {
        int x0 = 9999, x1 = 0, y0 = 9999, y1 = 0;
        for (int i = 0; i < si.w * si.h; i++) if (reference[i] != frame.index[i]) {
            int x = i % si.w, y = i / si.w;
            if (x < x0) x0 = x;
            if (x > x1) x1 = x;
            if (y < y0) y0 = y;
            if (y > y1) y1 = y;
        }
        printf("  bounds %d,%d..%d,%d\n", x0, y0, x1, y1);
    }
    uint32_t at = 0;
    if (compositor_diff_vga(&at)) store_bad++;
    if (!strcmp(pre2_phase, "MainFrameLoop") && frame.game.valid) {
        last_epoch = frame.game.level_epoch;
        if (frame.game.level == target_level) reached++;
        if (frame.game.dot_count > max_dots) max_dots = frame.game.dot_count;
        if (frame.game.sprite_count > max_sprites) max_sprites = frame.game.sprite_count;
    }
}
int main(int argc, char **argv)
{
    unsigned failed = 0, tested = 0, pixel_failed = 0, view_failed = 0;
    int only = -1;
    for (int a = 1; a + 1 < argc; a++) if (!strcmp(argv[a], "--level")) only = atoi(argv[a + 1]);
    for (target_level = 0; target_level < 16; target_level++) {
        if (only >= 0 && target_level != (unsigned)only) continue;
        tested++;
        reached = page_bad = selected = displayed = bad = store_bad = death = cut_count = last_epoch = max_dots = max_sprites = 0;
        memset(stamps, 0, sizeof stamps);
        first_game = 0; pre2_frames_run = 0; script_reset("none");
        rt_boot(arg_assets(argc, argv), 1995, 6, 1); compositor_init(); real_sink = pre2_scene_sink; pre2_scene_sink = watch;
        RtConfig cfg = { 0 }; rt_on_service = service; rt_on_display_frame = display; rt_on_pcm = 0;
        if (rt_init(&cfg)) return 1;
        for (uint64_t t = 0; t < 50ull * RT_NS_PER_SEC; t += RT_NS_PER_SEC / 60)
            if (rt_run_until(t) != RT_STEP_DONE || (first_game && t > first_game && t - first_game > 12000000000ull)) break;
        pre2_scene_sink = 0;
        printf("level %u: %u display frames, %u differ, %u transient store differences; sprites <= %u, dots <= %u, epoch %u, view mismatches %u, cuts %u\n",
               target_level, displayed, bad, store_bad, max_sprites, max_dots, last_epoch, page_bad, cut_count);
        int content_failed = !displayed || !max_sprites || !reached || (target_level == 0 && cut_count < 2);
        if (bad || content_failed) pixel_failed++;
        if (page_bad || content_failed) view_failed++;
        if (bad || page_bad || content_failed) failed++;
    }
    printf("game_levels: pixels %u/%u levels equal; semantic pages %u/%u equal\n", tested - pixel_failed, tested, tested - view_failed, tested);
    return failed != 0;
}
