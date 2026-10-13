/* Per-frame object update (oracle: src/MODULE3.ASM UpdateObjects cs:6822 and every routine
 * whose entry lies in cs:6822..8294): terrain response, the level-9 script and panel, the
 * level-5 sequence, the boss fight, the child handlers (ChildHandlerTable), the entity
 * handlers (EntityHandlerTable), the spawn tails, culling, timers, script walkers, the debris
 * and child pools, and the boss hazard and contact tests.
 *
 * Records are DATA at their original offsets: child slots 4FD0h.., particles 50A8h..,
 * entities at 8489h (a stride byte first; a stride >= 32h ends the list), debris 7DE6h,
 * level-5 projectiles 7DAFh. The handler dispatch reads the table word from the code segment
 * (CW(CS_ChildHandlerTable + index)) and switches on it: a word that is not one of the 26
 * handler entries is reported as OBJ_DISPATCH_UNSUPPORTED instead of being run.
 *
 * Routines of the other regions are called through their owners' headers (effects.h,
 * player.h, sprites.h). */
#include "pre2.h"
#include "render.h"
#include "system.h"
#include "vga.h"
#include "objects.h"
#include "effects.h"
#include "player.h"
#include "sprites.h"

/* ---- layout ---------------------------------------------------------------------------- */

#define CHILD_BASE      ((uint16_t)(DS_RenderSlots + 11 * RS_SIZE))  /* 4FD0h, 12 child slots */
#define PARTICLE_BASE   ((uint16_t)(DS_RenderSlots + 23 * RS_SIZE))  /* 50A8h, 32 particles */
#define PARTICLE_END    ((uint16_t)(PARTICLE_BASE + PARTICLE_SLOTS * RS_SIZE))
#define PLAYER_REC      ((uint16_t)(DS_RenderSlots + RS_SIZE))       /* 4F1Ch (PlayerX) */
#define HAZARD_SLOT0    ((uint16_t)DS_RenderSlots)                   /* 4F0Ah */
#define HAZARD_SLOT1    ((uint16_t)(DS_RenderSlots + 2 * RS_SIZE))   /* 4F2Eh, four slots */
#define ENTITY_LIST     0x8489u                                      /* no label */
#define GAUGE_SLOTS     0x56A2u                                      /* no label: 8 gauge sprites */
#define MIRROR_SLOTS    0x55EEu                                      /* no label: 5 mirrors */
#define LEVEL5_SLOT     ((uint16_t)DS_L5Projectile)                  /* 7DAFh, five 0Bh records */
#define DEBRIS_BASE     ((uint16_t)DS_DebrisRecord)                  /* 7DE6h, 20 records */
#define ENTITY_SCRIPT   0xA86Fu                                      /* no label: script lookup */
#define FRAME_MAP       0xA801u                                      /* no label: frame map */
#define HOP_TABLE_BASE  0xA6EDu                                      /* no label: boss key table */
#define BOSS_OPS_BASE   0xA571u                                      /* no label: boss op table */

#define RB(base, o)     DB((uint16_t)((base) + (o)))
#define RW(base, o)     DW((uint16_t)((base) + (o)))
#define WB(base, o, v)  SET_DB((uint16_t)((base) + (o)), (uint8_t)(v))
#define WW(base, o, v)  SET_DW((uint16_t)((base) + (o)), (uint16_t)(v))

/* ---- 16-bit and 8-bit helpers in the ASM's own terms ------------------------------------- */

static uint16_t neg16(uint16_t v) { return (uint16_t)(0u - v); }
static uint8_t  neg8(uint8_t v)   { return (uint8_t)(0u - v); }
static uint16_t abs16(uint16_t v) { return (int16_t)v < 0 ? neg16(v) : v; }
static uint16_t sar16(uint16_t v, unsigned n) { return (uint16_t)((int16_t)v >> n); }
static uint8_t  sar8(uint8_t v, unsigned n)   { return (uint8_t)((int8_t)v >> n); }
static uint8_t  sat_inc8(uint8_t v) { return v == 0xFF ? 0xFF : (uint8_t)(v + 1); }  /* add/sbb */
static uint8_t  sat_dec8(uint8_t v) { return v == 0 ? 0 : (uint8_t)(v - 1); }        /* sub/adc */

/* forward declarations (routines defined later in this file) */
void DrainPlayerEnergy(void);
void SpawnDriftParticle(void);
void SpawnFallingParticle(void);
void IncrementBossCounter(void);

/* ---- scripts, pools, culling, timers --------------------------------------------------- */

/* ComputeBobOffset (cs:6A7D): AX = signed offset from the motion class AL (translated through
 * ds:TileSolidProps); the child's X low nibble gives the phase. BX and DX are preserved. */
uint16_t ComputeBobOffset(uint16_t si, uint8_t al)
{
    uint8_t cls = DB((uint16_t)(DS_TileSolidProps + al));
    if (!(cls & 0x30)) return cbw(cls);
    uint8_t q = (uint8_t)((RW(si, 0) & 0x0F) / 3);       /* div bl: quotient in AL */
    if (cls & 0x10) return cbw((uint8_t)(q + (cls & 0x0F)));
    return cbw((uint8_t)(neg8(q) + (cls & 0x0F)));
}

/* ScriptStepBack (cs:8048): the script pointer +0Ch moves back to the previous jump word. */
void ScriptStepBack(uint16_t si)
{
    uint16_t bx = RW(si, RS_SCRIPT);
    do {
        bx = (uint16_t)(bx - 2);
    } while ((int16_t)DW(bx) >= 0);
    WW(si, RS_SCRIPT, bx);
}

/* ScriptStepForward (cs:8058): the script pointer +0Ch moves past the next jump word. */
void ScriptStepForward(uint16_t si)
{
    uint16_t bx = RW(si, RS_SCRIPT);
    while ((int16_t)DW(bx) >= 0) bx = (uint16_t)(bx + 2);
    bx = (uint16_t)(bx + 2);
    WW(si, RS_SCRIPT, bx);
}

/* AllocChildRecord (cs:806C): the lowest free child slot. Returns CF (1 = all 12 in use). */
int AllocChildRecord(uint16_t *di)
{
    uint16_t p = CHILD_BASE;
    for (unsigned i = 0; i < CHILD_SLOTS; i++, p = (uint16_t)(p + RS_SIZE)) {
        if (RW(p, RS_FRAME) == 0xFFFF) { *di = p; return 0; }
    }
    *di = p;                    /* full: DI is one past the last slot (50A8h), as the ASM leaves it */
    return 1;
}

/* AllocDebrisRecord (cs:8014): the first free debris record. No bound: when all 20 are in use
 * the search runs into the xlat table at ds:SideTileClassTable (see the ASM notes). */
uint16_t AllocDebrisRecord(void)
{
    uint16_t di = DEBRIS_BASE;
    while (DW(di) != 0xFFFF) di = (uint16_t)(di + DEBRIS_SIZE);
    return di;
}

/* IsInCameraWindow (cs:8022): CF = 1 when the world point (AX, DX) is outside the window. */
int IsInCameraWindow(uint16_t ax, uint16_t dx)
{
    ax = (uint16_t)(sar16(ax, 4) - DW(DS_CameraCol));
    if ((int16_t)ax < -2 || (int16_t)ax > 0x16) return 1;
    dx = (uint16_t)(sar16(dx, 4) - DW(DS_CameraRow));
    if ((int16_t)dx < -2 || (int16_t)dx > 0x0D) return 1;
    return 0;
}

/* TickEntityTimer (cs:8001): owner +7 saturates up; CF = 1 while (+7 >> 2) < owner +6. */
int TickEntityTimer(uint16_t owner)
{
    uint8_t t = sat_inc8(RB(owner, ENT_TIMER));
    WB(owner, ENT_TIMER, t);
    return (uint8_t)(t >> 2) < RB(owner, ENT_PERIOD);
}

/* loc_7CFF: the full death path of a child (owner timer cleared, record retired, owner flag
 * 04h cleared, and the owner entity retired unless flag 02h keeps it). */
static void ChildDeath(uint16_t si)
{
    uint16_t owner = RW(si, RS_OWNER);
    WB(owner, ENT_TIMER, 0);
    WW(si, RS_FRAME, 0xFFFF);
    WB(owner, ENT_FLAGS, RB(owner, ENT_FLAGS) & 0xFB);
    if (!(RB(owner, ENT_FLAGS) & 2)) WW(owner, ENT_SPRITE, 0xFFFF);
}

/* loc_7CDA: the state FFh branch shared by the handlers (falling child or death). */
static void ChildFallOrDie(uint16_t si)
{
    if (RB(si, RS_STATE) != 0xFF) return;
    uint16_t owner = RW(si, RS_OWNER);
    if (!(RB(owner, ENT_FLAGS) & 1)) { ChildDeath(si); return; }
    if (!(RB(si, RS_FLAGS) & 0x20)) {
        if ((int16_t)DW(DS_PlayerY) < (int16_t)RW(si, RS_Y)) { ChildDeath(si); return; }
    }
    if (!((int16_t)RW(si, RS_YVEL) >= (int16_t)0xF0)) WW(si, RS_YVEL, RW(si, RS_YVEL) + 0x0F);
}

/* CullChildAt (cs:8089): AX/DX = the position tested (owner or child). Skipped in state FFh or
 * when drawn (flag 20h). Far (|dX| > 140h or |dY| > 12Ch): state >= 0Ah takes the death path,
 * lower states retire the child and clear owner flag 04h and timer. */
void CullChildAt(uint16_t si, uint16_t ax, uint16_t dx)
{
    uint16_t bx = RW(si, RS_OWNER);
    if (RB(si, RS_STATE) == 0xFF) return;
    if (RB(si, RS_FLAGS) & 0x20) return;
    ax = abs16((uint16_t)(ax - DW(DS_PlayerX)));
    if (ax <= 0x140) {
        ax = abs16((uint16_t)(dx - DW(DS_PlayerY)));
        if (ax <= 0x12C) return;
    }
    if (RB(si, RS_STATE) >= 0x0A) { ChildDeath(si); return; }
    WW(si, RS_FRAME, 0xFFFF);
    WB(bx, ENT_FLAGS, RB(bx, ENT_FLAGS) & 0xFB);
    WB(bx, ENT_TIMER, 0);
}

/* CullChildFarFromPlayer (cs:8084): tests the child's own position. */
static void CullChildFarFromPlayer(uint16_t si)
{
    CullChildAt(si, RW(si, RS_X), RW(si, RS_Y));
}

/* SpawnDebrisLine (cs:7FD9): three debris records at the owner's X and Y - 18h; kind AL,
 * phase stepped by DL >> 2. */
void SpawnDebrisLine(uint16_t owner, uint8_t al, uint8_t dl)
{
    uint8_t step = sar8(dl, 2);
    uint8_t ch = step;
    for (int i = 0; i < 3; i++) {
        uint16_t di = AllocDebrisRecord();
        WW(di, DEBRIS_X, RW(owner, ENT_BOX));
        WW(di, DEBRIS_Y, (uint16_t)(RW(owner, 0x0B) - 0x18));
        WB(di, DEBRIS_KIND, al);
        WB(di, DEBRIS_PHASE, ch);
        ch = (uint8_t)(ch + step);
    }
}

/* SkipScriptToMarker (cs:80CB): record DI's script pointer moves past the next word 7D00h. */
void SkipScriptToMarker(uint16_t di)
{
    uint16_t si = RW(di, RS_SCRIPT);
    do {
        si = (uint16_t)(si + 2);
    } while (DW(si) != 0x7D00);
    si = (uint16_t)(si + 2);
    WW(di, RS_SCRIPT, si);
}

/* SetChildOrbitPosition (cs:7B53): X and Y from the owner's angle (+0Fh) and radius (+0Dh),
 * then SpawnDebrisLine. CL is 4 at every call (set by UpdateOrbitChild). */
static void SetChildOrbitPosition(uint16_t si, uint16_t owner)
{
    uint8_t a = RB(owner, 0x0F);
    uint8_t ch = RB(owner, 0x0D);
    uint8_t s = sar8(DB((uint16_t)(DS_SineTable + a)), 2);
    int16_t prod = (int16_t)((int8_t)s * (int8_t)ch);
    WW(si, RS_X, (uint16_t)(sar16((uint16_t)prod, 4) + RW(owner, ENT_BOX)));
    s = sar8(DB((uint16_t)(DS_byte_7090 + a)), 2);
    prod = (int16_t)((int8_t)s * (int8_t)ch);
    WW(si, RS_Y, (uint16_t)(sar16((uint16_t)prod, 4) + RW(owner, 0x0B)));
    SpawnDebrisLine(owner, RB(owner, 0x0F), RB(owner, 0x0D));
}

/* SetChildDirToPlayer (cs:7A50): XVel = +1 when the child is at or left of the player. */
static void SetChildDirToPlayer(uint16_t si)
{
    uint16_t dx = 1;
    if ((int16_t)RW(si, RS_X) > (int16_t)DW(DS_PlayerX)) dx = neg16(dx);
    WW(si, RS_XVEL, dx);
}

/* ---- terrain response ------------------------------------------------------------------ */

/* UpdateObjectTerrain (cs:698C): terrain response for a child whose owner has flag 08h
 * (climb or bounce with flags 40h and 80h). Reads the level tiles around the child. */
