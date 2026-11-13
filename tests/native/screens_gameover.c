/* Bounded GAME OVER construction: real decoded sprite assets, timer-driven exit, and
 * comparison of every store byte outside this pass's sprite rectangles on both pages. */
#include "script.h"
#include "c/screens.h"
#include "c/sprites.h"
#include "c/render.h"
#include "c/platform.h"
#include "c/vga.h"
#include "c/scene_screens.h"
#include "native/compositor.h"

static uint8_t rectangles[65536];
static void (*sink)(const SceneEvent *);
static unsigned failures, checked, last_tick;
static uint8_t status(void) { static unsigned n; return (++n & 1) ? 0 : 8; }

static void event(const SceneEvent *e)
{
    sink(e);
    if (e->kind != SCENE_SCREEN_COPY || e->a[2] != 0x1B80) return;
    uint16_t page = (uint16_t)e->a[1];
    for (int i = 0; i < 7040; i++) rectangles[(uint16_t)(page + i)] = 0;
    for (uint16_t s = 0x4F0A; s <= 0x5720; s = (uint16_t)(s + 18)) {
        uint16_t fw = DW(s + 4);
        if (fw == 0xFFFF) continue;
        uint16_t bx = (uint16_t)((fw & 0x1FFF) << 1);
        int w = DB(DS_FrameWidths + bx), h = DB(DS_FrameHeights + bx);
        int hx = (int8_t)DB(DS_FrameHotX + bx);
        if (fw & 0x8000) hx = (int8_t)(w - (uint8_t)hx);
        int x = (int16_t)DW(s) - hx;
        int bottom = (int16_t)DW(s + 2) + (int8_t)DB(DS_FrameHotY + bx) - DB(DS_FineScroll);
        int left = x < 0 ? 0 : x / 8, right = (x + w + 7) / 8, top = bottom - h;
        if (top < 0) top = 0;
        if (bottom > 176) bottom = 176;
        if (right > 40) right = 40;
        for (int y = top; y < bottom; y++) for (int b = left; b < right; b++)
            rectangles[(uint16_t)(page + y * 40 + b)] = 1;
    }
}

static void poll(int waiting)
{
    if (!waiting) return;
    if (last_tick != DW(DS_SnowFrameTick)) {
        last_tick = DW(DS_SnowFrameTick); checked++;
        unsigned bad = 0, first = 0;
        for (unsigned a = 0; a < 65536; a++) if (!rectangles[a]) for (unsigned d = 0; d < 8; d++) {
            unsigned colour = 0;
            for (int p = 0; p < 4; p++) colour |= ((pre2_vga.plane[p][a] >> (7 - d)) & 1) << p;
            if (comp_planar[a * 8 + d] != colour) { if (!bad) first = a; bad++; }
        }
        if (bad && !failures++) {
            printf("FAIL game-over tick %u: %u dots outside sprite rectangles, first %04Xh scroll %u\n", last_tick, bad, first, DB(DS_FineScroll));
            for (uint16_t s = 0x4F0A; s <= 0x5720; s = (uint16_t)(s + 18)) if (DW(s + 4) != 0xFFFF) {
                uint16_t bx = (uint16_t)((DW(s + 4) & 0x1FFF) << 1);
                printf("slot %04X frame %04X xy %d,%d wh %u,%u hot %d,%d\n", s, DW(s + 4), (int16_t)DW(s), (int16_t)DW(s + 2),
                    DB(DS_FrameWidths + bx), DB(DS_FrameHeights + bx), (int8_t)DB(DS_FrameHotX + bx), (int8_t)DB(DS_FrameHotY + bx));
            }
        }
    }
    SET_CW(CS_TimerTickCount, (uint16_t)(CW(CS_TimerTickCount) + 3));
    SET_DW(DS_word_27F0, (uint16_t)(DW(DS_word_27F0) + 3));
}

int main(int argc, char **argv)
{
    rt_boot(arg_assets(argc, argv), 1995, 6, 1); compositor_init();
    sink = pre2_scene_sink; pre2_scene_sink = event;
    pre2_host_poll = poll; pre2_host_io = 0; vga_status_hook = status; vga_access_hook = 0;
    SET_CB(CS_OptionVByte, 5); SET_CW(CS_OptionBits, 0); SET_CW(CS_ConventionalTopSeg, 0xA000);
    SET_DW(DS_NextFreeSegment, 0x4000); SET_DB(DS_ResourceFlags + 11, 8);
    SET_DB(DS_byte_27E4, 0xFF); SET_CB(CS_CpuClass, 3); SET_CB(CS_SbDetected, 0);
    SetLevelVideoMode();
    if (LoadSpriteSheet()) return 1;
    RunGameOverScreen();
    printf("screens_gameover: %u constructed frames, %u failures outside sprite rectangles\n", checked, failures);
    return failures || checked < 100;
}
