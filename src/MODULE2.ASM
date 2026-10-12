; Prehistorik 2 (PRE2.EXE), link module 2 of 4
; module interface (tools/symtab.py)
public AnimateTiles
public BlitTileToRing
public CheckVgaAdapter
public ClearStaggeredBlocks
public CopyRingWindowToPage
public DisableVideoOutput
public DrawForegroundOverSprites
public DrawSpriteObjects
public EnableVideoOutput
public EnterLevelView
public FlipDisplayPage
public LoadLevel
public LoadSpriteSheet
public NextWindRandom
public PresentFrame
public ReadClockYear
public RedrawOverlayTiles
public RevealLevelColumns
public RunIrisClose
public ScrollViewDown
public ScrollViewLeft
public ScrollViewRight
public ScrollViewUp
public SetCpuWriteMode
public SetLatchCopyMode
public SetLevelVideoMode
public ShowIntroPicture
public ShowLateYearCredits
public StepRandomSeed
public TryShowCredits
public TryShowIntroKeyed
public UpdateWindAndFlakes
public WaitVerticalRetrace
DATA segment word public 'DATA'
extrn AnimRemapPtr:word
extrn AnimTickCounter:byte
extrn AnimTileFlags:byte
extrn AnimVisibleFlag:byte
extrn BackdropCursor:word
extrn BackFileChar:byte
extrn byte_27E8:byte
extrn byte_2805:byte
extrn byte_2806:byte
extrn byte_2811:byte
extrn byte_282C:byte
extrn byte_2CF6:byte
extrn byte_2D96:byte
extrn byte_6688:byte
extrn byte_6689:byte
extrn byte_668A:byte
extrn byte_6788:byte
extrn byte_6789:byte
extrn byte_678A:byte
extrn byte_6888:byte
extrn byte_6889:byte
extrn byte_688A:byte
extrn byte_6BE7:byte
extrn byte_7090:byte
extrn byte_B198:byte
extrn CameraCol:word
extrn CameraLeftLimit:word
extrn CameraRow:word
extrn CheckpointX:word
extrn CheckpointY:word
extrn ClockYear:word
extrn CollectTotalDecor:word
extrn CollectTotalMain:word
extrn ColumnRing:word
extrn CurrentLevel:byte
extrn CurrentObject:word
extrn DisplayPage:word
extrn DrawPage:word
extrn ExpandDstSegment:word
extrn ExpandSrcSegment:word
extrn FineScroll:byte
extrn FlakeTable:word
extrn FrameDataOffsets:word
extrn FrameDataSegments:word
extrn FrameHeights:byte
extrn FrameHotX:byte
extrn FrameHotY:byte
extrn FrameWidths:byte
extrn GridDirtyFlag:byte
extrn HScrollDir:byte
extrn IrisAngleIndex:word
extrn IrisCenterX:word
extrn IrisCenterY:word
extrn IrisRadius:word
extrn IrisSpeed:word
extrn IrisSpeedCounter:word
extrn KeyDownTable:byte
extrn LevelFileChar:byte
extrn LevelFlags:byte
extrn LevelMapRowsTable:byte
extrn LevelPaletteTable:word
extrn LevelScriptTable:word
extrn LevelSegment:word
extrn LevelStartX:word
extrn LevelStartY:word
extrn MapRows:byte
extrn MixedTileCount:byte
extrn NextFreeSegment:word
extrn PictureSegment:word
extrn PlayerX:word
extrn PlayerY:word
extrn PrevCameraCol:word
extrn PrevCameraRow:word
extrn RandomSeed:word
extrn RingTileOffset:word
extrn RowRing:word
extrn ScreenRowBytes:word
extrn ScriptFrameCounter:word
extrn ScriptPointer:word
extrn ScrollAnimCounter:byte
extrn ShakeRows:word
extrn SineTable:byte
extrn SnowFrameTick:byte
extrn SnowWind:word
extrn SpriteBankA:word
extrn SpriteBankB:word
extrn SpriteDataSegment:word
extrn SpriteRelY:word
extrn SpriteSheetSegment:word
extrn TextByteColumn:word
extrn TextRowOffset:word
extrn TileClassTable:byte
extrn VisibleTileFlags:byte
extrn VScrollActive:byte
extrn WindRandA:byte
extrn WindRandB:byte
extrn WindRandC:byte
extrn WindRandWord:word
extrn word_0039:word
extrn word_003B:word
extrn word_27F0:word
extrn word_2A76:word
extrn word_2A7A:word
extrn word_2DC4:word
extrn word_2DCA:word
extrn word_2DCC:word
extrn word_2DD2:word
extrn word_6989:word
extrn word_6A88:word
extrn word_6B14:word
DATA ends
; end module interface
CODE segment word public 'CODE'
assume cs:CODE, ds:DATA, es:nothing, ss:nothing
; module interface (tools/symtab.py)
extrn ComputePasswordCode:near
extrn cvar_0000:byte
extrn HaltWithMessage:near
extrn LoadSqzFile:near
extrn OptionBits:word
extrn OptionVByte:byte
extrn PageInResource:near
extrn PollInputState:near
extrn PrintAsciiz:near
extrn PrintDecimalWord:near
extrn SetLevelPalette:near
extrn SetStartupPalette:near
extrn sub_5634:near
extrn sub_6772:near
extrn WaitForButtonPressRelease:near
extrn WaitSoundTicks:near
; end module interface

; Reads the BIOS real-time clock date (INT 1Ah AH=4) and stores the year as a binary number
; in ClockYear when it is below 2067. The BCD year is decoded digit by digit.
; In: nothing (the clock is read here).
; Out: ClockYear = year when the clock answered and the year is below 2067 (813h); otherwise
;      ClockYear is unchanged (0 at startup). CF = 1 on a clock error; otherwise CF is the
;      result of the final CMP (1 when the year was stored).
; Notes: MUL clobbers DX on every digit, so DH is 0 when the digit bits are shifted in.
;        The digit is added to AL with an 8-bit ADD, so a carry out of AL is lost: years 2048
;        and 2049 decode to 1792 and 1793 (quirk; their values are below every year gate).
ReadClockYear:
    mov ax, seg DATA
    mov ds, ax
    mov ah, 4
    int 1Ah
    jb loc_2408
    push dx
    xor ax, ax
    mov bl, 4
    mov bp, 0Ah
loc_23E7:
    mul bp
    shl cx, 1
    rcl dh, 1
    shl cx, 1
    rcl dh, 1
    shl cx, 1
    rcl dh, 1
    shl cx, 1
    rcl dh, 1
    add al, dh
    dec bl
    jne loc_23E7
    pop dx
    cmp ax, 813h
    jae loc_2408
    mov word ptr ClockYear, ax
loc_2408:
    ret
loc_2409:
    ret

; Shows the "still working in <year>" credits when ClockYear >= 1996 (7CCh), then waits.
; In: ClockYear. Out: nothing; returns at once when ClockYear < 1996.
; Draws with PrintAsciiz at TextRowOffset + TextByteColumn; prints ClockYear with
; PrintDecimalWord; ClearDisplayPage runs before and after the wait (WaitForButtonPressRelease).
ShowLateYearCredits:
    mov ax, seg DATA
    mov ds, ax
    cmp word ptr ds:ClockYear, 7CCh
    jb loc_2409
    call ClearDisplayPage
    call SetStartupPalette
    mov word ptr TextByteColumn, 5
    mov word ptr TextRowOffset, 960h
    mov si, 28CAh
    call PrintAsciiz
    mov word ptr TextByteColumn, 0
    add word ptr TextRowOffset, 1E0h
    mov si, 28D6h
    call PrintAsciiz
    mov dx, word ptr ClockYear
    call PrintDecimalWord
    mov si, 28F2h
    call PrintAsciiz
    mov word ptr TextByteColumn, 1
    mov word ptr TextRowOffset, 1680h
    mov si, 28F6h
    call PrintAsciiz
    mov word ptr TextByteColumn, 3
    add word ptr TextRowOffset, 1E0h
    mov si, 291Bh
    call PrintAsciiz
    call WaitForButtonPressRelease
    call ClearDisplayPage
    ret
loc_2479:
    stc
    ret

; Shows the credits screen when a key-state gate passes, otherwise fails.
; In: none. Gate: (ds:2811 AND ds:282C AND ds:2805) != 0 and KeyDownTable[sc] = 0 for every
;     scancode sc in 01h..7Eh except 11h, 1Dh and 38h (W, Ctrl and Alt in set 1).
; Out: CF = 1 when the gate fails (nothing drawn). Otherwise ShowCreditsScreen, a wait
;      (WaitForButtonPressRelease), ClearDisplayPage, a DAC block reload (SetLevelPalette,
;      palette chosen by byte_2D8A), and CF = 0.
; Notes: KeyDownTable (ds:27F4) is written per scancode by the keyboard ISR (MODULE1 loc_19B0).
TryShowCredits:
    mov al, byte ptr byte_2811
    and al, byte ptr byte_282C
    and al, byte ptr byte_2805
    je loc_2479
    mov bx, 1
    xor al, al
loc_248D:
    cmp bl, 11h
    je loc_24A3
    cmp bl, 1Dh
    je loc_24A3
    cmp bl, 38h
    je loc_24A3
    cmp byte ptr [bx+KeyDownTable], 0
    jne loc_2479
loc_24A3:
    inc bx
    cmp bl, 7Fh
    jb loc_248D
    push ax
    push bx
    push cx
    push dx
    push di
    push si
    push ds
    push es
    push bp
    call ShowCreditsScreen
    call WaitForButtonPressRelease
    call ClearDisplayPage
    call SetLevelPalette
    clc
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

; Unused (no caller in the binary). Sets the credits video mode (SetLevelVideoMode), draws the
; credits (ShowCreditsScreen) and waits until byte_27E8 is set or word_27F0 reaches 168h
; timer ticks counted from 0. Then sets word_27F0 = 870h.
; In/Out: none. All registers are preserved.
ShowCreditsTimed:
    push ax
    push bx
    push cx
    push dx
    push di
    push si
    push ds
    push es
    push bp
    mov ax, seg DATA
    mov ds, ax
    call SetLevelVideoMode
    call ShowCreditsScreen
    mov word ptr word_27F0, 0
loc_24E3:
    call PollInputState
    cmp byte ptr ds:byte_27E8, 0
    jne loc_24F5
    cmp word ptr ds:word_27F0, 168h
    jb loc_24E3
loc_24F5:
    mov word ptr word_27F0, 870h
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

; Clears the text page (ClearDisplayPage) and draws the credits lines (ds:2930h..29E4h) with
; PrintAsciiz at TextRowOffset and TextByteColumn. Mode 0Dh must already be set.
; In/Out: none. All registers are preserved.
ShowCreditsScreen:
    push ax
    push bx
    push cx
    push dx
    push di
    push si
    push ds
    push es
    push bp
    mov ax, seg DATA
    mov ds, ax
    call ClearDisplayPage
    call SetStartupPalette
    mov word ptr TextByteColumn, 1
    mov word ptr TextRowOffset, 140h
    mov si, 2930h
    call PrintAsciiz
    add word ptr TextRowOffset, 230h
    mov word ptr TextByteColumn, 0Eh
    mov si, 2955h
    call PrintAsciiz
    add word ptr TextRowOffset, 460h
    mov word ptr TextByteColumn, 4
    mov si, 2960h
    call PrintAsciiz
    add word ptr TextRowOffset, 230h
    mov word ptr TextByteColumn, 0Bh
    mov si, 297Fh
    call PrintAsciiz
    add word ptr TextRowOffset, 460h
    mov word ptr TextByteColumn, 9
    mov si, 2990h
    call PrintAsciiz
    add word ptr TextRowOffset, 230h
    mov word ptr TextByteColumn, 0Bh
    mov si, 29A5h
    call PrintAsciiz
    mov word ptr TextByteColumn, 0Fh
    mov word ptr TextRowOffset, 1770h
    mov si, 29B6h
    call PrintAsciiz
    mov word ptr TextByteColumn, 2
    mov word ptr TextRowOffset, 1A40h
    mov si, 29C0h
    call PrintAsciiz
    add word ptr TextRowOffset, 1E0h
    mov word ptr TextByteColumn, 0
    mov si, 29E4h
    call PrintAsciiz
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
loc_25C5:
    stc
    ret

; Shows the intro picture when a key-state gate passes. There is no year check.
; In: none. Gate: (ds:2811 AND ds:282C AND ds:2806) != 0 and KeyDownTable[sc] = 0 for every
;     scancode sc in 01h..7Eh except 12h, 1Dh and 38h (E, Ctrl and Alt in set 1).
; Out: CF = 1 when the gate fails. Otherwise ShowIntroBody runs and CF = 0.
TryShowIntroKeyed:
    mov al, byte ptr byte_2811
    and al, byte ptr byte_282C
    and al, byte ptr byte_2806
    je loc_25C5
    mov bx, 1
    xor al, al
loc_25D9:
    cmp bl, 12h
    je loc_25EE
    cmp bl, 1Dh
    je loc_25EE
    cmp bl, 38h
    je loc_25EE
    or al, byte ptr [bx+KeyDownTable]
    jne loc_25C5
loc_25EE:
    inc bx
    cmp bl, 7Fh
    jb loc_25D9
    jmp short ShowIntroBody

; Shows the title picture (LEVELI.SQZ and LEVELH.SQZ, 80 bytes x 415 rows per plane) when the
; clock year is at least 1994 (7CAh).
; In: ClockYear. Out: CF = 1 and nothing done when ClockYear < 1994; otherwise ShowIntroBody
;     runs and CF = 0.
; Notes: each LoadSqzFile call starts at NextFreeSegment, which is reset to the load
;        segment after the call (word_0039 is the base). So the first LEVELI load is
;        overwritten by the LEVELH load and has no effect; LEVELI is loaded again for planes 2
;        and 3. The decoded pictures are temporary.
ShowIntroPicture:
    cmp word ptr ds:ClockYear, 7CAh
    jb loc_25C5
ShowIntroBody:
; ShowIntroBody (shared by ShowIntroPicture and TryShowIntroKeyed):
;   INT 10h AX=12h: mode 12h (640x480x16 planar, 80 bytes per row).
;   INT 10h AX=1002h: attribute palette from ds:28B0 (identity 00h..0Fh), overscan 00h.
;   DAC 3C8h index 00h, then 16 entries of three equal values v (3C9h), v = 0, 4, ..., 60.
;   SEQ 3C4h index 02h (map mask): 01h, 02h from LEVELH; 04h, 08h from LEVELI. Each plane is a
;   rep movsw of 33200 bytes (0x81B0) from the decoded segment to A000:0000.
;   Then WaitForButtonPressRelease, then SetLevelVideoMode sets mode 0Dh and the game palette.
    push ax
    push bx
    push cx
    push dx
    push di
    push si
    push ds
    push es
    push bp
    mov ax, seg DATA
    mov ds, ax
    mov ax, word ptr word_0039
    mov word ptr NextFreeSegment, ax
    mov dx, 2A0Ch
    push ds
    push es
    call LoadSqzFile
    pop es
    pop ds
    mov word ptr NextFreeSegment, ax
    mov word ptr PictureSegment, ax
    mov ax, 12h
    int 10h
    mov ax, seg DATA
    mov ds, ax
    mov es, ax
    mov dx, 28B0h
    mov ax, 1002h
    int 10h
    mov dx, 3C8h
    xor al, al
    out dx, al
    inc dx
    mov cx, 10h
    xor al, al
loc_2642:
    out dx, al
    jmp short loc_2645
loc_2645:
    out dx, al
    jmp short loc_2648
loc_2648:
    out dx, al
    add al, 4
    loop loc_2642
    mov dx, 2A17h
    push ds
    push es
    call LoadSqzFile
    pop es
    pop ds
    mov word ptr NextFreeSegment, ax
    mov word ptr PictureSegment, ax
    mov ax, 0A000h
    mov es, ax
    mov bx, word ptr PictureSegment
    mov dx, 3C4h
    mov ax, 102h
loc_266C:
    xor si, si
    mov ds, bx
    out dx, ax
    mov cx, 40D8h
    xor di, di
    rep movsw
    add bx, 81Bh
    shl ah, 1
    cmp ah, 2
    jbe loc_266C
    mov ax, seg DATA
    mov ds, ax
    mov dx, 2A22h
    push ds
    push es
    call LoadSqzFile
    pop es
    pop ds
    mov word ptr NextFreeSegment, ax
    mov word ptr PictureSegment, ax
    mov ax, 0A000h
    mov es, ax
    mov bx, word ptr PictureSegment
    mov dx, 3C4h
    mov ax, 402h
loc_26A7:
    xor si, si
    mov ds, bx
    out dx, ax
    mov cx, 40D8h
    xor di, di
    rep movsw
    add bx, 81Bh
    shl ah, 1
    cmp ah, 8
    jbe loc_26A7
    call WaitForButtonPressRelease
    call SetLevelVideoMode
    pop bp
    pop es
    pop ds
    pop si
    pop di
    pop dx
    pop cx
    pop bx
    pop ax
    clc
    ret