void UpdateObjectTerrain(uint16_t si)
{
    uint16_t lseg = DW(DS_LevelSegment);
    uint16_t bx = (uint16_t)(((uint8_t)sar16(RW(si, 2), 4)) << 8 | (uint8_t)sar16(RW(si, 0), 4));
    uint8_t dl = rd8(lseg, bx);
    uint8_t dh = rd8(lseg, (uint16_t)(bx - 0x100));
    uint16_t ax = RW(si, RS_XVEL);
    if (ax != 0) ax = (int16_t)ax > 0 ? 1 : 0xFFFF;
    bx = (uint16_t)(bx + ax);
    uint8_t al = DB((uint16_t)(DS_SideTileClassTable + rd8(lseg, (uint16_t)(bx - 0x100))));
    uint16_t owner = RW(si, RS_OWNER);

    if ((RB(owner, ENT_FLAGS) & 0x40) && (RB(owner, ENT_FLAGS) & 0x80)) {
        if (al == 0) {
            ScriptStepBack(si);
            WW(si, RS_YVEL, 0);
            WB(owner, ENT_FLAGS, RB(owner, ENT_FLAGS) & 0x7F);
            WW(si, RS_Y, (uint16_t)(RW(si, RS_Y) - 0x10));
        } else {
            WW(si, RS_X, (uint16_t)(RW(si, RS_X) - sar16(RW(si, RS_XVEL), 4)));
        }
        return;
    }
    if (al != 0) {
        if (RB(owner, ENT_FLAGS) & 0x40) {
            ScriptStepForward(si);
            WB(owner, ENT_FLAGS, RB(owner, ENT_FLAGS) | 0x80);
            WW(si, RS_YVEL, 0xFFF0);
            WW(si, RS_X, (uint16_t)(RW(si, RS_X) + sar16(sar16(RW(si, RS_XVEL), 1), 1)));
            return;
        }
        WW(si, RS_XVEL, neg16(RW(si, RS_XVEL)));
        WW(si, RS_X, (uint16_t)(RW(si, RS_X) + sar16(RW(si, RS_XVEL), 4)));
    }
    /* loc_6A21: the bob wobble and the ground probe */
    WW(si, RS_Y, (uint16_t)(RW(si, RS_Y) - 0x10));
    uint8_t t = dl; dl = dh; dh = t;                                 /* xchg dl, dh */
    if (DB((uint16_t)(DS_GroundTypeTable + dl)) == 0) {
        WW(si, RS_Y, (uint16_t)(RW(si, RS_Y) + 0x10));
        dl = dh;
        if (DB((uint16_t)(DS_GroundTypeTable + dl)) == 0) {
            if (!((int16_t)RW(si, RS_YVEL) >= 0x100)) WW(si, RS_YVEL, RW(si, RS_YVEL) + 0x10);
            return;
        }
    }
    uint16_t off = ComputeBobOffset(si, dl);
    WB(si, RS_Y, RB(si, RS_Y) & 0xF0);
    WW(si, RS_Y, (uint16_t)(RW(si, RS_Y) + off));
    owner = RW(si, RS_OWNER);
    uint16_t dx = 0;
    if (!(RB(owner, ENT_FLAGS) & 0x20)) {
        uint16_t v = sar16(neg16(RW(si, RS_YVEL)), 1);
        dx = v;
        if (abs16(v) <= 0x20) dx = 0;
    }
    WW(si, RS_YVEL, dx);
}

/* ---- child handlers (ChildHandlerTable; SI = child) --------------------------------------- */

/* Child kind 0: ground enemy (cs:7C90). */
int UpdateGroundChild(uint16_t si)
{
    CullChildFarFromPlayer(si);
    uint16_t owner = RW(si, RS_OWNER);
    uint8_t st = RB(si, RS_STATE);
    if (st == 0) {
        if (TickEntityTimer(owner)) return OBJ_OK;
        uint16_t ax = (uint16_t)(RW(si, RS_Y) - DW(DS_PlayerY));
        if ((int16_t)ax >= (int16_t)0xB0) { ChildDeath(si); return OBJ_OK; }
        WB(owner, ENT_FLAGS, RB(owner, ENT_FLAGS) | 8);
        WB(si, RS_STATE, 1);
        return OBJ_OK;
    }
    if (st == 1) {
        if (RW(si, RS_YVEL) != 0) return OBJ_OK;
        WB(si, RS_STATE, 2);
        uint16_t dx = 0x20;
        if (!((int16_t)RW(si, RS_X) < (int16_t)DW(DS_PlayerX))) dx = neg16(dx);
        WW(si, RS_XVEL, dx);
        ScriptStepForward(si);
        return OBJ_OK;
    }
    if (st == 2) return OBJ_OK;
    ChildFallOrDie(si);
    return OBJ_OK;
}

/* Child kind 1: despawn only (cs:7C8C). */
int CullChildOnly(uint16_t si)
{
    CullChildFarFromPlayer(si);
    return OBJ_OK;
}

/* Child kind 2: vertical bob (cs:7C2D). */
int UpdateBobChild(uint16_t si)
{
    CullChildFarFromPlayer(si);
    uint16_t owner = RW(si, RS_OWNER);
    uint8_t st = RB(si, RS_STATE);
    if (st >= 2) {
        if (st == 0xFF) ChildFallOrDie(si);
        return OBJ_OK;
    }
    SpawnDebrisLine(owner, 0, (uint8_t)(RW(si, RS_Y) - RW(owner, 0x0B)));
    if (st == 0) {
        WW(si, RS_YVEL, cbw(RB(owner, 0x0E)) << 4);
        uint16_t ax = (uint16_t)(RW(si, RS_Y) - RW(owner, 0x0B));
        if ((int8_t)RB(owner, 0x0D) >= (int8_t)(uint8_t)ax) return OBJ_OK;
        WB(si, RS_STATE, 1);
        ScriptStepForward(si);
        return OBJ_OK;
    }
    WW(si, RS_YVEL, neg16(cbw(RB(owner, 0x0E)) << 4));
    uint16_t ax = (uint16_t)(RW(si, RS_Y) - RW(owner, 0x0B));
    if ((int16_t)ax >= 0) return OBJ_OK;
    WB(si, RS_STATE, 0);
    ScriptStepBack(si);
    return OBJ_OK;
}

/* Child kind 3: falling walker (cs:7B91). */
int UpdateWalkerChild(uint16_t si)
{
    CullChildFarFromPlayer(si);
    uint16_t owner = RW(si, RS_OWNER);
    uint8_t st = RB(si, RS_STATE);
    if (st == 0) {
        if (TickEntityTimer(owner)) return OBJ_OK;
        uint8_t near = (uint8_t)sar16(abs16((uint16_t)(RW(owner, ENT_BOX) - DW(DS_PlayerX))), 4);
        if (RB(owner, 0x0D) < near) return OBJ_OK;
        WB(owner, ENT_FLAGS, RB(owner, ENT_FLAGS) & 0xEF);
        WB(si, RS_STATE, 1);
        WW(si, RS_YVEL, 0x20);
        ScriptStepForward(si);
        return OBJ_OK;
    }
    if (st == 1) {
        SpawnDebrisLine(owner, 0, (uint8_t)(RW(si, RS_Y) - RW(owner, 0x0B)));
        uint16_t bx = (uint16_t)(((uint8_t)sar16(RW(si, RS_Y), 4)) << 8 | (uint8_t)sar16(RW(si, RS_X), 4));
        SET_ES(DW(DS_LevelSegment));      /* the ground probe leaves ES = level map (never restored) */
        if (DB((uint16_t)(DS_GroundTypeTable + rd8(DW(DS_LevelSegment), bx))) == 0) return OBJ_OK;
        WB(si, RS_Y, RB(si, RS_Y) & 0xF0);
        WW(si, RS_YVEL, 0);
        WB(si, RS_STATE, 2);
        owner = RW(si, RS_OWNER);
        WB(owner, ENT_FLAGS, RB(owner, ENT_FLAGS) | 0x48);
        uint16_t dx = 0x30;
        if (!((int16_t)DW(DS_PlayerX) >= (int16_t)RW(si, RS_X))) dx = neg16(dx);
        WW(si, RS_XVEL, dx);
        ScriptStepForward(si);
        return OBJ_OK;
    }
    if (st == 2) {
        if ((int16_t)RW(si, RS_X) < 0) WW(si, RS_XVEL, neg16(RW(si, RS_XVEL)));
        return OBJ_OK;
    }
    ChildFallOrDie(si);
    return OBJ_OK;
}

/* Child kind 4: orbit / pendulum (cs:7ADF). */
int UpdateOrbitChild(uint16_t si)
{
    uint16_t owner = RW(si, RS_OWNER);
    CullChildAt(si, RW(owner, ENT_BOX), RW(owner, 0x0B));
    uint8_t st = RB(si, RS_STATE);
    if (st == 0) {
        if (TickEntityTimer(owner)) return OBJ_OK;
        uint16_t dy = (uint16_t)(RW(si, RS_Y) - RW(owner, 0x0B));
        SpawnDebrisLine(owner, 0, (uint8_t)dy);
        if (RB(owner, 0x0D) <= (uint8_t)dy) { WB(si, RS_STATE, 1); return OBJ_OK; }
        WW(si, RS_Y, RW(si, RS_Y) + 2);
        return OBJ_OK;
    }
    if (st == 1) {
        SetChildOrbitPosition(si, owner);
        WB(owner, 0x0F, RB(owner, 0x0F) + 4);
        uint8_t lim = RB(owner, 0x0E);
        if (RB(owner, 0x0F) < lim) return OBJ_OK;
        WB(owner, 0x0F, lim);
        WB(si, RS_STATE, 2);
        return OBJ_OK;
    }
    if (st == 2) {
        SetChildOrbitPosition(si, owner);
        uint8_t dl = (RB(owner, 0x0F) & 0x80) ? 1 : 0xFF;
        WB(owner, 0x10, RB(owner, 0x10) + dl);
        WB(owner, 0x0F, RB(owner, 0x0F) + RB(owner, 0x10));
        return OBJ_OK;
    }
    if (st == 0xFF) ChildFallOrDie(si);
    return OBJ_OK;
}

/* Child kind 5: pounce (cs:7A60). */
int UpdatePounceChild(uint16_t si)
{
    CullChildFarFromPlayer(si);
    uint16_t owner = RW(si, RS_OWNER);
    uint8_t st = RB(si, RS_STATE);
    if (st == 0) {
        uint16_t dx = 1;
        if ((int16_t)RW(si, RS_X) > (int16_t)DW(DS_PlayerX)) dx = neg16(dx);
        WW(si, RS_XVEL, dx);
        uint16_t ax = abs16((uint16_t)(DW(DS_PlayerX) - RW(si, RS_X)));
        if (RB(owner, 0x0D) < (uint8_t)sar16(ax, 4)) return OBJ_OK;
        ax = abs16((uint16_t)(RW(si, RS_Y) - DW(DS_PlayerY)));
        if (RB(owner, 0x0E) < (uint8_t)sar16(ax, 4)) return OBJ_OK;
        WB(si, RS_STATE, 0x0A);
        ax = (uint16_t)(RB(owner, 0x0F) << 4);
        WW(si, RS_YVEL, ax);
        if (RB(si, RS_FACING) & 0x80) ax = neg16(ax);
        WW(si, RS_XVEL, ax);
        return OBJ_OK;
    }
    if (st == 0x0A) {
        if (abs16((uint16_t)(RW(si, RS_Y) - DW(DS_PlayerY))) <= 8) {
            ScriptStepForward(si);
            WB(si, RS_STATE, 0x0B);
            WW(si, RS_YVEL, 0);
        }
        return OBJ_OK;
    }
    if (st == 0xFF) ChildFallOrDie(si);
    return OBJ_OK;
}

/* Child kind 6: wanderer (cs:78EC). */
int UpdateWanderingChild(uint16_t si)
{
    CullChildFarFromPlayer(si);
    uint16_t owner = RW(si, RS_OWNER);
    uint8_t st = RB(si, RS_STATE);
    if (st == 0) {
        SetChildDirToPlayer(si);
        uint8_t near = (uint8_t)sar16(abs16((uint16_t)(DW(DS_PlayerX) - RW(si, RS_X))), 4);
        if (RB(owner, 0x0D) < near) return OBJ_OK;
        WB(si, RS_STATE, 1);
        WB(owner, 0x10, 0x18);
        WB(owner, 0x0E, 0);
        WB(owner, 0x0F, 0);
        WW(owner, 0x11, (uint16_t)(RW(si, RS_X) << 3));
        WW(owner, 0x13, (uint16_t)(RW(si, RS_Y) << 3));
        return OBJ_OK;
    }
    if (st != 1) {
        if (st == 0xFF) ChildFallOrDie(si);
        return OBJ_OK;
    }
    SetChildDirToPlayer(si);
    uint16_t bp = (uint16_t)(DW(DS_PlayerX) - RW(si, RS_X));
    uint32_t sq = (uint32_t)abs16(bp) * abs16(bp);
    uint16_t cx = abs16((uint16_t)(DW(DS_PlayerY) - RW(si, RS_Y)));
    sq += (uint32_t)cx * cx;
    SET_DW(DS_word_A30E, (uint16_t)sq);
    SET_DW(DS_word_A310, (uint16_t)(sq >> 16));
    if (sq < 0x2710u && DB(DS_AttackAnimLock) != 0) {
        uint8_t al = (sq < 0x0E10u) ? 0x68 : 0x40;
        if (cx >= 0x30) {
            WB(owner, 0x0F, 0xC0);
        } else {
            if ((int16_t)bp > 0) al = neg8(al);
            WB(owner, 0x0E, al);
        }
    } else {
        uint8_t left = (uint8_t)(RB(owner, 0x10) - 1);
        WB(owner, 0x10, left);
        if (left & 0x80) {
            uint16_t seed = StepRandomSeed();
            WB(owner, 0x10, (uint8_t)((seed & 0x0F) + 3));
            uint8_t r = NextWindRandom();
            /* NextWindRandom starts with mov ah,0, so the 'and ah,7' term of the ASM is zero */
            uint8_t v = (uint8_t)(r & 0x0F);
            if ((int16_t)DW(DS_PlayerX) < (int16_t)RW(si, RS_X)) v = neg8(v);
            SET_DB(DS_byte_6BC0, v);
            r = NextWindRandom();
            v = (uint8_t)(r & 0x0F);
            if ((int16_t)DW(DS_PlayerY) < (int16_t)(uint16_t)(RW(si, RS_Y) + 0x32)) v = neg8(v);
            SET_DB(DS_byte_6BC1, v);
            seed = StepRandomSeed();
            uint8_t e = (uint8_t)(seed & 0x1F);
            if ((int8_t)DB(DS_byte_6BC0) < 0) e = neg8(e);
            WB(owner, 0x0E, e);
            uint8_t q;
            if (DB(DS_byte_6BC0) == 0) {
                q = 2;
            } else {
                uint8_t ch = DB(DS_byte_6BC0);
                if ((int8_t)ch < 0) ch = neg8(ch);
                uint16_t ax = abs16(cbw(DB(DS_byte_6BC1)));
                ax = (uint16_t)(ax << 3);
                if (ch == 0) return OBJ_DIVIDE_ERROR;
                uint16_t quo = (uint16_t)(ax / ch);
                if (quo > 0xFF) return OBJ_DIVIDE_ERROR;
                q = (uint8_t)quo;
            }
            if (q >= 0x20) q = 0x20;
            if ((int8_t)DB(DS_byte_6BC1) < 0) q = neg8(q);
            WB(owner, 0x0F, q);
        }
    }
    WW(owner, 0x11, (uint16_t)(RW(owner, 0x11) + cbw(RB(owner, 0x0E))));
    WW(owner, 0x13, (uint16_t)(RW(owner, 0x13) + cbw(RB(owner, 0x0F))));
    WW(si, RS_X, sar16(RW(owner, 0x11), 3));
    WW(si, RS_Y, sar16(RW(owner, 0x13), 3));
    return OBJ_OK;
}

