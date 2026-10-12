"""Software mixer and MOD tracker: src/MODULE1.ASM (MixChannel, TrackerTick, ProcessChannelCell,
the EffectJumpTable handlers, and the SbIrqHandler block mix entered at loc_20CF) vs c/sound.c.

State is built explicitly in each setup (the initial snapshot is taken after setup):
  * volume table at DS:12C1h, generated as the SB init does (VolumeTable[v*64+s] = v*s>>6);
  * instrument records at DS:0BD2h (16 bytes each: length, volume word, loop start, loop
    length, sample offset, sample segment), sample k at segment 5000h + 100h*k;
  * channel arrays, song state, order table; patterns at ModuleSegment = 3000h with pattern p
    at segment 3000h + 40h*p (cells are 4 bytes at row*16 + channel*4);
  * the SFX bank at 6000h; the block-mix entry runs with SS = 7000h, SP = 0300h and an IRET
    frame at 02FEh (its SbIrqExit pops return to cs:0000, the harness stop address);
  * `real module` cases decode BOULA.TRK with the original LoadSqzFile and read the MOD
    layout from the decoded resource (the same fields ActivateResourceSlot fills in).
"""
import random
import zlib
from difftest import Case
from common import ROOT
from emu import DATA_SEG, CODE_SEG

PAT_SEG, SMP_SEG, SFX_SEG, STK_SEG = 0x3000, 0x5000, 0x6000, 0x7000
VOLUME_TABLE = 0x12C1
CHANNELS = (0, 2, 4, 6)
CHAN_FIELDS = dict(pos='ChannelPos', end='ChannelEnd', inst='ChannelInstrument', effect='ChannelEffect',
                   step='ChannelStep', note='ChannelNote', vol='ChannelVolume', slide='ChannelVolumeSlide',
                   phase='ChannelPhase')

# -- state writers ----------------------------------------------------------------------
def sample_seg(k):
    return SMP_SEG + 0x100 * k

def chan(m, ch, **kw):
    for k, v in kw.items():
        m.ww(m.data(CHAN_FIELDS[k]) + ch, v)

def inst(m, k, length, vol, ls, ll, off=0):
    rec = m.data('InstrumentTable') + 16 * k
    for o, v in ((0, length), (2, vol), (4, ls), (6, ll), (8, off), (10, sample_seg(k))):
        m.ww(rec + o, v)

def sample(m, k, data, off=0):
    m.write(sample_seg(k), off, data)

def volume_table(m):
    m.write(DATA_SEG, VOLUME_TABLE, bytes((v * s) >> 6 for v in range(65) for s in range(64)))

def mix_buffer(m, rng, n=0xB0):
    m.write(DATA_SEG, m.rw(m.data('MixBuffer')), bytes(rng.randrange(256) for _ in range(n)))

def cell_bytes(note=0, inst_no=0, cmd=0, param=0, hi=False):
    w0 = (note & 0x7FFF) | (0x8000 if hi else 0) | ((inst_no & 0x10) << 8)
    b2 = ((inst_no & 0x0F) << 4) | (cmd & 0x0F)
    return bytes((w0 & 0xFF, w0 >> 8, b2, param & 0xFF))

def put_cell(m, pat, row, ch, cell):
    m.write(PAT_SEG + 0x40 * pat, row * 16 + ch * 4, cell)

def random_cell(rng):
    if rng.random() < 0.25:
        return bytes(4)
    note = rng.choice([0] * 3 + list(range(1, 37)))
    inst_no = rng.randrange(32) if rng.random() < 0.6 else 0
    cmd = rng.choice([0, 0, 0x0A, 0x0B, 0x0C, 0x0D, 0x0F, 0x0E, 0x09, 0x01])
    param = rng.randrange(0, 8) if cmd == 0x0B else rng.randrange(256)
    return cell_bytes(note, inst_no, cmd, param, hi=rng.random() < 0.05)

def random_pattern(m, pat, rng):
    for row in range(64):
        for ch in range(4):
            put_cell(m, pat, row, ch, random_cell(rng))