; Advances the pseudo-random seed: RandomSeed = ror3(RandomSeed + 9248h).
; In: RandomSeed (initial data value 1234h). Out: RandomSeed updated; AX is clobbered.
StepRandomSeed:
    mov ax, word ptr RandomSeed
    add ax, 9248h
    ror ax, 1
    ror ax, 1
    ror ax, 1
    mov word ptr RandomSeed, ax
    ret
; DATA: sprite blitter state
    db 00h
; Sprite blitter state: code-segment variables (accessed through cs:) set per object by
; DrawSpriteObjects. They are meaningful only inside one object's draw. Words: SpriteSourceSkip
; (bytes), SpriteSrcOffset, SpriteSrcSegment, SpriteDestOffset, SpriteScreenX, SpriteTopRow.
; Bytes: MirrorBits (bit 0 = mirror), ClipXFlag, SpriteVisibleBytes, SpriteRowBytes,
; RowCounter, CurrentPlaneMask, SpriteVisibleRows, SpriteLeftClipBytes, SpriteRightClipBytes,
; EdgeByteInvert (toggled per pass), PlanePassMode (0 = skip, 10h = all planes, else 1..8).
; They lie before the blitter code at 26FAh, so the fatal-path erase (rep stosw from 26FAh)
; does not clear them.
SpriteSourceSkip label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
MirrorBits label byte
; UNKNOWN: not yet classified as code or data
    db 00h
ClipXFlag label byte
; UNKNOWN: not yet classified as code or data
    db 00h
SpriteVisibleBytes label byte
; UNKNOWN: not yet classified as code or data
    db 00h
SpriteRowBytes label byte
; UNKNOWN: not yet classified as code or data
    db 00h
RowCounter label byte
; UNKNOWN: not yet classified as code or data
    db 00h
CurrentPlaneMask label byte
; UNKNOWN: not yet classified as code or data
    db 00h
SpriteTopRow label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
SpriteScreenX label byte
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
SpriteVisibleRows label byte
; UNKNOWN: not yet classified as code or data
    db 00h
SpriteLeftClipBytes label byte
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
SpriteRightClipBytes label byte
; UNKNOWN: not yet classified as code or data
    db 00h
EdgeByteInvert label byte
; UNKNOWN: not yet classified as code or data
    db 00h
SpriteDestOffset label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
SpriteSrcOffset label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
SpriteSrcSegment label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
PlanePassMode label byte
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 00h

; Draws every active object of the object list into the draw page (DrawPage) and updates the
; object flags. Increments FrameCounter on entry (INC WORD, so the byte at ds:6BD6 also changes).
; In/Out: object records and sprite tables in ds. All registers and segments are preserved.
; Object list: 116 records of 18 bytes at ds:4F0Ah..5720h, walked from 5720h down. The record
;   at the lowest address is drawn last, on top. Record fields (ds offsets):
;   +0 word X (anchor); +2 word Y (anchor, the bottom edge of the sprite);
;   +4 word frame word: bit 15 mirror (horizontal flip); bit 14 flash request (drawn as colour
;      15 silhouette, cleared when drawn); bit 13 drawn flag (cleared on entry, set when drawn);
;      bits 0..12 frame index; 0FFFFh = empty record.
;   +11h byte blink countdown (saturates at 0): while it is nonzero, the record is drawn only
;      when (SnowFrameTick & 3) == 0.
; Frame tables: FrameWidths (px), FrameHeights (rows) at ds:7190 and FrameHotX/FrameHotY at
;   ds:752A/752B, each word per frame (index = frame * 2). FrameDataSegments/FrameDataOffsets
;   (ds:62E8, ds:5F48) locate the 5-block frame data (see LoadSpriteSheet).
;   FrameHotY was converted to (height - hot) by LoadSpriteSheet.
; Camera: CameraCol and CameraRow are in 16-pixel units (x16 here); FineScroll (0..255) adds to
;   Y; ShakeRows is added to Y before the camera.
; Window: columns 0..319 (ScreenRowBytes = 40 bytes per plane row); rows 0..175 (0B0h).
; Left = X - hotX - CameraCol*16 (mirror: X + hotX - w). Culled when left >= 320 or the right
;   edge is left of column 0 (an 8-bit carry test: exact only for |left| < 256).
; rel = Y + hotY + ShakeRows - (CameraRow*16 + FineScroll). Culled unless rel > 0 and
;   rel - h < 176. top = rel - h.
; Clipping: top rows above row 0 and bottom rows below row 176 are removed. Both counts go into
;   SpriteSourceSkip; each plane block starts at its first visible row because the previous
;   pass consumed only its visible rows. Columns clipped at the left or right are removed per
;   row (SpriteLeftClipBytes, SpriteRightClipBytes). A fractional X uses the VGA data rotate.
; Mirror: the frame is read backwards (DF = 1) and each byte is bit-reversed by XLAT.
; Passes: a clear pass writes NOT(mask) with AND, all four planes at once (SEQ 02h = 0Fh),
;   then one OR pass per colour plane with map mask 01h, 02h, 04h, 08h. Flash silhouettes skip
;   the clear pass and OR the mask into all planes (colour 15).
; VGA written: GC 05h=00h and GC 03h=00h (SetCpuWriteMode); SEQ 02h=0Fh; GC 03h=(x&7)|08h (AND)
;   for the clear pass; GC 03h=1003h or (x&7)|10h (OR) for the colour passes; SEQ 02h = the
;   plane mask. GC 04h (read map) and GC 08h (bit mask) are not written here.
; Frame 309 (bx = 26Ah) bypasses the camera and the left and rel > 0 tests (see note at 27DE).
DrawSpriteObjects:
    push ax
    push bx
    push cx
    push dx
    push di
    push si
    push ds
    push es
    push bp
    mov ax, seg DATA
    mov ds, ax
    inc word ptr SnowFrameTick
    mov si, 5720h
SpriteObjectLoop:
    mov word ptr CurrentObject, si
    cmp word ptr [si+4], -1
    jne loc_271C
    jmp near ptr NextSpriteObject
loc_271C:
    xor ax, ax
    mov byte ptr cs:ClipXFlag, al
    mov byte ptr cs:SpriteLeftClipBytes, al
    mov byte ptr cs:SpriteRightClipBytes, al
    mov byte ptr cs:EdgeByteInvert, al
    mov word ptr cs:SpriteSourceSkip, ax
    and byte ptr [si+5], 0DFh
    mov bx, word ptr [si+4]
    shl bx, 1
    rcl byte ptr cs:MirrorBits, 1
    mov al, 1
    sub byte ptr [si+11h], 1
    adc byte ptr [si+11h], 0
    je loc_2757
    test byte ptr SnowFrameTick, 3
    je loc_2757
    mov al, 0
    jmp short loc_275E
loc_2757:
    test bh, 80h
    je loc_2761
    mov al, 10h
loc_275E:
    and bh, 7Fh
loc_2761:
    mov byte ptr cs:PlanePassMode, al
    mov dx, word ptr [bx+FrameWidths]
    mov al, byte ptr [bx+FrameHotX]
    test byte ptr cs:MirrorBits, 1
    je loc_2779
    sub al, dl
    neg al
loc_2779:
    cbw
    neg ax
    add ax, word ptr [si]
    cmp bx, 26Ah
    jne loc_2792
    mov word ptr cs:SpriteScreenX, ax
    mov al, byte ptr [bx+FrameHotY]
    cbw
    add ax, word ptr [si+2]
    jmp short loc_27DE
loc_2792:
    mov cx, word ptr CameraCol
    shl cx, 1
    shl cx, 1
    shl cx, 1
    shl cx, 1
    sub ax, cx
    mov cx, ax
    mov word ptr cs:SpriteScreenX, ax
    cmp ax, 140h
    jl loc_27AE
    jmp near ptr NextSpriteObject
loc_27AE:
    add cl, dl
    adc ch, 0
    jns loc_27B8
    jmp near ptr NextSpriteObject
loc_27B8:
    mov al, byte ptr [bx+FrameHotY]
    cbw
    add ax, word ptr [si+2]
    add ax, word ptr ShakeRows
    mov cx, word ptr CameraRow
    shl cx, 1
    shl cx, 1
    shl cx, 1
    shl cx, 1
    add cl, byte ptr FineScroll
    adc ch, 0
    sub ax, cx
    jg loc_27DE
    jmp near ptr NextSpriteObject
loc_27DE:
; quirk: frame 309 reaches this label from the X calculation with no camera offset, no left
;   visibility test and no rel > 0 test; only the top < 176 test applies to it.
    mov word ptr SpriteRelY, ax
    mov byte ptr cs:SpriteVisibleRows, dh
    mov byte ptr cs:RowCounter, dh
    sub al, dh
    sbb ah, 0
    cmp ax, 0B0h
    jl loc_27F8
    jmp near ptr NextSpriteObject
loc_27F8:
    mov word ptr cs:SpriteTopRow, ax
    mov cl, byte ptr cs:cvar_0000
    shr dl, cl
    mov byte ptr cs:SpriteVisibleBytes, dl
    mov byte ptr cs:SpriteRowBytes, dl
    xor dh, dh
    xor cx, cx
    or ax, ax
    jns loc_2828
    neg ax
    sub byte ptr cs:SpriteVisibleRows, al
    mul dl
    mov cx, ax
    mov word ptr cs:SpriteSourceSkip, ax
    xor di, di
    jmp short loc_282E
loc_2828:
    mul byte ptr ScreenRowBytes
    mov di, ax
loc_282E:
    add di, word ptr ds:DrawPage
    mov word ptr cs:SpriteDestOffset, di
    mov ax, word ptr [bx+FrameDataSegments]
    mov word ptr cs:SpriteSrcSegment, ax
    mov ax, word ptr [bx+FrameDataOffsets]
    add ax, cx
    mov word ptr cs:SpriteSrcOffset, ax
    mov ax, word ptr SpriteRelY
    sub ax, 0B0h
    jle loc_285D
    sub byte ptr cs:SpriteVisibleRows, al
    mul dl
    add word ptr cs:SpriteSourceSkip, ax
loc_285D:
    mov ax, word ptr cs:SpriteScreenX
    or ax, ax
    jns loc_287E
    mov cl, byte ptr cs:cvar_0000
    sar ax, cl
    add byte ptr cs:SpriteVisibleBytes, al
    neg al
    mov byte ptr cs:SpriteLeftClipBytes, al
    inc byte ptr cs:ClipXFlag
    jmp short loc_28B6
loc_287E:
    mov cl, byte ptr cs:cvar_0000
    sar ax, cl
    add word ptr cs:SpriteDestOffset, ax
    mov ax, word ptr cs:SpriteScreenX
    mov cl, byte ptr cs:cvar_0000
    shr ax, cl
    add al, byte ptr cs:SpriteVisibleBytes
    adc ah, 0
    sub ax, word ptr ScreenRowBytes
    jb loc_28B6
    sub byte ptr cs:SpriteVisibleBytes, al
    inc byte ptr cs:ClipXFlag
    mov byte ptr cs:SpriteRightClipBytes, al
    not byte ptr cs:EdgeByteInvert
loc_28B6:
    or byte ptr [si+5], 20h
    and byte ptr [si+5], 0BFh
    call SetCpuWriteMode
    mov dx, 3C4h
    mov ax, 0F02h
    out dx, ax
    mov cl, byte ptr cs:SpriteScreenX
    and cl, 7
    mov ch, 0FFh
    shl ch, cl
    mov di, word ptr cs:SpriteDestOffset
    mov si, word ptr cs:SpriteSrcOffset
    mov dl, 0CEh
    mov al, 3
    mov ah, cl
    or ah, 8
    out dx, ax
    mov bh, ch
    not bh
    mov dh, byte ptr cs:SpriteVisibleBytes
    mov cl, dh
    mov al, cl
    cbw
    sub ax, 28h
    mov bp, ax
    mov dl, byte ptr cs:SpriteVisibleRows
    mov ds, word ptr cs:SpriteSrcSegment
    mov ax, 0A000h
    mov es, ax
    test byte ptr cs:MirrorBits, 1
    jne DrawSpriteMirrored
    jmp near ptr DrawSpriteStraight
DrawSpriteMirrored:
; quirk / translation note (mirror path): "xlat byte ptr cs:[bx]" is encoded 2E D7, so XLAT reads
;   CS:[BX + AL]: the bit-reversal table at cs:2F34h (byte i = reverse8(i), verified for all 256
;   entries). BX = 2F34h is set at 291Ch. DS (the frame segment) is not used by these reads.
    mov byte ptr cs:RowCounter, dl
    mov dx, bx
    mov bx, 2F34h
    std
    dec si
    cmp byte ptr cs:ClipXFlag, 0
    je loc_292C
    jmp near ptr loc_2AA1
loc_292C:
    mov al, cl
    cbw
    add si, ax
    cmp ch, 0FFh
    je loc_2939
    jmp near ptr loc_29D7
loc_2939:
    xor ch, ch
    cmp byte ptr cs:PlanePassMode, 10h
    je loc_2963
loc_2943:
    lodsb
    xlat byte ptr cs:[bx]
    not al
    xchg byte ptr es:[di], al
    inc di
    loop loc_2943
    mov cl, byte ptr cs:SpriteVisibleBytes
    mov al, cl
    cbw
    add si, ax
    add si, ax
    sub di, bp
    dec byte ptr cs:RowCounter
    jne loc_2943
loc_2963:
    push dx
    mov dx, 3CEh
    mov ax, 1003h
    out dx, ax
    mov dl, 0C4h
    mov al, 2
    out dx, al
    not byte ptr cs:EdgeByteInvert
    pop dx
    mov al, byte ptr cs:PlanePassMode
    or al, al
    je loc_29D4
    cmp al, 10h
    jne loc_2989
    mov al, 0Fh
    sub si, word ptr cs:SpriteSourceSkip
loc_2989:
    push dx
    mov byte ptr cs:CurrentPlaneMask, al
    mov dx, 3C5h
    out dx, al
    mov cl, byte ptr cs:SpriteVisibleBytes
    mov dl, byte ptr cs:SpriteVisibleRows
    mov byte ptr cs:RowCounter, dl
    mov di, word ptr cs:SpriteDestOffset
    add si, word ptr cs:SpriteSourceSkip
    pop dx
loc_29AC:
    lodsb
    xlat byte ptr cs:[bx]
    xchg byte ptr es:[di], al
    inc di
    loop loc_29AC
    mov cl, byte ptr cs:SpriteVisibleBytes
    mov al, cl
    cbw
    add si, ax
    add si, ax
    sub di, bp
    dec byte ptr cs:RowCounter
    jne loc_29AC
    mov al, byte ptr cs:CurrentPlaneMask
    shl al, 1
    cmp al, 10h
    jb loc_2989
loc_29D4:
    jmp near ptr NextSpriteObject
loc_29D7:
    cmp byte ptr cs:PlanePassMode, 10h
    je loc_2A10
loc_29DF:
    lodsb
    xlat byte ptr cs:[bx]
    mov dl, al
    and al, ch
    or al, ah
    not al
    xchg byte ptr es:[di], al
    inc di
    and dl, dh
    mov ah, dl
    dec cl
    jne loc_29DF
    not ah
    xchg byte ptr es:[di], ah
    mov cl, byte ptr cs:SpriteVisibleBytes
    mov al, cl
    cbw
    add si, ax
    add si, ax
    sub di, bp
    dec byte ptr cs:RowCounter
    jne loc_29DF
loc_2A10:
    push dx
    mov cl, byte ptr cs:SpriteScreenX
    and cl, 7
    mov dx, 3CEh
    mov al, 3
    mov ah, cl
    or ah, 10h
    out dx, ax
    mov dl, 0C4h
    mov al, 2
    out dx, al
    not byte ptr cs:EdgeByteInvert
    pop dx
    mov al, byte ptr cs:PlanePassMode
    or al, al
    je loc_2A9E
    cmp al, 10h
    jne loc_2A42
    mov al, 0Fh
    sub si, word ptr cs:SpriteSourceSkip
loc_2A42:
    push dx
    mov byte ptr cs:CurrentPlaneMask, al
    mov dx, 3C5h
    out dx, al
    mov cl, byte ptr cs:SpriteVisibleBytes
    mov dl, byte ptr cs:SpriteVisibleRows
    mov byte ptr cs:RowCounter, dl
    mov di, word ptr cs:SpriteDestOffset
    add si, word ptr cs:SpriteSourceSkip
    pop dx
    xor ax, ax