/* Child kind 7: leaper (cs:7898). */
int UpdateLeaperChild(uint16_t si)
{
    CullChildFarFromPlayer(si);
    uint16_t owner = RW(si, RS_OWNER);
    uint8_t st = RB(si, RS_STATE);
    if (st == 0) {
        uint16_t dx = 1;
        if ((int16_t)RW(si, RS_X) > (int16_t)DW(DS_PlayerX)) dx = neg16(dx);
        WW(si, RS_XVEL, dx);
        uint16_t ax = abs16((uint16_t)(DW(DS_PlayerX) - RW(si, RS_X)));
        if (RB(owner, 0x0D) < (uint8_t)sar16(ax, 4)) return OBJ_OK;
        WB(si, RS_STATE, 0x0A);
        ax = (uint16_t)(cbw(RB(owner, 0x0E)) << 4);
        WW(si, RS_YVEL, ax);
        if (RB(si, RS_FACING) & 0x50) ax = neg16(ax);
        WW(si, RS_XVEL, ax);
        ScriptStepForward(si);
        return OBJ_OK;
    }
    if (st == 0x0A) return OBJ_OK;
    if (st == 0xFF) ChildFallOrDie(si);
    return OBJ_OK;
}

/* Hop start (loc_7827): YVel = -(owner +0Eh << 4), XVel = +-(owner +0Fh << 4) toward the
 * player, owner flags = (flags & 0D3h) | 2Ch, state 0Ah, script stepped forward. */
static void HopStart(uint16_t si, uint16_t owner)
{
    WW(si, RS_YVEL, neg16((uint16_t)(cbw(RB(owner, 0x0E)) << 4)));
    uint16_t hx = (uint16_t)(cbw(RB(owner, 0x0F)) << 4);
    if (!((int16_t)DW(DS_PlayerX) > (int16_t)RW(si, RS_X))) hx = neg16(hx);
    WW(si, RS_XVEL, hx);
    WB(si, RS_STATE, 0x0A);
    WB(owner, ENT_FLAGS, (uint8_t)((RB(owner, ENT_FLAGS) & 0xD3) | 0x2C));
    ScriptStepForward(si);
}

/* Child kind 8: hop attack (cs:77DE). */
int UpdateHopChild(uint16_t si)
{
    CullChildFarFromPlayer(si);
    uint16_t owner = RW(si, RS_OWNER);
    if (RW(si, RS_XVEL) == 0) {
        uint16_t dx = 1;
        if (!((int16_t)RW(si, RS_X) <= (int16_t)DW(DS_PlayerX))) dx = neg16(dx);
        WW(si, RS_XVEL, dx);
    }
    uint8_t st = RB(si, RS_STATE);
    if (st == 0) {
        if (TickEntityTimer(owner)) return OBJ_OK;
        uint8_t a = (uint8_t)sar16(abs16((uint16_t)(DW(DS_PlayerX) - RW(si, RS_X))), 4);
        if (RB(owner, 0x0D) < a) return OBJ_OK;
        a = (uint8_t)sar16(abs16((uint16_t)(DW(DS_PlayerY) - RW(si, RS_Y))), 4);
        if (RB(owner, 0x10) >= a) HopStart(si, owner);
        return OBJ_OK;
    }
    if (st == 0x0A) {
        if ((int16_t)RW(si, RS_YVEL) < 0) return OBJ_OK;
        WB(si, RS_STATE, 0x0B);
        ScriptStepForward(si);
        return OBJ_OK;
    }
    if (st == 0x0B) {
        if ((int16_t)RW(si, RS_YVEL) > 0) return OBJ_OK;
        WB(si, RS_STATE, 0x0C);
        ScriptStepBack(si);
        ScriptStepBack(si);
        WW(si, RS_XVEL, 0);
        WB(owner, ENT_TIMER, 0);
        return OBJ_OK;
    }
    if (st == 0x0C) {
        if (!TickEntityTimer(owner)) HopStart(si, owner);
        return OBJ_OK;
    }
    if (st == 0xFF) ChildFallOrDie(si);
    return OBJ_OK;
}

/* Child kind 9: patrol (cs:773D). Removed (+4 = FFFFh, owner flag 04h cleared) when
 * |Y - PlayerY| >= 0BEh or the player is outside the owner's window. */
int UpdatePatrolChild(uint16_t si)
{
    uint16_t owner = RW(si, RS_OWNER);
    if (!(RB(si, RS_FLAGS) & 0x20) && RB(si, RS_STATE) != 0xFF) {
        int remove;
        if (abs16((uint16_t)(RW(si, RS_Y) - DW(DS_PlayerY))) >= 0xBE) {
            remove = 1;
        } else {
            uint16_t ax = RW(owner, 0x0D);
            uint16_t dx = DW(DS_PlayerX);
            if ((int16_t)dx < (int16_t)ax) {
                dx = (uint16_t)(dx + 0x1E0);
                remove = !((int16_t)dx >= (int16_t)ax);
            } else {
                ax = (uint16_t)(RW(owner, 0x0F) + 0x1E0);
                remove = !((int16_t)ax > (int16_t)dx);
            }
        }
        if (remove) {
            WW(si, RS_FRAME, 0xFFFF);
            WB(owner, ENT_FLAGS, RB(owner, ENT_FLAGS) & 0xFB);
            return OBJ_OK;
        }
    }
    uint8_t st = RB(si, RS_STATE);
    if (st == 0) {
        WW(si, RS_XVEL, cbw(RB(owner, 0x11)));
        uint16_t ax = (uint16_t)(cbw(RB(owner, 0x11)) + 3);
        if (!((int16_t)ax > (int16_t)RB(owner, 0x12))) WB(owner, 0x11, (uint8_t)ax);
        if (!((int16_t)RW(owner, 0x0F) < (int16_t)RW(si, RS_X))) return OBJ_OK;
        WB(si, RS_STATE, 1);
        return OBJ_OK;
    }
    if (st == 1) {
        WW(si, RS_XVEL, cbw(RB(owner, 0x11)));
        uint16_t ax = (uint16_t)(cbw(RB(owner, 0x11)) - 3);
        if (!((int16_t)ax < (int16_t)neg16(RB(owner, 0x12)))) WB(owner, 0x11, (uint8_t)ax);
        if ((int16_t)RW(owner, 0x0D) < (int16_t)RW(si, RS_X)) return OBJ_OK;
        WB(si, RS_STATE, 0);
        return OBJ_OK;
    }
    if (st == 0xFF) ChildFallOrDie(si);
    return OBJ_OK;
}

/* Child kind 10: charger (cs:7665). */
int UpdateChargerChild(uint16_t si)
{
    CullChildFarFromPlayer(si);
    uint16_t owner = RW(si, RS_OWNER);
    uint8_t st = RB(si, RS_STATE);
    if (DB(DS_CurrentLevel) == 5 && DB(DS_ShakeMagnitude) != 0 && st != 3 && st != 0xFF)
        WB(owner, ENT_TIMER, 1);
    if (st == 0) {
        WB(owner, ENT_FLAGS, RB(owner, ENT_FLAGS) | 0x18);
        if (RW(si, RS_YVEL) == 0) WB(si, RS_STATE, 1);
        return OBJ_OK;
    }
    if (st == 1) {
        if (RW(si, RS_YVEL) != 0) { WB(si, RS_STATE, 0); return OBJ_OK; }
        WB(owner, ENT_TIMER, 0x1E);
        WB(si, RS_STATE, 2);
        ScriptStepForward(si);
        return OBJ_OK;
    }
    if (st == 2) {
        uint16_t ax = RW(si, RS_XVEL);
        if ((int16_t)ax < 0) ax = neg16(ax);
        if (ax < 0x10) {
            if (DB(DS_ScriptFrameFlags) == 0) return OBJ_OK;
            WB(owner, ENT_FLAGS, 0x0F);
            uint16_t dx = RB(owner, 0x0D);
            if (!((int16_t)RW(si, RS_X) < (int16_t)DW(DS_PlayerX))) dx = neg16(dx);
            WW(si, RS_XVEL, dx);
        }
        if (DB(DS_SnowFrameTick) & 3) return OBJ_OK;
        WB(owner, ENT_TIMER, RB(owner, ENT_TIMER) - 1);
        if (RB(owner, ENT_TIMER) != 0) return OBJ_OK;
        WB(si, RS_STATE, 3);
        WW(si, RS_YVEL, 0);
        WW(si, RS_XVEL, sar16(RW(si, RS_XVEL), 15));
        WB(owner, ENT_FLAGS, 0x36);
        ScriptStepForward(si);
        return OBJ_OK;
    }
    if (st == 3) {
        if (DB(DS_ScriptFrameFlags) != 0) ChildDeath(si);
        return OBJ_OK;
    }
    if (st == 0xFF) ChildFallOrDie(si);
    return OBJ_OK;
}

/* Child kind 11: leap (cs:760F). */
int UpdateLeapChild(uint16_t si)
{
    CullChildFarFromPlayer(si);
    uint16_t owner = RW(si, RS_OWNER);
    uint8_t st = RB(si, RS_STATE);
    if (st == 0) {
        if (DB(DS_ScriptFrameFlags) == 0) return OBJ_OK;
        WB(si, RS_STATE, 1);
        WB(owner, ENT_FLAGS, RB(owner, ENT_FLAGS) & 0xEF);
        uint16_t ax = (uint16_t)(cbw(RB(owner, 0x0D)) << 4);
        if (!((int16_t)DW(DS_PlayerX) > (int16_t)RW(si, RS_X))) ax = neg16(ax);
        WW(si, RS_XVEL, ax);
        WW(si, RS_YVEL, neg16((uint16_t)(cbw(RB(owner, 0x0E)) << 4)));
        return OBJ_OK;
    }
    if (st == 1) {
        uint16_t lim = (uint16_t)(cbw(RB(owner, 0x0F)) << 4);
        if ((int16_t)RW(si, RS_YVEL) >= (int16_t)lim) return OBJ_OK;
        WW(si, RS_YVEL, RW(si, RS_YVEL) + 8);
        return OBJ_OK;
    }
    if (st == 0xFF) ChildFallOrDie(si);
    return OBJ_OK;
}

/* Child kind 12: horizontal shot (cs:75C4). */
int UpdateLaunchedChild(uint16_t si)
{
    uint16_t owner = RW(si, RS_OWNER);
    uint8_t st = RB(si, RS_STATE);
    if (st == 0) {
        uint16_t ax = cbw(RB(owner, 0x0D));
        if ((int16_t)DW(DS_PlayerX) < (int16_t)RW(si, RS_X)) ax = neg16(ax);
        WW(si, RS_XVEL, (uint16_t)(ax << 4));
        WB(si, RS_STATE, RB(si, RS_STATE) + 1);
        return OBJ_OK;
    }
    if (st == 1) {
        if (RB(si, RS_FLAGS) & 0x20) {
            WB(owner, ENT_TIMER, 0);
            return OBJ_OK;
        }
        WB(owner, ENT_TIMER, RB(owner, ENT_TIMER) + 1);
        if (RB(owner, ENT_TIMER) < 0x9A) return OBJ_OK;
        WB(si, RS_STATE, 0xFF);
        return OBJ_OK;
    }
    if (st == 0xFF) ChildFallOrDie(si);
    return OBJ_OK;
}

/* ---- entity handlers (EntityHandlerTable; SI = entity; return CF) --------------------------- */

/* Shared spawn tail (cs:7F31): AllocChildRecord, then the child copies the entity (X +9,
 * Y +0Bh, frame = entity sprite, owner = entity, +0Fh = entity +5); entity flags = 17h. */
