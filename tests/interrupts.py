"""Interrupt bodies (src/MODULE1.ASM loc_07AD, loc_17C0, SpeakerTimerIsr, SampleTimerIsr,
SbIrqHandler) vs c/interrupts.c.

Entry. Each handler is entered as the hardware would enter it: the stack holds the interrupt
frame at SS:SP (IP = 0 as the harness pushes it, then CS = CODE_SEG and FLAGS at SP+2 and
SP+4, written by the case). The original ends with IRET, which returns to cs:0000 where the
oracle stops. The C adapter calls the C body; the end of the interrupt is the EOI written by
the body (port 20h), which both sides log.

Chain. loc_17C0 with DOS busy jumps to the previous INT 08h vector (Int08SwapSlot). The slot
points at a stub in free memory (7000h:0000h) holding IRET; the oracle runs it, the C side
calls platform_chain_int08 (counted by the test model, pre2_int08_chains).

The in-DOS flag. DosCallActive is one DATA byte; the original addresses it as ds: (loc_17C0) and
as cs: (SampleTimerIsr, the DOS wrappers), and the cs: form is the same byte (CS:C667h = DS:2877h).
The cases set that byte once.

Port traffic: the port log compares every OUT (PIT, PIC, DMA, DSP, the OPL and SB ports).
Sample output is written to port 389h, PIT ch0 to 40h, the speaker sequence to 42h, the motor
off to 3F2h. The SB ack read (SbAckPort) returns 0 from the preset.
"""
import struct
from difftest import Case
from emu import DATA_SEG, CODE_SEG

FREE = 0x2530
STACK_SEG = 0x1010            # SS of the harness (emu.STACK_SEG)
STUB_SEG = 0x7000             # previous INT 08h vector: an IRET in free memory
REC = 0xF200                  # DATA offset of a speaker sequence record (unused DATA)
BANK_SEG = 0x7100             # SFX bank for the sample output
HEAP = [(FREE + 0x1000 * k, 0, 0x10000) for k in range(4)]

def cwrite(m, name, value, size=1):
    (m.wb if size == 1 else m.ww)(m.code(name), value, seg=CODE_SEG)

def dwrite(m, name, value, size=1):
    (m.wb if size == 1 else m.ww)(m.data(name), value)

def base(m, dos=0, ticks=0x0010, div=0, motor=0, bios_ticks=0, t27f0=0):
    """Interrupt frame, chain stub, tick and DOS-flag state, BIOS data area (0040h)."""
    m.write(STACK_SEG, 0x0200, struct.pack('<HH', CODE_SEG, 0x0202))   # CS, FLAGS of the frame
    m.wb(0x0000, 0xCF, seg=STUB_SEG)                                      # IRET
    m.write(CODE_SEG, m.code('Int08SwapSlot'), struct.pack('<HH', 0x0000, STUB_SEG))   # dword slot
    cwrite(m, 'TimerTickCount', ticks, size=2)
    cwrite(m, 'cvar_1D6B', div)
    cwrite(m, 'cvar_1D69', 0, size=2)
    dwrite(m, 'DosCallActive', dos)                  # the in-DOS flag (DATA:2877h)
    m.wb(0x40, motor, seg=0x0040)                    # motor timeout counter (40h:40h)
    m.wb(0x3F, 0x0F, seg=0x0040)
    m.ww(0x6C, bios_ticks & 0xFFFF, seg=0x0040)
    m.ww(0x6E, bios_ticks >> 16, seg=0x0040)
    m.ww(0x70, 0, seg=0x0040)
    dwrite(m, 'word_27F0', t27f0, size=2)
    dwrite(m, 'word_27F2', 0, size=2)
    cwrite(m, 'SbDetected', 0)
    cwrite(m, 'AdLibEnabled', 0)
    cwrite(m, 'SoundState', 0)
    cwrite(m, 'OptionBits', 0x0000, size=2)

