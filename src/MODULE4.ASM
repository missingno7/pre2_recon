; Prehistorik 2 (PRE2.EXE), link module 4 of 4
; module interface (tools/symtab.py)
public RunCallbackMapScreen
public RunGameOverScreen
public ShowWorldMapScrollIn
DATA segment word public 'DATA'
extrn byte_27E8:byte
extrn byte_2879:byte
extrn byte_6BE5:byte
extrn CameraCol:word
extrn CameraRow:word
extrn CodeEntryState:byte
extrn CodeInputCount:word
extrn CurrentLevel:byte
extrn DrawPage:word
extrn FineScroll:byte
extrn FrameCallback:word
extrn LastScanCode:byte
extrn MapScrollX:word
extrn MapScrollY:word
extrn NextFreeSegment:word
extrn PrevMapScrollX:word
extrn PrevMapScrollY:word
extrn PrevScreenStartOffset:word
extrn ScreenExitRequested:byte
extrn ScreenFirstFrame:byte
extrn ScreenStartOffset:word
extrn ShakeRows:word
extrn SnowFrameTick:byte
extrn TextAdvance:byte
extrn WavePhase:byte
extrn word_003D:word
extrn word_1F18:word
extrn word_27F0:word
extrn word_5088:word
extrn word_62DA:word
extrn word_667A:word
extrn word_7522:word
DATA ends
; end module interface
CODE segment word public 'CODE'
assume cs:CODE, ds:DATA, es:nothing, ss:nothing
; module interface (tools/symtab.py)
extrn ActivateResourceSlot:near
extrn CpuClass:byte
extrn DisableVideoOutput:near
extrn DrawSpriteObjects:near
extrn EnableVideoOutput:near
extrn NextWindRandom:near
extrn PageInResource:near
extrn PollInputState:near
extrn PresentFrame:near
extrn SetCpuWriteMode:near
extrn SetLatchCopyMode:near
extrn sub_9286:near
extrn sub_932F:near
; end module interface

; World map (stage select) scroll-in. Called at MODULE1 01A8 when CurrentLevel < 0Ah, unless
; CurrentLevel is 8 or 9 and byte_B198 is 0 (that case takes the castle load path). Stamps the
; stage marker sprite into
; the map buffer at the stage's node, then copies the map into VRAM one byte column per 8 px
; while MapScrollX runs 8 -> 640 (+1 per frame), holds 70 more frames, then fades (sub_9286).
; In: CurrentLevel (0..9). word_2875 = map buffer segment: 4 planes of 80 x 200 bytes, plane
;     stride 3E80h. word_62DA = marker sprite offset, word_667A = its segment; word_7522 =
;     marker size, low byte width in pixels, high byte rows.
;     Marker layout: mask plane (w*h bytes) then 4 colour planes (w*h each). Mask 1 = opaque.
; Out: nothing. CurrentLevel unchanged. LastScanCode = 0 and byte_6BE5 = 0 on return.
; Clobbers: all registers, DS, ES.
; Notes: Video is blanked (sub_3042) for setup and enabled (sub_3034) before the frame loop.
;   The video mode is not set here (see R1). DAC entries 0..15 loaded from ds:B0E8h.
;   Node position: column = x/8, row = (y - 20h) using only the low byte (mul ah, ah = 50h).
;   Frame loop: CRTC start = MapScrollY*28h + MapScrollX/8, masked to 13 bits (1FFFh);
;   pel pan (attribute 13h) = MapScrollX & 7.
;   Each frame: sti, sub_0DC1 (input sample), sub_990D (next retrace), cli.
;   Exit: byte_27E8 nonzero (action, from sub_0DC1) ends the scroll or the hold early.
;   The scroll copies a byte column (200 rows; source stride 50h, destination stride 28h) and
;   masks both offsets with 1FFFh. Exact ring semantics: see the report, not verified.
ShowWorldMapScrollIn:
    mov ax, seg DATA
    mov ds, ax
    mov es, ax
    call DisableVideoOutput
    mov ax, 2
    call ActivateResourceSlot
    mov cx, 5
    call PageInResource
    mov ax, seg DATA
    mov ds, ax
    mov es, word ptr NextFreeSegment
    mov bl, byte ptr CurrentLevel
    xor bh, bh
    shl bx, 1
    shl bx, 1
    mov dx, word ptr [bx-4EB8h]
    shr dx, 1
    shr dx, 1
    shr dx, 1
    mov ax, word ptr [bx-4EB6h]
    sub ax, 20h
    mov ah, 50h
    mul ah
    add ax, dx
    mov di, ax
    mov dx, word ptr word_7522
    shr dl, 1
    shr dl, 1
    shr dl, 1
    mov si, word ptr word_62DA
    mov ds, word ptr word_667A
    mov cl, dl
    xor ch, ch
    mov bp, cx
    neg bp
    add bp, 50h
    mov al, cl
    mul dh
    mov bx, ax
loc_9585:
    mov cl, dl