static int SpawnChildAtOwner(uint16_t si, uint16_t *out)
{
    uint16_t di;
    if (AllocChildRecord(&di)) { *out = di; return 1; }   /* DI stays one past the last slot (50A8h) */
    WB(di, RS_COUNT, 0);
    SET_DW(DS_LastSpawnedChild, di);
    WW(di, RS_X, RW(si, ENT_BOX));
    WW(di, RS_Y, RW(si, 0x0B));
    WW(di, RS_FRAME, RW(si, ENT_SPRITE));
    WW(di, RS_OWNER, si);
    WB(si, ENT_FLAGS, 0x17);
    WW(di, RS_XVEL, 0);
    WW(di, RS_YVEL, 0);
    WB(di, RS_STATE, 0);
    WB(di, RS_AUX, RB(si, 5));
    *out = di;
    return 0;
}

/* Entity kind 1 (cs:7F26): spawn when the entity point is in the camera window. */
int SpawnChildWhenVisible(uint16_t si)
{
    uint16_t di;
    if (IsInCameraWindow(RW(si, ENT_BOX), RW(si, 0x0B))) return 1;
    return SpawnChildAtOwner(si, &di);
}

/* Entity kind 0 (cs:7F6C): a child 0C0h to the side of the player, drop height B0h above. */
int SpawnDropOverPlayer(uint16_t si)
{
    uint8_t px = (uint8_t)sar16(DW(DS_PlayerX), 4);
    uint8_t tx = RB(si, ENT_BOX);
    if (px < tx) return 1;
    uint8_t d = (uint8_t)(px - tx);
    if (RB(si, 0x0B) < d) return 1;
    uint8_t py = (uint8_t)sar16(DW(DS_PlayerY), 4);
    uint8_t ty = RB(si, ENT_BOX + 1);
    if (py < ty) return 1;
    d = (uint8_t)(py - ty);
    if (RB(si, 0x0C) < d) return 1;
    uint16_t di;
    if (AllocChildRecord(&di)) return 1;
    WB(di, RS_COUNT, 0);
    SET_DW(DS_LastSpawnedChild, di);
    uint16_t dx = 0xC0;
    uint8_t toggle = (uint8_t)(DB(DS_SpawnSideToggle) ^ 1);
    SET_DB(DS_SpawnSideToggle, toggle);
    if (toggle != 0) dx = neg16(dx);
    WW(di, RS_X, (uint16_t)(DW(DS_PlayerX) + dx));
    WW(di, RS_Y, (uint16_t)(DW(DS_PlayerY) - 0xB0));
    WW(di, RS_FRAME, RW(si, ENT_SPRITE));
    WW(di, RS_OWNER, si);
    WB(si, ENT_FLAGS, 7);
    WB(di, RS_STATE, 0);
    WW(di, RS_YVEL, 0);
    WW(di, RS_XVEL, 0);
    WB(di, RS_AUX, RB(si, 5));
    return 0;
}

/* Entity kind 2 (cs:7EE2): a child at the entity point when that point is in the window. */
int SpawnFixedChild(uint16_t si)
{
    if (IsInCameraWindow(RW(si, ENT_BOX), RW(si, 0x0B))) return 1;
    uint16_t di;
    if (AllocChildRecord(&di)) return 1;
    WB(di, RS_COUNT, 0);
    SET_DW(DS_LastSpawnedChild, di);
    WW(di, RS_X, RW(si, ENT_BOX));
    WW(di, RS_Y, RW(si, 0x0B));
    WW(di, RS_FRAME, RW(si, ENT_SPRITE));
    WW(di, RS_OWNER, si);
    WB(si, ENT_FLAGS, 5);
    WW(di, RS_XVEL, 0);
    WW(di, RS_YVEL, 0);
    WB(di, RS_STATE, 0);
    WB(di, RS_AUX, RB(si, 5));
    return 0;
}

/* Entity kind 3 (cs:7ED8): visible spawn, then entity flags = 37h. */
int SpawnChildLatchState37(uint16_t si)
{
    int cf = SpawnChildWhenVisible(si);
    if (!cf) WB(si, ENT_FLAGS, 0x37);
    return cf;
}

/* Entity kind 4 (cs:7EBF): visible spawn, then flags 5, +0Fh = 0, +10h = 0. */
int SpawnChildResetCounters(uint16_t si)
{
    int cf = SpawnChildWhenVisible(si);
    if (cf) return 1;
    WB(si, ENT_FLAGS, 5);
    WB(si, 0x0F, 0);
    WB(si, 0x10, 0);
    return 0;
}

/* Entity kinds 5..8 (cs:7EB5): visible spawn, then flags = 5 (busy). */
int SpawnChildLatched(uint16_t si)
{
    int cf = SpawnChildWhenVisible(si);
    if (!cf) WB(si, ENT_FLAGS, 5);
    return cf;
}

/* Entity kind 9 (cs:7E97): clears +11h; visible spawn; flags = old | 5 (| 80h on level 6). */
int SpawnChildWithFlags(uint16_t si)
{
    WB(si, 0x11, 0);
    uint8_t al = RB(si, ENT_FLAGS);
    if (SpawnChildWhenVisible(si)) return 1;
    al = (uint8_t)(al | 5);
    if (DB(DS_CurrentLevel) == 6) al |= 0x80;
    WB(si, ENT_FLAGS, al);
    return 0;
}

/* Entity kind 10 (cs:7D9B): spawns when the player is inside the entity tile box and the period
 * has elapsed. The child X is PlayerX + a ring offset from ds:A343h; Y is the first passable
 * row above the player in the level column. Level 5 blocks it while shaking or in phase 3. */
int SpawnChildOnPlayerBox(uint16_t si)
{
    if (DB(DS_CurrentLevel) == 5) {
        if (DB(DS_ShakeMagnitude) != 0) return 1;
        if (DW(DS_Level5Phase) == 3) return 1;
    }
    WB(si, 7, sat_inc8(RB(si, 7)));
    if (RB(si, ENT_PERIOD) > (uint8_t)(RB(si, 7) >> 2)) return 1;
    uint8_t px = (uint8_t)sar16(DW(DS_PlayerX), 4);
    uint8_t tx = RB(si, ENT_BOX);
    if (px < tx) return 1;
    uint8_t d = (uint8_t)(px - tx);
    if (RB(si, 0x0B) < d) return 1;
    uint8_t py = (uint8_t)sar16(DW(DS_PlayerY), 4);
    uint8_t ty = RB(si, ENT_BOX + 1);
    if (py < ty) return 1;
    d = (uint8_t)(py - ty);
    if (RB(si, 0x0C) < d) return 1;
    uint16_t di;
    if (AllocChildRecord(&di)) return 1;
    WB(di, RS_COUNT, 0);
    SET_DW(DS_LastSpawnedChild, di);
    uint16_t bx = DW(DS_word_A341);
    uint16_t px16 = DW(DS_PlayerX);
    uint16_t x = (uint16_t)(px16 + DW((uint16_t)(bx + 0xA343)));
    bx = (uint16_t)((bx + 2) & 0x0F);
    SET_DW(DS_word_A341, bx);
    WW(di, RS_X, x);
    WW(di, RS_XVEL, ((int16_t)px16 >= (int16_t)x) ? 0 : 0xFFFF);
    uint16_t lseg = DW(DS_LevelSegment);
    SET_ES(lseg);                         /* the ground search leaves ES = level map (never restored) */
    uint16_t bp = (uint16_t)(((uint8_t)sar16(DW(DS_PlayerY), 4) + 4) << 8 | (uint8_t)sar16(x, 4));
    uint16_t limit = (uint16_t)(DB(DS_MapRows) << 8);
    int found = 0;
    for (int ah = 10; ah > 0; ah--) {
        if (bp < limit) {
            if (DB((uint16_t)(DS_GroundTypeTable + rd8(lseg, bp))) != 0 &&
                DB((uint16_t)(DS_GroundTypeTable + rd8(lseg, (uint16_t)(bp - 0x100)))) == 0 &&
                DB((uint16_t)(DS_GroundTypeTable + rd8(lseg, (uint16_t)(bp - 0x200)))) == 0) {
                found = 1;
            }
        }
        if (found) break;
        bp = (uint16_t)(bp - 0x100);
        if ((int16_t)bp < 0x300) break;
    }
    if (!found) return 1;
    WW(di, RS_Y, (uint16_t)((bp >> 8) << 4));
    WW(di, RS_FRAME, RW(si, ENT_SPRITE));
    WW(di, RS_OWNER, si);
    WB(si, ENT_FLAGS, 0x17);
    WB(di, RS_STATE, 0);
    WW(di, RS_YVEL, 0);
    WB(di, RS_AUX, RB(si, 5));
    return 0;
}

/* Entity kind 11 (cs:7D6E): periodic visible spawn with a random Y offset (0..3Fh). */
int SpawnChildRandomY(uint16_t si)
{
    WB(si, 7, sat_inc8(RB(si, 7)));
    if (RB(si, ENT_PERIOD) > (uint8_t)(RB(si, 7) >> 2)) return 1;
    if (SpawnChildWhenVisible(si)) return 1;
    WB(si, ENT_FLAGS, 0x37);
    uint8_t r = NextWindRandom() & 0x3F;
    uint16_t di = DW(DS_LastSpawnedChild);
    WW(di, RS_Y, (uint16_t)(RW(di, RS_Y) - r));
    return 0;
}

/* Entity kind 12 (cs:7D1B): timed spawn when the player passes the entity point (within the
 * 320..640 px band in X, or 180..360 px below it when closer). */
int SpawnChildOnApproach(uint16_t si)
{
    uint16_t ey = RW(si, 0x0B);
    if ((int16_t)DW(DS_PlayerY) <= (int16_t)ey) return 1;
    uint16_t ax = abs16((uint16_t)(RW(si, ENT_BOX) - DW(DS_PlayerX)));
    if ((int16_t)ax >= 0x280) return 1;
    if ((int16_t)ax < 0x140) {
        ax = (uint16_t)(DW(DS_PlayerY) - ey);
        if ((int16_t)ax >= 0x168) return 1;
        if ((int16_t)ax <= 0xB4) return 1;
    }
    WB(si, 7, sat_inc8(RB(si, 7)));
    if (RB(si, ENT_PERIOD) > (uint8_t)(RB(si, 7) >> 2)) return 1;
    uint16_t di;
    if (SpawnChildAtOwner(si, &di)) return 1;
    WB(si, ENT_FLAGS, 0x8F);
    WB(si, 7, 0);
    return 0;
}

/* ---- the boss ------------------------------------------------------------------------------ */

/* Player contact with one boss part (DI): when the part is drawn (+5 & 20h) and overlaps the
 * player, BossCounter -= 4 - byte_91FB (saturating at zero, with the ASM's sign test), the
 * player takes DrainPlayerEnergy and a knockback away from BossX; SFX 1. */
static void ApplyBossPartContact(uint16_t di)
{
    if (RW(di, 4) == 0xFFFF) return;
    if (!(RB(di, 5) & 0x20)) return;
    if (!TestHitboxOverlap(PLAYER_REC, di)) return;
    uint8_t al = (uint8_t)(4 - DB(DS_byte_91FB));
    uint8_t r = (uint8_t)(DB(DS_BossCounter) - al);
    SET_DB(DS_BossCounter, (r & 0x80) ? 0 : r);
    DrainPlayerEnergy();
    StartSoundEffect(1);
    SET_DB(DS_PlayerHurtTimer, 0x2C);
    SET_DB(DS_AttackAnimLock, 0);
    SET_DW(DS_PlayerVelY, 0xFF80);
    SET_DB(DS_MotionShift, 3);
    uint16_t ax = 0x80;
    if ((int16_t)DW(DS_PlayerX) < (int16_t)DW(DS_BossX)) ax = neg16(ax);
    SET_DW(DS_PlayerVelX, ax);
    SET_DB(DS_PlayerAirFlags, 0);
}

/* Player contact with the boss parts (cs:81B4): skipped while RespawnState is set. */
static void CheckPlayerBossContact(void)
{
    if (DB(DS_RespawnState) != 0) return;
    ApplyBossPartContact(DW(DS_word_A421));
    ApplyBossPartContact(DW(DS_word_A423));
    if (!TestHitboxOverlap(PLAYER_REC, DW(DS_word_A425))) return;
    if (DB(DS_LandingFlag) == 0) return;
    uint16_t ax = 0xFFC0;
    if (DB(DS_byte_27EA) != 0) {
        ax = 0xFF80;
        StartSoundEffect(3);
    }
    SET_DW(DS_PlayerVelY, ax);
}

/* DrainPlayerEnergy (cs:824D): hurt cooldown byte_6BC9 (reset to 5 on each wrap, energy -1),
 * a respawn request when the energy goes negative, and one particle at the player. */
void DrainPlayerEnergy(void)
{
    uint8_t b = (uint8_t)(DB(DS_byte_6BC9) - 1);
    SET_DB(DS_byte_6BC9, b);
    if (b & 0x80) {
        SET_DB(DS_byte_6BC9, 5);
        uint8_t e = (uint8_t)(DB(DS_Energy) - 1);
        SET_DB(DS_Energy, e);
        if (e & 0x80) TriggerPlayerRespawnCore();
    }
    SET_DW(DS_EffectOriginX, DW(DS_PlayerX));
    SET_DW(DS_EffectOriginY, (uint16_t)(DW(DS_PlayerY) - 0x30));
    uint16_t ax = 0x30;
    if (DB(DS_byte_6BC9) & 1) ax = neg16(ax);
    SET_DW(DS_EffectFrame, 0x2046);
    uint16_t dy = 0xFF80;
    SpawnEffectBurst(ax, &dy, 1);
}

/* IncrementBossCounter (cs:757A): BossCounter += 1, saturating at 0FFh. */
void IncrementBossCounter(void)
{
    SET_DB(DS_BossCounter, sat_inc8(DB(DS_BossCounter)));
}

/* Hazard test of one slot (cs:8182): the part at word_A423 (frame 19Dh/19Ch) frees the slot
 * without damage; otherwise the body part at word_A425 decides. Returns CF. */
