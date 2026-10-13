/* Program entry, level restart and the main frame loop (oracle: src/MODULE1.ASM
 * ProgramEntry cs:0008, LevelRestart cs:012F, MainFrameLoop cs:0214).
 *
 * The top level keeps the original call order exactly. Two stale-register contracts of the
 * original are explicit here:
 *  - UpdatePlayer consumes the CX that UpdateTerrainEntities leaves, which is always 0
 *    (its single exit is the `dec cx / je` that ends the 16-entity loop).
 *  - The render-slot fill at loc_01C4 (`rep stosw` of 40Bh words FFFFh at offset 4F0Ah)
 *    uses whatever ES the code before it left: the ES register model pre2_es (c/pre2.h),
 *    which every translated routine leaves as the original leaves ES. On the CurrentLevel
 *    < 0Ah path that is A000h (ShowWorldMapScrollIn leaves ES = A000h; ActivateResourceSlot
 *    and SetLevelVideoMode preserve it), so the words go to video memory through the VGA
 *    write logic (the off-screen tile ring), not to RenderSlots (quirk; ResetRenderSlots
 *    clears the slots afterwards; EnterLevelView redraws the ring before anything shows
 *    it). On the CurrentLevel >= 0Ah paths (warps into bonus levels, their exits and
 *    restarts, a front end that ends on such a level) ES is what the last frame's or the
 *    front end's code left: LevelSegment, A000h, DATA or a loader's output segment.
 * Hardware/DOS set-up that the original does inline (INT 24h vector, INT 12h, PIT) goes
 * through the same memory/port model as everything else.
 */
#include "pre2.h"
#include "ports.h"
#include "vga.h"
#include "game.h"
#include "sqz.h"
#include "system.h"
#include "sound.h"
#include "sprites.h"
#include "render.h"
#include "level.h"
#include "player.h"
#include "objects.h"
#include "effects.h"
#include "screens.h"
#include "credits.h"
#include "startup.h"
#include "platform.h"

#define PSP_SEG (LOAD_SEG - 0x10u)

/* Instrumentation for the host (not game state): the top-level step running now, and the
 * number of main-loop frames run. */
const char *pre2_phase = "start";
uint32_t pre2_frames_run;
#define STEP(call) (pre2_phase = #call, call)
#define GAME_AT_FRONT_END 0x7F01      /* internal status of the test entries (see restart_to_level) */

/* ProgramEntry loc_0016..loc_007E: the switches in the PSP command tail. */
static void parse_options(void)
{
    uint16_t si = 0x80;
    uint16_t cx = rd8(PSP_SEG, si++);
    while (cx) {
        uint8_t al = rd8(PSP_SEG, si++);
        if (al != '/') { cx--; continue; }
        al = (uint8_t)(rd8(PSP_SEG, si++) & 0xDF);
        if (al == 'F') SET_CW(CS_OptionBits, CW(CS_OptionBits) & 0xFFFE);
        if (al == 'M') SET_CB(CS_SoundEnabledFlag, 0);
        if (al == 'A') SET_CW(CS_OptionBits, CW(CS_OptionBits) & 0xFFFD);
        if (al == 'E') SET_CW(CS_OptionBits, CW(CS_OptionBits) & 0xFFFB);
        if (al == 'X') SET_CW(CS_OptionBits, CW(CS_OptionBits) & 0xFFF7);
        if (al == 'L') SET_CW(CS_OptionBits, CW(CS_OptionBits) & 0xFFEF);
        if (al == 'D') SET_CW(CS_OptionBits, CW(CS_OptionBits) & 0xFFDF);
        if (al == 'B') SET_CW(CS_OptionBits, CW(CS_OptionBits) | 0x40);
        /* quirk: every letter falls through to the /V test; anything but V ends the scan */
        if (al != 'V') break;
        SET_CB(CS_OptionVByte, 1);
        if (--cx == 0) break;          /* the loop instruction after /V */
    }
}

/* Every LoadSqzFile call of ProgramEntry and LevelRestart is wrapped in push es / pop es. */
static int load(uint16_t name, uint16_t *seg)
{
    uint16_t es_saved = pre2_es;
    int r = LoadSqzFile(name, seg);
    SET_ES(es_saved);
    return r;
}

/* loc_01C4: rep stosw 40Bh x FFFFh at ES:4F0Ah. */
static void fill_render_slots(uint16_t es)
{
    for (uint16_t i = 0; i < 0x40B; i++) {
        uint16_t di = (uint16_t)(DS_RenderSlots + 2*i);
        seg_wr8(es, di, 0xFF);
        seg_wr8(es, (uint16_t)(di + 1), 0xFF);
    }
}