loc_9587:
    mov al, byte ptr [si]
    not al
    and byte ptr es:[di], al
    and byte ptr es:[di+3E80h], al
    and byte ptr es:[di+7D00h], al
    and byte ptr es:[di-4480h], al
    mov al, byte ptr [bx+si]
    or byte ptr es:[di], al
    add si, bx
    mov al, byte ptr [bx+si]
    or byte ptr es:[di+3E80h], al
    add si, bx
    mov al, byte ptr [bx+si]
    or byte ptr es:[di+7D00h], al
    add si, bx
    mov al, byte ptr [bx+si]
    or byte ptr es:[di-4480h], al
    sub si, bx
    sub si, bx
    sub si, bx
    inc si
    inc di
    loop loc_9587
    add di, bp
    dec dh
    jne loc_9585
    mov ax, seg DATA
    mov ds, ax
    mov ax, 0A000h
    mov es, ax
    xor ax, ax
    mov word ptr MapScrollY, ax
    mov word ptr PrevMapScrollX, ax
    mov word ptr PrevMapScrollY, ax
    mov word ptr MapScrollX, 8
    push es
    push ds
    pop es
    mov dx, 0B0E8h
    xor bx, bx
    mov ax, 1012h
    mov cx, 10h
    int 10h
    pop es
    call EnableVideoOutput
    call SetCpuWriteMode
    mov dx, 3D4h
    mov ax, 2411h
    out dx, ax
    mov ax, 0E217h
    out dx, ax
    mov ax, 2601h
    out dx, ax
    mov dl, 0C4h
    mov al, 2
    out dx, al
loc_9613:
    mov ax, 28h
    mul word ptr MapScrollY
    mov bx, word ptr MapScrollX
    shr bx, 1
    shr bx, 1
    shr bx, 1
    add bx, ax
    and bh, 1Fh
    mov dx, 3D4h
    mov al, 0Ch
    mov ah, bh
    out dx, ax
    inc ax
    mov ah, bl
    out dx, ax
    xchg word ptr ScreenStartOffset, bx
    mov word ptr PrevScreenStartOffset, bx
    call SetCpuWriteMode
    sti
    call PollInputState
    call WaitNextVerticalRetrace
    cli
    mov dx, 3C4h
    mov al, 2
    out dx, al
    mov dx, 3C0h
    mov al, 33h
    out dx, al
    mov al, byte ptr MapScrollX
    and al, 7
    out dx, al
    mov dx, word ptr MapScrollX
    inc word ptr MapScrollX
    test dl, 7
    jne loc_969C
    mov bp, 1FFFh
    mov ds, word ptr NextFreeSegment
    shr dx, 1
    shr dx, 1
    shr dx, 1
    dec dx
    mov si, dx
    mov di, si
    mov dx, 3C5h
    mov al, 1
loc_967E:
    out dx, al
    mov cx, 0C8h
loc_9682:
    and di, bp
    movsb
    add si, 4Fh
    add di, 27h
    loop loc_9682
    sub di, 1F40h
    shl al, 1
    cmp al, 10h
    jb loc_967E
    mov ax, seg DATA
    mov ds, ax
loc_969C:
    cmp byte ptr ds:byte_27E8, 0
    jne loc_96C1
    cmp word ptr ds:MapScrollX, 280h
    jae loc_96AE
    jmp near ptr loc_9613
loc_96AE:
    sti
    mov cx, 46h
loc_96B2:
    call WaitNextVerticalRetrace
    call PollInputState
    cmp byte ptr ds:byte_27E8, 0
    jne loc_96C1
    loop loc_96B2
loc_96C1:
    mov ax, seg DATA
    mov ds, ax
    sti
    call sub_9286
    mov byte ptr LastScanCode, 0
    mov byte ptr byte_6BE5, 0
    ret

; Runs the map screen until ScreenExitRequested is nonzero. Used for the MODE choice and the
; ENTER CODE entry. Each frame: wait for retrace start, set the CRTC start and pel pan, call
; FrameCallback with interrupts disabled, then copy VRAM to scroll the view.
; In: DX = near offset (CS) of the frame callback: 991Fh (mode select) or 9985h (code entry).
;     word_2875 = map buffer segment; word_003D = segment of the shift-table source.
; Out: nothing. The callback leaves CurrentLevel / ExpertFlag set; fades (sub_9286) at the end.
; Clobbers: all registers, DS, ES.
; Notes: FrameCallback is stored at word_B1AE; the callback runs with IF = 0 and GC write
;   mode 0 (sub_452B before the call). Afterwards sub_453B sets write mode 1 and map mask 0Fh.
;   This loop does not call sub_0DC1, so byte_27E8 is not refreshed here; the callbacks read
;   the keyboard flags 282Dh/2810h directly.
;   Before the loop: copies the first 8000 bytes of each plane of word_2875 into VRAM (raw copy,
;   meaning not established), then builds 8 pixel-shifted tables (phase 0..7, 1050h bytes each)
;   into word_2875 from offset 0. Each phase reads 0x560 words from word_003D:1FE0h, byte-swaps
;   each, shifts it right by the phase and writes the word plus the bits shifted out (one byte);
;   then 30h zero words follow.
;   Palette: 16 DAC entries from ds:B118h. ScreenFirstFrame skips the pel-pan write once.
;   Column copy when bit 3 of MapScrollX changes; row copy by MapScrollY - PrevMapScrollY.
;   Quirk: the loop relies on BP = 1FFFh after the callback; AdvanceMapFrame sets it.
RunCallbackMapScreen:
    mov ax, seg DATA
    mov ds, ax
    mov es, ax
    mov word ptr FrameCallback, dx
    mov ax, 1
    call ActivateResourceSlot
    call DisableVideoOutput
    mov cx, 6
    call PageInResource
    mov ax, 0A000h
    mov es, ax
    mov ds, word ptr NextFreeSegment
    xor si, si
    xor di, di
    call SetCpuWriteMode
    mov dx, 3D4h
    mov ax, 2411h
    out dx, ax
    mov ax, 0E217h
    out dx, ax
    mov ax, 2601h
    out dx, ax
    mov dx, 3C4h
    mov al, 2
    out dx, al
    inc dx
    mov al, 1