static int CheckHazardHitsBoss(uint16_t si)
{
    if (RW(si, 4) == 0xFFFF) return 0;
    uint16_t di = DW(DS_word_A423);
    uint16_t ax = RW(di, 4) & 0x1FFF;
    if (ax == 0x19D || ax == 0x19C) {
        if (TestHitboxOverlap(si, di)) {
            WW(si, 4, 0xFFFF);
            return 0;
        }
    }
    return TestHitboxOverlap(si, DW(DS_word_A425));
}

/* Boss damage from a hazard hit (loc_80F7..loc_8181): the hit cooldown (word_A3FD, 22 frames),
 * SFX 8, the part flash bits, BossEnergy -= PlayerAttackPower. The ASM tests the sign of the
 * 16-bit result for the defeat (BossState 6). */
static void BossHit(uint16_t si)
{
    uint16_t tick = DW(DS_SnowFrameTick);
    if (abs16((uint16_t)(tick - DW(DS_word_A3FD))) <= 0x16) return;
    SET_DW(DS_word_A3FD, tick);
    StartSoundEffect(8);
    SET_DB(DS_byte_564D, DB(DS_byte_564D) | 0x40);
    SET_DB(DS_byte_565F, DB(DS_byte_565F) | 0x40);
    SET_DB(DS_byte_5671, DB(DS_byte_5671) | 0x40);
    SET_DB(DS_byte_5683, DB(DS_byte_5683) | 0x40);
    SET_DB(DS_byte_5695, DB(DS_byte_5695) | 0x40);
    SET_DB(DS_PlayerAltModeFlag, 0);
    IncrementBossCounter();
    uint16_t dmg = DB(DS_PlayerAttackPower);
    uint16_t e = (uint16_t)(DW(DS_BossEnergy) - dmg);
    SET_DW(DS_BossEnergy, e);
    if (e & 0x8000) {
        SET_DB(DS_BossState, 6);
        SET_DW(DS_BossVelY, 0xFF80);
        return;
    }
    if (dmg > 0x14) {
        uint16_t vx = 0x30;
        if (!((int16_t)DW(DS_BossX) >= (int16_t)DW(DS_PlayerX))) vx = neg16(vx);
        SET_DW(DS_BossVelX, vx);
        uint16_t vy = 0xFFC0;
        if ((int16_t)DW(DS_BossVelY) > 0) vy = (uint16_t)(vy << 1);
        SET_DW(DS_BossVelY, vy);
        SET_DB(DS_BossState, 5);
    }
    SET_DW(DS_BossStateTimer, 0);
    WW(si, 4, 0xFFFF);
}

/* CheckBossHits (cs:80DE): the four-slot hazard pass, stopping at the first hit. */
void CheckBossHits(void)
{
    if (CheckHazardHitsBoss(HAZARD_SLOT0)) { BossHit(HAZARD_SLOT0); return; }
    uint16_t si = HAZARD_SLOT1;
    for (int n = 0; n < 4; n++, si = (uint16_t)(si + RS_SIZE)) {
        if (CheckHazardHitsBoss(si)) { BossHit(si); return; }
    }
}

/* SetGaugeSprites (cs:7585): CX gauge sprites (capped at 8) in slots 56A2h.., the rest empty.
 * A nonzero count first activates resource slot 0Dh. */
int SetGaugeSprites(uint16_t cx)
{
    uint16_t si = GAUGE_SLOTS;
    if (cx >= 8) cx = 8;
    uint16_t count = cx;
    if (cx != 0) {
        int st = ActivateResourceSlot(0x0D);
        if (st) return st;
        uint16_t x = 0x0D;
        for (uint16_t i = 0; i < count; i++, si = (uint16_t)(si + RS_SIZE)) {
            WW(si, RS_FRAME, 0x135);
            WW(si, RS_X, x);
            WW(si, RS_Y, 0xAA);
            x = (uint16_t)(x + 5);
        }
    }
    for (uint16_t i = (uint16_t)(8 - count); i != 0; i--, si = (uint16_t)(si + RS_SIZE))
        WW(si, RS_FRAME, 0xFFFF);
    return OBJ_OK;
}

/* UpdateBossFight (cs:70D7): the boss state machine over BossState (0FFh = inactive; 6 = defeated,
 * set in BossHit; 0..7 active). Movement, tile clamps, animation op (sub_93B2) and hazard test. */
int UpdateBossFight(void)
{
    uint16_t cx, di, ax, dist, anim, bx, diff, a, q, v, vx, vy, lim, t;
    uint8_t b, al, st_now;
    int st;

    cx = (uint16_t)(DW(DS_BossEnergy) >> 3);
    di = DW(DS_word_564C);
    if (di == 0xFFFF || DB(DS_BossState) == 6 || !(di & 0x2000)) cx = 0;
    st = SetGaugeSprites(cx);
    if (st) return st;

    ax = (uint16_t)(sar16(DW(DS_BossVelX), 4) + DW(DS_BossX));
    if (!((int16_t)ax < 0) && !((int16_t)DW(DS_word_91F7) > (int16_t)ax) &&
        !((int16_t)DW(DS_word_91F9) < (int16_t)ax))
        SET_DW(DS_BossX, ax);

    SET_DW(DS_BossY, (uint16_t)(DW(DS_BossY) + sar16(DW(DS_BossVelY), 4)));
    if ((int16_t)DW(DS_BossVelY) < 0) goto loc_7165;
    SET_ES(DW(DS_LevelSegment));          /* the tile probe leaves ES = level map (never restored) */
    bx = (uint16_t)(((uint8_t)sar16(DW(DS_BossY), 4)) << 8 | (uint8_t)sar16(DW(DS_BossX), 4));
    if (DB((uint16_t)(DS_GroundTypeTable + rd8(DW(DS_LevelSegment), bx))) == 0) goto loc_7165;
    SET_DB(DS_BossY, DB(DS_BossY) & 0xF0);
    if (DW(DS_BossVelY) != 0) SET_DB(DS_ShakeMagnitude, 7);
    SET_DW(DS_BossVelY, 0);
    SET_DW(DS_BossVelX, 0);
    goto loc_7172;
loc_7165:
    if (!((int16_t)DW(DS_BossVelY) >= (int16_t)0xE0)) SET_DW(DS_BossVelY, (uint16_t)(DW(DS_BossVelY) + 0x10));
loc_7172:
    SET_DB(DS_byte_A3FA, DW(DS_BossX) < DW(DS_PlayerX) ? 1 : 0);
    dist = abs16((uint16_t)(DW(DS_PlayerX) - DW(DS_BossX)));
    SET_DW(DS_BossDistX, dist);
    if (dist > 0x190) return OBJ_OK;
    if (abs16((uint16_t)(DW(DS_PlayerY) - DW(DS_BossY))) > 0xFA) return OBJ_OK;

    st_now = DB(DS_BossState);
    if (st_now == 0) goto st0;
    if (st_now == 1) goto st1;
    if (st_now == 2) goto st2;
    if (st_now == 3) goto st3;
    if (st_now == 4) goto st4;
    if (st_now == 5) goto st5;
    if (st_now == 6) goto st6;
    if (st_now == 7) goto st7;
    goto anim;

st0:   /* loc_71AB */
    if (DW(DS_BossDistX) < 0xFA) {
        SET_DW(DS_BossStateTimer, 0);
        SET_DB(DS_BossState, 1);
    } else {
        SET_DB(DS_BossCounter, 0);
        SET_DW(DS_BossAnimPtr, 0xA427);
    }
    goto anim;

st1:   /* loc_71E0 */
    if (DW(DS_BossDistX) >= 0xFA) {
        SET_DB(DS_BossState, 0);
        goto anim;
    }
    SET_DW(DS_BossStateTimer, (uint16_t)(DW(DS_BossStateTimer) + 1));
    if (DW(DS_BossEnergy) < 0x3C) {
        SET_DB(DS_BossState, 2);
        goto anim;
    }
    SET_DW(DS_BossAnimPtr, 0xA434);
    if (DW(DS_BossStateTimer) >= 0x6E) {
        SET_DB(DS_BossState, 2);
        IncrementBossCounter();
        SET_DW(DS_BossStateTimer, 0);
    } else if (DB(DS_AttackAnimLock) != 0 && (DB(DS_SnowFrameTick) & 7) == 0) {
        IncrementBossCounter();
    } else if (DB(DS_BossCounter) >= 0x0A) {
        SET_DB(DS_BossState, 3);
        SET_DW(DS_BossStateTimer, 0);
    } else if (DB(DS_BossCounter) > 3) {
        SET_DB(DS_BossState, 2);
        SET_DW(DS_BossStateTimer, 0);
    }
    goto anim;

st2:   /* loc_726C */
    SET_DW(DS_BossStateTimer, (uint16_t)(DW(DS_BossStateTimer) + 1));
    if (DW(DS_BossStateTimer) == 0x2C) {
        CheckPlayerBossContact();
        SET_DW(DS_BossVelY, 0xFF20);
        if (DW(DS_BossEnergy) < 0x28 && DW(DS_BossDistX) < 0x50) {
            SET_DB(DS_BossState, 4);
            SET_DW(DS_BossStateTimer, 0);
        }
        SET_DW(DS_BossAnimPtr, 0xA45E);
        goto anim;
    }
    if (DW(DS_BossStateTimer) < 0x2C) {
        CheckPlayerBossContact();
        if (DW(DS_BossDistX) >= 0x4B) {
            SET_DW(DS_BossAnimPtr, 0xA43B);
        } else {
            SET_DB(DS_BossState, 3);
            SET_DW(DS_BossStateTimer, 0);
        }
        goto anim;
    }
    /* loc_72C4: timer above 2Ch */
    if ((int16_t)DW(DS_BossVelY) > 0)
        SET_DW(DS_BossAnimPtr, (int16_t)DW(DS_BossEnergy) >= 0x64 ? 0xA460 : 0xA462);
    if (DW(DS_BossDistX) <= 0x50) {
        SET_DW(DS_BossStateTimer, 0);
        SET_DB(DS_BossState, 3);
        SET_DB(DS_BossCounter, 0x0B);
        goto anim;
    }
    if (DW(DS_BossStateTimer) < 0x58) goto anim;
    if (DW(DS_BossStateTimer) != 0x58) {
        if (DW(DS_BossVelY) == 0) {
            SET_DB(DS_BossCounter, 3);
            SET_DW(DS_BossStateTimer, 0);
            SET_DB(DS_BossState, 1);
        }
        goto anim;
    }
    bx = (uint16_t)(DB(DS_byte_91FB) * 0x50);
    SET_DW(DS_BossAnimPtr, 0xA45E);
    diff = (uint16_t)(DW(DS_BossX) - DW(DS_PlayerX));
    a = abs16(diff);
    if (a > bx) {
        vy = 0xFFA0;
        vx = 0x30;
    } else {
        q = (uint16_t)(a / 14);
        if (q > 0xFF) return OBJ_DIVIDE_ERROR;
        v = (uint16_t)(q << 4);
        vx = (uint16_t)(v >> 1);
        vy = neg16(v);
    }
    SET_DW(DS_BossVelY, vy);
    if (!((int16_t)diff < 0)) vx = neg16(vx);
    SET_DW(DS_BossVelX, vx);
    goto anim;

st3:   /* loc_7370 */
    CheckPlayerBossContact();
    SET_DW(DS_BossStateTimer, (uint16_t)(DW(DS_BossStateTimer) + 1));
    lim = 0x42;
    t = abs16((uint16_t)(DW(DS_BossEnergy) - 0x32));
    if (t <= 0x19) lim = 0x9A;
    if (DW(DS_BossStateTimer) > lim) {
        SET_DW(DS_BossStateTimer, 0);
        SET_DB(DS_BossState, 2);
        goto anim;
    }
    if (DW(DS_BossVelY) == 0) {
        ax = DW(DS_BossDistX);
        if (ax >= 0x64) goto st3_far;
        if (ax <= 0x19) {
            SET_DW(DS_BossAnimPtr, 0xA470);
            goto anim;
        }
        if (ax <= 0x23) {
            SET_DB(DS_BossState, 4);
            SET_DW(DS_BossStateTimer, 0);
            goto anim;
        }
        if (DB(DS_BossCounter) == 0) {
            SET_DB(DS_BossState, 7);
            SET_DW(DS_BossStateTimer, 0);
        }
        SET_DW(DS_BossAnimPtr, 0xA46B);
        goto anim;
    }
    if ((int16_t)DW(DS_BossVelY) > 0) SET_DW(DS_BossAnimPtr, 0xA464);
    else SET_DW(DS_BossAnimPtr, 0xA45E);
    goto anim;
st3_far:  /* loc_73F1 */
    SET_DW(DS_BossVelY, 0xFFAF);
    vx = 0x50;
    if (!((int16_t)DW(DS_PlayerX) > (int16_t)DW(DS_BossX))) vx = neg16(vx);
    SET_DW(DS_BossVelX, vx);
    goto anim;

st4:   /* loc_741A */
    CheckPlayerBossContact();
    SET_DW(DS_BossStateTimer, (uint16_t)(DW(DS_BossStateTimer) + 1));
    if (DW(DS_BossStateTimer) > 0x42) {
        SET_DB(DS_BossState, 3);
        SET_DW(DS_BossStateTimer, 0);
        goto anim;
    }
    SET_DW(DS_BossAnimPtr, 0xA475);
    if (DW(DS_BossVelY) == 0) SET_DB(DS_ShakeMagnitude, 4);
    goto anim;

st5:   /* loc_7452 */
    SET_DW(DS_BossStateTimer, (uint16_t)(DW(DS_BossStateTimer) + 1));
    if (DW(DS_BossStateTimer) > 0x13) {
        SET_DW(DS_BossStateTimer, 0);
        SET_DB(DS_BossState, 1);
        goto anim;
    }
    SET_DW(DS_BossAnimPtr, 0xA47E);
    goto anim;

st6:   /* loc_747A: the defeat */
    SET_DW(DS_BossAnimPtr, 0xA480);
    if ((int16_t)DW(DS_BossVelY) < 0) goto anim;
    SET_DB(DS_BossState, 0xFF);
    SET_DW(DS_word_564C, 0xFFFF);
    SET_DW(DS_word_565E, 0xFFFF);
    SET_DW(DS_word_5670, 0xFFFF);
    SET_DW(DS_word_5682, 0xFFFF);
    SET_DW(DS_word_5694, 0xFFFF);
    SET_DW(DS_EffectOriginX, DW(DS_word_5648));
    SET_DW(DS_EffectOriginY, DW(DS_word_564A));
    SpawnFourDebrisParticles();
    SET_DW(DS_EffectOriginX, (uint16_t)(DW(DS_EffectOriginX) + 0x20));
    SET_DW(DS_EffectOriginY, (uint16_t)(DW(DS_EffectOriginY) + 0x20));
    SpawnFourDebrisParticles();
    SET_DW(DS_EffectOriginX, (uint16_t)(DW(DS_EffectOriginX) - 0x40));
    SpawnFourDebrisParticles();
    SET_DW(DS_EffectOriginX, (uint16_t)(DW(DS_EffectOriginX) - 0x20));
    SET_DW(DS_EffectOriginY, (uint16_t)(DW(DS_EffectOriginY) - 0x40));
    SpawnFourDebrisParticles();
    SET_DW(DS_EffectFrame, 0x63);
    uint16_t dz = 0;
    SpawnEffectBurst(0, &dz, 1);
    return OBJ_OK;

st7:   /* loc_74EA */
    SET_DW(DS_BossStateTimer, (uint16_t)(DW(DS_BossStateTimer) + 1));
    if (DW(DS_BossStateTimer) > 0x2C) {
        SET_DB(DS_BossState, 1);
        goto anim;
    }
    SET_DW(DS_BossAnimPtr, 0xA45E);
    if (DW(DS_BossVelY) == 0) {
        SET_DW(DS_BossVelY, 0xFF9F);
        vx = 0xFFD0;
        if (!((int16_t)DW(DS_PlayerX) > (int16_t)DW(DS_BossX))) vx = neg16(vx);
        SET_DW(DS_BossVelX, vx);
        goto anim;
    }
    if ((int16_t)DW(DS_BossVelY) >= 0) SET_DW(DS_BossAnimPtr, 0xA460);
    goto anim;

anim:  /* loc_7534: the animation op, its boss-part records and the hazard test */
    anim = DW(DS_BossAnimPtr);
    if (DW(DS_word_6C0A) != anim) {
        SET_DW(DS_word_6C0A, anim);
        SET_DW(DS_word_A3FF, anim);
    }
    for (;;) {
        bx = DW(DS_word_A3FF);
        b = DB(bx);
        if (!(b & 0x80)) break;
        SET_DW(DS_word_A3FF, (uint16_t)(DW(DS_word_A3FF) + cbw(b)));
    }
    al = DB(bx);
    SET_DB(DS_byte_A3F9, al);
    al = (uint8_t)(al & 0xBF);
    SET_DW(DS_word_A3FF, (uint16_t)(DW(DS_word_A3FF) + 1));
    SET_DW(DS_word_A403, DW(DS_BossX));
    SET_DW(DS_word_A405, DW(DS_BossY));
    {
        uint16_t di_out = 0;
        sub_93B2((uint16_t)((uint16_t)al * 0x14 + BOSS_OPS_BASE), &di_out);
    }
    CheckBossHits();
    return OBJ_OK;
}

