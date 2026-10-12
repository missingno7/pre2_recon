; Prehistorik 2 (PRE2.EXE), link module 1 of 4
STK segment para stack 'STACK'
    db 200h dup(0)
STK ends
; module interface (tools/symtab.py)
public ActivateResourceSlot
public AnimRemapPtr
public AnimTickCounter
public AnimTileFlags
public AnimVisibleFlag
public BackFileChar
public BackdropCursor
public BiosSeed
public BiosSeedReady
public CameraCol
public CameraLeftLimit
public CameraRow
public CheckpointX
public CheckpointY
public ClockYear
public CodeEntryState
public CodeInputCount
public CollectTotalDecor
public CollectTotalMain
public ColumnRing
public CpuClass
public CurrentLevel
public CurrentObject
public DisplayPage
public DrawPage
public EffectFrame
public EffectOriginX
public EffectOriginY
public ExpandDstSegment
public ExpandSrcSegment
public ExpertFlag
public FineScroll
public FlakeTable
public FollowerOverlapMode
public FrameCallback
public FrameDataOffsets
public FrameDataSegments
public FrameHeights
public FrameHotX
public FrameHotY
public FrameWidths
public GridDirtyFlag
public HScrollDir
public HaltWithMessage
public IrisAngleIndex
public IrisCenterX
public IrisCenterY
public IrisRadius
public IrisSpeed
public IrisSpeedCounter
public KeyDownTable
public LandingDepth
public LandingFlag
public LastDebrisSlot
public LastEffectTick
public LastScanCode
public LevelFileChar
public LevelFlags
public LevelMapRowsTable
public LevelPaletteTable
public LevelPropHeader
public LevelScriptTable
public LevelSegment
public LevelStartX
public LevelStartY
public LoadSqzFile
public MapRows
public MapScrollX
public MapScrollY
public MixedTileCount
public NextFreeSegment
public OptionBits
public OptionVByte
public PageInResource
public PictureSegment
public PlayerHurtTimer
public PlayerVelX
public PlayerVelY
public PlayerX
public PlayerY
public PollInputState
public PrevCameraCol
public PrevCameraRow
public PrevMapScrollX
public PrevMapScrollY
public PrevScreenStartOffset
public PrintAsciiz
public PrintDecimalWord
public RandomSeed
public RenderSlots
public RingTileOffset
public RowRing
public ScoreHigh
public ScoreLow
public ScreenExitRequested
public ScreenFirstFrame
public ScreenRowBytes
public ScreenStartOffset
public ScriptFrameCounter
public ScriptPointer
public ScrollAnimCounter
public SetLevelPalette
public SetStartupPalette
public ShakeRows
public SineTable
public SnowFrameTick
public SnowWind
public SpriteBankA
public SpriteBankB
public SpriteDataSegment
public SpriteRelY
public SpriteSheetSegment
public StartSoundEffect
public TextAdvance
public TextByteColumn
public TextRowOffset
public TileClassTable
public VScrollActive
public VisibleTileFlags
public WaitForButtonPressRelease
public WaitSoundTicks
public WavePhase
public WindRandA
public WindRandB
public WindRandC
public WindRandWord
public byte_0666
public byte_083D
public byte_083E
public byte_27D6
public byte_27D8
public byte_27E8
public byte_27E9
public byte_27EA
public byte_27EB
public byte_27EC
public byte_27ED
public byte_27F6
public byte_27F7
public byte_2805
public byte_2806
public byte_280D
public byte_2810
public byte_2811
public byte_282C
public byte_282D
public byte_282F
public byte_2830
public byte_2879
public byte_287C
public byte_287D
public byte_2CF6
public byte_2D96
public byte_4F1B
public byte_4F21
public byte_4F23
public byte_4F24
public byte_4F27
public byte_4F2B
public byte_4F2C
public byte_564D
public byte_565F
public byte_5671
public byte_5683
public byte_5695
public byte_6688
public byte_6689
public byte_668A
public byte_6788
public byte_6789
public byte_678A
public byte_6888
public byte_6889
public byte_688A
public byte_6BC0
public byte_6BC1
public byte_6BC5
public byte_6BC6
public byte_6BC7
public byte_6BC8
public byte_6BC9
public byte_6BCC
public byte_6BCD
public byte_6BCE
public byte_6BCF
public byte_6BD0
public byte_6BD1
public byte_6BD2
public byte_6BD3
public byte_6BD9
public byte_6BDB
public byte_6BDC
public byte_6BE0
public byte_6BE1
public byte_6BE4
public byte_6BE5
public byte_6BE6
public byte_6BE7
public byte_6BE8
public byte_6BEA
public byte_6BF3
public byte_6BFE
public byte_6BFF
public byte_6C00
public byte_6C01
public byte_6C02
public byte_6C03
public byte_6C04
public byte_6C05
public byte_6C12
public byte_6CA4
public byte_6CA5
public byte_6CA6
public byte_6CA7
public byte_6CA8
public byte_7090
public byte_78C6
public byte_7B06
public byte_7B07
public byte_7B08
public byte_7B18
public byte_7B19
public byte_7B1A
public byte_7B7F
public byte_7F5E
public byte_91FB
public byte_91FE
public byte_A324
public byte_A325
public byte_A328
public byte_A329
public byte_A32A
public byte_A32B
public byte_A32C
public byte_A340
public byte_A3F9
public byte_A3FA
public byte_A515
public byte_A516
public byte_B198
public cvar_0000
public dtbl_79EC
public dtbl_7D2F
public dtbl_7D95
public word_0039
public word_003B
public word_003D
public word_0100
public word_1F18
public word_27F0
public word_27F2
public word_287A
public word_2A76
public word_2A7A
public word_2DC4
public word_2DCA
public word_2DCC
public word_2DD2
public word_4F0C
public word_4F0E
public word_4F20
public word_4F25
public word_4F28
public word_4F2E
public word_4F30
public word_4F32
public word_4F34
public word_4F37
public word_4F3A
public word_4F3C
public word_4F40
public word_4F42
public word_4F44
public word_4F52
public word_4F54
public word_4F56
public word_5088
public word_5648
public word_564A
public word_564C
public word_565A
public word_565C
public word_565E
public word_566C
public word_566E
public word_5670
public word_567E
public word_5680
public word_5682
public word_5690
public word_5692
public word_5694
public word_62DA
public word_667A
public word_6989
public word_6A88
public word_6B14
public word_6BAB
public word_6BB1
public word_6BBE
public word_6BCA
public word_6BE2
public word_6BEB
public word_6BF1
public word_6BFA
public word_6BFC
public word_6C06
public word_6C08
public word_6C0A
public word_6C0C
public word_6C9E
public word_6CA0
public word_6CA2
public word_7522
public word_78C4
public word_79CE
public word_7B04
public word_7B1B
public word_7CDF
public word_7D9B
public word_7DA9
public word_91F7
public word_91F9
public word_91FC
public word_91FF
public word_9201
public word_A30E
public word_A310
public word_A326
public word_A32E
public word_A341
public word_A3F7
public word_A3FB
public word_A3FD
public word_A3FF
public word_A401
public word_A403
public word_A405
public word_A407
public word_A408
public word_A409
public word_A40B
public word_A40D
public word_A40E
public word_A40F
public word_A411
public word_A413
public word_A414
public word_A415
public word_A417
public word_A419
public word_A41A
public word_A41B
public word_A41D
public word_A41F
public word_A420
public word_A421
public word_A423
public word_A425
public word_A4F7
public word_A517
public word_A519
; end module interface
CODE segment para public 'CODE'
assume cs:CODE, ds:DATA, es:nothing, ss:nothing
; module interface (tools/symtab.py)
extrn AnimateTiles:near
extrn BuildEffectDrawList:near
extrn CheckVgaAdapter:near
extrn CopyRingWindowToPage:near
extrn DrawForegroundOverSprites:near
extrn DrawSpriteObjects:near
extrn EnterLevelView:near
extrn FadeOutPaletteToBlack:near
extrn LoadLevel:near
extrn LoadSpriteSheet:near
extrn PresentFrame:near
extrn ProcessPlayerContacts:near
extrn ReadClockYear:near
extrn RedrawOverlayTiles:near
extrn RunFrontEnd:near
extrn SetCpuWriteMode:near
extrn SetLevelVideoMode:near
extrn ShowLateYearCredits:near
extrn ShowMode13PictureFadeIn:near
extrn ShowPresentTitle:near
extrn ShowTitusScreen:near
extrn ShowWorldMapScrollIn:near
extrn sub_454E:near
extrn sub_45AF:near
extrn sub_4907:near
extrn sub_4B8E:near
extrn sub_4C30:near
extrn sub_4C69:near
extrn sub_5237:near
extrn sub_52FE:near
extrn sub_53F6:near
extrn sub_54AB:near
extrn sub_55FC:near
extrn sub_5643:near
extrn sub_581E:near
extrn sub_5850:near
extrn sub_5CCE:near
extrn sub_5CDB:near
extrn sub_5E62:near
extrn sub_5E96:near
extrn sub_5EC4:near
extrn sub_5F13:near
extrn sub_5F30:near
extrn sub_5F96:near
extrn sub_60DF:near
extrn sub_60FE:near
extrn sub_6210:near
extrn sub_6272:near
extrn sub_6277:near
extrn sub_652C:near
extrn sub_6539:near
extrn sub_65AF:near
extrn sub_65EF:near
extrn sub_6641:near
extrn sub_6645:near
extrn sub_6657:near
extrn sub_6660:near
extrn sub_6669:near
extrn sub_6672:near
extrn sub_6673:near
extrn sub_6772:near
extrn sub_67D7:near
extrn sub_6822:near
extrn UpdateFollowerSlots:near
extrn UpdateWindAndFlakes:near
; end module interface
; Unused: no reader or writer in src/ (initial bytes at cs:0000: 03h FFh FFh 1Fh 00h 00h 00h 00h).
; The old DATA comment saying the entry writes this byte is wrong: it writes cs:1, 2, 3, 5 and 6.
cvar_0000 label byte
; DATA: command-line option and CPU state bytes/words (written by entry 0008..007C, CPU probe 1C0C, INT 12h at 00A9)
    db 03h
; OptionVByte: FFh by default, 1 when /V is given (ProgramEntry). Read only by
; SetDacBlock0To15IfFlag, which tests for 5 (never stored).
OptionVByte label byte
; UNKNOWN: not yet classified as code or data
    db 0FFh
; SoundEnabledFlag: FFh by default, 0 when /M is given (ProgramEntry). InitSoundDevices returns
; at once when it is 0.
SoundEnabledFlag label byte
; UNKNOWN: not yet classified as code or data
    db 0FFh
; OptionBits (word, default 001Fh). Command-line effects (cleared = bit off unless stated):
;   01h /F: no reader in R1 (no effect observed).   02h /A: FRONT.SQZ not loaded at start-up.
;   04h /E: EMS not used.   08h /X: XMS not used.   10h /L: no resource cache (see
;   InitResourceMemory).   20h /D: resources loaded into the heap; never set by any path, so /D
;   has no effect.   40h /B: low-memory mode, set by /B and also by a memory check near cs:1F23
;   (outside R1); makes ActivateResourceSlot and sample placement skip.
OptionBits label word
; UNKNOWN: not yet classified as code or data
    db 1Fh, 00h
; CpuClass: 0 = 8086/8088, 2 = 80186/80286, 3 = 80386 or later (ProbeCpuClass). InitSoundDevices
; needs at least 2 for the OPL path; sub_1CEB uses the longer delays at 3.
CpuClass label byte
; UNKNOWN: not yet classified as code or data
    db 00h
; ConventionalTopSeg (word): INT 12h KB * 64, the first segment above conventional memory. Set by
; ProgramEntry; used as the heap limit by the loaders and the memory checks.
ConventionalTopSeg label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h

; Program entry (far entry 0020:0008). Parses the command tail (PSP 80h) into OptionVByte,
; SoundEnabledFlag and OptionBits, then runs the start-up sequence and MainFrameLoop.
; Start-up order (cs:0008..0126): option parse; NextFreeSegment = DATA + 0B1Ch; INT 24h =
;   CriticalErrorHandler (ds:90h/92h); SwapInt08Int09Vectors (install); INT 12h ->
;   ConventionalTopSeg; sub_23D4; ProbeCpuClass; CalibrateJoystick; KEYB.SQZ (ds:000Ch) loaded,
;   its first 800h bytes copied to DATA:003Fh; PIT ch0 = 16384 (~72.8 Hz, cs:00E5); ALLFONTS.SQZ
;   (ds:0015h) loaded and its segment stored at word_003D (see note there); sub_240A;
;   InitSoundDevices; sub_912B; ActivateResourceSlot(3); sub_9090; InitResourceMemory;
;   FRONT.SQZ (ds:0022h) -> word_003B unless OptionBits 02h (/A); sub_2DFA; then LevelRestart.
; Never returns to DOS: the only exit is the ESC quit in KeyboardIsr.
ProgramEntry:
    cli
    cld
    mov dx, 0FFFFh
    mov si, 80h
    lodsb
    cbw
    mov cx, ax
    jcxz loc_007E
loc_0016:
    lodsb
    cmp al, 2Fh
    je loc_001F
    loop loc_0016
    jmp short loc_007E
loc_001F:
    lodsb
    and al, 0DFh
    cmp al, 46h
    jne loc_002C
    and word ptr cs:OptionBits, 0FFFEh
loc_002C:
    cmp al, 4Dh
    jne loc_0036
    mov byte ptr cs:SoundEnabledFlag, 0
loc_0036:
    cmp al, 41h
    jne loc_0040
    and word ptr cs:OptionBits, 0FFFDh
loc_0040:
    cmp al, 45h
    jne loc_004A
    and word ptr cs:OptionBits, 0FFFBh
loc_004A:
    cmp al, 58h
    jne loc_0054
    and word ptr cs:OptionBits, 0FFF7h
loc_0054:
    cmp al, 4Ch
    jne loc_005E
    and word ptr cs:OptionBits, 0FFEFh
loc_005E:
    cmp al, 44h
    jne loc_0068
    and word ptr cs:OptionBits, 0FFDFh
loc_0068:
    cmp al, 42h
    jne loc_0072
    or word ptr cs:OptionBits, 40h
loc_0072:
    cmp al, 56h
    jne loc_007E
    mov byte ptr cs:OptionVByte, 1
    loop loc_0016
loc_007E:
    mov ax, seg DATA
    mov ds, ax
    mov ax, 0B1BCh
    mov cl, 4
    shr ax, cl
    add ax, seg DATA+1
    mov word ptr NextFreeSegment, ax
    push ds
    xor ax, ax
    mov ds, ax
    mov word ptr ds:[90h], 0FC0h
    mov word ptr ds:[92h], cs
    pop ds
    call SwapInt08Int09Vectors
    int 12h
    mov cl, 6
    shl ax, cl
    mov word ptr cs:ConventionalTopSeg, ax
    sti
    call ReadClockYear
    mov ax, seg DATA
    mov ds, ax
    call ProbeCpuClass
    call CalibrateJoystick
    call CheckVgaAdapter
    call SetLevelVideoMode
    mov dx, 0Ch
    push ds
    push es
    call LoadSqzFile
    pop es
    pop ds
    mov word ptr NextFreeSegment, ax
    mov word ptr word_003D, ax
    xor si, si
    mov ds, word ptr word_003D
    mov ax, seg DATA
    mov es, ax
    mov di, 3Fh
    mov cx, 400h
    rep movsw
    mov ax, 4000h
    out 40h, al
    mov al, ah
    out 40h, al
    mov dx, 15h
    push ds
    push es
    call LoadSqzFile
    pop es
    pop ds
    mov word ptr ds:[3Dh], ax
    call ShowLateYearCredits
    call InitSoundDevices
    call ShowTitusScreen
    mov ax, 3
    call ActivateResourceSlot
    call ShowPresentTitle
    call InitResourceMemory
    test word ptr cs:OptionBits, 2
    je loc_0126
    mov dx, 22h
    push ds
    push es
    call LoadSqzFile
    pop es
    pop ds
    mov word ptr word_003B, ax
loc_0126:
    call LoadSpriteSheet
    mov ax, word ptr NextFreeSegment
    mov word ptr word_0039, ax
; Level (re)start, entered from ProgramEntry and from MainFrameLoop when sub_4C69 returns CF = 1.
; Resets NextFreeSegment from word_0039 and calls the level-change routines when AX is nonzero.
; For level ids 8..9 with byte_B198 = 0 it loads CASTLE.SQZ (ds:002Ch) and jumps back here with
; AX = 1. Then activates slot byte_2D20[level] (ActivateResourceSlot), sets the level palette
; (SetLevelPalette), runs the level set-up calls and falls into MainFrameLoop.
LevelRestart:
    mov dx, word ptr word_0039
    mov word ptr NextFreeSegment, dx
    or ax, ax
    je loc_0163
    call FadeOutPaletteToBlack
    call SetLevelVideoMode
    mov byte ptr byte_27D8, 2
    mov byte ptr byte_6CA7, 0
    mov byte ptr byte_7B19, 14h
    mov byte ptr byte_7B18, 0
    mov byte ptr byte_6CA8, 0
    mov ax, 3
    call ActivateResourceSlot
    call RunFrontEnd
loc_0163:
    cmp byte ptr ds:CurrentLevel, 8
    jb loc_019E
    cmp byte ptr ds:CurrentLevel, 0Ah
    jae loc_019E
    cmp byte ptr ds:byte_B198, 0
    jne loc_019E
    mov dx, 2Ch
    push ds
    push es
    call LoadSqzFile
    pop es
    pop ds
    mov word ptr NextFreeSegment, ax
    mov word ptr LevelSegment, ax
    xor si, si
    mov ds, ax
    mov cl, 0FFh
    call ShowMode13PictureFadeIn
    mov ax, seg DATA
    mov ds, ax
    call WaitForButtonPressRelease
    mov ax, 1
    jmp short LevelRestart
loc_019E:
    cmp byte ptr ds:CurrentLevel, 0Ah
    jae loc_01AB
    call SetLevelVideoMode
    call ShowWorldMapScrollIn
loc_01AB:
    mov bl, byte ptr CurrentLevel
    xor bh, bh
    mov al, byte ptr [bx+byte_2D20]
    xor ah, ah
    call ActivateResourceSlot
    cmp byte ptr ds:CurrentLevel, 0Ah
    jae loc_01C4
    call SetLevelVideoMode
loc_01C4:
    mov ax, 0FFFFh
    mov di, 4F0Ah
    mov cx, 40Bh
    rep stosw
    mov al, byte ptr CurrentLevel
    call LoadLevel
    call SetLevelPalette
    call sub_5237
    mov ax, seg DATA
    mov ds, ax
    call sub_55FC
    call EnterLevelView
    call sub_454E
    call CopyRingWindowToPage
    call PresentFrame
    mov word ptr word_A341, 0
    mov byte ptr byte_6BCC, 0
    mov byte ptr WindRandA, 5
    mov byte ptr WindRandB, 22h
    mov byte ptr WindRandC, 86h
    mov word ptr WindRandWord, 0E58Dh
    mov byte ptr LastScanCode, 0
