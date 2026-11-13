/* Constructed, noninteractive screen calls. Compare the complete indexed store, including
 * wrapped destinations and the off-screen backdrop; no screenshot golden files. */
#include "script.h"
#include "c/credits.h"
#include "c/screens.h"
#include "c/render.h"
#include "c/system.h"
#include "c/platform.h"
#include "c/sqz.h"
#include "c/ports.h"
#include "c/vga.h"
#include "c/scene_screens.h"
#include "native/compositor.h"

static const char *assets;
static int failures, checks, map_frames, typed_code;
static unsigned status_reads, button_polls;
static void (*sink)(const SceneEvent *);

static void equal(const char *name)
{
    uint32_t a = 0, n = compositor_diff_vga(&a);
    checks++;
    if (n) { printf("FAIL %s: %u dots, first %04Xh\n", name, n, a); failures++; }
}

static uint8_t status(void) { return (++status_reads & 1) ? 0 : 8; }

static void button(int waiting)
{
    (void)waiting;
    /* Inspect mode 12 before the wait restores mode 0D. */
    if (!button_polls) equal("button wait / complete picture or credits");
    SET_DB(DS_byte_2832, (++button_polls & 1) ? 1 : 0);
    SET_DB(DS_byte_2810, 0); SET_DB(DS_byte_282D, 0);
}

static void observe(const SceneEvent *e)
{
    if (e->kind == SCENE_SCREEN_TRANSITION && e->a[0] == 5) equal("completed map frame");
    sink(e);
    if (e->kind == SCENE_SCREEN_MAP_EDGES) {
        map_frames++;
        SET_DB(DS_LastScanCode, 0x80);
        /* Exercise four input characters, wrong-code hold/reset, then cancellation. */
        if (typed_code && map_frames >= 10 && map_frames < 14)
            SET_DB(DS_LastScanCode, (uint8_t)(2 + map_frames - 10));
        if (map_frames >= 200) { SET_DB(DS_byte_2810, 1); SET_DB(DS_LastScanCode, 0); }
    }
}

static void reset(void)
{
    rt_boot(assets, 1997, 6, 1);
    pre2_host_poll = button; pre2_host_io = 0;
    vga_access_hook = 0; vga_status_hook = status;
    status_reads = button_polls = 0;
    compositor_init(); sink = pre2_scene_sink; pre2_scene_sink = observe;
    SET_CB(CS_OptionVByte, 5); SET_CB(CS_CpuClass, 3);
    SET_CB(CS_SbDetected, 0); SET_CW(CS_OptionBits, 0);
    SET_CW(CS_ConventionalTopSeg, 0xA000);
    SET_DW(DS_NextFreeSegment, 0x4000); SET_DW(DS_word_0039, 0x4000);
    SET_DW(DS_ModuleSegment, 0x3000); SET_DW(DS_word_003D, 0x7000);
    SET_DB(DS_byte_27E4, 0xFF);
    for (unsigned i = 0; i < 65536; i++) wr8(0x7000, (uint16_t)i, (uint8_t)(i * 73 + (i >> 7)));
    SetLevelVideoMode();
}

static void text(void)
{
    reset();
    ShowCreditsScreen(); equal("credits");
    SET_DW(DS_ClockYear, 1997); ShowLateYearCredits(); equal("late credits clear");
    SET_DW(DS_DisplayPage, 0xFFF0); SET_DW(DS_TextRowOffset, 0);
    SET_DW(DS_TextByteColumn, 0);
    PrintDecimalWord(0); PrintDecimalWord(65535);
    DrawFontChar('A'); DrawFontChar(' '); DrawFontChar('[');
    equal("decimal/text 16-bit destination wrap");
    ClearDisplayPage(); equal("display clear wraps");
    DisableVideoOutput(); EnableVideoOutput(); equal("blank/unblank keeps memory");
}

static void maps(void)
{
    for (typed_code = 0; typed_code < 2; typed_code++) {
        reset(); map_frames = 0;
        pre2_host_poll = 0;
        SET_DB(DS_ResourceFlags + 6, 8);
        int r = RunCallbackMapScreen(typed_code ? CS_UpdateCodeEntryFrame : CS_UpdateModeSelectFrame);
        equal(typed_code ? "ENTER CODE banks/ring" : "MODE banks/ring");
        if (r || map_frames < 200) { printf("FAIL callback result %d / %d frames\n", r, map_frames); failures++; }
    }
    reset(); pre2_host_poll = 0;
    SET_DB(DS_ResourceFlags + 5, 8); SET_DB(DS_CurrentLevel, 7);
    SET_DW(DS_word_7522, 0x0810); SET_DW(DS_word_62DA, 0); SET_DW(DS_word_667A, 0x6000);
    for (unsigned i = 0; i < 80; i++) wr8(0x6000, (uint16_t)i, (uint8_t)(i * 17));
    ShowWorldMapScrollIn(); equal("world-map marker and rolling columns");
}

static void pictures(void)
{
    reset(); ShowTitusScreen(); equal("TITUS");
    ShowPresentTitle(); equal("PRESENT logo/morph");
    /* CASTLE plus an independently constructed planar picture and backdrop. THEEND is
     * loaded by its own filename below, to avoid relying on a hard-coded table offset. */
    uint16_t seg = 0;
    if (LoadSqzFile(0x002C, &seg)) { failures++; return; }
    SET_DW(DS_LevelSegment, seg); ShowMode13PictureFadeIn(255, 0); equal("CASTLE");
    const char *end = "THEEND.SQZ";
    for (unsigned i = 0; i <= strlen(end); i++) SET_DB(0x100 + i, end[i]);
    SET_DW(DS_NextFreeSegment, 0x4000);
    if (LoadSqzFile(0x100, &seg)) { failures++; return; }
    SET_DW(DS_LevelSegment, seg); ShowMode13PictureFadeIn(255, 0); equal("THEEND");
    for (unsigned i = 0; i < 32768; i++) wr8(0x5000, (uint16_t)i, (uint8_t)(i * 31 + (i >> 4)));
    ShowPlanarPicture(0x5000); equal("planar picture");
    /* Seed the saved backdrop through a semantic picture upload, then compare copies. */
    scene_emit(SCENE_SCREEN_PICTURE, 6, 0x5000, 0, 0x3F40, 8000, 15, 0);
    SetCpuWriteMode();
    for (int p = 0; p < 4; p++) {
        port_out16(0x3C4, (uint16_t)(((1 << p) << 8) | 2));
        for (int i = 0; i < 8000; i++) vga_write((uint16_t)(0x3F40 + i), rd8(0x5000, (uint16_t)(p * 8000 + i)));
    }
    SET_DW(DS_DrawPage, 0xFFF0); RestoreBackdropToDrawPage(); equal("intro restore wraps");
    SET_DW(DS_DrawPage, 0x2000); SET_DB(DS_FineScroll, 185); CopyBackdropStrip(); equal("game-over backdrop strip");
    reset();
    SET_DB(DS_byte_2811, 1); SET_DB(DS_byte_282C, 1); SET_DB(DS_byte_2806, 1);
    if (TryShowIntroKeyed()) { printf("FAIL keyed intro gate\n"); failures++; }
    equal("keyed mode-12 intro / mode reset");
}

int main(int argc, char **argv)
{
    setvbuf(stdout, 0, _IONBF, 0);
    assets = arg_assets(argc, argv);
    printf("screens: text\n"); text();
    printf("screens: callback maps\n"); maps();
    printf("screens: pictures\n"); pictures();
    printf("screens: %d store checks, %d failures\n", checks, failures);
    return failures != 0;
}
