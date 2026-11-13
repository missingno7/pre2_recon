/* Constructed operation boundaries: persistent ring/backdrop surfaces, HUD, spans and points.
 * Both renderers start with identical patterned pixels; only the reference uses VGA planes. */
#include "c/pre2.h"
#include "c/vga.h"
#include "c/render.h"
#include "c/level.h"
#include "c/sprites.h"
#include "c/vram_store.h"
#include "c/scene_game.h"
#include "c/objects.h"
#include "native/compositor.h"
#include "native/machine.h"
#include <stdio.h>

void DrawLevel9Panel(uint8_t selector);
static unsigned cases, failed;
static void check(const char *operation, int parameter)
{
    uint32_t first = 0, n = compositor_diff_vga(&first); cases++;
    if (n && failed++ < 20) printf("%s %d: %u dots at %04X\n", operation, parameter, n, first);
}
static void seed(void)
{
    vga_reset(); pre2_vga.gc[6] = 1;
    for (unsigned a = 0; a < 65536; a++) {
        for (int p = 0; p < 4; p++) pre2_vga.plane[p][a] = (uint8_t)((a * (29 + p * 4) + p * 71) ^ (a >> 6));
        for (int x = 0; x < 8; x++) {
            uint8_t c = 0;
            for (int p = 0; p < 4; p++) c |= (uint8_t)(((pre2_vga.plane[p][a] >> (7 - x)) & 1) << p);
            *comp_px((uint16_t)a, x) = c;
        }
    }
}
int main(int argc, char **argv)
{
    rt_boot(argc > 2 ? argv[2] : "assets", 1995, 6, 1); compositor_init();
    SET_DW(DS_NextFreeSegment, 0x2530); SET_CW(CS_ConventionalTopSeg, 0xA000);
    SET_DW(DS_word_003D, 0x8000); SET_DW(DS_word_003B, 0x7000);
    for (unsigned a = 0; a < 65536; a++) {
        wr8(0x8000, (uint16_t)a, (uint8_t)((a * 93) ^ (a >> 3)));
        wr8(0x7000, (uint16_t)a, (uint8_t)((a * 43) ^ (a >> 5)));
    }
    if (LoadSpriteSheet()) return 1;
    seed();
    /* Synthetic opaque, transparent and mixed tiles, then the real classifier builds masks. */
    for (int t = 0; t < 256; t++) for (int p = 0; p < 4; p++) for (int a = 0; a < 32; a++) {
        uint8_t b = t % 3 == 0 ? 255 : t % 3 == 1 ? 0 : (uint8_t)((t * 43 + p * 71 + a * 39) & 0xAA);
        pre2_tile_cache.slot[t][p][a] = b; pre2_vga.plane[p][0x5E80 + t * 32 + a] = b;
    }
    scene_emit(SCENE_GAME_TILES, 0); ClassifyTileCache();
    SET_DW(DS_DrawPage, 0x2000); SET_DW(DS_DisplayPage, 0);
    for (int fine = 0; fine < 16; fine++) for (int t = 0; t < 3; t++) {
        SET_DB(DS_FineScroll, fine); SET_DW(DS_BackdropCursor, 0x7E80 + fine * 40);
        SetLatchCopyMode(); BlitTileCell((uint16_t)t, 0x5B00); check("tile", t * 16 + fine);
    }
    for (int col = 0; col < 20; col++) for (int row = 0; row < 12; row++) {
        SET_DW(DS_ColumnRing, col); SET_DW(DS_RowRing, row); ComputeRingTileOffset((uint16_t)col);
        SET_DB(DS_FineScroll, (col + row) & 15); SET_DW(DS_ShakeRows, (col + row) % 7);
        CopyRingWindowToPage(); check("ring", row * 20 + col);
    }
    SetCpuWriteMode(); SET_DW(DS_ShakeRows, 0); SET_DB(DS_FineScroll, 0);
    DrawStatusBarBackground(); check("hud background", 0);
    SetCpuWriteMode();
    ClearBonusLetterRow(); check("hud bonus", 0);
    for (int i = 0; i < 17; i++) { BlitHudGlyph((uint8_t)i, 0x3C00); check("hud glyph", i); }
    for (int i = 0; i < 10; i++) { BlitHudDigit((uint8_t)i, 0x3C00); check("hud digit", i); }
    for (int i = 0; i < 26; i++) { BlitHudSpriteGlyph((uint8_t)i, 0x3C00); check("hud letter", i); }
    for (int x = 0; x < 16; x++) for (int w = 0; w < 25; w++) {
        SetCpuWriteMode(); pre2_vga.seq[2] = 15;
        BlackoutSpan((uint16_t)x, (uint16_t)w, 90); check("iris span", x * 32 + w);
    }
    SET_DW(DS_LevelSegment, 0x6000); SET_DW(DS_CameraCol, 0); SET_DW(DS_CameraRow, 0);
    for (unsigned a = 0; a < 65536; a++) wr8(0x6000, (uint16_t)a, (uint8_t)(a * 73 + (a >> 8)));
    for (int i = 0; i < 3; i++) { DrawLevel9Panel((uint8_t)i); check("level-nine panel", i); }
    SET_DW(0x8167 + 10, 17);
    for (int y = 0; y < 12; y++) {
        SetCpuWriteMode(); DrawForegroundTile((uint16_t)(y << 8), 5); check("foreground", y);
    }
    SET_DW(DS_ScriptPointer, 0xF000); SET_DW(0xF000, 0xFFFF); SET_DW(DS_SnowFrameTick, 1);
    for (int wind = 1; wind <= 32; wind++) {
        SET_DW(DS_SnowWind, wind);
        for (int i = 0; i < 256; i++) SET_DW(DS_FlakeTable + i * 2, i * 43);
        UpdateWindAndFlakes(); check("flakes", wind);
    }
    for (int i = 0; i < 20; i++) SET_DW(DS_DebrisRecord + i * 6, 0xFFFF);
    for (int angle = 0; angle < 256; angle += 7) {
        SET_DW(DS_DebrisRecord, 40); SET_DW(DS_DebrisRecord + 2, 60);
        SET_DB(DS_DebrisRecord + 4, angle); SET_DB(DS_DebrisRecord + 5, 7);
        UpdateEffectParticles(); check("consumed particle", angle);
        if (DW(DS_DebrisRecord) != 0xFFFF) return 1;
    }
    /* A fly on every timer parity, against all sixteen background colours. */
    for (int c = 0; c < 16; c++) for (int timer = 1; timer < 4; timer++) {
        for (int p = 0; p < 4; p++) pre2_vga.plane[p][0x2190] = c & (1 << p) ? 255 : 0;
        for (int x = 0; x < 8; x++) *comp_px(0x2190, x) = (uint8_t)c;
        for (int i = 0; i < 20; i++) SET_DW(0x6EA9 + i * 8, 0x55AA);
        SET_DW(0x6EA9, 24); SET_DW(0x6EAB, 80); SET_DW(0x6EAD, 0); SET_DB(0x6EAF, timer);
        SET_DW(DS_CameraCol, 0); SET_DW(DS_CameraRow, 0);
        UpdateFireflies(); check("firefly", c * 4 + timer);
    }
    printf("game_operations: %u bounded cases, %u failures\n", cases, failed);
    return failed != 0;
}