loc_9717:
    out dx, al
    mov cx, 0FA0h
    rep movsw
    xor di, di
    shl al, 1
    cmp al, 4
    jb loc_9717
    mov ax, seg DATA
    mov ds, ax
    mov es, word ptr NextFreeSegment
    mov ds, word ptr word_003D
    mov dh, 0
    xor di, di
loc_9736:
    mov si, 1FE0h
    mov cx, 560h
loc_973C:
    lodsw
    xchg al, ah
    xor dl, dl
    mov bl, dh
    or bl, bl
    je loc_974F
loc_9747:
    shr ax, 1
    rcr dl, 1
    dec bl
    jne loc_9747
loc_974F:
    xchg al, ah
    stosw
    mov al, dl
    stosb
    loop loc_973C
    mov cx, 30h
    xor ax, ax
    rep stosw
    inc dh
    cmp dh, 8
    jb loc_9736
    mov ax, seg DATA
    mov ds, ax
    mov ax, 0A000h
    mov es, ax
    xor ax, ax
    mov word ptr MapScrollX, ax
    mov word ptr MapScrollY, ax
    mov word ptr PrevMapScrollX, ax
    mov word ptr PrevMapScrollY, ax
    mov word ptr CodeInputCount, ax
    mov byte ptr ScreenExitRequested, al
    mov byte ptr CodeEntryState, 0
    mov byte ptr TextAdvance, 4
    mov byte ptr WavePhase, 2
    mov byte ptr ScreenFirstFrame, 1
    push es
    push ds
    pop es
    mov dx, 0B118h
    xor bx, bx
    mov ax, 1012h
    mov cx, 10h
    int 10h
    pop es
loc_97A8:
    mov ax, 28h
    mul word ptr MapScrollY
    mov bx, word ptr MapScrollX
    shr bx, 1
    shr bx, 1
    shr bx, 1
    add bx, ax
    and bh, 1Fh
    mov dx, 3D4h
    mov al, 0Ch
    mov ah, bh
    out dx, ax
    inc ax
    mov ah, bl
    out dx, ax
    xchg word ptr ScreenStartOffset, bx
    mov word ptr PrevScreenStartOffset, bx
    call SetCpuWriteMode
    sti
    call WaitVerticalRetraceStart
    cli
    mov dx, 3C4h
    mov al, 2
    out dx, al
    cmp byte ptr ds:ScreenFirstFrame, 0
    mov byte ptr ScreenFirstFrame, 0
    jne loc_97F8
    mov dx, 3C0h
    mov al, 33h
    out dx, al
    mov al, byte ptr MapScrollX
    and al, 7
    out dx, al
loc_97F8:
    call word ptr FrameCallback
    mov ax, seg DATA
    mov ds, ax
    call SetLatchCopyMode
    mov al, byte ptr PrevMapScrollX
    mov ah, byte ptr MapScrollX
    and ax, 808h
    cmp al, ah
    je loc_9836
    mov si, word ptr ScreenStartOffset
    add si, 27h
    lea di, [si-28h]
    mov cx, 0C8h
    mov ax, 0A000h
    mov ds, ax
loc_9824:
    and si, bp
    and di, bp
    movsb
    add si, 27h
    add di, 27h
    loop loc_9824
    mov ax, seg DATA
    mov ds, ax
loc_9836:
    mov dx, word ptr MapScrollY
    sub dx, word ptr PrevMapScrollY
    je loc_9877
    js loc_984D
    mov si, word ptr ScreenStartOffset
    dec si
    lea di, [si+word_1F18]
    jmp short loc_9860
loc_984D:
    neg dx
    mov si, word ptr ScreenStartOffset
    dec si
    mov al, 28h
    mul dl
    sub si, ax
    mov di, si
    add si, 1F18h
loc_9860:
    mov ax, 0A000h
    mov ds, ax
    mov al, 29h
    mul dl
    mov cx, ax
loc_986B:
    and si, bp
    and di, bp
    movsb
    loop loc_986B
    mov ax, seg DATA
    mov ds, ax