def sp_record(m, divisor=0x0200, start=0x0300, count=1, step=0x0010, reload=3):
    m.ww(REC + 0, divisor)
    m.ww(REC + 2, start)
    m.ww(REC + 4, count)
    m.ww(REC + 6, step)
    m.ww(REC + 8, reload)
    dwrite(m, 'SpeakerSequencePtr', REC, size=2)

CASES = []
def case(title, routine, cfunc, setup, out=(), ranges=(), budget=5_000_000, files=False):
    CASES.append(Case(title, routine, cfunc, setup=setup, out_regs=out,
                      ranges=[(0x0040, 0, 0x100)] + list(ranges) + HEAP, budget=budget, files=files))

# -- tick path: loc_07AD (TimerTickCount, divider, BIOS body every 4th tick) -----------------
def tick(title, **kw):
    case(title, 'loc_07AD', 'c_loc_07AD', lambda m: base(m, **kw), ranges=[(0x0000, 0x20, 4)])
tick('tick: count, divider 0 -> 1, EOI only', ticks=0x0010, div=0)
tick('tick: divider 2 -> 3, EOI only', ticks=0x00FF, div=2)
tick('tick: divider 3 -> 0, BIOS body (motor, day counter)', ticks=0x0100, div=3, motor=5, bios_ticks=0x0000)
tick('tick: divider wraps 255 -> 0 through the BIOS body', ticks=0xFFFF, div=0xFF)

# -- BIOS tick body: loc_17C0 ----------------------------------------------------------------
def bios(title, **kw):
    case(title, 'loc_17C0', 'c_loc_17C0', lambda m: base(m, **kw))
bios('BIOS body: motor 5 -> 4, day counter +1', motor=5, bios_ticks=0x00010000)
bios('BIOS body: motor 1 -> 0 turns the motors off', motor=1, bios_ticks=0x00000010)
bios('BIOS body: 40:6Ch carry into 40:6Eh', motor=7, bios_ticks=0x0000FFFF)
bios('BIOS body: midnight wrap at 1800B0h', motor=7, bios_ticks=0x001800AF)
bios('BIOS body: word_27F0 carry into word_27F2', motor=7, t27f0=0xFFFF)
bios('BIOS body: DOS busy chains to the previous vector', dos=1, motor=7)

# -- speaker sequence: SpeakerTimerIsr ----------------------------------------------------------
def speaker(title, rec=True, sb=0, adlib=0, record=None, **kw):
    def setup(m):
        base(m, **kw)
        cwrite(m, 'SbDetected', sb)
        cwrite(m, 'AdLibEnabled', adlib)
        if rec: sp_record(m, **(record or {}))
        else: dwrite(m, 'SpeakerSequencePtr', 0xFFFF, size=2)
    case(title, 'SpeakerTimerIsr', 'c_SpeakerTimerIsr', setup, ranges=[(0x0000, 0x20, 4)])
speaker('speaker: record counts down (no end)', record=dict(count=5))
speaker('speaker: last count reloads the record and writes 000Ah', record=dict(count=1, reload=3))
speaker('speaker: idle pointer FFFFh', rec=False)
speaker('speaker: silent while a sound device is active', sb=1, record=dict(count=1))
speaker('speaker: silent while AdLib is active', adlib=1, record=dict(count=1))
speaker('speaker: tick path with a full wrap of the divisor', record=dict(divisor=0x0004, start=0x0004, count=1, step=0x0008, reload=2), ticks=0xFFFF)

# -- sample output: SampleTimerIsr --------------------------------------------------------------
def sample_setup(**kw):
    def setup(m):
        base(m, **kw.get('base', {}))
        cwrite(m, 'cvar_1D69', kw.get('n', 5), size=2)
        cwrite(m, 'SoundState', kw.get('sound', 1))
        dwrite(m, 'SfxBytesLeft', kw.get('left', 0x10), size=2)
        dwrite(m, 'SfxPlayPos', kw.get('pos', 0x0100), size=2)
        dwrite(m, 'SfxBankSegment', BANK_SEG, size=2)
        for k in range(0x40):
            m.wb(0x0100 + k, (k * 7) & 0xFF, seg=BANK_SEG)
    return setup