/* ---- level 5 and level 9 ------------------------------------------------------------------ */

/* UpdateLevel5Sequence (cs:6D34): the five L5 projectiles, the phase timers and the hit tests
 * against the player and the boss-like parts. */
int UpdateLevel5Sequence(void)
{
    int st = SetGaugeSprites((uint16_t)((2u - DW(DS_Level5Phase)) << 1));
    if (st) return st;
    uint16_t bx = LEVEL5_SLOT;
    uint16_t di = MIRROR_SLOTS;
    for (int n = 5; n > 0; n--, bx = (uint16_t)(bx + 0x0B), di = (uint16_t)(di + RS_SIZE)) {
        uint16_t bp = RW(bx, L5P_FRAME);
        if (bp == 0xFFFF) continue;
        int hit = 0;
        if (DB(DS_RespawnState) == 0) hit = TestHitboxOverlap(PLAYER_REC, di);
        if (hit) {
            SET_DB(DS_PlayerHurtTimer, 0x2C);
            SET_DB(DS_AttackAnimLock, 0);
            SET_DW(DS_PlayerVelY, 0xFF80);
            SET_DB(DS_MotionShift, 3);
            SET_DW(DS_PlayerVelX, 0xFF80);
            DrainPlayerEnergy();
            bp = 0xFFFF;
            WW(bx, L5P_FRAME, bp);
        } else {
            uint16_t dx = sar16(RW(bx, L5P_YVEL), 4);
            WW(bx, L5P_Y, (uint16_t)(RW(bx, L5P_Y) + dx));
            uint16_t ax = cbw(RB(bx, L5P_DRIFT_STEP));
            WW(bx, L5P_DRIFT, (uint16_t)(RW(bx, L5P_DRIFT) + ax));
            dx = RW(bx, L5P_DRIFT);
            if ((int16_t)dx >= 0x20 || (int16_t)dx < -0x20) WB(bx, L5P_DRIFT_STEP, neg8(RB(bx, L5P_DRIFT_STEP)));
            dx = sar16(RW(bx, L5P_DRIFT), 4);
            if (dx != 0) bp = (uint16_t)(bp + ((int16_t)dx > 0 ? 1 : 2));
            WW(bx, L5P_X, (uint16_t)(RW(bx, L5P_X) + dx));
            WW(di, 0, RW(bx, L5P_X));
            WW(di, 2, RW(bx, L5P_Y));
            if ((int16_t)RW(bx, L5P_Y) > 0x7D8) {
                bp = 0xFFFF;
                WW(bx, L5P_FRAME, bp);
            }
        }
        WW(di, RS_FRAME, bp);
    }
    /* loc_6DDE: phase tables for the parts */
    SET_DW(DS_word_5670, 0xFFFF);
    SET_DW(DS_word_565E, 0xFFFF);
    if (DW(DS_Level5Phase) < 2) {
        uint16_t si = (uint16_t)(0x0C * DB(DS_byte_A329) + 0xA4B5);
        SET_DW(DS_word_566C, DW(si));
        SET_DW(DS_word_566E, DW((uint16_t)(si + 2)));
        SET_DW(DS_word_5670, DW((uint16_t)(si + 4)));
        SET_DW(DS_word_565A, DW((uint16_t)(si + 6)));
        SET_DW(DS_word_565C, DW((uint16_t)(si + 8)));
        SET_DW(DS_word_565E, DW((uint16_t)(si + 10)));
    }
    {
        uint16_t si = (uint16_t)(0x0C * DB(DS_byte_A32A) + 0xA485);
        SET_DW(DS_word_5690, DW(si));
        SET_DW(DS_word_5692, DW((uint16_t)(si + 2)));
        SET_DW(DS_word_5694, DW((uint16_t)(si + 4)));
        SET_DW(DS_word_567E, DW((uint16_t)(si + 6)));
        SET_DW(DS_word_5680, DW((uint16_t)(si + 8)));
        SET_DW(DS_word_5682, DW((uint16_t)(si + 10)));
    }
    {
        uint16_t si = (uint16_t)(6 * DB(DS_byte_A328) + 0xA4E5);
        SET_DW(DS_word_5648, DW(si));
        SET_DW(DS_word_564A, DW((uint16_t)(si + 2)));
        uint16_t third = DW((uint16_t)(si + 4));
        SET_DW(DS_word_564C, DW(DS_Level5Phase) == 0 ? third : 0xFFFF);
    }
    if (!(DB(DS_SnowFrameTick) & 3)) {
        uint8_t al = (uint8_t)(DB(DS_byte_A328) + 1);
        if (al > 2) al = 0;
        SET_DB(DS_byte_A328, al);
    }
    if (DB(DS_byte_A324) == 1) {
        SET_DB(DS_byte_A329, 2);
        SET_DB(DS_byte_A32A, (uint8_t)(DB(DS_byte_A32A) - 1));
        SET_DB(DS_byte_A32B, (uint8_t)(DB(DS_byte_A32B) + 5));
    }
    if (DB(DS_byte_A324) != 0) {
        SET_DB(DS_byte_A324, (uint8_t)(DB(DS_byte_A324) - 1));
        goto loc_6F8F;
    }
    if (DB(DS_byte_A32B) != 0) goto loc_6F8F;
    {
        uint8_t al = (uint8_t)(DB(DS_byte_A329) + 1);
        if (al > 2) al = 0;
        SET_DB(DS_byte_A329, al);
        al = (uint8_t)(DB(DS_byte_A32A) + 1);
        if (al > 2) al = 0;
        SET_DB(DS_byte_A32A, al);
        uint8_t ah = 1;
        if (al != 1) {
            ah = 3;
            SET_DB(DS_ShakeMagnitude, 4);
            SET_DW(DS_PlayerX, (uint16_t)(DW(DS_PlayerX) + 2));
            uint16_t p = LEVEL5_SLOT;
            int found = 0;
            for (int n = 5; n > 0; n--, p = (uint16_t)(p + 0x0B)) {
                if (RW(p, L5P_FRAME) == 0xFFFF) { found = 1; break; }
            }
            if (found) {
                WW(p, L5P_FRAME, 0x1B3);
                WW(p, L5P_Y, (uint16_t)(DW(DS_PlayerY) - 0x96));
                uint8_t r = NextWindRandom();
                uint8_t t = r & 0x7C;
                if (t & 4) t = neg8(t);
                WW(p, L5P_X, (uint16_t)(cbw(t) + DW(DS_PlayerX)));
                uint16_t vel = (uint16_t)((r & 3) + 1);
                WW(p, L5P_YVEL, (uint16_t)(vel << 4));
                WW(p, L5P_DRIFT, 0);
                WB(p, L5P_DRIFT_STEP, 4);
            }
        }
        SET_DB(DS_byte_A32B, ah);
        SET_DB(DS_byte_A32C, (uint8_t)(DB(DS_byte_A32C) - 1));
        if (DB(DS_byte_A32C) == 0) {
            SET_DB(DS_byte_A32C, (uint8_t)((NextWindRandom() & 0x0F) << 3));
            SET_DB(DS_byte_A32B, (uint8_t)(((NextWindRandom() & 0x0F) << 3) + 0x40));
            SET_DB(DS_byte_A329, 0);
        }
    }
    goto loc_6F3C;

loc_6F3C:
    if (DB(DS_RespawnState) != 0) goto loc_70CB;
    if (TestHitboxOverlap(PLAYER_REC, 0x5690u)) goto loc_6F59;
    if (!TestHitboxOverlap(PLAYER_REC, 0x567Eu)) goto loc_6F8F;
loc_6F59:
    if ((int16_t)DW(DS_PlayerY) > 0x7BB) {
        SET_DW(DS_PlayerVelY, 0xFF60);
        SET_DB(DS_PlayerHurtTimer, 0x2C);
        SET_DB(DS_AttackAnimLock, 0);
        SET_DB(DS_MotionShift, 3);
        SET_DW(DS_PlayerVelX, 0xFF80);
        DrainPlayerEnergy();
        DrainPlayerEnergy();
        DrainPlayerEnergy();
    } else {
        SET_DW(DS_PlayerVelY, 0xFF60);
    }
    goto loc_6FBB;

loc_6F8F:
    if (DB(DS_RespawnState) != 0) goto loc_70CB;
    if (!TestHitboxOverlap(PLAYER_REC, 0x5690u)) goto loc_6FBB;
    if (DB(DS_LandingFlag) == 0) goto loc_6FBB;
    {
        uint16_t ax = 0xFFC0;
        if (DB(DS_byte_27EA) != 0) ax = 0xFF80;
        SET_DW(DS_PlayerVelY, ax);
    }

loc_6FBB:
    {
        uint16_t si = (uint16_t)(0x5648 + 0x24 * DB(DS_Level5Phase));
        uint16_t di = DS_RenderSlots;
        if (RW(di, 4) != 0xFFFF) {
            SET_DB(DS_FollowerOverlapMode, 1);
            int hit = TestHitboxOverlap(si, di);
            SET_DB(DS_FollowerOverlapMode, 0);
            if (hit) goto loc_7001;
        }
        di = HAZARD_SLOT1;
        for (int n = 4; n > 0; n--, di = (uint16_t)(di + RS_SIZE)) {
            if (RW(di, 4) == 0xFFFF) continue;
            SET_DB(DS_FollowerOverlapMode, 1);
            int hit = TestHitboxOverlap(si, di);
            SET_DB(DS_FollowerOverlapMode, 0);
            if (hit) goto loc_7001;
        }
        goto loc_70A5;
loc_7001:
        WB(si, 5, RB(si, 5) ^ 0x40);
        if (DW(DS_Level5Phase) != 0) WB((uint16_t)(si - 0x0D), 0, RB((uint16_t)(si - 0x0D), 0) ^ 0x40);
        WW(di, 4, 0xFFFF);
        SET_DB(DS_byte_A324, 6);
        if (DW(DS_Level5Phase) < 2) {
            SET_DB(DS_byte_A329, 3);
            SET_DB(DS_byte_A32A, 3);
        }
        SET_DB(DS_byte_A325, (uint8_t)(DB(DS_byte_A325) - 1));
        if (DB(DS_byte_A325) != 0) goto loc_70A5;
        SET_DB(DS_byte_A325, 7);
        SET_DW(DS_Level5Phase, (uint16_t)(DW(DS_Level5Phase) + 1));
        if (DW(DS_Level5Phase) != 3) goto loc_70A5;
        SET_DW(DS_EffectOriginX, (uint16_t)(DW(DS_word_5648) - 0xC8));
        SET_DW(DS_EffectOriginY, DW(DS_word_564A));
        SpawnFourDebrisParticles();
        SET_DW(DS_EffectOriginX, (uint16_t)(DW(DS_EffectOriginX) + 0x20));
        SET_DW(DS_EffectOriginY, (uint16_t)(DW(DS_EffectOriginY) + 0x20));
        SpawnFourDebrisParticles();
        SET_DW(DS_EffectOriginX, (uint16_t)(DW(DS_EffectOriginX) - 0x40));
        SpawnFourDebrisParticles();
        SET_DW(DS_EffectOriginX, (uint16_t)(DW(DS_EffectOriginX) - 0x20));
        SET_DW(DS_EffectOriginY, (uint16_t)(DW(DS_EffectOriginY) - 0x40));
        SpawnFourDebrisParticles();
        SET_DW(DS_EffectFrame, 0x63);
        uint16_t dz = 0;
    SpawnEffectBurst(0, &dz, 1);
        for (uint16_t p = MIRROR_SLOTS, n = 5; n != 0; n--, p = (uint16_t)(p + RS_SIZE)) WW(p, RS_FRAME, 0xFFFF);
        for (uint16_t p = 0x5648, n = 5; n != 0; n--, p = (uint16_t)(p + RS_SIZE)) WW(p, RS_FRAME, 0xFFFF);
    }

loc_70A5:
    if ((int16_t)DW(DS_PlayerX) <= 0x3D8) goto loc_70CB;
    SET_DB(DS_PlayerHurtTimer, 0x2C);
    SET_DB(DS_AttackAnimLock, 0);
    SET_DW(DS_PlayerVelY, 0xFF70);
    SET_DB(DS_MotionShift, 3);
    SET_DW(DS_PlayerVelX, 0xFF60);
    DrainPlayerEnergy();

loc_70CB:
    SET_DB(DS_byte_A32B, sat_dec8(DB(DS_byte_A32B)));
    return OBJ_OK;
}

