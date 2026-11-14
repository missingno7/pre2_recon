/* Every non-world CRT publication, including CRT/plane-pass timing. The model
 * observer/publication hooks are chained here without changing the shared core. */
#include "script.h"
#include "c/scene_screens.h"
#include "c/vga.h"
#include "native/compositor.h"
#include "native/enh_screens.h"
#include <assert.h>
#undef assert
#define assert(test) do { if (!(test)) { fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #test); exit(1); } } while (0)
#ifdef _WIN32
#include <direct.h>
#endif

static void (*sink)(const SceneEvent *);
static Pre2Frame frame;
static uint8_t indices[2048 * 480], retained_index[64000];
static FrameScreenView retained;
static unsigned counts[8], mismatches[8], invalid, world_frames, total, retained_checks;
static unsigned seen[16], pending[16];
static uint64_t best_brightness[16];
typedef struct { FrameScreenView view; uint8_t palette[256][3], mask; uint16_t w, h; } Review;
static Review reviews[16];
static void choose_shot(unsigned n, uint64_t brightness)
{
    if (brightness <= best_brightness[n]) return;
    best_brightness[n] = brightness; pending[n] = 1;
    reviews[n].view = frame.screen_view; reviews[n].mask = frame.pel_mask;
    reviews[n].w = frame.w; reviews[n].h = frame.h;
    memcpy(reviews[n].palette, frame.palette, sizeof frame.palette);
}
static int enter_code, options, requested, code_frames, saw_code_error, saw_code_reset, saw_expert;
static uint64_t gameover_start;
static const char *scenario;
typedef struct { const char *name; unsigned kind, frames, bad; } PhaseCount;
static PhaseCount phases[64];
static unsigned phase_count;
static PhaseCount *phase_for(const Pre2Frame *f)
{
    for (unsigned i = 0; i < phase_count; i++) if (phases[i].kind == f->screen_view.kind && !strcmp(phases[i].name, f->screen)) return &phases[i];
    assert(phase_count < 64); PhaseCount *p = &phases[phase_count++];
    p->name = f->screen; p->kind = f->screen_view.kind; return p;
}
static const char *kinds[] = {"blank/loading", "picture", "menu", "CARTE", "credits", "level-intro", "tally", "game-over"};
static const char *names[] = {"titus", "present", "menu", "password", "options", "carte_reveal", "carte_end", "intro", "tally", "gameover", "theend", "castle", "creators", "latecredits", "loading", "front_title"};
#define SHEET_W (356 + 468 + 640)
#define SHEET_ROW 208
static uint32_t sheet[SHEET_W * SHEET_ROW * 16];

