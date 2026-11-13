/* Bounded renderer test: real cels over a patterned backdrop, every pass and clip edge.
 * This calls only the loader and sprite routine, never boots or plays the DOS executable. */
#include "c/pre2.h"
#include "c/sprites.h"
#include "c/vga.h"
#include "c/scene_game.h"
#include "native/machine.h"
#include "native/compositor.h"
#include <stdio.h>
#include <string.h>

static void backdrop(void)
{
    for (unsigned a = 0; a < 65536; a++) {
        for (int p = 0; p < 4; p++) pre2_vga.plane[p][a] = (uint8_t)((a * (17 + p * 6) + p * 53) ^ (a >> 5));
        for (int x = 0; x < 8; x++) {
            uint8_t col = 0;
            for (int p = 0; p < 4; p++) col |= (uint8_t)(((pre2_vga.plane[p][a] >> (7 - x)) & 1) << p);
            *comp_px((uint16_t)a, x) = col;
        }
    }
}
int main(int argc, char **argv)
{
    rt_boot(argc > 2 ? argv[2] : "assets", 1995, 6, 1); compositor_init();
    SET_CW(CS_ConventionalTopSeg, 0xA000); SET_DW(DS_NextFreeSegment, 0x2530);
    if (LoadSpriteSheet()) return 1;
    pre2_vga.gc[6] = 1;
    static const int xs[] = { -100, -35, -20, -1, 0, 3, 7, 19, 100, 101, 104, 300, 305, 319, 320, 321, 340 };
    static const int ys[] = { -30, 1, 10, 60, 170, 190, 220 };
    unsigned cases = 0, bad = 0;
    for (int f = 0; f < 1024 && DB(DS_FrameWidths + f * 2); f++) {
        /* Every shipped frame gets a central draw. The large player cel and screen-space
         * frame exercise the complete position/pass matrix. */
        int nx = f == 0 || f == 309 ? (int)(sizeof xs / sizeof xs[0]) : 1;
        int ny = nx == 1 ? 1 : (int)(sizeof ys / sizeof ys[0]);
        for (int m = 0; m < 2; m++) for (int mode = 0; mode < 3; mode++)
        for (int xx = 0; xx < nx; xx++) for (int yy = 0; yy < ny; yy++) {
            backdrop();
            for (int i = 0; i < 116; i++) SET_DW(DS_RenderSlots + i * 18 + 4, 0xFFFF);
            SET_DW(DS_RenderSlots, nx == 1 ? 100 : xs[xx]);
            SET_DW(DS_RenderSlots + 2, ny == 1 ? 90 : ys[yy]);
            SET_DW(DS_RenderSlots + 4, f | (m ? 0x8000 : 0) | (mode == 2 ? 0x4000 : 0));
            SET_DB(DS_RenderSlots + 17, mode == 0 ? 10 : 0);
            SET_DW(DS_CameraCol, 0); SET_DW(DS_CameraRow, 0); SET_DB(DS_FineScroll, 0);
            SET_DW(DS_ShakeRows, 0); SET_DW(DS_DrawPage, 0x2000); SET_DW(DS_SnowFrameTick, 0);
            DrawSpriteObjects();
            uint32_t first = 0, diff = compositor_diff_vga(&first); cases++;
            if (diff) {
                if (bad++ < 15) printf("cel %d mirror %d mode %d x %d y %d: %u dots at %04X\n", f, m, mode, nx == 1 ? 100 : xs[xx], ny == 1 ? 90 : ys[yy], diff, first);
            }
        }
    }
    printf("game_sprites: %u bounded cases, %u failures\n", cases, bad);
    return bad != 0;
}