loc_9877:
    cmp byte ptr ds:ScreenExitRequested, 0
    jne loc_9881
    jmp near ptr loc_97A8
loc_9881:
    sti
    call sub_9286
    ret

; Draws a string of glyphs at the current text position, erasing the previous cell first.
; The string at DS:BX ends at the first byte below '0' other than space (NUL, '-', ...).
; In: BX = offset (DS) of the string. ES = A000h (VRAM). DS = DATA. TextCursorOffset = x cursor
;     (byte offset in a row), TextAdvance = cell advance in bytes, TextFontBank = glyph bank,
;     word_2875 = font segment, ScreenStartOffset / PrevScreenStartOffset = current / previous.
; Out: BX = offset after the terminator. TextCursorOffset advanced by TextAdvance per glyph.
; Clobbers: AX, CX, SI, DI, DX, flags (BP must be 1FFFh; the callers set it).
; Notes: Glyph index = char - '0' (digits 0..9); other chars: letters index = char - 37h; space
;   uses glyph 2Bh. Glyph record = 60h bytes. Erase pass: 3 bytes x 12 rows at the previous
;   origin, planes 2 and 3 only (map mask 0Ch). Draw pass: rows at the current origin, planes
;   2 then 3 (masks 4, 8), with the second plane read 0Ch bytes after the first (layout to be
;   verified). Row pitch 28h. Text origin = start + cursor + 50h.
DrawFontString:
    mov al, byte ptr [bx]
    inc bx
    mov dl, byte ptr ds:[0B1ABh]
    xor dh, dh
    cmp al, 20h
    jne loc_9897
    mov al, 2Bh
    jmp short loc_98A1
loc_9897:
    sub al, 30h
    jb loc_98FF
    cmp al, 9
    jbe loc_98A1
    sub al, 7
loc_98A1:
    mov ah, 60h
    mul ah
    add ax, word ptr ds:[0B1ACh]
    add ax, 6
    mov si, ax
    add word ptr ds:[0B1A6h], dx
    mov ax, word ptr ds:[0B1A6h]
    add ax, 50h
    mov di, ax
    add ax, word ptr ds:[0B1A1h]
    push ax
    add di, word ptr ds:[0B1A3h]
    mov ds, word ptr ds:[2875h]
    mov dx, 3C5h
    mov al, 0Ch
    out dx, al
    mov cx, 0Ch
    xor ax, ax
loc_98D2:
    and di, bp
    stosw
    stosb
    add di, 25h
    loop loc_98D2
    pop di
    mov al, 4
loc_98DE:
    out dx, al
    mov cx, 0Ch
loc_98E2:
    and di, bp
    movsw
    movsb
    add di, 25h
    loop loc_98E2
    add si, 0Ch
    sub di, 1E0h
    shl al, 1
    cmp al, 10h
    jb loc_98DE
    mov ax, seg DATA
    mov ds, ax
    jmp short DrawFontString
loc_98FF:
    ret

; Busy-waits until the vertical retrace bit (port 3DAh bit 3) is set, i.e. a retrace has started.
; In: none. Out: none. Preserves AX and DX.
WaitVerticalRetraceStart:
    push ax
    push dx
    mov dx, 3DAh
loc_9905:
    in al, dx
    test al, 8
    je loc_9905
    pop dx
    pop ax
    ret

; Waits for the current retrace (if any) to end, then for the next one to start.
; In: none. Out: none. Preserves AX and DX. Port 3DAh bit 3 (vertical retrace).
; Notes: if the second read already shows a retrace, it returns at once (race window).
WaitNextVerticalRetrace:
    push ax
    push dx
    mov dx, 3DAh
loc_9912:
    in al, dx
    test al, 8
    jne loc_9912
    in al, dx
    test al, 8
    je loc_9905
    pop dx
    pop ax
    ret

; FrameCallback for the MODE (BEGINNER / EXPERT) choice. Advances the map, draws "MODE" and the
; current choice, then reads input.
; In: ExpertFlag, LastScanCode (set by the keyboard ISR), byte 282Dh and byte 2810h (keys).
; Out: ExpertFlag toggled by a make code of 48h (up), 4Bh (left), 4Dh (right) or 50h (down).
;      ScreenExitRequested incremented if 282Dh or 2810h is nonzero (fire / confirm).
; Notes: Break codes (bit 7 of LastScanCode) are ignored. Other make codes are consumed
;   (LastScanCode = 0) before the arrow test; the exit path leaves the scan code in place.
;   Draws with TextAdvance = 4 at 0C38h ("MODE") and TextAdvance = 3 at 1185h (choice).
UpdateModeSelectFrame:
    call AdvanceMapFrame
    mov byte ptr ds:[0B1ABh], 4
    mov bx, 0B180h
    mov word ptr ds:[0B1A6h], 0C38h
    call DrawFontString
    mov word ptr ds:[0B1A6h], 1185h
    mov byte ptr ds:[0B1ABh], 3
    mov bx, 0B185h
    cmp byte ptr ds:[0B197h], 0
    je loc_994B
    mov bx, 0B18Eh
