/* Program entry, level restart and the main frame loop (oracle: src/MODULE1.ASM
 * ProgramEntry cs:0008, LevelRestart cs:012F, MainFrameLoop cs:0214).
 *
 * The top level keeps the original call order exactly. Two stale-register contracts of the
 * original are explicit here:
 *  - UpdatePlayer consumes the CX that UpdateTerrainEntities leaves, which is always 0
 *    (its single exit is the `dec cx / je` that ends the 16-entity loop).
 *  - The render-slot fill at loc_01C4 (`rep stosw` of 40Bh words FFFFh at offset 4F0Ah)
 *    uses whatever ES the code before it left. On the CurrentLevel < 0Ah path that is A000h
 *    (ShowWorldMapScrollIn leaves ES = A000h; ActivateResourceSlot and SetLevelVideoMode
 *    preserve it), so the words go to video memory through the VGA write logic (the
 *    off-screen tile ring), not to RenderSlots (quirk; ResetRenderSlots clears the slots
 *    afterwards). The ES of the other path is tracked in `level_es`. EnterLevelView redraws
 *    the whole tile ring later in the level start, so the stray VRAM words are overwritten
 *    before anything shows them (a mutant writing elsewhere is not observable in tests).
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

/* ES as the original would hold it at loc_01C4 for the CurrentLevel >= 0Ah path; set by
 * the code that leads there. GAME_ES_UNKNOWN makes the fill fail loudly instead of guessing. */
#define GAME_ES_UNKNOWN 0xFFFFFFFFu
static uint32_t level_es = GAME_ES_UNKNOWN;   /* register model, not game state */

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

static int load(uint16_t name, uint16_t *seg) { return LoadSqzFile(name, seg); }

/* loc_01C4: rep stosw 40Bh x FFFFh at ES:4F0Ah. */
static int fill_render_slots(uint32_t es)
{
    if (es == GAME_ES_UNKNOWN) return GAME_UNSUPPORTED;
    for (uint16_t i = 0; i < 0x40B; i++) {
        uint16_t di = (uint16_t)(DS_RenderSlots + 2*i);
        if (es == 0xA000u) { vga_write(di, 0xFF); vga_write((uint16_t)(di + 1), 0xFF); }
        else wr16((uint16_t)es, di, 0xFFFF);
    }
    return 0;
}

int ProgramEntry(void)
{
    int r;
    uint16_t seg;
    parse_options();
    /* free paragraph after DATA: (B1BCh >> 4) + seg DATA + 1 */
    SET_DW(DS_NextFreeSegment, (uint16_t)((0xB1BCu >> 4) + DATA_SEG + 1));
    wr16(0, 0x90, CS_CriticalErrorHandler);              /* INT 24h vector */
    wr16(0, 0x92, CODE_SEG);
    SwapInt08Int09Vectors();
    SET_CW(CS_ConventionalTopSeg, (uint16_t)(platform_conventional_kb() << 6));   /* INT 12h */
    { int carry; ReadClockYear(&carry); }
    ProbeCpuClass();
    CalibrateJoystick();
    if ((r = CheckVgaAdapter())) return r;
    SetLevelVideoMode();
    if ((r = load(0x000C, &seg))) return r;             /* KEYB.SQZ */
    SET_DW(DS_NextFreeSegment, seg);
    SET_DW(DS_word_003D, seg);
    for (uint16_t i = 0; i < 0x800; i++) wr8(DATA_SEG, (uint16_t)(0x3F + i), rd8(seg, i));
    port_out8(0x40, 0x00); port_out8(0x40, 0x40);       /* PIT ch0 divisor 4000h */
    uint16_t keyb = seg;
    if ((r = load(0x0015, &seg))) return r;             /* ALLFONTS.SQZ */
    /* quirk: DS is still the KEYB segment here (push ds/pop ds around the load), so the
     * result goes to KEYB:003Dh, which is also where ALLFONTS was just loaded */
    wr16(keyb, 0x3D, seg);
    ShowLateYearCredits();
    if ((r = InitSoundDevices())) return r;
    if ((r = ShowTitusScreen())) return r;
    if ((r = ActivateResourceSlot(3))) return r;
    if ((r = ShowPresentTitle())) return r;
    if ((r = InitResourceMemory())) return r;
    if (CW(CS_OptionBits) & 2) {
        if ((r = load(0x0022, &seg))) return r;         /* FRONT.SQZ */
        SET_DW(DS_word_003B, seg);
    }
    if ((r = LoadSpriteSheet())) return r;
    SET_DW(DS_word_0039, DW(DS_NextFreeSegment));
    level_es = GAME_ES_UNKNOWN;
    return LevelRestart(DW(DS_NextFreeSegment));
}

int LevelRestart(uint16_t ax)
{
    int r;
    uint16_t seg;
    for (;;) {
        SET_DW(DS_NextFreeSegment, DW(DS_word_0039));
        if (ax) {
            FadeOutPaletteToBlack();
            SetLevelVideoMode();
            SET_DB(DS_Lives, 2);
            SET_DB(DS_BonusLetterMask, 0);
            SET_DB(DS_PlayerAttackPower, 0x14);
            SET_DB(DS_PlayerWeaponType, 0);
            SET_DB(DS_byte_6CA8, 0);
            if ((r = ActivateResourceSlot(3))) return r;
            if ((r = RunFrontEnd())) return r;
            level_es = GAME_ES_UNKNOWN;
        }
        uint8_t level = DB(DS_CurrentLevel);
        if (level >= 8 && level < 0x0A && DB(DS_byte_B198) == 0) {
            if ((r = load(0x002C, &seg))) return r;     /* CASTLE.SQZ */
            SET_DW(DS_NextFreeSegment, seg);
            SET_DW(DS_LevelSegment, seg);
            /* BX at the call: LoadSqzFile leaves the file handle in BX (its close) */
            ShowMode13PictureFadeIn(0xFF, CW(CS_SqzFileHandle));
            WaitForButtonPressRelease();
            ax = 1;
            continue;
        }
        if (DB(DS_CurrentLevel) < 0x0A) {
            SetLevelVideoMode();
            ShowWorldMapScrollIn();
            level_es = 0xA000u;
        }
        if ((r = LevelStart(level_es))) return r;
        r = MainFrameLoop(&ax);
        if (r) return r;
        level_es = GAME_ES_UNKNOWN;      /* whatever UpdateLevelState's exit leaves */
        /* UpdateLevelState set CF: restart with its AX */
    }
}

/* loc_01AB .. MainFrameLoop: activate the level's resource slot, set up and show the level.
 * `es` = the ES the original holds at loc_01C4 (see the file comment). */
int LevelStart(uint32_t es)
{
    int r;
    if ((r = ActivateResourceSlot(DB((uint16_t)(DS_byte_2D20 + DB(DS_CurrentLevel)))))) return r;
    if (DB(DS_CurrentLevel) < 0x0A) SetLevelVideoMode();
    if ((r = fill_render_slots(es))) return r;
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
    while ((r = MainFrameStep(ax_out)) == 0) { }
    return r == 1 ? 0 : r;
}

/* Test entries: c_loc_01AB runs LevelStart with ES from the register file; c_MainFrameLoop
 * runs CX iterations (stopping early when a restart is requested; AX = its value). */
EXPORT int c_loc_01AB(Regs *r) { return LevelStart(r->es == 0xA000u ? 0xA000u : r->es); }
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