/* DrawLevel9Panel (cs:6C0D): one 6 x 7 block of the level-9 panel. Tile indices come from the
 * level segment at 14h + 6*AL; each indexes a 32-byte tile in the VGA window (A000:5E80h +
 * index*32, read through the latches) and is copied to VRAM from 3F58h. */
void DrawLevel9Panel(uint8_t al)
{
    uint16_t di = 0x3F58;
    SetLatchCopyMode();
    uint16_t bx = (uint16_t)(al * 6 + 0x14);
    uint16_t lseg = DW(DS_LevelSegment);
    for (int row = 7; row > 0; row--) {
        for (int col = 6; col > 0; col--) {
            uint8_t idx = rd8(lseg, bx);
            bx = (uint16_t)(bx + 1);
            uint16_t si = (uint16_t)((uint16_t)idx << 5) + 0x5E80u;
            for (int k = 0; k < 16; k++) {
                vga_write(di, vga_read(si));
                si = (uint16_t)(si + 1);
                di = (uint16_t)(di + 1);
                vga_write(di, vga_read(si));
                si = (uint16_t)(si + 1);
                di = (uint16_t)(di + 1);
                di = (uint16_t)(di + 0x26);
            }
            di = (uint16_t)(di - 0x27E);
        }
        bx = (uint16_t)(bx + 0xFA);
        di = (uint16_t)(di + 0x274);
    }
}

/* EmitLevel9Sparkles (cs:6BDB): four single particles at (B9h, 1Eh). */
void EmitLevel9Sparkles(void)
{
    SET_DW(DS_EffectOriginX, 0xB9);
    SET_DW(DS_EffectOriginY, 0x1E);
    uint16_t ax = 0x20, dx = 0xFF60;
    for (int i = 0; i < 4; i++) {
        SET_DW(DS_EffectFrame, 0x2137);
        SpawnEffectBurst(ax, &dx, 1);
        ax = neg16(ax);
        if (!((int16_t)ax < 0)) {
            ax = (uint16_t)(ax - 0x10);
            dx = (uint16_t)(dx - 0x10);
        }
    }
}

/* RunLevel9Script (cs:6ADD): the level-9 bytecode interpreter; one command group per frame. */
int RunLevel9Script(void)
{
    SET_ES(0xA000);                           /* ES = VGA window for the rest of the frame (never restored) */
    if (DW(DS_Level9ScriptPtr) == 0xFFFF) {
        SET_DW(DS_word_A4F7, 0xA4F9);
        SET_DW(DS_word_A519, 0x18);
        SET_DB(DS_byte_A515, 0);
        SET_DB(DS_byte_A516, 3);
    }
    int st = SetGaugeSprites((uint16_t)((int16_t)DW(DS_word_A519) >> 2));
    if (st) return st;
    if (DW(DS_word_A519) == 0) {
        DrawLevel9Panel(2);
        return OBJ_OK;
    }
    if (DB(DS_byte_A515) == 0) {
        uint16_t bx = DW(DS_word_A4F7);
        uint16_t ax = DW(bx);
        if (ax == 0xFFFF) {
            bx = (uint16_t)(bx + DW((uint16_t)(bx + 2)));
            ax = DW(bx);
        }
        SET_DW(DS_Level9ScriptPtr, ax);
        SET_DB(DS_byte_A515, DB((uint16_t)(bx + 2)));
        bx = (uint16_t)(bx + 4);
        SET_DW(DS_word_A4F7, bx);
    }
    uint16_t si = HAZARD_SLOT1;
    for (int n = 4; n > 0; n--, si = (uint16_t)(si + RS_SIZE)) {
        if (RW(si, 4) == 0xFFFF) continue;
        if (((uint16_t)(RW(si, 0) - 0xB9)) >= 0x32) continue;
        if (((uint16_t)(RW(si, 2) - 0x1E)) >= 0x32) continue;
        SET_DW(DS_Level9ScriptPtr, 0xA534);
        uint8_t nv = sat_dec8(DB(DS_word_A519));
        SET_DB(DS_word_A519, nv);
        if (nv == 0) EmitLevel9Sparkles();
        SET_DB(DS_byte_A516, (uint8_t)(DB(DS_byte_A516) + 1));
        SET_DB(DS_byte_A516, DB(DS_byte_A516) & 3);
        if (DB(DS_byte_A516) == 0) SET_DW(DS_Level9ScriptPtr, 0xA540);
        break;
    }
    uint16_t bx = DW(DS_Level9ScriptPtr);
    for (;;) {
        uint8_t al = DB(bx);
        if (al == 0xFF) { SpawnDriftParticle(); bx = (uint16_t)(bx + 1); continue; }
        if (al == 0xFE) { SpawnFallingParticle(); bx = (uint16_t)(bx + 1); continue; }
        if (al == 0xFD) { StartSoundEffect(2); bx = (uint16_t)(bx + 1); continue; }
        if (!(al & 0x80)) {
            SET_DW(DS_Level9ScriptPtr, (uint16_t)(DW(DS_Level9ScriptPtr) + 1));
            DrawLevel9Panel(al);
            return OBJ_OK;
        }
        if (al != 3) SET_DB(DS_byte_A515, sat_dec8(DB(DS_byte_A515)));
        bx = (uint16_t)(bx + cbw(al));
        SET_DW(DS_Level9ScriptPtr, bx);
    }
}

/* SpawnDriftParticle (cs:6CA7): one particle, sprite 1CAh, X velocity -8*(random & 0Fh). */
void SpawnDriftParticle(void)
{
    uint16_t si = PARTICLE_BASE;
    int n;
    for (n = PARTICLE_SLOTS; n > 0; n--, si = (uint16_t)(si + RS_SIZE))
        if (RW(si, RS_FRAME) == 0xFFFF) break;
    if (n == 0) return;
    WW(si, RS_FRAME, 0x1CA);
    WW(si, RS_X, 0xC8);
    WW(si, RS_Y, 0x58);
    WB(si, RS_BLINK, 0);
    WW(si, RS_SCRIPT, 0x84);
    WW(si, RS_FACING, 0xFFFF);
    uint16_t ax = (uint16_t)(NextWindRandom() & 0x0F);
    WW(si, RS_OWNER, neg16((uint16_t)(ax << 3)));
    WW(si, RS_STATE, 0);
}

/* SpawnFallingParticle (cs:6CF1): one particle, sprite 1CBh, X = random - 10h, lifetime 42h. */
void SpawnFallingParticle(void)
{
    uint16_t si = PARTICLE_BASE;
    int n;
    for (n = PARTICLE_SLOTS; n > 0; n--, si = (uint16_t)(si + RS_SIZE))
        if (RW(si, RS_FRAME) == 0xFFFF) break;
    if (n == 0) return;
    WW(si, RS_FRAME, 0x1CB);
    uint16_t ax = (uint16_t)((NextWindRandom() & 0x7F) - 0x10);
    WW(si, RS_X, ax);
    WW(si, RS_Y, 0);
    WB(si, RS_BLINK, 0);
    WW(si, RS_SCRIPT, 0x42);
    WW(si, RS_FACING, 0xFFFF);
    WW(si, RS_OWNER, 0);
    WW(si, RS_STATE, 0);
}

/* unused_81B0 (cs:81B0): a bare STC, nothing calls it. */
int unused_81B0(void)
{
    return 1;
}

/* ---- dispatch and the per-frame update ------------------------------------------------ */

enum { DISPATCH_CHILD = 0, DISPATCH_ENTITY = 1 };

/* Runs the routine at code offset `target` with SI = record. Child routines return a status;
 * entity routines return CF in *cf. Entity-table words that name child routines (and any
 * other offset) are reported, not run. */
static int DispatchHandler(uint16_t target, uint16_t si, int context, int *cf)
{
    *cf = 0;
    switch (target) {
    case CS_UpdateGroundChild:      return context == DISPATCH_CHILD ? UpdateGroundChild(si) : OBJ_DISPATCH_UNSUPPORTED;
    case CS_CullChildOnly:          return context == DISPATCH_CHILD ? CullChildOnly(si) : OBJ_DISPATCH_UNSUPPORTED;
    case CS_UpdateBobChild:         return context == DISPATCH_CHILD ? UpdateBobChild(si) : OBJ_DISPATCH_UNSUPPORTED;
    case CS_UpdateWalkerChild:      return context == DISPATCH_CHILD ? UpdateWalkerChild(si) : OBJ_DISPATCH_UNSUPPORTED;
    case CS_UpdateOrbitChild:       return context == DISPATCH_CHILD ? UpdateOrbitChild(si) : OBJ_DISPATCH_UNSUPPORTED;
    case CS_UpdatePounceChild:      return context == DISPATCH_CHILD ? UpdatePounceChild(si) : OBJ_DISPATCH_UNSUPPORTED;
    case CS_UpdateWanderingChild:   return context == DISPATCH_CHILD ? UpdateWanderingChild(si) : OBJ_DISPATCH_UNSUPPORTED;
    case CS_UpdateLeaperChild:      return context == DISPATCH_CHILD ? UpdateLeaperChild(si) : OBJ_DISPATCH_UNSUPPORTED;
    case CS_UpdateHopChild:         return context == DISPATCH_CHILD ? UpdateHopChild(si) : OBJ_DISPATCH_UNSUPPORTED;
    case CS_UpdatePatrolChild:      return context == DISPATCH_CHILD ? UpdatePatrolChild(si) : OBJ_DISPATCH_UNSUPPORTED;
    case CS_UpdateChargerChild:     return context == DISPATCH_CHILD ? UpdateChargerChild(si) : OBJ_DISPATCH_UNSUPPORTED;
    case CS_UpdateLeapChild:        return context == DISPATCH_CHILD ? UpdateLeapChild(si) : OBJ_DISPATCH_UNSUPPORTED;
    case CS_UpdateLaunchedChild:    return context == DISPATCH_CHILD ? UpdateLaunchedChild(si) : OBJ_DISPATCH_UNSUPPORTED;
    case CS_SpawnDropOverPlayer:    *cf = SpawnDropOverPlayer(si); return OBJ_OK;
    case CS_SpawnChildWhenVisible:  *cf = SpawnChildWhenVisible(si); return OBJ_OK;
    case CS_SpawnFixedChild:        *cf = SpawnFixedChild(si); return OBJ_OK;
    case CS_SpawnChildLatchState37: *cf = SpawnChildLatchState37(si); return OBJ_OK;
    case CS_SpawnChildResetCounters:*cf = SpawnChildResetCounters(si); return OBJ_OK;
    case CS_SpawnChildLatched:      *cf = SpawnChildLatched(si); return OBJ_OK;
    case CS_SpawnChildWithFlags:    *cf = SpawnChildWithFlags(si); return OBJ_OK;
    case CS_SpawnChildOnPlayerBox:  *cf = SpawnChildOnPlayerBox(si); return OBJ_OK;
    case CS_SpawnChildRandomY:      *cf = SpawnChildRandomY(si); return OBJ_OK;
    case CS_SpawnChildOnApproach:   *cf = SpawnChildOnApproach(si); return OBJ_OK;
    default:                        return OBJ_DISPATCH_UNSUPPORTED;
    }
}

/* UpdateObjects (cs:6822): boss, level 5 and level 9 updates, the 12 child slots, then the
 * entity list at 8489h. Returns OBJ_OK or the first boundary status reached. */
