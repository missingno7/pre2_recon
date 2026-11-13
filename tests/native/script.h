/* Scripted runs shared by the native tests (header-only; one test program per .c file).
 *
 * The script is the host smoke test's (docs/smoke-test.md) in logical time: RETURN tapped
 * (0.1 s) every 1.2 s through the start-up screens, the front end and the map (hands off while
 * the map scrolls in), then RIGHT held in level 1. A scenario then writes the state that
 * triggers a transition 2 s into the level and taps RETURN through the screens that follow:
 *   none, warp (bonus room 0Ah), ending (THEEND), gameover, castle (beginner end), and
 *   attract (hands off from the front end: the attract demo, which needs year < 1996). */
#ifndef NATIVE_TEST_SCRIPT_H
#define NATIVE_TEST_SCRIPT_H
#include "c/pre2.h"
#include "c/game.h"
#include "native/machine.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    const char *scenario;
    uint64_t last, gameplay_ns;
    int held, down, poked, hands_off;
} Script;
static Script script;

static void script_reset(const char *scenario)
{
    memset(&script, 0, sizeof script);
    script.scenario = scenario ? scenario : "none";
}

static void script_poke(void)
{
    const char *s = script.scenario;
    if (!strcmp(s, "warp")) { SET_DB(DS_CurrentLevel, 3); SET_DB(DS_LevelEndMode, 2); }
    else if (!strcmp(s, "ending")) SET_DB(DS_EndSignal, 0xFF);
    else if (!strcmp(s, "gameover")) SET_DB(DS_EndSignal, 1);
    else if (!strcmp(s, "castle")) { SET_DB(DS_CurrentLevel, 7); SET_DB(DS_LevelEndMode, 1); SET_DB(DS_byte_B198, 0); }
}

/* rt_on_service */
static void script_service(void)
{
    Script *k = &script;
    uint64_t now = rt_now();
    if (!k->gameplay_ns && pre2_frames_run > 0) {
        k->gameplay_ns = now;
        if (k->down) { rt_key_at(0x9C, now); k->down = 0; }
    }
    if (!strcmp(k->scenario, "attract")) {
        if (!strcmp(pre2_phase, "RunFrontEnd()")) k->hands_off = 1;
        if (k->hands_off) { if (k->down) { rt_key_at(0x9C, now); k->down = 0; } return; }
    }
    int pokes = strcmp(k->scenario, "none") && strcmp(k->scenario, "attract");
    if (k->gameplay_ns && pokes && !k->poked && now - k->gameplay_ns >= 2000000000ull) {
        if (k->held) { rt_key_at(0xCD, now); k->held = 0; }
        script_poke(); k->poked = 1; k->last = now;
    }
    if (k->gameplay_ns && !k->poked) { if (!k->held) { rt_key_at(0x4D, now); k->held = 1; } return; }
    if (k->down && now - k->last >= 100000000ull) { rt_key_at(0x9C, now); k->down = 0; }
    if (!strcmp(pre2_phase, "ShowWorldMapScrollIn()")) return;
    if (!k->down && now - k->last >= 1200000000ull) { rt_key_at(0x1C, now); k->down = 1; k->last = now; }
}

static uint64_t fnv64(const void *p, size_t n, uint64_t h)
{
    const uint8_t *b = (const uint8_t *)p;
    for (size_t i = 0; i < n; i++) { h ^= b[i]; h *= 1099511628211ull; }
    return h;
}
#define FNV0 1469598103934665603ull

static const char *arg_assets(int argc, char **argv)
{
    for (int i = 1; i + 1 < argc; i++) if (!strcmp(argv[i], "--assets")) return argv[i + 1];
    return "assets";
}
#endif
