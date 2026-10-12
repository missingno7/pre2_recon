"""Bootstrap classification evidence for the generator (code-segment offsets, hex).

Every entry carries the static evidence that proves it. This file feeds only
tools/gen.py during bootstrap; once the source is hand-maintained it is deleted.
"""

CODE_SEEDS = [
    0x0008,   # MZ entry 0020:0008
    0x0FC0,   # INT 24h handler installed at entry (mov [0090h],0FC0h / mov [0092h],cs)
    0x075C, 0x1823,                  # far pointers at cs:027A/027E (relocations 7, 8)
    0x1FCE, 0x1FE4, 0x1FFA, 0x2010,  # far pointers at ds:0E4D.. (relocations 53..56)
    0x06F6,   # INT 08h handler: 08CE mov es:[0020h],06F6h / es:[0022h],cs
    0x1C4B,   # INT 06h (invalid opcode) handler for the CPU probe: 1C22 mov es:[0018h],1C4Bh
    0x204D,   # SB IRQ handler: 1EC3 mov ax,204Dh / xchg es:[bx],ax (vector 8+irq)
    0x991F, 0x9985,   # callbacks passed in DX to 96D5, stored to ds:B1AE, called at 97F8
]

# Word tables of near code pointers: (segment 'cs'|'ds', offset, entries, evidence)
CODE_TABLES = [
    ('ds', 0x79EC, 2,  '6268 call [bx+79ECh]; entries 3.. are not code'),
    ('cs', 0x6AA9, 13, '68FC call cs:[bx+6AA9h]; 13 words up to the next table'),
    ('cs', 0x6AC3, 13, '6944 call cs:[bx+6AC3h]; 13 words up to code at 6ADD'),
    ('ds', 0x7D2F, 9,  '5A0B call [bx+7D2Fh]; 9 words then zero'),
    ('ds', 0x7D6F, 8,  '596A add bx,40h then 5A0B call [bx+7D2Fh]: second table at 7D2F+40h; text follows'),
    ('ds', 0x0B64, 16, '238C jmp [bx+0B64h], bx = effect nibble*2 (0..15)'),
    ('ds', 0x7D95, 13, '5CA4/5CC6 call [bx+7D95h], 5C04 [bx+7D9Bh], 5C33 [bx+7DA9h]; 13 words then zero'),
]

# Proven instructions without any caller (no call/jump/vector/table/pointer reaches them;
# searched: all E8/E9/Jcc targets, image words, relocations, vector writes). Decoded as
# code, named unused_XXXX.
DEAD_CODE = [
    0x16F9,   # shr bp,1 / dec dl / je 1700 / ret, after the unconditional jmp at 16F7
    0x19C7,   # protection check (INT 1/INT 3 vectors, cmp es:[di],0CDh); erased by 1BF9
    0x1ABB,   # INT 13h wrapper through the patched far call at 1AFA, disk helpers
    0x1BD5,   # hex number printer (INT 10h AH=0Eh)
    0x1BF9,   # self-wipe: zeroes cs:19C7 up to its caller's return address (callers 1A59, 1A95)
    0x24C9,   # wait loop on ds:27F0/ds:27E8 (no reference)
    0x443E,   # lone ret after jmp 104F at 443B
    0x66D3,   # box overlap test returning CF
    0x6760,   # helper after it
    0x81B0,   # stc / ret
]

# Classified data inside the code segment: (start, end, class). Evidence: readers and
# writers found for every variable block (bootstrap report), init values as shown.
CS_DATA = [
    (0x0000, 0x0008, 'command-line option and CPU state bytes/words (written by entry 0008..007C, CPU probe 1C0C, INT 12h at 00A9)'),
    (0x0272, 0x027A, 'detected SB port words (written 1DA2..1DC1)'),
    (0x1204, 0x1208, 'SQZ loader: file handle (cs:1204) and output segment (cs:1206)'),
    (0x1409, 0x1414, 'EMS/XMS copy state: dword cs:1409, words 140D/1410/1412, mode byte 140F'),
    (0x1D67, 0x1D6F, 'timer/sound counters and flags'),
    (0x1F90, 0x1F91, 'toggle flag (not at 1F91)'),
    (0x26DF, 0x26FA, 'sprite blitter state'),
    (0x2F34, 0x3034, 'bit-reversal table: byte i = reverse8(i); xlat base set at 291C (DS caveat: DS = sprite buffer there)'),
    (0x3050, 0x3054, 'screen offset words'),
    (0x3A25, 0x3A27, 'word variable'),
    (0x45AE, 0x45AF, 'counter byte'),
    (0x6771, 0x6772, 'flag byte'),
    (0x9153, 0x9155, 'bound and index bytes'),
]