loc_2A67:
    lodsb
    xlat byte ptr cs:[bx]
    mov dl, al
    and al, ch
    or al, ah
    xchg byte ptr es:[di], al
    inc di
    and dl, dh
    mov ah, dl
    dec cl
    jne loc_2A67
    xchg byte ptr es:[di], ah
    mov cl, byte ptr cs:SpriteVisibleBytes
    mov al, cl
    cbw
    add si, ax
    add si, ax
    sub di, bp
    dec byte ptr cs:RowCounter
    jne loc_2A67
    mov al, byte ptr cs:CurrentPlaneMask
    shl al, 1
    cmp al, 10h
    jb loc_2A42
loc_2A9E:
    jmp near ptr NextSpriteObject
loc_2AA1:
    mov al, byte ptr cs:SpriteRowBytes
    cbw
    add si, ax
loc_2AA8:
    mov al, byte ptr cs:SpriteLeftClipBytes
    cbw
    or ax, ax
    je loc_2ABC
    dec ax
    sub si, ax
    lodsb
    xlat byte ptr cs:[bx]
    mov dl, al
    inc cx
    jmp short loc_2ACB
loc_2ABC:
    lodsb
    xlat byte ptr cs:[bx]
    mov dl, al
    and al, ch
    or al, ah
    not al
    xchg byte ptr es:[di], al
    inc di
loc_2ACB:
    and dl, dh
    mov ah, dl
    dec cl
    jne loc_2ABC
    not ah
    or ah, byte ptr cs:EdgeByteInvert
    xchg byte ptr es:[di], ah
    mov cl, byte ptr cs:SpriteVisibleBytes
    mov al, byte ptr cs:SpriteRowBytes
    cbw
    add si, ax
    sub al, byte ptr cs:SpriteRightClipBytes
    add si, ax
    sub di, bp
    dec byte ptr cs:RowCounter
    jne loc_2AA8
    push dx
    mov cl, byte ptr cs:SpriteScreenX
    and cl, 7
    mov dx, 3CEh
    mov al, 3
    mov ah, cl
    or ah, 10h
    out dx, ax
    mov dl, 0C4h
    mov al, 2
    out dx, al
    not byte ptr cs:EdgeByteInvert
    pop dx
    mov al, byte ptr cs:PlanePassMode
    or al, al
    jne loc_2B23
    jmp near ptr loc_2BC0
loc_2B23:
    cmp al, 10h
    jne loc_2B36
    mov al, 0Fh
    mov byte ptr cs:CurrentPlaneMask, 1
    sub si, word ptr cs:SpriteSourceSkip
    jmp short loc_2B3A
loc_2B36:
    mov byte ptr cs:CurrentPlaneMask, al
loc_2B3A:
    push dx
    mov dx, 3C5h
    out dx, al
    mov cl, byte ptr cs:SpriteVisibleBytes
    mov dl, byte ptr cs:SpriteVisibleRows
    mov byte ptr cs:RowCounter, dl
    mov di, word ptr cs:SpriteDestOffset
    add si, word ptr cs:SpriteSourceSkip
    pop dx
loc_2B59:
    mov al, byte ptr cs:SpriteLeftClipBytes
    cbw
    or ax, ax
    je loc_2B6D
    dec ax
    sub si, ax
    lodsb
    xlat byte ptr cs:[bx]
    mov dl, al
    inc cx
    jmp short loc_2B7A
loc_2B6D:
    lodsb
    xlat byte ptr cs:[bx]
    mov dl, al
    and al, ch
    or al, ah
    xchg byte ptr es:[di], al
    inc di
loc_2B7A:
    and dl, dh
    mov ah, dl
    dec cl
    jne loc_2B6D
    and ah, byte ptr cs:EdgeByteInvert
    xchg byte ptr es:[di], ah
    mov cl, byte ptr cs:SpriteVisibleBytes
    mov al, byte ptr cs:SpriteRowBytes
    cbw
    add si, ax
    sub al, byte ptr cs:SpriteRightClipBytes
    add si, ax
    sub di, bp
    dec byte ptr cs:RowCounter
    jne loc_2B59
    shl byte ptr cs:CurrentPlaneMask, 1
    mov al, byte ptr cs:CurrentPlaneMask
    cmp al, 10h
    jae loc_2BC0
    cmp byte ptr cs:PlanePassMode, 10h
    jne loc_2BBD
    mov al, 0Fh
loc_2BBD:
    jmp near ptr loc_2B3A
loc_2BC0:
    jmp near ptr NextSpriteObject
DrawSpriteStraight:
    cmp byte ptr cs:ClipXFlag, 0
    je loc_2BCE
    jmp near ptr loc_2CEA
loc_2BCE:
    cmp ch, 0FFh
    jne loc_2C45
    xor ch, ch
    cmp byte ptr cs:PlanePassMode, 10h
    je loc_2BED
loc_2BDD:
    lodsb
    not al
    scasb
    dec di
    stosb
    loop loc_2BDD
    mov cl, dh
    sub di, bp
    dec dl
    jne loc_2BDD
loc_2BED:
    mov dx, 3CEh
    mov ax, 1003h
    out dx, ax
    mov dl, 0C4h
    mov al, 2
    out dx, al
    mov al, byte ptr cs:PlanePassMode
    or al, al
    je loc_2C42
    cmp al, 10h
    jne loc_2C0C
    mov al, 0Fh
    sub si, word ptr cs:SpriteSourceSkip
loc_2C0C:
    mov byte ptr cs:CurrentPlaneMask, al
    mov dx, 3C5h
    out dx, al
    mov dh, byte ptr cs:SpriteVisibleBytes
    mov cl, dh
    mov dl, byte ptr cs:SpriteVisibleRows
    mov di, word ptr cs:SpriteDestOffset
    add si, word ptr cs:SpriteSourceSkip
loc_2C2A:
    lodsb
    scasb
    dec di
    stosb
    loop loc_2C2A
    sub di, bp
    mov cl, dh
    dec dl
    jne loc_2C2A
    mov al, byte ptr cs:CurrentPlaneMask
    shl al, 1
    cmp al, 10h
    jb loc_2C0C
loc_2C42:
    jmp near ptr NextSpriteObject
loc_2C45:
    cmp byte ptr cs:PlanePassMode, 10h
    je loc_2C70
    xor ax, ax
loc_2C4F:
    lodsb
    mov bl, al
    and al, ch
    or al, ah
    not al
    scasb
    dec di
    stosb
    and bl, bh
    mov ah, bl
    dec cl
    jne loc_2C4F
    not ah
    xchg byte ptr es:[di], ah
    mov cl, dh
    sub di, bp
    dec dl
    jne loc_2C45
loc_2C70:
    mov cl, byte ptr cs:SpriteScreenX
    and cl, 7
    mov dx, 3CEh
    mov al, 3
    mov ah, cl
    or ah, 10h
    out dx, ax
    mov dl, 0C4h
    mov al, 2
    out dx, al
    not byte ptr cs:EdgeByteInvert
    mov al, byte ptr cs:PlanePassMode
    or al, al
    je loc_2CE7
    cmp al, 10h
    jne loc_2CA0
    mov al, 0Fh
    sub si, word ptr cs:SpriteSourceSkip
loc_2CA0:
    mov byte ptr cs:CurrentPlaneMask, al
    mov dx, 3C5h
    out dx, al
    mov dh, byte ptr cs:SpriteVisibleBytes
    mov cl, dh
    mov dl, byte ptr cs:SpriteVisibleRows
    mov di, word ptr cs:SpriteDestOffset
    add si, word ptr cs:SpriteSourceSkip
loc_2CBE:
    xor ax, ax
loc_2CC0:
    lodsb
    mov bl, al
    and al, ch
    or al, ah
    scasb
    dec di
    stosb
    and bl, bh
    mov ah, bl
    dec cl
    jne loc_2CC0
    xchg byte ptr es:[di], ah
    mov cl, dh
    sub di, bp
    dec dl
    jne loc_2CBE
    mov al, byte ptr cs:CurrentPlaneMask
    shl al, 1
    cmp al, 10h
    jb loc_2CA0
loc_2CE7:
    jmp near ptr NextSpriteObject
loc_2CEA:
    mov al, byte ptr cs:SpriteLeftClipBytes
    cbw
    or ax, ax
    je loc_2CFC
    dec ax
    inc cx
    add si, ax
    lodsb
    mov bl, al
    jmp short loc_2D08
loc_2CFC:
    lodsb
    mov bl, al
    and al, ch
    or al, ah
    not al
    scasb
    dec di
    stosb
loc_2D08:
    and bl, bh
    mov ah, bl
    dec cl
    jne loc_2CFC
    not ah
    or ah, byte ptr cs:EdgeByteInvert
    xchg byte ptr es:[di], ah
    mov al, byte ptr cs:SpriteRightClipBytes
    cbw
    add si, ax
    mov cl, byte ptr cs:SpriteVisibleBytes
    sub di, bp
    dec dl
    jne loc_2CEA
    mov cl, byte ptr cs:SpriteScreenX
    and cl, 7
    mov dx, 3CEh
    mov al, 3
    mov ah, cl
    or ah, 10h
    out dx, ax
    mov dl, 0C4h
    mov al, 2
    out dx, al
    not byte ptr cs:EdgeByteInvert
    mov al, byte ptr cs:PlanePassMode
    or al, al
    jne loc_2D54
    jmp near ptr NextSpriteObject
loc_2D54:
    cmp al, 10h
    jne loc_2D67
    mov al, 0Fh
    mov byte ptr cs:CurrentPlaneMask, 1
    sub si, word ptr cs:SpriteSourceSkip
    jmp short loc_2D6B
loc_2D67:
    mov byte ptr cs:CurrentPlaneMask, al
loc_2D6B:
    mov dx, 3C5h
    out dx, al
    mov cl, byte ptr cs:SpriteVisibleBytes
    mov dl, byte ptr cs:SpriteVisibleRows
    mov di, word ptr cs:SpriteDestOffset
    add si, word ptr cs:SpriteSourceSkip
loc_2D83:
    mov al, byte ptr cs:SpriteLeftClipBytes
    cbw
    or ax, ax
    je loc_2D95
    dec ax
    inc cx
    add si, ax
    lodsb
    mov bl, al
    jmp short loc_2D9F
loc_2D95:
    lodsb
    mov bl, al
    and al, ch
    or al, ah
    scasb
    dec di
    stosb
loc_2D9F:
    and bl, bh
    mov ah, bl
    dec cl
    jne loc_2D95
    and ah, byte ptr cs:EdgeByteInvert
    xchg byte ptr es:[di], ah
    mov al, byte ptr cs:SpriteRightClipBytes
    cbw
    add si, ax
    mov cl, byte ptr cs:SpriteVisibleBytes
    sub di, bp
    dec dl
    jne loc_2D83
    shl byte ptr cs:CurrentPlaneMask, 1
    mov al, byte ptr cs:CurrentPlaneMask
    cmp al, 10h
    jae NextSpriteObject
    cmp byte ptr cs:PlanePassMode, 10h
    jne loc_2D6B
    mov al, 0Fh
    jmp short loc_2D6B
NextSpriteObject:
    cld
    mov ax, seg DATA
    mov ds, ax
    mov si, word ptr CurrentObject
    sub si, 12h
    cmp si, 4F0Ah
    jb loc_2DF0
    jmp near ptr SpriteObjectLoop
loc_2DF0:
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

; Loads SPRITES.SQZ (LoadSqzFile) and prepares the sprite frames.
; In: none (file name at ds:2A2Eh). Out: SpriteDataSegment = segment of the raw data;
;     FrameDataSegments/FrameDataOffsets set for frames 0..459 (frame 460 has width and height 0);
;     FrameHotY[f] = FrameHeights[f] - FrameHotY[f] for frames up to the first zero height;
;     NextFreeSegment set past size + size/4 + 1 paragraphs. All registers are preserved.
; Raw data (SPRITES.SQZ after decompression): frames in table order. Frame f holds four planes
;   (plane k = colour bit k), each ceil(w/8) * h bytes, rows top to bottom, bit 7 = leftmost.
; Expanded frame (5 blocks of ceil(w/8) * h bytes): block 0 = OR of the four planes (opaque
;   mask; colour 0 is transparent); blocks 1..4 = planes 0..3, unchanged.
; Method: the raw data moves up size/4 + 1 paragraphs (MoveSpriteDataUp), then each frame is
;   expanded from the low end while the output stays below the input (ExpandSpriteFrames).
; Notes: the hot-Y pass stops at a zero height; the expansion and BuildFrameOffsets stop at a
;   zero width. FrameWidths and FrameHeights are the bytes of one word per frame (stride 2).
LoadSpriteSheet:
    push ax
    push bx
    push cx
    push dx
    push di
    push si
    push ds
    push es
    push bp
    mov ax, seg DATA
    mov ds, ax
    mov dx, 2A2Eh
    push ds
    push es
    call LoadSqzFile
    pop es
    pop ds
    mov word ptr SpriteDataSegment, ax
    xor bx, bx
loc_2E17:
    mov al, byte ptr [bx+FrameHeights]
    or al, al
    je loc_2E2B
    sub byte ptr [bx+FrameHotY], al
    neg byte ptr [bx+FrameHotY]
    inc bx
    inc bx
    jmp short loc_2E17
loc_2E2B:
    std
    mov ax, word ptr NextFreeSegment
    mov bx, word ptr SpriteDataSegment
    sub ax, bx
    mov bp, ax
    mov dx, ax
    shr dx, 1
    shr dx, 1
    inc dx
    add ax, dx
    add ax, bx
    mov word ptr NextFreeSegment, ax
    dec ax
    mov bx, ax
    sub ax, dx
MoveSpriteDataUp:
    mov ds, ax
    mov es, bx
    mov si, 0Fh
    mov di, si
    mov cx, 10h
    rep movsb
    dec ax
    dec bx
    dec bp
    jne MoveSpriteDataUp
    cld
    mov dx, seg DATA
    mov ds, dx
    inc ax
    inc bx
    mov word ptr ExpandDstSegment, ax
    mov word ptr ExpandSrcSegment, bx
    mov bx, 7190h
    xor si, si
    xor di, di
ExpandSpriteFrames:
; Expansion loop: source pointer ExpandSrcSegment:SI, destination ExpandDstSegment:DI.
    mov ax, si
    shr ax, 1
    shr ax, 1
    shr ax, 1
    shr ax, 1
    add word ptr ExpandSrcSegment, ax
    and si, 0Fh
    mov ax, di
    shr ax, 1
    shr ax, 1
    shr ax, 1
    shr ax, 1
    add word ptr ExpandDstSegment, ax
    and di, 0Fh
    mov ax, word ptr [bx]
    or al, al
    je BuildFrameOffsets
    ror al, 1
    ror al, 1
    ror al, 1
    test al, 0E0h
    je loc_2EA9
    and al, 1Fh
    inc al
loc_2EA9:
    inc bx
    inc bx
    mul ah
    mov bp, ax
    mov es, word ptr ExpandDstSegment
    mov ds, word ptr ExpandSrcSegment
    mov cx, bp
    push si
loc_2EBA:
    mov al, byte ptr [si]
    or al, byte ptr ds:[bp+si]
    add si, bp
    or al, byte ptr ds:[bp+si]
    add si, bp
    or al, byte ptr ds:[bp+si]
    sub si, bp
    sub si, bp
    stosb
    inc si
    loop loc_2EBA
    pop si
    mov cx, bp
    shl cx, 1
    rep movsw
    mov ax, seg DATA
    mov ds, ax
    jmp short ExpandSpriteFrames
BuildFrameOffsets:
    mov ax, seg DATA
    mov ds, ax
    mov es, ax
    mov bx, word ptr SpriteDataSegment
    mov si, 7190h
    xor di, di
    xor dx, dx
loc_2EF1:
    lodsw
    or al, al
    je loc_2F2A
    ror al, 1
    ror al, 1
    ror al, 1
    test al, 0E0h
    je loc_2F04
    and al, 1Fh
    inc al
loc_2F04:
    mul ah
    mov bp, ax
    shl ax, 1
    shl ax, 1
    add ax, bp
    mov word ptr [di+FrameDataOffsets], dx
    mov word ptr [di+FrameDataSegments], bx
    cmp dx, 1000h
    jb loc_2F24
    sub dx, 1000h
    add bx, 100h
loc_2F24:
    inc di
    inc di
    add dx, ax
    jmp short loc_2EF1