def song(m, speed=1, ticks=1, order_pos=0, row=0, orders=(0,), length=6):
    m.wb(m.data('SongSpeed'), speed)
    m.wb(m.data('SongTickCount'), ticks)
    m.ww(m.data('SongOrderPos'), order_pos)
    m.ww(m.data('SongRow'), row)
    m.ww(m.data('ModuleSegment'), PAT_SEG)
    m.ww(m.data('PatternDataOffset'), 0)
    m.wb(m.data('SongLength'), length)
    for i, p in enumerate(orders):
        m.wb(m.data('OrderTable') + i, p)

def default_instruments(m, rng):
    for k in range(32):
        end = rng.randrange(0x200, 0x2000)
        inst(m, k, length=end, vol=rng.randrange(0, 0x41), ls=rng.randrange(0, 0x100),
             ll=rng.randrange(0, 0x80), off=rng.randrange(0, 0x400))

def stack_frame(m):
    """IRET frame for SbIrqExit: popped bp, es, ds, si, di, dx, cx, bx, ax, then IP, CS, FLAGS."""
    words = (0, DATA_SEG, DATA_SEG, 0, 0, 0, 0, 0, 0)
    for i, v in enumerate(words):
        m.ww(0x02FE + 2 * i, v, seg=STK_SEG)
    m.ww(0x0310, 0, seg=STK_SEG)
    m.ww(0x0312, CODE_SEG, seg=STK_SEG)
    m.ww(0x0314, 0x0202, seg=STK_SEG)

def case(title, routine, cfunc, setup, regs=None, files=False):
    return Case(title, routine, cfunc, regs=regs or {}, setup=setup, out_regs=(), files=files)

# -- MixChannel ------------------------------------------------------------------------
def mix_setup(seed, chans, insts, samples=None, buf=None):
    """chans: {ch: {field: value}}; insts: {k: (length, vol, ls, ll, off)}."""
    def setup(m):
        rng = random.Random(seed)
        volume_table(m)
        mix_buffer(m, rng)
        if buf:
            buf(m)
        for k in range(8):
            if samples is None:
                sample(m, k, bytes(rng.randrange(64) for _ in range(0x1000)))
            else:
                sample(m, k, samples(k))
        for k, p in insts.items():
            inst(m, k, *p)
        for ch, kw in chans.items():
            chan(m, ch, **kw)
    return setup

def mix_case(title, chans, insts, **kw):
    return case(title, 'MixChannel', 'c_MixChannel', mix_setup(zlib.crc32(title.encode()) & 0xFFFF, chans, insts, **kw),
                regs={'BX': min(chans)})