int UpdateObjects(void)
{
    int st;
    if (DB(DS_BossState) != 0xFF) {
        st = UpdateBossFight();
        if (st) return st;
    }
    if (DB(DS_CurrentLevel) == 5 && (DB(DS_LevelFlags) & 0xFE) == 0 && DW(DS_Level5Phase) != 3) {
        st = UpdateLevel5Sequence();
        if (st) return st;
    }
    if (DB(DS_CurrentLevel) == 9) {
        st = RunLevel9Script();
        if (st) return st;
    }

    uint16_t si = CHILD_BASE;
    for (unsigned n = 0; n < CHILD_SLOTS; n++, si = (uint16_t)(si + RS_SIZE)) {
        if (RW(si, RS_FRAME) == 0xFFFF) continue;
        WW(si, RS_Y, RW(si, RS_Y) + sar16(RW(si, RS_YVEL), 4));
        if (RW(si, RS_XVEL) != 0xFFFF) WW(si, RS_X, RW(si, RS_X) + sar16(RW(si, RS_XVEL), 4));

        uint16_t owner = RW(si, RS_OWNER);
        if (RB(owner, ENT_FLAGS) & 8) UpdateObjectTerrain(si);

        /* script: negative words are relative jumps; other words are frame commands */
        uint16_t bx = RW(si, RS_SCRIPT);
        uint16_t dx;
        for (;;) {
            dx = DW(bx);
            if ((int16_t)dx >= 0) break;
            bx = (uint16_t)(bx + dx);
        }
        uint8_t ah = (uint8_t)((dx >> 8) & 0xE0) | DB(DS_word_6BE2);
        SET_DB(DS_ScriptFrameFlags, ah);
        dx = (uint16_t)(dx + 0x138);
        int notfound = 0;
        if (DW(DS_word_6BE2) == 0) {
            notfound = 1;
        } else {
            if (DW(DS_word_6BE2) == 7) SET_DB(DS_ShakeMagnitude, 9);
            dx &= 0x1FFF;
            uint16_t di = FRAME_MAP;
            for (;;) {
                if (dx < DW(di)) { notfound = 1; break; }
                if (dx <= DW((uint16_t)(di + 2))) {
                    dx = (uint16_t)(DW((uint16_t)(di + 4)) + 0x35);
                    break;
                }
                di = (uint16_t)(di + 6);
            }
        }
        if (notfound) {
            bx = (uint16_t)(bx + 2);
            WW(si, RS_SCRIPT, bx);
        }
        dx &= 0x1FFF;
        dx |= (uint16_t)(RB(si, RS_FACING) & 0x80) << 8;
        WW(si, RS_FRAME, (RW(si, RS_FRAME) & 0x6000) | dx);

        int cf = 0;
        uint16_t index = (uint8_t)(RB(owner, ENT_KIND) << 1);
        uint16_t es_saved = pre2_es;          /* the handler call sits between push es / pop es */
        st = DispatchHandler(CW((uint16_t)(CS_ChildHandlerTable + index)), si, DISPATCH_CHILD, &cf);
        if (st) return st;
        pre2_es = es_saved;
    }

    for (uint16_t e = ENTITY_LIST; ; ) {
        uint8_t stride = DB(e);
        if (stride >= 0x32) break;
        if (RW(e, ENT_SPRITE) != 0xFFFF && !(RB(e, ENT_FLAGS) & 4) &&
            (DB(DS_byte_B198) == 1 || !(RB(e, ENT_KIND) & 0x80))) {
            uint16_t index = (uint8_t)(RB(e, ENT_KIND) << 1);
            int cf = 0;
            uint16_t es_saved = pre2_es;      /* the handler call sits between push es / pop es */
            st = DispatchHandler(CW((uint16_t)(CS_EntityHandlerTable + index)), e, DISPATCH_ENTITY, &cf);
            if (st) return st;
            pre2_es = es_saved;
            if (!cf) {
                /* spawned: the child's script pointer comes from the table at ds:A86Fh */
                uint16_t ax = (uint16_t)(RW(e, ENT_SPRITE) - 0x138);
                uint16_t key = RB(e, ENT_KIND) & 0x7F;
                uint16_t bx = ENTITY_SCRIPT;
                uint32_t guard = 0;
                for (;;) {
                    bx = (uint16_t)(bx + 2);
                    if (DW(bx) == 0x7D01 && DW((uint16_t)(bx + 2)) == key) break;
                    if (++guard > 0x10000) return OBJ_RUNAWAY;
                }
                bx = (uint16_t)(bx + 4);
                guard = 0;
                while (DW(bx) != ax) {
                    bx = (uint16_t)(bx + 2);
                    if (++guard > 0x10000) return OBJ_RUNAWAY;
                }
                WW(DW(DS_LastSpawnedChild), RS_SCRIPT, bx);
            }
        }
        if (stride == 0) return OBJ_RUNAWAY;   /* the original repeats this record forever */
        e = (uint16_t)(e + stride);
    }
    return OBJ_OK;
}
/* ---- register adapters (differential tests, tests/objects.py) ---------------------------- */
EXPORT int c_UpdateObjects(Regs *r) { int st = UpdateObjects(); r->ds = DATA_SEG; return st; }

EXPORT int c_UpdateGroundChild(Regs *r) { int st = UpdateGroundChild(r->si); r->ds = DATA_SEG; return st; }

EXPORT int c_CullChildOnly(Regs *r) { int st = CullChildOnly(r->si); r->ds = DATA_SEG; return st; }

EXPORT int c_UpdateBobChild(Regs *r) { int st = UpdateBobChild(r->si); r->ds = DATA_SEG; return st; }

EXPORT int c_UpdateWalkerChild(Regs *r) { int st = UpdateWalkerChild(r->si); r->ds = DATA_SEG; return st; }

EXPORT int c_UpdateOrbitChild(Regs *r) { int st = UpdateOrbitChild(r->si); r->ds = DATA_SEG; return st; }

EXPORT int c_UpdatePounceChild(Regs *r) { int st = UpdatePounceChild(r->si); r->ds = DATA_SEG; return st; }

EXPORT int c_UpdateWanderingChild(Regs *r) { int st = UpdateWanderingChild(r->si); r->ds = DATA_SEG; return st; }

EXPORT int c_UpdateLeaperChild(Regs *r) { int st = UpdateLeaperChild(r->si); r->ds = DATA_SEG; return st; }

EXPORT int c_UpdateHopChild(Regs *r) { int st = UpdateHopChild(r->si); r->ds = DATA_SEG; return st; }

EXPORT int c_UpdatePatrolChild(Regs *r) { int st = UpdatePatrolChild(r->si); r->ds = DATA_SEG; return st; }

EXPORT int c_UpdateChargerChild(Regs *r) { int st = UpdateChargerChild(r->si); r->ds = DATA_SEG; return st; }

EXPORT int c_UpdateLeapChild(Regs *r) { int st = UpdateLeapChild(r->si); r->ds = DATA_SEG; return st; }

EXPORT int c_UpdateLaunchedChild(Regs *r) { int st = UpdateLaunchedChild(r->si); r->ds = DATA_SEG; return st; }

EXPORT int c_SpawnDropOverPlayer(Regs *r) { set_cf(r, SpawnDropOverPlayer(r->si)); r->ds = DATA_SEG; return 0; }

EXPORT int c_SpawnChildWhenVisible(Regs *r) { set_cf(r, SpawnChildWhenVisible(r->si)); r->ds = DATA_SEG; return 0; }

EXPORT int c_SpawnFixedChild(Regs *r) { set_cf(r, SpawnFixedChild(r->si)); r->ds = DATA_SEG; return 0; }

EXPORT int c_SpawnChildLatchState37(Regs *r) { set_cf(r, SpawnChildLatchState37(r->si)); r->ds = DATA_SEG; return 0; }

EXPORT int c_SpawnChildResetCounters(Regs *r) { set_cf(r, SpawnChildResetCounters(r->si)); r->ds = DATA_SEG; return 0; }

EXPORT int c_SpawnChildLatched(Regs *r) { set_cf(r, SpawnChildLatched(r->si)); r->ds = DATA_SEG; return 0; }

EXPORT int c_SpawnChildWithFlags(Regs *r) { set_cf(r, SpawnChildWithFlags(r->si)); r->ds = DATA_SEG; return 0; }

EXPORT int c_SpawnChildOnPlayerBox(Regs *r) { set_cf(r, SpawnChildOnPlayerBox(r->si)); r->ds = DATA_SEG; return 0; }

EXPORT int c_SpawnChildRandomY(Regs *r) { set_cf(r, SpawnChildRandomY(r->si)); r->ds = DATA_SEG; return 0; }

EXPORT int c_SpawnChildOnApproach(Regs *r) { set_cf(r, SpawnChildOnApproach(r->si)); r->ds = DATA_SEG; return 0; }

EXPORT int c_UpdateObjectTerrain(Regs *r) { UpdateObjectTerrain(r->si); r->ds = DATA_SEG; return 0; }

EXPORT int c_ScriptStepBack(Regs *r) { ScriptStepBack(r->si); r->ds = DATA_SEG; return 0; }

EXPORT int c_ScriptStepForward(Regs *r) { ScriptStepForward(r->si); r->ds = DATA_SEG; return 0; }

EXPORT int c_ComputeBobOffset(Regs *r) { r->ax = ComputeBobOffset(r->si, (uint8_t)r->ax); r->ds = DATA_SEG; return 0; }

EXPORT int c_AllocChildRecord(Regs *r) { uint16_t di = 0; set_cf(r, AllocChildRecord(&di)); r->di = di; r->ds = DATA_SEG; return 0; }

EXPORT int c_AllocDebrisRecord(Regs *r) { r->di = AllocDebrisRecord(); r->ds = DATA_SEG; return 0; }

EXPORT int c_IsInCameraWindow(Regs *r) { set_cf(r, IsInCameraWindow(r->ax, r->dx)); r->ds = DATA_SEG; return 0; }

EXPORT int c_CullChildAt(Regs *r) { CullChildAt(r->si, r->ax, r->dx); r->ds = DATA_SEG; return 0; }

EXPORT int c_TickEntityTimer(Regs *r) { set_cf(r, TickEntityTimer(r->bx)); r->ds = DATA_SEG; return 0; }

EXPORT int c_SpawnDebrisLine(Regs *r) { SpawnDebrisLine(r->bx, (uint8_t)r->ax, (uint8_t)r->dx); r->ds = DATA_SEG; return 0; }

EXPORT int c_SkipScriptToMarker(Regs *r) { SkipScriptToMarker(r->di); r->ds = DATA_SEG; return 0; }

EXPORT int c_SetGaugeSprites(Regs *r) { int st = SetGaugeSprites(r->cx); r->ds = DATA_SEG; return st; }

EXPORT int c_UpdateBossFight(Regs *r) { int st = UpdateBossFight(); r->ds = DATA_SEG; return st; }

EXPORT int c_UpdateLevel5Sequence(Regs *r) { int st = UpdateLevel5Sequence(); r->ds = DATA_SEG; return st; }

EXPORT int c_RunLevel9Script(Regs *r) { int st = RunLevel9Script(); r->ds = DATA_SEG; return st; }

EXPORT int c_DrawLevel9Panel(Regs *r) { DrawLevel9Panel((uint8_t)r->ax); r->ds = DATA_SEG; return 0; }

EXPORT int c_EmitLevel9Sparkles(Regs *r) { EmitLevel9Sparkles(); r->ds = DATA_SEG; return 0; }

EXPORT int c_SpawnDriftParticle(Regs *r) { SpawnDriftParticle(); r->ds = DATA_SEG; return 0; }

EXPORT int c_SpawnFallingParticle(Regs *r) { SpawnFallingParticle(); r->ds = DATA_SEG; return 0; }

EXPORT int c_CheckBossHits(Regs *r) { CheckBossHits(); r->ds = DATA_SEG; return 0; }

EXPORT int c_CheckPlayerBossContact(Regs *r) { CheckPlayerBossContact(); r->ds = DATA_SEG; return 0; }

EXPORT int c_DrainPlayerEnergy(Regs *r) { DrainPlayerEnergy(); r->ds = DATA_SEG; return 0; }

EXPORT int c_IncrementBossCounter(Regs *r) { IncrementBossCounter(); r->ds = DATA_SEG; return 0; }

EXPORT int c_unused_81B0(Regs *r) { set_cf(r, unused_81B0()); r->ds = DATA_SEG; return 0; }

EXPORT int c_CheckHazardHitsBoss(Regs *r) { set_cf(r, CheckHazardHitsBoss(r->si)); r->ds = DATA_SEG; return 0; }

EXPORT int c_ApplyBossPartContact(Regs *r) { ApplyBossPartContact(r->di); r->ds = DATA_SEG; return 0; }








EXPORT int c_BossHit(Regs *r) { BossHit(r->si); r->ds = DATA_SEG; return 0; }

EXPORT int c_ChildDeath(Regs *r) { ChildDeath(r->si); r->ds = DATA_SEG; return 0; }

EXPORT int c_ChildFallOrDie(Regs *r) { ChildFallOrDie(r->si); r->ds = DATA_SEG; return 0; }

EXPORT int c_CullChildFarFromPlayer(Regs *r) { CullChildFarFromPlayer(r->si); r->ds = DATA_SEG; return 0; }

EXPORT int c_SetChildOrbitPosition(Regs *r) { SetChildOrbitPosition(r->si, r->bx); r->ds = DATA_SEG; return 0; }

EXPORT int c_SetChildDirToPlayer(Regs *r) { SetChildDirToPlayer(r->si); r->ds = DATA_SEG; return 0; }

EXPORT int c_HopStart(Regs *r) { HopStart(r->si, r->bx); r->ds = DATA_SEG; return 0; }

EXPORT int c_SpawnChildAtOwner(Regs *r) { uint16_t di = 0; set_cf(r, SpawnChildAtOwner(r->si, &di)); r->di = di; r->ds = DATA_SEG; return 0; }