loc_2F2A:
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
; DATA: bit-reversal table, byte i = reverse8(i): mirrors sprite rows through `xlat byte ptr cs:[bx]` (BX = 2F34h at 291C)
    db 00h, 80h, 40h, 0C0h, 20h, 0A0h, 60h, 0E0h, 10h, 90h, 50h, 0D0h
    db 30h, 0B0h, 70h, 0F0h, 08h, 88h, 48h, 0C8h, 28h, 0A8h, 68h, 0E8h
    db 18h, 98h, 58h, 0D8h, 38h, 0B8h, 78h, 0F8h, 04h, 84h, 44h, 0C4h
    db 24h, 0A4h, 64h, 0E4h, 14h, 94h, 54h, 0D4h, 34h, 0B4h, 74h, 0F4h
    db 0Ch, 8Ch, 4Ch, 0CCh, 2Ch, 0ACh, 6Ch, 0ECh, 1Ch, 9Ch, 5Ch, 0DCh
    db 3Ch, 0BCh, 7Ch, 0FCh, 02h, 82h, 42h, 0C2h, 22h, 0A2h, 62h, 0E2h
    db 12h, 92h, 52h, 0D2h, 32h, 0B2h, 72h, 0F2h, 0Ah, 8Ah, 4Ah, 0CAh
    db 2Ah, 0AAh, 6Ah, 0EAh, 1Ah, 9Ah, 5Ah, 0DAh, 3Ah, 0BAh, 7Ah, 0FAh
    db 06h, 86h, 46h, 0C6h, 26h, 0A6h, 66h, 0E6h, 16h, 96h, 56h, 0D6h
    db 36h, 0B6h, 76h, 0F6h, 0Eh, 8Eh, 4Eh, 0CEh, 2Eh, 0AEh, 6Eh, 0EEh
    db 1Eh, 9Eh, 5Eh, 0DEh, 3Eh, 0BEh, 7Eh, 0FEh, 01h, 81h, 41h, 0C1h
    db 21h, 0A1h, 61h, 0E1h, 11h, 91h, 51h, 0D1h, 31h, 0B1h, 71h, 0F1h
    db 09h, 89h, 49h, 0C9h, 29h, 0A9h, 69h, 0E9h, 19h, 99h, 59h, 0D9h
    db 39h, 0B9h, 79h, 0F9h, 05h, 85h, 45h, 0C5h, 25h, 0A5h, 65h, 0E5h
    db 15h, 95h, 55h, 0D5h, 35h, 0B5h, 75h, 0F5h, 0Dh, 8Dh, 4Dh, 0CDh
    db 2Dh, 0ADh, 6Dh, 0EDh, 1Dh, 9Dh, 5Dh, 0DDh, 3Dh, 0BDh, 7Dh, 0FDh
    db 03h, 83h, 43h, 0C3h, 23h, 0A3h, 63h, 0E3h, 13h, 93h, 53h, 0D3h
    db 33h, 0B3h, 73h, 0F3h, 0Bh, 8Bh, 4Bh, 0CBh, 2Bh, 0ABh, 6Bh, 0EBh
    db 1Bh, 9Bh, 5Bh, 0DBh, 3Bh, 0BBh, 7Bh, 0FBh, 07h, 87h, 47h, 0C7h
    db 27h, 0A7h, 67h, 0E7h, 17h, 97h, 57h, 0D7h, 37h, 0B7h, 77h, 0F7h
    db 0Fh, 8Fh, 4Fh, 0CFh, 2Fh, 0AFh, 6Fh, 0EFh, 1Fh, 9Fh, 5Fh, 0DFh
    db 3Fh, 0BFh, 7Fh, 0FFh

; Re-enables video output: reads 3DAh to reset the attribute flip-flop, then writes 20h to
; 3C0h (index 0 with PAS set). Preserves all registers except flags.
EnableVideoOutput:
    push dx
    push ax
    mov dx, 3DAh
    in al, dx
    mov dl, 0C0h
    mov al, 20h
    out dx, al
    pop ax
    pop dx
    ret

; Blanks video output: reads 3DAh to reset the flip-flop, then writes 00h to 3C0h (PAS clear).
; Notes: EnableVideoOutput restores the picture. Not used around every page change.
DisableVideoOutput:
    push dx
    push ax
    mov dx, 3DAh
    in al, dx
    mov dl, 0C0h
    xor al, al
    out dx, al
    pop ax
    pop dx
    ret
WipeColumnOffset label word
; DATA: screen offset words
    db 00h, 00h
WipeColumnBase label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h

; Reveals DrawPage into DisplayPage as a centre-out column wipe (level entry).
; In: DrawPage and DisplayPage set. Out: DisplayPage shows the drawn picture.
; Sets write mode via SetLatchCopyMode; ten steps. Each step copies two 2-byte columns
; (16 px wide, 176 scanlines) with CopyColumnStrip at byte offsets 20-2k and 20+2k
; (k = 0..9), then waits for two vertical retraces. Ends with SetCpuWriteMode.
; Notes: the 176-scanline band is the play field; 24 bottom scanlines are presumably HUD.
; Clobbers: WipeColumnBase/WipeColumnOffset (also used by ClearStaggeredBlocks).
RevealLevelColumns:
    call SetLatchCopyMode
    mov ax, seg DATA
    mov ds, ax
    mov word ptr cs:WipeColumnBase, 14h
    mov word ptr cs:WipeColumnOffset, 0
loc_306A:
    mov di, word ptr cs:WipeColumnBase
    mov cx, 0B0h
    call CopyColumnStrip
    add di, word ptr cs:WipeColumnOffset
    call CopyColumnStrip
    call WaitVerticalRetrace
    call WaitVerticalRetrace
    sub word ptr cs:WipeColumnBase, 2
    add word ptr cs:WipeColumnOffset, 4
    cmp word ptr cs:WipeColumnOffset, 28h
    jb loc_306A
    call SetCpuWriteMode
    ret

; Copies one 2-byte-wide column of 176 scanlines from DrawPage to DisplayPage.
; In: DI = byte offset of the column in the page (40 bytes per scanline).
; Requires latch-copy mode (SetLatchCopyMode) and map mask 0Fh, as set by RevealLevelColumns.
; Preserves AX CX DI SI DS ES. Destroys no other state.
CopyColumnStrip:
    push ax
    push cx
    push di
    push si
    push ds
    push es
    mov si, di
    add si, word ptr DrawPage
    add di, word ptr ds:DisplayPage
    mov ax, 0A000h
    mov ds, ax
    mov es, ax
    mov cx, 0B0h
loc_30B5:
    movsb
    movsb
    add di, 26h
    add si, 26h
    loop loc_30B5
    pop es
    pop ds
    pop si
    pop di
    pop cx
    pop ax
    ret

; Blanks a staggered pattern of 16x10 pixel blocks on DisplayPage (a dissolve-style blank).
; Resets FineScroll to 0. Nine bands of 10 scanlines; each band clears 20 blocks at the band
; and 20 blocks at an offset that starts at 166 scanlines and shrinks by 20 per band.
; Uses WipeColumnBase/WipeColumnOffset as scratch. Visual intent unclear.
ClearStaggeredBlocks:
    push ax
    push cx
    push dx
    push di
    push ds
    push es
    call SetCpuWriteMode
    mov dx, 3C4h
    mov ax, 0F02h
    out dx, ax
    mov ax, seg DATA
    mov ds, ax
    mov byte ptr FineScroll, 0
    mov ax, word ptr DisplayPage
    mov word ptr cs:WipeColumnBase, ax
    mov word ptr cs:WipeColumnOffset, 19F0h
loc_30EE:
    mov di, word ptr cs:WipeColumnBase
    mov cx, 14h
loc_30F6:
    call ClearBlock16x10
    inc di
    inc di
    loop loc_30F6
    mov di, word ptr cs:WipeColumnBase
    add di, word ptr cs:WipeColumnOffset
    mov cx, 14h
loc_310A:
    call ClearBlock16x10
    inc di
    inc di
    loop loc_310A
    call WaitVerticalRetrace
    call WaitVerticalRetrace
    add word ptr cs:WipeColumnBase, 190h
    sub word ptr cs:WipeColumnOffset, 320h
    jae loc_30EE
    call SetCpuWriteMode
    pop es
    pop ds
    pop di
    pop dx
    pop cx
    pop ax
    ret

; Zeroes one 16x10 pixel block (two bytes wide, ten scanlines) at ES:DI, all planes (map mask
; 0Fh and write mode 0 must be set by the caller: ClearStaggeredBlocks does this).
; Out: DI unchanged. Stride 40 bytes per scanline.
ClearBlock16x10:
    push ax
    push cx
    push di
    push es
    mov ax, 0A000h
    mov es, ax
    mov cx, 0Ah
    xor ax, ax
loc_313F:
    stosw
    add di, 26h
    loop loc_313F
    pop es
    pop di
    pop cx
    pop ax
    ret

; Clears DisplayPage (8000 bytes = 320x200 in one EGA plane, four planes) to zero.
; Notes: clears DisplayPage (the page not being drawn into), not DrawPage.
; Sets write mode 0 through SetCpuWriteMode and map mask 0Fh.
ClearDisplayPage:
    push ax
    push cx
    push dx
    push di
    push es
    call SetCpuWriteMode
    mov ax, 0A000h
    mov es, ax
    mov dx, 3C4h
    mov ax, 0F02h
    out dx, ax
    mov di, word ptr ds:DisplayPage
    mov cx, 0FA0h
    xor ax, ax
    rep stosw
    pop es
    pop di
    pop dx
    pop cx
    pop ax
    ret

; Iris-close transition: each frame blacks out the scanlines outside an ellipse of IrisRadius.
; In: DrawPage, DisplayPage, PlayerX/PlayerY, CameraCol/CameraRow, FineScroll, IrisRadius
;   (starts at 0E6h), IrisSpeed (starts at 4; incremented every 20 frames).
; Steps: copies DrawPage to DisplayPage once; sets render slots 2.. to empty (+4 = FFFFh);
;   per frame computes 65 boundary points from SineTable (the second table is at 7090h),
;   clears the spans outside them with BlackoutSpan, then PresentFrame, sub_6772 (the
;   fade pass, outside this region), and subtracts IrisSpeed from IrisRadius.
; Out: exits when IrisRadius <= 0.
; Notes: the second render-slot clearing loop (73h entries) repeats the first (71h entries)
;   with the same value; the second is redundant. IrisCenterX/Y come from PlayerX/Y.
RunIrisClose:
    mov di, word ptr ds:DisplayPage
    mov si, word ptr DrawPage
    call SetLatchCopyMode
    mov ax, 0A000h
    mov es, ax
    mov ds, ax
    mov cx, 1F40h
    rep movsb
    mov ax, seg DATA
    mov ds, ax
    mov di, 4F1Ch
    mov cx, 71h
loc_3191:
    add di, 12h
    mov word ptr [di+4], 0FFFFh
    loop loc_3191
    mov di, 4F1Ch
    mov ax, 0FFFFh
    mov cx, 73h
loc_31A4:
    add di, 12h
    mov word ptr [di+4], ax
    loop loc_31A4
    mov cl, 4
    mov ax, word ptr CameraCol
    shl ax, cl
    sub ax, word ptr PlayerX
    neg ax
    mov word ptr IrisCenterX, ax
    mov ax, word ptr CameraRow
    shl ax, cl
    sub ax, word ptr PlayerY
    neg ax
    sub ax, 10h
    sub al, byte ptr FineScroll
    sbb ah, 0
    mov word ptr IrisCenterY, ax
    shl ax, 1
    mov dx, 0F0h
    cmp ax, dx
    jae loc_31DF
    mov ax, dx
loc_31DF:
    mov word ptr word_2DC4, ax
    mov word ptr IrisRadius, 0E6h
    mov word ptr IrisSpeed, 4
    mov word ptr IrisSpeedCounter, 0
loc_31F4:
    xor si, si
    mov word ptr word_2DCC, 7D0h
    mov word ptr IrisAngleIndex, 0
    xor bp, bp
loc_3204:
    mov bx, word ptr IrisAngleIndex
    mov al, byte ptr [bx+byte_7090]
    mul byte ptr IrisRadius
    mov cl, 6
    sar ax, cl
    add ax, word ptr IrisCenterY
    cmp ax, word ptr word_2DCC
    jge loc_3240
    cmp ax, word ptr word_2DC4
    jge loc_3240
    mov word ptr word_2DCC, ax
    mov word ptr [si+word_6B14], ax
    mov al, byte ptr [bx+SineTable]
    mul byte ptr IrisRadius
    sar ax, cl
    add ax, word ptr IrisCenterX
    mov word ptr [si+word_6A88], ax
    inc si
    inc si
    inc bp
loc_3240:
    inc word ptr IrisAngleIndex
    cmp word ptr ds:IrisAngleIndex, 40h
    jbe loc_3204
    xor si, si
    mov ax, word ptr IrisCenterX
    mov word ptr word_2DCA, ax
    mov ax, word ptr word_2DC4
    mov word ptr word_2DD2, ax
loc_3259:
    mov ax, word ptr [si+word_6B14]
    cmp ax, word ptr word_2DD2
    jne loc_326F
    dec bp
    js loc_32B0
    mov ax, word ptr [si+word_6A88]
    mov word ptr word_2DCA, ax
    inc si
    inc si
loc_326F:
    call SetCpuWriteMode
    mov dx, 3C4h
    mov ax, 0F02h
    out dx, ax
    mov dx, word ptr word_2DD2
    mov bx, word ptr word_2DCA
    mov cx, 140h
    sub cx, bx
    push dx
    call BlackoutSpan
    mov ax, word ptr IrisCenterY
    shl ax, 1
    sub ax, dx
    mov dx, ax
    call BlackoutSpan
    mov ax, word ptr IrisCenterX
    shl ax, 1
    sub ax, bx
    mov cx, ax
    xor bx, bx
    call BlackoutSpan
    pop dx
    call BlackoutSpan
    dec word ptr word_2DD2
    je loc_32B0
    jmp short loc_3259
loc_32B0:
    inc word ptr IrisSpeedCounter
    cmp word ptr ds:IrisSpeedCounter, 14h
    jb loc_32C5
    mov word ptr IrisSpeedCounter, 0
    inc word ptr IrisSpeed
loc_32C5:
    call DrawSpriteObjects
    call FlipDisplayPage
    call WaitVerticalRetrace
    call sub_6772
    mov ax, word ptr IrisSpeed
    sub word ptr IrisRadius, ax
    jle loc_32DD
    jmp near ptr loc_31F4
loc_32DD:
    ret

; Blacks out a horizontal run of pixels in DrawPage (colour 0 in all planes).
; In: BX = x (0..319), CX = width, DX = y (0..199). Returns at once when BX >= 320,
; CX > 320 or DX >= 200. Out: nothing changed except VRAM.
; Notes: the width check is CX <= 320 only; x + CX is not checked, so a run that crosses the
;   right edge is not clipped (VGA writes wrap to the next scanline).
BlackoutSpan:
    push ax
    push bx
    push cx
    push dx
    push di
    cmp bx, 140h
    jae loc_335D
    cmp cx, 140h
    ja loc_335D
    cmp dx, 0C8h
    jae loc_335D
    mov al, 28h
    mul dl
    add ax, word ptr DrawPage
    mov di, bx
    shr di, 1
    shr di, 1
    shr di, 1
    add di, ax
    push cx
    test bl, 7
    jne loc_3312
    cmp cx, 8
    jb loc_3349
loc_3312:
    mov cl, bl
    and cl, 7
    mov dx, 3CEh
    mov ax, 803h
    out dx, ax
    mov al, 0FFh
    shr al, cl
    not al
    pop cx
    xchg byte ptr es:[di], al
    inc di
    mov ax, 5
    mov dx, 3CEh
    out dx, ax
    mov ax, 3
    out dx, ax
    and bx, 7
    add cx, bx
    mov dx, cx
    push cx
    shr cx, 1
    shr cx, 1
    shr cx, 1
    je loc_3349
    dec cx
    xor al, al
    rep stosb
loc_3349:
    pop cx
    and cl, 7
    je loc_335D
    mov dx, 3CEh
    mov ax, 803h
    out dx, ax
    mov al, 0FFh
    shr al, cl
    xchg byte ptr es:[di], al
loc_335D:
    pop di
    pop dx
    pop cx
    pop bx
    pop ax
    ret

; Scrolls the tile view one scanline up (FineScroll decrement, then a tile row when it wraps).
; In: DL = amount subtracted from FineScroll. Out: CF = 0 on success; CF = 1 when CameraRow
;   is already 0 (top edge).
; Effects: on wrap, decrements CameraRow and RowRing (mod 0Ch) and draws the new top tile row
;   with DrawTileRow; increments ScrollAnimCounter.
ScrollViewUp:
    push ax
    push cx
    push dx
    push di
    mov ax, seg DATA
    mov ds, ax
    cmp word ptr ds:CameraRow, 0
    je loc_33A7
    inc byte ptr ScrollAnimCounter
    sub byte ptr FineScroll, dl
    jns loc_33A4
    add byte ptr FineScroll, 10h
    dec word ptr CameraRow
    mov ax, word ptr RowRing
    dec ax
    jns loc_338F
    mov ax, 0Bh
loc_338F:
    xor di, di
    mov word ptr RowRing, ax
    mov ax, word ptr ColumnRing
    call ComputeRingTileOffset
    mov ah, byte ptr CameraRow
    mov al, byte ptr CameraCol
    call DrawTileRow
loc_33A4:
    clc
    jmp short loc_33A8