loc_994B:
    call DrawFontString
    mov bl, byte ptr ds:[2874h]
    test bl, 80h
    jne loc_9984
    mov al, byte ptr ds:[282Dh]
    or al, byte ptr ds:[2810h]
    jne loc_9980
    mov byte ptr ds:[2874h], 0
    cmp bl, 48h
    je loc_9979
    cmp bl, 4Bh
    je loc_9979
    cmp bl, 4Dh
    je loc_9979
    cmp bl, 50h
    jne loc_9984
loc_9979:
    xor byte ptr ds:[0B197h], 1
    jmp short loc_9984
loc_9980:
    inc byte ptr ds:[0B1B2h]
loc_9984:
    ret

; FrameCallback for the ENTER CODE screen: 4 hex digits. Advances the map and draws the prompt
; and the 4 input boxes (ds:B170h). Input is read only in CodeEntryState 0.
; In: CurrentLevel = 0FFh (set by the caller; stays 0FFh if the entry is cancelled).
; Out: on acceptance CurrentLevel = level mod 10 and ExpertFlag = level >= 10.
;      ScreenExitRequested is incremented at once on cancel, one frame after acceptance.
; States: 0 typing; 1 wrong code, CodeErrorTimer counts to 8Ch then the boxes reset;
;   2 accepted, next frame exits and resets the boxes.
; Input: 282Dh or 2810h nonzero cancels typing. Break codes are ignored. A make code is looked up
;   in the scan table ds:B068h (1..0 for keys 02h..0Bh, 'A'..'F' for some letter keys, '-' =
;   ignored). Each digit adds its low nibble to CodeValue (16-bit, four nibbles).
; Check: after 4 digits, accepted if (a) CodeHistory0..2 and CodeValue-1 satisfy the hash
;   (36C8h, 8BD1h, 8E71h) with CodeValue-1 <= 13h, giving level CodeValue-1; or (b) CodeValue
;   equals sub_932F(i) for some i in 0..12h, giving level i. Otherwise CodeEntryState = 1 and the
;   history shifts (CodeHistory0 <- CodeHistory1 <- CodeHistory2 <- CodeValue).
; Notes: sub_932F hashes machine ROM bytes (F000:FFF0.., C000 option ROMs) into word_A333, so the
;   table codes depend on the machine's ROM (translation-critical). Scan table quirks: keys 10h
;   and 1Eh both give 'A'; 2Ch gives 'C'; 2Eh gives 'B'.
UpdateCodeEntryFrame:
    call AdvanceMapFrame
    mov byte ptr ds:[0B1ABh], 3
    mov bx, 0B175h
    mov word ptr ds:[0B1A6h], 0AF2h
    call DrawFontString
    mov byte ptr ds:[0B1ABh], 4
    mov bx, 0B170h
    mov word ptr ds:[0B1A6h], 12C9h
    call DrawFontString
    cmp byte ptr ds:[0B1AAh], 1
    jne loc_99B4
    jmp near ptr loc_9AB4
loc_99B4:
    cmp byte ptr ds:[0B1AAh], 2
    jne loc_99BE
    jmp near ptr loc_9AC4
loc_99BE:
    mov bl, byte ptr ds:[2874h]
    test bl, 80h
    je loc_99CA
    jmp near ptr loc_9ADF
loc_99CA:
    mov al, byte ptr ds:[282Dh]
    or al, byte ptr ds:[2810h]
    je loc_99D6
    jmp near ptr loc_9AC4
loc_99D6:
    mov byte ptr ds:[2874h], 0
    xor bh, bh
    mov al, byte ptr [bx-4F98h]
    cmp al, 2Dh
    jne loc_99E8
    jmp near ptr loc_9ADF
loc_99E8:
    mov ah, al
    sub ah, 30h
    cmp ah, 9
    jbe loc_99F5
    sub ah, 7
loc_99F5:
    mov dx, word ptr ds:[0B1B9h]
    mov cl, 4
    shl ah, cl
    shl ah, 1
    rcl dx, 1
    shl ah, 1
    rcl dx, 1
    shl ah, 1
    rcl dx, 1
    shl ah, 1
    rcl dx, 1
    mov word ptr ds:[0B1B9h], dx
    mov bx, word ptr ds:[0B1A8h]
    mov byte ptr [bx-4E90h], al
    inc word ptr ds:[0B1A8h]
    inc bl
    cmp bl, 4
    jae loc_9A27
    jmp near ptr loc_9ADF
loc_9A27:
    push bp
    mov word ptr ds:[0B1B0h], 0
    mov di, word ptr ds:[0B1B3h]
    mov si, word ptr ds:[0B1B5h]
    mov bp, word ptr ds:[0B1B7h]
    mov ax, di
    rol ax, 1
    rol ax, 1
    mul ah
    xor ax, si
    mul bp
    mov bx, ax
    add bx, si
    ror bx, 1
    rol bl, 1
    xor bl, bh
    xor bx, bp
    pop bp
    cmp ax, 36C8h
    jne loc_9A6E
    cmp bx, 8BD1h
    jne loc_9A6E
    cmp dx, 8E71h
    jne loc_9A6E
    mov dx, word ptr ds:[0B1B9h]
    dec dx
    cmp dx, 13h
    jbe loc_9A9B