def sample(title, **kw):
    case(title, 'SampleTimerIsr', 'c_SampleTimerIsr', sample_setup(**kw), ranges=[(BANK_SEG, 0, 0x200)])
sample('sample: one byte out to 389h (SFX playing)', n=5)
sample('sample: SoundState bit 0 clear, no output', n=5, sound=0)
sample('sample: no SFX bytes left, no output', n=5, left=0)
sample('sample: every 128th call takes the tick path', n=0x7F)
sample('sample: DOS busy: PIT 4000h, tick path every call', n=5, base=dict(dos=1))
sample('sample: sample byte at the top of the bank', n=5, pos=0x013F, left=0x20)

# -- SB block: SbIrqHandler ---------------------------------------------------------------------
def sb_setup(**kw):
    def setup(m):
        base(m, **kw.get('base', {}))
        cwrite(m, 'SoundState', kw.get('sound', 3))
        cwrite(m, 'OptionBits', kw.get('option', 0x0040), size=2)
        cwrite(m, 'SbAckPort', 0x022E, size=2)
        cwrite(m, 'SbWritePort', 0x022C, size=2)
        dwrite(m, 'PlayBuffer', 0x10C9, size=2)     # buffer A plays, buffer B is mixed
        dwrite(m, 'MixBuffer', 0x1171, size=2)      # (swapped on every block)
        dwrite(m, 'PlayDmaRecord', 0x10BB, size=2)  # records: page byte, offset word
        dwrite(m, 'MixDmaRecord', 0x10BE, size=2)
        m.wb(0x10BB, 0x01); m.ww(0x10BC, 0x2000); m.wb(0x10BE, 0x02); m.ww(0x10BF, 0x4000)
        dwrite(m, 'SfxBytesLeft', kw.get('left', 0), size=2)
        dwrite(m, 'SfxPlayPos', 0x0200, size=2)
        dwrite(m, 'SfxBankSegment', BANK_SEG, size=2)
        for k in range(0x180):
            m.wb(0x0200 + k, (k * 13) & 0xFF, seg=BANK_SEG)
        dwrite(m, 'SbIrqCount', kw.get('count', 0))
    return setup

def sb(title, **kw):
    case(title, 'SbIrqHandler', 'c_SbIrqHandler', sb_setup(**kw),
         ranges=[(BANK_SEG, 0, 0x400)],
         budget=20_000_000)
sb('SB block: 168 bytes of silence, no tracker (OptionBits 40h)', option=0x0040)
sb('SB block: SFX copy of 168 bytes, then silence', option=0x0040, left=0x00C0)
sb('SB block: SFX remainder shorter than a block', option=0x0040, left=0x0051)
sb('SB block: early exit when SoundState is not 3', option=0x0040, sound=1)
sb('SB block: counter increments on every IRQ', option=0x0040, sound=0, count=7)
sb('SB block: tracker and channel mix enabled (OptionBits 0)', option=0x0000, left=0x0000)


# -- ES register model: the same cases entered with ES not equal to DATA --------------------------
# (a routine that must leave ES alone, or set it, is only visible when the entry ES differs from
# what it would load; tools/difftest.py compares pre2_es with the oracle's ES after every case)
def _es_variants(cases, picks, es=0x7C20):
    """picks: (routine, title substring or '', how many) -> clones of the first matching cases."""
    import copy
    out = []
    for routine, sub, limit in picks:
        n = 0
        for c in cases:
            if c.routine == routine and sub in c.title and n < limit:
                d = copy.copy(c)
                d.title = c.title + ' [entry ES 7C20h]'
                d.regs = dict(c.regs, ES=es)
                out.append(d)
                n += 1
    return out
CASES += _es_variants(CASES, [
    ('loc_07AD', '', 3), ('loc_17C0', '', 2), ('SpeakerTimerIsr', '', 2), ('SampleTimerIsr', '', 3),
    ('SbIrqHandler', '', 6)])