loc_33A7:
    stc
loc_33A8:
    pop di
    pop dx
    pop cx
    pop ax
    ret

; Scrolls the tile view one scanline down. In: DL = amount added to FineScroll.
; Out: CF = 1 when CameraRow >= MapRows - 0Bh (bottom edge), otherwise CF = 0.
; Effects: on wrap, advances CameraRow and RowRing (mod 0Ch) and draws the new bottom tile row
;   at ring row 11; increments ScrollAnimCounter.
ScrollViewDown:
    push ax
    push cx
    push dx
    push di
    mov ax, seg DATA
    mov ds, ax
    mov al, byte ptr MapRows
    xor ah, ah
    sub ax, 0Bh
    cmp word ptr CameraRow, ax
    jae loc_340E
    inc byte ptr ScrollAnimCounter
    add byte ptr FineScroll, dl
    cmp byte ptr ds:FineScroll, 10h
    jb loc_340B
    sub byte ptr FineScroll, 10h
    inc word ptr CameraRow
    cmp word ptr CameraRow, ax
    jb loc_33E7
    mov byte ptr FineScroll, 0
loc_33E7:
    mov ax, word ptr RowRing
    inc ax
    cmp ax, 0Ch
    jb loc_33F2
    xor ax, ax
loc_33F2:
    mov di, 1B80h
    mov word ptr RowRing, ax
    mov ax, word ptr ColumnRing
    call ComputeRingTileOffset
    mov ah, byte ptr CameraRow
    mov al, byte ptr CameraCol
    add ax, 0B00h
    call DrawTileRow
loc_340B:
    clc
    jmp short loc_340F
loc_340E:
    stc
loc_340F:
    pop di
    pop dx
    pop cx
    pop ax
    ret

; Scrolls the tile view one tile column left. Out: CF = 1 when CameraCol is already 0.
; Effects: decrements CameraCol and ColumnRing (wraps to 13h), draws the new left column
; (ring column 0) with DrawTileColumn.
ScrollViewLeft:
    push ax
    push cx
    push dx
    push di
    mov ax, seg DATA
    mov ds, ax
    xor di, di
    cmp word ptr ds:CameraCol, 0
    je loc_3487
    dec word ptr CameraCol
    mov ax, word ptr ColumnRing
    dec ax
    jns loc_3472
    mov ax, 13h
    jmp short loc_3472

; Scrolls the tile view one tile column right. Out: CF = 1 at the right limit.
; Limit: if CameraLeftLimit >= (PlayerX >> 4) - 14h the limit is CameraLeftLimit, else 0ECh.
; Effects: advances CameraCol and ColumnRing (mod 14h) and draws ring column 19 with
; DrawTileColumn.
ScrollViewRight:
    push ax
    push cx
    push dx
    push di
    mov ax, seg DATA
    mov ds, ax
    mov ax, word ptr PlayerX
    sar ax, 1
    sar ax, 1
    sar ax, 1
    sar ax, 1
    sub ax, 14h
    cmp word ptr CameraLeftLimit, ax
    jge loc_3457
    mov ax, 0ECh
    jmp short loc_345A
loc_3457:
    mov ax, word ptr CameraLeftLimit
loc_345A:
    cmp word ptr CameraCol, ax
    jae loc_3487
    mov di, 13h
    inc word ptr CameraCol
    mov ax, word ptr ColumnRing
    inc ax
    cmp ax, 14h
    jb loc_3472
    xor ax, ax
loc_3472:
    mov word ptr ColumnRing, ax
    call ComputeRingTileOffset
    mov ah, byte ptr CameraRow
    mov al, byte ptr CameraCol
    add ax, di
    call DrawTileColumn
    clc
    jmp short loc_3488
loc_3487:
    stc
loc_3488:
    pop di
    pop dx
    pop cx
    pop ax
    ret

; Draws one 20-tile row (16x16 tiles) of the level map into the tile ring.
; In: AX = map offset of the first tile (row*256 + col; LevelSegment-relative),
;   DI = ring offset relative to RingTileOffset. ES is set to A000h internally.
; Effects: BlitTileCell per tile; BackdropCursor = 7E80h + DI; ORs the per-tile values of
;   AnimTileFlags into AnimVisibleFlag, TileFlagsTable into VisibleTileFlags and TileClassTable
;   into GridDirtyFlag. Sets latch-copy mode; wraps the ring at 5D40h (subtract 1E00h).
DrawTileRow:
    push ax
    push cx
    push dx
    push di
    push si
    push ds
    push es
    push bp
    mov word ptr ds:BackdropCursor, di
    add word ptr BackdropCursor, 7E80h
    push ds
    pop es
    call SetLatchCopyMode
    mov si, ax
    add di, word ptr ds:RingTileOffset
    mov dx, word ptr ColumnRing
    mov ds, word ptr LevelSegment
    mov cx, 14h
loc_34B5:
    cmp di, 5D40h
    jb loc_34BF
    sub di, 1E00h
loc_34BF:
    lodsb
    mov ah, al
    mov bx, 6988h
    xlat byte ptr es:[bx]
    or byte ptr es:[6BBDh], al
    mov al, ah
    mov bx, 805Eh
    xlat byte ptr es:[bx]
    or byte ptr es:[2DF2h], al
    mov al, ah
    mov bx, 4DF8h
    xlat byte ptr es:[bx]
    or byte ptr es:[2DF4h], al
    mov al, ah
    push es
    mov bp, 0A000h
    mov es, bp
    call BlitTileCell
    pop es
    add word ptr es:[2DF6h], 2
    inc dx
    cmp dx, 14h
    jb loc_3501
    sub di, 28h
    xor dx, dx
loc_3501:
    loop loc_34B5
    pop bp
    pop es
    pop ds
    pop si
    pop di
    pop dx
    pop cx
    pop ax
    ret

; Draws one 12-tile column of the level map into the tile ring (the column form of DrawTileRow).
; In: AX = map offset of the top tile (row*256 + col), DI = ring column (0..13h).
; The ring column wraps by 40 bytes when ColumnRing + DI reaches 14h.
DrawTileColumn:
    mov word ptr ds:BackdropCursor, di
    add word ptr BackdropCursor, 7E80h
    push ds
    push es
    push ds
    pop es
    call SetLatchCopyMode
    mov si, ax
    mov ax, di
    shl di, 1
    add di, word ptr ds:RingTileOffset
    add ax, word ptr ColumnRing
    cmp ax, 14h
    jb loc_3533
    sub di, 28h
loc_3533:
    mov ds, word ptr LevelSegment
    mov cx, 0Ch
loc_353A:
    cmp di, 5D40h
    jb loc_3544
    sub di, 1E00h
loc_3544:
    lodsb
    mov ah, al
    mov bx, 6988h
    xlat byte ptr es:[bx]
    or byte ptr es:[6BBDh], al
    mov al, ah
    mov bx, 805Eh
    xlat byte ptr es:[bx]
    or byte ptr es:[2DF2h], al
    mov al, ah
    mov bx, 4DF8h
    xlat byte ptr es:[bx]
    or byte ptr es:[2DF4h], al
    mov al, ah
    push es
    mov dx, 0A000h
    mov es, dx
    call BlitTileCell
    pop es
    add word ptr es:[2DF6h], 40h
    add si, 0FFh
    add di, 27Eh
    loop loc_353A
    pop es
    pop ds
    ret

; Computes the ring address of a tile cell. In: AX = ring column (0..13h), DH = RowRing.
; Out: RingTileOffset = 3F40h + 2*AX + 640*RowRing (640 = 16 scanlines of 40 bytes).
; Clobbers: DX, BX.
ComputeRingTileOffset:
    shl ax, 1
    mov dh, byte ptr RowRing
    xor dl, dl
    mov bx, dx
    shl bx, 1
    shr dx, 1
    add ax, dx
    add ax, bx
    add ax, 3F40h
    mov word ptr RingTileOffset, ax
    ret

; Refreshes the overlay tiles (class >= 1) of the visible 20x12 window in the ring.
; Redraws when ScrollAnimCounter != 0, or when the camera cell changed (PrevCameraCol/Row)
; and GridDirtyFlag is set. Otherwise returns without drawing.
; Out: PrevCameraCol/Row = current camera cell; ScrollAnimCounter and GridDirtyFlag cleared
;   when a redraw happens; VisibleTileFlags cleared.
; Notes: EnterLevelView stores 55AAh into PrevCameraCol to force a compare mismatch.
RedrawOverlayTiles:
    push ax
    push bx
    push cx
    push dx
    push di
    push si
    push ds
    push es
    push bp
    mov ax, seg DATA
    mov ds, ax
    cmp byte ptr ds:ScrollAnimCounter, 0
    jne loc_35DB
    mov ax, word ptr CameraCol
    cmp word ptr PrevCameraCol, ax
    mov word ptr PrevCameraCol, ax
    jne loc_35D1
    mov ax, word ptr CameraRow
    cmp word ptr PrevCameraRow, ax
    mov word ptr PrevCameraRow, ax
    jne loc_35D1
    jmp near ptr loc_365B
loc_35D1:
    cmp byte ptr ds:GridDirtyFlag, 0
    jne loc_35DB
    jmp near ptr loc_365B
loc_35DB:
    mov es, word ptr LevelSegment
    call SetLatchCopyMode
    xor al, al
    mov byte ptr GridDirtyFlag, al
    mov byte ptr ScrollAnimCounter, al
    mov byte ptr VisibleTileFlags, al
    mov ah, byte ptr CameraRow
    mov al, byte ptr CameraCol
    mov si, ax
    mov ch, 0Ch
    xor bh, bh
    mov di, word ptr ds:RingTileOffset
    mov word ptr BackdropCursor, 7E80h
loc_3604:
    mov dx, word ptr ColumnRing
    mov cl, 14h
loc_360A:
    mov bl, byte ptr es:[si]
    mov al, byte ptr [bx-7FA2h]
    or byte ptr VisibleTileFlags, al
    cmp byte ptr [bx+TileClassTable], 1
    jb loc_3632
    mov byte ptr GridDirtyFlag, 1
    mov al, bl
    cmp di, 5D40h
    jb loc_362D
    sub di, 1E00h
loc_362D:
    call BlitTileToVram
    dec di
    dec di
loc_3632:
    inc dx
    cmp dx, 14h
    jb loc_363D
    sub di, 28h
    xor dx, dx
loc_363D:
    add word ptr BackdropCursor, 2
    inc di
    inc di
    inc si
    dec cl
    jne loc_360A
    add word ptr BackdropCursor, 258h
    add di, 280h
    add si, 0ECh
    dec ch
    jne loc_3604
loc_365B:
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
loc_3665:
    jmp near ptr loc_3717

; Animates tiles: advances AnimTickCounter and redraws the visible animated tiles through the
; current phase table (AnimRemapPtr = 6688h, 6788h or 6888h, cycled).
; Gate: AnimVisibleFlag must be set. The phase advances every 4 ticks, or every 2 when
; SnowWind >= 14h (so wind speeds up tile animation).
; Out: AnimRemapPtr advanced; VisibleTileFlags and AnimVisibleFlag recomputed.
AnimateTiles:
    push ax
    push bx
    push cx
    push dx
    push di
    push si
    push ds
    push es
    push bp
    mov ax, seg DATA
    mov ds, ax
    mov es, word ptr LevelSegment
    call SetLatchCopyMode
    cmp byte ptr ds:AnimVisibleFlag, 0
    je loc_3665
    mov al, 3
    cmp word ptr ds:SnowWind, 14h
    jb loc_368F
    mov al, 1
loc_368F:
    inc byte ptr AnimTickCounter
    test byte ptr AnimTickCounter, al
    jne loc_3665
    mov ax, word ptr AnimRemapPtr
    inc ah
    cmp ax, 6988h
    jne loc_36A6
    sub ah, 3
loc_36A6:
    mov word ptr AnimRemapPtr, ax
    mov byte ptr VisibleTileFlags, 0
    mov byte ptr AnimVisibleFlag, 0
    mov ah, byte ptr CameraRow
    mov al, byte ptr CameraCol
    mov si, ax
    mov ch, 0Bh
    xor bh, bh
    mov di, word ptr ds:RingTileOffset
loc_36C4:
    mov dx, word ptr ColumnRing
    mov cl, 14h
loc_36CA:
    mov bl, byte ptr es:[si]
    mov al, byte ptr [bx-7FA2h]
    or byte ptr VisibleTileFlags, al
    cmp byte ptr [bx+AnimTileFlags], 0
    je loc_36F9
    mov byte ptr AnimVisibleFlag, 1
    mov al, bl
    mov bx, word ptr AnimRemapPtr
    xlat
    xor bh, bh
    cmp di, 5D40h
    jb loc_36F4
    sub di, 1E00h
loc_36F4:
    call BlitTileToVram
    dec di
    dec di
loc_36F9:
    inc dx
    cmp dx, 14h
    jb loc_3704
    sub di, 28h
    xor dx, dx
loc_3704:
    inc di
    inc di
    inc si
    dec cl
    jne loc_36CA
    add di, 280h
    add si, 0ECh
    dec ch
    jne loc_36C4
loc_3717:
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

; Redraws foreground tiles (TileFlagsTable bit 40h) over the render slots of the view.
; Gate: command-line flag word (cvar_0003) bit 0002h set (cleared by option A), and
;   VisibleTileFlags bit 40h set. Each render slot with +4 != FFFFh and bit 2000h set in +4
;   is checked (slot loop ends at 5732h). Reads the map through ES = LevelSegment.
; For each foreground map cell next to a slot it calls DrawForegroundTile.
DrawForegroundOverSprites:
    test word ptr cs:OptionBits, 2
    je loc_3731
    test byte ptr VisibleTileFlags, 40h
    jne loc_3732
loc_3731:
    ret
loc_3732:
    mov si, 4F0Ah
    mov es, word ptr LevelSegment
    call SetCpuWriteMode
loc_373C:
    mov bx, word ptr [si+4]
    cmp bx, -1
    jne loc_3747
    jmp near ptr loc_37EA
loc_3747:
    test bh, 20h
    jne loc_374F
    jmp near ptr loc_37EA
loc_374F:
    mov cl, 4
    mov ax, word ptr [si+2]
    sar ax, cl
    dec ax
    xchg al, ah
    mov di, word ptr [si]
    sar di, cl
    add di, ax
    mov cx, di
    sub ch, byte ptr CameraRow
    sub cl, byte ptr CameraCol
    mov dx, 3
    test byte ptr [si+2], 0Fh
    je loc_3779
    add di, 100h
    inc ch
    inc dx
loc_3779:
    cmp ch, 0Bh
    jae loc_37E3
    dec ch
    xor bh, bh
    cmp cl, 2
    jl loc_3799
    mov bl, byte ptr es:[di-2]
    test byte ptr [bx-7FA2h], 40h
    je loc_3799
    mov ax, di
    dec ax
    dec ax
    call DrawForegroundTile
loc_3799:
    cmp cl, 1
    jl loc_37AF
    mov bl, byte ptr es:[di-1]
    test byte ptr [bx-7FA2h], 40h
    je loc_37AF
    mov ax, di
    dec ax
    call DrawForegroundTile
loc_37AF:
    cmp cl, 0
    jl loc_37C8
    cmp cl, 14h
    jge loc_37C8
    mov bl, byte ptr es:[di]
    test byte ptr [bx-7FA2h], 40h
    je loc_37C8
    mov ax, di
    call DrawForegroundTile
loc_37C8:
    cmp cl, 13h
    jge loc_37E3
    cmp cl, 0FFh
    jl loc_37E3
    mov bl, byte ptr es:[di+1]
    test byte ptr [bx-7FA2h], 40h
    je loc_37E3
    mov ax, di
    inc ax
    call DrawForegroundTile
loc_37E3:
    sub di, 100h
    dec dx
    jne loc_3779
loc_37EA:
    add si, 12h
    cmp si, 5732h
    jae loc_37F6
    jmp near ptr loc_373C
loc_37F6:
    ret

; Draws one foreground tile (colour 0 transparent) on DrawPage.
; In: AX = map offset of the cell, BX = tile id. The graphic is taken from the word table at
;   DS:8167h + 2*tile, times 128 bytes, in the segment held in the word at DS:003Bh.
; Out: nothing. Destination: DrawPage + (row-CameraRow)*640 + (col-CameraCol)*2 - 40*FineScroll.
; Notes: AND mask pass (GC function AND, 803h), then per-plane writes; masks are inverted OR of
;   the four planes. The map row/col are split from AX.
DrawForegroundTile:
    push ax
    push bx
    push cx
    push dx
    push di
    push si
    push ds
    push es
    push bp
    shl bx, 1
    mov si, word ptr [bx-7E99h]
    mov cl, 7
    shl si, cl
    sub ah, byte ptr CameraRow
    sub al, byte ptr CameraCol
    mov bl, al
    mov al, ah
    xor ah, ah
    shl ax, 1
    shl ax, 1
    mov dx, 0A0h
    mul dx
    xor bh, bh
    shl bx, 1
    add ax, bx
    add ax, word ptr DrawPage
    mov di, ax
    mov al, 28h
    mul byte ptr FineScroll
    sub di, ax
    mov ax, word ptr DrawPage
    cmp di, ax
    jae loc_383F
    jmp near ptr loc_3918