static void bmp(const char *path, const uint32_t *p, unsigned w, unsigned h)
{
    FILE *file = fopen(path, "wb"); assert(file);
    uint8_t hd[54] = {'B','M'}; uint32_t size = 54 + w * h * 4; int32_t height = -(int32_t)h;
    memcpy(hd + 2, &size, 4); hd[10] = 54; hd[14] = 40; memcpy(hd + 18, &w, 4);
    memcpy(hd + 22, &height, 4); hd[26] = 1; hd[28] = 32;
    assert(fwrite(hd, 1, 54, file) == 54); assert(fwrite(p, 4, (size_t)w*h, file) == (size_t)w*h); fclose(file);
}
static uint32_t rgb(const EnhImage *im, uint8_t c)
{
    const uint8_t *p = im->palette[c & im->pel_mask];
    return 0xff000000u | (uint32_t)((p[0] << 2) | (p[0] >> 4)) << 16 |
        (uint32_t)((p[1] << 2) | (p[1] >> 4)) << 8 | ((p[2] << 2) | (p[2] >> 4));
}
static void shot(unsigned n)
{
    const unsigned widths[] = {356, 468, 640}; unsigned offset = 0;
    for (unsigned k = 0; k < 3; k++) {
        unsigned w = widths[k], h = frame.h;
        if (frame.w == 640) w = 640;
        EnhImage im = {0}; im.index = indices; im.capacity = sizeof indices; im.stride = w;
        assert(!enh_screens_render(&frame.screen_view, frame.palette, frame.pel_mask,
            frame.screen_view.pan_x, frame.screen_view.pan_y, w, h, &im));
        static uint32_t pixels[640 * 480];
        for (unsigned i = 0; i < w*h; i++) pixels[i] = rgb(&im, indices[i]);
        char path[256]; snprintf(path, sizeof path, "build/screens_shots/%s_%u.bmp", names[n], w); bmp(path, pixels, w, h);
        unsigned draw_w = widths[k];
        for (unsigned y = 0; y < 200; y++) for (unsigned x = 0; x < draw_w; x++)
            sheet[(n * SHEET_ROW + 8 + y) * SHEET_W + offset + x] = pixels[(y * h / 200) * w + x * w / draw_w];
        for (unsigned bit = 0; bit < 5; bit++) for (unsigned y = 1; y < 6; y++) for (unsigned x = 0; x < 5; x++)
            sheet[(n * SHEET_ROW + y) * SHEET_W + offset + 2 + bit * 7 + x] = ((n + 1) & (1u << bit)) ? 0xffffffffu : 0xff444444u;
        offset += draw_w;
    }
    seen[n] = 1;
}
static void event(const SceneEvent *e)
{
    sink(e); draw_screens_observe(e);
    if (e->kind == SCENE_SCREEN_PICTURE && e->a[2] == 0x5e80) gameover_start = rt_now();
    if (e->kind == SCENE_SCREEN_MAP_EDGES) {
        code_frames++;
        if (enter_code) {
            if (DB(DS_CodeEntryState) == 1) saw_code_error = 1;
            if (saw_code_error && !DB(DS_CodeEntryState) && !DW(DS_CodeInputCount)) saw_code_reset = 1;
        }
        if (options && DB(DS_ExpertFlag)) saw_expert = 1;
        if (options && (code_frames == 60 || code_frames == 160)) {
            rt_key_at(0x4d, rt_now()); rt_key_at(0xcd, rt_now() + 20000000);
        }
        if (enter_code && code_frames >= 10 && code_frames < 22 && !((code_frames - 10) % 3)) {
            uint8_t sc = (uint8_t)(2 + (code_frames - 10) / 3);
            rt_key_at(sc, rt_now()); rt_key_at((uint8_t)(sc | 0x80), rt_now() + 20000000);
        }
    }
}
static void service(void)
{
    if (!strcmp(scenario, "gameover") && gameover_start && rt_now() - gameover_start < 12000000000ull) {
        if (script.down) { rt_key_at(0x9c, rt_now()); script.down = 0; }
        return;
    }
    if (!enter_code && !options) { script_service(); return; }
    if (strcmp(pre2_phase, "RunFrontEnd()")) { script_service(); return; }
    if (script.down) { rt_key_at(0x9c, rt_now()); script.down = 0; }
    if (!requested) {
        if (enter_code) SET_DB(DS_byte_27F7, 1);
        /* MODE is the front-end options screen: request it directly, then
         * toggle beginner/expert with timed arrow taps in event(). */
        else SET_DB(DS_byte_27F6, 1);
        requested = 1;
    }
}
static void display(uint64_t t, uint16_t start)
{
    compositor_publish(t, start, &frame); draw_screens_publish(&frame, start); total++;
    FrameScreenView *v = &frame.screen_view;
    if (frame.game.valid && frame.game.world_valid) { world_frames++; return; }
    if (!v->valid) {
        if (!invalid++) printf("INVALID %s %s %ux%u\n", scenario, frame.screen, frame.w, frame.h);
        return;
    }
    counts[v->kind]++; PhaseCount *phase = phase_for(&frame); phase->frames++;
    EnhImage im = {0}; im.index = indices; im.capacity = sizeof indices; im.stride = frame.w;
    int result = enh_screens_render(v, frame.palette, frame.pel_mask, v->pan_x, v->pan_y, frame.w, frame.h, &im);
    size_t pixels = (size_t)frame.w * frame.h;
    if (result || memcmp(indices, frame.index, pixels)) {
        phase->bad++;
        if (!mismatches[v->kind]++) {
            size_t first = 0, n = 0;
            for (size_t i = 0; i < pixels; i++) if (indices[i] != frame.index[i]) { if (!n) first = i; n++; }
            printf("MISMATCH %s %s %s: %zu pixels first %zu,%zu %u/%u origin %u count %u pan %.0f transition %u %u\n",
                scenario, kinds[v->kind], frame.screen, n, first % frame.w, first / frame.w,
                indices[first], frame.index[first], v->rows[0].origin, v->rows[0].count, v->pan_x, frame.transition.kind, frame.transition.step);
        }
    }
    /* Immutable snapshots must still render after subsequent ring/text writes. */
    if (retained.valid && frame.w == 320 && total % 101 == 0) {
        EnhImage old = {0}; old.index = indices; old.capacity = sizeof indices; old.stride = 320;
        assert(!enh_screens_render(&retained, frame.palette, frame.pel_mask, retained.pan_x, retained.pan_y, 320, 200, &old));
        assert(!memcmp(indices, retained_index, 64000)); retained_checks++;
    }
    if (v->kind == FRAME_SCREEN_MENU && !retained.valid) {
        retained = *v; memcpy(retained_index, frame.index, 64000);
    }
    if (v->kind == FRAME_SCREEN_EMPTY) choose_shot(14, 1);
    if (v->blank) return;
    uint64_t bright = 0;
    for (unsigned i = 0; i < (unsigned)frame.w * frame.h; i++) {
        const uint8_t *colour = frame.palette[frame.index[i] & frame.pel_mask];
        bright += colour[0] + colour[1] + colour[2];
    }
    if (!bright) return;
    if (!strcmp(frame.screen, "ShowTitusScreen()")) choose_shot(0, bright);
    if (!strcmp(frame.screen, "ShowPresentTitle()") && frame.transition.step > 5) choose_shot(1, bright);
    if (v->kind == FRAME_SCREEN_MENU && code_frames > 50 && (!options || (saw_expert && DB(DS_ExpertFlag) && code_frames > 80 && code_frames < 140))) choose_shot(enter_code ? 3 : options ? 4 : 2, bright);
    if (v->kind == FRAME_SCREEN_WORLD_MAP && v->pan_x > 420 && v->pan_x < 500) choose_shot(5, bright);
    if (v->kind == FRAME_SCREEN_WORLD_MAP && v->pan_x >= 639) choose_shot(6, bright);
    if (v->kind == FRAME_SCREEN_INTRO) choose_shot(7, bright);
    if (v->kind == FRAME_SCREEN_TALLY) choose_shot(8, bright);
    if (v->kind == FRAME_SCREEN_GAME_OVER) choose_shot(9, bright);
    if (v->kind == FRAME_SCREEN_PICTURE && !strcmp(scenario, "ending") && script.poked && frame.w == 320 && !strcmp(frame.screen, "MainFrameLoop")) choose_shot(10, bright);
    if (v->kind == FRAME_SCREEN_PICTURE && !strcmp(scenario, "castle") && script.poked && frame.w == 320 && !strcmp(frame.screen, "MainFrameLoop")) choose_shot(11, bright);
    if (frame.w == 640) choose_shot(12, bright);
    if (v->kind == FRAME_SCREEN_PICTURE && !strcmp(frame.screen, "RunFrontEnd()")) choose_shot(15, bright);
    if (v->kind == FRAME_SCREEN_CREDITS) choose_shot(13, bright);
}
static void run(const char *assets, int year, const char *name, int production)
{
    scenario = name; enter_code = !strcmp(name, "code"); options = !strcmp(name, "options");
    memset(counts, 0, sizeof counts); memset(mismatches, 0, sizeof mismatches);
    invalid = world_frames = total = 0; requested = code_frames = 0; retained.valid = 0;
    phase_count = 0; memset(phases, 0, sizeof phases); memset(pending, 0, sizeof pending);
    gameover_start = 0; saw_code_error = saw_code_reset = saw_expert = 0;
    pre2_frames_run = 0; script_reset(enter_code || options ? "none" : name);
    rt_boot(assets, year, 6, 1); compositor_init(); vga_memory_off = production;
    sink = pre2_scene_sink; pre2_scene_sink = event;
    rt_on_service = service; rt_on_display_frame = display; rt_on_pcm = 0;
    RtConfig cfg = {0}; cfg.audio_native = 1; assert(!rt_init(&cfg));
    uint64_t end = (enter_code || options ? 35ull : 75ull) * RT_NS_PER_SEC;
    for (uint64_t b = 0; b < end; b += RT_NS_PER_SEC / 60) {
        if (rt_run_until(b) != RT_STEP_DONE || ((enter_code || options) && code_frames >= 240)) break;
    }
    printf("enh_screens: %d %s vga_off=%d: %u total / %u world / %u invalid\n", year, name, production, total, world_frames, invalid);
    unsigned bad = invalid;
    for (unsigned k = 0; k < 8; k++) if (counts[k] || mismatches[k]) {
        printf("  %s: %u frames / %u mismatches\n", kinds[k], counts[k], mismatches[k]); bad += mismatches[k];
    }
    for (unsigned i = 0; i < phase_count; i++) printf("    %s / %s: %u frames / %u mismatches\n", phases[i].name, kinds[phases[i].kind], phases[i].frames, phases[i].bad);
    assert(!bad); if (enter_code || options) assert(code_frames >= 240);
    if (enter_code) assert(saw_code_error && saw_code_reset);
    if (options) assert(saw_expert);
    for (unsigned i = 0; i < 16; i++) if (pending[i]) {
        frame.screen_view = reviews[i].view; frame.w = reviews[i].w; frame.h = reviews[i].h;
        frame.pel_mask = reviews[i].mask; memcpy(frame.palette, reviews[i].palette, sizeof frame.palette);
        shot(i);
    }
    pre2_scene_sink = 0; vga_memory_off = 0;
}
int main(int argc, char **argv)
{
    setvbuf(stdout, 0, _IONBF, 0);
#ifdef _WIN32
    _mkdir("build/screens_shots");
#endif
    for (unsigned i = 0; i < sizeof sheet / sizeof *sheet; i++) sheet[i] = 0xff202020u;
    const char *assets = arg_assets(argc, argv);
    run(assets, 1995, "attract", 0); run(assets, 1997, "attract", 0);
    run(assets, 1995, "code", 0); run(assets, 1995, "options", 0);
    run(assets, 1995, "ending", 0); run(assets, 1995, "castle", 0); run(assets, 1995, "gameover", 0);
    run(assets, 1995, "ending", 1);
    bmp("build/screens_shots/contact.bmp", sheet, SHEET_W, SHEET_ROW * 16);
    FILE *manifest = fopen("build/screens_shots/manifest.txt", "w"); assert(manifest);
    for (unsigned i = 0; i < 16; i++) fprintf(manifest, "%u %s %s; columns 356 / 468 / 640 (creators native 640x480)\n", i + 1, names[i], seen[i] ? "captured" : "NOT REACHED");
    fclose(manifest);
    printf("enh_screens: %u immutable history checks; shots", retained_checks);
    for (unsigned i = 0; i < 16; i++) printf(" %s=%u", names[i], seen[i]);
    puts(""); return 0;
}