def mix_cases():
    cases = []
    one = {0: (0x1000, 0x20, 0, 0, 0x20)}
    cases.append(mix_case('silent channel (pos FFFFh)',
                          {2: dict(pos=0xFFFF, end=0x0100, inst=0, vol=0x20, step=0x0100)}, one))
    cases.append(mix_case('plain playback, step 1.5, no end',
                          {0: dict(pos=0x0100, end=0x0800, inst=0, vol=0x20, step=0x0080, phase=0x0040)},
                          {0: (0x1000, 0x20, 0x0100, 0x0200, 0x20)}))
    cases.append(mix_case('plain playback, step 2.x with fraction',
                          {0: dict(pos=0x0010, end=0x0FF0, inst=0, vol=0x18, step=0x0281, phase=0x0020)},
                          {0: (0x1000, 0x18, 0, 0, 0)}))
    cases.append(mix_case('one-shot ends exactly at block end',
                          {4: dict(pos=0x0200, end=0x0200 + 168, inst=0, vol=0x30, step=0x0000)},
                          {0: (0x1000, 0x30, 0, 0, 0x20)}))
    cases.append(mix_case('one-shot ends inside block: fade with P != 0',
                          {0: dict(pos=0x0200, end=0x0232, inst=0, vol=0x40, step=0x0000)},
                          {0: (0x1000, 0x40, 0, 0, 0x20)}))

    def zero_p(m):
        m.wb(m.rw(m.data('MixBuffer')) + 50, 0)     # P (byte before the wrap) == 0: no fade
    cases.append(mix_case('one-shot ends inside block: P == 0 (no fade)',
                          {0: dict(pos=0x0200, end=0x0232, inst=0, vol=0x00, step=0x0000)},
                          {0: (0x1000, 0x00, 0, 0, 0x20)}, buf=zero_p))
    cases.append(mix_case('one-shot, fade with P = 1 (one byte then stop)',
                          {2: dict(pos=0x0200, end=0x0230, inst=0, vol=0x40, step=0x0000)},
                          {0: (0x1000, 0x40, 0, 0, 0x20)}))
    # loop wrap quirk: wrap at iteration 41, raw loop start 8 read from sample segment,
    # CX not decremented on the wrap iteration (169 bytes written), new end stored at
    # ChannelEnd + 16*index.
    for idx, title in ((0, 'index 0: stray store hits channel 0 end'),
                       (3, 'index 3: stray store hits ChannelVolumeSlide[0]'),
                       (4, 'index 4: stray store hits instrument 0 length'),
                       (5, 'index 5: stray store hits instrument 1 length')):
        insts = {idx: (0x1000, 0x30, 0x0008, 0x0020, 0x0020), 1: (0x0777, 0x11, 0x0004, 0x0010, 0x0040)}
        cases.append(mix_case(f'looped wrap inside block (quirk), {title}',
                              {0: dict(pos=0x0200, end=0x0228, inst=idx, vol=0x30, step=0x0000)},
                              insts))
    cases.append(mix_case('looped sample ends exactly at block end: loop restart, no wrap',
                          {2: dict(pos=0x0100, end=0x0100 + 168, inst=0, vol=0x20, step=0x0000)},
                          {0: (0x1000, 0x20, 0x0010, 0x0040, 0x0020)}))
    cases.append(mix_case('looped, wrap with negative step integer',
                          {0: dict(pos=0x0300, end=0x0400, inst=0, vol=0x28, step=0xFF80, phase=0x0010)},
                          {0: (0x1000, 0x28, 0x0100, 0x0100, 0x0010)}))
    cases.append(mix_case('one-shot, step -2 and loop length 2 (boundary)',
                          {6: dict(pos=0x0300, end=0x0380, inst=0, vol=0x10, step=0xFE00)},
                          {0: (0x1000, 0x10, 0x0000, 0x0002, 0x0000)}))
    # every volume 0..40h (row base volume*64), a volume past the clamp, and the 16-bit wrap
    for v in range(0x41):
        cases.append(mix_case(f'volume {v:02X}h: xlat row, full-range sample bytes',
                              {6: dict(pos=0x0040, end=0x2000, inst=0, vol=v, step=0x0100)},
                              {0: (0x2000, v, 0, 0, 0)},
                              samples=lambda k: random.Random(0x77 + k).randbytes(0x1000)))
    for v in (0x41, 0x80, 0x0400, 0x8000, 0xFFFF):
        cases.append(mix_case(f'volume {v:04X}h: row offset wraps 16 bits',
                              {0: dict(pos=0x0040, end=0x2000, inst=0, vol=v, step=0x0100)},
                              {0: (0x2000, v, 0, 0, 0)}))
    for seed in range(12):
        rng = random.Random(1000 + seed)
        chans, insts = {}, {}
        for k in range(8):
            insts[k] = (0x1000, rng.randrange(0, 0x41), rng.randrange(0x80, 0x180),
                        rng.choice([0, 1, 2, rng.randrange(3, 0x60)]), rng.randrange(0, 0x400))
        for ch in CHANNELS:
            if rng.random() < 0.2:
                chans[ch] = dict(pos=0xFFFF, end=0x0100, inst=0, vol=0x20, step=0x0100)
                continue
            pos = rng.randrange(0, 0x800)
            end = max(pos + rng.randrange(1, 0x200), 0x280)
            chans[ch] = dict(pos=pos, end=end, inst=rng.randrange(8), vol=rng.randrange(0x41),
                             step=rng.randrange(0x10000), phase=rng.randrange(256))
        cases.append(mix_case(f'random channel state {seed}', chans, insts,
                              samples=lambda k, s=seed: random.Random(s * 31 + k).randbytes(0x1000)))
    return cases

# -- effect handlers and cells (direct entry) -----------------------------------------
def handler_case(title, routine, cfunc, al, ch=0, setup_extra=None):
    def setup(m):
        volume_table(m)
        for k in range(8):
            inst(m, k, 0x1000, 0x20, 0, 0, 0)
        chan(m, 0, pos=0x0100, end=0x0800, inst=1, vol=0x20, step=0x0200, slide=0x0011, effect=0)
        chan(m, 2, pos=0x0100, end=0x0800, inst=2, vol=0x22, step=0x0200, slide=0x0022, effect=0)
        chan(m, 4, pos=0x0100, end=0x0800, inst=3, vol=0x24, step=0x0200, slide=0x0033, effect=0)
        chan(m, 6, pos=0x0100, end=0x0800, inst=4, vol=0x26, step=0x0200, slide=0x0044, effect=0)
        song(m, speed=6, ticks=3, order_pos=5, row=9, orders=(0, 1, 2, 3, 4, 5, 6, 7), length=6)
        if setup_extra:
            setup_extra(m)
    return case(title, routine, cfunc, setup, regs={'AX': al, 'DI': ch})