loc_383F:
    add ax, 1900h
    cmp di, ax
    jb loc_3849
    jmp near ptr loc_3918
loc_3849:
    mov ax, 0A000h
    mov es, ax
    mov ds, word ptr word_003B
    mov dx, 3C4h
    mov ax, 0F02h
    out dx, ax
    mov dl, 0CEh
    mov ax, 803h
    out dx, ax
    mov cx, 10h
loc_3862:
    lodsw
    or ax, word ptr [si+1Eh]
    or ax, word ptr [si+3Eh]
    or ax, word ptr [si+5Eh]
    not ax
    xchg byte ptr es:[di], al
    xchg byte ptr es:[di+1], ah
    add di, 28h
    loop loc_3862
    sub si, 20h
    mov ax, 3
    out dx, ax
    mov dl, 0C4h
    mov al, 2
    out dx, al
    mov dl, 0CEh
    mov al, 4
    out dx, al
    mov bx, 1
loc_388E:
    mov dl, 0C5h
    mov al, bl
    out dx, al
    mov dl, 0CFh
    mov al, bh
    out dx, al
    sub di, 280h
    lodsw
    or word ptr es:[di], ax
    add di, 28h
    lodsw
    or word ptr es:[di], ax
    add di, 28h
    lodsw
    or word ptr es:[di], ax
    add di, 28h
    lodsw
    or word ptr es:[di], ax
    add di, 28h
    lodsw
    or word ptr es:[di], ax
    add di, 28h
    lodsw
    or word ptr es:[di], ax
    add di, 28h
    lodsw
    or word ptr es:[di], ax
    add di, 28h
    lodsw
    or word ptr es:[di], ax
    add di, 28h
    lodsw
    or word ptr es:[di], ax
    add di, 28h
    lodsw
    or word ptr es:[di], ax
    add di, 28h
    lodsw
    or word ptr es:[di], ax
    add di, 28h
    lodsw
    or word ptr es:[di], ax
    add di, 28h
    lodsw
    or word ptr es:[di], ax
    add di, 28h
    lodsw
    or word ptr es:[di], ax
    add di, 28h
    lodsw
    or word ptr es:[di], ax
    add di, 28h
    lodsw
    or word ptr es:[di], ax
    add di, 28h
    inc bh
    shl bl, 1
    cmp bl, 10h
    jae loc_3918
    jmp near ptr loc_388E
loc_3918:
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

; Advances snow wind from the level script and draws the flakes. Called once per frame.
; Every 4th frame (SnowFrameTick low 2 bits = 0): steps the script at ScriptPointer
;   (6-byte entries: word frame time, word wind delta, word wind limit; word -1 ends),
;   using ScriptFrameCounter. SnowWind is clamped to 0 and to the entry's limit.
; Every frame while SnowWind != 0: each FlakeTable word gets +4Fh, and a pixel is plotted
;   into DrawPage with GC function OR (1003h) and map mask 0Fh.
; Notes: SnowWind also sets the tile-animation rate (see AnimateTiles).
UpdateWindAndFlakes:
    inc word ptr ScriptFrameCounter
    test byte ptr SnowFrameTick, 3
    jne loc_3963
    mov ax, word ptr ScriptFrameCounter
    mov bx, word ptr ScriptPointer
    cmp word ptr [bx], -1
    je loc_3963
    cmp word ptr [bx], ax
    jae loc_394E
    mov dx, word ptr [bx+2]
    add word ptr SnowWind, dx
    cmp word ptr [bx+6], ax
    jae loc_394E
    add word ptr ScriptPointer, 6
loc_394E:
    mov ax, word ptr SnowWind
    cmp ax, 0
    jge loc_3958
    xor ax, ax
loc_3958:
    cmp word ptr [bx+4], ax
    jge loc_3960
    mov ax, word ptr [bx+4]
loc_3960:
    mov word ptr SnowWind, ax
loc_3963:
    cmp word ptr ds:SnowWind, 0
    je loc_39DE
    mov ax, 0A000h
    mov es, ax
    mov dx, 3CEh
    mov ax, 1003h
    out dx, ax
    mov ax, 5
    out dx, ax
    mov dl, 0C4h
    mov ax, 0F02h
    out dx, ax
    mov dx, word ptr SnowWind
    mov bp, dx
    shr bp, 1
    mov si, 6CA9h
loc_398B:
    add word ptr [si], 4Fh
    lodsw
    mov bx, ax
    mov cl, al
    rol cl, 1
    and cl, 3
    sub bx, word ptr CameraCol
    and bh, 1Fh
    cmp bx, 1B58h
    jb loc_39AC
    sub bx, 1B58h
    mov word ptr [si-2], bx
loc_39AC:
    add bx, word ptr DrawPage
    mov ax, 302h
    shl ax, cl
    xchg byte ptr es:[bx], al
    mov cx, word ptr SnowWind
    cmp dx, bp
    jbe loc_39C4
    xchg byte ptr es:[bx+28h], ah
loc_39C4:
    dec dx
    jne loc_398B
    call NextWindRandom
    xor ah, ah
    shl ax, 1
    mov bx, ax
    call NextWindRandom
    mov dh, al
    call NextWindRandom
    mov ah, dh
    mov word ptr [bx+FlakeTable], ax
loc_39DE:
    ret

; Returns the next byte from the four-byte mixing generator (WindRandA..C, WindRandWord).
; Out: AL = new WindRandB. Preserves BX CX DX DS.
NextWindRandom:
    push bx
    push cx
    push dx
    push ds
    mov ax, seg DATA
    mov ds, ax
    mov dx, word ptr WindRandWord
    mov al, byte ptr WindRandA
    mov ah, 0
    add dx, ax
    mov word ptr WindRandWord, dx
    mov al, byte ptr WindRandA
    add al, 3
    add al, dh
    mov byte ptr WindRandA, al
    mov bl, byte ptr WindRandB
    add bl, byte ptr WindRandC
    add bl, bl
    add bl, al
    mov byte ptr WindRandB, bl
    mov cl, byte ptr WindRandC
    xor cl, al
    xor cl, bl
    mov byte ptr WindRandC, cl
    mov al, byte ptr WindRandB
    pop ds
    pop dx
    pop cx
    pop bx
    ret
ShakeOffsetBytes label word
; DATA: word variable
    db 00h, 00h

; Copies the play field from the tile ring into DrawPage with fine vertical scroll and shake.
; The copy starts at DrawPage + ShakeOffsetBytes (40*ShakeRows) and reads the ring from
;   RingTileOffset, wrapping horizontally at ColumnRing (two parts) and vertically at the
;   bottom of the ring (second part from 3F40h).
; Rows from the first ring part = min(0C0h - FineScroll - 16*RowRing, 0B0h - ShakeRows);
;   the remaining rows (up to 0B0h - ShakeRows in total) come from the ring top at 3F40h.
; Out: DrawPage holds the scrolled picture; the top ShakeOffsetBytes bytes are zeroed.
CopyRingWindowToPage:
    push bx
    push di
    push si
    push ds
    push es
    mov ax, seg DATA
    mov ds, ax
    mov al, 28h
    mul byte ptr ShakeRows
    mov word ptr cs:ShakeOffsetBytes, ax
    mov si, word ptr RingTileOffset
    mov di, word ptr ds:DrawPage
    mov dl, 14h
    mov dh, byte ptr ColumnRing
    sub dl, dh
    shl dl, 1
    shl dh, 1
    call SetLatchCopyMode
    mov cl, 4
    add di, word ptr cs:ShakeOffsetBytes
    mov bp, 0C0h
    mov al, byte ptr FineScroll
    cbw
    sub bp, ax
    mov ax, word ptr RowRing
    shl ax, cl
    sub bp, ax
    cmp bp, 0B0h
    jb loc_3A7A
    mov bp, 0B0h
    xor bx, bx
    sub bp, word ptr ShakeRows
    jmp short loc_3A89
loc_3A7A:
    mov bx, 0B0h
    sub bx, bp
    sub bx, word ptr ShakeRows
    jns loc_3A89
    add bp, bx
    xor bx, bx
loc_3A89:
    mov al, 28h
    mul byte ptr FineScroll
    add si, ax
    xor ch, ch
    mov ax, 0A000h
    mov es, ax
    mov ds, ax
    mov ax, 28h
    or bp, bp
    jne loc_3AA3
    xchg bx, bp
loc_3AA3:
    mov cl, dl
    rep movsb
    sub si, ax
    mov cl, dh
    rep movsb
    add si, ax
    dec bp
    jne loc_3AA3
    or bx, bx
    je loc_3ACC
    mov si, 3F40h
    mov cl, dh
    add si, cx
loc_3ABD:
    mov cl, dl
    rep movsb
    sub si, ax
    mov cl, dh
    rep movsb
    add si, ax
    dec bx
    jne loc_3ABD
loc_3ACC:
    mov ax, seg DATA
    mov ds, ax
    mov cx, word ptr cs:ShakeOffsetBytes
    shr cx, 1
    je loc_3AEC
    mov di, word ptr ds:DrawPage
    call SetCpuWriteMode
    mov dx, 3C4h
    mov ax, 0F02h
    out dx, ax
    xor ax, ax
    rep stosw
loc_3AEC:
    pop es
    pop ds
    pop si
    pop di
    pop bx
    ret

; Enters the level view: zeroes the camera and scroll state (FineScroll too, via
; ClearStaggeredBlocks), draws the 20x12 ring from the map, then reveals it.
; Sequence: wait while VScrollActive/HScrollDir are set (running sub_5634); if LevelFlags
;   bit 1 (value 2) is clear and PlayerX/16 - CameraCol >= 0Ch, pan right up to ten tiles; set
;   GridDirtyFlag; RedrawOverlayTiles; CopyRingWindowToPage; RevealLevelColumns.
; Notes: the ring fill loop calls BlitTileToVram for all 20x12 cells.
EnterLevelView:
    push ds
    push es
    mov ax, seg DATA
    mov ds, ax
    xor ax, ax
    call ClearStaggeredBlocks
    mov word ptr CameraCol, ax
    mov word ptr CameraRow, ax
    mov word ptr ColumnRing, ax
    mov word ptr RowRing, ax
    call ComputeRingTileOffset
    mov di, word ptr ds:RingTileOffset
    mov es, word ptr LevelSegment
    xor si, si
    call SetLatchCopyMode
    mov ch, 0Ch
loc_3B1C:
    mov cl, 14h
loc_3B1E:
    mov al, byte ptr es:[si]
    call BlitTileToVram
    inc si
    dec cl
    jne loc_3B1E
    add si, 0ECh
    add di, 258h
    dec ch
    jne loc_3B1C
loc_3B35:
    call sub_5634
    mov al, byte ptr VScrollActive
    or al, byte ptr HScrollDir
    jne loc_3B35
    test byte ptr LevelFlags, 2
    jne loc_3B60
    mov ax, word ptr PlayerX
    mov cl, 4
    sar ax, cl
    sub ax, word ptr CameraCol
    cmp ax, 0Ch
    jb loc_3B60
    mov cx, 0Ah
loc_3B5B:
    call ScrollViewRight
    loop loc_3B5B
loc_3B60:
    mov byte ptr GridDirtyFlag, 1
    mov word ptr PrevCameraCol, 55AAh
    call RedrawOverlayTiles
    call CopyRingWindowToPage
    call RevealLevelColumns
    pop es
    pop ds
    ret

; Blits a tile into the tile ring at ring offset DI (adds 3F40h), then BlitTileToVram.
BlitTileToRing:
    add di, 3F40h

; Blits one tile into planar VRAM at offset DI (sets ES = A000h), via BlitTileCell.
BlitTileToVram:
    push dx
    push es
    mov dx, 0A000h
    mov es, dx
    call BlitTileCell
    pop es
    pop dx
    ret

; Blits one 16x16 tile cell from the tile cache to ES:DI (ring or VRAM).
; In: AX = tile id (0..255), ES:DI = destination, DS = DATA.
; Class from TileClassTable[tile]: 0 = opaque (latch copy of the cache slot, 16 rows of 2 bytes);
;   1 = fully transparent (RestoreTileBackdrop only); >= 2 = masked (RestoreTileBackdrop, then
;   the 32-byte AND mask at 2DF8h + 32*(class-2), then each plane OR-ed in).
; Cache slot: VRAM 5E80h + 32*tile, the same offset in each of the four planes.
BlitTileCell:
    push si
    push ds
    push bx
    mov bx, seg DATA
    mov ds, bx
    xor ah, ah
    mov bx, ax
    cmp byte ptr [bx+TileClassTable], 1
    jae loc_3BF6
    shl ax, 1
    shl ax, 1
    shl ax, 1
    shl ax, 1
    shl ax, 1
    mov si, ax
    add si, 5E80h
    push es
    pop ds
    mov bp, 26h
    movsb
    movsb
    add di, bp
    movsb
    movsb
    add di, bp
    movsb
    movsb
    add di, bp
    movsb
    movsb
    add di, bp
    movsb
    movsb
    add di, bp
    movsb
    movsb
    add di, bp
    movsb
    movsb
    add di, bp
    movsb
    movsb
    add di, bp
    movsb
    movsb
    add di, bp
    movsb
    movsb
    add di, bp
    movsb
    movsb
    add di, bp
    movsb
    movsb
    add di, bp
    movsb
    movsb
    add di, bp
    movsb
    movsb
    add di, bp
    movsb
    movsb
    add di, bp
    movsb
    movsb
    sub di, 258h
    pop bx
    pop ds
    pop si
    ret
loc_3BF6:
    mov bl, byte ptr [bx+TileClassTable]
    sub bl, 2
    jae loc_3C06
    call RestoreTileBackdrop
    pop bx
    pop ds
    pop si
    ret
loc_3C06:
    push ds
    call RestoreTileBackdrop
    pop ds
    push cx
    push dx
    push di
    shl bx, 1
    shl bx, 1
    shl bx, 1
    shl bx, 1
    shl bx, 1
    add bx, 2DF8h
    shl ax, 1
    shl ax, 1
    shl ax, 1
    shl ax, 1
    shl ax, 1
    mov si, ax
    add si, 5E80h
    dec di
    dec di
    call SetCpuWriteMode
    mov dx, 3C4h
    mov ax, 0F02h
    out dx, ax
    mov dl, 0CEh
    mov ax, 803h
    out dx, ax
    mov ax, word ptr [bx]
    xchg byte ptr es:[di], al
    xchg byte ptr es:[di+1], ah
    mov ax, word ptr [bx+2]
    xchg byte ptr es:[di+28h], al
    xchg byte ptr es:[di+29h], ah
    mov ax, word ptr [bx+4]
    xchg byte ptr es:[di+50h], al
    xchg byte ptr es:[di+51h], ah
    mov ax, word ptr [bx+6]
    xchg byte ptr es:[di+78h], al
    xchg byte ptr es:[di+79h], ah
    mov ax, word ptr [bx+8]
    xchg byte ptr es:[di+0A0h], al
    xchg byte ptr es:[di+0A1h], ah
    mov ax, word ptr [bx+0Ah]
    xchg byte ptr es:[di+0C8h], al
    xchg byte ptr es:[di+0C9h], ah
    mov ax, word ptr [bx+0Ch]
    xchg byte ptr es:[di+0F0h], al
    xchg byte ptr es:[di+0F1h], ah
    mov ax, word ptr [bx+0Eh]
    xchg byte ptr es:[di+118h], al
    xchg byte ptr es:[di+119h], ah
    mov ax, word ptr [bx+10h]
    xchg byte ptr es:[di+140h], al
    xchg byte ptr es:[di+141h], ah
    mov ax, word ptr [bx+12h]
    xchg byte ptr es:[di+168h], al
    xchg byte ptr es:[di+169h], ah
    mov ax, word ptr [bx+14h]
    xchg byte ptr es:[di+190h], al
    xchg byte ptr es:[di+191h], ah
    mov ax, word ptr [bx+16h]
    xchg byte ptr es:[di+1B8h], al
    xchg byte ptr es:[di+1B9h], ah
    mov ax, word ptr [bx+18h]
    xchg byte ptr es:[di+1E0h], al
    xchg byte ptr es:[di+1E1h], ah
    mov ax, word ptr [bx+1Ah]
    xchg byte ptr es:[di+208h], al
    xchg byte ptr es:[di+209h], ah
    mov ax, word ptr [bx+1Ch]
    xchg byte ptr es:[di+230h], al
    xchg byte ptr es:[di+231h], ah
    mov ax, word ptr [bx+1Eh]
    xchg byte ptr es:[di+258h], al
    xchg byte ptr es:[di+259h], ah
    mov dl, 0CEh
    mov ax, 3
    out dx, ax
    mov al, 4
    out dx, al
    mov dl, 0C4h
    mov al, 2
    out dx, al
    mov cl, 0
    mov ch, 1
    mov al, 1
    push es
    pop ds