loc_9A6E:
    xor dx, dx
loc_9A70:
    mov ax, dx
    call sub_932F
    cmp word ptr ds:[0B1B9h], ax
    je loc_9A9B
    inc dx
    cmp dx, 12h
    jbe loc_9A70
    mov si, 0B1B5h
    mov ax, word ptr [si]
    mov word ptr [si-2], ax
    mov ax, word ptr [si+2]
    mov word ptr [si], ax
    mov ax, word ptr [si+4]
    mov word ptr [si+2], ax
    mov byte ptr ds:[0B1AAh], 1
    jmp short loc_9ADF
loc_9A9B:
    mov al, 0
    cmp dl, 0Ah
    jb loc_9AA6
    sub dl, 0Ah
    inc ax
loc_9AA6:
    mov byte ptr ds:[2D8Ah], dl
    mov byte ptr ds:[0B197h], al
    mov byte ptr ds:[0B1AAh], 2
    jmp short loc_9ADF
loc_9AB4:
    inc word ptr ds:[0B1B0h]
    mov bx, word ptr ds:[0B1B0h]
    sub bx, 8Ch
    jb loc_9ADF
    jmp short loc_9AC8
loc_9AC4:
    inc byte ptr ds:[0B1B2h]
loc_9AC8:
    mov bx, 0B170h
    mov word ptr [bx], 5B5Bh
    mov word ptr [bx+2], 5B5Bh
    mov byte ptr ds:[0B1AAh], 0
    mov word ptr ds:[0B1A8h], 0
loc_9ADF:
    ret

; Per-frame update used by both callbacks. Scrolls the map view and advances the wave phase.
; MapScrollX -= 4 (old value kept in PrevMapScrollX); WavePhase += 1;
; if cs:cvar_0005 is nonzero, MapScrollY += (byte ds:[6F90h + WavePhase]) sar 4 (old value kept
; in PrevMapScrollY); TextFontBank = (old MapScrollX & 7) * 1080h.
; In: DS = DATA. Out: BP = 1FFFh (the callers rely on it), AX, BX, CL, SI, flags.
; Notes: The wave term is 0..1 row per frame with the table values seen (0..23).
AdvanceMapFrame:
    mov bp, 1FFFh
    mov ax, word ptr ds:[0B19Dh]
    sub ax, 4
    xchg word ptr ds:[0B19Dh], ax
    mov word ptr ds:[0B199h], ax
    add byte ptr ds:[0B1A5h], 1
    cmp byte ptr cs:CpuClass, 0
    je loc_9B14
    mov al, byte ptr ds:[0B1A5h]
    mov bx, 6F90h
    xlat
    cbw
    mov cl, 4
    sar ax, cl
    add ax, word ptr ds:[0B19Fh]
    xchg word ptr ds:[0B19Fh], ax
    mov word ptr ds:[0B19Bh], ax
loc_9B14:
    mov al, byte ptr ds:[0B199h]
    and ax, 7
    mov si, 1080h
    mul si
    mov word ptr ds:[0B1ACh], ax
    ret

; GAME OVER screen. The backdrop scrolls sideways while the letters of "GAMEOVER" fall in and
; three objects move on sine tables. Ends after 630 timer ticks (word_27F0) or on action
; (byte_27E8 nonzero, sampled by sub_0DC1). Returns at once if byte_2879 is nonzero (gameplay).
; In: byte_2879 = 0. word_2875 = backdrop segment, 4 planes of 8000 bytes; each plane is copied
;     to VRAM offset 3F40h (off-screen, beyond the 8K display ring).
; Out: nothing. Fades via sub_9286. word_27F0 holds the timeout (incremented by the timer ISR).
; Notes: Entered from sub_4C69 (loc_5063 path, byte_6BE5 == 1), after sub_509D, sub_25C7 and
;   sub_02CC(AX = 11h); the picture load by sub_02CC is inferred. Afterwards the caller sets
;   CurrentLevel = 0, the stage counter word_4F20 = 0Dh and calls sub_51DF.
;   Palette: 16 DAC entries from ds:AFE8h. Calls sub_0492 with CX = 0Bh.
;   Object records (stride 12h): 116 records from 4F0Ah get [+4] = 0FFFFh (inactive);
;   8 letters at 4F1Ch (frame id = byte at ds:B018h + 0B0h), 3 items at 5138h, 514Ah, 515Ch,
;   whose path pointers are B020h, B032h, B054h; archive records at 4FD0h and 4FE2h.
;   Text: 'GAMEOVER' at ds:B018h. The record field meanings are partly inferred; see the report.
RunGameOverScreen:
    mov ax, seg DATA
    mov ds, ax
    mov es, ax
    xor ax, ax
    cmp byte ptr byte_2879, al
    je loc_9B35
    jmp near ptr loc_9C86