; Per-frame loop of the game (after ProgramEntry's start-up). STI, then in order:
;   sub_581E sub_88D7 sub_6822 sub_6210 sub_60FE sub_60DF sub_4907 sub_5850 sub_8295 sub_8922
;   sub_52FE sub_53F6 sub_5643 sub_3668 sub_35A1 sub_3A27 sub_4B8E sub_26FA sub_3721 sub_54AB
;   sub_3922 sub_4C69. If CF = 0 after sub_4C69: sub_45AF sub_44FB sub_6772 sub_67D7 sub_4C30,
;   then back to this label. If CF = 1: jump to LevelRestart (level change or restart).
MainFrameLoop:
    mov ax, seg DATA
    mov ds, ax
    sti
    call sub_581E
    call UpdateFollowerSlots
    call sub_6822
    call sub_6210
    call sub_60FE
    call sub_60DF
    call sub_4907
    call sub_5850
    call ProcessPlayerContacts
    call BuildEffectDrawList
    call sub_52FE
    call sub_53F6
    call sub_5643
    call AnimateTiles
    call RedrawOverlayTiles
    call CopyRingWindowToPage
    call sub_4B8E
    call DrawSpriteObjects
    call DrawForegroundOverSprites
    call sub_54AB
    call UpdateWindAndFlakes
    call sub_4C69
    jae loc_0261
    jmp near ptr LevelRestart
loc_0261:
    call sub_45AF
    call PresentFrame
    call sub_6772
    call sub_67D7
    call sub_4C30
    jmp short MainFrameLoop
; SbBasePort (word): the detected Sound Blaster base (e.g. 220h) written by sub_1D6F; the initial
; words 0000h, 000Ah, 000Ch, 000Eh are offsets until detection replaces them.
SbBasePort label word
; DATA: detected SB port words (written 1DA2..1DC1)
    db 00h, 00h
; SbReadPort (word): SB base + 0Ah, DSP read data port (sub_1D6F reads AAh here).
SbReadPort label word
; UNKNOWN: not yet classified as code or data
    db 0Ah, 00h
; SbWritePort (word): SB base + 0Ch, DSP write port; busy while bit 7 of a read is set.
SbWritePort label word
; UNKNOWN: not yet classified as code or data
    db 0Ch, 00h
; SbAckPort (word): SB base + 0Eh, DSP read-buffer status port (not used in R1).
SbAckPort label word
; UNKNOWN: not yet classified as code or data
    db 0Eh, 00h
; Int08SwapSlot: four words at cs:027Ah, the swap slot for IVT 08h (offset, segment), followed by
; the IVT 09h slot at cs:027Eh (no label in the source; a `cvar_027E label dword` would name it).
; Initially holds SpeakerTimerIsr and KeyboardIsr (both 0020:xxxx); SwapInt08Int09Vectors
; exchanges them with the IVT and so holds the BIOS vectors while the game's handlers are
; installed. The busy path of the timer body (loc_17C0) chains through this slot.
Int08SwapSlot label dword
    dd SpeakerTimerIsr
    dd KeyboardIsr

; Starts sound effect DL (0..10). Sample mode (cvar_1D6C or cvar_1D6D equal to 1): copies the
; start offset from DATA:1009h+4*DL into SfxPlayPos and the length from DATA:100Bh+4*DL into
; SfxBytesLeft (offsets inside SfxBankSegment). Otherwise (speaker mode) points
; SpeakerSequencePtr at the 10-byte record DATA:1037h+10*DL.
; In: DL = effect index (no range check). Out: none. Preserves all registers.
StartSoundEffect:
    cmp byte ptr cs:AdLibEnabled, 1
    je loc_02A9
    cmp byte ptr cs:SbDetected, 1
    je loc_02A9
    push ax
    push bx
    push ds
    mov ax, seg DATA
    mov ds, ax
    mov al, 0Ah
    mul dl
    add ax, 1037h
    mov word ptr SpeakerSequencePtr, ax
    pop ds
    pop bx
    pop ax
    jmp short loc_02C8
loc_02A9:
    push ax
    push bx
    push ds
    mov bx, dx
    shl bx, 1
    shl bx, 1
    mov ax, seg DATA
    mov ds, ax
    mov ax, word ptr [bx+word_1009]
    mov word ptr SfxPlayPos, ax
    mov ax, word ptr [bx+word_100B]
    mov word ptr SfxBytesLeft, ax
    pop ds
    pop bx
    pop ax
loc_02C8:
    ret
loc_02C9:
    jmp near ptr loc_0482

; Makes resource slot AX the active slot. Returns at once when cvar_1D6D is 0 (no Sound
; Blaster), when the slot already has bit 01h, when byte_2879 is 1, or when OptionBits 40h is
; set (that path only performs the sound update at the end). Otherwise: sets bit 01h of
; ResourceFlags[AX], clears it on every other slot that lacks bit 08h (pinned), clears cvar_1D6E
; bit 1, calls PageInResource(AX), then rebuilds the bank descriptor tables (DATA:0BD2h..0BDCh,
; DATA:0DC2h..0DC7h) from the resource header in ModuleSegment and rescales the bank bytes in
; place with (b + 80h) >> 2. Ends with the sound update (sub_214E if cvar_1D6E bit 0) and sets
; cvar_1D6E bit 1.
; In: AX = slot (0..17). Out: none. Preserves all registers.
; Notes: adds 80h paragraphs (2 KB) to NextFreeSegment on entry and removes them on every exit.
;   The purpose of the bank rebuild is not proven by R1 (palette-like use suspected).
ActivateResourceSlot:
    push ax
    push bx
    push cx
    push dx
    push di
    push si
    push ds
    push es
    push bp
    add word ptr NextFreeSegment, 80h
    cmp byte ptr cs:SbDetected, 0
    je loc_02C9
    mov si, ax
    mov al, byte ptr [si+ResourceFlags]
    test al, 1
    jne loc_02C9
    cmp byte ptr ds:byte_2879, 1
    je loc_02C9
    test word ptr cs:OptionBits, 40h
    je loc_0300
    jmp near ptr loc_046F
loc_0300:
    or byte ptr [si+ResourceFlags], 1
    xor di, di
    mov cx, 12h
loc_030A:
    cmp di, si
    je loc_031A
    test byte ptr [di+ResourceFlags], 8
    jne loc_031A
    and byte ptr [di+ResourceFlags], 0FEh
loc_031A:
    inc di
    loop loc_030A
    mov cx, si
    and byte ptr cs:SoundState, 0FDh
    call PageInResource
    mov ax, seg DATA
    mov es, ax
    mov ds, word ptr es:ModuleSegment
    mov si, 14h
    xor di, di
    mov dx, 0Fh
    cmp word ptr ds:[438h], 2E4Dh
    jne loc_034C
    cmp word ptr ds:[43Ah], 2E4Bh
    jne loc_034C
    mov dl, 1Fh
loc_034C:
    mov byte ptr es:InstrumentCount, dl
loc_0351:
    add si, 16h
    lodsw
    xchg al, ah
    shl ax, 1
    mov word ptr es:[di+InstrumentTable], ax
    lodsw
    xchg al, ah
    mov word ptr es:[di+word_0BD4], ax
    lodsw
    xchg al, ah
    shl ax, 1
    mov word ptr es:[di+word_0BD6], ax
    lodsw
    xchg al, ah
    shl ax, 1
    mov word ptr es:[di+word_0BD8], ax
    add di, 10h
    dec dx
    jne loc_0351
    lodsb
    mov byte ptr es:SongLength, al
    inc si
    mov di, 0DC7h
    mov cx, 40h
    rep movsw
    mov ax, seg DATA
    mov ds, ax
    mov es, word ptr ModuleSegment
    cmp byte ptr ds:InstrumentCount, 0Fh
    je loc_03A1
    add si, 4
loc_03A1:
    mov cl, byte ptr SongLength
    xor ch, ch
    mov di, 0DC7h
    xor ah, ah
loc_03AC:
    mov al, byte ptr [di]
    cmp al, ah
    jbe loc_03B4
    mov ah, al
loc_03B4:
    inc di
    loop loc_03AC
    inc ah
    mov byte ptr PatternCount, ah
    mov word ptr PatternDataOffset, si
    xor al, al
    shr ax, 1
    shr ax, 1
    add ax, word ptr ModuleSegment
    xor bx, bx
    mov cl, byte ptr InstrumentCount
    xor ch, ch
loc_03D3:
    mov dx, si
    and si, 0Fh
    mov word ptr [bx+word_0BDA], si
    shr dx, 1
    shr dx, 1
    shr dx, 1
    shr dx, 1
    add ax, dx
    mov word ptr [bx+word_0BDC], ax
    mov bp, word ptr [bx+InstrumentTable]
    or bp, bp
    je loc_0407
    mov di, si
    mov es, ax
    mov dx, ax
loc_03F8:
    mov al, byte ptr es:[di]
    add al, 80h
    shr al, 1
    shr al, 1
    stosb
    dec bp
    jne loc_03F8
    mov ax, dx
loc_0407:
    add si, word ptr [bx+InstrumentTable]
    add bx, 10h
    loop loc_03D3
    mov ax, seg DATA
    mov ds, ax
    mov es, ax
    mov si, word ptr PatternDataOffset
    mov dh, byte ptr PatternCount
    mov ds, word ptr ModuleSegment
    xor dl, dl
loc_0425:
    lodsw
    or ax, ax
    je loc_043F
    mov cx, 24h
    mov di, 0E75h
    xchg al, ah
    and ah, 0Fh
    repne scasw
    sub cx, 24h
    neg cx
    mov word ptr [si-2], cx
loc_043F:
    inc si
    inc si
    dec dx
    jne loc_0425
    mov ax, es
    mov ds, ax
    mov word ptr SongOrderPos, 0
    mov word ptr SongRow, 0
    mov byte ptr SongTickCount, 1
    mov byte ptr SongSpeed, 6
    mov di, 0B8Ah
    mov cx, 4
    mov ax, 0FFFFh
    rep stosw
    inc ax
    mov cx, 20h
    rep stosw
loc_046F:
    cli
    test byte ptr cs:SoundState, 1
    je loc_047B
    call StartSbBlock
loc_047B:
    or byte ptr cs:SoundState, 2
    sti
loc_0482:
    sub word ptr NextFreeSegment, 80h
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

; Makes resource slot CX resident. Location code = ResourceFlags bits 02h and 04h:
;   0 (no copy): name from ResourceNameAt, SQZ load by sub_107B into the heap, then unless pinned
;     (08h) copy 105Bh paragraphs into ModuleSegment and restore NextFreeSegment;
;   1 (02h): copy from the conventional segment in ResourceAddressTable[CX] (sub_14D8);
;   2 (04h): map EMS logical pages from ResourceAddressTable[CX] with INT 67h AH=44h (handle
;     EmsHandle, physical pages 0 and 1), then copy (sub_14D8);
;   3 (06h): XMS move with XmsDriverEntry, function 0Bh, from XMS offset ResourceAddressTable[CX].
; In: CX = slot. Out: the data is at the location above; AX is not defined on return.
; Notes: an EMS or XMS failure goes to the fatal XMS message in CriticalErrorHandler (loc_0FEC).
PageInResource:
    mov si, cx
    mov al, byte ptr [si+ResourceFlags]
    mov ah, al
    and ah, 8
    mov byte ptr byte_0B16, ah
    shr al, 1
    and al, 3
    jne loc_04AA
    jmp near ptr loc_0699
loc_04AA:
    cmp al, 1
    jne loc_04E1
    mov bx, si
    shl bx, 1
    shl bx, 1
    mov ax, word ptr NextFreeSegment
    cmp byte ptr ds:byte_0B16, 0
    jne loc_04C1
    mov ax, word ptr ModuleSegment
loc_04C1:
    mov es, ax
    mov ds, word ptr [bx+ResourceAddressTable]
    mov si, 11h
    xor di, di
    mov byte ptr cs:LzssSourceMode, 1
    call DecodeLzssStream
    mov byte ptr cs:LzssSourceMode, 0
    mov ax, word ptr NextFreeSegment
    jmp near ptr loc_06D4
loc_04E1:
    cmp al, 2
    jne loc_0542
    mov ah, 44h
    mov al, 0
    mov bx, si
    shl bx, 1
    shl bx, 1
    mov bx, word ptr [bx+ResourceAddressTable]
    mov si, bx
    and si, 0Fh
    mov cl, 0Ah
    shl si, cl
    add si, 11h
    shr bx, 1
    shr bx, 1
    shr bx, 1
    shr bx, 1
    mov dx, word ptr EmsHandle
    int 67h
    inc bx
    mov ax, 4401h
    int 67h
    mov word ptr cs:EmsNextLogicalPage, bx
    mov ax, word ptr NextFreeSegment
    cmp byte ptr ds:byte_0B16, 0
    jne loc_0525
    mov ax, word ptr ModuleSegment
loc_0525:
    mov es, ax
    xor di, di
    mov ds, word ptr EmsFrameSegment
    mov byte ptr cs:LzssSourceMode, 2
    call DecodeLzssStream
    mov byte ptr cs:LzssSourceMode, 0
    mov ax, word ptr NextFreeSegment
    jmp near ptr loc_06D4
loc_0542:
    cmp al, 3
    je loc_0549
    jmp near ptr loc_0699
loc_0549:
    mov al, byte ptr [si+ResourceSizeKb]
    xor ah, ah
    xor bx, bx
    mov cx, 0Ah
loc_0554:
    shl ax, 1
    rcl bx, 1
    loop loc_0554
    mov word ptr cs:XmsBytesLeftLow, ax
    mov word ptr cs:XmsBytesLeftHigh, bx
    mov bx, si
    shl bx, 1
    shl bx, 1
    mov ax, word ptr [bx+ResourceAddressTable]
    xor dx, dx
    mov cx, 0Ah
loc_0572:
    shl ax, 1
    rcl dx, 1
    loop loc_0572
    mov word ptr XmsSourceOffsetLow, ax
    mov word ptr XmsSourceOffsetHigh, dx
    mov word ptr XmsMoveLength, 18h
    mov word ptr word_0B23, 0
    mov word ptr word_0B2D, 0B31h
    mov word ptr word_0B2F, ds
    mov word ptr word_0B2B, 0
    mov ax, word ptr XmsHandle
    mov word ptr XmsSourceHandle, ax
    mov si, 0B21h
    mov ah, 0Bh
    inc byte ptr cs:DosCallActive
    call dword ptr XmsDriverEntry
    cmp ax, 1
    je loc_05B7
    jmp near ptr ReportXmsError
loc_05B7:
    dec byte ptr cs:DosCallActive
    mov dl, byte ptr byte_0B3F
    mov ax, word ptr word_0B40
    mov cx, 4
    shr dl, 1
    shr dl, 1
loc_05CA:
    shr dl, 1
    rcr ax, 1
    loop loc_05CA
    inc ax
    cmp byte ptr ds:byte_0B16, 0
    jne loc_05DA
    xor ax, ax
loc_05DA:
    mov dx, ax
    add dx, word ptr NextFreeSegment
    cmp dx, word ptr cs:ConventionalTopSeg
    jb loc_05EA
    jmp near ptr loc_06D5
loc_05EA:
    mov es, dx
    mov ax, dx
    sub ax, word ptr cs:ConventionalTopSeg
    neg ax
    mov dx, 0F000h
    cmp ax, 0F00h
    ja loc_0603
    mov cl, 4
    shl ax, cl
    mov dx, ax
loc_0603:
    mov word ptr cs:LzssChunkBytes, dx
    sub dx, 80h
    mov word ptr cs:LzssRefillThreshold, dx
    mov word ptr word_0B2D, 0
    mov word ptr word_0B2F, es
    mov ax, word ptr cs:LzssChunkBytes
    cmp word ptr cs:XmsBytesLeftHigh, 0
    jne loc_0632
    cmp word ptr cs:XmsBytesLeftLow, ax
    jae loc_0632
    mov ax, word ptr cs:XmsBytesLeftLow
loc_0632:
    sub word ptr cs:XmsBytesLeftLow, ax
    sbb word ptr cs:XmsBytesLeftHigh, 0
    mov word ptr XmsMoveLength, ax
    mov word ptr word_0B23, 0
    mov ah, 0Bh
    mov si, 0B21h
    inc byte ptr cs:DosCallActive
    call dword ptr XmsDriverEntry
    cmp ax, 1
    je loc_065C
    jmp near ptr ReportXmsError
loc_065C:
    dec byte ptr cs:DosCallActive
    mov ax, word ptr XmsMoveLength
    add word ptr XmsSourceOffsetLow, ax
    adc word ptr XmsSourceOffsetHigh, 0
    xor di, di
    mov si, 11h
    mov byte ptr cs:LzssSourceMode, 3
    mov ax, word ptr NextFreeSegment
    cmp byte ptr ds:byte_0B16, 0
    jne loc_0685
    mov ax, word ptr ModuleSegment
loc_0685:
    mov ds, ax
    push ds
    push es
    pop ds
    pop es
    call DecodeLzssStream
    mov byte ptr cs:LzssSourceMode, 0
    mov ax, word ptr NextFreeSegment
    jmp short loc_06D4
loc_0699:
    call ResourceNameAt
    push word ptr NextFreeSegment
    cmp byte ptr ds:byte_0B16, 0
    jne loc_06CD
    push ds
    push es
    call LoadSqzFile
    mov dx, word ptr ModuleSegment
    mov bx, 105Bh
loc_06B3:
    xor si, si
    xor di, di
    mov ds, ax
    mov es, dx
    mov cx, 8
    rep movsw
    inc dx
    inc ax
    dec bx
    jne loc_06B3
    pop es
    pop ds
    pop word ptr NextFreeSegment
    jmp short loc_06D4
loc_06CD:
    call LoadSqzFile
    pop word ptr NextFreeSegment
loc_06D4:
    ret
loc_06D5:
    mov ax, 3
    int 10h
    mov ax, seg DATA
    mov ds, ax
    mov ah, 9
    mov dx, 840h
    int 21h
    jmp near ptr HaltWithMessage
loc_06E9:
    mov ax, 4000h
    out 40h, al
    mov al, ah
    out 40h, al
    pop ax
    jmp near ptr loc_07AD

; INT 08h handler for sample output through port 389h; installed by InitAdlibSampler (0020h).
; Busy (DATA:2877 nonzero): PIT ch0 = 16384 and the tick path. Otherwise PIT ch0 = 142
; (~8.4 kHz); while cvar_1D6E bit 0 is set and SfxBytesLeft is nonzero, outputs one sample:
; byte SfxBankSegment:SfxPlayPos, xor 80h, >> 1, translated through DATA:0F05h, to port 389h.
; Every 128th call (cvar_1D69) takes the tick path.
; Tick path (loc_07AD): cvar_1D67++, cvar_1D6B = (cvar_1D6B + 1) & 3; every 4th tick jumps to
; loc_17C0 (BIOS-tick body: DATA:006Ch / 40:006Ch counter, floppy motor timeout, midnight wrap).
; Other ticks and the sample path end with EOI and IRET. Preserves all registers.
SampleTimerIsr:
    push ax
    cmp byte ptr cs:DosCallActive, 0
    jne loc_06E9
    mov al, 8Eh
    out 40h, al
    xor al, al
    out 40h, al
    pop ax
    inc word ptr cs:cvar_1D69
    and word ptr cs:cvar_1D69, 7Fh
    jne loc_0718
    jmp near ptr loc_07AD
loc_0718:
    push ax
    push bx
    push dx
    push si
    push ds
    test byte ptr cs:SoundState, 1
    je loc_0752
    mov ax, seg DATA
    mov ds, ax
    cmp word ptr ds:SfxBytesLeft, 0
    je loc_0752
    dec word ptr SfxBytesLeft
    mov si, word ptr SfxPlayPos
    inc word ptr SfxPlayPos
    mov dx, ds
    mov ds, word ptr SfxBankSegment
    lodsb
    xor al, 80h
    mov ds, dx
    mov dx, 389h
    mov bx, 0F05h
    shr al, 1
    xlat
    out dx, al
loc_0752:
    mov al, 20h
    out 20h, al
    pop ds
    pop si
    pop dx
    pop bx
    pop ax
    iret

; INT 08h handler when no sample device is active (cvar_1D6C and cvar_1D6D both 0). Steps the
; PC-speaker sequence at SpeakerSequencePtr (-1 = idle): writes the current divisor to PIT ch2
; (port 42h, low then high byte), subtracts the step, and when the count reaches zero reloads the
; record and sets the pointer to -1 and divisor 000Ah. Record (10 bytes): +0 divisor, +2 start
; divisor, +4 count, +6 step, +8 count reload. Then the tick path of SampleTimerIsr.
; Initial value of Int08SwapSlot (installed first by SwapInt08Int09Vectors).
; Preserves all registers.
SpeakerTimerIsr:
    push ax
    push bx
    push si
    push ds
    mov ax, seg DATA
    mov ds, ax
    mov al, byte ptr cs:SbDetected
    or al, byte ptr cs:AdLibEnabled
    jne loc_07A9
    mov si, word ptr SpeakerSequencePtr
    cmp si, -1
    je loc_07A9
    mov bx, word ptr [si]
    mov al, bl
    out 42h, al
    mov al, bh
    out 42h, al
    sub bx, word ptr [si+6]
    mov word ptr [si], bx
    dec word ptr [si+4]
    jne loc_07A9
    mov ax, word ptr [si+8]
    mov word ptr [si+4], ax
    mov ax, word ptr [si+2]
    mov word ptr [si], ax
    mov word ptr SpeakerSequencePtr, 0FFFFh
    mov bx, 0Ah
    mov al, bl
    out 42h, al
    mov al, bh
    out 42h, al
loc_07A9:
    pop ds
    pop si
    pop bx
    pop ax
loc_07AD:
    inc word ptr cs:TimerTickCount
    inc byte ptr cs:cvar_1D6B
    and byte ptr cs:cvar_1D6B, 3
    je loc_07C6
    push ax
    mov al, 20h
    out 20h, al
    pop ax
    iret
loc_07C6:
    jmp near ptr loc_17C0

; Sound set-up, called once from ProgramEntry. Saves the PIC mask in PicMaskSaved. Returns at once
; when SoundEnabledFlag is 0 (/M). Otherwise loads SAMPLE.SQZ (ds:0859h) into SfxBankSegment
; and builds the 11 (start, length) sample pairs at DATA:1009h+4k / DATA:100Bh+4k. Then:
;   Sound Blaster detected (sub_1D6F): cvar_1D6D = 1, SbBasePort and the port words set;
;   else if CpuClass >= 2 and the OPL probe (sub_1D22) succeeds: InitAdlibSampler, cvar_1D6C = 1,
;     cvar_1D6E = 1;
;   else PC speaker: PIT ch2 mode 3 (port 43h, B6h), port 61h bits 0-1 set, and NextFreeSegment
;     reset to SfxBankSegment (the sample file is discarded).
; Out: cvar_1D6C, cvar_1D6D, cvar_1D6E, SfxBankSegment, NextFreeSegment, PicMaskSaved.
InitSoundDevices:
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
    in al, 21h
    mov byte ptr PicMaskSaved, al
    cmp byte ptr cs:SoundEnabledFlag, 0
    je loc_0849
    mov dx, 859h
    push ds
    push es
    call LoadSqzFile
    pop es
    pop ds
    mov word ptr SfxBankSegment, ax
    sub ax, word ptr NextFreeSegment
    neg ax
    mov word ptr SfxBankParagraphs, ax
    mov si, 1009h
    mov cx, 0Bh
    xor dx, dx
loc_0802:
    mov ax, word ptr [si+2]
    mov word ptr [si], dx
    add dx, ax
    add si, 4
    loop loc_0802
    call DetectSoundBlaster
    jb loc_081B
    mov byte ptr cs:SbDetected, 1
    jmp short loc_0849
loc_081B:
    call DetectAdLib
    jb loc_0839
    cmp byte ptr cs:CpuClass, 2
    jb loc_0839
    call InitAdlibSampler
    mov byte ptr cs:AdLibEnabled, 1
    mov byte ptr cs:SoundState, 1
    jmp short loc_0849
loc_0839:
    mov ax, word ptr SfxBankSegment
    mov word ptr NextFreeSegment, ax
    mov al, 0B6h
    out 43h, al
    in al, 61h
    or al, 3
    out 61h, al
loc_0849:
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

; Programs the OPL (ports 388h/389h through sub_1CEB) for sample output, waits for the OPL status
; at 388h to clear, sets PIT ch0 to 142 (~8.4 kHz) and writes SampleTimerIsr straight into IVT 08h
; (cs:06F6h) over the swapped-in handler. Called only from InitSoundDevices.
; In: none. Out: IVT 08h, PIT ch0 divisor 142. Interrupts are enabled again at return (STI).
; Clobbers: AX, CX, DX; ES is left 0000h.
InitAdlibSampler:
    mov ax, 2001h
    call OplWriteRegister
    mov ax, 0BDh
    call OplWriteRegister
    mov ax, 0B0h
    call OplWriteRegister
    mov ax, 2120h
    call OplWriteRegister
    mov ax, 0F060h
    call OplWriteRegister
    mov ax, 0F080h
    call OplWriteRegister
    mov ax, 1C0h
    call OplWriteRegister
    mov ax, 0E0h
    call OplWriteRegister
    mov ax, 3F43h
    call OplWriteRegister
    mov ax, 6004h
    call OplWriteRegister
    mov ax, 0F302h
    call OplWriteRegister
    mov ax, 8FA0h
    call OplWriteRegister
    mov ax, 2EB0h
    call OplWriteRegister
    mov ax, 2304h
    call OplWriteRegister
    mov dx, 388h
    cli
loc_08AB:
    in al, dx
    test al, 0F0h
    jne loc_08AB
    mov ax, 0A0h
    call OplWriteRegister
    mov ax, 20B0h
    call OplWriteRegister
    mov ax, 3F40h
    call OplWriteRegister
    mov al, 8Eh
    out 40h, al
    xor al, al
    out 40h, al
    xor ax, ax
    mov es, ax
    mov word ptr es:[20h], 6F6h
    mov word ptr es:[22h], cs
    sti
    ret

; Sets up the resource cache. If OptionBits 10h (/L) is clear: clears the resident bit 02h of
; ResourceFlags[0..17] and returns (every resource is loaded from disk on use).
; Otherwise EMS (unless OptionBits 04h, /E): INT 67h AH=40h/41h/42h/43h give EmsFrameSegment,
; EmsFreeKb (= pages * 16) and EmsHandle (55AAh = none). XMS (unless OptionBits 08h, /X, or when
; the current DOS drive from INT 21h AH=19h is above B:): INT 2Fh AX=4300h/4310h give XmsDriverEntry;
; AH=08h/09h give XmsFreeKb and XmsHandle for all free XMS. Then each of the 18 ResourceSlotList
; entries, in order, is placed: EMS (EmsFreeKb >= size and EMS handle set) via LoadResourceToEms,
; else XMS (XmsFreeKb >= size, handle set, flags 10h clear) via LoadResourceToXms, else if the
; resident bit 02h is set: LoadResourceFile when OptionBits 20h is set, otherwise bit 02h is
; cleared. Which entries are tried depends on cvar_1D6D and the pinned bit 08h.
; Out: the EMS/XMS variables, ResourceFlags, ResourceAddressTable, NextFreeSegment.
; Notes: the XMS branch is skipped when the current drive (AH=19h) is above 1.
InitResourceMemory:
    mov ax, seg DATA
    mov ds, ax
    test word ptr cs:OptionBits, 10h
    jne loc_08F7
    mov cx, 12h
    mov di, 0ABCh
loc_08F0:
    and byte ptr [di], 0FDh
    inc di
    loop loc_08F0
    ret
loc_08F7:
    test word ptr cs:OptionBits, 4
    je loc_094F
    xor ax, ax
    mov es, ax
    mov es, word ptr es:[19Eh]
    mov di, 0Ah
    mov cx, 4
    mov si, 0B49h
    repe cmpsw
    jne loc_094F
    mov ah, 40h
    int 67h
    or ah, ah
    jne loc_094F
    mov ah, 41h
    int 67h
    or ah, ah
    jne loc_094F
    mov word ptr EmsFrameSegment, bx
    mov ah, 42h
    int 67h
    or ah, ah
    jne loc_094F
    mov ax, bx
    shl ax, 1
    shl ax, 1
    shl ax, 1
    shl ax, 1
    mov word ptr EmsFreeKb, ax
    or bx, bx
    je loc_094F
    mov ah, 43h
    int 67h
    or ah, ah
    jne loc_094F
    mov word ptr EmsHandle, dx
loc_094F:
    test word ptr cs:OptionBits, 8
    je loc_0991
    mov ah, 19h
    int 21h
    cmp al, 1
    ja loc_0991
    mov ax, 4300h
    int 2Fh
    cmp al, 80h
    jne loc_0991
    mov ax, 4310h
    int 2Fh
    mov word ptr XmsDriverEntry, bx
    mov word ptr word_0B19, es
    mov ah, 8
    call dword ptr XmsDriverEntry
    mov word ptr XmsFreeKb, ax
    mov ah, 9
    mov dx, word ptr XmsFreeKb
    call dword ptr XmsDriverEntry
    or ax, ax
    je loc_0991
    mov word ptr XmsHandle, dx
loc_0991:
    xor si, si
    mov cx, 12h
loc_0996:
    push si
    mov al, byte ptr [si+ResourceSlotList]
    xor ah, ah
    mov si, ax
    cmp byte ptr cs:SbDetected, 1
    je loc_09B0
    test byte ptr [si+ResourceFlags], 8
    je loc_0A23
    jmp short loc_09C0
loc_09B0:
    test byte ptr [si+ResourceFlags], 8
    jne loc_09C0
    test word ptr cs:OptionBits, 40h
    jne loc_0A23
loc_09C0:
    mov al, byte ptr [si+ResourceSizeKb]
    xor ah, ah
    cmp word ptr EmsFreeKb, ax
    jb loc_09DD
    cmp word ptr ds:EmsHandle, 55AAh
    je loc_09DD
    call LoadResourceToEms
    sub word ptr EmsFreeKb, ax
    jmp short loc_0A23
loc_09DD:
    cmp word ptr XmsFreeKb, ax
    jb loc_09FF
    cmp word ptr ds:XmsHandle, 55AAh
    je loc_09FF
    test byte ptr [si+ResourceFlags], 10h
    jne loc_09FF
    sub word ptr XmsFreeKb, ax
    call LoadResourceToXms
    add word ptr XmsNextOffsetKb, ax
    jmp short loc_0A23
loc_09FF:
    test byte ptr [si+ResourceFlags], 2
    je loc_0A23
    test word ptr cs:OptionBits, 20h
    je loc_0A1E
    call LoadResourceFile
    mov bx, si
    shl bx, 1
    shl bx, 1
    mov word ptr [bx+ResourceAddressTable], ax
    jmp short loc_0A23
loc_0A1E:
    and byte ptr [si+ResourceFlags], 0FDh
loc_0A23:
    pop si
    inc si
    dec cx
    je loc_0A2B
    jmp near ptr loc_0996
loc_0A2B:
    ret

; Loads slot SI from disk into the heap (LoadRawFile, name from ResourceNameAt), copies it
; into EMS pages through EmsFrameSegment (INT 67h AH=44h, handle EmsHandle), sets ResourceFlags
; [SI] to location code 04h (other bits cleared), ResourceAddressTable[SI] = EmsNextOffsetKb and
; advances EmsNextOffsetKb by the size. NextFreeSegment is restored. The caller reduces EmsFreeKb.
; In: SI = slot (size from ResourceSizeKb[SI]). Out: none.
LoadResourceToEms:
    push ax
    push bx
    push cx
    push dx
    push di
    push si
    push ds
    push es
    push bp
    mov di, si
    mov cx, si
    call ResourceNameAt
    push word ptr NextFreeSegment
    call LoadRawFile
    mov word ptr NextFreeSegment, ax
    mov al, byte ptr [di+ResourceFlags]
    and al, 0F9h
    or al, 4
    mov byte ptr [di+ResourceFlags], al
    mov cl, byte ptr [di+ResourceSizeKb]
    xor ch, ch
    shl di, 1
    shl di, 1
    mov ax, word ptr EmsNextOffsetKb
    mov word ptr [di+ResourceAddressTable], ax
    mov es, word ptr EmsFrameSegment
    mov dx, word ptr EmsHandle
loc_0A6B:
    xor si, si
    mov al, 0
    mov ah, 44h
    mov bx, word ptr EmsNextOffsetKb
    mov di, bx
    shr bx, 1
    shr bx, 1
    shr bx, 1
    shr bx, 1
    and di, 0Fh
    int 67h
    sub di, 10h
    neg di
    mov bx, cx
    cmp bx, di
    jb loc_0A91
    mov bx, di
loc_0A91:
    xchg bx, cx
    xchg cl, ch
    shl cx, 1
    shl cx, 1
    mov ax, word ptr EmsNextOffsetKb
    and ax, 0Fh
    xchg al, ah
    shl ax, 1
    shl ax, 1
    mov di, ax
    mov ds, word ptr NextFreeSegment
    mov bp, cx
    rep movsb
    mov ax, seg DATA
    mov ds, ax
    mov cl, 4
    shr bp, cl
    add word ptr NextFreeSegment, bp
    mov cl, 6
    shr bp, cl
    add word ptr EmsNextOffsetKb, bp
    mov cx, bx
    sub cx, bp
    je loc_0ACC
    jmp short loc_0A6B
loc_0ACC:
    pop word ptr NextFreeSegment
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

; Loads slot SI from disk into the heap (LoadRawFile) and moves it into the XMS block at
; XmsNextOffsetKb with XMS function 0Bh (XmsDriverEntry). Sets ResourceFlags[SI] to location code 06h,
; ResourceAddressTable[SI] = XmsNextOffsetKb. The caller reduces XmsFreeKb and advances
; XmsNextOffsetKb by the size.
; In: SI = slot, AX = size in KB. Out: AX preserved.
LoadResourceToXms:
    push ax
    push bx
    push cx
    push dx
    push di
    push si
    push ds
    push es
    push bp
    mov dx, ax
    mov ax, word ptr XmsNextOffsetKb
    mov bx, si
    shl bx, 1
    shl bx, 1
    mov word ptr [bx+ResourceAddressTable], ax
    mov al, byte ptr [si+ResourceFlags]
    and al, 0F9h
    or al, 6
    mov byte ptr [si+ResourceFlags], al
    mov ax, dx
    xor dx, dx
    mov cx, 0Ah
loc_0B05:
    shl ax, 1
    rcl dx, 1
    loop loc_0B05
    mov word ptr XmsMoveLength, ax
    mov word ptr word_0B23, dx
    mov word ptr XmsSourceHandle, 0
    mov word ptr XmsSourceOffsetLow, 0
    mov ax, word ptr NextFreeSegment
    mov word ptr XmsSourceOffsetHigh, ax
    mov ax, word ptr XmsHandle
    mov word ptr word_0B2B, ax
    mov ax, word ptr XmsNextOffsetKb
    xor dx, dx
    mov cx, 0Ah
loc_0B32:
    shl ax, 1
    rcl dx, 1
    loop loc_0B32
    mov word ptr word_0B2D, ax
    mov word ptr word_0B2F, dx
    mov cx, si
    call ResourceNameAt
    push word ptr NextFreeSegment
    call LoadRawFile
    pop word ptr NextFreeSegment
    mov si, 0B21h
    mov ah, 0Bh
    inc byte ptr cs:DosCallActive
    call dword ptr XmsDriverEntry
    cmp ax, 1
    je loc_0B65
    jmp near ptr ReportXmsError
loc_0B65:
    dec byte ptr cs:DosCallActive
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

; Loads the file named by ResourceNameAt(SI) raw with LoadRawFile (not SQZ). Only reached
; through InitResourceMemory when OptionBits 20h is set, which no code path sets: effectively
; unused in the shipped configuration.
; In: SI = slot. Out: AX = segment of the file (heap top before the load); heap advanced.
LoadResourceFile:
    push cx
    push si
    mov cx, si
    call ResourceNameAt
    call LoadRawFile
    pop si
    pop cx
    ret

; Returns the file name of resource CX: DX = DS offset of the CX-th NUL-terminated name in the
; table that starts at DATA:09E0h (first name PRES.TRK).
; In: CX = index. Out: DX = name offset. Clobbers SI and CX.
ResourceNameAt:
    mov si, 9E0h
    jcxz loc_0B8F
loc_0B86:
    inc si
    cmp byte ptr [si], 0
    jne loc_0B86
    loop loc_0B86
    inc si
loc_0B8F:
    mov dx, si
    ret

; Sets DS = ES = DATA and calls SetDacBlock0To15IfFlag with DX = 287Eh (16-entry DAC table).
; Out: DS = ES = DATA. Effective result: no DAC change (see SetDacBlock0To15IfFlag).
SetStartupPalette:
    mov ax, seg DATA
    mov ds, ax
    mov es, ax
    mov dx, 287Eh
    call SetDacBlock0To15IfFlag
    ret

; Sets DAC registers 0..15 (INT 10h AX=1012h) from the 16-entry palette at the offset stored in
; word_2D00 for the level id byte_2D8A (ES:DX = DS:[word_2D00 + 2*byte_2D8A]).
; In: none. Out: DS = ES = DATA. Clobbers AX, BX, CX, DX.
SetLevelPalette:
    mov ax, seg DATA
    mov ds, ax
    mov es, ax
    mov ax, 1012h
    mov bl, byte ptr CurrentLevel
    xor bh, bh
    shl bx, 1
    mov dx, word ptr [bx+LevelPaletteTable]
    xor bx, bx
    mov cx, 10h
    int 10h
    ret

; Loops over PollInputState until a button flag (byte_2832 or byte_27E8) is set, then until both
; flags are clear again. In: none. Out: none. Callers: LevelRestart (around loc_0196) and four
; places in other modules.
WaitForButtonPressRelease:
    mov ax, seg DATA
    mov ds, ax
loc_0BC3:
    call PollInputState
    mov al, byte ptr byte_2832
    or al, byte ptr byte_27E8
    je loc_0BC3
loc_0BCF:
    call PollInputState
    mov al, byte ptr byte_27E8
    or al, byte ptr byte_2832
    jne loc_0BCF
    ret

; Sets DAC registers 0..15 (INT 10h AX=1012h) from ES:DX when OptionVByte equals 5. Only
; SetStartupPalette calls this. OptionVByte is only ever 0FFh (default) or 1 (/V), so the palette
; write never happens in the shipped code paths.
SetDacBlock0To15IfFlag:
    mov al, byte ptr cs:OptionVByte
    cmp al, 5
    jne loc_0BEE
    mov ax, 1012h
    xor bx, bx
    mov cx, 10h
    int 10h
loc_0BEE:
    ret

; Prints DX as an unsigned decimal number (0..65535) with DrawFontChar: five digit positions,
; leading zeros printed as spaces (the last digit is always printed).
; In: DX. Out: none. Clobbers AX, BX, DX; DATA:27D5h is used as the leading-zero flag.
PrintDecimalWord:
    mov byte ptr byte_27D5, 0
    mov bx, 2710h
loc_0BF7:
    mov al, 30h
loc_0BF9:
    cmp dx, bx
    jb loc_0C03
    sub dx, bx
    inc al
    jmp short loc_0BF9
loc_0C03:
    cmp al, 30h
    jne loc_0C17
    cmp bx, 1
    je loc_0C17
    cmp byte ptr ds:byte_27D5, 0
    jne loc_0C17
    mov al, 20h
    jmp short loc_0C1C
loc_0C17:
    mov byte ptr byte_27D5, 1
loc_0C1C:
    call DrawFontChar
    mov ax, bx
    push dx
    xor dx, dx
    mov bx, 0Ah
    div bx
    pop dx
    mov bx, ax
    or bx, bx
    jne loc_0BF7
    ret

; Draws the NUL-terminated string at DS:SI with DrawFontChar. In: DS:SI. Out: SI past the NUL.
; Preserves AX.
PrintAsciiz:
    push ax
loc_0C32:
    lodsb
    or al, al
    je loc_0C3C
    call DrawFontChar
    jmp short loc_0C32
loc_0C3C:
    pop ax
    ret

; Draws one character at the text cursor. In: AL = character. Cursor byte offset = word_2385 +
; word_2383 + word_2DD6; word_2383 is then incremented. A space clears a 12-row by 4-plane column
; of VGA memory (A000h, sequencer map mask all planes). Other characters: glyph index = AL - 30h,
; minus 2 when that exceeds 9. The 48-byte glyph (12 rows x 4 planes, index * 30h) is read from
; word_003D and written plane by plane. sub_452B (outside R1) sets up the VGA state.
; Preserves all registers.
DrawFontChar:
    push ax
    push bx
    push cx
    push dx
    push di
    push si
    push ds
    push es
    push bp
    mov di, word ptr ds:TextRowOffset
    add di, word ptr ds:TextByteColumn
    add di, word ptr ds:DisplayPage
    inc word ptr TextByteColumn
    cmp al, 20h
    jne loc_0C7B
    mov ax, 0A000h
    mov es, ax
    call SetCpuWriteMode
    mov dx, 3C4h
    mov ax, 0F02h
    out dx, ax
    xor al, al
    mov cx, 0Ch
loc_0C6F:
    stosb
    add di, 27h
    loop loc_0C6F
    sub di, 1E0h
    jmp short loc_0CB2
loc_0C7B:
    sub al, 30h
    cmp al, 9
    jbe loc_0C83
    sub al, 2
loc_0C83:
    mov ds, word ptr word_003D
    mov ah, 30h
    mul ah
    mov si, ax
    mov ax, 0A000h
    mov es, ax
    call SetCpuWriteMode
    mov dx, 3C4h
    mov al, 2
    out dx, al
    inc dx
    mov al, 1
loc_0C9E:
    out dx, al
    mov cx, 0Ch
loc_0CA2:
    movsb
    add di, 27h
    loop loc_0CA2
    sub di, 1E0h
    shl al, 1
    cmp al, 8
    jbe loc_0C9E
loc_0CB2:
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

; Finds the joystick on game port 201h. Samples the port 0C80h times to see which axis-line pair
; toggles (masks byte_27E6 / byte_27E7), sets byte_27E4 (bit shift for ReadJoystickTimes; FFh
; means no joystick) and calibrates the X and Y limit words (DATA:27DCh..27E2h, 27DEh, 27E0h) from
; ReadJoystickTimes readings. Up to three attempts (byte_27E5). Interrupts are disabled while
; sampling and enabled at return. Out: byte_27E4 and the limit words.
CalibrateJoystick:
    mov ax, seg DATA
    mov ds, ax
    mov byte ptr byte_27E5, 3
loc_0CC6:
    mov byte ptr byte_27D9, 0
    mov word ptr word_27DA, 320h
    mov dx, 201h
    mov cx, 0C80h
    mov ah, 3
    cli
loc_0CDA:
    in al, dx
    test al, 3
    je loc_0CE2
    and ah, 2
loc_0CE2:
    test al, 0Ch
    je loc_0CE9
    and ah, 1
loc_0CE9:
    loop loc_0CDA
    mov al, 0FFh
    test ah, 2
    je loc_0CF5
    mov al, byte ptr byte_27E7
loc_0CF5:
    test ah, 1
    je loc_0CFD
    mov al, byte ptr byte_27E6
loc_0CFD:
    mov byte ptr byte_27E4, al
    call ReadJoystickTimes
    jb loc_0D51
    cmp ax, 320h
    jae loc_0D51
    cmp bx, 320h
    jae loc_0D51
    or ax, ax
    je loc_0D51
    or bx, bx
    je loc_0D51
    mov cl, 2
    mov bp, ax
    shr ax, cl
    sub ax, bp
    neg ax
    mov word ptr word_27DC, ax
    mov ax, bp
    shr ax, 1
    add ax, bp
    mov word ptr word_27E2, ax
    mov bp, bx
    shr bx, cl
    sub bx, bp
    neg bx
    mov word ptr word_27DE, bx
    mov bx, bp
    shr bx, cl
    add bx, bp
    mov word ptr word_27E0, bx
    cmp ax, bx
    jae loc_0D4A
    mov ax, bx
loc_0D4A:
    shl ax, 1
    mov word ptr word_27DA, ax
    jmp short loc_0D69
loc_0D51:
    dec byte ptr byte_27E5
    je loc_0D64
    mov al, byte ptr byte_27E6
    xchg byte ptr byte_27E7, al
    mov byte ptr byte_27E6, al
    jmp near ptr loc_0CC6
loc_0D64:
    mov byte ptr byte_27E4, 0FFh
loc_0D69:
    sti
    ret

; Reads the joystick timing on port 201h: fires the one-shot and counts how long the X and Y
; lines stay high, up to the count word at DATA:27DAh. In: byte_27E4 = bit shift (FFh = none),
; byte_27D9 = 0. Out: CF=0 with AX = X count and BX = Y count; CF=1 when there is no joystick,
; the port is busy, or the lines do not start low. Updates the button bytes byte_27E8/27E9
; from the port bits. Interrupt flag is restored (PUSHF/POPF); CF is the result.
ReadJoystickTimes:
    pushf
    mov cl, byte ptr byte_27E4
    or cl, cl
    js loc_0DBE
    cmp byte ptr ds:byte_27D9, 0
    jne loc_0DBE
    mov dx, 201h
    in al, dx
    shr al, cl
    test al, 3
    jne loc_0DBE
    xor al, 30h
    shr al, 1
    shr al, 1
    shr al, 1
    shr al, 1
    xor bp, bp
    xor di, di
    xor bx, bx
    shr al, 1
    sbb byte ptr ds:byte_27E8, bh
    shr al, 1
    sbb byte ptr ds:byte_27E9, bh
    mov si, word ptr word_27DA
    cli
    out dx, al
loc_0DA7:
    in al, dx
    shr al, cl
    test al, 3
    je loc_0DB9
    shr al, 1
    adc di, bp
    shr al, 1
    adc bx, bp
    dec si
    jne loc_0DA7
loc_0DB9:
    mov ax, di
    popf
    clc
    ret
loc_0DBE:
    popf
    stc
    ret

; Per-frame input poll. Reads the joystick through ReadJoystickTimes and the key-derived flag
; bytes (groups at byte_2804..byte_2870) into the direction and button bytes byte_27E8..27ED and
; byte_27EA..27ED. With byte_2879 nonzero it first decodes the next run-length entry of the input
; log at DATA:003Fh (cursor word_287A); byte_2879 = 1 then stops. Otherwise live sampling
; continues and a (value, count) entry is appended to that log while word_287A < 7FCh (the log
; is seeded by the first 800h bytes of KEYB.SQZ, see ProgramEntry). Out: the bytes above and
; word_287A. All registers preserved.
PollInputState:
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
    cmp byte ptr ds:byte_2879, 0
    je loc_0E51
    cmp byte ptr ds:LastScanCode, 0
    je loc_0DE2
    mov byte ptr byte_6BE5, 1
loc_0DE2:
    mov si, word ptr word_287A
    or si, si
    je loc_0DF1
    cmp byte ptr ds:byte_287D, 0
    jne loc_0E0E
loc_0DF1:
    mov ax, word ptr [si+word_003F]
    inc si
    inc si
    cmp ax, 55AAh
    jne loc_0E01
    mov byte ptr byte_6BE5, 1
loc_0E01:
    mov word ptr word_287A, si
    mov byte ptr byte_287C, al
    mov byte ptr byte_287D, ah
    jmp short loc_0E15
loc_0E0E:
    dec byte ptr byte_287D
    mov al, byte ptr byte_287C
loc_0E15:
    xor ah, ah
    shr al, 1
    rcl ah, 1
    mov byte ptr byte_27ED, ah
    xor ah, ah
    shr al, 1
    rcl ah, 1
    mov byte ptr byte_27EC, ah
    xor ah, ah
    shr al, 1
    rcl ah, 1
    mov byte ptr byte_27EA, ah
    xor ah, ah
    shr al, 1
    rcl ah, 1
    mov byte ptr byte_27EB, ah
    xor ah, ah
    shr al, 1
    rcl ah, 1
    mov byte ptr byte_27E8, ah
    xor ah, ah
    shr al, 1
    rcl ah, 1
    mov byte ptr byte_27E9, ah
loc_0E51:
    cmp byte ptr ds:byte_2879, 1
    jne loc_0E5B
    jmp near ptr loc_0F75
loc_0E5B:
    xor ax, ax
    mov byte ptr byte_27EA, al
    mov byte ptr byte_27EB, al
    mov byte ptr byte_27EC, al
    mov byte ptr byte_27ED, al
    mov byte ptr byte_27E8, al
    mov byte ptr byte_27E9, al
    call ReadJoystickTimes
    jb loc_0EA4
    cmp ax, word ptr word_27DC
    ja loc_0E81
    mov byte ptr byte_27ED, 0FFh
    jmp short loc_0E8C
loc_0E81:
    cmp ax, word ptr word_27E2
    jb loc_0E8C
    mov byte ptr byte_27EC, 0FFh
loc_0E8C:
    cmp bx, word ptr word_27DE
    ja loc_0E99
    mov byte ptr byte_27EA, 0FFh
    jmp short loc_0EA4
loc_0E99:
    cmp bx, word ptr word_27E0
    jb loc_0EA4
    mov byte ptr byte_27EB, 0FFh
loc_0EA4:
    mov al, byte ptr byte_283C
    or al, byte ptr byte_2870
    or al, byte ptr byte_281D
    or al, byte ptr byte_2804
    or byte ptr byte_27EA, al
    mov al, byte ptr byte_2844
    or al, byte ptr byte_286F
    or al, byte ptr ds:byte_283E
    or al, byte ptr byte_2812
    or byte ptr byte_27EB, al
    mov al, byte ptr byte_2841
    or al, byte ptr byte_286D
    or al, byte ptr byte_2842
    or al, byte ptr byte_281B
    or byte ptr byte_27EC, al
    mov al, byte ptr byte_283F
    or al, byte ptr byte_286E
    or al, byte ptr byte_281F
    or al, byte ptr byte_281A
    or byte ptr byte_27ED, al
    mov al, byte ptr byte_2810
    or al, byte ptr byte_282D
    or al, byte ptr byte_2868
    or al, byte ptr byte_284B
    or byte ptr byte_27E8, al
    mov al, byte ptr byte_2840
    or byte ptr byte_27E9, al
    mov si, word ptr word_287A
    cmp si, 7FCh
    jae loc_0F75
    xor al, al
    mov ah, byte ptr byte_27E9
    shr ah, 1
    rcl al, 1
    mov ah, byte ptr byte_27E8
    shr ah, 1
    rcl al, 1
    mov ah, byte ptr byte_27EB
    shr ah, 1
    rcl al, 1
    mov ah, byte ptr byte_27EA
    shr ah, 1
    rcl al, 1
    mov ah, byte ptr byte_27EC
    shr ah, 1
    rcl al, 1
    mov ah, byte ptr byte_27ED
    shr ah, 1
    rcl al, 1
    or si, si
    jne loc_0F54
    mov ah, byte ptr CurrentLevel
    mov byte ptr ds:byte_083E, ah
    jmp short loc_0F67
loc_0F54:
    cmp byte ptr [si+word_003D], al
    jne loc_0F67
    cmp byte ptr [si+byte_003E], 0FFh
    je loc_0F67
    inc byte ptr [si+byte_003E]
    jmp short loc_0F75
loc_0F67:
    mov byte ptr [si+word_003F], al
    mov byte ptr [si+byte_0040], 0
    add word ptr word_287A, 2
loc_0F75:
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

; Reads the whole file named at DS:DX into the heap at NextFreeSegment (one INT 21h read of up to
; FFFEh bytes), closes it and advances NextFreeSegment by bytes/16 + 1. Sets byte_2878 = 1 while it
; runs. In: DS:DX = ASCIIZ name in DATA. Out: AX = segment of the data (the old NextFreeSegment).
; Open failure: loc_1012 shows the swap message with the name, waits for RETURN (sub_105F) and
; retries (byte_2878 = 1 retries this routine). Read failure: fatal read error (loc_0FFF), halt.
; Notes: the MOV BX,BX before the close is a no-op.
LoadRawFile:
    mov ax, seg DATA
    mov ds, ax
    mov byte ptr RawLoadInProgress, 1
    mov ax, 3D00h
    int 21h
    jae loc_0F93
    jmp near ptr ReportOpenError
loc_0F93:
    mov dx, word ptr NextFreeSegment
    push dx
    push ds
    mov ds, dx
    mov bx, ax
    mov cx, 0FFFEh
    mov dx, 0
    mov ah, 3Fh
    int 21h
    jb ReportReadError
    mov cl, 4
    shr ax, cl
    inc ax
    pop ds
    add word ptr NextFreeSegment, ax
    mov bx, bx
    mov ah, 3Eh
    int 21h
    pop ax
    mov byte ptr RawLoadInProgress, 0
    ret

; INT 24h handler (vector set in ProgramEntry at ds:90h/92h). Never returns to DOS: sets text
; mode, prints "FATAL: Disk missing !" (DATA:0864h) with INT 21h AH=09h, then halts in loc_104F,
; which prints "PRESS ESC TO RESET & DEBUG." and spins with interrupts on. ESC is handled by
; KeyboardIsr. Other entries reuse the halt: loc_0FD9 (DATA:087Ah, squeezed file error), loc_0FEC
; (DATA:0896h, XMS error), loc_0FFF (DATA:08C7h, read error), loc_06D5 (DATA:0840h, memory).
; Notes: the program does not restore this vector; DOS normally restores INT 22h-24h from the PSP
; at normal termination (not verified in R1).
CriticalErrorHandler:
    mov ax, 3
    int 10h
    mov ax, seg DATA
    mov ds, ax
    mov byte ptr cs:DosCallActive, 0
    mov ah, 9
    mov dx, 864h
    int 21h
    jmp short HaltWithMessage
; Fatal: prints "FATAL: squeezed file error." then HaltWithMessage. Never returns.
ReportSqzError:
    mov ax, seg DATA
    mov ds, ax
    mov ax, 3
    int 10h
    mov ah, 9
    mov dx, 87Ah
    int 21h
    jmp short HaltWithMessage
; Fatal: prints "FATAL: XMS error, use /X or another XMS driver !" then HaltWithMessage.
ReportXmsError:
    mov ax, 3
    int 10h
    mov ax, seg DATA
    mov ds, ax
    mov ah, 9
    mov dx, 896h
    int 21h
    jmp short HaltWithMessage
; Fatal: prints "FATAL: read error !" (video mode 3 first) then HaltWithMessage. Never returns.
ReportReadError:
    mov ax, 3
    int 10h
    mov ax, seg DATA
    mov ds, ax
    mov ah, 9
    mov dx, 8C7h
    int 21h
    jmp short HaltWithMessage
; Open-failure path of both loaders. Video mode 3, prints "SWAP MESSAGE: Insert the disk with "
; then the file name (DX, ASCIIZ), then WaitRetryPrompt. On RETURN it restores DX/AX/BX/CX and
; re-enters LoadRawFile when RawLoadInProgress = 1, else LoadSqzFile.
; In: DX = file name offset (DATA). AX = DOS error (unused).
; Notes: The NUL after the name is found by scanning up to 90 bytes; the byte after it is
;        temporarily replaced by '$' for the print and restored.
ReportOpenError:
    push ax
    push bx
    push cx
    push dx
    push di
    mov ax, 3
    int 10h
    mov di, dx
    push dx
    mov ah, 9
    mov dx, 8DBh
    int 21h
    push ds
    pop es
    mov cx, 5Ah
    xor ax, ax
    repne scasb
    mov bl, byte ptr [di]
    mov byte ptr [di], 24h
    pop dx
    mov ah, 9
    int 21h
    mov byte ptr [di], bl
    call WaitRetryPrompt
    pop di
    pop dx
    pop cx
    pop bx
    pop ax
    cmp byte ptr ds:RawLoadInProgress, 1
    jne loc_104D
    jmp near ptr LoadRawFile
loc_104D:
    jmp short LoadSqzFile
; Prints "PRESS ESC TO RESET & DEBUG." and spins with interrupts enabled. Never returns.
HaltWithMessage:
    mov ah, 9
    mov dx, 8FFh
    int 21h
    mov byte ptr cs:DosCallActive, 0
    sti
loc_105D:
    jmp short loc_105D

; Prints "Press RETURN to continue or ESC to cancel." and waits for the RETURN key.
; In: none. Out: jumps to sub_447D when LastScanCode = 1Ch (RETURN make code); sub_447D restores
;     the display and RETs to the caller (ReportOpenError), which then retries the load.
; Notes: Only RETURN is accepted; ESC is printed as an option but never tested here.
;        The wait is a busy loop on LastScanCode with interrupts enabled.
WaitRetryPrompt:
    mov byte ptr cs:DosCallActive, 0
    mov ah, 9
    mov dx, 91Dh
    int 21h
    mov byte ptr LastScanCode, 0
loc_1071:
    mov al, byte ptr LastScanCode
    cmp al, 1Ch
    jne loc_1071
    jmp near ptr SetLevelVideoMode

; Opens the SQZ file named at DS:DX and decompresses it into the next free segment.
; In: DX = offset (in DATA) of the ASCIIZ file name.
; Out: AX = segment of the decompressed data (the NextFreeSegment value on entry). DS = DATA.
;      NextFreeSegment += paragraphs from the header. ES = SqzOutputSegment (not reset).
; Format: first 6 bytes. B4 4C -> LzssStream; byte 1 == 10h -> LzwStream; else Huffman+RLE.
; Errors: open -> ReportOpenError; 6-byte read -> ReportReadError; no room below cs:0006 ->
;         "FATAL: Not enough memory" and halt.
; Notes: The header and the file are read with the handle kept in SqzFileHandle.
LoadSqzFile:
    mov ax, seg DATA
    mov ds, ax
    mov es, ax
    mov byte ptr cs:DosCallActive, 1
    mov ax, 3D00h
    int 21h
    mov byte ptr cs:DosCallActive, 0
    jae loc_1098
    jmp near ptr ReportOpenError
loc_1098:
    mov word ptr cs:SqzFileHandle, ax
    mov ax, word ptr NextFreeSegment
    mov word ptr cs:SqzOutputSegment, ax
    mov ds, ax
    mov byte ptr cs:DosCallActive, 1
    mov bx, word ptr cs:SqzFileHandle
    mov cx, 6
    mov dx, 41Ah
    mov ah, 3Fh
    int 21h
    jae loc_10BF
    jmp near ptr ReportReadError
loc_10BF:
    dec byte ptr cs:DosCallActive
    mov si, dx
    lodsw
    cmp ax, 4CB4h
    jne loc_10CF
    jmp near ptr LoadLzssStream
loc_10CF:
    cmp ah, 10h
    jne DecodeHuffmanRle
    jmp near ptr LoadLzwStream
; Huffman + RLE path of LoadSqzFile (the header is neither LZSS nor LZW).
; Header: bytes 0-3 = size as LE words, high word first. Bytes 4-5 = tree size (LE).
;         Tree bytes start at file offset 6, then the bit stream (MSB first).
; Out: AX = SqzOutputSegment via HuffmanStreamEnd. Output placement: SqzOutputSegment:0000.
; Notes: The size is not used to stop decoding. Stream blocks of 2800h bytes go to DS:041Ah.
;        A tree longer than 414h bytes is overwritten by the first block read.
DecodeHuffmanRle:
    mov bx, ax
    lodsw
    mov cx, 4
loc_10DD:
    shr bx, 1
    rcr ax, 1
    loop loc_10DD
    inc ax
    mov cx, word ptr [si]
    add ax, word ptr es:[2875h]
    mov word ptr es:[2875h], ax
    cmp word ptr cs:ConventionalTopSeg, ax
    ja loc_10F9
    jmp near ptr loc_06D5
loc_10F9:
    mov ds, ax
    mov ax, word ptr cs:SqzOutputSegment
    mov es, ax
    mov byte ptr cs:DosCallActive, 1
    mov bx, word ptr cs:SqzFileHandle
    mov cx, cx
    mov dx, 6
    mov ah, 3Fh
    int 21h
    jae loc_111A
    jmp near ptr ReportReadError
loc_111A:
    dec byte ptr cs:DosCallActive
    mov dl, 1
    mov word ptr ds:[0], 2
    xor di, di
loc_1129:
; Huffman main loop. An RLE symbol before any output reads ES:[FFFFh]. Window moves happen at
; loc_117C only (DI >= 4000h), so a long run can run past the window before the move.
    xor bx, bx
    call ReadHuffmanSymbol
    or bh, bh
    je loc_1178
    cmp bl, 2
    jb loc_1147
    mov cl, bl
    xor ch, ch
    mov bp, ax
    mov al, byte ptr es:[di-1]
    rep stosb
    mov ax, bp
    jmp short loc_117C
loc_1147:
    or bl, bl
    jne loc_115E
    xor bx, bx
    call ReadHuffmanSymbol
    mov bp, ax
    mov cx, bx
    mov al, byte ptr es:[di-1]
    rep stosb
    mov ax, bp
    jmp short loc_117C
loc_115E:
    xor bx, bx
    call ReadHuffmanSymbol
    mov ch, bl
    xor bx, bx
    call ReadHuffmanSymbol
    mov cl, bl
    mov bp, ax
    mov al, byte ptr es:[di-1]
    rep stosb
    mov ax, bp
    jmp short loc_117C
loc_1178:
    mov byte ptr es:[di], bl
    inc di
loc_117C:
    cmp di, 4000h
    jb loc_1129
    mov bx, es
    add bx, 3FFh
    mov es, bx
    sub di, 3FF0h
    jmp short loc_117C
; Huffman stream block refill: reads up to 2800h bytes to DS:041Ah. DS:0 = bytes left, rounded up
;      to even, reduced by 2 per 16-bit word; zero means the block is used up.
; Out: AX = next stream word (bytes swapped, MSB first), SI = 041Ah + bytes used, DL = 17 (the
;      bit step decrements it to 16 for this word).
; Notes: With an odd byte count the final word includes one byte past the data (stale memory).
;        That is why odd-length streams can emit symbols past the header size.
RefillHuffmanBuffer:
    sub word ptr ds:[0], 2
    jne loc_11CB
    push bx
    push di
    mov byte ptr cs:DosCallActive, 1
    mov bx, word ptr cs:SqzFileHandle
    mov cx, 2800h
    mov dx, 41Ah
    mov ah, 3Fh
    int 21h
    jae loc_11B3
    jmp near ptr ReportReadError
loc_11B3:
    dec byte ptr cs:DosCallActive
    mov word ptr ds:[0], ax
    or ax, ax
    pop di
    pop bx
    je HuffmanStreamEnd
    shr ax, 1
    adc word ptr ds:[0], 0
    mov si, 41Ah
loc_11CB:
    lodsw
    xchg al, ah
    mov dl, 11h

; Reads one Huffman symbol from the tree at DS:0006, consuming bits MSB first.
; In: BX = 0 (tree root); DS = work segment (tree at DS:6, see DecodeHuffmanRle).
; Out: BX = symbol (0..7FFFh). Clobbers AX, DL, CF. The bit buffer is in AX.
; Notes: Node N = two LE words at DS:6+N (bit 0) and DS:8+N (bit 1); a child word is the next node
;        offset; bit 15 set = leaf. Refills 16 bits at a time through RefillHuffmanBuffer.
;        When the stream is exhausted there is no return: HuffmanStreamEnd pops this routine's
;        return address and returns from LoadSqzFile with AX = SqzOutputSegment.
ReadHuffmanSymbol:
    dec dl
    je RefillHuffmanBuffer
    rol ax, 1
    jae loc_11DA
    inc bx
    inc bx
loc_11DA:
    mov bx, word ptr [bx+6]
    or bh, bh
    jns ReadHuffmanSymbol
    and bh, 7Fh
    ret
; End of the Huffman stream (a block read returned 0 bytes). Pops the return address of the
; ReadHuffmanSymbol call (a nonlocal exit), closes the handle and returns from LoadSqzFile.
; Out: AX = SqzOutputSegment.
HuffmanStreamEnd:
    pop ax
    mov byte ptr cs:DosCallActive, 1
    mov bx, word ptr cs:SqzFileHandle
    mov ah, 3Eh
    int 21h
    dec byte ptr cs:DosCallActive
    mov ax, seg DATA
    mov ds, ax
    mov ax, word ptr cs:SqzOutputSegment
    ret
SqzFileHandle label word
; DATA: SQZ loader: file handle (cs:1204) and output segment (cs:1206)
    db 00h, 00h
SqzOutputSegment label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
; LZW path of LoadSqzFile (header byte 1 == 10h). Seeks to 0, reads the 4-byte header to
; SqzOutputSegment:000Eh, checks (byte1 & F0h) == 10h, sizes the output, and decodes.
; In: SqzFileHandle, SqzOutputSegment. Out: AX = SqzOutputSegment, file closed.
; Notes: The seek result is not checked. Paragraphs = (size >> 4) + 1 with size =
;        ((byte0 & 0Fh) << 16) | word(byte2). Output is not size-checked; END (101h) ends it.
LoadLzwStream:
    xor dx, dx
    xor cx, cx
    mov ax, 4200h
    mov byte ptr cs:DosCallActive, 1
    int 21h
    mov byte ptr cs:DosCallActive, 0
    mov ax, seg DATA
    mov es, ax
    mov ax, word ptr cs:SqzOutputSegment
    mov ds, ax
    mov cx, 4
    mov dx, 0Eh
    mov bx, word ptr cs:SqzFileHandle
    mov ah, 3Fh
    mov byte ptr cs:DosCallActive, 1
    int 21h
    jae loc_1242
    jmp near ptr ReportReadError
loc_1242:
    mov byte ptr cs:DosCallActive, 0
    mov al, byte ptr ds:[0Fh]
    and al, 0F0h
    cmp al, 10h
    je loc_1254
    jmp near ptr LoadLzssStream
loc_1254:
    xor di, di
    mov bx, word ptr ds:[0Eh]
    mov ax, word ptr ds:[10h]
    mov cx, 4
loc_1260:
    shr bx, 1
    rcr ax, 1
    loop loc_1260
    inc ax
    add ax, word ptr es:[2875h]
    mov word ptr es:[2875h], ax
    cmp word ptr cs:ConventionalTopSeg, ax
    ja loc_127A
    jmp near ptr loc_06D5
loc_127A:
    mov ds, ax
    mov ax, word ptr cs:SqzOutputSegment
    mov es, ax
    mov word ptr ds:[0], di
    mov word ptr ds:[2], di
loc_128A:
; LZW restart after CLEAR (100h) and at stream start: resets the dictionary, reads a code.
; A first code of 101h ends the stream with empty output. Otherwise its low byte is emitted.
    call ResetLzwDictionary
    call ReadLzwCode
    cmp ax, 101h
    je loc_12A7
    mov word ptr ds:[5], ax
    mov byte ptr ds:[4], al
    stosb
loc_129C:
    call ReadLzwCode
    cmp ax, 100h
    je loc_128A
    cmp ax, 101h
loc_12A7:
    je loc_131E
    mov word ptr ds:[7], ax
    mov si, 4C10h
    cmp word ptr ds:[0Ch], ax
    ja loc_12BF
    mov dl, byte ptr ds:[4]
    mov byte ptr [si], dl
    inc si
    mov ax, word ptr ds:[5]
loc_12BF:
    or ah, ah
    je loc_12D4
    mov bx, ax
    mov dl, byte ptr [bx+1C10h]
    mov byte ptr [si], dl
    inc si
    shl bx, 1
    mov ax, word ptr [bx+2C10h]
    jmp short loc_12BF
loc_12D4:
    mov byte ptr [si], al
    inc si
    mov byte ptr ds:[4], al
loc_12DA:
    dec si
    mov al, byte ptr [si]
    stosb
    cmp di, 8000h
    jb loc_12EF
    mov ax, es
    add ax, 800h
    mov es, ax
    sub di, 8000h
loc_12EF:
    cmp si, 4C10h
    jne loc_12DA
    mov bx, word ptr ds:[0Ch]
    cmp bx, 1000h
    jae loc_1315
    mov dl, byte ptr ds:[4]
    mov byte ptr [bx+1C10h], dl
    mov dx, word ptr ds:[5]
    shl bx, 1
    mov word ptr [bx+2C10h], dx
    inc word ptr ds:[0Ch]
loc_1315:
; Reached when the next free code is 1000h or more (dictionary full): no entry is added, the
; previous code is still saved, and decoding continues at 12 bits.
    mov ax, word ptr ds:[7]
    mov word ptr ds:[5], ax
    jmp near ptr loc_129C
loc_131E:
    mov byte ptr cs:DosCallActive, 1
    mov bx, word ptr cs:SqzFileHandle
    mov ah, 3Eh
    int 21h
    dec byte ptr cs:DosCallActive
    mov ax, seg DATA
    mov ds, ax
    mov ax, word ptr cs:SqzOutputSegment
    ret

; Resets the LZW state: code width 9, next free code 102h, threshold 200h.
; In: DS = LZW work segment (the paragraph after the output block).
; Out: DS:0Ah = 200h, DS:09h = 9, DS:0Ch = 102h.
ResetLzwDictionary:
    mov word ptr ds:[0Ah], 200h
    mov byte ptr ds:[9], 9
    mov word ptr ds:[0Ch], 102h
    ret

; Reads the next LZW code (9..12 bits, MSB first) and returns it.
; In: DS = LZW work segment: DS:0Ch next free code, DS:0Ah width threshold, DS:09h width.
; Out: AX = code. CF/flags as after the last compare.
; Notes: Width grows by one when next free code == threshold (equality; threshold doubles; no
;        growth past 12 bits). Bit position DS:02h (bit 0 = DS:000Eh); bit limit DS:00h =
;        (bytes read + 2) * 8. On a short limit, the old block's last 2 bytes (DS:1C0Eh) are
;        carried to DS:000Eh and 1C00h bytes are read to DS:0010h. A 0-byte read is not detected.
ReadLzwCode:
    mov ax, word ptr ds:[0Ch]
    cmp word ptr ds:[0Ah], ax
    jne loc_1366
    cmp byte ptr ds:[9], 0Ch
    je loc_1366
    inc byte ptr ds:[9]
    shl word ptr ds:[0Ah], 1
loc_1366:
    mov bx, word ptr ds:[2]
    mov si, bx
    add bl, byte ptr ds:[9]
    adc bh, 0
    cmp word ptr ds:[0], bx
    jae loc_13CD
    mov cx, 1C00h
    mov dx, 10h
    mov bx, 10h
    sub bx, word ptr ds:[0]
    add bx, word ptr ds:[2]
    mov word ptr ds:[2], bx
    mov si, bx
    mov ax, word ptr ds:[1C0Eh]
    mov word ptr ds:[0Eh], ax
    mov bx, word ptr cs:SqzFileHandle
    mov ah, 3Fh
    mov byte ptr cs:DosCallActive, 1
    int 21h
    jae loc_13AA
    jmp near ptr ReportReadError
loc_13AA:
    dec byte ptr cs:DosCallActive
    inc ax
    inc ax
    shl ax, 1
    shl ax, 1
    shl ax, 1
    mov word ptr ds:[0], ax
    push di
    shr di, 1
    shr di, 1
    shr di, 1
    shr di, 1
    mov ax, es
    add ax, di
    mov es, ax
    pop di
    and di, 0Fh
loc_13CD:
    mov cx, si
    shr si, 1
    shr si, 1
    shr si, 1
    and cl, 7
    mov ax, word ptr [si+0Eh]
    xchg al, ah
    xor dh, dh
    shl ax, cl
    mov ch, 10h
    sub ch, cl
    sub ch, byte ptr ds:[9]
    jae loc_13F6
    neg ch
    mov cl, 8
    sub cl, ch
    mov dh, byte ptr [si+10h]
    shr dh, cl
loc_13F6:
    mov bl, byte ptr ds:[9]
    xor bh, bh
    add word ptr ds:[2], bx
    mov cl, 10h
    sub cl, bl
    shr ax, cl
    or al, dh
    ret
XmsBytesLeftLow label word
; DATA: EMS/XMS copy state: dword cs:1409, words 140D/1410/1412, mode byte 140F
    db 00h, 00h
XmsBytesLeftHigh label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
EmsNextLogicalPage label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
LzssSourceMode label byte
; UNKNOWN: not yet classified as code or data
    db 00h
LzssChunkBytes label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
LzssRefillThreshold label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
; LZSS path of LoadSqzFile (bytes 0-1 = B4 4C). Seeks to 0 (checked), reads the 17-byte header
; to SqzOutputSegment:0000 (not checked), tests the signature bytes 4..7 (9Dh 89h 64h 6Ch),
; sizes the output, reads the first stream chunk to DS:0000, and enters DecodeLzssStream.
; In: SqzFileHandle. Out: via DecodeLzssStream (AX = SqzOutputSegment, mode 0).
; Notes: Header byte 9 (00/01) is not read by any code here. Paragraphs = ((byte14>>2)<<16 |
;        word(15..16)) >> 4 + 1 (see the report). Chunk = LzssChunkBytes (loc_149D).
LoadLzssStream:
    xor dx, dx
    xor cx, cx
    mov ax, 4200h
    mov byte ptr cs:DosCallActive, 1
    int 21h
    jae loc_1428
    jmp near ptr ReportSqzError
loc_1428:
; The 17-byte header read has no carry test. A failed read is caught only if the signature at
; bytes 4..7 is wrong. Bytes 2..3 and 8..13 are never read.
    dec byte ptr cs:DosCallActive
    mov ax, seg DATA
    mov es, ax
    mov ax, word ptr cs:SqzOutputSegment
    mov ds, ax
    mov cx, 11h
    xor dx, dx
    mov bx, word ptr cs:SqzFileHandle
    mov ah, 3Fh
    inc byte ptr cs:DosCallActive
    int 21h
    dec byte ptr cs:DosCallActive
    cmp word ptr ds:[4], 899Dh
    je loc_145B
loc_1458:
    jmp near ptr ReportSqzError
loc_145B:
    cmp word ptr ds:[6], 6C64h
    jne loc_1458
    mov dl, byte ptr ds:[0Eh]
    mov ax, word ptr ds:[0Fh]
    mov cx, 4
    shr dl, 1
    shr dl, 1
loc_1471:
    shr dl, 1
    rcr ax, 1
    loop loc_1471
    inc ax
    add ax, word ptr es:[2875h]
    mov word ptr es:[2875h], ax
    mov ds, ax
    mov dx, 0F000h
    sub ax, word ptr cs:ConventionalTopSeg
    jb loc_1490
    jmp near ptr loc_06D5
loc_1490:
    neg ax
    cmp ax, 0F00h
    jae loc_149D
    mov cl, 4
    shl ax, cl
    mov dx, ax
loc_149D:
; LzssChunkBytes = min(0F000h, (cs:0006 - end) * 16); LzssRefillThreshold = chunk - 80h, which
; wraps unsigned when the chunk is under 80h (no refills then; not reachable with shipped data).
    mov word ptr cs:LzssChunkBytes, dx
    add dx, 0
    sub dx, 80h
    mov word ptr cs:LzssRefillThreshold, dx
    mov es, word ptr cs:SqzOutputSegment
    xor di, di
    mov si, 0
    inc byte ptr cs:DosCallActive
    mov bx, word ptr cs:SqzFileHandle
    mov cx, word ptr cs:LzssChunkBytes
    mov dx, 0
    mov ah, 3Fh
    int 21h
    jae loc_14D3
    jmp near ptr ReportSqzError
loc_14D3:
    dec byte ptr cs:DosCallActive

; LZSS core: decodes the bit stream at DS:SI into ES:DI until the end marker.
; In: DS:SI = stream start (SI = 0 for the file path, 11h for memory and XMS sources, the EMS
;     page offset + 11h for EMS), ES:DI = output (DI = 0), LzssSourceMode = 0..3 (section 4).
; Out: mode 0: handle closed, AX = SqzOutputSegment, DS = DATA. Modes 1..3: AX = DATA segment,
;      DS = DATA, nothing closed.
; Notes: No output size check; the end marker (short match, distance byte FFh) stops it. Copies
;        read ES:[(DI + distance) & 0FFFFh]. Callers: 04D2 (mode 1), 0533 (mode 2), 068B (mode 3).
DecodeLzssStream:
    mov dl, 10h
    lodsw
    xchg bp, ax
    xor cx, cx
    jmp short loc_14EF
loc_14E0:
    call LzssNextBitWord
    jmp short loc_14F5
loc_14E5:
    call LzssNextBitWord
    jmp short loc_1500
loc_14EA:
    call LzssNextBitWord
    jmp short loc_150C
loc_14EF:
    shr bp, 1
    dec dl
    je loc_14E0
loc_14F5:
    jae loc_14FA
    movsb
    jmp short loc_14EF
loc_14FA:
    shr bp, 1
    dec dl
    je loc_14E5
loc_1500:
    lodsb
    mov ah, 0FFh
    xchg bx, ax
    jb loc_154D
    shr bp, 1
    dec dl
    je loc_14EA
loc_150C:
    jb loc_151C
    cmp bl, bh
    je loc_152C
loc_1512:
    mov cl, 2
loc_1514:
    mov al, byte ptr es:[bx+di]
    stosb
    loop loc_1514
    jmp short loc_14EF
loc_151C:
    mov cl, 3
loc_151E:
    shr bp, 1
    dec dl
    je loc_1548
loc_1524:
    rcl bh, 1
    loop loc_151E
    dec bh
    jmp short loc_1512
loc_152C:
    jmp near ptr loc_15D7
loc_152F:
    call LzssNextBitWord
    jmp short loc_1553
loc_1534:
    call LzssNextBitWord
    jmp short loc_155B
loc_1539:
    call LzssNextBitWord
    jmp short loc_1567
loc_153E:
    call LzssNextBitWord
    jmp short loc_156F
loc_1543:
    call LzssNextBitWord
    jmp short loc_1583
loc_1548:
    call LzssNextBitWord
    jmp short loc_1524
loc_154D:
    shr bp, 1
    dec dl
    je loc_152F
loc_1553:
    rcl bh, 1
    shr bp, 1
    dec dl
    je loc_1534
loc_155B:
    jb loc_1577
    mov dh, 2
    mov cl, 3
loc_1561:
    shr bp, 1
    dec dl
    je loc_1539
loc_1567:
    jb loc_1575
    shr bp, 1
    dec dl
    je loc_153E
loc_156F:
    rcl bh, 1
    add dh, dh
    loop loc_1561
loc_1575:
    sub bh, dh
loc_1577:
    mov dh, 2
    mov cl, 4
loc_157B:
    inc dh
    shr bp, 1
    dec dl
    je loc_1543
loc_1583:
    jb loc_159A
    loop loc_157B
    shr bp, 1
    dec dl
    je loc_15C3
loc_158D:
    jae loc_159F
    inc dh
    shr bp, 1
    dec dl
    je loc_15C8
loc_1597:
    adc dh, 0
loc_159A:
    mov cl, dh
    jmp near ptr loc_1514
loc_159F:
    shr bp, 1
    dec dl
    je loc_15CD
loc_15A5:
    jb loc_15BA
    mov cl, 3
    mov dh, 0
loc_15AB:
    shr bp, 1
    dec dl
    je loc_15D2
loc_15B1:
    rcl dh, 1
    loop loc_15AB
    add dh, 9
    jmp short loc_159A
loc_15BA:
    lodsb
    mov cl, al
    add cx, 11h
    jmp near ptr loc_1514
loc_15C3:
    call LzssNextBitWord
    jmp short loc_158D
loc_15C8:
    call LzssNextBitWord
    jmp short loc_1597
loc_15CD:
    call LzssNextBitWord
    jmp short loc_15A5
loc_15D2:
    call LzssNextBitWord
    jmp short loc_15B1
loc_15D7:
; LZSS exit. Mode 0 closes the handle (loc_15E5) and returns SqzOutputSegment. Modes 1..3 return
; with AX = DATA's segment and do not close the handle (none of their own is open).
    cmp byte ptr cs:LzssSourceMode, 0
    je loc_15E5
    mov ax, seg DATA
    mov ds, ax
    ret
loc_15E5:
    mov byte ptr cs:DosCallActive, 1
    mov bx, word ptr cs:SqzFileHandle
    mov ah, 3Eh
    int 21h
    dec byte ptr cs:DosCallActive
    mov ax, seg DATA
    mov ds, ax
    mov ax, word ptr cs:SqzOutputSegment
    ret
loc_1603:
    cmp di, 0F000h
    jb loc_1614
    sub di, 8000h
    mov ax, es
    add ax, 800h
    mov es, ax
loc_1614:
    cmp si, word ptr cs:LzssRefillThreshold
    ja RefillXmsChunk
    lodsw
    xchg bp, ax
    shr dl, 1
    mov dl, 10h
    ret
; Mode-3 input refill from extended memory. Rewinds XmsSourceOffset by the unread bytes
; (LzssChunkBytes - SI), adds them back to XmsBytesLeft, then moves min(XmsBytesLeft, chunk)
; bytes with XmsDriverEntry AH=0Bh (EMB at DATA:0B21h) into DS:0000.
; Notes: Result AX != 1 -> ReportXmsError. XmsBytesLeft is a 32-bit count of unmoved bytes.
RefillXmsChunk:
    push ax
    push bx
    push cx
    push dx
    push di
    push si
    push ds
    push es
    push bp
    mov ax, word ptr cs:LzssChunkBytes
    sub si, ax
    neg si
    mov dx, seg DATA
    mov ds, dx
    sub word ptr XmsSourceOffsetLow, si
    sbb word ptr XmsSourceOffsetHigh, 0
    add word ptr cs:XmsBytesLeftLow, si
    adc word ptr cs:XmsBytesLeftHigh, 0
    cmp word ptr cs:XmsBytesLeftHigh, 0
    jne loc_165F
    cmp word ptr cs:XmsBytesLeftLow, ax
    jae loc_165F
    mov ax, word ptr cs:XmsBytesLeftLow
loc_165F:
    sub word ptr cs:XmsBytesLeftLow, ax
    sbb word ptr cs:XmsBytesLeftHigh, 0
    mov word ptr XmsMoveLength, ax
    mov si, 0B21h
    mov ah, 0Bh
    inc byte ptr cs:DosCallActive
    call dword ptr XmsDriverEntry
    cmp ax, 1
    je loc_1683
    jmp near ptr ReportXmsError
loc_1683:
    dec byte ptr cs:DosCallActive
    mov ax, word ptr XmsMoveLength
    add word ptr XmsSourceOffsetLow, ax
    adc word ptr XmsSourceOffsetHigh, 0
    pop bp
    pop es
    pop ds
    pop si
    pop di
    pop dx
    pop cx
    pop bx
    pop ax
    mov si, 0
    jmp near ptr loc_1735
loc_16A3:
    cmp di, 0F000h
    jb loc_16BE
    sub di, 8000h
    mov ax, es
    add ax, 800h
    mov es, ax
    cmp ax, word ptr cs:ConventionalTopSeg
    jb loc_16BE
    jmp near ptr loc_06D5
loc_16BE:
    cmp si, 7F80h
    jae RemapEmsPair
    lodsw
    xchg bp, ax
    shr dl, 1
    mov dl, 10h
    ret
; Mode-2 input refill: maps the next EMS logical page pair (EmsNextLogicalPage, +1) to physical
; pages 0 and 1 with INT 67h AH=44h and EmsHandle, then SI -= 4000h.
; Notes: No status check on the INT 67h calls.
RemapEmsPair:
    push ax
    push bx
    push cx
    push dx
    push ds
    mov ax, seg DATA
    mov ds, ax
    mov dx, word ptr EmsHandle
    mov bx, word ptr cs:EmsNextLogicalPage
    inc word ptr cs:EmsNextLogicalPage
    mov ax, 4400h
    int 67h
    inc bx
    mov ax, 4401h
    int 67h
    sub si, 4000h
    pop ds
    pop dx
    pop cx
    pop bx
    pop ax
    jmp short loc_1735

; Dead fragment: a copy of the LZSS bit step that falls through to LzssNextBitWord. Reached by
; no instruction (it follows an unconditional jump). Kept for the byte image.
unused_16F9:
    shr bp, 1
    dec dl
    je LzssNextBitWord
    ret

; Advances the LZSS bit reader by one 16-bit word and refreshes the window for the source mode.
; In: CF = the bit just taken from BP (carried through the refill); DL = 0.
; Out: BP = next word, DL = 16, CF = that carried bit, SI += 2 (or reset by a refill).
; Modes: 1 plain fetch. 2 (EMS): if DI >= F000h shift ES (fatal if ES >= cs:0006); if
;        SI >= 7F80h RemapEmsPair. 3 (XMS): if DI >= F000h shift ES; if SI > threshold
;        RefillXmsChunk. 0 (file): if SI >= threshold RefillFileChunk (no DI test that time);
;        else if DI >= C000h shift ES.
; Notes: Window tests run once per word, not per byte: a word's worth of output can pass the
;        threshold (at most a few hundred bytes).
LzssNextBitWord:
    rcl dl, 1
    cmp byte ptr cs:LzssSourceMode, 1
    je loc_1735
    cmp byte ptr cs:LzssSourceMode, 2
    je loc_16A3
    cmp byte ptr cs:LzssSourceMode, 3
    jne loc_171D
    jmp near ptr loc_1603
loc_171D:
    cmp si, word ptr cs:LzssRefillThreshold
    jae RefillFileChunk
    cmp di, 0C000h
    jb loc_1735
    sub di, 8000h
    mov ax, es
    add ax, 800h
    mov es, ax
loc_1735:
    lodsw
    xchg bp, ax
    shr dl, 1
    mov dl, 10h
    ret
; Mode-0 input refill: moves the unread tail (DS:SI..LzssChunkBytes) to DS:0000 and reads SI
; bytes from the file after it; SI = 0. Read failure -> ReportSqzError.
; Notes: A short read near EOF is not detected; the buffer tail then holds stale bytes.
RefillFileChunk:
    push bx
    push cx
    push dx
    push di
    mov cx, word ptr cs:LzssRefillThreshold
    add cx, 80h
    sub cx, si
    mov di, 0
    mov ax, word ptr cs:LzssChunkBytes
    sub ax, cx
    push es
    push ds
    pop es
    rep movsb
    pop es
    mov dx, di
    mov bx, word ptr cs:SqzFileHandle
    mov cx, ax
    mov ah, 3Fh
    inc byte ptr cs:DosCallActive
    int 21h
    dec byte ptr cs:DosCallActive
    jae loc_1776
    jmp near ptr ReportSqzError
loc_1776:
    mov si, 0
    pop di
    pop dx
    pop cx
    pop bx
    jmp short loc_1735

; Exchanges IVT 08h and 09h (es:0020h..0027h) with the four words at cs:Int08SwapSlot. The first
; call installs SpeakerTimerIsr (08h) and KeyboardIsr (09h) and leaves the BIOS vectors in the
; slot; the second call (quit path in KeyboardIsr) restores the BIOS vectors. Also stores the
; INT 03h segment in word_2381 and clears KeyDownTable (128 bytes at DATA:27F4h).
; In/Out: none. Interrupts are off during the exchange; flags are restored; DS and ES preserved.
SwapInt08Int09Vectors:
    push ax
    push cx
    push di
    push si
    push ds
    push es
    mov ax, seg DATA
    mov ds, ax
    xor ax, ax
    mov es, ax
    mov ax, word ptr es:[0Eh]
    mov word ptr word_2381, ax
    mov si, 27Ah
    mov di, 20h
    mov cx, 4
    pushf
    cli
    cld
loc_17A1:
    mov ax, word ptr es:[di]
    xchg word ptr cs:[si], ax
    stosw
    inc si
    inc si
    loop loc_17A1
    push ds
    pop es
    mov di, 27F4h
    xor ax, ax
    mov cx, 40h
    rep stosw
    popf
    pop es
    pop ds
    pop si
    pop di
    pop cx
    pop ax
    ret
loc_17C0:
    push ax
    push ds
    push di
    push dx
    mov ax, seg DATA
    mov ds, ax
    add word ptr word_27F0, 1
    adc word ptr word_27F2, 0
    cmp byte ptr ds:DosCallActive, 0
    jne loc_181A
    mov ax, 40h
    mov ds, ax
    dec byte ptr ds:[40h]
    jne loc_17F0
    and byte ptr ds:[3Fh], 0F0h
    mov al, 0Ch
    mov dx, 3F2h
    out dx, al
loc_17F0:
    mov di, 6Ch
    inc word ptr [di]
    jne loc_17FA
    inc word ptr [di+2]
loc_17FA:
    cmp word ptr [di+2], 18h
    jne loc_1811
    cmp word ptr [di], 0B0h
    jne loc_1811
    xor ax, ax
    mov word ptr [di], ax
    mov word ptr [di+2], ax
    inc ax
    mov word ptr [di+4], ax
loc_1811:
    mov al, 20h
    out 20h, al
    pop dx
    pop di
    pop ds
    pop ax
    iret
loc_181A:
    pop dx
    pop di
    pop ds
    pop ax
    jmp dword ptr cs:Int08SwapSlot

; INT 09h keyboard handler (installed by SwapInt08Int09Vectors). Reads port 60h into
; LastScanCode, acknowledges the keyboard (port 61h bit 7 pulse), sets KeyDownTable[code & 7Fh]
; to FFh on make and 00h on break, sends EOI, IRET. F3 (3Dh) toggles cvar_1D6E bit 0 (sound on);
; ESC (01h) with DATA:2877 = 0 runs the quit: SwapInt08Int09Vectors (restore), EMS free
; (INT 67h AH=45h, EmsHandle), XMS free (AH=0Ah), text mode, the credit line at DATA:094Ah,
; a wait on PIT ch0, zeroing of cs:0000..1937h and about 350 KB above the code, PIC unmask, and
; INT 21h AH=4Ch from interrupt context.
; Notes: the code after that INT 21h (out 64h,0FEh and a far jump to F000:FFF0) is unreachable.
KeyboardIsr:
    push ds
    push bx
    push ax
    cld
    mov ax, seg DATA
    mov ds, ax
    in al, 60h
    mov bl, al
    in al, 61h
    mov ah, al
    or al, 80h
    out 61h, al
    mov al, ah
    out 61h, al
    mov byte ptr LastScanCode, bl
    cmp bl, 1
    jne loc_184B
    cmp byte ptr cs:DosCallActive, 0
loc_184B:
    je loc_1850
    jmp near ptr loc_1976
loc_1850:
    inc byte ptr DosCallActive
    cmp byte ptr cs:SbDetected, 0
    je loc_18B3
    mov al, byte ptr cs:SoundState
    and al, 3
    cmp al, 3
    jne loc_1885
    mov byte ptr cs:SoundState, 0
    mov cl, byte ptr SbIrqLine
    mov al, 0FEh
    rol al, cl
    out 21h, al
    mov al, 20h
    out 20h, al
    mov al, byte ptr SbIrqCount
    sti
loc_187E:
    cmp byte ptr SbIrqCount, al
    je loc_187E
    cli
loc_1885:
    mov bl, byte ptr SbIrqLine
    add bl, 8
    xor bh, bh
    shl bx, 1
    shl bx, 1
    xor ax, ax
    mov es, ax
    mov ax, word ptr SbSavedIrqOffset
    mov word ptr es:[bx], ax
    mov ax, word ptr SbSavedIrqSegment
    mov word ptr es:[bx+2], ax
    mov dx, word ptr cs:SbWritePort
    mov cx, 0FFFFh
loc_18AB:
    in al, dx
    test al, 80h
    loopne loc_18AB
    mov al, 0D3h
    out dx, al
loc_18B3:
    mov ax, seg DATA
    mov ds, ax
    mov dx, word ptr EmsHandle
    cmp dx, 55AAh
    je loc_18D0
    mov bp, 3
loc_18C5:
    mov ah, 45h
    int 67h
    or ah, ah
    je loc_18D0
    dec bp
    jne loc_18C5
loc_18D0:
    mov dx, word ptr XmsHandle
    cmp dx, 55AAh
    je loc_18EA
    mov ah, 0Ah
    inc byte ptr cs:DosCallActive
    call dword ptr XmsDriverEntry
    dec byte ptr cs:DosCallActive
loc_18EA:
    mov ax, 3
    int 10h
    mov si, 94Ah
    mov cx, 1
    mov ah, 0Eh
    mov bx, 3
loc_18FA:
    lodsb
    or al, al
    je loc_1903
    int 10h
    jmp short loc_18FA
loc_1903:
    xor al, al
    out 40h, al
    out 40h, al
    mov dx, 40h
loc_190C:
    in al, dx
    mov ah, al
    in al, dx
    xchg al, ah
    cmp ax, 0F000h
    jb loc_190C
loc_1917:
    in al, dx
    mov ah, al
    in al, dx
    xchg al, ah
    cmp ax, 6000h
    ja loc_1917
    mov ax, 3
    int 10h
    call SwapInt08Int09Vectors
    mov al, 0Ah
    out 42h, al
    xor al, al
    out 42h, al
    in al, 61h
    and al, 0FCh
    out 61h, al
    mov ax, cs
    mov es, ax
    mov cx, 1938h
    xor di, di
    rep stosb
    mov di, 1976h
    mov dx, 15Eh
loc_1949:
    mov cx, 400h
    rep stosb
    sub di, 400h
    add ax, 40h
    mov es, ax
    dec dx
    jne loc_1949
    mov al, 0
    out 21h, al
    mov al, 20h
    out 20h, al
    xor al, al
    out 40h, al
    out 40h, al
    mov ax, 4C00h
    int 21h
    mov al, 0FEh
    out 64h, al
    db 0EAh, 0F0h, 0FFh, 00h, 0F0h ; ENCODED: ljmp 0xf000:0xfff0
loc_1976:
    cmp bl, 3Dh
    jne loc_19B0
    xor byte ptr cs:SoundState, 1
    cmp byte ptr cs:SbDetected, 1
    je loc_19A3
    cmp byte ptr cs:AdLibEnabled, 1
    je loc_19B0
    in al, 61h
    and al, 0FCh
    test byte ptr cs:SoundState, 1
    je loc_199F
    or al, 3
loc_199F:
    out 61h, al
    jmp short loc_19B0
loc_19A3:
    test byte ptr cs:SoundState, 1
    je loc_19B0
    call StartSbBlock
    jmp short loc_19B0
loc_19B0:
    and bl, 7Fh
    xor bh, bh
    mov al, byte ptr LastScanCode
    cbw
    not ah
    mov byte ptr [bx+KeyDownTable], ah
    mov al, 20h
    out 20h, al
    pop ax
    pop bx
    pop ds
    iret

; Unused: no caller. Copy-protection check for the original disk. Compares the first byte of the
; INT 03h handler with 0CDh (INT opcode, debugger test), probes disk sectors through
; UnusedProtectionDiskProbe and on failure shows the "run with the original disk" text (DATA:2387h)
; or resets the machine (out 64h,0FEh; far jump F000:FFF0). Erases itself with
; UnusedWipeProtectionCode. Stores the INT 03h vector in cs:1AFBh (used as the far-call slot by
; UnusedProtectionDiskIo).
UnusedProtectionCheck:
    push ax
    push bx
    push cx
    push dx
    push di
    push si
    push ds
    push es
    push bp
    cli
    in al, 21h
    push ax
    mov al, 3
    out 21h, al
    xor ax, ax
    mov ds, ax
    les di, dword ptr ds:[0Ch]
    cmp byte ptr es:[di], 0CDh
    je loc_1A16
    mov word ptr ds:[4], 0FFF0h
    mov word ptr ds:[6], 0F000h
    mov ax, word ptr ds:[0Ch]
    mov dx, word ptr ds:[0Eh]
    mov bx, ax
    shr bx, 1
    shr bx, 1
    shr bx, 1
    shr bx, 1
    add bx, dx
    cmp bx, 0C000h
    jb loc_1A16
    mov word ptr cs:[1AFBh], ax
    mov word ptr cs:[1AFDh], dx
    jmp short loc_1A1F
loc_1A16:
    mov al, 0FEh
    out 64h, al
    db 0EAh, 0F0h, 0FFh, 00h, 0F0h ; ENCODED: ljmp 0xf000:0xfff0
loc_1A1F:
    mov ax, seg DATA
    mov ds, ax
    mov es, ax
    mov byte ptr byte_23CD, 0
    call UnusedProtectionDiskProbe
    jae loc_1A95
    call UnusedProtectionDiskProbe
    jae loc_1A95
    xor byte ptr byte_23CD, 1
    call UnusedProtectionDiskProbe
    jae loc_1A95
    call UnusedProtectionDiskProbe
    jae loc_1A95
    mov ax, seg DATA
    mov ds, ax
    push cs
    pop es
    mov word ptr NextFreeSegment, 0
    mov di, 26FAh
    mov cx, 800h
    rep stosb
    call UnusedWipeProtectionCode
    xor ax, ax
    mov es, ax
    mov word ptr es:[0Ch], 0FFF0h
    mov word ptr es:[0Eh], 0F000h
    xor al, al
    out 21h, al
    mov ax, 3
    int 10h
    mov dx, 2387h
    mov ah, 9
    int 21h
    mov ah, 0Eh
    mov al, byte ptr byte_27D1
    add al, 30h
    int 10h
    mov al, 2Dh
    int 10h
    mov dx, word ptr word_27D2
    call UnusedPrintHexWord
    jmp near ptr HaltWithMessage
loc_1A95:
    call UnusedWipeProtectionCode
    cmp byte ptr ds:byte_27D4, 1
    je loc_1AAC
    mov ah, 19h
    int 21h
    cmp al, 3
    jb loc_1AAC
    mov byte ptr byte_27D4, 1
loc_1AAC:
    pop ax
    and al, 0FCh
    out 21h, al
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

; Unused: reached only from UnusedProtectionDiskProbe. For AH = 02h it calls through the far
; pointer at cs:1AFBh with PUSHF (the INT 13h-shaped call; the slot is the INT 03h vector per
; UnusedProtectionCheck, so the intended target is unclear). Other AH values use the placeholder
; far call at cs:1AF9h (encoded as lcall 0:0). In: INT 13h registers. Out: AX stored at DATA:27CFh.
UnusedProtectionDiskIo:
    cmp ah, 2
    jne loc_1AF9
    push ax
    mov ax, es
    shl ax, 1
    shl ax, 1
    shl ax, 1
    shl ax, 1
    add ax, bx
    add ax, 200h
    jae loc_1AF8
    pop ax
    push bx
    push cx
    push dx
    push di
    push si
    push ds
    push bx
    mov bx, 25CEh
    pushf
    call dword ptr cs:[1AFBh]
    mov word ptr ds:[27CFh], ax
    mov si, bx
    pop di
    cld
    push es
    pop ds
    mov cx, 100h
    rep movsb
    pop ds
    pop si
    pop di
    pop dx
    pop cx
    pop bx
    ret
loc_1AF8:
    pop ax
loc_1AF9:
    pushf
    db 9Ah, 00h, 00h, 00h, 00h ; ENCODED: lcall 0, 0
    mov word ptr ds:[27CFh], ax
    ret

; Unused: reached only from UnusedProtectionCheck. Reads sectors through UnusedProtectionDiskIo
; at several cylinder/head positions, checks the words 4C42h and 3255h in the read buffer
; (DATA:23CEh, 23D0h) and records the outcome in byte_27CE, byte_27D1 and word_27D2.
; Out: CF=0 when the signature matches, else CF=1. Clobbers AX, BX, CX, DX, DI.
UnusedProtectionDiskProbe:
    mov dl, byte ptr byte_23CD
    xor ah, ah
    call UnusedProtectionDiskIo
    mov ax, 201h
    mov bx, 23CEh
    mov cx, 2
    mov dl, byte ptr byte_23CD
    mov dh, 0
    mov byte ptr byte_27CE, 1
    call UnusedProtectionDiskIo
    jb loc_1B82
    mov byte ptr byte_27D4, 0
    mov ax, 2701h
    cmp byte ptr [bx], 0F9h
    jne loc_1B3A
    mov byte ptr byte_27D4, 1
    mov ax, 4F00h
loc_1B3A:
    mov byte ptr byte_23CC, ah
    mov byte ptr byte_23CB, al
    mov byte ptr byte_27CE, 2
    mov cl, 1
loc_1B48:
    mov ax, 201h
    mov bx, 23CEh
    mov ch, byte ptr byte_23CC
    mov dh, byte ptr byte_23CB
    call UnusedProtectionDiskIo
    jb loc_1B82
    inc cl
    cmp cl, 0Ah
    jbe loc_1B48
    mov ax, 201h
    mov cl, 0F7h
    mov dl, byte ptr byte_23CD
    call UnusedProtectionDiskIo
    mov byte ptr byte_27CE, 3
    jae loc_1B82
    mov byte ptr byte_27CE, 4
    mov word ptr word_27CF, ax
    cmp ah, 10h
    je loc_1B96
loc_1B82:
    mov al, byte ptr byte_27CE
    cmp byte ptr byte_27D1, al
    jae loc_1B94
    mov byte ptr byte_27D1, al
    mov ax, word ptr word_27CF
    mov word ptr word_27D2, ax
loc_1B94:
    stc
    ret
loc_1B96:
    mov byte ptr byte_27CE, 5
    mov ax, word ptr word_23CE
    mov word ptr word_27CF, ax
    cmp ax, 4C42h
    jne loc_1B82
    mov byte ptr byte_27CE, 6
    mov ax, word ptr word_23D0
    mov word ptr word_27CF, ax
    cmp ax, 3255h
    jne loc_1B82
    mov byte ptr byte_27CE, 9
    mov di, 25CDh
    mov al, byte ptr [di]
    xor bx, bx
loc_1BC2:
    dec bx
    cmp byte ptr [bx+di], al
    je loc_1BC2
    not bx
    mov word ptr word_27CF, bx
    cmp bx, 0A8h
    jb loc_1B82
    clc
    ret

; Unused: reached only from UnusedProtectionCheck. Prints DX as four hex digits through INT 10h
; teletype output. In: DX. Out: none.
UnusedPrintHexWord:
    mov cx, 4
loc_1BD8:
    xor al, al
    shl dx, 1
    rcl al, 1
    shl dx, 1
    rcl al, 1
    shl dx, 1
    rcl al, 1
    shl dx, 1
    rcl al, 1
    cmp al, 9
    jbe loc_1BF0
    add al, 7
loc_1BF0:
    add al, 30h
    mov ah, 0Eh
    int 10h
    loop loc_1BD8
    ret

; Unused: reached only from the protection routines. Zeroes the code bytes from cs:19C7h up to
; the return address on the stack (the caller's offset), so the protection code erases itself.
; In: return offset at [SP]. Out: none. Clobbers BP.
UnusedWipeProtectionCode:
    mov bp, sp
    mov ax, word ptr [bp]
    mov bp, 19C7h
loc_1C01:
    mov byte ptr cs:[bp], 0
    inc bp
    cmp bp, ax
    jne loc_1C01
    ret

; Sets CpuClass: 0 = 8086/8088 (shift count not masked), 2 = 80186/80286 (count masked, 386 opcode
; faults), 3 = 80386 or later. Hooks INT 06h (IVT 0018h) to CpuProbeInt06Handler and restores it.
; In: none. Out: CpuClass; DX = 02C5h (left over). Interrupts are disabled and re-enabled (STI).
ProbeCpuClass:
    cli
    push ds
    push es
    mov ax, seg DATA
    mov ds, ax
    xor ax, ax
    mov es, ax
    push word ptr es:[18h]
    push word ptr es:[1Ah]
    mov word ptr es:[18h], 1C4Bh
    mov word ptr es:[1Ah], cs
    mov al, 1
    mov cl, 20h
    shl al, cl
    cmp al, 1
    je loc_1C40
    mov byte ptr cs:CpuClass, 0
    jmp short loc_1C54
loc_1C40:
    P386
    xor eax, eax
    P8086
    mov byte ptr cs:CpuClass, 3
    jmp short loc_1C54

; INT 06h (invalid opcode) handler while ProbeCpuClass runs: sets CpuClass = 2, discards the
; three-word fault frame (ADD SP,6) and falls into ProbeCpuClass's restore code (loc_1C54).
CpuProbeInt06Handler:
    mov byte ptr cs:CpuClass, 2
    add sp, 6
loc_1C54:
    mov dx, 2C5h
    pop word ptr es:[1Ah]
    pop word ptr es:[18h]
    sti
    pop es
    pop ds
    ret

; Waits until TimerTickCount has advanced by 3 or more since word_27EE, then stores the
; new count in word_27EE. Called from sub_44FB (MODULE2).
; In: none. Out: word_27EE = TimerTickCount; byte_6BE7 incremented when the advance was exactly 3.
; Notes: busy-wait with no timeout. The advance is |word_27EE - TimerTickCount| (16-bit).
;        Only an advance of exactly 3 increments byte_6BE7; larger advances do not.
WaitSoundTicks:
    mov ax, seg DATA
    mov ds, ax
    mov byte ptr byte_6BE7, 0
loc_1C6F:
    mov ax, word ptr word_27EE
    sub ax, word ptr cs:TimerTickCount
    jns loc_1C7B
    neg ax
loc_1C7B:
    cmp ax, 3
    jb loc_1C6F
    ja loc_1C86
    inc byte ptr byte_6BE7
loc_1C86:
    mov ax, word ptr cs:TimerTickCount
    mov word ptr word_27EE, ax
    ret

; Writes AL to the DSP write port (SbWritePort), busy-waiting while bit 7 is set.
; In: AL = byte. Out: none. Preserves all registers.
; Notes: no timeout. Used after DetectSoundBlaster succeeded (SbIrqHandler, StartSbBlock).
DspWriteWait:
    push ax
    push dx
    mov ah, al
    mov dx, word ptr cs:SbWritePort
loc_1C97:
    in al, dx
    test al, 80h
    jne loc_1C97
    mov al, ah
    out dx, al
    pop dx
    pop ax
    ret

; Writes AL to the DSP with a bounded wait: 10 rounds of up to 65535 polls of bit 7
; of SbWritePort.
; In: AL = byte. Out: CF=0 written; CF=1 on timeout. Preserves AX, BX, CX, DX.
; Notes: on timeout the routine unhooks the probe ISRs if hooked (ToggleIrqProbeHooks),
;        restores the PIC mask from PicMaskSaved, sets text mode 03h, prints the message at
;        ds:0973h ("Sound Blaster does not work correctly."), then calls sub_105F (message at
;        91Dh, waits for scan code 1Ch, then jumps to sub_447D). CF=1 is therefore not a
;        clean failure return.
DspWriteChecked:
    push ax
    push dx
    push cx
    push bx
    mov ah, al
    mov dx, word ptr cs:SbWritePort
    mov bx, 0Ah
loc_1CB0:
    mov cx, 0FFFFh
loc_1CB3:
    in al, dx
    test al, 80h
    loopne loc_1CB3
    jcxz loc_1CC3
    mov al, ah
    out dx, al
    pop bx
    pop cx
    pop dx
    pop ax
    clc
    ret
loc_1CC3:
    dec bx
    jne loc_1CB0
    cmp byte ptr cs:IrqProbeHooked, 0
    je loc_1CD1
    call ToggleIrqProbeHooks
loc_1CD1:
    mov al, byte ptr PicMaskSaved
    out 21h, al
    mov ax, 3
    int 10h
    mov ah, 9
    mov dx, 973h
    int 21h
    call WaitRetryPrompt
    pop bx
    pop cx
    pop dx
    pop ax
    stc
    ret

; OPL2 (AdLib) register write: AL = register, AH = value. Writes address port 388h, then
; data port 389h, with a busy-wait after each. Loop counts depend on the CPU class byte
; cs:[5] (set by the CPU probe):
;   cs:[5] < 3 (8086/286): 5 loops after the address write, 35 after the data write.
;   cs:[5] >= 3 (386+):    40 loops after the address write, 280 after the data write.
; In: AL, AH. Out: none. Preserves all registers (flags changed).
; Notes: the OPL needs about 3.3 us after an address and 23 us after data; a port must
;        wait at least that long.
OplWriteRegister:
    push dx
    push ax
    push cx
    cmp byte ptr cs:CpuClass, 3
    jae loc_1D0C
    mov dx, 388h
    out dx, al
    mov cx, 5
loc_1CFD:
    loop loc_1CFD
    inc dx
    mov al, ah
    out dx, al
    mov cx, 23h
loc_1D06:
    loop loc_1D06
    pop cx
    pop ax
    pop dx
    ret
loc_1D0C:
    mov dx, 388h
    out dx, al
    mov cx, 28h
loc_1D13:
    loop loc_1D13
    inc dx
    mov al, ah
    out dx, al
    mov cx, 118h
loc_1D1C:
    loop loc_1D1C
    pop cx
    pop ax
    pop dx
    ret

; Detects an OPL2 by its timer flags. Sequence: reg 01h = 20h; reg 04h = 63h (both timers
; masked and started), then poll status 388h for 65535 reads expecting bits 6 and 5 clear;
; reg 04h = 23h, expect status & 60h == 40h (timer 1 expired); reg 04h = 43h, expect
; status & 60h == 20h (timer 2 expired).
; In: none. Out: CF=0 OPL2 present; CF=1 not found. Clobbers AX, CX, DX.
; Notes: timer counts (regs 02h/03h) are never written; detection uses their power-on values.
;        Timer 2 is left running (reg 04h = 43h); no IRQ is enabled (reg 04h bit 7 clear).
DetectAdLib:
    mov ax, 2001h
    call OplWriteRegister
    mov ax, 6304h
    call OplWriteRegister
    mov dx, 388h
    mov cx, 0FFFFh
loc_1D34:
    in al, dx
    and al, 60h
    jne loc_1D65
    loop loc_1D34
    mov ax, 2304h
    call OplWriteRegister
    mov cx, 0FFFFh
loc_1D44:
    in al, dx
    and al, 60h
    cmp al, 40h
    je loc_1D4F
    loop loc_1D44
    jmp short loc_1D65
loc_1D4F:
    mov ax, 4304h
    call OplWriteRegister
    mov cx, 0FFFFh
loc_1D58:
    in al, dx
    and al, 60h
    cmp al, 20h
    je loc_1D63
    loop loc_1D58
    jmp short loc_1D65
loc_1D63:
    clc
    ret
loc_1D65:
    stc
    ret
TimerTickCount label word
; DATA: timer/sound counters and flags
    db 00h, 00h
cvar_1D69 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
cvar_1D6B label byte
; UNKNOWN: not yet classified as code or data
    db 00h
AdLibEnabled label byte
; UNKNOWN: not yet classified as code or data
    db 00h
SbDetected label byte
; UNKNOWN: not yet classified as code or data
    db 00h
SoundState label byte
; UNKNOWN: not yet classified as code or data
    db 00h

; Finds a Sound Blaster and prepares 8-bit DMA playback. Steps:
;  1. Scan DSP bases 210h, 220h .. 260h: reset (base+6), read base+0Ah == AAh once
;     base+0Eh bit 7 is set. Stores SbBasePort, SbReadPort, SbWritePort, SbAckPort.
;  2. PIC mask 53h; ToggleIrqProbeHooks (counting ISRs on IRQ 2,3,5,7); 10 x WaitCrtRetrace.
;  3. TC C1h; DMA ch1 page 0 offset 0 count 17 (mode 49h); DSP 14h 10h 00h; 2 x
;     WaitCrtRetrace; unhook. Exactly one probe counter equal to 1 gives SbIrqLine
;     (= counter index k, IRQ k, INT 08h+k).
;  4. PIC mask = PicMaskSaved and not (1 shl k). INT 08h+k := SbIrqHandler (old vector
;     saved to SbSavedIrqOffset/Segment).
;  5. If cs:[3] bit 40h is clear: if (cs:[6] - SS) >= 7D00h, ModuleSegment = NextFreeSegment
;     and NextFreeSegment += 105Ch paragraphs; else message at ds:09A6h ("not enough memory"),
;     bit 40h set, no allocation (cs:[6] is an R1 word; its units are not verified here).
;  6. SFX bank (SfxBankParagraphs*16 bytes) converted in place to 6-bit; volume table built
;     at 12C1h.
;  7. DMA records for buffers A (10C9h) and B (1171h) built with LinearDmaAddress; mode 49h.
;  8. D1h speaker on; TC 89h (8403 Hz); SoundState bit 0 set.
; In: none. Out: CF=0 success; CF=1 failure, PIC mask restored and probe ISRs unhooked.
; Notes: no SB at all returns CF=1 silently; a DSP write timeout goes through
;        DspWriteChecked. Bases 270h and above are not tried. ES=0 (set by
;        ToggleIrqProbeHooks) is needed by WaitCrtRetrace.
DetectSoundBlaster:
    mov ax, seg DATA
    mov ds, ax
    mov dx, 216h
loc_1D77:
    mov al, 1
    out dx, al
    mov cx, 14h
loc_1D7D:
    jmp short loc_1D7F
loc_1D7F:
    loop loc_1D7D
    mov al, 0
    out dx, al
    and dl, 0F0h
    or dl, 0Eh
    mov cx, 7D0h
loc_1D8D:
    in al, dx
    and al, 80h
    loope loc_1D8D
    jcxz DetectSbNextBase
    and dl, 0F0h
    or dl, 0Ah
    in al, dx
    cmp al, 0AAh
    jne DetectSbNextBase
    and dl, 0F0h
    mov word ptr cs:SbBasePort, dx
    mov ax, word ptr cs:SbBasePort
    mov dx, ax
    or dl, 0Ah
    mov word ptr cs:SbReadPort, dx
    mov dx, ax
    or dl, 0Ch
    mov word ptr cs:SbWritePort, dx
    or al, 0Eh
    mov word ptr cs:SbAckPort, ax
    jmp short loc_1DD9
DetectSbNextBase:
    and dl, 0F0h
    add dl, 16h
    cmp dl, 6Fh
    jb loc_1D77
DetectSbFail:
; quirk: CF=1 return leaves SoundState bit 0 clear and the PIC mask restored (PicMaskSaved).
; quirk: the probe hooks are removed on the normal paths; a DspWriteChecked timeout does its
; quirk: own unhook and then calls sub_105F, so the caller sees CF=1 only when the DSP never
; quirk: answered.
    mov al, byte ptr PicMaskSaved
    out 21h, al
    stc
    ret
loc_1DD9:
    mov al, 53h
    out 21h, al
    call ToggleIrqProbeHooks
    mov cx, 0Ah
loc_1DE3:
    call WaitCrtRetrace
    loop loc_1DE3
    mov al, 0D1h
    call DspWriteChecked
    jb DetectSbFail
    xor bx, bx
    xor dl, dl
    mov al, 40h
    call DspWriteChecked
    jb DetectSbFail
    mov al, 0C1h
    call DspWriteChecked
    jb DetectSbFail
    mov al, 5
    out 0Ah, al
    mov al, 0
    out 0Ch, al
    mov al, 49h
    out 0Bh, al
    mov ax, bx
    out 2, al
    mov al, ah
    out 2, al
    mov al, dl
    out 83h, al
    mov ax, 10h
    out 3, al
    mov al, ah
    out 3, al
    mov al, 1
    out 0Ah, al
    mov al, 14h
    call DspWriteChecked
    jb DetectSbFail
    mov ax, 10h
    mov al, al
    call DspWriteChecked
    jb DetectSbFail
    mov al, ah
    mov al, al
    call DspWriteChecked
    jb DetectSbFail
    call WaitCrtRetrace
    call WaitCrtRetrace
    call ToggleIrqProbeHooks
    mov al, 0D3h
    call DspWriteChecked
    jb DetectSbFail
    mov si, 0E67h
    mov bx, 0FFFFh
    mov cx, 8
loc_1E59:
    cmp byte ptr [si], 1
    jne loc_1E6A
    or bx, bx
    js loc_1E65
    jmp near ptr DetectSbFail
loc_1E65:
    mov bx, 8
    sub bx, cx
loc_1E6A:
    inc si
    loop loc_1E59
    or bx, bx
    jns loc_1E74
    jmp near ptr DetectSbFail
loc_1E74:
    mov byte ptr SbIrqLine, bl
    mov cl, bl
    mov al, 1
    shl al, cl
    not al
    and al, byte ptr PicMaskSaved
    out 21h, al
    mov ax, seg DATA
    mov es, ax
    mov di, 12C1h
    xor cl, cl
loc_1E90:
    xor ch, ch
loc_1E92:
    mov al, cl
    mul ch
    shr ax, 1
    shr ax, 1
    shr ax, 1
    shr ax, 1
    shr ax, 1
    shr ax, 1
    stosb
    inc ch
    cmp ch, 40h
    jne loc_1E92
    inc cl
    cmp cl, 41h
    jne loc_1E90
    xor ax, ax
    mov es, ax
    mov bl, byte ptr SbIrqLine
    add bl, 8
    xor bh, bh
    shl bx, 1
    shl bx, 1
    cli
    mov ax, 204Dh
    xchg word ptr es:[bx], ax
    mov word ptr SbSavedIrqOffset, ax
    mov ax, cs
    xchg word ptr es:[bx+2], ax
    mov word ptr SbSavedIrqSegment, ax
    sti
    mov dx, ds
    mov cx, 10C9h
    call LinearDmaAddress
    mov byte ptr DmaPageA, cl
    mov word ptr DmaOffsetA, dx
    mov dx, ds
    mov cx, 1171h
    call LinearDmaAddress
    mov byte ptr DmaPageB, cl
    mov word ptr DmaOffsetB, dx
    test word ptr cs:OptionBits, 40h
    jne loc_1F2F
    mov ax, word ptr cs:ConventionalTopSeg
    mov dx, ss
    sub ax, dx
    cmp ax, 7D00h
    jae loc_1F23
    mov ax, 3
    int 10h
    mov ah, 9
    mov dx, 9A6h
    int 21h
    call WaitRetryPrompt
    or byte ptr cs:OptionBits, 40h
    jmp short loc_1F2F
loc_1F23:
    mov ax, word ptr NextFreeSegment
    mov word ptr ModuleSegment, ax
    add word ptr NextFreeSegment, 105Ch
loc_1F2F:
    mov ax, word ptr SfxBankSegment
    mov es, ax
    xor di, di
    mov ax, word ptr SfxBankParagraphs
    mov cl, 4
    shl ax, cl
    mov cx, ax
loc_1F3F:
    mov al, byte ptr es:[di]
    add al, 80h
    shr al, 1
    shr al, 1
    stosb
    loop loc_1F3F
    or byte ptr cs:SoundState, 1
    mov al, 0D1h
    call DspWriteChecked
    jae loc_1F5B
    jmp near ptr DetectSbFail
loc_1F5B:
    mov al, 40h
    call DspWriteChecked
    jae loc_1F65
    jmp near ptr DetectSbFail
loc_1F65:
    mov al, 89h
    call DspWriteChecked
    jae loc_1F6F
    jmp near ptr DetectSbFail
loc_1F6F:
    ret

; Waits for the start of the next retrace interval: finishes the current one, then waits
; for the next.
; Colour CRTC (base 3D4h): input status 3DAh bit 3 (vertical). Mono (3B4h): 3BAh bit 0
; (horizontal).
; In: ES = 0 (BIOS CRTC base read at 0040:0063h). Out: none. Preserves AX, DX.
; Notes: monochrome adapters are paced by horizontal retrace in this code.
WaitCrtRetrace:
    push ax
    push dx
    mov dx, word ptr es:[463h]
    add dl, 6
    mov ah, 8
    cmp dl, 0DAh
    je loc_1F83
    mov ah, 1
loc_1F83:
    in al, dx
    test al, ah
    jne loc_1F83
loc_1F88:
    in al, dx
    test al, ah
    je loc_1F88
    pop dx
    pop ax
    ret
IrqProbeHooked label byte
; DATA: toggle flag (not at 1F91)
    db 00h

; Toggles the IRQ probe: exchanges IVT entries INT 08h..0Fh (es:[20h+4*i]) with the eight
; dword entries at IrqVectorTable (ds:0E47h). Entries holding FFFF:FFFF are skipped.
; Flips IrqProbeHooked (nonzero = hooked). Calling it twice restores the vectors.
; In: none. Out: ES = 0, DS = DATA, IrqProbeHooked toggled. Clobbers AX, CX, DX, SI, DI.
; Notes: interrupts are disabled during the exchange. After the first call the table holds
;        the original BIOS vectors, which the chained probe ISRs jump to.
ToggleIrqProbeHooks:
    not byte ptr cs:IrqProbeHooked
    cli
    xor ax, ax
    mov es, ax
    mov ax, seg DATA
    mov ds, ax
    mov si, 0E47h
    mov di, 20h
    mov cx, 8
loc_1FA9:
    mov ax, word ptr [si]
    mov dx, word ptr [si+2]
    cmp ax, 0FFFFh
    jne loc_1FB8
    cmp dx, -1
    je loc_1FC4
loc_1FB8:
    xchg word ptr es:[di], ax
    mov word ptr [si], ax
    xchg word ptr es:[di+2], dx
    mov word ptr [si+2], dx
loc_1FC4:
    add di, 4
    add si, 4
    loop loc_1FA9
    sti
    ret

; IRQ-probe counter for IRQ 2 (INT 0Ah): acknowledges the SB (read SbAckPort), increments
; IrqProbeCounts[2] (ds:0E69h), sends EOI, then jumps to the original vector at ds:0E4Fh.
; In: DS must be DATA (the handler uses the interrupted DS). Out: none (the chained
; handler IRETs).
; Notes: the counter is a byte; DetectSoundBlaster needs exactly one counter equal to 1.
CountSbIrq2:
    push ax
    push dx
    mov dx, word ptr cs:SbAckPort
    in al, dx
    inc byte ptr ds:[0E69h]
    mov al, 20h
    out 20h, al
    pop dx
    pop ax
    jmp dword ptr ds:[0E4Fh]

; IRQ-probe counter for IRQ 3 (INT 0Bh): ack, IrqProbeCounts[3] (ds:0E6Ah), EOI, chain
; through ds:0E53h. In: DS = DATA. Out: none (chained handler IRETs).
CountSbIrq3:
    push ax
    push dx
    mov dx, word ptr cs:SbAckPort
    in al, dx
    inc byte ptr ds:[0E6Ah]
    mov al, 20h
    out 20h, al
    pop dx
    pop ax
    jmp dword ptr ds:[0E53h]

; IRQ-probe counter for IRQ 5 (INT 0Dh): ack, IrqProbeCounts[5] (ds:0E6Ch), EOI, chain
; through ds:0E5Bh. In: DS = DATA. Out: none (chained handler IRETs).
CountSbIrq5:
    push ax
    push dx
    mov dx, word ptr cs:SbAckPort
    in al, dx
    inc byte ptr ds:[0E6Ch]
    mov al, 20h
    out 20h, al
    pop dx
    pop ax
    jmp dword ptr ds:[0E5Bh]

; IRQ-probe counter for IRQ 7 (INT 0Fh): ack, IrqProbeCounts[7] (ds:0E6Eh), EOI, chain
; through ds:0E63h. In: DS = DATA. Out: none (chained handler IRETs).
CountSbIrq7:
; quirk: the four probe ISRs use the interrupted DS. They are safe only because detection
; quirk: runs with DS = DATA.
    push ax
    push dx
    mov dx, word ptr cs:SbAckPort
    in al, dx
    inc byte ptr ds:[0E6Eh]
    mov al, 20h
    out 20h, al
    pop dx
    pop ax
    jmp dword ptr ds:[0E63h]

; Converts segment:offset into the DMA page and offset of the 168-byte block that starts
; there. In: DX = segment, CX = offset. Out: CL = page (linear bits 16-19), DX = linear
; bits 0-15 (linear = DX*16 + CX). Preserves AX.
; Notes: if DX > FF57h (the block would cross a 64K boundary) it recomputes with DATA:1219h,
;        but callers keep the mixer buffer pointers at 10C9h/1171h, so DMA and mixer then
;        disagree. Dormant unless the low word of DATA*16+10C9h or DATA*16+1171h exceeds FF57h.
;        If DATA*16+1219h also exceeds FF57h the routine loops forever.
LinearDmaAddress:
    push ax
    rol dx, 1
    rol dx, 1
    rol dx, 1
    rol dx, 1
    mov ax, dx
    and dl, 0F0h
    and ax, 0Fh
    add dx, cx
    mov cx, ax
    adc cl, ch
    pop ax
    cmp dx, 0FF57h
    jbe loc_204C
    mov dx, seg DATA
    mov cx, 1219h
    jmp short LinearDmaAddress
loc_204C:
    ret

; SB 8-bit DMA completion ISR (vector INT 08h + SbIrqLine). Once per 168-byte block:
;  1. Ack (read SbAckPort), SbIrqCount++.
;  2. Exit unless SoundState & 3 == 3 (device ready and output started).
;  3. Swap PlayBuffer/MixBuffer and PlayDmaRecord/MixDmaRecord.
;  4. Program DMA ch1 from PlayDmaRecord; DSP 14h A7h 00h (168 bytes).
;  5. If cs:[3] bit 40h is clear: TrackerTick.
;  6. Fill MixBuffer: SFX bytes (SfxBytesLeft) or zeros, to 168 bytes.
;  7. If cs:[3] bit 40h is clear: MixChannel for channels 0-2, and channel 3 only when no
;     SFX remains.
;  8. EOI (out 20h, 20h), IRET. Registers preserved.
; In: none (IRQ context, IF=0). Out: none.
; Notes: see the R3 proposal header for the exact byte order and the overrun and stale-byte
;        cases.
SbIrqHandler:
    push ax
    push bx
    push cx
    push dx
    push di
    push si
    push ds
    push es
    push bp
    cld
    mov ax, seg DATA
    mov ds, ax
    mov es, ax
    mov dx, word ptr cs:SbAckPort
    in al, dx
    inc byte ptr SbIrqCount
    mov al, byte ptr cs:SoundState
    and al, 3
    cmp al, 3
    je SbIrqPlay
    jmp near ptr SbIrqExit
SbIrqPlay:
    mov ax, word ptr PlayBuffer
    xchg word ptr MixBuffer, ax
    mov word ptr PlayBuffer, ax
    mov ax, word ptr PlayDmaRecord
    xchg word ptr MixDmaRecord, ax
    mov word ptr PlayDmaRecord, ax
    mov bx, word ptr PlayDmaRecord
    mov al, 5
    out 0Ah, al
    mov al, 0
    out 0Ch, al
    mov ax, word ptr [bx+1]
    out 2, al
    mov al, ah
    out 2, al
    mov al, byte ptr [bx]
    out 83h, al
    mov ax, 0A7h
    out 3, al
    mov al, ah
    out 3, al
    mov al, 1
    out 0Ah, al
    mov al, 14h
    call DspWriteWait
    mov ax, 0A7h
    mov al, al
    call DspWriteWait
    mov al, ah
    mov al, al
    call DspWriteWait
    test word ptr cs:OptionBits, 40h
    jne loc_20CF
    call TrackerTick
loc_20CF:
    mov ax, seg DATA
    mov ds, ax
    mov es, ax
    mov di, word ptr ds:MixBuffer
    mov cx, word ptr SfxBytesLeft
    jcxz SfxSilenceBlock
    cmp cx, 0A8h
    jb SfxCopyBlock
    mov cx, 0A8h
SfxCopyBlock:
; quirk: odd SFX remainder n: (n-1) bytes copied, the last source byte dropped, and bytes
; quirk: A6h-A7h of the block not written.
    sub word ptr SfxBytesLeft, cx
    mov ax, cx
    mov si, word ptr SfxPlayPos
    mov ds, word ptr SfxBankSegment
    shr cx, 1
    rep movsw
    sub ax, 0A8h
    neg ax
    mov cx, ax
    xor ax, ax
    shr cx, 1
    rep stosw
    mov ax, es
    mov ds, ax
    mov word ptr SfxPlayPos, si
    jmp short loc_2119
SfxSilenceBlock:
    xor ax, ax
    mov cx, 54h
    rep stosw
loc_2119:
    test word ptr cs:OptionBits, 40h
    jne SbIrqExit
    xor bx, bx
    call MixChannel
    mov bx, 2
    call MixChannel
    mov bx, 4
    call MixChannel
    cmp word ptr ds:SfxBytesLeft, 0
    jne SbIrqExit
    mov bx, 6
    call MixChannel
SbIrqExit:
    mov al, 20h
    out 20h, al
    pop bp
    pop es
    pop ds
    pop si
    pop di
    pop dx
    pop cx
    pop bx
    pop ax
    iret

; Starts the first 168-byte block from PlayDmaRecord (no swap, no mixing): DMA ch1 is
; programmed, then DSP 14h A7h 00h. In: none. Out: none. Preserves all registers.
; Notes: callers set SoundState bit 1 afterwards. Uses DspWriteWait (no timeout).
StartSbBlock:
    push ax
    push bx
    push dx
    mov bx, word ptr PlayDmaRecord
    mov al, 5
    out 0Ah, al
    mov al, 0
    out 0Ch, al
    mov ax, word ptr [bx+1]
    out 2, al
    mov al, ah
    out 2, al
    mov al, byte ptr [bx]
    out 83h, al
    mov ax, 0A7h
    out 3, al
    mov al, ah
    out 3, al
    mov al, 1
    out 0Ah, al
    mov al, 14h
    call DspWriteWait
    mov ax, 0A7h
    mov al, al
    call DspWriteWait
    mov al, ah
    mov al, al
    call DspWriteWait
    pop dx
    pop bx
    pop ax
    ret

; Adds one MOD channel into MixBuffer (168 bytes) and advances its sample position.
; In: BX = channel offset (0, 2, 4, 6). State: ChannelPos, ChannelEnd, ChannelInstrument,
;     ChannelVolume, ChannelStep, ChannelPhase (arrays indexed by channel offset).
; Out: ChannelPos, ChannelEnd, ChannelPhase updated. Preserves BX and all registers except SP.
; Per output byte: out = (out + VolumeTable[volume*64 + sample]) mod 256, then pos advances
;   by 1 + the signed integer part of (phase + step); phase keeps its low 8 bits.
; Notes: ChannelPos FFFFh = silent. SP is used as the limit (SP = sample offset + ChannelEnd)
;        and restored at the end; interrupts must stay disabled. Wrap and overrun cases are
;        documented at MixChannelWrap, MixChannelOneShotFade and MixChannelStore.
MixChannel:
    mov word ptr MixChannelOffset, bx
    mov di, bx
    mov dx, word ptr [bx+ChannelVolume]
    shl dx, 1
    shl dx, 1
    shl dx, 1
    shl dx, 1
    shl dx, 1
    shl dx, 1
    mov bp, word ptr [bx+ChannelStep]
    mov ax, word ptr [bx+ChannelPos]
    cmp ax, 0FFFFh
    jne loc_21B5
    jmp near ptr loc_227B
loc_21B5:
    mov cx, word ptr [bx+ChannelEnd]
    sub cx, ax
    mov bx, word ptr [bx+ChannelInstrument]
    shl bx, 1
    shl bx, 1
    shl bx, 1
    shl bx, 1
    les si, dword ptr [bx+word_0BDA]
    add si, ax
    mov bx, dx
    add bx, 12C1h
    mov dx, word ptr [di+ChannelPhase]
    mov word ptr MixSavedSp, sp
    mov sp, cx
    add sp, si
    mov cx, 0A8h
    mov di, word ptr ds:MixBuffer
loc_21E6:
    lods byte ptr es:[si]
    xlat
    add byte ptr [di], al
    inc di
    add dx, bp
    mov al, dh
    xor dh, dh
    cbw
    add si, ax
    cmp si, sp
    ja MixChannelWrap
    loop loc_21E6
    jmp short MixChannelStore
MixChannelWrap:
; quirk: loop wrap (sample end inside a block, loop length > 2 bytes): si := loop-start field
; quirk: (raw BD6h, relative to the sample start, read through the sample segment, so it is
; quirk: off by the far-pointer offset); ChannelEnd not updated; the wrap iteration does not
; quirk: decrement CX (one extra output byte past the 168-byte block); the new end is stored
; quirk: at ds:(ChannelEnd + 16*instrument_index) instead of this channel's ChannelEnd.
    mov word ptr MixSavedVolumeRow, bx
    mov bx, word ptr MixChannelOffset
    mov bx, word ptr [bx+ChannelInstrument]
    shl bx, 1
    shl bx, 1
    shl bx, 1
    shl bx, 1
    mov ax, word ptr [bx+word_0BD8]
    cmp ax, 2
    jbe MixChannelOneShotFade
    mov si, word ptr [bx+word_0BD6]
    add ax, si
    mov word ptr [bx+ChannelEnd], ax
    mov bx, word ptr MixSavedVolumeRow
    jmp short loc_21E6
MixChannelOneShotFade:
; quirk: one-shot sample (loop length <= 2 bytes) ends inside the block: the previous output
; quirk: byte P is added to the following bytes as P, P-1, ... until P == 0 or the block is
; quirk: full, then the channel goes silent.
    mov al, byte ptr [di-1]
    or al, al
    je MixChannelStore
loc_2231:
    add byte ptr [di], al
    inc di
    dec ax
    je MixChannelStore
    loop loc_2231
MixChannelStore:
; quirk: at block end a looping channel uses ChannelEnd (updated correctly here, unlike
; quirk: MixChannelWrap). A channel whose ChannelEnd was not updated after a mid-block wrap
; quirk: keeps the old end for the rest of this block.
    mov sp, word ptr MixSavedSp
    mov ax, ds
    mov es, ax
    mov bx, word ptr MixChannelOffset
    mov di, bx
    mov word ptr [di+ChannelPhase], dx
    mov bx, word ptr [bx+ChannelInstrument]
    shl bx, 1
    shl bx, 1
    shl bx, 1
    shl bx, 1
    sub si, word ptr [bx+word_0BDA]
    cmp word ptr [di+ChannelEnd], si
    ja loc_2277
    mov si, 0FFFFh
    mov ax, word ptr [bx+word_0BD8]
    cmp ax, 2
    jbe loc_2277
    mov si, word ptr [bx+word_0BD6]
    add ax, si
    mov word ptr [di+ChannelEnd], ax
loc_2277:
    mov word ptr [di+ChannelPos], si
loc_227B:
    ret

; Advances the MOD sequencer by one tick (one 168-byte block, 50 Hz). Called from
; SbIrqHandler.
; 1. Volume slides: channels with a nonzero ChannelVolumeSlide get
;    ChannelVolume += slide, clamped to 0..64.
; 2. SongTickCount--; if nonzero return; otherwise SongTickCount = SongSpeed.
; 3. Processes row SongRow of pattern OrderTable[SongOrderPos] for channels 0-3
;    (ProcessChannelCell). es = ModuleSegment + pattern*64; si = SongRow*16 +
;    PatternDataOffset, latched before the channels.
; 4. SongRow++; at 64: SongOrderPos++ (kept if its low byte <= SongLength, else 0),
;    SongRow = 0.
; In/Out: state above. Clobbers all registers.
; Notes: Bxx/Dxx change the row/order used for the next row only (row pointer latched).
;        The order wrap compares only the low byte of the new position (cmp byte, al).
TrackerTick:
    xor di, di
loc_227E:
    mov ax, word ptr [di+ChannelVolumeSlide]
    or ax, ax
    je loc_229A
    add ax, word ptr [di+ChannelVolume]
    jns loc_228E
    xor ax, ax
loc_228E:
    cmp ax, 40h
    jle loc_2296
    mov ax, 40h
loc_2296:
    mov word ptr [di+ChannelVolume], ax
loc_229A:
    inc di
    inc di
    cmp di, 8
    jb loc_227E
    dec byte ptr SongTickCount
    jne loc_230F
    mov al, byte ptr SongSpeed
    mov byte ptr SongTickCount, al
    mov bx, word ptr SongOrderPos
    xor al, al
    mov ah, byte ptr [bx+OrderTable]
    inc bx
    shr ax, 1
    shr ax, 1
    add ax, word ptr ModuleSegment
    mov es, ax
    mov si, word ptr SongRow
    shl si, 1
    shl si, 1
    shl si, 1
    shl si, 1
    add si, word ptr PatternDataOffset
    xor di, di
    call ProcessChannelCell
    add si, 4
    inc di
    inc di
    call ProcessChannelCell
    add si, 4
    inc di
    inc di
    call ProcessChannelCell
    add si, 4
    inc di
    inc di
    call ProcessChannelCell
    inc word ptr SongRow
    cmp word ptr ds:SongRow, 40h
    jne loc_230F
    mov ax, word ptr SongOrderPos
    inc ax
    cmp byte ptr SongLength, al
    jae loc_2306
    xor ax, ax
loc_2306:
    mov word ptr SongOrderPos, ax
    mov word ptr SongRow, 0
loc_230F:
    ret
loc_2310:
    ret

; Decodes one 4-byte cell at ES:SI and applies it to the channel at DI (channel offset).
; In: ES:SI = cell (word0 = note index, byte2/byte3 = instrument/effect), DI = channel
;     offset.
; Out: channel fields updated (see TrackerTick notes); SI unchanged.
; Notes: ChannelVolumeSlide is cleared first, even for empty cells. Instrument-only cells
;        restart the sample at position 0 and reset volume and phase, with no note. The
;        effect is dispatched for any nonzero cell with AL = parameter.
ProcessChannelCell:
    mov word ptr [di+ChannelVolumeSlide], 0
    mov ax, word ptr es:[si]
    mov dx, word ptr es:[si+2]
    mov bx, ax
    or bx, dx
    je loc_2310
    xchg dl, dh
    mov bl, dh
    shr bl, 1
    shr bl, 1
    shr bl, 1
    shr bl, 1
    mov bh, ah
    and bh, 10h
    or bl, bh
    xor bh, bh
    and dh, 0Fh
    mov word ptr [di+ChannelEffect], dx
    or bx, bx
    je loc_236D
    dec bx
    mov word ptr [di+ChannelInstrument], bx
    shl bx, 1
    shl bx, 1
    shl bx, 1
    shl bx, 1
    mov word ptr [di+ChannelPos], 0
    mov cx, word ptr [bx+InstrumentTable]
    mov word ptr [di+ChannelEnd], cx
    mov cx, word ptr [bx+word_0BD4]
    mov word ptr [di+ChannelVolume], cx
    mov word ptr [di+ChannelPhase], 0
loc_236D:
    and ax, 7FFFh
    je loc_2382
    mov word ptr [di+ChannelNote], ax
    mov bx, ax
    shl bx, 1
    mov ax, word ptr [bx+StepTable]
    mov word ptr [di+ChannelStep], ax
loc_2382:
    mov ax, word ptr [di+ChannelEffect]
    mov bl, ah
    xor bh, bh
    shl bx, 1
    jmp word ptr [bx+EffectJumpTable]

; Effect handler for commands 0-9 and 0Eh (not implemented in PRE2). RET.
IgnoreEffect:
    ret

; Axy (cmd 0Ah): if x (high nibble of AL) is nonzero the slide is +x, else -y (16-bit).
; In: AL = parameter, DI = channel. Out: ChannelVolumeSlide[DI] set.
EffectVolumeSlide:
    mov ah, al
    shr ah, 1
    shr ah, 1
    shr ah, 1
    shr ah, 1
    je loc_23A3
    mov al, ah
    xor ah, ah
    jmp short loc_23A5
loc_23A3:
    neg ax
loc_23A5:
    mov word ptr [di+ChannelVolumeSlide], ax
    ret

; Bxx (cmd 0Bh): SongOrderPos = AL (0..255, not range-checked), SongRow = FFFFh (the next
; row is 0).
; In: AL. Out: SongOrderPos, SongRow.
EffectPositionJump:
    xor ah, ah
    mov word ptr SongOrderPos, ax
    mov word ptr SongRow, 0FFFFh
    ret

; Cxx (cmd 0Ch): ChannelVolume[DI] = min(AL, 64) (AL >= 80h gives 64).
; In: AL, DI. Out: ChannelVolume[DI].
EffectSetVolume:
    cbw
    cmp ax, 40h
    jbe loc_23BF
    mov ax, 40h
loc_23BF:
    mov word ptr [di+ChannelVolume], ax
    ret

; Dxx (cmd 0Dh): SongRow = AL - 1 (decimal, not BCD; AL = 0 gives FFFFh), SongOrderPos++.
; The next row is therefore AL. Rows above 63 read past the pattern (no range check).
; A row of 64 triggers the row-64 wrap, which adds a second order increment.
EffectPatternBreak:
    dec al
    cbw
    mov word ptr SongRow, ax
    inc word ptr SongOrderPos
    ret

; Fxx (cmd 0Fh): SongSpeed = AL, applied at the next row reload. No tempo mode: any value
; is a speed. Speed 0 reloads 0, so the countdown wraps and each row lasts 256 ticks.
EffectSetSpeed:
    mov byte ptr SongSpeed, al
    ret
CODE ends
DATA segment word public 'DATA'
; DATA: text
    db 'KEYB.SQZ', 0
; DATA: text
    db 'ALLFONTS.SQZ', 0
; DATA: text
    db 'FRONT.SQZ', 0
; DATA: text
    db 'CASTLE.SQZ', 0
ClockYear label word
; UNKNOWN: not yet classified as code or data
    db 0BCh, 07h
word_0039 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_003B label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_003D label word
; quirk: sub_0008 stores the KEYB.SQZ segment here, then resets NextFreeSegment to that segment,
; so ALLFONTS.SQZ decompresses at the same segment and this word ends as the ALLFONTS segment.
; The later store at cs:00F8 (mov ds:[3Dh], ax) runs with DS = that segment, so it writes the
; segment value into ALLFONTS offset 3Dh, not into DATA. DATA:003D already holds the same value,
; so the DATA word is right and only ALLFONTS byte 3Dh-3Eh is overwritten. The same label is used
; as a displacement base in PollInputState ([si+word_003D]); rename with care.
; UNKNOWN: not yet classified as code or data
    db 00h
byte_003E label byte
; UNKNOWN: not yet classified as code or data
    db 00h
word_003F label word
; UNKNOWN: not yet classified as code or data
    db 00h
byte_0040 label byte
; UNKNOWN: not yet classified as code or data
    db 192 dup(0)
word_0100 label word
; UNKNOWN: not yet classified as code or data
    db 1382 dup(0)
byte_0666 label byte
; UNKNOWN: not yet classified as code or data
    db 471 dup(0)
byte_083D label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_083E label byte
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
; DATA: text
    db 'FATAL: Not enough memory$'
; DATA: text
    db 'SAMPLE.SQZ', 0
; DATA: text
    db 'FATAL: Disk missing !$'
; DATA: text
    db 'FATAL: squeezed file error.$'
; DATA: text
    db 'FATAL: XMS error, use /X or another XMS driver !$'
; DATA: text
    db 'FATAL: read error !$'
; DATA: text
    db 'SWAP MESSAGE: Insert the disk with $'
; UNKNOWN: not yet classified as code or data
    db 0Ah, 0Dh
; DATA: text
    db 'PRESS ESC TO RESET & DEBUG.$'
; UNKNOWN: not yet classified as code or data
    db 0Ah, 0Dh
; DATA: text
    db 'Press RETURN to continue or ESC to cancel.$'
; DATA: text
    db '100% MACHINE LANGUAGE by ERIC ZMIRO 1992', 0
; DATA: text
    db 'SORRY: Your Sound Blaster does not work correctly.$'
; DATA: text
    db 'SORRY: There is not enough memory for SoundTracker Music.$'
; DATA: text
    db 'PRES.TRK', 0
; DATA: text
    db 'CODE.TRK', 0
; DATA: text
    db 'CARTE.TRK', 0
; DATA: text
    db 'PRESENTA.TRK', 0
; DATA: text
    db 'GLACE.TRK', 0
; DATA: text
    db 'MAP.SQZ', 0
; DATA: text
    db 'MOTIF.SQZ', 0
; DATA: text
    db 'UNION.SQZ', 0
; DATA: text
    db 'MENU.SQZ', 0
; DATA: text
    db 'MINES.TRK', 0
; DATA: text
    db 'MYSTERY.TRK', 0
; DATA: text
    db 'GAMEOVER.SQZ', 0
; DATA: text
    db 'MENU2.SQZ', 0
; DATA: text
    db 'MONSTER.TRK', 0
; DATA: text
    db 'FINAL.TRK', 0
; DATA: text
    db 'BRAVO.TRK', 0
; DATA: text
    db 'KOOL.TRK', 0
; DATA: text
    db 'BOULA.TRK', 0
ResourceSlotList label byte
; UNKNOWN: not yet classified as code or data
    db 0Dh, 05h, 06h, 01h, 02h, 07h, 11h, 0Bh, 0Ch, 00h, 0Fh, 03h
    db 08h, 09h, 0Ah, 04h, 10h, 0Eh
ResourceSizeKb label byte
; UNKNOWN: not yet classified as code or data
    db 1Eh, 04h, 05h, 2Dh, 1Ch, 24h, 0Ah, 2Dh, 15h, 16h, 1Eh, 0Bh
    db 13h, 19h, 1Bh, 17h, 1Ah, 13h
ResourceFlags label byte
; UNKNOWN: not yet classified as code or data
    db 00h, 02h, 02h, 00h, 00h, 08h, 0Ah, 08h, 08h, 00h, 00h, 08h
    db 08h, 00h, 00h, 00h, 00h, 00h
ResourceAddressTable label word
; UNKNOWN: not yet classified as code or data
    db 72 dup(0)
byte_0B16 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
XmsDriverEntry label dword
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_0B19 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
XmsNextOffsetKb label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
XmsFreeKb label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
XmsHandle label word
; UNKNOWN: not yet classified as code or data
    db 0AAh, 55h
XmsMoveLength label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_0B23 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
XmsSourceHandle label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
XmsSourceOffsetLow label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
XmsSourceOffsetHigh label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_0B2B label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_0B2D label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_0B2F label word
; UNKNOWN: not yet classified as code or data
    db 16 dup(0)
byte_0B3F label byte
; UNKNOWN: not yet classified as code or data
    db 00h
word_0B40 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 45h, 4Dh, 4Dh
    db 58h, 58h, 58h, 58h, 30h
EmsFrameSegment label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
EmsFreeKb label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
EmsHandle label word
; UNKNOWN: not yet classified as code or data
    db 0AAh, 55h
EmsNextOffsetKb label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
SfxBankSegment label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
SfxBankParagraphs label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
SbIrqCount label byte
; UNKNOWN: not yet classified as code or data
    db 00h
ModuleSegment label word
; Set at cs:1F23 (outside R1) to NextFreeSegment, then NextFreeSegment += 105Ch: a window of
; 105Ch paragraphs at the heap top that PageInResource copies resources into.
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
MixSavedSp label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
MixChannelOffset label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
EffectJumpTable label word
; Sixteen word entries, index = effect command: 0-9 and 0Eh IgnoreEffect, 0Ah
; EffectVolumeSlide, 0Bh EffectPositionJump, 0Ch EffectSetVolume, 0Dh EffectPatternBreak,
; 0Fh EffectSetSpeed.
    dw offset IgnoreEffect
    dw offset IgnoreEffect
    dw offset IgnoreEffect
    dw offset IgnoreEffect
    dw offset IgnoreEffect
    dw offset IgnoreEffect
    dw offset IgnoreEffect
    dw offset IgnoreEffect
    dw offset IgnoreEffect
    dw offset IgnoreEffect
    dw offset EffectVolumeSlide
    dw offset EffectPositionJump
    dw offset EffectSetVolume
    dw offset EffectPatternBreak
    dw offset IgnoreEffect
    dw offset EffectSetSpeed
SongSpeed label byte
; UNKNOWN: not yet classified as code or data
    db 01h
SongTickCount label byte
; UNKNOWN: not yet classified as code or data
    db 01h
SongOrderPos label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
SongRow label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
ChannelPos label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
ChannelEnd label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
ChannelInstrument label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
ChannelEffect label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
ChannelStep label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
ChannelNote label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
ChannelVolume label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
ChannelVolumeSlide label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
ChannelPhase label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
InstrumentTable label word
; DATA: instrument records: 31 x 16 bytes (see InstrumentTable comment in R3 header)
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_0BD4 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_0BD6 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_0BD8 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_0BDA label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_0BDC label word
; UNKNOWN: not yet classified as code or data
    db 486 dup(0)
SongLength label byte
; UNKNOWN: not yet classified as code or data
    db 00h
PatternCount label byte
; UNKNOWN: not yet classified as code or data
    db 00h
InstrumentCount label byte
; UNKNOWN: not yet classified as code or data
    db 00h
PatternDataOffset label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
OrderTable label byte
; DATA: song order table: OrderTable[pos] = pattern number (SongLength entries used)
; UNKNOWN: not yet classified as code or data
    db 128 dup(0)
IrqVectorTable label dword
; UNKNOWN: not yet classified as code or data
    db 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    dd CountSbIrq2
    dd CountSbIrq3
; UNKNOWN: not yet classified as code or data
    db 0FFh, 0FFh, 0FFh, 0FFh
    dd CountSbIrq5
; UNKNOWN: not yet classified as code or data
    db 0FFh, 0FFh, 0FFh, 0FFh
    dd CountSbIrq7
IrqProbeCounts label byte
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
PicMaskSaved label byte
; UNKNOWN: not yet classified as code or data
    db 00h
SbIrqLine label byte
; UNKNOWN: not yet classified as code or data
    db 00h
SbSavedIrqOffset label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
SbSavedIrqSegment label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
PeriodTable label word
; UNKNOWN: not yet classified as code or data
    db 58h, 03h, 28h, 03h, 0FAh, 02h, 0D0h, 02h, 0A6h, 02h
    db 80h, 02h, 5Ch, 02h, 3Ah, 02h, 1Ah, 02h, 0FCh, 01h, 0E0h, 01h
    db 0C5h, 01h, 0ACh, 01h, 94h, 01h, 7Dh, 01h, 68h, 01h, 53h, 01h
    db 40h, 01h, 2Eh, 01h, 1Dh, 01h, 0Dh, 01h, 0FEh, 00h, 0F0h, 00h
    db 0E2h, 00h, 0D6h, 00h, 0CAh, 00h, 0BEh, 00h, 0B4h, 00h, 0AAh, 00h
    db 0A0h, 00h, 97h, 00h, 8Fh, 00h, 87h, 00h, 7Fh, 00h, 78h, 00h
StepTable label word
; DATA: note step table: 37 signed words, StepTable[n] for note n = 1..36 (index 0 unused)
; UNKNOWN: not yet classified as code or data
    db 71h, 00h, 7Fh, 0FFh, 87h, 0FFh, 8Fh, 0FFh, 98h, 0FFh, 0A1h, 0FFh
    db 0ABh, 0FFh, 0B5h, 0FFh, 0C0h, 0FFh, 0CBh, 0FFh, 0D7h, 0FFh, 0E4h, 0FFh
    db 0F1h, 0FFh, 00h, 00h, 0Fh, 00h, 1Fh, 00h, 30h, 00h, 43h, 00h
    db 56h, 00h, 6Ah, 00h, 80h, 00h, 97h, 00h, 0AFh, 00h, 0C8h, 00h
    db 0E4h, 00h, 00h, 01h, 1Eh, 01h, 40h, 01h, 60h, 01h, 84h, 01h
    db 0ACh, 01h, 0D5h, 01h, 0FEh, 01h, 2Bh, 02h, 5Eh, 02h, 91h, 02h
    db 0C9h, 02h
; DATA: text
    db '?72/,*(', 27h, '&$'
; UNKNOWN: not yet classified as code or data
    db 23h, 22h, 21h, 21h, 20h, 1Fh, 1Eh, 1Eh, 1Dh, 1Ch, 1Ch, 1Bh
    db 1Bh, 1Ah, 1Ah, 19h, 19h, 19h, 18h, 18h, 17h, 17h, 17h, 16h
    db 16h, 16h, 15h, 15h, 15h, 15h, 14h, 14h, 14h, 14h, 13h, 13h
    db 13h, 13h, 12h, 12h, 12h, 12h, 11h, 11h, 11h, 11h, 11h, 10h
    db 10h, 10h, 10h, 10h, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Eh, 0Eh
    db 0Eh, 0Eh, 0Eh, 0Eh, 0Dh, 0Dh, 0Dh, 0Dh, 0Dh, 0Dh, 0Dh, 0Ch
    db 0Ch, 0Ch, 0Ch, 0Ch, 0Ch, 0Ch, 0Ch, 0Bh, 0Bh, 0Bh, 0Bh, 0Bh
    db 0Bh, 0Bh, 0Bh, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah
    db 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 08h, 08h, 08h
    db 08h, 08h, 08h, 08h, 08h, 08h, 08h, 08h, 07h, 07h, 07h, 07h
    db 07h, 07h, 07h, 07h, 07h, 07h, 07h, 07h, 06h, 06h, 06h, 06h
    db 06h, 06h, 06h, 06h, 06h, 06h, 06h, 06h, 06h, 05h, 05h, 05h
    db 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 04h, 04h
    db 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h
    db 04h, 04h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h
    db 03h, 03h, 03h, 03h, 03h, 03h, 02h, 02h, 02h, 02h, 02h, 02h
    db 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h
    db 01h, 01h, 01h, 01h, 01h, 01h, 01h, 01h, 01h, 01h, 01h, 01h
    db 01h, 01h, 01h, 01h, 01h, 01h, 01h, 01h
; UNKNOWN: not yet classified as code or data
    db 21 dup(0)
SfxPlayPos label word
; UNKNOWN: not yet classified as code or data
    db 0FFh, 0FFh
SfxBytesLeft label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 0FFh
word_1009 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_100B label word
; UNKNOWN: not yet classified as code or data
    db 8Eh, 18h, 00h, 00h, 80h, 1Ch, 00h, 00h, 5Eh, 23h, 00h, 00h
    db 0E6h, 19h, 00h, 00h, 0B2h, 0Ah, 00h, 00h, 12h, 09h, 00h, 00h
    db 00h, 00h, 00h, 00h, 0D2h, 35h, 00h, 00h, 0C4h, 06h, 00h, 00h
    db 86h, 1Ch, 00h, 00h, 2Eh, 0Eh
SpeakerSequencePtr label word
; UNKNOWN: not yet classified as code or data
    db 0FFh, 0FFh, 70h, 17h, 70h, 17h, 08h, 00h, 9Ch, 0FFh, 08h, 00h
    db 40h, 1Fh, 40h, 1Fh, 0Ah, 00h, 2Ch, 01h, 0Ah, 00h, 0E8h, 03h
    db 0E8h, 03h, 12h, 00h, 0D4h, 0FEh, 12h, 00h, 40h, 1Fh, 40h, 1Fh
    db 08h, 00h, 64h, 00h, 08h, 00h, 0E8h, 03h, 0E8h, 03h, 06h, 00h
    db 14h, 00h, 06h, 00h, 0Ah, 00h, 0Ah, 00h, 03h, 00h, 08h, 00h
    db 03h, 00h, 0Ah, 00h, 0Ah, 00h, 03h, 00h, 0F8h, 0FFh, 03h, 00h
    db 0E8h, 03h, 0E8h, 03h, 0Ah, 00h, 28h, 00h, 0Ah, 00h, 0E8h, 03h
    db 0E8h, 03h, 0Ah, 00h, 28h, 00h, 0Ah, 00h, 40h, 1Fh, 40h, 1Fh
    db 0Ah, 00h, 2Ch, 01h, 0Ah, 00h, 0E8h, 03h, 0E8h, 03h, 0Ah, 00h
    db 28h, 00h, 0Ah, 00h, 0E8h, 03h, 0E8h, 03h, 0Ah, 00h, 14h, 00h
    db 0Ah, 00h, 0E8h, 03h, 0E8h, 03h, 0Ah, 00h, 28h, 00h, 0Ah, 00h
MixSavedVolumeRow label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
DmaPageA label byte
; UNKNOWN: not yet classified as code or data
    db 00h
DmaOffsetA label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
DmaPageB label byte
; UNKNOWN: not yet classified as code or data
    db 00h
DmaOffsetB label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
PlayBuffer label word
; UNKNOWN: not yet classified as code or data
    db 0C9h, 10h
PlayDmaRecord label word
; UNKNOWN: not yet classified as code or data
    db 0BBh, 10h
MixBuffer label word
; UNKNOWN: not yet classified as code or data
    db 71h, 11h
MixDmaRecord label word
; UNKNOWN: not yet classified as code or data
    db 0BEh, 10h
; UNKNOWN: not yet classified as code or data
    db 504 dup(0)
; DATA: volume table: VolumeTable[v*64+s] = (v*s)>>6, v = 0..64, s = 0..63
    db 3159 dup(0)
word_1F18 label word
; UNKNOWN: not yet classified as code or data
    db 1001 dup(0)
; UNKNOWN: not yet classified as code or data
    db 2Dh, 2Dh, 31h, 32h, 33h, 34h, 35h, 36h, 37h, 38h, 39h, 30h
    db 2Dh, 2Dh, 2Dh, 2Dh, 41h, 2Dh, 45h, 2Dh, 2Dh, 2Dh, 2Dh, 2Dh
    db 2Dh, 2Dh, 2Dh, 2Dh, 2Dh, 2Dh, 41h, 2Dh, 44h, 46h, 2Dh, 2Dh
    db 2Dh, 2Dh, 2Dh, 2Dh, 2Dh, 2Dh, 2Dh, 2Dh, 2Dh, 2Dh, 43h, 2Dh
    db 42h, 2Dh, 2Dh, 2Dh, 2Dh, 2Dh, 2Dh, 2Dh, 2Dh, 2Dh, 2Dh, 2Dh
    db 2Dh, 2Dh, 2Dh, 2Dh, 2Dh, 2Dh, 2Dh, 2Dh, 2Dh, 2Dh, 2Dh, 2Dh
    db 2Dh, 2Dh, 2Dh, 2Dh, 2Dh, 2Dh, 2Dh, 2Dh, 2Dh, 2Dh, 2Dh, 2Dh
    db 2Dh, 2Dh, 2Dh, 2Dh, 2Dh, 2Dh, 2Dh, 2Dh, 2Dh, 2Dh, 2Dh, 2Dh
    db 2Dh, 2Dh, 2Dh, 2Dh, 2Dh, 2Dh, 2Dh, 2Dh, 2Dh, 2Dh, 2Dh, 2Dh
    db 2Dh, 2Dh, 2Dh, 2Dh, 2Dh, 2Dh, 2Dh, 2Dh, 2Dh, 2Dh, 2Dh, 2Dh
    db 2Dh, 2Dh, 2Dh, 2Dh, 2Dh, 2Dh, 2Dh, 2Dh
word_2381 label word
; Written by SwapInt08Int09Vectors (INT 03h segment) and never read.
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
TextByteColumn label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
TextRowOffset label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
; DATA: text
    db 'YOU MUST RUN THE GAME WITH THE ORIGINAL DISK #1 IN ANY DRIVE! err.#$'
byte_23CB label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_23CC label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_23CD label byte
; UNKNOWN: not yet classified as code or data
    db 00h
word_23CE label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_23D0 label word
; UNKNOWN: not yet classified as code or data
    db 1022 dup(0)
byte_27CE label byte
; UNKNOWN: not yet classified as code or data
    db 00h
word_27CF label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
byte_27D1 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
word_27D2 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
byte_27D4 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_27D5 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_27D6 label byte
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
byte_27D8 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_27D9 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
word_27DA label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_27DC label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_27DE label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_27E0 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_27E2 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
byte_27E4 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_27E5 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_27E6 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_27E7 label byte
; UNKNOWN: not yet classified as code or data
    db 02h
byte_27E8 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_27E9 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_27EA label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_27EB label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_27EC label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_27ED label byte
; UNKNOWN: not yet classified as code or data
    db 00h
word_27EE label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_27F0 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_27F2 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
KeyDownTable label byte
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
byte_27F6 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_27F7 label byte
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    db 00h
byte_2804 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_2805 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_2806 label byte
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 00h, 00h, 00h, 00h, 00h
byte_280D label byte
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 00h
byte_2810 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_2811 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_2812 label byte
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
byte_281A label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_281B label byte
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
byte_281D label byte
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
byte_281F label byte
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    db 00h
byte_282C label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_282D label byte
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
byte_282F label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_2830 label byte
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
byte_2832 label byte
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
byte_283C label byte
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
byte_283E label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_283F label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_2840 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_2841 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_2842 label byte
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
byte_2844 label byte
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 00h, 00h, 00h, 00h, 00h
byte_284B label byte
; UNKNOWN: not yet classified as code or data
    db 29 dup(0)
byte_2868 label byte
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 00h, 00h, 00h
byte_286D label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_286E label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_286F label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_2870 label byte
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 00h, 00h
LastScanCode label byte
; UNKNOWN: not yet classified as code or data
    db 00h
NextFreeSegment label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
DosCallActive label byte
; UNKNOWN: not yet classified as code or data
    db 00h
RawLoadInProgress label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_2879 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
word_287A label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
byte_287C label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_287D label byte
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 3Ch, 00h, 00h, 3Ch
    db 00h, 00h, 3Ch, 00h, 3Ch, 3Ch, 1Ch, 3Ch, 3Ch, 0Ch, 3Ch, 38h
    db 00h, 38h, 30h, 00h, 34h, 2Ch, 00h, 34h, 24h, 00h, 2Ch, 24h
    db 00h, 24h, 1Ch, 00h, 20h, 18h, 00h, 18h, 14h, 00h, 14h, 10h
    db 00h
PictureSegment label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 00h, 01h, 02h, 03h, 04h, 05h, 06h, 07h, 08h, 09h
    db 0Ah, 0Bh, 0Ch, 0Dh, 0Eh, 0Fh, 00h
RandomSeed label word
; UNKNOWN: not yet classified as code or data
    db 34h, 12h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
; DATA: text
    db 'YEAAA > > >', 0
; DATA: text
    db 'MY GAME IS STILL WORKING IN', 0
; UNKNOWN: not yet classified as code or data
    db 20h, 3Ch, 3Ch, 00h
; DATA: text
    db 'PROGRAMMED IN 1992 ON AT >286 12MHZ>', 0
; DATA: text
    db '> > > ENJOY OLDIES<<', 0
; DATA: text
    db 'CODER> DESIGNER AND ARTIST DIRECTOR>', 0
; DATA: text
    db 'ERIC ZMIRO', 0
; DATA: text
    db '>MAIN GRAPHICS AND BACKGROUND>', 0
; DATA: text
    db 'FRANCIS FOURNIER', 0
; DATA: text
    db '>MONSTERS AND HEROS>', 0
; DATA: text
    db 'LYES  BELAIDOUNI', 0
; DATA: text
    db 'THANKS TO', 0
; DATA: text
    db 'CRISTELLE> GIL ESPECHE AND CORINNE>', 0
; DATA: text
    db 'SEBASTIEN BECHET AND OLIVIER AKA DELTA>', 0
; DATA: text
    db 'LEVELI.SQZ', 0
; DATA: text
    db 'LEVELH.SQZ', 0
; DATA: text
    db 'LEVELI.SQZ', 0
; UNKNOWN: not yet classified as code or data
    db 00h
; DATA: text
    db 'SPRITES.SQZ', 0
; DATA: text
    db 'FATAL! : Video not 100% compatible VGA !$'
; UNKNOWN: not yet classified as code or data
    db 00h, 01h, 02h, 03h, 04h, 05h, 06h, 07h, 08h, 09h, 0Ah, 0Bh
    db 0Ch, 0Dh, 0Eh, 0Fh, 00h
CollectTotalMain label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_2A76 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
CollectTotalDecor label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_2A7A label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 00h, 00h, 00h, 38h, 28h, 20h, 28h, 18h, 10h, 20h
    db 10h, 08h, 10h, 28h, 38h, 28h, 00h, 08h, 05h, 05h, 05h, 38h
    db 30h, 08h, 00h, 18h, 10h, 08h, 20h, 0Ch, 08h, 30h, 18h, 10h
    db 10h, 10h, 18h, 18h, 18h
; DATA: text
    db '   (((888', 0
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 38h, 28h, 20h, 28h, 18h, 10h, 20h, 10h, 08h, 18h
    db 18h, 24h, 28h, 00h, 08h, 00h, 00h, 00h, 38h, 30h, 08h, 18h
    db 1Ch, 2Ch, 18h, 24h, 2Ch, 28h, 30h, 38h, 34h, 20h, 14h, 00h
    db 18h, 14h, 00h, 24h, 1Ch, 14h
; DATA: text
    db '4,<<8', 0
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 38h, 28h, 20h, 28h, 18h, 10h, 20h, 10h, 08h, 0Ch
    db 08h, 14h, 28h, 00h, 08h, 00h, 00h, 00h, 38h, 30h, 08h, 00h
    db 0Eh, 00h, 00h, 14h, 00h, 00h, 1Ah, 00h, 0Ch, 0Ah, 02h, 14h
    db 18h, 14h, 1Ch
; DATA: text
    db '  ,(,<<8', 0
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 38h, 28h, 20h, 28h, 18h, 10h, 20h, 10h, 08h, 10h
    db 28h, 38h, 28h, 00h, 08h, 04h, 04h, 00h, 38h, 30h, 08h, 18h
    db 00h, 00h, 28h, 04h, 00h, 3Ch, 14h, 00h, 34h, 20h, 14h, 20h
    db 14h, 24h, 28h, 1Ch
; DATA: text
    db '<,,<48<', 0
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 38h, 28h, 20h, 28h, 18h, 10h, 20h, 10h, 08h, 10h
    db 28h, 38h, 28h, 00h, 08h, 04h, 04h, 04h, 38h, 30h, 08h, 00h
    db 10h, 10h, 00h, 20h, 10h, 08h, 30h, 08h, 14h, 08h, 00h, 00h
    db 18h, 14h, 00h, 24h, 1Ch, 14h
; DATA: text
    db '4,<<<', 0
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 3Ch, 24h, 14h, 2Ch, 0Ch, 00h, 1Ch, 08h, 04h
; DATA: text
    db '<0((', 0
; UNKNOWN: not yet classified as code or data
    db 08h, 04h, 04h, 04h, 3Ch, 2Ch, 08h, 08h, 18h, 00h, 18h, 20h
    db 0Ch, 28h, 2Ch, 10h, 34h, 18h, 10h, 24h, 08h, 18h, 2Ch, 18h
    db 24h
; DATA: text
    db '4$'
; DATA: text
    db '0<48', 0
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 38h, 28h, 20h, 28h, 18h, 10h, 20h, 10h, 08h, 10h
    db 28h, 38h, 28h, 00h, 08h, 04h, 04h, 04h, 38h, 30h, 08h, 00h
    db 14h, 14h, 00h, 20h, 14h, 08h, 30h, 1Ch, 34h, 20h, 14h, 04h
    db 08h, 1Ch, 10h, 14h, 30h, 14h, 24h
; DATA: text
    db '8,4<', 0
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 38h, 28h, 20h, 28h, 18h, 10h, 20h, 10h, 08h
; DATA: text
    db '(4<(', 0
; UNKNOWN: not yet classified as code or data
    db 08h, 04h, 04h, 04h, 38h, 30h, 08h, 00h, 14h, 18h, 00h, 20h
    db 1Ch, 00h, 30h, 28h, 08h, 14h, 0Ch, 14h, 1Ch, 14h
; DATA: text
    db ' ( (0(8<8', 0
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 38h, 28h, 20h, 28h, 18h, 10h, 20h, 10h, 08h, 24h
    db 28h, 3Ch, 28h, 00h, 08h, 04h, 04h, 04h, 38h, 30h, 08h, 00h
    db 18h, 10h, 08h, 20h, 0Ch, 08h, 30h, 18h, 14h, 10h, 0Ch, 1Ch
    db 18h, 14h, 24h, 20h, 1Ch, 2Ch, 28h, 24h, 3Ch, 38h, 34h, 00h
    db 00h, 00h, 38h, 28h, 20h, 28h, 18h, 10h, 20h, 10h, 08h
; DATA: text
    db ',(<(', 0
; UNKNOWN: not yet classified as code or data
    db 08h, 04h, 04h, 04h, 38h, 30h, 08h, 1Ch, 00h, 14h, 2Ch, 14h
    db 1Ch, 3Ch, 1Ch, 1Ch, 0Ch, 10h, 18h, 10h, 14h, 20h, 14h, 1Ch
    db 2Ch, 1Ch
; DATA: text
    db '(4,8<', 0
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 38h, 28h, 20h, 28h, 18h, 10h, 20h, 10h, 08h, 10h
    db 28h, 38h, 28h, 00h, 08h, 00h, 00h, 00h, 38h, 30h, 08h, 00h
    db 18h, 10h, 08h, 20h, 0Ch, 08h, 30h, 18h, 10h, 10h, 10h, 18h
    db 18h, 18h
; DATA: text
    db '   (((888', 0
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 38h, 28h, 20h, 28h, 18h, 10h, 20h, 10h, 08h, 10h
    db 28h, 38h, 28h, 00h, 08h, 00h, 00h, 00h, 38h, 30h, 08h, 00h
    db 14h, 00h, 00h, 20h, 00h, 08h, 30h, 08h, 34h, 20h, 14h, 1Ch
    db 08h, 08h, 28h, 14h, 14h
; DATA: text
    db '<((888', 0
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 38h, 28h, 20h, 28h, 18h, 10h, 20h, 10h, 08h, 10h
    db 28h, 38h, 28h, 00h, 08h, 00h, 00h, 00h, 38h, 30h, 08h, 00h
    db 14h, 00h, 00h, 20h, 00h, 08h, 30h, 08h, 34h, 20h, 14h, 14h
    db 18h, 1Ch, 1Ch, 20h, 24h, 28h, 28h, 2Ch, 38h, 38h, 38h
WindRandA label byte
; UNKNOWN: not yet classified as code or data
    db 05h
WindRandB label byte
; UNKNOWN: not yet classified as code or data
    db 22h
WindRandC label byte
; UNKNOWN: not yet classified as code or data
    db 86h
WindRandWord label word
; UNKNOWN: not yet classified as code or data
    db 8Dh, 0E5h
ExpandDstSegment label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
ExpandSrcSegment label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
MapRows label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_2CF6 label byte
; UNKNOWN: not yet classified as code or data
    db 0FFh, 0Ch, 0Bh, 0Ah, 0FFh, 0FFh, 00h, 0FFh, 0FFh, 0Eh
LevelPaletteTable label word
; UNKNOWN: not yet classified as code or data
    db 0FCh, 2Bh, 5Ch, 2Ch, 0CCh, 2Bh, 9Ch, 2Bh, 0Ch, 2Bh, 6Ch, 2Bh
    db 0ACh, 2Ah, 3Ch, 2Bh, 0DCh, 2Ah, 0DCh, 2Ah, 8Ch, 2Ch, 8Ch, 2Ch
    db 8Ch, 2Ch, 0BCh, 2Ch, 0DCh, 2Ah, 0ACh, 2Ah
byte_2D20 label byte
; UNKNOWN: not yet classified as code or data
    db 09h, 09h, 00h, 00h, 00h, 0Dh, 04h, 04h, 0Ah, 0Dh, 10h, 10h
    db 10h, 09h, 0Eh, 04h
LevelMapRowsTable label byte
; UNKNOWN: not yet classified as code or data
    db 31h, 68h, 31h, 2Dh, 80h, 80h, 80h, 56h, 6Eh, 0Ch, 18h, 33h
    db 33h, 26h, 0ADh, 54h
LevelScriptTable label word
; UNKNOWN: not yet classified as code or data
    db 60h, 2Dh, 60h, 2Dh, 60h, 2Dh, 60h, 2Dh, 60h, 2Dh, 60h, 2Dh
    db 60h, 2Dh, 60h, 2Dh, 60h, 2Dh, 60h, 2Dh, 60h, 2Dh, 60h, 2Dh
    db 60h, 2Dh, 60h, 2Dh, 60h, 2Dh, 76h, 2Dh, 0FFh, 0FFh, 0B8h, 01h
    db 01h, 00h, 28h, 00h, 70h, 03h, 06h, 00h, 64h, 00h, 28h, 05h
    db 01h, 00h, 0FFh, 00h, 0FFh, 0FFh, 0B8h, 01h, 01h, 00h, 28h, 00h
    db 70h, 03h, 06h, 00h, 64h, 00h, 28h, 05h, 01h, 00h, 0FFh, 00h
    db 0FFh, 0FFh
CurrentLevel label byte
; UNKNOWN: not yet classified as code or data
    db 00h, 4Ch, 45h, 56h, 45h, 4Ch
LevelFileChar label byte
; DATA: text
    db '0.SQZ', 0
byte_2D96 label byte
; UNKNOWN: not yet classified as code or data
    db 30h, 30h, 30h, 31h, 31h, 31h, 32h, 33h, 33h, 30h, 34h, 34h
    db 34h, 35h, 30h, 32h, 42h, 41h, 43h, 4Bh
BackFileChar label byte
; DATA: text
    db '0.SQZ', 0
ScreenRowBytes label word
; UNKNOWN: not yet classified as code or data
    db 28h, 00h, 00h, 00h
SpriteDataSegment label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 00h, 00h, 00h, 00h
RingTileOffset label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
ScriptPointer label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
ScriptFrameCounter label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
IrisSpeed label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
IrisSpeedCounter label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_2DC4 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
IrisCenterY label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
IrisCenterX label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_2DCA label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_2DCC label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
IrisAngleIndex label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
IrisRadius label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_2DD2 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 00h, 00h
DisplayPage label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
DrawPage label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
LevelSegment label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
SpriteSheetSegment label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 00h, 00h
PrevCameraCol label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
PrevCameraRow label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
CameraCol label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
CameraRow label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
ColumnRing label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
RowRing label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
SpriteRelY label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
CurrentObject label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 00h, 00h
VisibleTileFlags label byte
; UNKNOWN: not yet classified as code or data
    db 00h
MixedTileCount label byte
; UNKNOWN: not yet classified as code or data
    db 00h
GridDirtyFlag label byte
; UNKNOWN: not yet classified as code or data
    db 00h
ScrollAnimCounter label byte
; UNKNOWN: not yet classified as code or data
    db 00h
BackdropCursor label word
; UNKNOWN: not yet classified as code or data
    db 8194 dup(0)
TileClassTable label byte
; UNKNOWN: not yet classified as code or data
    db 274 dup(0)
RenderSlots label word
; DATA: render slot records, 116 x 18 bytes (slot 1 = player; +4 word, FFFFh = empty)
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_4F0C label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_4F0E label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    db 00h
byte_4F1B label byte
; UNKNOWN: not yet classified as code or data
    db 00h
PlayerX label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
PlayerY label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_4F20 label word
; UNKNOWN: not yet classified as code or data
    db 00h
byte_4F21 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
PlayerVelX label word
; UNKNOWN: not yet classified as code or data
    db 00h
byte_4F23 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_4F24 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
word_4F25 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
byte_4F27 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
word_4F28 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
PlayerVelY label word
; UNKNOWN: not yet classified as code or data
    db 00h
byte_4F2B label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_4F2C label byte
; UNKNOWN: not yet classified as code or data
    db 00h
PlayerHurtTimer label byte
; UNKNOWN: not yet classified as code or data
    db 00h
word_4F2E label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_4F30 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_4F32 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_4F34 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 00h
word_4F37 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 00h
word_4F3A label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_4F3C label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 00h, 00h
word_4F40 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_4F42 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_4F44 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    db 00h, 00h
word_4F52 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_4F54 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_4F56 label word
; UNKNOWN: not yet classified as code or data
    db 306 dup(0)
word_5088 label word
; UNKNOWN: not yet classified as code or data
    db 1472 dup(0)
word_5648 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_564A label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_564C label word
; UNKNOWN: not yet classified as code or data
    db 00h
byte_564D label byte
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    db 00h
word_565A label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_565C label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_565E label word
; UNKNOWN: not yet classified as code or data
    db 00h
byte_565F label byte
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    db 00h
word_566C label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_566E label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_5670 label word
; UNKNOWN: not yet classified as code or data
    db 00h
byte_5671 label byte
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    db 00h
word_567E label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_5680 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_5682 label word
; UNKNOWN: not yet classified as code or data
    db 00h
byte_5683 label byte
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    db 00h
word_5690 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_5692 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_5694 label word
; UNKNOWN: not yet classified as code or data
    db 00h
byte_5695 label byte
; UNKNOWN: not yet classified as code or data
    db 157 dup(0)
; UNKNOWN: not yet classified as code or data
    db 2070 dup(0)
FrameDataOffsets label word
; UNKNOWN: not yet classified as code or data
    db 914 dup(0)
word_62DA label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    db 00h, 00h
FrameDataSegments label word
; UNKNOWN: not yet classified as code or data
    db 914 dup(0)
word_667A label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    db 00h, 00h
byte_6688 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_6689 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_668A label byte
; UNKNOWN: not yet classified as code or data
    db 254 dup(0)
byte_6788 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_6789 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_678A label byte
; UNKNOWN: not yet classified as code or data
    db 254 dup(0)
byte_6888 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_6889 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_688A label byte
; UNKNOWN: not yet classified as code or data
    db 254 dup(0)
AnimTileFlags label byte
; UNKNOWN: not yet classified as code or data
    db 00h
word_6989 label word
; UNKNOWN: not yet classified as code or data
    db 255 dup(0)
word_6A88 label word
; UNKNOWN: not yet classified as code or data
    db 140 dup(0)
word_6B14 label word
; UNKNOWN: not yet classified as code or data
    db 140 dup(0)
; DATA: text
    db 'THEEND.SQZ', 0
word_6BAB label word
; UNKNOWN: not yet classified as code or data
    db 0AAh, 55h
CheckpointX label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
CheckpointY label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_6BB1 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
AnimVisibleFlag label byte
; UNKNOWN: not yet classified as code or data
    db 00h
word_6BBE label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
byte_6BC0 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_6BC1 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
AnimRemapPtr label word
; UNKNOWN: not yet classified as code or data
    db 88h, 66h
FineScroll label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_6BC5 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_6BC6 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_6BC7 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_6BC8 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_6BC9 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
word_6BCA label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
byte_6BCC label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_6BCD label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_6BCE label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_6BCF label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_6BD0 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_6BD1 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_6BD2 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_6BD3 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
AnimTickCounter label byte
; UNKNOWN: not yet classified as code or data
    db 00h
SnowFrameTick label byte
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 00h, 00h
byte_6BD9 label byte
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
byte_6BDB label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_6BDC label byte
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 00h, 00h
byte_6BE0 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_6BE1 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
word_6BE2 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
byte_6BE4 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_6BE5 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_6BE6 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_6BE7 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_6BE8 label byte
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
byte_6BEA label byte
; UNKNOWN: not yet classified as code or data
    db 00h
word_6BEB label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
HScrollDir label byte
; UNKNOWN: not yet classified as code or data
    db 00h
VScrollActive label byte
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 00h
word_6BF1 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
byte_6BF3 label byte
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 00h
SnowWind label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
ShakeRows label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_6BFA label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_6BFC label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
byte_6BFE label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_6BFF label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_6C00 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_6C01 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_6C02 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_6C03 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_6C04 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_6C05 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
word_6C06 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_6C08 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_6C0A label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_6C0C label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
ScoreLow label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
ScoreHigh label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
byte_6C12 label byte
; UNKNOWN: not yet classified as code or data
    db 140 dup(0)
word_6C9E label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_6CA0 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_6CA2 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
byte_6CA4 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_6CA5 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_6CA6 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_6CA7 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_6CA8 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
FlakeTable label word
; UNKNOWN: not yet classified as code or data
    db 676 dup(0)
; UNKNOWN: not yet classified as code or data
    db 24h
; UNKNOWN: not yet classified as code or data
    db 18 dup(0)
; UNKNOWN: not yet classified as code or data
    db 01h, 00h, 00h, 00h, 0Ah, 00h, 00h, 00h, 64h, 00h, 00h, 00h
    db 0E8h, 03h, 00h, 00h, 10h, 27h, 01h, 00h, 0A0h, 86h, 0Fh, 00h
    db 40h, 42h, 98h, 00h, 80h, 96h, 0F5h, 05h, 00h, 0E1h, 9Ah, 3Bh
    db 00h, 0CAh, 91h, 1Ch, 0F2h, 1Bh, 0E3h, 1Ch, 6Ch, 1Ch, 1Dh, 1Ch
SineTable label byte
; UNKNOWN: not yet classified as code or data
    db 00h, 01h, 03h, 04h, 06h, 07h, 09h, 0Ah, 0Ch, 0Eh, 0Fh, 11h
    db 12h, 14h, 15h, 17h, 18h, 19h, 1Bh, 1Ch, 1Eh, 1Fh, 20h, 22h
    db 23h, 24h
; DATA: text
    db '&', 27h, '()*,-./012345667899:;;<<==>>>???????@???????>>>==<<;;:998766543210/.-,*)(', 27h, '&$'
; UNKNOWN: not yet classified as code or data
    db 23h, 22h, 20h, 1Fh, 1Eh, 1Ch, 1Bh, 19h, 18h, 17h, 15h, 14h
    db 12h, 11h, 0Fh, 0Eh, 0Ch, 0Ah, 09h, 07h, 06h, 04h, 03h, 01h
    db 0FFh, 0FEh, 0FCh, 0FBh, 0F9h, 0F8h, 0F6h, 0F5h, 0F3h, 0F1h, 0F0h, 0EEh
    db 0EDh, 0EBh, 0EAh, 0E8h, 0E7h, 0E6h, 0E4h, 0E3h, 0E1h, 0E0h, 0DFh, 0DDh
    db 0DCh, 0DBh, 0D9h, 0D8h, 0D7h, 0D6h, 0D5h, 0D3h, 0D2h, 0D1h, 0D0h, 0CFh
    db 0CEh, 0CDh, 0CCh, 0CBh, 0CAh, 0C9h, 0C9h, 0C8h, 0C7h, 0C6h, 0C6h, 0C5h
    db 0C4h, 0C4h, 0C3h, 0C3h, 0C2h, 0C2h, 0C1h, 0C1h, 0C1h, 0C0h, 0C0h, 0C0h
    db 0C0h, 0C0h, 0C0h, 0C0h, 0C0h, 0C0h, 0C0h, 0C0h, 0C0h, 0C0h, 0C0h, 0C0h
    db 0C1h, 0C1h, 0C1h, 0C2h, 0C2h, 0C3h, 0C3h, 0C4h, 0C4h, 0C5h, 0C6h, 0C6h
    db 0C7h, 0C8h, 0C9h, 0C9h, 0CAh, 0CBh, 0CCh, 0CDh, 0CEh, 0CFh, 0D0h, 0D1h
    db 0D2h, 0D3h, 0D5h, 0D6h, 0D7h, 0D8h, 0D9h, 0DBh, 0DCh, 0DDh, 0DFh, 0E0h
    db 0E1h, 0E3h, 0E4h, 0E6h, 0E7h, 0E8h, 0EAh, 0EBh, 0EDh, 0EEh, 0F0h, 0F1h
    db 0F3h, 0F5h, 0F6h, 0F8h, 0F9h, 0FBh, 0FCh, 0FEh
byte_7090 label byte
; DATA: text
    db '@???????>>>==<<;;:998766543210/.-,*)(', 27h, '&$'
; UNKNOWN: not yet classified as code or data
    db 23h, 22h, 20h, 1Fh, 1Eh, 1Ch, 1Bh, 19h, 18h, 17h, 15h, 14h
    db 12h, 11h, 0Fh, 0Eh, 0Ch, 0Ah, 09h, 07h, 06h, 04h, 03h, 01h
    db 00h, 0FEh, 0FCh, 0FBh, 0F9h, 0F8h, 0F6h, 0F5h, 0F3h, 0F1h, 0F0h, 0EEh
    db 0EDh, 0EBh, 0EAh, 0E8h, 0E7h, 0E6h, 0E4h, 0E3h, 0E1h, 0E0h, 0DFh, 0DDh
    db 0DCh, 0DBh, 0D9h, 0D8h, 0D7h, 0D6h, 0D5h, 0D3h, 0D2h, 0D1h, 0D0h, 0CFh
    db 0CEh, 0CDh, 0CCh, 0CBh, 0CAh, 0C9h, 0C9h, 0C8h, 0C7h, 0C6h, 0C6h, 0C5h
    db 0C4h, 0C4h, 0C3h, 0C3h, 0C2h, 0C2h, 0C1h, 0C1h, 0C1h, 0C0h, 0C0h, 0C0h
    db 0C0h, 0C0h, 0C0h, 0C0h, 0C0h, 0C0h, 0C0h, 0C0h, 0C0h, 0C0h, 0C0h, 0C0h
    db 0C1h, 0C1h, 0C1h, 0C2h, 0C2h, 0C3h, 0C3h, 0C4h, 0C4h, 0C5h, 0C6h, 0C6h
    db 0C7h, 0C8h, 0C9h, 0C9h, 0CAh, 0CBh, 0CCh, 0CDh, 0CEh, 0CFh, 0D0h, 0D1h
    db 0D2h, 0D3h, 0D5h, 0D6h, 0D7h, 0D8h, 0D9h, 0DBh, 0DCh, 0DDh, 0DFh, 0E0h
    db 0E1h, 0E3h, 0E4h, 0E6h, 0E7h, 0E8h, 0EAh, 0EBh, 0EDh, 0EEh, 0F0h, 0F1h
    db 0F3h, 0F5h, 0F6h, 0F8h, 0F9h, 0FBh, 0FCh, 0FEh, 00h, 01h, 03h, 04h
    db 06h, 07h, 09h, 0Ah, 0Ch, 0Eh, 0Fh, 11h, 12h, 14h, 15h, 17h
    db 18h, 19h, 1Bh, 1Ch, 1Eh, 1Fh, 20h, 22h, 23h, 24h, 26h, 27h
    db 28h, 29h, 2Ah, 2Ch, 2Dh, 2Eh, 2Fh, 30h, 31h, 32h, 33h, 34h
    db 35h, 36h, 36h, 37h, 38h, 39h, 39h, 3Ah, 3Bh, 3Bh, 3Ch, 3Ch
    db 3Dh, 3Dh, 3Eh, 3Eh, 3Eh, 3Fh, 3Fh, 3Fh, 3Fh, 3Fh, 3Fh, 3Fh
FrameWidths label byte
; UNKNOWN: not yet classified as code or data
    db 28h
FrameHeights label byte
; UNKNOWN: not yet classified as code or data
    db 24h, 20h, 23h, 18h, 24h
; DATA: text
    db ' " % % $'
; UNKNOWN: not yet classified as code or data
    db 20h, 23h, 20h, 23h, 20h, 23h, 18h, 23h, 20h, 26h, 28h, 1Fh
    db 30h, 1Fh, 20h, 1Fh, 20h, 24h, 18h, 20h, 28h, 1Eh, 28h, 1Fh
    db 28h, 1Fh, 20h, 1Eh, 18h, 1Eh, 20h, 1Fh, 18h, 1Eh, 18h, 20h
    db 28h, 22h, 20h, 21h, 20h, 23h, 20h, 22h, 28h, 1Fh, 28h, 20h
    db 28h, 1Eh, 28h, 1Fh, 30h, 20h, 20h, 24h, 18h, 20h, 20h, 20h
    db 20h, 21h, 18h, 1Dh, 18h, 11h, 10h, 10h, 18h, 11h, 20h, 0Eh
    db 18h, 0Fh, 10h, 0Fh, 18h, 0Eh, 20h, 0Bh, 18h, 10h, 20h, 19h
    db 20h, 1Fh, 18h, 21h, 20h, 1Fh, 20h, 1Fh, 08h, 06h, 08h, 08h
    db 10h, 09h, 10h, 0Bh, 10h, 0Bh, 10h, 13h, 18h, 12h, 18h, 0Dh
    db 10h, 10h, 10h, 10h, 10h, 10h, 10h, 0Fh, 18h, 0Fh, 18h, 13h
    db 18h, 18h, 20h, 13h, 18h, 19h, 10h, 0Fh, 10h, 0Fh, 10h, 09h
    db 10h, 0Fh, 18h, 09h, 18h, 09h, 18h, 09h, 18h, 09h, 18h, 09h
    db 18h, 09h, 18h, 09h, 18h, 09h, 18h, 09h, 18h, 09h, 18h, 09h
    db 18h, 09h, 28h, 0Dh, 30h, 0Dh, 30h, 0Dh, 30h, 0Dh, 38h, 11h
    db 18h, 13h, 10h, 0Dh, 10h, 0Dh, 10h, 0Dh, 10h, 0Dh, 10h, 0Dh
    db 18h, 13h, 20h, 16h, 18h, 1Eh, 18h, 1Dh, 18h, 21h, 10h, 21h
    db 18h, 21h, 18h, 07h, 18h, 07h, 18h, 07h, 18h, 07h, 18h, 07h
    db 18h, 07h, 30h, 41h, 28h, 2Ch, 30h, 20h, 28h, 2Eh, 20h, 26h
    db 28h, 23h, 30h, 21h, 28h, 21h, 30h, 0Eh, 30h, 1Dh, 30h, 1Fh
    db 30h, 13h, 30h, 14h, 30h, 17h, 30h, 1Bh, 30h, 17h, 28h, 1Bh
    db 28h, 22h, 18h, 0Eh, 10h, 0Fh, 18h, 13h, 10h, 12h, 10h, 12h
    db 18h, 0Ch, 18h, 13h, 10h, 19h, 18h, 15h, 18h, 17h, 10h, 1Bh
    db 10h, 14h, 10h, 17h, 10h, 15h, 18h, 0Bh, 18h, 0Bh, 10h, 16h
    db 10h, 16h, 10h, 13h, 10h, 12h, 10h, 12h, 10h, 13h, 10h, 1Dh
    db 10h, 15h, 10h, 1Ch, 18h, 14h, 18h, 0Fh, 10h, 0Fh, 10h, 13h
    db 10h, 15h, 10h, 15h, 18h, 11h, 18h, 0Ch, 18h, 0Dh, 18h, 0Dh
    db 18h, 11h, 18h, 16h, 10h, 0Dh, 10h, 10h, 10h, 18h, 10h, 11h
    db 18h, 0Bh, 10h, 17h, 18h, 12h, 18h, 0Fh, 18h, 0Fh, 18h, 0Fh
    db 10h, 13h, 18h, 14h, 10h, 12h, 08h, 17h, 10h, 15h, 10h, 12h
    db 10h, 0Ch, 10h, 0Ch, 10h, 0Ch, 18h, 17h, 10h, 12h, 10h, 10h
    db 10h, 0Eh, 10h, 16h, 20h, 0Dh, 18h, 10h, 18h, 12h, 10h, 0Dh
    db 18h, 0Eh, 10h, 13h, 10h, 19h, 08h, 16h, 10h, 0Dh, 10h, 13h
    db 18h, 10h, 18h, 12h, 18h, 0Dh, 10h, 0Fh, 10h, 1Ch, 10h, 0Eh
    db 10h, 18h, 18h, 12h, 20h, 14h, 20h, 11h, 10h, 0Bh, 10h, 1Ah
    db 18h, 12h, 18h, 0Dh, 18h, 0Ch, 10h, 0Fh, 10h, 13h, 10h, 13h
    db 10h, 13h, 10h, 13h, 10h, 13h, 18h, 10h, 18h, 16h, 10h, 12h
    db 18h, 10h, 10h, 0Ch, 10h, 0Ch, 10h, 0Bh, 20h, 10h, 28h, 0Eh
    db 28h, 0Eh, 28h, 0Eh, 20h, 0Eh, 20h, 0Dh, 10h, 12h, 10h, 12h
    db 18h, 12h, 20h, 29h, 20h, 1Dh, 18h, 14h, 28h, 21h, 30h, 1Ah
    db 10h, 0Bh, 10h, 0Bh, 10h, 0Bh, 10h, 0Bh, 10h, 0Bh, 10h, 0Bh
    db 10h, 0Bh, 10h, 0Bh, 10h, 0Bh, 10h, 0Bh, 10h, 0Bh, 10h, 0Bh
    db 10h, 0Bh, 10h, 0Bh, 10h, 0Bh, 10h, 0Bh, 10h, 0Bh, 10h, 0Bh
    db 10h, 0Bh, 10h, 0Bh, 10h, 0Bh, 10h, 0Bh, 10h, 0Bh, 10h, 0Bh
    db 10h, 0Bh, 10h, 0Bh, 10h, 0Bh, 08h, 08h, 10h, 0Bh, 08h, 0Bh
    db 08h, 05h, 08h, 05h, 10h, 0Ch, 30h, 28h, 38h, 1Eh, 30h, 30h
    db 20h, 33h, 30h, 46h, 30h, 46h, 28h, 1Bh, 30h, 1Dh, 38h, 0Eh
    db 10h, 0Ch, 10h, 0Ch, 10h, 0Ch, 10h, 0Ch, 10h, 0Ch, 10h, 0Ch
    db 10h, 0Ch, 10h, 0Ch, 10h, 0Ch, 10h, 0Ch, 10h, 0Ch, 10h, 0Ch
    db 10h, 0Ch, 10h, 0Ch, 10h, 0Ch, 10h, 0Ch, 20h, 15h, 08h, 0Ah
    db 10h, 0Ch, 10h, 0Ch, 10h, 0Ch, 40h, 1Fh, 20h, 10h, 08h, 0Ah
    db 30h, 15h, 10h, 0Ch, 08h, 0Ch, 10h, 0Ch, 10h, 1Ah, 18h, 18h
    db 20h, 1Dh, 18h, 1Eh, 20h, 1Dh, 30h, 28h, 20h, 13h, 20h, 19h
    db 18h, 1Ch, 20h, 0Eh, 20h, 0Fh, 20h, 0Eh, 20h, 0Fh, 10h, 1Ch
    db 10h, 1Ch, 10h, 1Ch, 10h, 1Ch, 18h, 15h, 18h, 19h, 20h, 17h
    db 20h, 17h, 18h, 20h, 18h, 1Eh, 20h, 16h, 28h, 0Eh, 28h, 0Ch
    db 18h, 0Fh, 20h, 0Bh, 18h, 15h, 10h, 0Ah, 10h, 09h, 08h, 0Ah
    db 10h, 0Ah, 10h, 0Ah, 18h, 07h, 10h, 0Eh, 18h, 0Ch, 18h, 0Bh
    db 28h, 1Dh, 20h, 1Ch, 20h, 1Ch, 28h, 18h, 28h, 24h, 28h, 1Fh
    db 18h, 20h, 20h, 1Fh, 20h, 1Fh, 20h, 1Fh, 10h, 15h, 10h, 15h
    db 10h, 18h, 18h, 18h, 28h, 14h, 28h, 12h, 20h, 1Ch, 08h, 05h
    db 18h, 07h, 20h, 0Ch, 20h, 15h, 18h, 06h, 18h, 08h, 18h, 16h
    db 20h, 1Ch, 18h, 1Fh, 28h, 1Eh, 20h, 1Ah, 20h, 13h, 10h, 13h
    db 18h, 12h, 18h, 0Bh, 18h, 05h, 30h, 1Bh, 30h, 1Dh, 30h, 29h
    db 30h, 20h, 20h, 03h, 20h, 05h, 28h, 0Bh, 30h, 13h, 38h, 1Bh
    db 40h, 20h, 40h, 1Fh, 40h, 1Fh, 40h, 20h, 30h, 27h, 30h, 1Dh
    db 30h, 1Eh, 30h, 1Bh, 40h, 22h, 38h, 24h, 28h, 20h, 40h, 23h
    db 40h, 2Bh, 38h, 2Dh, 20h, 25h, 20h, 25h, 20h, 22h, 20h, 25h
    db 30h, 2Eh, 30h, 2Bh, 38h, 1Eh, 28h, 1Ch, 28h, 18h, 38h, 10h
    db 20h, 31h, 20h, 3Bh, 20h, 30h, 38h, 22h, 40h, 1Bh, 20h, 1Eh
    db 20h, 1Ch, 20h, 20h, 28h, 15h, 20h, 20h, 20h, 1Fh, 20h, 1Eh
    db 30h, 1Ch, 50h, 20h, 28h, 1Eh, 28h, 23h, 28h, 3Eh, 38h, 16h
    db 38h, 18h, 38h, 17h, 28h, 10h, 28h, 12h, 20h, 0Eh, 68h, 29h
    db 10h, 16h, 18h, 17h, 18h, 17h, 18h, 17h, 20h, 17h, 18h, 18h
    db 20h, 18h, 20h, 18h, 20h, 17h, 20h, 1Bh, 20h, 1Bh, 20h, 18h
    db 18h, 18h, 20h, 14h, 30h, 1Ch, 40h, 1Ch, 30h, 1Fh, 30h, 26h
word_7522 label word
; UNKNOWN: not yet classified as code or data
    db 28h, 22h, 20h, 1Ah, 30h, 17h, 00h, 00h
FrameHotX label byte
; UNKNOWN: not yet classified as code or data
    db 14h
FrameHotY label byte
; UNKNOWN: not yet classified as code or data
    db 24h, 10h, 23h, 0Ch, 24h, 10h, 24h, 10h, 25h, 10h, 25h, 0Fh
    db 24h, 10h, 23h, 0Fh, 23h, 0Fh, 23h, 0Ch, 23h, 10h, 26h, 14h
    db 1Fh, 18h, 1Fh, 10h, 1Fh, 10h, 24h, 0Ch, 20h, 14h, 1Eh, 14h
    db 1Fh, 14h, 1Fh, 10h, 1Eh, 0Ch, 1Eh, 10h, 1Fh, 0Ch, 1Eh, 0Ch
    db 20h, 14h, 22h, 10h, 21h, 10h, 23h, 10h, 22h, 14h, 1Fh, 14h
    db 20h, 14h, 1Eh, 14h, 1Fh, 18h, 20h, 10h, 24h, 0Ch, 20h, 10h
    db 20h, 10h, 21h, 0Ch, 1Dh, 0Eh, 11h, 09h, 10h, 0Eh, 11h, 0Fh
    db 10h, 09h, 0Fh, 0Ah, 0Fh, 0Ch, 0Eh, 10h, 0Bh, 0Ch, 10h, 10h
    db 19h, 10h, 1Fh, 0Ch, 21h, 10h, 1Fh, 10h, 1Fh, 04h, 06h, 04h
    db 08h, 08h, 09h, 08h, 0Bh, 08h, 0Bh, 08h, 13h, 0Ch, 12h, 0Ch
    db 0Dh, 08h, 10h, 08h, 10h, 08h, 10h, 08h, 0Fh, 0Ch, 0Fh, 0Ch
    db 13h, 11h, 0Fh, 0Eh, 0Dh, 07h, 0Ah, 08h, 0Fh, 08h, 0Fh, 08h
    db 0Ch, 08h, 0Fh, 0Bh, 09h, 0Ch, 09h, 0Ch, 09h, 0Ch, 09h, 0Ch
    db 09h, 0Ch, 09h, 0Bh, 09h, 0Ch, 09h, 0Ch, 09h, 0Ch, 09h, 0Ch
    db 09h, 0Ch, 09h, 14h, 0Dh, 17h, 0Dh, 16h, 0Dh, 17h, 0Dh, 1Bh
    db 11h, 0Eh, 05h, 07h, 0Dh, 07h, 0Dh, 07h, 0Dh, 07h, 0Dh, 07h
    db 0Dh, 06h, 13h, 0Fh, 16h, 0Bh, 1Eh, 0Bh, 1Dh, 0Bh, 21h, 06h
    db 21h, 0Bh, 21h, 0Bh, 07h, 0Bh, 07h, 0Bh, 07h, 0Bh, 07h, 0Bh
    db 07h, 0Bh, 07h, 19h, 41h, 16h, 2Ch, 16h, 20h, 13h, 2Eh, 10h
    db 26h, 13h, 23h, 11h, 21h, 14h, 21h, 18h, 0Eh, 18h, 1Dh, 18h
    db 1Fh, 0Ch, 13h, 0Ch, 14h, 09h, 17h, 09h, 1Bh, 0Fh, 17h, 0Ch
    db 1Bh, 0Ch, 1Fh, 0Ch, 0Eh, 08h, 0Fh, 0Ch, 13h, 08h, 12h, 08h
    db 12h, 0Ch, 0Ch, 0Ch, 13h, 08h, 19h, 0Ch, 15h, 0Ch, 17h, 08h
    db 1Bh, 08h, 14h, 08h, 17h, 08h, 15h, 0Ch, 0Bh, 0Ch, 0Bh, 08h
    db 16h, 08h, 16h, 08h, 13h, 08h, 12h, 08h, 12h, 08h, 13h, 08h
    db 1Dh, 08h, 15h, 08h, 1Ch, 0Ch, 14h, 0Ch, 0Fh, 08h, 0Fh, 08h
    db 13h, 08h, 15h, 08h, 15h, 0Ch, 11h, 0Ch, 0Ch, 0Ch, 0Dh, 0Ch
    db 0Dh, 0Ch, 11h, 0Ch, 16h, 08h, 0Dh, 08h, 10h, 08h, 18h, 08h
    db 11h, 0Ch, 0Bh, 08h, 17h, 0Ch, 12h, 0Ch, 0Fh, 0Ch, 0Fh, 0Ch
    db 0Fh, 08h, 13h, 0Ch, 14h, 08h, 12h, 04h, 17h, 08h, 15h, 08h
    db 12h, 08h, 0Ch, 08h, 0Ch, 08h, 0Ch, 0Ch, 17h, 08h, 12h, 08h
    db 10h, 08h, 0Eh, 08h, 16h, 10h, 0Dh, 0Ch, 10h, 0Ch, 12h, 08h
    db 0Dh, 0Ch, 0Eh, 08h, 13h, 08h, 19h, 04h, 16h, 08h, 0Dh, 08h
    db 13h, 0Ch, 10h, 0Ch, 12h, 0Ch, 0Dh, 08h, 0Fh, 08h, 1Ch, 08h
    db 0Eh, 08h, 18h, 0Ch, 12h, 10h, 14h, 10h, 11h, 08h, 0Bh, 08h
    db 1Ah, 0Ch, 12h, 0Ch, 0Dh, 0Ch, 0Ch, 08h, 0Fh, 08h, 13h, 08h
    db 13h, 08h, 13h, 08h, 13h, 08h, 13h, 0Ch, 10h, 0Ch, 16h, 08h
    db 12h, 0Ch, 10h, 08h, 0Ch, 08h, 0Ch, 08h, 0Bh, 10h, 10h, 14h
    db 0Eh, 14h, 0Eh, 14h, 0Eh, 10h, 0Eh, 10h, 0Dh, 08h, 12h, 08h
    db 12h, 0Ch, 12h, 10h, 29h, 10h, 1Dh, 0Ch, 14h, 14h, 21h, 18h
    db 1Ah, 08h, 0Bh, 08h, 0Bh, 08h, 0Bh, 08h, 0Bh, 08h, 0Bh, 08h
    db 0Bh, 08h, 0Bh, 08h, 0Bh, 08h, 0Bh, 08h, 0Bh, 08h, 0Bh, 08h
    db 0Bh, 08h, 0Bh, 08h, 0Bh, 08h, 0Bh, 08h, 0Bh, 08h, 0Bh, 08h
    db 0Bh, 08h, 0Bh, 08h, 0Bh, 08h, 0Bh, 08h, 0Bh, 08h, 0Bh, 08h
    db 0Bh, 08h, 0Bh, 08h, 0Bh, 08h, 0Bh, 04h, 08h, 08h, 0Bh, 04h
    db 0Bh, 04h, 05h, 04h, 05h, 08h, 0Ch, 18h, 28h, 1Ch, 1Eh, 18h
    db 30h, 10h, 33h, 22h, 46h, 13h, 46h, 0Dh, 1Ah, 19h, 1Dh, 1Ch
    db 0Eh, 08h, 0Ch, 08h, 0Ch, 08h, 0Ch, 08h, 0Ch, 08h, 0Ch, 08h
    db 0Ch, 08h, 0Ch, 08h, 0Ch, 08h, 0Ch, 08h, 0Ch, 08h, 0Ch, 08h
    db 0Ch, 08h, 0Ch, 08h, 0Ch, 08h, 0Ch, 08h, 0Ch, 10h, 15h, 04h
    db 0Ah, 08h, 0Ch, 08h, 0Ch, 08h, 0Ch, 20h, 1Fh, 10h, 10h, 04h
    db 0Ah, 18h, 15h, 07h, 0Bh, 04h, 0Ch, 08h, 0Ch, 08h, 1Ah, 0Ch
    db 18h, 10h, 1Dh, 0Ch, 1Eh, 10h, 1Dh, 18h, 28h, 10h, 13h, 10h
    db 19h, 0Ch, 1Ch, 10h, 0Eh, 10h, 0Fh, 10h, 0Eh, 10h, 0Fh, 08h
    db 1Ch, 08h, 1Ch, 08h, 1Ch, 08h, 1Ch, 0Ch, 15h, 0Ch, 19h, 10h
    db 17h, 11h, 17h, 0Ch, 20h, 0Ch, 1Fh, 10h, 16h, 14h, 0Eh, 14h
    db 09h, 0Ch, 0Fh, 0Fh, 0Bh, 0Ch, 15h, 08h, 0Ah, 08h, 09h, 04h
    db 0Ah, 08h, 0Ah, 08h, 0Ah, 0Ch, 07h, 08h, 0Eh, 0Ch, 0Ch, 0Ch
    db 0Bh, 14h, 1Dh, 10h, 1Ch, 10h, 1Ch, 14h, 18h, 14h, 24h, 14h
    db 1Fh, 0Ch, 20h, 0Fh, 1Fh, 0Dh, 1Fh, 10h, 1Fh, 02h, 15h, 01h
    db 15h, 0FFh, 18h, 0FFh, 18h, 10h, 14h, 10h, 14h, 0Ch, 1Ch, 04h
    db 05h, 0Ch, 07h, 10h, 0Ch, 10h, 15h, 0Ch, 06h, 0Ch, 08h, 0Ch
    db 16h, 10h, 1Ch, 0Ch, 1Fh, 14h, 1Eh, 10h, 1Ah, 10h, 13h, 08h
    db 13h, 0Ch, 12h, 0Ch, 0Bh, 0Ch, 05h, 18h, 1Bh, 18h, 1Dh, 18h
    db 29h, 18h, 20h, 10h, 03h, 10h, 05h, 14h, 0Bh, 18h, 13h, 1Bh
    db 1Ah, 20h, 20h, 1Eh, 1Fh, 20h, 1Fh, 21h, 20h, 18h, 27h, 18h
    db 1Dh, 15h, 1Eh, 18h, 1Bh, 20h, 22h, 1Ch, 24h, 14h, 20h, 20h
    db 23h, 20h, 2Bh, 1Ch, 2Dh, 10h, 25h, 10h, 25h, 10h, 22h, 10h
    db 25h, 18h, 2Eh, 18h, 2Bh, 1Ch, 1Eh, 14h, 1Ch, 14h, 18h, 1Ch
    db 10h, 10h, 31h, 10h, 3Bh, 10h, 30h, 1Ch, 22h, 20h, 1Bh, 20h
    db 32h, 28h, 21h, 20h, 28h, 14h, 15h, 10h, 20h, 10h, 1Fh, 10h
    db 1Eh, 18h, 1Ch, 28h, 20h, 14h, 1Eh, 14h, 23h, 14h, 3Eh, 1Ch
    db 19h, 1Ch, 19h, 1Ch, 19h, 14h, 10h, 14h, 12h, 10h, 0Eh, 34h
    db 29h, 08h, 16h, 0Ch, 17h, 0Ch, 17h, 0Ch, 17h, 10h, 17h, 0Ch
    db 18h, 10h, 18h, 10h, 18h, 10h, 17h, 10h, 1Bh, 10h, 1Bh, 10h
    db 18h, 0Ch, 18h, 10h, 14h, 19h, 1Ch, 20h, 1Bh, 18h, 1Dh, 18h
    db 26h, 14h, 22h, 10h, 1Ah, 17h, 17h, 00h, 00h
word_78C4 label word
; UNKNOWN: not yet classified as code or data
    db 4Ah, 79h
byte_78C6 label byte
; UNKNOWN: not yet classified as code or data
    db 01h, 01h, 01h, 01h, 01h, 01h, 02h, 02h, 02h, 02h, 02h, 02h
    db 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h
    db 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h
    db 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h
    db 03h, 03h, 03h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h
    db 04h, 04h, 04h, 04h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h
    db 05h, 06h, 06h, 06h, 06h, 06h, 06h, 06h, 07h, 07h, 07h, 07h
    db 07h, 07h, 08h, 08h, 08h, 09h, 09h, 09h, 0Ah, 0Ah, 0Bh, 0Bh
    db 0Ch, 0Dh, 0Eh, 10h, 10h, 10h, 10h, 10h, 10h, 10h, 10h, 10h
    db 10h, 10h, 10h, 10h, 10h, 10h, 10h, 10h, 10h, 10h, 10h, 10h
    db 10h, 10h, 10h, 10h, 10h, 10h, 10h, 10h, 10h, 10h, 10h, 10h
    db 01h, 01h, 01h, 01h, 02h, 02h, 02h, 02h, 03h, 03h, 04h, 04h
    db 05h, 06h, 07h, 08h, 09h, 09h, 09h, 09h, 0Ah, 0Ah, 0Ah, 0Ah
    db 0Ah, 0Ah, 0Ah, 0Bh, 0Bh, 0Bh, 0Bh, 0Bh, 0Bh, 0Bh, 0Bh, 0Bh
    db 0Ch, 0Ch, 0Ch, 0Ch, 0Ch, 0Ch, 0Ch, 0Ch, 0Ch, 0Ch, 0Ch, 0Ch
    db 0Ch, 0Dh, 0Dh, 0Dh, 0Dh, 0Dh, 0Dh, 0Dh, 0Dh, 0Dh, 0Dh, 0Dh
    db 0Dh, 0Dh, 0Dh, 0Dh, 0Dh, 0Dh, 0Eh, 0Eh, 0Eh, 0Eh, 0Eh, 0Eh
    db 0Eh, 0Eh, 0Eh, 0Eh, 0Eh, 0Eh, 0Eh, 0Eh, 0Eh, 0Eh, 0Eh, 0Eh
    db 0Eh, 0Eh, 0Eh, 0Eh, 0Eh, 0Eh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh
    db 0Fh, 0Fh, 0Fh, 0Fh, 10h, 10h, 10h, 10h, 10h, 10h, 10h, 10h
    db 10h, 10h, 10h, 10h, 10h, 10h, 10h, 10h, 10h, 10h, 10h, 10h
    db 10h, 10h, 10h, 10h, 10h, 10h, 10h, 10h, 10h, 10h, 10h, 10h
word_79CE label word
; UNKNOWN: not yet classified as code or data
    db 0BFh, 0FFh, 0CDh, 0FFh, 0DDh, 0FFh, 0ECh, 0FFh, 0F6h, 0FFh, 0FBh, 0FFh
    db 0FEh, 0FFh, 0FFh, 0FFh, 00h, 00h, 10h, 00h, 60h, 00h, 80h, 00h
    db 0B0h, 00h, 00h, 01h, 00h, 02h
dtbl_79EC label word
    dw offset sub_6272
    dw offset sub_6277
; UNKNOWN: not yet classified as code or data
    db 22h, 00h, 3Ah, 00h, 0Bh, 00h, 10h, 00h, 23h, 00h, 3Bh, 00h
    db 07h, 00h, 19h, 00h, 24h, 00h, 3Ch, 00h, 0E9h, 0FFh, 02h, 00h
    db 25h, 00h, 3Dh, 00h, 0Bh, 00h, 03h, 00h, 26h, 00h, 3Eh, 00h
    db 08h, 00h, 15h, 00h, 0Eh, 00h, 3Fh, 00h, 0EEh, 0FFh, 1Bh, 00h
    db 0Fh, 00h, 40h, 00h, 08h, 00h, 12h, 00h, 10h, 00h, 41h, 00h
    db 0F7h, 0FFh, 0F6h, 0FFh, 0AAh, 55h, 22h, 00h, 0EBh, 00h, 0Fh, 00h
    db 1Fh, 00h, 23h, 00h, 0ECh, 00h, 0FAh, 0FFh, 0Fh, 00h, 24h, 00h
    db 0F0h, 00h, 0E6h, 0FFh, 0F6h, 0FFh, 25h, 00h, 0EEh, 00h, 14h, 00h
    db 00h, 00h, 26h, 00h, 0ECh, 00h, 0FAh, 0FFh, 0Fh, 00h, 0Eh, 00h
    db 0EFh, 00h, 0E7h, 0FFh, 14h, 00h, 0Fh, 00h, 0EBh, 00h, 0Eh, 00h
    db 1Fh, 00h, 10h, 00h, 0EDh, 00h, 0EEh, 0FFh, 0EBh, 0FFh, 0AAh, 55h
    db 22h, 00h, 43h, 00h, 08h, 00h, 27h, 00h, 23h, 00h, 43h, 00h
    db 02h, 00h, 1Eh, 00h, 24h, 00h, 44h, 00h, 0F4h, 0FFh, 0Dh, 00h
    db 25h, 00h, 44h, 00h, 02h, 00h, 1Eh, 00h, 0Eh, 00h, 44h, 00h
    db 00h, 00h, 20h, 00h, 0Fh, 00h, 43h, 00h, 08h, 00h, 25h, 00h
    db 10h, 00h, 44h, 00h, 0FAh, 0FFh, 03h, 00h, 0AAh, 55h, 0D0h, 00h
    db 0C0h, 0FFh, 43h, 00h, 44h, 00h, 45h, 00h, 5Bh, 00h, 0F8h, 0FFh
    db 22h, 00h, 12h, 01h, 08h, 00h, 13h, 00h, 23h, 00h, 13h, 01h
    db 02h, 00h, 0Ah, 00h, 24h, 00h, 14h, 01h, 0F4h, 0FFh, 0FDh, 0FFh
    db 25h, 00h, 15h, 01h, 02h, 00h, 0Ah, 00h, 0Eh, 00h, 14h, 01h
    db 00h, 00h, 0Ch, 00h, 0Fh, 00h, 12h, 01h, 08h, 00h, 11h, 00h
    db 10h, 00h, 12h, 01h, 0FAh, 0FFh, 0EFh, 0FFh, 0AAh, 55h, 0D0h, 00h
    db 0E0h, 0FFh, 12h, 01h, 13h, 01h, 14h, 01h, 15h, 01h, 0F8h, 0FFh
word_7B04 label word
; UNKNOWN: not yet classified as code or data
    db 0F0h, 79h
byte_7B06 label byte
; UNKNOWN: not yet classified as code or data
    db 02h
byte_7B07 label byte
; UNKNOWN: not yet classified as code or data
    db 19h
byte_7B08 label byte
; UNKNOWN: not yet classified as code or data
    db 00h, 32h, 7Ah, 06h, 1Eh, 00h, 74h, 7Ah, 06h, 14h, 01h, 0BCh
    db 7Ah, 0Ch, 1Eh, 03h
byte_7B18 label byte
; UNKNOWN: not yet classified as code or data
    db 03h
byte_7B19 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_7B1A label byte
; UNKNOWN: not yet classified as code or data
    db 03h
word_7B1B label word
; UNKNOWN: not yet classified as code or data
    db 7Ch, 00h, 7Bh, 00h, 7Ah, 00h, 79h, 00h, 7Dh, 00h, 7Eh, 00h
    db 7Fh, 00h, 27h, 00h, 77h, 00h, 0FEh, 0EFh, 28h, 00h, 77h, 00h
    db 00h, 0F0h, 29h, 00h, 77h, 00h, 00h, 0EFh, 2Ah, 00h, 77h, 00h
    db 01h, 0F0h, 2Bh, 00h, 77h, 00h, 00h, 0F1h, 2Ch, 00h, 77h, 00h
    db 0FEh, 0F1h, 2Dh, 00h, 77h, 00h, 01h, 0F2h, 2Eh, 00h, 77h, 00h
    db 0FFh, 0F5h, 2Fh, 00h, 77h, 00h, 02h, 0F0h, 30h, 00h, 79h, 00h
    db 0FAh, 0E7h, 31h, 00h, 79h, 00h, 0F7h, 0E1h, 32h, 00h, 79h, 00h
    db 0F5h, 0DFh, 33h, 00h, 79h, 00h, 0F3h, 0E1h, 34h, 00h, 79h, 00h
    db 0F3h, 0E1h, 00h, 00h
byte_7B7F label byte
; UNKNOWN: not yet classified as code or data
    db 00h, 03h, 05h, 07h, 02h, 06h, 00h, 00h, 01h, 03h, 04h, 07h
    db 02h, 06h, 01h, 00h, 01h, 03h, 04h, 07h, 02h, 06h, 00h, 00h
    db 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 09h, 00h, 0FEh, 0FFh
    db 2Fh, 00h, 0FEh, 0FFh, 00h, 00h, 00h, 00h, 01h, 00h, 01h, 00h
    db 02h, 00h, 02h, 00h, 03h, 00h, 03h, 00h, 04h, 00h, 04h, 00h
    db 05h, 00h, 05h, 00h, 0E8h, 0FFh, 27h, 00h, 27h, 00h, 28h, 00h
    db 28h, 00h, 29h, 00h, 29h, 00h, 2Ah, 00h, 2Ah, 00h, 2Bh, 00h
    db 2Bh, 00h, 2Ch, 00h, 2Ch, 00h, 0E8h, 0FFh, 0Bh, 00h, 0Bh, 00h
    db 0Bh, 00h, 0Bh, 00h, 0Ch, 00h, 0FEh, 0FFh, 22h, 00h, 22h, 00h
    db 23h, 00h, 23h, 00h, 24h, 00h, 24h, 00h, 24h, 40h, 0F2h, 0FFh
    db 15h, 00h, 15h, 00h, 15h, 00h, 15h, 00h, 16h, 00h, 16h, 00h
    db 16h, 00h, 16h, 00h, 17h, 00h, 17h, 00h, 17h, 00h, 17h, 00h
    db 18h, 00h, 18h, 00h, 18h, 00h, 18h, 00h, 13h, 00h, 13h, 00h
    db 13h, 00h, 13h, 00h, 14h, 00h, 14h, 00h, 14h, 00h, 14h, 00h
    db 0D0h, 0FFh, 12h, 00h, 12h, 00h, 12h, 00h, 12h, 00h, 12h, 00h
    db 11h, 00h, 11h, 00h, 11h, 00h, 11h, 00h, 11h, 00h, 0ECh, 0FFh
    db 25h, 00h, 25h, 00h, 25h, 00h, 26h, 00h, 26h, 00h, 26h, 00h
    db 0Eh, 00h, 0Eh, 00h, 0Eh, 40h, 0EEh, 0FFh, 0Fh, 00h, 0Fh, 00h
    db 0Fh, 00h, 23h, 00h, 23h, 00h, 23h, 00h, 10h, 00h, 10h, 00h
    db 10h, 40h, 0EEh, 0FFh, 21h, 00h, 0FEh, 0FFh, 19h, 00h, 19h, 00h
    db 1Ah, 00h, 1Ah, 00h, 1Bh, 00h, 1Bh, 00h, 1Ch, 00h, 1Ch, 00h
    db 0F8h, 0FFh, 06h, 00h, 06h, 00h, 06h, 00h, 07h, 00h, 07h, 00h
    db 07h, 00h, 06h, 00h, 06h, 00h, 06h, 00h, 06h, 00h, 07h, 00h
    db 07h, 00h, 07h, 00h, 07h, 00h, 06h, 00h, 06h, 00h, 06h, 00h
    db 06h, 00h, 06h, 00h, 07h, 00h, 07h, 00h, 07h, 00h, 07h, 00h
    db 07h, 00h, 0ECh, 0FFh, 09h, 00h, 09h, 00h, 09h, 00h, 09h, 00h
    db 0Ah, 00h, 0Ah, 00h, 0Ah, 00h, 0Ah, 00h, 09h, 00h, 09h, 00h
    db 09h, 00h, 09h, 00h, 08h, 00h, 08h, 00h, 08h, 00h, 08h, 00h
    db 0E0h, 0FFh, 1Dh, 00h, 1Dh, 00h, 1Dh, 00h, 1Fh, 00h, 1Fh, 00h
    db 1Fh, 00h, 0F4h, 0FFh
word_7CDF label word
; UNKNOWN: not yet classified as code or data
    db 9Fh, 7Bh, 0A7h, 7Bh, 0DBh, 7Bh, 0E7h, 7Bh, 0F7h
; DATA: text
    db '{)|?|S|g|', 0
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    db 00h, 7Dh, 7Ch, 0AFh, 7Ch, 6Bh, 7Ch, 0D1h, 7Ch
; UNKNOWN: not yet classified as code or data
    db 24 dup(0)
; UNKNOWN: not yet classified as code or data
    db 0A3h, 7Bh, 0C1h, 7Bh, 00h, 00h, 0A3h, 7Bh, 0A3h, 7Bh, 0A3h, 7Bh
    db 0A3h, 7Bh, 0A3h, 7Bh
dtbl_7D2F label word
    dw offset sub_5CDB
    dw offset sub_5EC4
    dw offset sub_5F30
    dw offset sub_5F96
    dw offset sub_5E62
    dw offset sub_5E96
    dw offset sub_5F96
    dw offset sub_5F96
    dw offset sub_5CCE
; UNKNOWN: not yet classified as code or data
    db 46 dup(0)
dtbl_7D6F label word
    dw offset sub_5E96
    dw offset sub_5EC4
    dw offset sub_5F13
    dw offset sub_5E96
    dw offset sub_5E96
    dw offset sub_5E96
    dw offset sub_5E96
    dw offset sub_5E96
; DATA: text
    db 'LEVEL COMPLETED$'
; DATA: text
    db 'SCORE$'
dtbl_7D95 label word
    dw offset sub_652C
    dw offset sub_6539
    dw offset sub_65AF
word_7D9B label word
    dw offset sub_65EF
    dw offset sub_6641
    dw offset sub_6657
    dw offset sub_6660
    dw offset sub_6669
    dw offset sub_6645
    dw offset sub_65AF
word_7DA9 label word
    dw offset sub_6672
    dw offset sub_6673
    dw offset sub_65AF
; UNKNOWN: not yet classified as code or data
    db 431 dup(0)
byte_7F5E label byte
; UNKNOWN: not yet classified as code or data
    db 256 dup(0)
TileFlagsTable label byte
; UNKNOWN: not yet classified as code or data
    db 256 dup(0)
LevelPropHeader label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
LevelStartX label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
LevelStartY label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
CameraLeftLimit label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
LevelFlags label byte
; UNKNOWN: not yet classified as code or data
    db 00h
word_8167 label word
; UNKNOWN: not yet classified as code or data
    db 652 dup(0)
; DATA: proximity trigger records, 15 x 10 bytes (+0 map offset, +2 width, +3 rows)
    db 150 dup(0)
; UNKNOWN: not yet classified as code or data
    db 2048 dup(0)
SpriteBankA label word
; UNKNOWN: not yet classified as code or data
    db 35h, 00h
SpriteBankB label word
; UNKNOWN: not yet classified as code or data
    db 38h, 01h
; DATA: secret-tile patch records, 80 x 5 bytes (+0 new tile, +1 saved tile, +3 offset)
; UNKNOWN: not yet classified as code or data
    db 400 dup(0)
byte_8E1D label byte
; UNKNOWN: not yet classified as code or data
    db 256 dup(0)
; DATA: effect source records, 70 x 7 bytes (+4 word X, FFFFh = inactive)
    db 490 dup(0)
; DATA: terrain entity records, 16 x 15 bytes (+4 word X, FFFFh = inactive)
; UNKNOWN: not yet classified as code or data
    db 240 dup(0)
word_91F7 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_91F9 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
byte_91FB label byte
; UNKNOWN: not yet classified as code or data
    db 00h
word_91FC label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
byte_91FE label byte
; UNKNOWN: not yet classified as code or data
    db 00h
word_91FF label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_9201 label word
; UNKNOWN: not yet classified as code or data
    db 4263 dup(0)
byte_A2A8 label byte
; UNKNOWN: not yet classified as code or data
    db 80 dup(0)
; DATA: text
    db 'PRESENT.SQZ', 0
; DATA: text
    db 'TITUS.SQZ', 0
word_A30E label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_A310 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
FollowerOverlapMode label byte
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 17h, 10h, 10h, 07h, 14h, 04h, 04h, 04h, 00h, 00h
    db 00h, 00h, 00h, 00h, 00h, 00h
byte_A324 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_A325 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
word_A326 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
byte_A328 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_A329 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_A32A label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_A32B label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_A32C label byte
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_A32E label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
LandingFlag label byte
; UNKNOWN: not yet classified as code or data
    db 00h
LandingDepth label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
BiosSeed label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
BiosSeedReady label byte
; UNKNOWN: not yet classified as code or data
    db 00h
EffectOriginX label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
EffectOriginY label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
EffectFrame label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
LastEffectTick label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
LastDebrisSlot label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
byte_A340 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
word_A341 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_A343 label word
; UNKNOWN: not yet classified as code or data
    db 78h, 00h, 64h, 00h, 0A6h, 0FFh, 6Eh, 00h, 88h, 0FFh, 28h, 00h
    db 50h, 00h, 3Ch, 00h
word_A353 label word
; DATA: score table: word per debris code 4Ah..5Ah (10,20,..,10000); ScoreLow/High += it
; UNKNOWN: not yet classified as code or data
    db 0Ah, 00h, 14h, 00h, 1Eh, 00h, 32h, 00h, 3Ch, 00h, 46h, 00h
    db 4Bh, 00h, 50h, 00h, 64h, 00h, 0C8h, 00h, 0F4h, 01h, 20h, 03h
    db 0E8h, 03h, 0D0h, 07h, 0B8h, 0Bh, 70h, 17h, 10h, 27h
byte_A375 label byte
; UNKNOWN: not yet classified as code or data
    db 10h, 0Fh, 0Dh, 0Eh, 0Ch, 0Ch, 0Dh, 0Eh, 00h, 00h, 00h, 00h
    db 00h, 00h, 00h, 00h, 00h, 00h, 03h, 03h, 02h, 02h, 01h, 01h
    db 03h, 02h, 03h, 03h, 03h, 02h, 03h, 00h, 00h, 00h, 00h, 00h
    db 01h, 02h, 02h, 03h, 03h, 01h, 03h, 03h, 01h, 03h, 0Bh, 01h
    db 00h, 03h, 01h, 03h, 02h, 02h, 02h, 00h, 03h, 02h, 01h, 00h
    db 02h, 03h, 01h, 01h, 01h, 02h, 03h, 01h, 03h, 03h, 00h, 09h
    db 0Ah, 08h, 08h, 09h, 0Ah, 08h, 07h, 08h, 0Bh, 0Ah, 04h, 06h
    db 04h, 07h, 05h, 04h, 04h, 05h, 07h, 04h, 0Bh, 07h, 03h, 06h
    db 06h, 06h, 06h, 03h, 06h, 0Ah, 07h, 05h, 05h, 0Bh, 0Ah, 08h
    db 09h, 00h, 9Ah, 9Ah
word_A3E5 label word
; UNKNOWN: not yet classified as code or data
    db 9Bh, 00h, 0E0h, 00h, 0E1h, 00h, 2Dh, 01h, 2Eh, 01h, 2Fh, 01h
byte_A3F1 label byte
; DATA: debris count table: byte i = pieces for (object phase >> 3) & 7 (6 bytes + 2 zero)
; UNKNOWN: not yet classified as code or data
    db 01h, 02h, 03h, 04h, 06h, 08h
word_A3F7 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
byte_A3F9 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_A3FA label byte
; UNKNOWN: not yet classified as code or data
    db 00h
word_A3FB label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_A3FD label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_A3FF label word
; UNKNOWN: not yet classified as code or data
    db 27h, 0A4h
word_A401 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_A403 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_A405 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_A407 label word
; UNKNOWN: not yet classified as code or data
    db 00h
word_A408 label word
; UNKNOWN: not yet classified as code or data
    db 00h
word_A409 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_A40B label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_A40D label word
; UNKNOWN: not yet classified as code or data
    db 00h
word_A40E label word
; UNKNOWN: not yet classified as code or data
    db 00h
word_A40F label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_A411 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_A413 label word
; UNKNOWN: not yet classified as code or data
    db 00h
word_A414 label word
; UNKNOWN: not yet classified as code or data
    db 00h
word_A415 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_A417 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_A419 label word
; UNKNOWN: not yet classified as code or data
    db 00h
word_A41A label word
; UNKNOWN: not yet classified as code or data
    db 00h
word_A41B label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_A41D label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_A41F label word
; UNKNOWN: not yet classified as code or data
    db 00h
word_A420 label word
; UNKNOWN: not yet classified as code or data
    db 00h
word_A421 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_A423 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
word_A425 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 40h, 40h, 40h, 40h
    db 40h, 40h, 0F4h, 00h, 00h, 00h, 40h, 40h, 40h, 0FAh, 10h, 50h
    db 10h, 11h, 51h, 11h, 10h, 50h, 10h, 11h, 51h, 11h, 10h, 50h
    db 10h, 11h, 51h, 11h, 00h, 00h, 40h, 40h, 00h, 00h, 40h, 40h
    db 00h, 00h, 40h, 40h, 00h, 00h, 40h, 40h, 0DEh, 03h, 0FFh, 04h
    db 0FFh, 07h, 0FFh, 0Dh, 0Dh, 0Dh, 0Eh, 0Eh, 0Eh, 0FAh, 0Dh, 0Dh
    db 0Ch, 0Ch, 0FCh, 0Dh, 0Dh, 0Eh, 0Eh, 0FCh, 12h, 12h, 52h, 12h
    db 0Eh, 0Eh, 4Eh, 0Eh, 0F8h, 05h, 0FFh, 05h, 05h, 0Fh, 0Fh, 0FCh
    db 0A9h, 03h, 0D4h, 07h, 0A4h, 01h, 0D1h, 03h, 0B9h, 07h, 0A5h, 01h
    db 0A2h, 03h, 0A6h, 07h, 0A6h, 01h, 0B4h, 03h, 9Fh, 07h, 0A7h, 01h
    db 96h, 03h, 8Ah, 07h, 0A8h, 01h, 0B6h, 03h, 9Ch, 07h, 0A9h, 01h
    db 0E0h, 03h, 95h, 07h, 0B6h, 01h, 00h, 00h, 00h, 00h, 0FFh, 0FFh
    db 0D5h, 03h, 0C1h, 07h, 0AAh, 01h, 0FAh, 03h, 0B6h, 07h, 0ABh, 01h
    db 0E8h, 03h, 0A8h, 07h, 0ACh, 01h, 0FFh, 03h, 88h, 07h, 0ADh, 01h
    db 0D9h, 03h, 9Bh, 07h, 0AEh, 01h, 0FFh, 03h, 0A8h, 07h, 0AFh, 01h
    db 05h, 04h, 0A9h, 07h, 0B7h, 01h, 00h, 00h, 00h, 00h, 0FFh, 0FFh
    db 0E3h, 03h, 73h, 07h, 0B0h, 01h, 0E4h, 03h, 71h, 07h, 0B1h, 01h
    db 0E4h, 03h, 73h, 07h, 0B2h, 01h
word_A4F7 label word
; UNKNOWN: not yet classified as code or data
    db 0F9h, 0A4h, 1Bh, 0A5h, 05h, 00h, 5Ah, 0A5h, 01h, 00h, 1Bh, 0A5h
    db 03h, 00h, 5Ah, 0A5h, 02h, 00h, 1Bh, 0A5h, 02h, 00h, 40h, 0A5h
    db 01h, 00h, 0FFh, 0FFh, 0E8h, 0FFh
byte_A515 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_A516 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
word_A517 label word
; UNKNOWN: not yet classified as code or data
    db 1Bh, 0A5h
word_A519 label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 01h, 01h, 01h, 01h, 01h, 01h, 01h, 01h, 01h, 01h
    db 01h, 01h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h
    db 04h, 04h, 0E8h, 03h, 03h, 03h, 0FDh, 03h, 03h, 03h, 03h, 03h
    db 03h, 03h, 0F1h, 00h, 00h, 0FFh, 03h, 03h, 0FEh, 03h, 00h, 00h
    db 00h, 03h, 03h, 03h, 03h, 00h, 0FEh, 00h, 03h, 03h, 03h, 00h
    db 00h, 03h, 03h, 03h, 0E7h, 04h, 04h, 04h, 04h, 04h, 04h, 04h
    db 01h, 01h, 01h, 01h, 01h, 01h, 01h, 00h, 00h, 00h, 0FFh, 00h
    db 00h, 00h, 00h, 0EAh, 0A2h, 01h, 9Ah, 01h, 95h, 01h, 9Fh, 01h
    db 0A0h, 01h, 1Bh, 0A4h, 0Fh, 0A4h, 09h, 0A4h, 03h, 0A4h, 15h, 0A4h
    db 0A2h, 01h, 9Ah, 01h, 96h, 01h, 9Dh, 01h, 0A0h, 01h, 1Bh, 0A4h
    db 0Fh, 0A4h, 09h, 0A4h, 03h, 0A4h, 15h, 0A4h, 0A2h, 01h, 9Ah, 01h
    db 96h, 01h, 9Ch, 01h, 0A0h, 01h, 1Bh, 0A4h, 0Fh, 0A4h, 09h, 0A4h
    db 03h, 0A4h, 15h, 0A4h, 0A1h, 01h, 99h, 01h, 96h, 01h, 9Dh, 01h
    db 9Dh, 81h, 1Bh, 0A4h, 0Fh, 0A4h, 09h, 0A4h, 03h, 0A4h, 15h, 0A4h
    db 0A2h, 01h, 9Bh, 01h, 96h, 01h, 9Dh, 01h, 9Dh, 81h, 1Bh, 0A4h
    db 0Fh, 0A4h, 09h, 0A4h, 03h, 0A4h, 15h, 0A4h, 0A2h, 01h, 99h, 01h
    db 98h, 81h, 9Eh, 01h, 9Eh, 81h, 1Bh, 0A4h, 0Fh, 0A4h, 09h, 0A4h
    db 03h, 0A4h, 15h, 0A4h, 0A2h, 01h, 9Ah, 01h, 98h, 01h, 9Ch, 01h
    db 9Dh, 81h, 1Bh, 0A4h, 0Fh, 0A4h, 09h, 0A4h, 03h, 0A4h, 15h, 0A4h
    db 0A2h, 01h, 9Ah, 01h, 95h, 01h, 9Eh, 01h, 9Eh, 81h, 1Bh, 0A4h
    db 0Fh, 0A4h, 09h, 0A4h, 03h, 0A4h, 15h, 0A4h, 0A2h, 01h, 99h, 01h
    db 95h, 01h, 9Eh, 01h, 9Dh, 81h, 1Bh, 0A4h, 0Fh, 0A4h, 09h, 0A4h
    db 03h, 0A4h, 15h, 0A4h, 0A3h, 01h, 99h, 01h, 95h, 01h, 9Eh, 01h
    db 9Dh, 81h, 1Bh, 0A4h, 0Fh, 0A4h, 09h, 0A4h, 03h, 0A4h, 15h, 0A4h
    db 0A2h, 01h, 99h, 01h, 95h, 01h, 9Eh, 01h, 0A0h, 01h, 1Bh, 0A4h
    db 0Fh, 0A4h, 09h, 0A4h, 03h, 0A4h, 15h, 0A4h, 0A3h, 01h, 99h, 01h
    db 95h, 01h, 9Dh, 01h, 0A0h, 01h, 1Bh, 0A4h, 0Fh, 0A4h, 09h, 0A4h
    db 03h, 0A4h, 15h, 0A4h, 0A3h, 01h, 9Bh, 01h, 95h, 01h, 9Eh, 01h
    db 9Dh, 81h, 1Bh, 0A4h, 0Fh, 0A4h, 09h, 0A4h, 03h, 0A4h, 15h, 0A4h
    db 0A3h, 01h, 9Bh, 01h, 95h, 01h, 9Dh, 01h, 9Dh, 81h, 1Bh, 0A4h
    db 0Fh, 0A4h, 09h, 0A4h, 03h, 0A4h, 15h, 0A4h, 0A3h, 01h, 9Bh, 01h
    db 95h, 01h, 9Dh, 01h, 0A0h, 01h, 1Bh, 0A4h, 0Fh, 0A4h, 09h, 0A4h
    db 03h, 0A4h, 15h, 0A4h, 0A2h, 01h, 9Ah, 01h, 98h, 01h, 9Ch, 01h
    db 9Dh, 81h, 1Bh, 0A4h, 0Fh, 0A4h, 09h, 0A4h, 03h, 0A4h, 15h, 0A4h
    db 0A2h, 01h, 9Ah, 01h, 97h, 01h, 9Eh, 01h, 9Dh, 81h, 1Bh, 0A4h
    db 0Fh, 0A4h, 09h, 0A4h, 03h, 0A4h, 15h, 0A4h, 0A2h, 01h, 9Ah, 01h
    db 97h, 01h, 9Dh, 01h, 9Eh, 81h, 1Bh, 0A4h, 0Fh, 0A4h, 09h, 0A4h
    db 03h, 0A4h, 15h, 0A4h, 0A3h, 01h, 9Ah, 01h, 95h, 01h, 0A0h, 81h
    db 9Dh, 81h, 1Bh, 0A4h, 0Fh, 0A4h, 09h, 0A4h, 15h, 0A4h, 03h, 0A4h
    db 0A2h, 01h, 99h, 01h, 0F8h, 0E9h, 0A2h, 01h, 9Ah, 01h, 0FBh, 0EBh
    db 0A2h, 01h, 9Bh, 01h, 0F6h, 0E8h, 0A1h, 01h, 99h, 01h, 0F6h, 0DBh
    db 0A1h, 01h, 9Ah, 01h, 01h, 0DDh, 0A1h, 01h, 9Bh, 01h, 0F3h, 0DAh
    db 0A3h, 01h, 99h, 01h, 0EDh, 0F0h, 0A3h, 01h, 9Ah, 01h, 0F0h, 0F2h
    db 0A3h, 01h, 9Bh, 01h, 0EBh, 0F0h, 99h, 01h, 95h, 01h, 0FFh, 0E8h
    db 9Ah, 01h, 95h, 01h, 0FBh, 0EAh, 9Bh, 01h, 95h, 01h, 0F8h, 0F7h
    db 99h, 01h, 96h, 01h, 0FFh, 0E8h, 9Ah, 01h, 96h, 01h, 0FBh, 0EAh
    db 9Bh, 01h, 96h, 01h, 0F8h, 0F7h, 99h, 01h, 97h, 01h, 0FFh, 0E8h
    db 9Ah, 01h, 97h, 01h, 0FBh, 0EAh, 9Bh, 01h, 97h, 01h, 0F8h, 0F7h
    db 99h, 01h, 98h, 01h, 0FFh, 0E8h, 9Ah, 01h, 98h, 01h, 0FBh, 0EAh
    db 9Bh, 01h, 98h, 01h, 0F8h, 0F7h, 99h, 01h, 98h, 81h, 0FFh, 0E8h
    db 9Ah, 01h, 98h, 81h, 0FBh, 0EAh, 9Bh, 01h, 98h, 81h, 0F8h, 0F7h
    db 99h, 01h, 9Ch, 01h, 0E2h, 0ECh, 99h, 01h, 9Dh, 01h, 0DDh, 0F1h
    db 99h, 01h, 9Eh, 01h, 0D6h, 0E9h, 99h, 01h, 9Fh, 01h, 0E8h, 0Ch
    db 9Ah, 01h, 9Ch, 01h, 0E2h, 0ECh, 9Ah, 01h, 9Dh, 01h, 0E0h, 0F1h
    db 9Ah, 01h, 9Eh, 01h, 0D4h, 0EAh, 9Ah, 01h, 9Fh, 01h, 0EAh, 0Fh
    db 9Bh, 01h, 9Ch, 01h, 0DEh, 0F8h, 9Bh, 01h, 9Dh, 01h, 0DCh, 02h
    db 9Bh, 01h, 9Eh, 01h, 0CEh, 0FBh, 9Bh, 01h, 9Fh, 01h, 0E6h, 1Dh
    db 99h, 01h, 9Dh, 81h, 21h, 0EBh, 99h, 01h, 9Eh, 81h, 2Ch, 0E4h
    db 99h, 01h, 0A0h, 01h, 12h, 10h, 9Ah, 01h, 9Dh, 81h, 18h, 0EEh
    db 9Ah, 01h, 9Eh, 81h, 29h, 0EAh, 9Ah, 01h, 0A0h, 01h, 09h, 13h
    db 9Bh, 01h, 9Dh, 81h, 1Ah, 0F9h, 9Bh, 01h, 9Eh, 81h, 29h, 0F3h
    db 9Bh, 01h, 0A0h, 01h, 0Ch, 1Eh, 9Ah, 01h, 0A0h, 81h, 0ECh, 16h
    db 38h, 01h, 3Ch, 01h, 56h, 00h, 3Eh, 01h, 49h, 01h, 6Fh, 00h
    db 4Ah, 01h, 53h, 01h, 6Dh, 00h, 54h, 01h, 55h, 01h, 5Dh, 00h
    db 56h, 01h, 58h, 01h, 5Ch, 00h, 59h, 01h, 5Ch, 01h, 68h, 00h
    db 5Dh, 01h, 62h, 01h, 64h, 00h, 63h, 01h, 66h, 01h, 7Dh, 00h
    db 67h, 01h, 6Dh, 01h, 6Ch, 00h, 6Eh, 01h, 71h, 01h, 6Fh, 00h
    db 72h, 01h, 7Ah, 01h, 6Bh, 00h, 7Bh, 01h, 7Dh, 01h, 56h, 00h
    db 7Eh, 01h, 81h, 01h, 5Ah, 00h, 82h, 01h, 8Bh, 01h, 51h, 00h
    db 8Ch, 01h, 91h, 01h, 5Eh, 00h, 92h, 01h, 94h, 01h, 0A1h, 00h
    db 0FFh, 0FFh, 91h, 00h, 91h, 00h, 92h, 00h, 92h, 00h, 93h, 00h
    db 93h, 00h, 0F4h, 0FFh, 01h, 7Dh, 00h, 00h, 28h, 00h, 0FEh, 0FFh
    db 26h, 00h, 26h, 00h, 26h, 00h, 26h, 00h, 25h, 00h, 25h, 00h
    db 25h, 00h, 25h, 00h, 27h, 00h, 27h, 00h, 27h, 00h, 27h, 00h
    db 25h, 00h, 25h, 00h, 25h, 00h, 25h, 00h, 0E0h, 0FFh, 00h, 7Dh
    db 2Ah, 00h, 0FEh, 0FFh, 96h, 00h, 0FEh, 0FFh, 94h, 00h, 94h, 00h
    db 94h, 00h, 95h, 00h, 95h, 00h, 95h, 00h, 96h, 00h, 96h, 00h
    db 96h, 00h, 97h, 00h, 97h, 00h, 97h, 00h, 0E8h, 0FFh, 00h, 7Dh
    db 94h, 00h, 0FEh, 0FFh, 01h, 7Dh, 01h, 00h, 05h, 00h, 0FEh, 0FFh
    db 01h, 7Dh, 02h, 00h, 06h, 00h, 07h, 00h, 0FEh, 0FFh, 06h, 00h
    db 0FEh, 0FFh, 00h, 7Dh, 11h, 00h, 0FEh, 0FFh, 01h, 7Dh, 03h, 00h
    db 36h, 00h, 36h, 00h, 37h, 00h, 37h, 00h, 38h, 00h, 38h, 00h
    db 39h, 00h, 39h, 00h, 06h, 00h, 0FEh, 0FFh, 07h, 00h, 0FEh, 0FFh
    db 08h, 00h, 08h, 00h, 09h, 00h, 09h, 00h, 0Ah, 00h, 0Ah, 00h
    db 0Bh, 00h, 0Bh, 00h, 0F0h, 0FFh, 10h, 00h, 10h, 00h, 0Ch, 00h
    db 0Ch, 00h, 0Dh, 00h, 0Dh, 00h, 0Eh, 00h, 0Eh, 00h, 0Fh, 00h
    db 0Fh, 00h, 0F0h, 0FFh, 00h, 7Dh, 11h, 00h, 0FEh, 0FFh, 01h, 7Dh
    db 04h, 00h, 06h, 00h, 07h, 00h, 0FEh, 0FFh, 00h, 7Dh, 11h, 00h
    db 0FEh, 0FFh, 01h, 7Dh, 05h, 00h, 12h, 00h, 13h, 00h, 0FCh, 0FFh
    db 14h, 00h, 15h, 00h, 0FCh, 0FFh, 00h, 7Dh, 16h, 00h, 0FEh, 0FFh
    db 5Ch, 00h, 5Ch, 00h, 5Ch, 00h, 5Ch, 00h, 5Ah, 00h, 5Ah, 00h
    db 5Ah, 00h, 5Ah, 00h, 5Bh, 00h, 5Bh, 00h, 5Bh, 00h, 5Bh, 00h
    db 0E8h, 0FFh, 5Ch, 00h, 5Ch, 00h, 5Ch, 00h, 5Ah, 00h, 5Ah, 00h
    db 5Ah, 00h, 5Bh, 00h, 5Bh, 00h, 5Bh, 00h, 0EEh, 0FFh, 00h, 7Dh
    db 5Bh, 00h, 0FEh, 0FFh, 01h, 7Dh, 06h, 00h, 12h, 00h, 13h, 00h
    db 0FCh, 0FFh, 00h, 7Dh, 16h, 00h, 0FEh, 0FFh, 8Dh, 00h, 8Dh, 00h
    db 8Dh, 00h, 8Eh, 00h, 8Eh, 00h, 8Eh, 00h, 8Fh, 00h, 8Fh, 00h
    db 8Fh, 00h, 8Eh, 00h, 8Eh, 00h, 8Eh, 00h, 0E8h, 0FFh, 00h, 7Dh
    db 90h, 00h, 0FEh, 0FFh, 01h, 7Dh, 07h, 00h, 12h, 00h, 13h, 00h
    db 0FCh, 0FFh, 12h, 00h, 13h, 00h, 0FCh, 0FFh, 00h, 7Dh, 16h, 00h
    db 0FEh, 0FFh, 5Ch, 00h, 5Ch, 00h, 5Ch, 00h, 5Ch, 00h, 5Ah, 00h
    db 5Ah, 00h, 5Ah, 00h, 5Ah, 00h, 5Bh, 00h, 5Bh, 00h, 5Bh, 00h
    db 5Bh, 00h, 0E8h, 0FFh, 5Ch, 00h, 0FEh, 0FFh, 00h, 7Dh, 5Ah, 00h
    db 0FEh, 0FFh, 01h, 7Dh, 08h, 00h, 21h, 00h, 0FEh, 0FFh, 22h, 00h
    db 22h, 00h, 22h, 00h, 22h, 00h, 24h, 00h, 0FEh, 0FFh, 24h, 00h
    db 24h, 00h, 24h, 00h, 24h, 00h, 23h, 00h, 0FEh, 0FFh, 00h, 7Dh
    db 22h, 00h, 0FEh, 0FFh, 1Dh, 00h, 0FEh, 0FFh, 1Ch, 00h, 0FEh, 0FFh
    db 1Dh, 00h, 0FEh, 0FFh, 00h, 7Dh, 1Ch, 00h, 0FEh, 0FFh, 1Eh, 00h
    db 0FEh, 0FFh, 1Fh, 00h, 1Fh, 00h, 20h, 00h, 20h, 00h, 1Eh, 00h
    db 1Eh, 00h, 0F4h, 0FFh, 1Fh, 00h, 1Fh, 00h, 20h, 00h, 20h, 00h
    db 1Eh, 00h, 1Eh, 00h, 0F4h, 0FFh, 00h, 7Dh, 1Eh, 00h, 0FEh, 0FFh
    db 25h, 00h, 0FEh, 0FFh, 28h, 00h, 28h, 00h, 29h, 00h, 0FEh, 0FFh
    db 28h, 00h, 0FEh, 0FFh, 00h, 7Dh, 2Ah, 00h, 0FEh, 0FFh, 54h, 00h
    db 54h, 00h, 54h, 00h, 54h, 00h, 54h, 00h, 54h, 00h, 55h, 00h
    db 55h, 00h, 55h, 00h, 55h, 00h, 55h, 00h, 55h, 00h, 0E8h, 0FFh
    db 57h, 00h, 0FEh, 0FFh, 58h, 00h, 0FEh, 0FFh, 00h, 7Dh, 59h, 00h
    db 0FEh, 0FFh, 01h, 7Dh, 09h, 00h, 17h, 00h, 17h, 00h, 18h, 00h
    db 18h, 00h, 0F8h, 0FFh, 00h, 7Dh, 1Bh, 00h, 0FEh, 0FFh, 46h, 00h
    db 46h, 00h, 47h, 00h, 47h, 00h, 48h, 00h, 48h, 00h, 0F4h, 0FFh
    db 00h, 7Dh, 49h, 00h, 0FEh, 0FFh, 2Bh, 00h, 2Bh, 00h, 2Ch, 00h
    db 2Ch, 00h, 2Dh, 00h, 2Dh, 00h, 0F4h, 0FFh, 00h, 7Dh, 2Eh, 00h
    db 0FEh, 0FFh, 01h, 00h, 01h, 00h, 02h, 00h, 02h, 00h, 03h, 00h
    db 03h, 00h, 02h, 00h, 02h, 00h, 0F0h, 0FFh, 00h, 7Dh, 04h, 00h
    db 0FEh, 0FFh, 5Ah, 00h, 5Ah, 00h, 5Bh, 00h, 5Bh, 00h, 5Ch, 00h
    db 5Ch, 00h, 0F4h, 0FFh, 00h, 7Dh, 5Ah, 00h, 0FEh, 0FFh, 81h, 00h
    db 81h, 00h, 81h, 00h, 82h, 00h, 82h, 00h, 82h, 00h, 83h, 00h
    db 83h, 00h, 83h, 00h, 0EEh, 0FFh, 00h, 7Dh, 84h, 00h, 84h, 00h
    db 85h, 00h, 85h, 00h, 86h, 00h, 86h, 00h, 8Ah, 00h, 8Ah, 00h
    db 8Bh, 00h, 8Bh, 00h, 87h, 00h, 87h, 00h, 0ECh, 0FFh, 01h, 7Dh
    db 0Ah, 00h, 45h, 00h, 0FEh, 0FFh, 44h, 00h, 44h, 00h, 44h, 00h
    db 44h, 00h, 43h, 00h, 43h, 00h, 43h, 00h, 43h, 00h, 00h, 00h
    db 00h, 00h, 00h, 00h, 00h, 00h, 03h, 00h, 03h, 00h, 03h, 00h
    db 03h, 40h, 01h, 00h, 01h, 00h, 01h, 00h, 02h, 00h, 02h, 00h
    db 02h, 00h, 03h, 00h, 03h, 00h, 03h, 00h, 02h, 00h, 02h, 00h
    db 02h, 00h, 0E8h, 0FFh, 00h, 00h, 00h, 00h, 43h, 00h, 43h, 00h
    db 44h, 00h, 44h, 00h, 45h, 40h, 45h, 00h, 0FEh, 0FFh, 00h, 7Dh
    db 04h, 00h, 0FEh, 0FFh, 4Ah, 00h, 0FEh, 0FFh, 4Bh, 00h, 4Bh, 00h
    db 4Bh, 00h, 4Ch, 00h, 4Ch, 00h, 4Ch, 00h, 4Dh, 00h, 4Dh, 00h
    db 4Dh, 00h, 4Eh, 00h, 4Eh, 00h, 4Eh, 40h, 4Fh, 00h, 4Fh, 00h
    db 4Fh, 00h, 50h, 00h, 50h, 00h, 50h, 00h, 51h, 00h, 51h, 00h
    db 51h, 00h, 52h, 00h, 52h, 00h, 52h, 00h, 0E8h, 0FFh, 4Eh, 00h
    db 4Eh, 00h, 4Dh, 00h, 4Dh, 00h, 4Ch, 00h, 4Ch, 00h, 4Bh, 00h
    db 4Bh, 00h, 4Ah, 40h, 4Ah, 00h, 0FEh, 0FFh, 00h, 7Dh, 53h, 00h
    db 0FEh, 0FFh, 3Ah, 00h, 0FEh, 0FFh, 3Ah, 00h, 3Ah, 00h, 3Ah, 00h
    db 3Bh, 00h, 3Bh, 00h, 3Bh, 00h, 3Ch, 00h, 3Ch, 00h, 3Ch, 00h
    db 3Dh, 00h, 3Dh, 00h, 3Dh, 40h, 3Eh, 00h, 3Eh, 00h, 3Eh, 00h
    db 3Fh, 00h, 3Fh, 00h, 3Fh, 00h, 0F4h, 0FFh, 3Fh, 00h, 3Fh, 00h
    db 3Fh, 00h, 40h, 00h, 40h, 00h, 40h, 00h, 41h, 00h, 41h, 00h
    db 41h, 00h, 42h, 00h, 42h, 00h, 42h, 40h, 42h, 00h, 0FEh, 0FFh
    db 00h, 7Dh, 3Fh, 00h, 0FEh, 0FFh, 01h, 7Dh, 0Bh, 00h, 2Fh, 00h
    db 2Fh, 00h, 2Fh, 00h, 30h, 00h, 30h, 00h, 30h, 00h, 2Fh, 00h
    db 2Fh, 00h, 2Fh, 00h, 30h, 00h, 30h, 00h, 30h, 00h, 2Fh, 00h
    db 2Fh, 00h, 2Fh, 00h, 30h, 00h, 30h, 00h, 30h, 00h, 31h, 00h
    db 31h, 00h, 32h, 00h, 32h, 40h, 33h, 00h, 33h, 00h, 34h, 00h
    db 34h, 00h, 0F8h, 0FFh, 00h, 7Dh, 35h, 00h, 0FEh, 0FFh, 01h, 7Dh
    db 0Ch, 00h, 80h, 00h, 88h, 00h, 89h, 00h, 0FCh, 0FFh, 00h, 7Dh
    db 84h, 00h, 84h, 00h, 85h, 00h, 85h, 00h, 86h, 00h, 86h, 00h
    db 8Ah, 00h, 8Ah, 00h, 8Bh, 00h, 8Bh, 00h, 87h, 00h, 87h, 00h
    db 0ECh, 0FFh, 2Bh, 00h, 2Bh, 00h, 2Ch, 00h, 2Ch, 00h, 2Dh, 00h
    db 2Dh, 00h, 0F4h, 0FFh, 00h, 7Dh, 2Eh, 00h, 0FEh, 0FFh, 3Eh, 00h
    db 3Eh, 00h, 3Fh, 00h, 3Fh, 00h, 0F8h, 0FFh, 00h, 7Dh, 3Eh, 00h
    db 0FEh, 0FFh, 57h, 00h, 57h, 00h, 58h, 00h, 58h, 00h, 0F8h, 0FFh
    db 00h, 7Dh, 59h, 00h, 0FEh, 0FFh, 00h, 00h, 00h, 0Fh, 12h, 18h
    db 0Ch, 0Fh, 15h, 09h, 0Ch, 12h, 00h, 0Fh, 12h, 0Ch, 0Ch, 12h
    db 00h, 00h, 00h, 00h, 15h, 18h, 00h, 0Ch, 0Ch, 00h, 0Fh, 0Fh
    db 06h, 12h, 12h, 00h, 09h, 0Ch, 00h, 0Ch, 12h, 00h, 0Fh, 18h
    db 00h, 12h, 1Bh, 0Fh, 15h, 1Bh, 00h, 00h, 00h
; DATA: text
    db '8?.6:$'
; UNKNOWN: not yet classified as code or data
    db 35h, 35h, 1Bh, 30h, 2Ah, 14h, 2Bh, 1Fh, 0Dh, 26h, 14h, 08h
    db 21h, 0Ah, 03h, 1Ch, 00h, 00h, 06h, 00h, 00h, 28h, 1Ch, 14h
    db 0Eh, 0Ch, 0Eh, 06h, 0Ah, 0Ah, 3Ah, 12h, 20h, 3Ah, 12h, 20h
    db 3Ah, 12h, 20h, 28h, 00h, 08h, 3Ch, 2Ch, 10h, 34h, 20h, 00h
; DATA: text
    db '80(<4 <8(<<0,$'
; UNKNOWN: not yet classified as code or data
    db 24h, 0Ch, 10h, 04h, 10h, 18h, 08h, 14h, 20h, 0Ch, 1Ch, 28h
    db 1Ch, 20h, 2Ch, 20h, 24h
; DATA: text
    db '4$'
; DATA: text
    db ',<,0,(', 0
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 3Ch, 3Ch, 3Ch, 38h, 28h, 20h, 28h, 18h, 10h, 20h
    db 10h, 08h, 14h, 10h, 14h, 18h, 00h, 00h, 18h, 14h, 18h, 18h
    db 18h, 1Ch, 20h, 20h, 1Ch, 24h, 1Ch, 1Ch, 28h, 20h, 18h, 18h
    db 24h, 18h, 14h, 0Ch, 14h, 38h, 38h, 38h, 28h, 28h, 28h, 14h
    db 0Ch, 14h, 1Ch, 0Ch, 14h, 2Eh, 24h, 1Eh, 32h, 28h, 20h, 35h
    db 2Eh, 25h, 35h, 2Eh, 25h, 38h, 34h, 2Ch, 14h, 0Ch, 14h, 14h
    db 12h, 08h, 10h, 12h, 0Ah, 00h, 00h, 00h, 38h, 34h, 2Ch, 18h
    db 0Ch, 10h, 14h, 10h, 14h, 14h, 10h, 14h, 14h, 0Ch, 14h, 14h
    db 0Ch, 14h, 14h, 10h, 14h, 00h, 0Ah, 14h, 0Eh, 06h, 10h, 06h
    db 00h, 00h, 06h, 00h, 00h, 06h, 00h, 00h, 06h, 00h, 00h, 06h
    db 00h, 00h, 06h, 00h, 00h, 06h, 00h, 00h, 06h, 00h, 00h, 06h
    db 00h, 00h, 06h, 00h, 00h, 06h, 00h, 00h, 06h, 00h, 00h, 06h
    db 00h, 00h, 06h, 00h, 00h, 06h, 00h, 00h, 06h, 00h, 00h, 06h
    db 00h, 00h, 06h, 00h, 00h, 06h, 00h, 00h, 06h, 00h, 00h, 1Ch
    db 00h, 00h, 1Ch, 00h, 00h, 1Ch, 00h, 00h, 1Ch, 00h, 00h, 1Ch
    db 00h, 00h, 1Ch, 00h, 00h, 1Ch, 00h, 00h, 1Ch, 00h, 00h, 1Ch
    db 00h, 00h, 1Ch, 00h, 00h, 1Ch, 00h, 00h, 1Ch, 00h, 00h, 1Ch
    db 00h, 00h, 1Ch, 00h, 00h, 1Ch, 00h, 00h, 1Ch, 00h, 00h, 1Ch
    db 00h, 00h, 1Ch, 00h, 00h, 1Ch, 00h, 00h, 1Ch, 00h, 00h, 1Ch
    db 00h, 00h, 21h, 0Ah, 03h, 21h, 09h, 03h, 21h, 09h, 03h, 21h
    db 09h, 03h, 21h, 09h, 03h, 21h, 09h, 03h, 21h, 09h, 03h, 21h
    db 09h, 03h, 21h, 09h, 03h, 21h, 09h, 03h, 21h, 09h, 03h, 21h
    db 09h, 03h, 21h, 09h, 03h, 21h, 09h, 03h, 21h, 09h, 03h, 21h
    db 09h, 03h, 21h, 09h, 03h, 21h, 09h, 03h, 21h, 09h, 03h, 26h
    db 14h, 08h, 26h, 14h, 08h, 26h, 14h, 08h, 26h, 14h, 08h, 26h
    db 14h, 08h, 26h, 14h, 08h, 26h, 14h, 08h, 26h, 14h, 08h, 26h
    db 14h, 08h, 26h, 14h, 08h, 26h, 14h, 08h, 26h, 14h, 08h, 26h
    db 14h, 08h, 26h, 14h, 08h, 26h, 14h, 08h, 26h, 14h, 08h, 26h
    db 14h, 08h, 26h, 14h, 08h, 26h, 14h, 08h, 26h, 14h, 08h, 2Bh
    db 1Fh, 0Dh, 2Bh, 1Eh, 0Dh, 2Bh, 1Eh, 0Dh, 2Bh, 1Eh, 0Dh, 2Bh
    db 1Eh, 0Dh, 2Bh, 1Eh, 0Dh, 2Bh, 1Eh, 0Dh, 2Bh, 1Eh, 0Dh, 2Bh
    db 1Eh, 0Dh, 2Bh, 1Eh, 0Dh, 2Bh, 1Eh, 0Dh, 2Bh, 1Eh, 0Dh, 2Bh
    db 1Eh, 0Dh, 2Bh, 1Eh, 0Dh, 2Bh, 1Eh, 0Dh, 2Bh, 1Eh, 0Dh, 2Bh
    db 1Eh, 0Dh, 2Bh, 1Eh, 0Dh, 2Bh, 1Eh, 0Dh, 2Bh, 1Eh, 0Dh, 30h
    db 2Ah, 14h, 30h, 29h, 14h, 30h, 29h, 14h, 30h, 29h, 14h, 30h
    db 29h, 14h, 30h, 29h, 14h, 30h, 29h, 14h, 30h, 29h, 14h, 30h
    db 29h, 14h, 30h, 29h, 14h, 30h, 29h, 14h, 30h, 29h, 14h, 30h
    db 29h, 14h, 30h, 29h, 14h, 30h, 29h, 14h, 30h, 29h, 14h, 30h
    db 29h, 14h, 30h, 29h, 14h, 30h, 29h, 14h, 30h, 29h, 14h, 35h
    db 35h, 1Bh, 35h, 34h, 1Bh, 35h, 34h, 1Bh, 35h, 34h, 1Bh, 35h
    db 34h, 1Bh, 35h, 34h, 1Bh, 35h, 34h, 1Bh, 35h, 34h, 1Bh, 35h
    db 34h, 1Bh, 35h, 34h, 1Bh, 35h, 34h, 1Bh, 35h, 34h, 1Bh, 35h
    db 34h, 1Bh, 35h, 34h, 1Bh, 35h, 34h, 1Bh, 35h, 34h, 1Bh, 35h
    db 34h, 1Bh, 35h, 34h, 1Bh, 35h, 34h, 1Bh, 35h, 34h, 1Bh, 36h
    db 3Ah, 24h
; DATA: text
    db '6:$'
; DATA: text
    db '6:$'
; DATA: text
    db '6:$'
; DATA: text
    db '6:$'
; DATA: text
    db '6:$'
; DATA: text
    db '6:$'
; DATA: text
    db '6:$'
; DATA: text
    db '6:$'
; DATA: text
    db '6:$'
; DATA: text
    db '6:$'
; DATA: text
    db '6:$'
; DATA: text
    db '6:$'
; DATA: text
    db '6:$'
; DATA: text
    db '6:$'
; DATA: text
    db '6:$'
; DATA: text
    db '6:$'
; DATA: text
    db '6:$'
; DATA: text
    db '6:$'
; DATA: text
    db '6:$'
; UNKNOWN: not yet classified as code or data
    db 38h, 3Fh, 2Eh, 38h, 3Fh, 2Eh, 38h, 3Fh, 2Eh, 38h, 3Fh, 2Eh
    db 38h, 3Fh, 2Eh, 38h, 3Fh, 2Eh, 38h, 3Fh, 2Eh, 38h, 3Fh, 2Eh
    db 38h, 3Fh, 2Eh, 38h, 3Fh, 2Eh, 38h, 3Fh, 2Eh, 38h, 3Fh, 2Eh
    db 38h, 3Fh, 2Eh, 38h, 3Fh, 2Eh, 38h, 3Fh, 2Eh, 38h, 3Fh, 2Eh
    db 38h, 3Fh, 2Eh, 38h, 3Fh, 2Eh, 38h, 3Fh, 2Eh, 38h, 3Fh, 2Eh
    db 3Ah, 12h, 22h, 3Ah, 12h, 22h, 3Ah, 12h, 22h, 3Ah, 12h, 22h
    db 3Ah, 12h, 22h, 3Ah, 12h, 22h, 3Ah, 12h
; DATA: text
    db '"???', 0
; DATA: VGA DAC palette: 16 RGB triples (6-bit) for RunGameOverScreen
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 00h, 38h, 28h, 20h, 28h, 18h, 10h, 20h, 10h, 08h
    db 04h, 00h, 1Ch, 28h, 00h, 08h, 00h, 04h, 10h, 38h, 30h, 08h
    db 00h, 14h, 00h, 00h, 20h, 00h, 08h, 30h, 08h, 34h, 20h, 14h
    db 00h, 10h, 24h, 14h, 20h, 24h, 20h, 28h, 2Ch, 38h, 38h, 38h
; UNKNOWN: not yet classified as code or data
    db 47h, 41h, 4Dh, 45h, 4Fh, 56h, 45h, 52h, 01h, 00h, 01h, 00h
    db 02h, 00h, 02h, 00h, 03h, 00h, 03h, 00h, 02h, 00h, 02h, 00h
    db 0F0h, 0FFh, 26h, 00h, 26h, 00h, 26h, 00h, 26h, 00h, 25h, 00h
    db 25h, 00h, 25h, 00h, 25h, 00h, 27h, 00h, 27h, 00h, 27h, 00h
    db 27h, 00h, 25h, 00h, 25h, 00h, 25h, 00h, 25h, 00h, 0E0h, 0FFh
    db 81h, 00h, 81h, 00h, 81h, 00h, 82h, 00h, 82h, 00h, 82h, 00h
    db 83h, 00h, 83h, 00h, 83h, 00h, 0EEh, 0FFh
; DATA: text
    db '--1234567890----A-E-----------A-DF------------C-B-------------------------------------------------------------------------------'
; DATA: VGA DAC palette: 16 RGB triples (6-bit) for the map screen (INT 10h 1012h)
    db 0
; UNKNOWN: not yet classified as code or data
    db 00h, 00h, 38h, 28h, 20h, 28h, 18h, 10h, 20h, 10h, 08h, 10h
    db 28h, 38h, 28h, 00h, 08h, 00h, 00h, 00h, 38h, 30h, 08h, 00h
    db 14h, 00h, 00h, 20h, 00h, 08h, 30h, 08h, 34h, 20h, 14h, 14h
    db 18h, 1Ch, 1Ch, 20h, 24h, 28h, 28h, 2Ch, 38h, 38h, 38h
; DATA: VGA DAC palette: 16 RGB triples (6-bit) for RunCallbackMapScreen
; UNKNOWN: not yet classified as code or data
    db 08h
    db 10h, 1Ch, 00h, 04h, 04h, 0Ch, 14h, 20h, 10h, 18h, 20h, 38h
    db 30h, 08h, 38h, 30h, 08h, 38h, 30h, 08h, 38h, 30h, 08h, 34h
    db 20h, 14h, 34h, 20h, 14h, 34h, 20h, 14h, 34h, 20h, 14h, 00h
    db 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
word_B148 label word
; DATA: map stage node table: 10 word pairs (x in pixels, y) for stages 0..9
; UNKNOWN: not yet classified as code or data
    db 05h, 00h
word_B14A label word
; UNKNOWN: not yet classified as code or data
    db 55h, 00h, 51h, 00h, 5Fh, 00h, 78h, 00h, 7Fh, 00h, 98h, 00h
    db 93h, 00h, 0E9h, 00h, 8Ah, 00h, 49h, 01h, 71h, 00h, 0A9h, 01h
    db 6Ah, 00h, 14h, 02h, 0B2h, 00h, 5Eh, 02h, 6Ch, 00h, 5Eh, 02h
    db 50h, 00h
; DATA: text
    db '[[[[', 0
; DATA: text
    db 'ENTER CODE', 0
; DATA: text
    db 'MODE', 0
; DATA: text
    db 'BEGINNER', 0
; DATA: text
    db ' EXPERT ', 0
ExpertFlag label byte
; UNKNOWN: not yet classified as code or data
    db 00h
byte_B198 label byte
; UNKNOWN: not yet classified as code or data
    db 00h
PrevMapScrollX label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
PrevMapScrollY label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
MapScrollX label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
MapScrollY label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
ScreenStartOffset label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
PrevScreenStartOffset label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
WavePhase label byte
; UNKNOWN: not yet classified as code or data
    db 00h
TextCursorOffset label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
CodeInputCount label word
; UNKNOWN: not yet classified as code or data
    db 00h, 00h
CodeEntryState label byte
; UNKNOWN: not yet classified as code or data
    db 00h
TextAdvance label byte
; UNKNOWN: not yet classified as code or data
    db 00h
TextFontBank label word
; UNKNOWN: not yet classified as code or data
    db 2 dup(?)
FrameCallback label word
    db 2 dup(?)
CodeErrorTimer label word
; UNKNOWN: not yet classified as code or data
    db 2 dup(?)
ScreenExitRequested label byte
    db 1 dup(?)
CodeHistory0 label word
; UNKNOWN: not yet classified as code or data
    db 2 dup(?)
CodeHistory1 label word
; UNKNOWN: not yet classified as code or data
    db 2 dup(?)
CodeHistory2 label word
; UNKNOWN: not yet classified as code or data
    db 2 dup(?)
CodeValue label word
; UNKNOWN: not yet classified as code or data
    db 2 dup(?)
ScreenFirstFrame label byte
    db 1 dup(?)
DATA ends
end ProgramEntry
