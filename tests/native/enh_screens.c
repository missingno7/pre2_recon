/* Constructed source/state boundaries, independent of the original executable. */
#include "script.h"
#include "c/scene_screens.h"
#include "c/scene_game.h"
#include "c/vga.h"
#include "c/platform.h"
#include "c/render.h"
#include "c/credits.h"
#include "native/compositor.h"
#include "native/enh_screens.h"
#include <assert.h>
#undef assert
#define assert(test) do { if (!(test)) { fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #test); exit(1); } } while (0)
#include <math.h>

static void (*sink)(const SceneEvent *);
static Pre2Frame frame;
static FrameScreenView old;
static uint8_t pixels[2048 * 480], expected[64000];
static unsigned checks;
static void event(const SceneEvent *e) { sink(e); }
static void publish(uint16_t start)
{
    compositor_publish(0, start, &frame);
    assert(frame.screen_view.valid);
}
static EnhImage image(unsigned width)
{
    EnhImage im = {0}; im.index = pixels; im.capacity = sizeof pixels; im.stride = width;
    return im;
}
static void parity(void)
{
    EnhImage im = image(frame.w);
    assert(!enh_screens_render(&frame.screen_view, frame.palette, frame.pel_mask,
        frame.screen_view.pan_x, frame.screen_view.pan_y, frame.w, frame.h, &im));
    assert(!memcmp(pixels, frame.index, (size_t)frame.w * frame.h)); checks++;
}
static void menu(void)
{
    SetLevelVideoMode();
    /* Two low-channel background images; high channels deliberately look like
     * solid text ghosts everywhere in the ring. They must stay in the aperture. */
    for (unsigned i = 0; i < 65536; i++) wr8(0x6000, (uint16_t)i, (uint8_t)(i * 73u + (i >> 8)));
    scene_emit(SCENE_SCREEN_PICTURE, 6, 0x6000, 0, 0, 8192, 3, 0);
    for (unsigned i = 0; i < 65536; i++) wr8(0x7000, (uint16_t)i, 255);
    scene_emit(SCENE_SCREEN_PICTURE, 6, 0x7000, 0, 0, 8192, 12, 0);
    scene_emit(SCENE_SCREEN_FONT_BANKS, 1, 0x6000);
    pre2_vga.crtc[1] = 0x26; pre2_vga.crtc[0x17] = 0xe2; pre2_vga.attr[0x13] = 7;
    publish(0x1fff); parity(); old = frame.screen_view; enh_screen_view_retain(&old); memcpy(expected, frame.index, sizeof expected);
    const unsigned widths[] = {320, 321, 356, 468, 640, 2048};
    for (unsigned k = 0; k < sizeof widths / sizeof *widths; k++) {
        unsigned w = widths[k], pad = (w - 320) / 2;
        EnhImage im = image(w);
        assert(!enh_screens_render(&old, frame.palette, frame.pel_mask, old.pan_x, old.pan_y, w, 200, &im));
        for (unsigned y = 0; y < 200; y++) for (unsigned x = 0; x < w - 8; x++) {
            uint8_t c = pixels[y * w + x];
            if (x < pad || x >= pad + 312) assert(c < 4);
            else assert(c >= 12);
        }
        checks++;
    }
    /* The source and faithful surfaces can both be destroyed after capture. */
    memset(comp_planar, 5, sizeof comp_planar);
    for (unsigned i = 0; i < 65536; i++) wr8(0x6000, (uint16_t)i, 0);
    scene_emit(SCENE_SCREEN_CLEAR, 2, 0, 8192);
    EnhImage im = image(320);
    assert(!enh_screens_render(&old, frame.palette, frame.pel_mask, old.pan_x, old.pan_y, 320, 200, &im));
    assert(!memcmp(pixels, expected, sizeof expected)); checks++;
    assert(old.layers && old.font_banks);
    assert(old.layers->kind == FRAME_SCREEN_FONT && old.layers->previous->image);
    /* Fractional indexed sampling floors, including negative positions. */
    assert(!enh_screens_render(&old, frame.palette, frame.pel_mask, old.pan_x + .99, old.pan_y + .99, 320, 200, &im));
    assert(!memcmp(pixels, expected, sizeof expected)); checks++;
    assert(enh_screens_render(&old, frame.palette, frame.pel_mask, NAN, 0, 320, 200, &im) == -1);
    assert(enh_screens_render(&old, frame.palette, frame.pel_mask, old.pan_x, 0, 319, 200, &im) == -1);
    im.capacity = 63999;
    assert(enh_screens_render(&old, frame.palette, frame.pel_mask, old.pan_x, 0, 320, 200, &im) == -1); checks++;
    enh_screen_view_release(&old); memset(&old, 0, sizeof old);
}
static void clears(void)
{
    SetLevelVideoMode();
    /* Give logical zero a visible value and retain a separate literal black. */
    pre2_vga.attr[0] = 5; pre2_vga.dac[5][0] = 63;
    for (unsigned i = 0; i < 65536; i++) wr8(0x6000, (uint16_t)i, 255);
    scene_emit(SCENE_SCREEN_PICTURE, 6, 0x6000, 0, 0, 8000, 15, 0);
    scene_emit(SCENE_SCREEN_CLEAR, 2, 400, 40);
    publish(0); parity();
    assert(frame.screen_view.rows[10].side_clear);
    assert(!frame.screen_view.rows[9].side_clear);
    EnhImage im = image(468);
    assert(!enh_screens_render(&frame.screen_view, frame.palette, frame.pel_mask, 0, 0, 468, 200, &im));
    assert(pixels[10 * 468] == 5 && pixels[10 * 468 + 467] == 5);
    assert(pixels[9 * 468] == 0 && pixels[9 * 468 + 467] == 0); checks++;
}
static void map(void)
{
    SetLevelVideoMode(); pre2_vga.crtc[1] = 0x26; pre2_vga.crtc[0x17] = 0xe2;
    for (unsigned i = 0; i < 65536; i++) wr8(0x6000, (uint16_t)i, 255);
    scene_emit(SCENE_SCREEN_WORLD_MAP, 1, 0x6000);
    /* Observe before and between plane uploads. Every unseen channel stays
     * hidden even though the immutable stamped master is already complete. */
    scene_emit(SCENE_SCREEN_TRANSITION, 6, 5, 2, 400, 640, 400, 0);
    for (unsigned p = 0; p < 4; p++) {
        scene_emit(SCENE_SCREEN_WORLD_COLUMN, 2, 400, 1 << p);
        publish(50); parity();
        EnhImage im = image(640);
        assert(!enh_screens_render(&frame.screen_view, frame.palette, frame.pel_mask, 400, 0, 640, 200, &im));
        for (unsigned x = 240; x < 320; x++) assert(pixels[x] == (1u << (p + 1)) - 1u);
        for (unsigned x = 0; x < 240; x++) assert(pixels[x] == 0);
        for (unsigned x = 0; x < 320; x++) assert(pixels[199 * 640 + x] == 0);
        checks++;
    }
    scene_emit(SCENE_SCREEN_WORLD_COLUMN, 2, 632, 15);
    scene_emit(SCENE_SCREEN_TRANSITION, 6, 5, 2, 639, 640, 639, 0);
    publish(79); parity();
    for (unsigned p = 0; p < 4; p++) assert(frame.screen_view.revealed[p] == 632);
    EnhImage im = image(2048);
    assert(!enh_screens_render(&frame.screen_view, frame.palette, frame.pel_mask, 639, 0, 2048, 200, &im));
    for (unsigned x = 2040; x < 2048; x++) assert(pixels[x] == frame.screen_view.border);
    checks++;
}
static void credits(void)
{
    SetLevelVideoMode(); SET_DW(DS_word_003D, 0x7000);
    ShowCreditsScreen(); publish(0); parity();
    assert(frame.screen_view.kind == FRAME_SCREEN_CREDITS);
    /* Even a caller that does not publish CRT frames cannot grow an unbounded
     * operation recipe. The compiled native surface retains the composed state. */
    enh_screen_memory_reset_peaks();
    for (unsigned i = 0; i < 10000; i++) scene_emit(SCENE_SCREEN_CHAR, 3, 0x7000, 'A', (int)(i % 4000));
    assert(enh_screen_memory_stats().live_layers <= 256);
    assert(enh_screen_memory_stats().peak_bytes < 8u * 1024u * 1024u);
    publish(0); parity();
}
static void high_resolution(void)
{
    bios_set_mode(0x12);
    for (unsigned i = 0; i < 65536; i++) wr8(0x6000, (uint16_t)i, (uint8_t)(i * 29u));
    scene_emit(SCENE_SCREEN_PICTURE, 6, 0x6000, 0, 0, 38400, 15, 0);
    publish(0); parity(); assert(frame.w == 640 && frame.h == 480);
    EnhImage im = image(640);
    assert(enh_screens_render(&frame.screen_view, frame.palette, frame.pel_mask, 0, 0, 468, 480, &im) == -1);
    assert(enh_screens_render(&frame.screen_view, frame.palette, frame.pel_mask, 0, 0, 640, 200, &im) == -1);
    checks++;
}
int main(int argc, char **argv)
{
    rt_boot(arg_assets(argc, argv), 1995, 6, 1); compositor_init();
    sink = pre2_scene_sink; pre2_scene_sink = event;
    pre2_phase = "constructed screens"; pre2_host_poll = 0; pre2_host_io = 0;
    menu(); clears(); map(); credits(); high_resolution();
    printf("enh_screens: %u constructed checks, ring/text/clear/reveal/retention/640x480 all exact\n", checks);
    compositor_init(); assert(enh_screen_memory_stats().live_bytes == 0);
    return 0;
}