loc_9B35:
    mov word ptr CameraCol, ax
    mov word ptr CameraRow, ax
    mov word ptr ShakeRows, ax
    mov byte ptr FineScroll, al
    mov cx, 0Bh
    call PageInResource
    mov ax, 0A000h
    mov es, ax
    mov di, 3F40h
    mov ds, word ptr NextFreeSegment
    xor si, si
    mov dx, 3C4h
    mov al, 2
    out dx, al
    inc dx
    mov al, 0Fh
    out dx, al
    xor ax, ax
    mov cx, 0FA0h
    rep stosw
    mov al, 1
loc_9B68:
    out dx, al
    mov cx, 0FA0h
    rep movsw
    sub di, 1F40h
    shl al, 1
    cmp al, 10h
    jb loc_9B68
    mov ax, seg DATA
    mov ds, ax
    mov es, ax
    mov dx, 0AFE8h
    xor bx, bx
    mov ax, 1012h
    mov cx, 10h
    int 10h
    mov di, 4F0Ah
    mov ax, 0FFFFh
    mov cx, 74h
loc_9B95:
    mov word ptr [di+4], ax
    add di, 12h
    loop loc_9B95
    mov di, 4F1Ch
    mov cx, 4
    mov si, 0B018h
    mov dx, 2Ch
    mov bx, 0E0h
loc_9BAC:
    lodsb
    xor ah, ah
    add ax, 0B0h
    mov word ptr [di+4], ax
    mov word ptr [di], dx
    mov word ptr [di+2], bx
    call NextWindRandom
    and ax, 7
    jne loc_9BC3
    inc ax
loc_9BC3:
    mov word ptr [di+0Eh], ax
    add dx, 18h
    add di, 12h
    loop loc_9BAC
    mov cx, 4
    add dx, 30h
loc_9BD4:
    lodsb
    xor ah, ah
    add ax, 0B0h
    mov word ptr [di+4], ax
    mov word ptr [di], dx
    mov word ptr [di+2], bx
    call NextWindRandom
    and ax, 7
    mov word ptr [di+0Eh], ax
    add dx, 18h
    add di, 12h
    loop loc_9BD4
    add di, 0B4h
    mov ax, 96h
    mov word ptr [di], ax
    mov word ptr [di+2], 135h
    mov word ptr [di+4], 64h
    add di, 12h
    mov word ptr [di], ax
    inc word ptr [di]
    mov word ptr [di+2], 108h
    mov word ptr [di+4], 1C4h
    add di, 12h
    mov word ptr [di], ax
    mov word ptr [di+2], 121h
    mov word ptr [di+4], 68h
    add di, 12h
    mov word ptr [di], ax
    mov word ptr [di+2], 119h
    mov word ptr [di+4], 62h
    mov di, 5138h
    mov byte ptr [di+8], 0
    mov word ptr [di+0Ch], 0B020h
    add di, 12h
    mov byte ptr [di+8], 55h
    mov word ptr [di+0Ch], 0B032h
    add di, 12h
    mov byte ptr [di+8], 0AAh
    mov word ptr [di+0Ch], 0B054h
    add di, 12h
    mov word ptr word_27F0, 0
loc_9C62:
; The frame loop: FineScroll, sprites, the copy, the input sample, the retrace wait; ends
; on the 630-tick timeout or byte_27E8 (see RunGameOverScreen).
    call UpdateGameOverSprites
    call CopyBackdropStrip
    call DrawSpriteObjects
    call PresentFrame
    call PollInputState
    call WaitNextVerticalRetrace
    cmp word ptr ds:word_27F0, 276h
    jae loc_9C83
    cmp byte ptr ds:byte_27E8, 0
    je loc_9C62
loc_9C83:
    call sub_9286
loc_9C86:
    ret

; Copies the 176-row backdrop window (7040 bytes = 176 * 40) inside VRAM from offset
; 3F40h + 28h * FineScroll to offset word_2DD8, all four planes at once.
; In: FineScroll (0..0B9h), word_2DD8 = destination offset. Out: none; all registers kept.
; Notes: Uses write mode 1 (latched copy, sub_453B) and leaves it set; RunGameOverScreen does
;   not restore mode 0. The source offset (3F40h and up) is above the 8K display ring.
CopyBackdropStrip:
    push ax
    push bx
    push cx
    push dx
    push di
    push si
    push ds
    push es
    push bp
    call SetLatchCopyMode
    mov ax, 0A000h
    mov es, ax
    mov di, word ptr ds:DrawPage
    mov dx, 3C4h
    mov ax, 0F02h
    out dx, ax
    mov al, 28h
    mul byte ptr FineScroll
    mov si, ax
    add si, 3F40h
    mov cx, 1B80h
    push es
    pop ds
    rep movsb
    pop bp
    pop es
    pop ds
    pop si
    pop di
    pop dx
    pop cx
    pop bx
    pop ax
    ret