def effect_cases():
    cases = []
    for al in (0x00, 0x03, 0x30, 0x0F, 0xF0, 0xFF, 0x11):
        for ch in (0, 2, 6):
            cases.append(handler_case(f'Axy slide AL={al:02X}h channel {ch}', 'EffectVolumeSlide',
                                      'c_EffectVolumeSlide', al, ch))
    for al in (0x00, 0x3F, 0x40, 0x41, 0x7F, 0x80, 0xFF):
        cases.append(handler_case(f'Cxx volume AL={al:02X}h', 'EffectSetVolume', 'c_EffectSetVolume', al, 4))
    for al in (0x00, 0x01, 0x20, 0x40, 0x7F, 0x80, 0x81, 0xFF):
        cases.append(handler_case(f'Dxx pattern break AL={al:02X}h', 'EffectPatternBreak',
                                  'c_EffectPatternBreak', al))
    for al in (0x00, 0x05, 0x7F, 0xFF):
        cases.append(handler_case(f'Bxx position jump AL={al:02X}h', 'EffectPositionJump',
                                  'c_EffectPositionJump', al))
    for al in (0x00, 0x01, 0x20, 0xFF):
        cases.append(handler_case(f'Fxx speed AL={al:02X}h', 'EffectSetSpeed', 'c_EffectSetSpeed', al))
    return cases

def cell_case(title, cell, ch, setup_extra=None):
    def setup(m):
        volume_table(m)
        for k in range(32):
            inst(m, k, 0x0100 + 0x10 * k, 0x10 + k, 0x0008, 0x0010, 0x0004)
        for c in CHANNELS:
            chan(m, c, pos=0x0040, end=0x0200, inst=0, vol=0x20, step=0x0100, phase=0x0010, slide=0x0005)
        seg = PAT_SEG
        m.write(seg, 0x0100, cell)
        if setup_extra:
            setup_extra(m)
    return case(title, 'ProcessChannelCell', 'c_ProcessChannelCell',
                setup, regs={'ES': PAT_SEG, 'SI': 0x0100, 'DI': ch})

def cell_cases():
    return [
        cell_case('empty cell clears the slide only', bytes(4), 2),
        cell_case('note only: step from table, no instrument', cell_bytes(note=0x21), 0),
        cell_case('note bit 15 ignored, instrument 17 (b1 bit 4)', cell_bytes(0x8000 | 0x0C, 17, 0, 0), 4),
        cell_case('instrument only: restart at 0 with default volume and phase', cell_bytes(0, 5, 0, 0), 6),
        cell_case('instrument 31 with note and unused effect 0Eh', cell_bytes(0x30, 31, 0x0E, 0x77), 0),
        cell_case('unused effect 09h with param', cell_bytes(0x10, 2, 0x09, 0x55), 2),
        cell_case('effect Axy in a cell with a note', cell_bytes(0x14, 3, 0x0A, 0x20), 4),
        cell_case('effect Cxx in a cell with instrument 1', cell_bytes(0x14, 1, 0x0C, 0x90), 6),
    ]

# -- TrackerTick ------------------------------------------------------------------------
def tick_setup(seed, speed=3, ticks=1, order_pos=0, row=0, orders=(1, 3, 2, 5, 0, 4, 6, 7), length=6,
               chans=None, cells=None, rand_patterns=(1, 3, 2, 5, 0, 4, 6, 7), pdo=0):
    def setup(m):
        rng = random.Random(seed)
        volume_table(m)
        mix_buffer(m, rng)
        default_instruments(m, rng)
        for p in rand_patterns:
            random_pattern(m, p, rng)
        song(m, speed=speed, ticks=ticks, order_pos=order_pos, row=row, orders=orders, length=length)
        m.ww(m.data('PatternDataOffset'), pdo)
        for ch in CHANNELS:
            kw = dict(pos=rng.randrange(0x0200), end=0x0400 + rng.randrange(0x400), inst=rng.randrange(8),
                      vol=rng.randrange(0x41), step=rng.randrange(0x10000), phase=rng.randrange(256),
                      slide=0, effect=0, note=0)
            kw.update((chans or {}).get(ch, {}))
            chan(m, ch, **kw)
        for (pat, r, c), cell in (cells or {}).items():
            put_cell(m, pat, r, c, cell)
    return setup

