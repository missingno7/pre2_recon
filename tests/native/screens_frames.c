/* Full CRT publication checks for startup and attract screens. Sprite events belong to
 * rgame; until integrated, every remaining differing store byte must lie in one of the
 * byte-aligned rectangles captured immediately before those sprite draws. */
#include "script.h"
#include "c/scene_screens.h"
#include "c/vga.h"
#include "native/compositor.h"

static void (*sink)(const SceneEvent *);
static uint8_t sprite_area[65536];
static unsigned frames, sprite_frames, failures, picture_frames, mode12_frames;
static int enter_code, requested, code_frames, year, picture_active, transition_run;

static void sprite_rectangles(uint16_t page)
{
    for (uint16_t slot = 0x4F0A; slot <= 0x5720; slot = (uint16_t)(slot + 18)) {
        uint16_t fw = DW(slot + 4);
        if (fw == 0xFFFF) continue;
        uint16_t bx = (uint16_t)((fw & 0x1FFF) << 1);
        int w = DB(DS_FrameWidths + bx), h = DB(DS_FrameHeights + bx);
        int hx = (int8_t)DB(DS_FrameHotX + bx);
        if (fw & 0x8000) hx = (int8_t)(w - (uint8_t)hx);
        int x = (int16_t)DW(slot) - hx - (int16_t)(DW(DS_CameraCol) << 4);
        int bottom = (int16_t)DW(slot + 2) + (int8_t)DB(DS_FrameHotY + bx)
                   + (int16_t)DW(DS_ShakeRows) - (int16_t)(DW(DS_CameraRow) << 4) - DB(DS_FineScroll);
        int left = x < 0 ? 0 : x / 8, right = (x + w + 7) / 8;
        int top = bottom - h;
        if (top < 0) top = 0;
        if (bottom > 176) bottom = 176;
        if (right > 40) right = 40;
        for (int y = top; y < bottom; y++) for (int b = left; b < right; b++)
            sprite_area[(uint16_t)(page + y * 40 + b)] = 1;
    }
}

static void event(const SceneEvent *e)
{
    sink(e);
    if (e->kind == SCENE_MODE_SET) {
        picture_active = (e->a[0] & 0x7F) == 0x12 || (e->a[0] & 0x7F) == 0x13;
        if (!(e->a[0] & 0x80)) memset(sprite_area, 0, sizeof sprite_area);
    }
    if (e->kind == SCENE_SCREEN_COPY && e->a[1] != 0x3F40) {
        /* The restore has completed at this event. Forget the old pose on this page;
         * the other page retains its preceding pose until its own restore. */
        for (unsigned i = 0; i < (unsigned)e->a[2]; i++) sprite_area[(uint16_t)(e->a[1] + i)] = 0;
        sprite_rectangles((uint16_t)e->a[1]);
    }
    if (enter_code && e->kind == SCENE_SCREEN_MAP_EDGES) {
        code_frames++;
        if (code_frames >= 10 && code_frames < 14) {
            uint8_t sc = (uint8_t)(2 + code_frames - 10);
            rt_key_at(sc, rt_now()); rt_key_at((uint8_t)(sc | 0x80), rt_now() + 1000000);
        }
    }
}

static void service(void)
{
    if (!enter_code) { script_service(); return; }
    if (strcmp(pre2_phase, "RunFrontEnd()")) { script_service(); return; }
    if (script.down) { rt_key_at(0x9C, rt_now()); script.down = 0; }
    if (!requested) { SET_DB(DS_byte_27F7, 1); requested = 1; }
}

static void display(uint64_t t, uint16_t start)
{
    (void)t; (void)start;
    frames++;
    if (picture_active) picture_frames++;
    if (rd8(0x40, 0x49) == 0x12) mode12_frames++;
    /* LevelStart inherits this diagnostic phase while rgame loads/renders the level. */
    if (!picture_active && (!strcmp(pre2_phase, "SetLevelVideoMode()") || !strcmp(pre2_phase, "MainFrameLoop"))) return;
    uint32_t a = 0;
    if (!compositor_diff_vga(&a)) return;
    int outside = 0;
    for (unsigned off = 0; off < 65536; off++) for (int d = 0; d < 8; d++) {
        unsigned colour = 0;
        for (int p = 0; p < 4; p++) colour |= ((pre2_vga.plane[p][off] >> (7 - d)) & 1) << p;
        if (comp_planar[off * 8 + (unsigned)d] != colour && !sprite_area[off]) outside++;
    }
    if (outside) {
        if (!failures++) { printf("FAIL year %d %s frame %u: %d dots outside sprite rectangles, first address %04Xh\n",
            year, pre2_phase, frames, outside, a); }
    } else sprite_frames++;
}

static void run(const char *assets, int y, const char *scenario)
{
    year = y; enter_code = !strcmp(scenario, "code");
    transition_run = !strcmp(scenario, "ending") || !strcmp(scenario, "castle");
    frames = sprite_frames = failures = picture_frames = mode12_frames = 0; picture_active = 0;
    requested = code_frames = 0;
    pre2_frames_run = 0; script_reset(enter_code ? "none" : scenario);
    rt_boot(assets, y, 6, 1); compositor_init();
    sink = pre2_scene_sink; pre2_scene_sink = event;
    rt_on_service = service; rt_on_display_frame = display; rt_on_pcm = 0;
    RtConfig cfg = { 0 };
    if (rt_init(&cfg)) exit(1);
    uint64_t end = (enter_code ? 35ull : transition_run ? 75ull : 49ull) * RT_NS_PER_SEC;
    for (uint64_t b = 0; b < end; b += RT_NS_PER_SEC / 60) {
        if (rt_run_until(b) != RT_STEP_DONE || (!transition_run && pre2_frames_run) || (enter_code && code_frames >= 240)) break;
    }
    printf("screens_frames: year %d %s, %u displays (%u picture / %u mode12), %u with sprite-only differences, %u failures, %d code frames\n",
           y, scenario, frames, picture_frames, mode12_frames, sprite_frames, failures, code_frames);
    if (failures || (enter_code && code_frames < 240) || (!strcmp(scenario, "ending") && !mode12_frames)) exit(1);
    pre2_scene_sink = 0;
}

int main(int argc, char **argv)
{
    setvbuf(stdout, 0, _IONBF, 0);
    const char *assets = arg_assets(argc, argv);
    run(assets, 1995, "attract"); run(assets, 1997, "attract"); run(assets, 1995, "code");
    run(assets, 1995, "ending"); run(assets, 1995, "castle");
    return 0;
}
