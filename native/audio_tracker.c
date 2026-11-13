/* Faithful PRE2 sequencer, not a standard MOD player. Oracle: c/sound.c. */
#include "audio_internal.h"
#define CHW(name,ch) DW((uint16_t)(DS_##name+(ch)))
#define SET_CHW(name,ch,v) SET_DW((uint16_t)(DS_##name+(ch)),v)
#define REC_W(field,rec) DW((uint16_t)(DS_##field+(rec)))

/* Effect handlers (EffectJumpTable order). */

/* Axy: x = high nibble -> slide +x; x = 0 -> slide -y (16-bit). */
static void EffectVolumeSlide(Audio *a, uint16_t ch, uint8_t al)
{
    uint8_t hi = (uint8_t)(al >> 4);
    uint16_t slide = hi != 0 ? hi : (uint16_t)(0u - al);
    SET_CHW(ChannelVolumeSlide, ch, slide);
}

/* Bxx: order = AL (not range checked), row = FFFFh so the next row is 0. */
static void EffectPositionJump(Audio *a, uint8_t al)
{
    SET_DW(DS_SongOrderPos, al);
    SET_DW(DS_SongRow, 0xFFFFu);
}

/* Cxx: volume = min(sign-extended AL, 40h); a negative AL (>= 80h) compares as large. */
static void EffectSetVolume(Audio *a, uint16_t ch, uint8_t al)
{
    uint16_t v = (uint16_t)(int16_t)(int8_t)al;                /* cbw */
    if (v > 0x40u) v = 0x40u;                                  /* jbe: unsigned */
    SET_CHW(ChannelVolume, ch, v);
}

/* Dxx: row = sign-extended (AL - 1) (8-bit decrement, cbw), order incremented. */
static void EffectPatternBreak(Audio *a, uint8_t al)
{
    uint8_t row = (uint8_t)(al - 1);
    SET_DW(DS_SongRow, (uint16_t)(int16_t)(int8_t)row);
    SET_DW(DS_SongOrderPos, (uint16_t)(DW(DS_SongOrderPos) + 1));
}

/* Fxx: speed = AL, used at the next row reload. */
static void EffectSetSpeed(Audio *a, uint8_t al)
{
    SET_DB(DS_SongSpeed, al);
}

/* Jump through EffectJumpTable (16 code offsets, index = effect command). */
static void run_effect(Audio *a, uint16_t target, uint16_t ch, uint8_t al)
{
    switch (target) {
    case CS_EffectVolumeSlide:  EffectVolumeSlide(a, ch, al); break;
    case CS_EffectPositionJump: EffectPositionJump(a, al); break;
    case CS_EffectSetVolume:    EffectSetVolume(a, ch, al); break;
    case CS_EffectPatternBreak: EffectPatternBreak(a, al); break;
    case CS_EffectSetSpeed:     EffectSetSpeed(a, al); break;
    default: break;                                            /* IgnoreEffect */
    }
}

static void ProcessChannelCell(Audio *a, uint16_t es, uint16_t si, uint16_t ch)
{
    SET_CHW(ChannelVolumeSlide, ch, 0);
    uint16_t w0 = rd16(es, si);                                /* note word */
    uint16_t w2 = rd16(es, (uint16_t)(si + 2));                /* instrument/effect, param */
    if ((w0 | w2) == 0) return;                                /* loc_2310: empty cell */

    uint8_t b1 = (uint8_t)(w0 >> 8);
    uint8_t b2 = (uint8_t)(w2 & 0xFF);                         /* instrument high bits, cmd */
    uint8_t b3 = (uint8_t)(w2 >> 8);                           /* effect parameter */
    uint16_t inst = (uint16_t)((b2 >> 4) | (b1 & 0x10));       /* 1..31, 0 = none */
    uint16_t effect = (uint16_t)(((b2 & 0x0Fu) << 8) | b3);
    SET_CHW(ChannelEffect, ch, effect);

    if (inst != 0) {                                           /* restart with the instrument */
        uint16_t idx = (uint16_t)(inst - 1);
        uint16_t rec = (uint16_t)(idx << 4);
        SET_CHW(ChannelInstrument, ch, idx);
        SET_CHW(ChannelPos, ch, 0);
        SET_CHW(ChannelEnd, ch, REC_W(InstrumentTable, rec));
        SET_CHW(ChannelVolume, ch, REC_W(word_0BD4, rec));
        SET_CHW(ChannelPhase, ch, 0);
    }
    uint16_t note = w0 & 0x7FFFu;
    if (note != 0) {
        SET_CHW(ChannelNote, ch, note);
        SET_CHW(ChannelStep, ch, DW((uint16_t)((note << 1) + DS_StepTable)));
    }
    uint16_t cmd = (uint16_t)(effect >> 8);
    run_effect(a, DW((uint16_t)(DS_EffectJumpTable + 2u * cmd)), ch, b3);
}

void audio_tracker(Audio *a)
{
    /* 1. volume slides, clamped to 0..40h */
    for (uint16_t ch = 0; ch < 8; ch += 2) {
        uint16_t slide = CHW(ChannelVolumeSlide, ch);
        if (slide != 0) {
            uint16_t v = (uint16_t)(CHW(ChannelVolume, ch) + slide);
            if (v & 0x8000u) v = 0;                            /* jns: negative -> 0 */
            if (v > 0x40u) v = 0x40u;
            SET_CHW(ChannelVolume, ch, v);
        }
    }
    /* 2. countdown; the row is processed when it reaches 0 */
    uint8_t ticks = (uint8_t)(DB(DS_SongTickCount) - 1);
    SET_DB(DS_SongTickCount, ticks);
    if (ticks != 0) return;
    SET_DB(DS_SongTickCount, DB(DS_SongSpeed));

    /* 3. cells of the current row for channels 0-3; es and si are latched before them */
    uint16_t pattern = DB((uint16_t)(DS_OrderTable + DW(DS_SongOrderPos)));
    uint16_t es = (uint16_t)((pattern << 6) + DW(DS_ModuleSegment));
    /* The row address is latched across all four effects. */
    uint16_t si = (uint16_t)((DW(DS_SongRow) << 4) + DW(DS_PatternDataOffset));
    ProcessChannelCell(a, es, si, 0);
    ProcessChannelCell(a, es, (uint16_t)(si + 4), 2);
    ProcessChannelCell(a, es, (uint16_t)(si + 8), 4);
    ProcessChannelCell(a, es, (uint16_t)(si + 12), 6);

    /* 4. next row; at 64 the next order (kept while its low byte <= SongLength) */
    uint16_t row = (uint16_t)(DW(DS_SongRow) + 1);
    SET_DW(DS_SongRow, row);
    if (row != 0x40u) return;
    uint16_t next = (uint16_t)(DW(DS_SongOrderPos) + 1);
    if (DB(DS_SongLength) < (uint8_t)next) next = 0;           /* cmp byte, al; jae */
    SET_DW(DS_SongOrderPos, next);
    SET_DW(DS_SongRow, 0);
}