def tick_case(title, seed=1, **kw):
    return case(title, 'TrackerTick', 'c_TrackerTick', tick_setup(seed, **kw))

def tick_cases():
    cases = []
    cases.append(tick_case('slides applied, countdown only (ticks 3 -> 2)', ticks=3,
                           chans={0: dict(slide=3, vol=2), 2: dict(slide=0xFFFB, vol=2),
                                  4: dict(slide=0x30, vol=0x30), 6: dict(slide=0xFFF0, vol=0x08)}))
    cases.append(tick_case('row processed on countdown 1 -> 0, plain row', ticks=1, speed=5, row=5))
    # early channel jump: Bxx on channel 0; channels 1-3 still play row 7 of the old order
    cases.append(tick_case('Bxx on channel 0: channels 1-3 use the latched row', ticks=1, row=7,
                           cells={(1, 7, 0): cell_bytes(20, 2, 0x0B, 2), (1, 7, 1): cell_bytes(33, 5, 0x0C, 0x30),
                                  (1, 7, 2): cell_bytes(9, 0, 0x0A, 0x31), (1, 7, 3): cell_bytes(12, 9, 0x0F, 2)}))
    cases.append(tick_case('Dxx on channel 1: row 20h, order +1', ticks=1, row=7,
                           cells={(1, 7, 1): cell_bytes(15, 1, 0x0D, 0x21), (1, 7, 0): cell_bytes(40, 3)}))
    cases.append(tick_case('Dxx 40h on channel 3: row 64 wraps and the order increments twice', ticks=1,
                           row=10, order_pos=0,
                           cells={(1, 10, 3): cell_bytes(10, 0, 0x0D, 0x40)}))
    cases.append(tick_case('Dxx 00h: row FFFFh then +1 gives row 0, order +1', ticks=1, row=10,
                           cells={(1, 10, 0): cell_bytes(10, 0, 0x0D, 0x00)}))
    cases.append(tick_case('speed 0: countdown 0 -> FFh, no row', ticks=0, speed=0, row=3))
    cases.append(tick_case('speed 0: reload 0 and process the row (256-tick rows)', ticks=1, speed=0, row=3))
    cases.append(tick_case('order wrap: next order equal to SongLength is kept', ticks=1, row=63,
                           order_pos=5, length=6))
    cases.append(tick_case('order wrap: next order above SongLength goes to 0', ticks=1, row=63,
                           order_pos=6, length=6))
    cases.append(tick_case('order wrap quirk: low byte 00h of 100h is kept', ticks=1, row=63,
                           order_pos=0xFF, length=0x80))
    cases.append(tick_case('Fxx: new speed read at the next reload, not this tick', ticks=1, speed=5, row=4,
                           cells={(1, 4, 0): cell_bytes(0, 0, 0x0F, 0x03)}))
    cases.append(tick_case('Axy and Cxx in one row: slides for the next tick', ticks=1, row=8,
                           cells={(1, 8, 0): cell_bytes(0, 0, 0x0A, 0x30), (1, 8, 1): cell_bytes(0, 0, 0x0A, 0x0F),
                                  (1, 8, 2): cell_bytes(0, 0, 0x0C, 0x80), (1, 8, 3): cell_bytes(0, 0, 0x0C, 0x3F)},
                           chans={0: dict(vol=0x20), 2: dict(vol=0x10), 4: dict(vol=0x08), 6: dict(vol=0x30)}))
    cases.append(tick_case('empty cell clears slide, instrument-only cell restarts', ticks=1, row=12,
                           chans={2: dict(slide=5, pos=0x0100), 4: dict(pos=0x0100, phase=0x0080, step=0x0123)},
                           cells={(1, 12, 1): bytes(4), (1, 12, 2): cell_bytes(0, 2, 0, 0)}))
    cases.append(tick_case('note bit 15 ignored and instrument 17 from byte 1', ticks=1, row=13,
                           cells={(1, 13, 0): cell_bytes(0x0C, 17, 0, 0, hi=True)}))
    cases.append(tick_case('unused effects 0Eh/09h/00h with params change nothing', ticks=1, row=14,
                           cells={(1, 14, 0): cell_bytes(0x20, 4, 0x0E, 0x40), (1, 14, 1): cell_bytes(0, 0, 0x09, 9),
                                  (1, 14, 2): cell_bytes(0, 0, 0x00, 0x55)}))
    cases.append(tick_case('row from a non-zero pattern data offset', ticks=1, row=2, pdo=0x0100))
    for seed in range(6):
        rng = random.Random(500 + seed)
        cases.append(tick_case(f'random tick state {seed}', seed=500 + seed, ticks=rng.randrange(1, 3),
                               speed=rng.randrange(1, 9), row=rng.randrange(64), order_pos=rng.randrange(6),
                               length=rng.choice([6, 9, 0x20]), orders=tuple(rng.randrange(8) for _ in range(8))))
    return cases