loc_3D1A:
    mov dl, 0C5h
    out dx, al
    mov dl, 0CFh
    mov al, cl
    out dx, al
    lodsw
    or word ptr [di], ax
    lodsw
    or word ptr [di+28h], ax
    lodsw
    or word ptr [di+50h], ax
    lodsw
    or word ptr [di+78h], ax
    lodsw
    or word ptr [di+0A0h], ax
    lodsw
    or word ptr [di+0C8h], ax
    lodsw
    or word ptr [di+0F0h], ax
    lodsw
    or word ptr [di+118h], ax
    lodsw
    or word ptr [di+140h], ax
    lodsw
    or word ptr [di+168h], ax
    lodsw
    or word ptr [di+190h], ax
    lodsw
    or word ptr [di+1B8h], ax
    lodsw
    or word ptr [di+1E0h], ax
    lodsw
    or word ptr [di+208h], ax
    lodsw
    or word ptr [di+230h], ax
    lodsw
    or word ptr [di+258h], ax
    sub si, 20h
    inc cl
    shl ch, 1
    mov al, ch
    cmp al, 10h
    jb loc_3D1A
    call SetLatchCopyMode
    pop di
    pop dx
    pop cx
    pop bx
    pop ds
    pop si
    ret

; Restores the backdrop under one 16x16 cell: copies 2 bytes x 16 scanlines from the backdrop
; ring at BackdropCursor - 40*FineScroll (VRAM 7E80h + ring offset) to ES:DI (ring/VRAM).
; In: DI = destination cell, BackdropCursor set by DrawTileRow. Requires latch-copy mode.
; Notes: the destination offset wraps at 5D40h; the source offset is not wrapped.
RestoreTileBackdrop:
    push di
    push ax
    mov al, 28h
    mul byte ptr FineScroll
    mov si, word ptr BackdropCursor
    sub si, ax
    pop ax
    push es
    pop ds
    movsb
    movsb
    add di, 26h
    add si, 26h
    cmp di, 5D40h
    jb loc_3DA7
    sub di, 1E00h
loc_3DA7:
    movsb
    movsb
    add di, 26h
    add si, 26h
    cmp di, 5D40h
    jb loc_3DB9
    sub di, 1E00h
loc_3DB9:
    movsb
    movsb
    add di, 26h
    add si, 26h
    cmp di, 5D40h
    jb loc_3DCB
    sub di, 1E00h
loc_3DCB:
    movsb
    movsb
    add di, 26h
    add si, 26h
    cmp di, 5D40h
    jb loc_3DDD
    sub di, 1E00h
loc_3DDD:
    movsb
    movsb
    add di, 26h
    add si, 26h
    cmp di, 5D40h
    jb loc_3DEF
    sub di, 1E00h
loc_3DEF:
    movsb
    movsb
    add di, 26h
    add si, 26h
    cmp di, 5D40h
    jb loc_3E01
    sub di, 1E00h
loc_3E01:
    movsb
    movsb
    add di, 26h
    add si, 26h
    cmp di, 5D40h
    jb loc_3E13
    sub di, 1E00h
loc_3E13:
    movsb
    movsb
    add di, 26h
    add si, 26h
    cmp di, 5D40h
    jb loc_3E25
    sub di, 1E00h
loc_3E25:
    movsb
    movsb
    add di, 26h
    add si, 26h
    cmp di, 5D40h
    jb loc_3E37
    sub di, 1E00h
loc_3E37:
    movsb
    movsb
    add di, 26h
    add si, 26h
    cmp di, 5D40h
    jb loc_3E49
    sub di, 1E00h
loc_3E49:
    movsb
    movsb
    add di, 26h
    add si, 26h
    cmp di, 5D40h
    jb loc_3E5B
    sub di, 1E00h
loc_3E5B:
    movsb
    movsb
    add di, 26h
    add si, 26h
    cmp di, 5D40h
    jb loc_3E6D
    sub di, 1E00h
loc_3E6D:
    movsb
    movsb
    add di, 26h
    add si, 26h
    cmp di, 5D40h
    jb loc_3E7F
    sub di, 1E00h
loc_3E7F:
    movsb
    movsb
    add di, 26h
    add si, 26h
    cmp di, 5D40h
    jb loc_3E91
    sub di, 1E00h
loc_3E91:
    movsb
    movsb
    add di, 26h
    add si, 26h
    cmp di, 5D40h
    jb loc_3EA3
    sub di, 1E00h
loc_3EA3:
    movsb
    movsb
    sub di, 258h
    pop di
    inc di
    inc di
    ret

; Applies the secret-tile patches of the level: for each record of the 80-entry table at 8C8Dh
; (5 bytes: +0 new tile, +1 old tile (saved here), +3 word LevelSegment offset or -1), writes the
; new tile at that offset and saves the old one. Adds the number of applied patches to
; CollectTotalMain.
ApplySecretTilePatches:
    push es
    mov es, word ptr LevelSegment
    mov si, 8C8Dh
    mov cx, 50h
    xor dx, dx
loc_3EBA:
    mov bx, word ptr [si+3]
    cmp bx, -1
    je loc_3ECB
    inc dx
    mov al, byte ptr [si]
    xchg byte ptr es:[bx], al
    mov byte ptr [si+1], al
loc_3ECB:
    add si, 5
    loop loc_3EBA
    add word ptr CollectTotalMain, dx
    pop es
    ret

; Loads level AL and prepares its structures (see report). Called once per level.
; In: AL = level index (stored in CurrentLevel).
; Out: LevelSegment, SpriteSheetSegment, MapRows, tile cache, backdrop, render slots, effect,
;   terrain, trigger and secret tables, and the 815Eh..9202h state block set up.
; Steps:
;   1. Totals: levels 0..0Ah zero CollectTotalDecor, CollectTotalMain, and the words 2A7A and
;      2A76. Above 0Ah only CollectTotalDecor and CollectTotalMain are halved (shr).
;   2. LevelFileChar = '1'+level ('9' then 'A'); BackFileChar = byte_2D96[level].
;   3. Fills 7E5Eh..9202h with 0FFh and render slots 1..115 (4F1Ch..5731h) with FFFFh.
;   4. LoadSqzFile("LEVELn.SQZ") -> LevelSegment; LoadLevelTileSet (MapRows, tile cache).
;   5. Copies 5029 bytes (13A5h) from the variant block after the tile data to 7E5Eh..9202h.
;   6. Stashes LevelSegment bytes into VRAM 9DC0h (see notes), then copies plane 3 back.
;   7. LoadSqzFile("BACKn.SQZ") (four planes of 8000 bytes) into the backdrop at VRAM 7E80h.
;   8. Sprite sheet via sub_0492 (SpriteSheetSegment); UploadSpriteTiles; BuildAnimatedTileTables;
;      ClassifyTileCache; ScriptPointer = LevelScriptTable[level]; ScriptFrameCounter = 0;
;      CheckpointX/Y = LevelStartX/Y.
;   9. ScrollEffectSources, RebaseEntitySpriteRefs, ApplySecretTilePatches, state snapshot
;      815Eh..9202h -> 9203h, SavePristineTriggerRows, BuildDecorList, then CollectTotalDecor
;      += effect sources with X in 6Eh..75h or 80h..0DBh. Levels 0..9 with byte 2CF6[level] !=
;      0FFh double CollectTotalMain and CollectTotalDecor.
; Notes: see the three notes below and the report's data layout for 7E5Eh..9202h.
LoadLevel:
; quirk: for levels above 0Ah, CollectTotalMain and CollectTotalDecor are halved (shr) and the
;   words 2A76/2A7A are kept; levels 0..0Ah zero all four.
    push ds
    mov dx, seg DATA
    mov ds, dx
    mov es, dx
    mov byte ptr CurrentLevel, al
    xor bx, bx
    cmp al, 0Ah
    ja loc_3EF9
    mov word ptr CollectTotalDecor, bx
    mov word ptr CollectTotalMain, bx
    mov word ptr word_2A7A, bx
    mov word ptr word_2A76, bx
    jmp short loc_3F01
loc_3EF9:
    shr word ptr CollectTotalDecor, 1
    shr word ptr CollectTotalMain, 1
loc_3F01:
    mov bl, al
    mov bl, byte ptr [bx+byte_2D96]
    mov byte ptr BackFileChar, bl
    add al, 31h
    cmp al, 39h
    jbe loc_3F13
    add al, 7
loc_3F13:
    mov byte ptr LevelFileChar, al
    mov di, 7E5Eh
    mov cx, 13A5h
    mov al, 0FFh
    rep stosb
    mov di, 4F1Ch
    mov cx, 40Bh
    rep stosw
    call SetCpuWriteMode
    mov dx, 2D8Bh
    call LoadSqzFile
    mov word ptr LevelSegment, ax
    call LoadLevelTileSet
    mov ax, seg DATA
    mov es, ax
    mov di, 7E5Eh
    mov cx, 180h
    rep movsw
    movsw
    movsw
    movsw
    movsw
    movsb
    mov cx, 100h
    rep movsw
    mov cx, 46h
    rep movsw
    mov cx, 4Bh
    rep movsw
    mov cx, 400h
    rep movsw
    movsw
    movsw
    mov cx, 190h
    rep movsb
    mov cx, 100h
    rep movsb
    mov cx, 1EAh
    rep movsb
    mov cx, 0F0h
    rep movsb
    movsw
    movsw
    mov cx, 4
    rep movsw
    push ax
    push bx
    push cx
    push dx
    push di
    push si
    push ds
    push es
    push bp
    mov ax, seg DATA
    mov ds, ax
    mov ax, 0A000h
    mov es, ax
    mov ds, word ptr LevelSegment
    xor si, si
    call SetCpuWriteMode
    mov dx, 3C4h
    mov ax, 102h
loc_3F9C:
; quirk: the stash of LevelSegment into VRAM 9DC0h runs four passes (map mask 1,2,4,8) on one
;   source pointer, so plane p receives the bytes after plane p-1 (the source wraps at 64 KB).
    mov di, 9DC0h
    out dx, ax
    mov cx, 3120h
    rep movsw
    shl ah, 1
    cmp ah, 10h
    jne loc_3F9C
    mov ax, seg DATA
    mov ds, ax
    push word ptr NextFreeSegment
    mov ax, word ptr LevelSegment
    mov word ptr NextFreeSegment, ax
    mov cx, 7
    call PageInResource
    mov word ptr SpriteSheetSegment, ax
    mov word ptr NextFreeSegment, ax
    call UploadSpriteTiles
    mov ax, seg DATA
    mov ds, ax
    pop word ptr NextFreeSegment
    mov ax, seg DATA
    mov ds, ax
    mov es, word ptr LevelSegment
    xor di, di
    mov ax, 0A000h
    mov ds, ax
    mov dx, 3CEh
    mov ax, 4
loc_3FE9:
; quirk: the four read-map passes all write LevelSegment:0 with the same length, so only the
;   last pass (read map 3) survives. Confirm the intent before translation.
    out dx, ax
    mov cx, 3120h
    mov si, 9DC0h
    rep movsw
    inc ah
    cmp ah, 3
    jbe loc_3FE9
    call SetCpuWriteMode
    push word ptr ds:[2875h]
    mov dx, 2DA6h
    call LoadSqzFile
    mov ds, ax
    mov ax, 0A000h
    mov es, ax
    mov dx, 3C4h
    mov ax, 102h
    xor si, si
loc_4015:
    out dx, ax
    mov di, 7E80h
    mov cx, 0FA0h
    rep movsw
    shl ah, 1
    cmp ah, 10h
    jb loc_4015
    pop word ptr ds:[2875h]
    pop bp
    pop es
    pop ds
    pop si
    pop di
    pop dx
    pop cx
    pop bx
    pop ax
    call BuildAnimatedTileTables
    call ClassifyTileCache
    mov bl, byte ptr CurrentLevel
    xor bh, bh
    shl bx, 1
    mov ax, word ptr [bx+LevelScriptTable]
    mov word ptr ScriptPointer, ax
    mov word ptr ScriptFrameCounter, 0
    mov ax, word ptr LevelStartX
    mov word ptr CheckpointX, ax
    mov ax, word ptr LevelStartY
    mov word ptr CheckpointY, ax
    call ScrollEffectSources
    call RebaseEntitySpriteRefs
    call ApplySecretTilePatches
    call BuildDecorList
    mov di, 9203h
    mov si, 815Eh
    mov cx, 10A5h
    rep movsb
    call SavePristineTriggerRows
    mov si, 8F1Dh
    xor dx, dx
    mov cx, 46h
loc_407B:
    mov ax, word ptr [si+4]
    cmp ax, 0FFFFh
    je loc_4098
    cmp ax, 6Eh
    jb loc_4098
    cmp ax, 0DBh
    ja loc_4098
    cmp ax, 80h
    jae loc_4097
    cmp ax, 75h
    ja loc_4098
loc_4097:
    inc dx
loc_4098:
    add si, 7
    loop loc_407B
    add word ptr CollectTotalDecor, dx
    mov bl, byte ptr CurrentLevel
    xor bh, bh
    cmp bl, 0Ah
    jae loc_40BB
    cmp byte ptr [bx+byte_2CF6], 0FFh
    je loc_40BB
    shl word ptr CollectTotalDecor, 1
    shl word ptr CollectTotalMain, 1
loc_40BB:
    pop ds
    ret

; Builds the decor list: pointers into DecorPtrList (6A88h) to effect sources whose word +4,
; masked with 1FFFh, lies in 11Bh..12Ah. Bubble-sorts the pointers by each target's word +0,
; then sets +4 of each group of four to 11Bh + a nibble of the value from sub_932F
; (CurrentLevel, plus 0Ah when byte B198 is set).
; Notes: the whole routine does nothing when the count is 0 or not a multiple of 4.
; Notes: 6A88h is reused as a table of iris edges by RunIrisClose.
BuildDecorList:
    mov si, 8F1Dh
    mov di, 6A88h
    mov cx, 46h
    xor bp, bp
loc_40C8:
    mov ax, word ptr [si+4]
    and ah, 1Fh
    sub ax, 11Bh
    cmp ax, 0Fh
    ja loc_40DB
    mov word ptr [di], si
    inc di
    inc di
    inc bp
loc_40DB:
    add si, 7
    loop loc_40C8
    mov ax, bp
    or ax, ax
    je loc_414C
    and ax, 3
    or ax, ax
    jne loc_414C
loc_40ED:
    mov si, 6A88h
    mov cx, bp
    xor dx, dx
    jmp short loc_4109
loc_40F6:
    mov bx, word ptr [si]
    mov ax, word ptr [bx]
    mov bx, word ptr [si+2]
    cmp word ptr [bx], ax
    jae loc_4107
    xchg word ptr [si], bx
    mov word ptr [si+2], bx
    inc dx
loc_4107:
    inc si
    inc si
loc_4109:
    loop loc_40F6
    or dx, dx
    jne loc_40ED
    mov al, byte ptr CurrentLevel
    cmp byte ptr ds:byte_B198, 0
    je loc_411B
    add al, 0Ah
loc_411B:
    cbw
    call ComputePasswordCode
    mov si, 6A88h
loc_4122:
    mov dx, ax
    mov cx, 4
loc_4127:
    mov di, word ptr [si]
    inc si
    inc si
    shl dx, 1
    rcl bx, 1
    shl dx, 1
    rcl bx, 1
    shl dx, 1
    rcl bx, 1
    shl dx, 1
    rcl bx, 1
    and bx, 0Fh
    add bx, 11Bh
    mov word ptr [di+4], bx
    loop loc_4127
    sub bp, 4
    jne loc_4122
loc_414C:
    ret

; Shifts the effect sources (70 records from 8F1Dh, stride 7) and terrain entities (16 records
; from 9107h, stride 0Fh) by the bank rebase: X = X + 35h - SpriteBankA, skipping X = FFFFh.
ScrollEffectSources:
    mov dx, word ptr SpriteBankA
    mov si, 8F1Dh
    mov cx, 46h
loc_4157:
    cmp word ptr [si+4], -1
    je loc_4164
    sub word ptr [si+4], dx
    add word ptr [si+4], 35h
loc_4164:
    add si, 7
    loop loc_4157
    mov si, 9107h
    mov cx, 10h
loc_416F:
    cmp word ptr [si+4], -1
    je loc_417C
    sub word ptr [si+4], dx
    add word ptr [si+4], 35h
loc_417C:
    add si, 0Fh
    loop loc_416F
    ret

