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