; One GAME OVER animation step: FineScroll += 1 up to 0B9h; the 8 letter records: y (+2) +=
; velocity (+0Eh), the velocity steps +1 per frame through -7..7 and wraps; the 3 item records:
; x and y from the sine-like tables ds:6F90h and ds:7090h (phase +8 += 2 per frame), sprite id
; from the path list at +0Ch (negative word = relative jump); items sorted by y (non-increasing);
; an item with y >= 135h is copied to an archive record and deactivated.
; In: records as set up by RunGameOverScreen. Out: records and FineScroll updated.
; Notes: word_5088 cycles 68h..6Dh every 4 frames (byte_6BD5 & 3 == 0), used by the item frames.
;   Quirk: each item pass writes 0FFFFh to ds:(item - 164h) + 4, i.e. the archive records at
;   4FD4h, 4FE6h and 4FF8h, which the archive copy then reuses.
UpdateGameOverSprites:
    push ax
    push bx
    push cx
    push dx
    push di
    push si
    cmp byte ptr ds:FineScroll, 0B9h
    jae loc_9CD1
    inc byte ptr FineScroll
loc_9CD1:
    test byte ptr SnowFrameTick, 3
    jne loc_9CEA
    mov ax, word ptr word_5088
    and ah, 1Fh
    inc ax
    cmp ax, 6Eh
    jb loc_9CE7
    mov ax, 68h
loc_9CE7:
    mov word ptr word_5088, ax
loc_9CEA:
    mov si, 4F1Ch
    mov cx, 8
loc_9CF0:
    mov ax, word ptr [si+0Eh]
    inc ax
    cmp al, 8
    jl loc_9CFB
    neg ax
    inc ax
loc_9CFB:
    mov word ptr [si+0Eh], ax
    add word ptr [si+2], ax
    add si, 12h
    loop loc_9CF0
    mov si, 5138h
    mov cx, 3
loc_9D0C:
    mov word ptr [si-164h], 0FFFFh
    mov al, byte ptr [si+8]
    mov bx, 6F90h
    xlat
    cbw
    mov dx, 41h
    imul dx
    sar dx, 1
    rcr ax, 1
    sar dx, 1
    rcr ax, 1
    sar dx, 1
    rcr ax, 1
    sar dx, 1
    rcr ax, 1
    sar dx, 1
    rcr ax, 1
    sar dx, 1
    rcr ax, 1
    add ax, 96h
    mov word ptr [si], ax
    mov al, byte ptr [si+8]
    mov bx, 7090h
    xlat
    cbw
    mov dx, 0Ah
    imul dx
    sar dx, 1
    rcr ax, 1
    sar dx, 1
    rcr ax, 1
    sar dx, 1
    rcr ax, 1
    sar dx, 1
    rcr ax, 1
    sar dx, 1
    rcr ax, 1
    sar dx, 1
    rcr ax, 1
    mov word ptr [si+6], ax
    add ax, 135h
    mov word ptr [si+2], ax
    add byte ptr [si+8], 2
    mov bx, word ptr [si+0Ch]
loc_9D71:
    mov ax, word ptr [bx]
    or ax, ax
    jns loc_9D7B
    add bx, ax
    jmp short loc_9D71
loc_9D7B:
    add ax, 138h
    mov dx, word ptr [si+6]
    and dx, 8000h
    or ax, dx
    mov word ptr [si+4], ax
    inc bx
    inc bx
    mov word ptr [si+0Ch], bx
    add si, 12h
    dec cx
    je loc_9D98
    jmp near ptr loc_9D0C
loc_9D98:
    mov cx, 2
    mov si, 5138h
loc_9D9E:
    lea di, [si+12h]
    mov ax, word ptr [si+2]
    cmp word ptr [di+2], ax
    jl loc_9DAE
    call SwapSpriteRecords
    jmp short loc_9D98
loc_9DAE:
    add si, 12h
    loop loc_9D9E
    mov di, 4FD0h
    mov si, 5138h
    mov cx, 2
loc_9DBC:
    cmp word ptr [si+2], 135h
    jl loc_9DCB
    call CopySpriteRecord
    mov word ptr [si+4], 0FFFFh
loc_9DCB:
    add si, 12h
    add di, 12h
    loop loc_9DBC
    pop si
    pop di
    pop dx
    pop cx
    pop bx
    pop ax
    ret

; Exchanges the 18-byte records at DS:SI and DS:DI.
; In: SI, DI. Out: records swapped. Preserves AX, CX, SI, DI.
SwapSpriteRecords:
    push ax
    push cx
    push si
    push di
    mov cx, 12h
loc_9DE1:
    mov al, byte ptr [si]
    xchg byte ptr [di], al
    mov byte ptr [si], al
    inc si
    inc di
    loop loc_9DE1
    pop di
    pop si
    pop cx
    pop ax
    ret

; Copies the 18-byte record at DS:SI to ES:DI (ES = DATA in RunGameOverScreen).
; In: SI, DI. Out: destination record set. Preserves CX, SI, DI.
CopySpriteRecord:
    push cx
    push si
    push di
    mov cx, 12h
    rep movsb
    pop di
    pop si
    pop cx
    ret
CODE ends
DATA segment word public 'DATA'
DATA ends
end