# -- SbIrqHandler block mix (entry loc_20CF, after the port work) ---------------------------
def block_setup(seed, left, play_pos, options=0, sfx_seed=None):
    def setup(m):
        rng = random.Random(seed)
        volume_table(m)
        mix_buffer(m, rng)
        for k in range(8):
            sample(m, k, bytes(rng.randrange(64) for _ in range(0x1000)))
        for k in range(8):
            inst(m, k, 0x1000, rng.randrange(0x41), rng.randrange(0x80, 0x180), rng.choice([0, 1, 0x30]),
                 rng.randrange(0, 0x20))
        for ch in CHANNELS:
            chan(m, ch, pos=rng.randrange(0x40, 0x200), end=0x0800, inst=rng.randrange(8),
                 vol=rng.randrange(0x41), step=rng.randrange(0x10000), phase=rng.randrange(256))
        chan(m, 6, pos=0x0040, end=0x0900, inst=3, vol=0x30, step=0x0100)
        m.ww(m.data('SfxBytesLeft'), left)
        m.ww(m.data('SfxPlayPos'), play_pos)
        m.ww(m.data('SfxBankSegment'), SFX_SEG)
        m.write(SFX_SEG, 0, bytes(random.Random(sfx_seed or seed).randrange(256) for _ in range(0x10000)))
        m.ww(m.code('OptionBits'), options, seg=CODE_SEG)
        stack_frame(m)
    return setup

def block_cases():
    blk = dict(regs={'SP': 0x0300, 'SS': STK_SEG})
    cases = []
    def add(title, seed, left, play_pos, options=0):
        cases.append(Case(title, 'loc_20CF', 'c_loc_20CF', regs=dict(blk['regs']),
                          setup=block_setup(seed, left, play_pos, options), out_regs=()))
    add('no SFX: silence, mix channels 0, 2, 4 and 6', 1, 0, 0x0000)
    add('SFX of 300h bytes: copy A8h, mix 0, 2, 4 only', 2, 0x0300, 0x0010)
    add('SFX remainder A7h: odd count drops a byte, A6-A7 unwritten', 3, 0x00A7, 0x0100)
    add('SFX of 1 byte: no copy, zero fill 53h words, mix 6', 4, 0x0001, 0x2000)
    add('SFX exactly A8h: copy, nothing left, mix 6', 5, 0x00A8, 0x3000)
    add('SFX copy with OptionBits 40h: copied but no mixing', 6, 0x0200, 0x0400, options=0x0040)
    add('silence with OptionBits 40h: no mixing', 7, 0, 0, options=0x0040)
    add('SFX source offset wraps within the bank segment (FFF0h)', 8, 0x0100, 0xFFF0)
    add('SFX remainder 1 with odd position', 9, 0x0001, 0x0001)
    return cases