; Remaps the entity arena (variable-stride records from 8489h: byte 0 = stride, word +2 = X)
; between the two sprite bank windows. A = SpriteBankA, B = SpriteBankB: X >= B becomes
; X - B + 138h; A <= X < B becomes X - A + 35h; X < A is unchanged. X = FFFFh is skipped.
; Out: SpriteBankA = 35h, SpriteBankB = 138h.
; Notes: the loop ends when the stride byte is above 32h (unsigned compare).
RebaseEntitySpriteRefs:
    mov dx, word ptr SpriteBankA
    mov bx, word ptr SpriteBankB
    cmp dx, -1
    je loc_41BD
    mov si, 8489h
loc_4192:
    cmp byte ptr [si], 32h
    ja loc_41BD
    mov ax, word ptr [si+2]
    cmp ax, 0FFFFh
    je loc_41B6
    cmp ax, bx
    jb loc_41AA
    sub ax, bx
    add ax, 138h
    jmp short loc_41B3
loc_41AA:
    cmp ax, dx
    jb loc_41B6
    sub ax, dx
    add ax, 35h
loc_41B3:
    mov word ptr [si+2], ax
loc_41B6:
    mov al, byte ptr [si]
    cbw
    add si, ax
    jmp short loc_4192
loc_41BD:
    mov word ptr SpriteBankA, 35h
    mov word ptr SpriteBankB, 138h
    ret

; Saves the original map cells of the proximity triggers (15 records, stride 0Ah from 83F3h;
; records whose first word is -1 are skipped) into the pristine bank at NextFreeSegment.
; Each saved record: word map offset, byte rows (DL), byte width (DH), then the cell bytes;
; the bank is terminated by a word FFFFh. Each trigger's +6 word = its bank offset.
; Notes: rows = trigger +3 + trigger +8; the start row is moved up by trigger +8 rows.
SavePristineTriggerRows:
    mov es, word ptr NextFreeSegment
    xor di, di
    mov word ptr es:[di], 0FFFFh
    mov bx, 83F3h
    mov cx, 0Fh
loc_41DB:
    cmp word ptr [bx], -1
    jne loc_41E6
loc_41E0:
    add bx, 0Ah
    loop loc_41DB
    ret
loc_41E6:
    mov si, word ptr [bx]
    mov ah, byte ptr [bx+8]
    xor al, al
    sub si, ax
    mov dl, byte ptr [bx+3]
    add dl, byte ptr [bx+8]
    mov dh, byte ptr [bx+2]
    mov es, word ptr NextFreeSegment
    mov ds, word ptr LevelSegment
    mov word ptr es:[di], si
    mov word ptr es:[di+2], dx
    add di, 4
    mov al, dh
loc_420C:
    movsb
    dec al
    jne loc_420C
    mov al, dh
    xor ah, ah
    sub si, ax
    add si, 100h
    dec dl
    jne loc_420C
    mov dx, seg DATA
    mov ds, dx
    neg ax
    add ax, di
    mov word ptr [bx+6], ax
    mov word ptr es:[di], 0FFFFh
    jmp short loc_41E0

; Classifies the 256 tile cache slots (VRAM 5E80h + 32*tile), read with GC colour compare
; (colour 0). TileClassTable[tile] = 0 when no pixel is colour 0 (opaque); 1 when every pixel
; is colour 0 (fully transparent); otherwise a masked class 2, 3, ... (MixedTileCount) with its
; 32-byte mask copied to 2DF8h + 32*(class-2). GridDirtyFlag = 1 when any class-1 or masked
; tile exists. PrevCameraCol = 0AA55h.
ClassifyTileCache:
    push ds
    push es
    mov ax, seg DATA
    mov ds, ax
    mov ax, 0A000h
    mov es, ax
    mov dx, 3CEh
    mov ax, 0F07h
    out dx, ax
    mov ax, 2
    out dx, ax
    mov ax, 805h
    out dx, ax
    xor bx, bx
    mov si, 5E80h
    mov byte ptr MixedTileCount, 1
    mov di, 2DF8h
loc_425A:
    mov cx, 20h
    mov dx, 0FFh
loc_4260:
    mov al, byte ptr es:[si]
    inc si
    mov byte ptr [di], al
    inc di
    and dl, al
    or dh, al
    loop loc_4260
    cmp dh, 0
    jne loc_427B
    mov byte ptr [bx+TileClassTable], dh
    sub di, 20h
    jmp short loc_429F
loc_427B:
    cmp dl, 0FFh
    jne loc_428F
    mov byte ptr [bx+TileClassTable], 1
    mov byte ptr GridDirtyFlag, 1
    sub di, 20h
    jmp short loc_429F
loc_428F:
    inc byte ptr MixedTileCount
    mov al, byte ptr MixedTileCount
    mov byte ptr [bx+TileClassTable], al
    mov byte ptr GridDirtyFlag, 1
loc_429F:
    inc bx
    cmp bx, 100h
    jne loc_425A
    mov word ptr PrevCameraCol, 0AA55h
    pop es
    pop ds
    ret

; Builds the animation phase tables. For each tile id: if TileFlagsTable[tile] bit 80h is set,
; the tile starts an animated triple (t, t+1, t+2). Phase 0 maps t->t, t+1->t+1, t+2->t+2;
; phase 1 cycles them forward and phase 2 backward, in the tables at 6688h, 6788h, 6888h.
; AnimTileFlags[t] = 1 for the triple's first tile only. Other tiles map to themselves, with
; AnimTileFlags 0. Out: AnimVisibleFlag = 1 when any animated tile exists.
; Notes: the middle and last members of a triple keep AnimTileFlags 0 (not redrawn).
BuildAnimatedTileTables:
    mov ax, seg DATA
    mov ds, ax
    xor bx, bx
    mov cx, 100h
    xor dl, dl
loc_42BB:
    test byte ptr [bx-7FA2h], 80h
    jne loc_42D5
    mov byte ptr [bx+byte_6688], bl
    mov byte ptr [bx+byte_6788], bl
    mov byte ptr [bx+byte_6888], bl
    mov byte ptr [bx+AnimTileFlags], 0
    jmp short loc_430C
loc_42D5:
    mov dl, 1
    mov al, bl
    mov byte ptr [bx+byte_6688], al
    mov byte ptr [bx+byte_678A], al
    mov byte ptr [bx+byte_6889], al
    inc ax
    mov byte ptr [bx+byte_6689], al
    mov byte ptr [bx+byte_6788], al
    mov byte ptr [bx+byte_688A], al
    inc ax
    mov byte ptr [bx+byte_668A], al
    mov byte ptr [bx+byte_6789], al
    mov byte ptr [bx+byte_6888], al
    mov byte ptr [bx+AnimTileFlags], 1
    mov word ptr [bx+word_6989], 101h
    inc bx
    inc bx
loc_430C:
    inc bx
    or bh, bh
    je loc_42BB
    mov byte ptr AnimVisibleFlag, dl
    ret

; Loads the level tile set from LevelSegment. In: CurrentLevel, LevelSegment.
; Out: MapRows = LevelMapRowsTable[level]; NextFreeSegment = LevelSegment + 256*MapRows;
;   TileIndexTable (25CEh, 256 words) = the variant's first 512 bytes; tile cache slots
;   VRAM 5E80h + 32*k for each k whose entry is below 100h (four planes, 32 bytes each, from
;   variant + 200h + 128*entry); SI = variant offset after the used tile data.
; Notes: entries at or above 100h are filled later by UploadSpriteTiles.
LoadLevelTileSet:
    mov ax, word ptr LevelSegment
    mov bl, byte ptr CurrentLevel
    mov bh, byte ptr [bx+LevelMapRowsTable]
    mov byte ptr ds:MapRows, bh
    xor bl, bl
    mov cl, 4
    shr bx, cl
    add ax, bx
    mov word ptr NextFreeSegment, ax
    mov dx, ds
    mov es, dx
    mov ds, ax
    xor si, si
    mov di, 25CEh
    mov cx, 100h
    rep movsw
    mov ax, 0A000h
    mov es, ax
    mov di, 5E80h
    mov dx, 3C4h
    mov al, 2
    out dx, al
    inc dx
    xor bx, bx
    mov bp, 200h
loc_4354:
    push ds
    mov si, word ptr [bx]
    cmp si, 100h
    jae loc_437A
    mov cl, 7
    add bp, 80h
    mov al, 1
    shl si, cl
    add si, 200h
loc_436B:
    out dx, al
    mov cx, 10h
    rep movsw
    sub di, 20h
    shl al, 1
    cmp al, 10h
    jb loc_436B
loc_437A:
    inc bx
    inc bx
    add di, 20h
    pop ds
    cmp bx, 200h
    jb loc_4354
    mov si, bp
    ret

; Uploads sprite-sheet tiles into the tile cache: for each TileIndexTable word w >= 100h,
; copies sprite-sheet graphic (w - 100h) (SpriteSheetSegment + 8 paragraphs each) into the
; cache slot of that entry (VRAM 5E80h + 32*k, four planes).
UploadSpriteTiles:
    mov ax, 0A000h
    mov es, ax
    mov ax, seg DATA
    mov ds, ax
    mov di, 5E80h
    mov dx, 3C4h
    mov al, 2
    out dx, al
    inc dx
    mov bx, 25CEh
loc_43A0:
    push ds
    mov ax, word ptr [bx]
    sub ax, 100h
    jb loc_43C5
    mov cl, 3
    shl ax, cl
    add ax, word ptr SpriteSheetSegment
    mov ds, ax
    xor si, si
    mov al, 1
loc_43B6:
    out dx, al
    mov cx, 10h
    rep movsw
    sub di, 20h
    shl al, 1
    cmp al, 10h
    jb loc_43B6
loc_43C5:
    inc bx
    inc bx
    add di, 20h
    pop ds
    cmp bx, 27CEh
    jb loc_43A0
    ret

; Checks the adapter for VGA at startup. When cvar_0001 is 0FFh (not yet probed): sets mode 0Dh,
; loads DAC registers 40h..49h from DS:2A7Ch (10 colours), reads them back, then calls
; ProbeVideoMemory. On success cvar_0001 = 5 and CF = 0. On failure it prints the "FATAL" string
; at 2A3Ah with INT 21h/09h, sets text mode (INT 10h AX=0003h) and jumps to loc_104F.
; Notes: cvar_0001 is set to 1 by command-line option V (probe skipped) and to 5 by a good probe;
;   WaitVerticalRetrace uses 3DAh for any nonzero value.
; Notes: the caller runs SetLevelVideoMode afterwards in every case.
CheckVgaAdapter:
    cmp byte ptr cs:OptionVByte, 0FFh
    je loc_43DD
    jmp near ptr SetLevelVideoMode
loc_43DD:
    mov ax, seg DATA
    mov ds, ax
    mov ax, 0Dh
    int 10h
    mov dx, 3C8h
    mov al, 40h
    out dx, al
    inc dx
    mov cx, 1Eh
    mov si, 2A7Ch
loc_43F4:
    lodsb
    and al, 3Fh
    out dx, al
    loop loc_43F4
    mov cx, 1Eh
    sub si, cx
    mov al, 40h
    mov dl, 0C7h
    out dx, al
    mov dl, 0C9h
loc_4406:
    mov ah, byte ptr [si]
    inc si
    and ah, 3Fh
    in al, dx
    cmp al, ah
    jne loc_441F
    loop loc_4406
    call ProbeVideoMemory
    jb loc_441F
    mov byte ptr cs:OptionVByte, 5
    ret
loc_441F:
    mov word ptr NextFreeSegment, 0
    push cs
    pop es
    mov di, 26FAh
    mov cx, 3E8h
    rep stosw
    mov ax, 3
    int 10h
    mov ah, 9
    mov dx, 2A3Ah
    int 21h
    jmp near ptr HaltWithMessage

; Unused: a bare RET with no callers.
unused_443E:
    ret

; Probes video memory: sets mode 0Dh, writes 01h to A000:2000h and reads it back; then with
; GC rotate 1 and function OR writes A0h there and expects 51h. Out: CF = 0 on success.
; Notes: overwrites DrawPage 1 (offset 2000h) and leaves GC data rotate/function at 11h.
ProbeVideoMemory:
    mov ax, 0Dh
    int 10h
    mov ax, 0A000h
    mov es, ax
    mov dx, 3C4h
    mov ax, 102h
    out dx, ax
    mov dl, 0CEh
    mov ax, 5
    out dx, ax
    mov ax, 4
    out dx, ax
    mov al, 1
    mov byte ptr es:[2000h], al
    cmp byte ptr es:[2000h], al
    jne loc_447B
    mov ax, 1103h
    out dx, ax
    mov byte ptr es:[2000h], 0A0h
    cmp byte ptr es:[2000h], 51h
    jne loc_447B
    clc
    ret
loc_447B:
    stc
    ret

; Sets the level video mode: INT 10h AX=0Dh (320x200, 16 colours); CRTC 11h = 24h (not
; write-protected, vertical interrupt disabled); DisplayPage = 0, DrawPage = 2000h; the 17-byte
; palette/overscan table at DS:2A63h (INT 10h AX=1002h); the 16-entry DAC block at
; LevelPaletteTable[CurrentLevel] (INT 10h AX=1012h, BX=0, CX=16).
; Notes: called at startup after CheckVgaAdapter and from several level paths.
SetLevelVideoMode:
    push ax
    push bx
    push cx
    push dx
    push di
    push si
    push ds
    push es
    push bp
    mov ax, seg DATA
    mov ds, ax
    mov es, ax
    mov ax, 0Dh
    int 10h
    mov dx, 3D4h
    mov ax, 2411h
    out dx, ax
    mov word ptr DisplayPage, 0
    mov word ptr DrawPage, 2000h
    mov dx, 2A63h
    mov ax, 1002h
    int 10h
    mov bl, byte ptr CurrentLevel
    xor bh, bh
    shl bx, 1
    mov dx, word ptr [bx+LevelPaletteTable]
    mov ax, 1012h
    xor bx, bx
    mov cx, 10h
    int 10h
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

; Waits for the start of a vertical retrace: with cvar_0001 != 0 it waits for bit 3 of 3DAh to
; clear, then to set. With cvar_0001 = 0 (monochrome) it uses 3BAh bit 0 instead.
; Preserves AX and DX.
WaitVerticalRetrace:
    push ax
    push dx
    cmp byte ptr cs:OptionVByte, 0
    je loc_44E9
    mov dx, 3DAh
    mov ah, 8
loc_44DC:
    in al, dx
    test al, ah
    jne loc_44DC
loc_44E1:
    in al, dx
    test al, ah
    je loc_44E1
    pop dx
    pop ax
    ret
loc_44E9:
    mov dx, 3BAh
    mov ah, 1
loc_44EE:
    in al, dx
    test al, ah
    jne loc_44EE
loc_44F3:
    in al, dx
    test al, ah
    je loc_44F3
    pop dx
    pop ax
    ret

; Presents the drawn frame and waits for the frame timer: calls FlipDisplayPage, then sub_1C65
; (the frame timing routine, which also clears byte_6BE7).
; Notes: the test of byte_6BE7 before the call has identical branches (no effect).
PresentFrame:
    cmp byte ptr ds:byte_6BE7, 0
    je loc_4502
loc_4502:
    call FlipDisplayPage
    call WaitSoundTicks
    ret

; Shows DrawPage on screen and swaps the page variables. CRTC 0Ch = high byte of the old
; DrawPage (start address in 256-byte steps); then DisplayPage := old DrawPage and
; DrawPage := old DisplayPage.
; Notes: only index 0Ch is written; the low start byte (index 0Dh) is not touched.
FlipDisplayPage:
    push ax
    push bx
    push dx
    push ds
    mov dx, 3D4h
    mov ax, seg DATA
    mov ds, ax
    mov bx, word ptr DrawPage
    mov al, 0Ch
    mov ah, bh
    out dx, ax
    xchg word ptr DisplayPage, bx
    mov word ptr DrawPage, bx
    pop ds
    pop dx
    pop bx
    pop ax
    ret

; Sets graphics controller write mode 0 and replace function (CPU writes): GC 05h = 00h,
; GC 03h = 00h. Preserves all registers.
SetCpuWriteMode:
    push ax
    push dx
    mov dx, 3CEh
    mov ax, 5
    out dx, ax
    mov ax, 3
    out dx, ax
    pop dx
    pop ax
    ret

; Sets graphics controller write mode 1 (latch copy: MOVSB/XCHG VRAM-to-VRAM copies all four
; latched planes) and map mask 0Fh. GC 05h = 01h, SEQ 02h = 0Fh.
; Notes: SetCpuWriteMode undoes this. Callers rely on it before MOVSB or XCHG in VRAM.
SetLatchCopyMode:
    push ax
    push dx
    mov dx, 3CEh
    mov ax, 105h
    out dx, ax
    mov dl, 0C4h
    mov ax, 0F02h
    out dx, ax
    pop dx
    pop ax
    ret
CODE ends
DATA segment word public 'DATA'
DATA ends
end