int ProgramEntry(void)
{
    int r;
    uint16_t seg;
    SET_ES(PSP_SEG);                                     /* DOS starts the program with ES = PSP */
    parse_options();
    /* free paragraph after DATA: (B1BCh >> 4) + seg DATA + 1 */
    SET_DW(DS_NextFreeSegment, (uint16_t)((0xB1BCu >> 4) + DATA_SEG + 1));
    wr16(0, 0x90, CS_CriticalErrorHandler);              /* INT 24h vector */
    wr16(0, 0x92, CODE_SEG);
    SwapInt08Int09Vectors();
    SET_CW(CS_ConventionalTopSeg, (uint16_t)(platform_conventional_kb() << 6));   /* INT 12h */
    { int carry; ReadClockYear(&carry); }
    STEP(ProbeCpuClass());
    STEP(CalibrateJoystick());
    if ((r = STEP(CheckVgaAdapter()))) return r;
    STEP(SetLevelVideoMode());
    if ((r = load(0x000C, &seg))) return r;             /* KEYB.SQZ */
    SET_DW(DS_NextFreeSegment, seg);
    SET_DW(DS_word_003D, seg);
    SET_ES(DATA_SEG);
    for (uint16_t i = 0; i < 0x800; i++) wr8(DATA_SEG, (uint16_t)(0x3F + i), rd8(seg, i));
    port_out8(0x40, 0x00); port_out8(0x40, 0x40);       /* PIT ch0 divisor 4000h */
    uint16_t keyb = seg;
    if ((r = load(0x0015, &seg))) return r;             /* ALLFONTS.SQZ */
    /* quirk: DS is still the KEYB segment here (push ds/pop ds around the load), so the
     * result goes to KEYB:003Dh, which is also where ALLFONTS was just loaded */
    wr16(keyb, 0x3D, seg);
    STEP(ShowLateYearCredits());
    if ((r = STEP(InitSoundDevices()))) return r;
    if ((r = STEP(ShowTitusScreen()))) return r;
    if ((r = ActivateResourceSlot(3))) return r;
    if ((r = STEP(ShowPresentTitle()))) return r;
    if ((r = STEP(InitResourceMemory()))) return r;
    if (CW(CS_OptionBits) & 2) {
        if ((r = load(0x0022, &seg))) return r;         /* FRONT.SQZ */
        SET_DW(DS_word_003B, seg);
    }
    if ((r = STEP(LoadSpriteSheet()))) return r;
    SET_DW(DS_word_0039, DW(DS_NextFreeSegment));
    return LevelRestart(DW(DS_NextFreeSegment));
}

/* LevelRestart up to the first MainFrameLoop frame: the front end when AX != 0, the CASTLE
 * picture (back to the front end), the world map below level 0Ah, the level start. Test
 * entries pass stop_at_front_end = 1 to return GAME_AT_FRONT_END when RunFrontEnd is due
 * (the oracle cases stop at its entry: it waits for input). */
static int restart_to_level(uint16_t *ax, int stop_at_front_end)
{
    int r;
    uint16_t seg;
    for (;;) {
        SET_DW(DS_NextFreeSegment, DW(DS_word_0039));
        if (*ax) {
            STEP(FadeOutPaletteToBlack());
            STEP(SetLevelVideoMode());
            SET_DB(DS_Lives, 2);
            SET_DB(DS_BonusLetterMask, 0);
            SET_DB(DS_PlayerAttackPower, 0x14);
            SET_DB(DS_PlayerWeaponType, 0);
            SET_DB(DS_byte_6CA8, 0);
            if ((r = ActivateResourceSlot(3))) return r;
            if (stop_at_front_end) return GAME_AT_FRONT_END;
            if ((r = STEP(RunFrontEnd()))) return r;
        }
        uint8_t level = DB(DS_CurrentLevel);
        if (level >= 8 && level < 0x0A && DB(DS_byte_B198) == 0) {
            if ((r = load(0x002C, &seg))) return r;     /* CASTLE.SQZ */
            SET_DW(DS_NextFreeSegment, seg);
            SET_DW(DS_LevelSegment, seg);
            /* BX at the call: LoadSqzFile leaves the file handle in BX (its close) */
            ShowMode13PictureFadeIn(0xFF, CW(CS_SqzFileHandle));
            STEP(WaitForButtonPressRelease());
            *ax = 1;
            continue;
        }
        if (DB(DS_CurrentLevel) < 0x0A) {
            STEP(SetLevelVideoMode());
            STEP(ShowWorldMapScrollIn());
        }
        return LevelStart();
    }
}

int LevelRestart(uint16_t ax)
{
    for (;;) {
        int r = restart_to_level(&ax, 0);
        if (r) return r;
        r = MainFrameLoop(&ax);
        if (r) return r;
        /* UpdateLevelState set CF: restart with its AX (ES as its exit left it) */
    }
}