# -- real module: BOULA.TRK decoded by the original LoadSqzFile ----------------------------
def module_setup(name, seed, row=0, mix=False):
    """Decodes the TRK resource with the original loader on the oracle side (before the
    snapshot), then fills the tracker state the way ActivateResourceSlot does from the MOD
    header: instrument records (length, volume word = finetune<<8 | volume, loop start and
    length in bytes, sample pointer), rescaled sample bytes ((b + 80h) >> 2), order table,
    pattern count and pattern data offset 1084. With mix=True the channels play the first
    four instruments from their start with positive steps, for SbIrqHandler's block mix."""
    def setup(m):
        rng = random.Random(seed)
        volume_table(m)
        mix_buffer(m, rng)
        m.ww(m.code('ConventionalTopSeg'), 0xA000, seg=CODE_SEG)
        m.ww(m.data('NextFreeSegment'), 0x2530)
        m.write(DATA_SEG, 0x000C, name.encode() + b'\0')
        seg = m.call('LoadSqzFile', {'DX': 0x000C})['AX']
        m.ints[0x21].log.clear()
        hdr = m.read(seg, 0, 1084)
        assert hdr[0x438:0x43C] == b'M.K.', 'not a 31-sample MOD resource'
        songlen = hdr[950]
        orders = hdr[952:952 + 128]
        pats = max(orders[:songlen]) + 1
        cur_seg = seg + pats * 64                    # sample area: patterns first, then samples
        run = 1084                                   # running byte offset (ASM: si, masked per sample)
        for k in range(31):
            p = 20 + 30 * k
            length = int.from_bytes(hdr[p + 22:p + 24], 'big') * 2
            fine, vol = hdr[p + 24], hdr[p + 25]
            ls = int.from_bytes(hdr[p + 26:p + 28], 'big') * 2
            ll = int.from_bytes(hdr[p + 28:p + 30], 'big') * 2
            cur_seg += run >> 4
            off = run & 0xF
            if length:
                raw = m.read(cur_seg, off, length)
                m.write(cur_seg, off, bytes(((b + 0x80) & 0xFF) >> 2 for b in raw))
            rec = m.data('InstrumentTable') + 16 * k
            for o, v in ((0, length), (2, (fine << 8) | vol), (4, ls), (6, ll), (8, off), (10, cur_seg)):
                m.ww(rec + o, v)
            run = off + length
        m.wb(m.data('InstrumentCount'), 31)
        m.wb(m.data('SongLength'), songlen)
        m.write(DATA_SEG, m.data('OrderTable'), orders)
        m.wb(m.data('PatternCount'), pats)
        m.ww(m.data('PatternDataOffset'), 1084)
        m.ww(m.data('ModuleSegment'), seg)
        m.wb(m.data('SongSpeed'), 6)
        m.wb(m.data('SongTickCount'), 1)
        m.ww(m.data('SongOrderPos'), 0)
        m.ww(m.data('SongRow'), row)
        for ch in CHANNELS:
            chan(m, ch, pos=0xFFFF, end=0, inst=0, vol=0, step=0, phase=0, slide=0, effect=0, note=0)
        if mix:
            stack_frame(m)
            m.ww(m.data('SfxBytesLeft'), 0)
            m.ww(m.code('OptionBits'), 0, seg=CODE_SEG)
            picked = [k for k in range(31) if m.rw(m.data('InstrumentTable') + 16 * k) > 0][:4]
            for ch, k in zip(CHANNELS, picked):
                note = 12 + 2 * ch                  # positive steps only: a MOD note range
                chan(m, ch, pos=0, end=m.rw(m.data('InstrumentTable') + 16 * k), inst=k,
                     vol=m.rw(m.data('InstrumentTable') + 16 * k + 2) & 0xFF, step=m.rw(m.data('StepTable') + 2 * note),
                     phase=0, note=note)
    return setup

def module_cases():
    out = [Case('real module BOULA.TRK: tick on row 0 (real cells, instruments, pattern 0)',
                'TrackerTick', 'c_TrackerTick', regs={}, setup=module_setup('BOULA.TRK', 9, row=0),
                out_regs=(), files=True),
           Case('real module BOULA.TRK: tick on row 33 (second tick of a row)',
                'TrackerTick', 'c_TrackerTick', regs={}, setup=module_setup('BOULA.TRK', 9, row=33),
                out_regs=(), files=True)]
    out.append(Case('real module BOULA.TRK: block mix of the first four samples',
                    'loc_20CF', 'c_loc_20CF', regs={'SP': 0x0300, 'SS': STK_SEG},
                    setup=module_setup('BOULA.TRK', 9, mix=True), out_regs=(), files=True))
    return out

CASES = (mix_cases() + effect_cases() + cell_cases() + tick_cases() + block_cases() + module_cases())