/* loc_01AB .. MainFrameLoop: activate the level's resource slot, set up and show the level.
 * The fill at loc_01C4 uses the ES the code before it left (see the file comment). */
int LevelStart(void)
{
    int r;
    if ((r = ActivateResourceSlot(DB((uint16_t)(DS_byte_2D20 + DB(DS_CurrentLevel)))))) return r;
    if (DB(DS_CurrentLevel) < 0x0A) STEP(SetLevelVideoMode());
    fill_render_slots(pre2_es);
    if ((r = LoadLevel(DB(DS_CurrentLevel)))) return r;
    SetLevelPalette();
    ResetLevelState();
    ResetRenderSlots();
    EnterLevelView();
    DrawStatusBarBackground();
    CopyRingWindowToPage();
    PresentFrame();
    SET_DW(DS_word_A341, 0);
    SET_DB(DS_SpawnSideToggle, 0);
    SET_DB(DS_WindRandA, 5);
    SET_DB(DS_WindRandB, 0x22);
    SET_DB(DS_WindRandC, 0x86);
    SET_DW(DS_WindRandWord, 0xE58D);
    SET_DB(DS_LastScanCode, 0);
    return 0;
}

/* One iteration of MainFrameLoop. Returns 0 to continue, 1 when UpdateLevelState set CF
 * (*ax_out = its AX: LevelRestart follows), or an error status. */


int MainFrameStep(uint16_t *ax_out)
{
    int r;
    pre2_frames_run++;
    {
        UpdatePopupRing();
        UpdateFollowerSlots();
        if ((r = UpdateObjects())) return r;
        if ((r = UpdatePlayerProjectiles())) return r;
        UpdateBurstParticles();
        TickDebrisSlots();
        UpdateTerrainEntities();
        if ((r = UpdatePlayer(0))) return r;           /* CX = 0 from UpdateTerrainEntities */
        ProcessPlayerContacts();
        BuildEffectDrawList();
        TryCaveTrigger();
        UpdateProximityTriggers();
        UpdateCameraFollow();
        AnimateTiles();
        RedrawOverlayTiles();
        CopyRingWindowToPage();
        UpdateEffectParticles();
        DrawSpriteObjects();
        DrawForegroundOverSprites();
        UpdateFireflies();
        UpdateWindAndFlakes();
        uint16_t ax = 0;
        int cf = UpdateLevelState(&ax);
        if (cf < 0 || cf > 1) return cf;               /* an unsupported path inside */
        if (cf) { *ax_out = ax; return 1; }
        if ((r = UpdateHud())) return r;
        PresentFrame();
        StepPaletteFade();
        sub_67D7();
        UpdateCameraShake();
    }
    return 0;
}

int MainFrameLoop(uint16_t *ax_out)
{
    int r;
    pre2_phase = "MainFrameLoop";
    while ((r = MainFrameStep(ax_out)) == 0) { }
    return r == 1 ? 0 : r;
}

/* Test entries: c_loc_01AB runs LevelStart (ES from the register file via pre2_es); c_MainFrameLoop
 * runs CX iterations (stopping early when a restart is requested; AX = its value). */
EXPORT int c_loc_01AB(Regs *r) { (void)r; return LevelStart(); }
/* c_LevelRestart: LevelRestart with AX up to the first frame (the oracle stops at
 * MainFrameLoop) or up to RunFrontEnd (the oracle stops at its entry). c_MainFrameLoopRestart:
 * up to CX frames; a restart requested by UpdateLevelState runs on to the next level's first
 * frame or to RunFrontEnd (AX = the restart's AX). */
/* c_parse_options: the switch scan of the PSP command tail (the oracle stops at loc_007E). */
EXPORT int c_parse_options(Regs *r) { (void)r; parse_options(); return 0; }
EXPORT int c_LevelRestart(Regs *r)
{
    uint16_t ax = r->ax;
    int st = restart_to_level(&ax, 1);
    return st == GAME_AT_FRONT_END ? 0 : st;
}
EXPORT int c_MainFrameLoopRestart(Regs *r)
{
    for (uint16_t i = 0; i < r->cx; i++) {
        uint16_t ax = 0;
        int st = MainFrameStep(&ax);
        if (st == 1) {
            r->ax = ax;
            st = restart_to_level(&ax, 1);
            return st == GAME_AT_FRONT_END ? 0 : st;
        }
        if (st) return st;
    }
    return 0;
}
EXPORT int c_MainFrameLoop(Regs *r)
{
    for (uint16_t i = 0; i < r->cx; i++) {
        uint16_t ax = 0;
        int st = MainFrameStep(&ax);
        if (st == 1) { r->ax = ax; return 0; }
        if (st) return st;
    }
    return 0;
}
