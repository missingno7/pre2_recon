; Prehistorik 2 (PRE2.EXE), link module 3 of 4
; module interface (tools/symtab.py)
public sub_454E
public sub_45AF
public sub_4907
public sub_4B8E
public sub_4C30
public sub_4C69
public sub_5237
public sub_52FE
public sub_53F6
public sub_54AB
public sub_55FC
public sub_5634
public sub_5643
public sub_581E
public sub_5850
public sub_5CCE
public sub_5CDB
public sub_5E62
public sub_5E96
public sub_5EC4
public sub_5F13
public sub_5F30
public sub_5F96
public sub_60DF
public sub_60FE
public sub_6210
public sub_6272
public sub_6277
public sub_652C
public sub_6539
public sub_65AF
public sub_65EF
public sub_6641
public sub_6645
public sub_6657
public sub_6660
public sub_6669
public sub_6672
public sub_6673
public sub_6772
public sub_67D7
public sub_6822
public sub_8295
public sub_88D7
public sub_8922
public sub_8E45
public sub_9090
public sub_912B
public sub_919F
public sub_9286
public sub_932F
DATA segment word public 'DATA'
extrn AnimVisibleFlag:byte
extrn byte_0666:byte
extrn byte_083D:byte
extrn byte_083E:byte
extrn byte_27D6:byte
extrn byte_27D8:byte
extrn byte_27E8:byte
extrn byte_27E9:byte
extrn byte_27EA:byte
extrn byte_27EB:byte
extrn byte_27EC:byte
extrn byte_27ED:byte
extrn byte_27F6:byte
extrn byte_27F7:byte
extrn byte_280D:byte
extrn byte_2810:byte
extrn byte_282D:byte
extrn byte_282F:byte
extrn byte_2830:byte
extrn byte_2879:byte
extrn byte_287C:byte
extrn byte_287D:byte
extrn byte_4F1B:byte
extrn byte_4F21:byte
extrn byte_4F23:byte
extrn byte_4F24:byte
extrn byte_4F27:byte
extrn byte_4F2B:byte
extrn byte_4F2C:byte
extrn byte_4F2D:byte
extrn byte_564D:byte
extrn byte_565F:byte
extrn byte_5671:byte
extrn byte_5683:byte
extrn byte_5695:byte
extrn byte_6BC0:byte
extrn byte_6BC1:byte
extrn byte_6BC5:byte
extrn byte_6BC6:byte
extrn byte_6BC7:byte
extrn byte_6BC8:byte
extrn byte_6BC9:byte
extrn byte_6BCC:byte
extrn byte_6BCD:byte
extrn byte_6BCE:byte
extrn byte_6BCF:byte
extrn byte_6BD0:byte
extrn byte_6BD1:byte
extrn byte_6BD2:byte
extrn byte_6BD3:byte
extrn byte_6BD9:byte
extrn byte_6BDB:byte
extrn byte_6BDC:byte
extrn byte_6BE0:byte
extrn byte_6BE1:byte
extrn byte_6BE4:byte
extrn byte_6BE5:byte
extrn byte_6BE6:byte
extrn byte_6BE8:byte
extrn byte_6BEA:byte
extrn byte_6BF3:byte
extrn byte_6BFE:byte
extrn byte_6BFF:byte
extrn byte_6C00:byte
extrn byte_6C01:byte
extrn byte_6C02:byte
extrn byte_6C03:byte
extrn byte_6C04:byte
extrn byte_6C05:byte
extrn byte_6C12:byte
extrn byte_6CA4:byte
extrn byte_6CA5:byte
extrn byte_6CA6:byte
extrn byte_6CA7:byte
extrn byte_6CA8:byte
extrn byte_7090:byte
extrn byte_78C6:byte
extrn byte_7B06:byte
extrn byte_7B07:byte
extrn byte_7B08:byte
extrn byte_7B18:byte
extrn byte_7B19:byte
extrn byte_7B1A:byte
extrn byte_7B7F:byte
extrn byte_7F5E:byte
extrn byte_91FB:byte
extrn byte_91FE:byte
extrn byte_A312:byte
extrn byte_A324:byte
extrn byte_A325:byte
extrn byte_A328:byte
extrn byte_A329:byte
extrn byte_A32A:byte
extrn byte_A32B:byte
extrn byte_A32C:byte
extrn byte_A330:byte
extrn byte_A335:byte
extrn byte_A340:byte
extrn byte_A3F9:byte
extrn byte_A3FA:byte
extrn byte_A515:byte
extrn byte_A516:byte
extrn byte_B198:byte
extrn CameraCol:word
extrn CameraLeftLimit:word
extrn CameraRow:word
extrn CheckpointX:word
extrn CheckpointY:word
extrn CollectTotalDecor:word
extrn CollectTotalMain:word
extrn CurrentLevel:byte
extrn DisplayPage:word
extrn DrawPage:word
extrn dtbl_79EC:word
extrn dtbl_7D2F:word
extrn dtbl_7D95:word
extrn ExpertFlag:byte
extrn FineScroll:byte
extrn FrameDataOffsets:word
extrn FrameDataSegments:word
extrn FrameHeights:byte
extrn FrameHotX:byte
extrn FrameWidths:byte
extrn GridDirtyFlag:byte
extrn HScrollDir:byte
extrn LevelFlags:byte
extrn LevelPaletteTable:word
extrn LevelPropHeader:word
extrn LevelScriptTable:word
extrn LevelSegment:word
extrn LevelStartX:word
extrn LevelStartY:word
extrn MapRows:byte
extrn NextFreeSegment:word
extrn PlayerX:word
extrn PlayerY:word
extrn PrevCameraCol:word
extrn RenderSlots:word
extrn ScriptFrameCounter:word
extrn ScriptPointer:word
extrn ShakeRows:word
extrn SineTable:byte
extrn SnowFrameTick:byte
extrn SnowWind:word
extrn TileClassTable:byte
extrn VScrollActive:byte
extrn word_0039:word
extrn word_003D:word
extrn word_0100:word
extrn word_27F0:word
extrn word_27F2:word
extrn word_287A:word
extrn word_2A76:word
extrn word_2A7A:word
extrn word_4F0C:word
extrn word_4F0E:word
extrn word_4F20:word
extrn word_4F22:word
extrn word_4F25:word
extrn word_4F28:word
extrn word_4F2A:word
extrn word_4F2E:word
extrn word_4F30:word
extrn word_4F32:word
extrn word_4F34:word
extrn word_4F37:word
extrn word_4F3A:word
extrn word_4F3C:word
extrn word_4F40:word
extrn word_4F42:word
extrn word_4F44:word
extrn word_4F52:word
extrn word_4F54:word
extrn word_4F56:word
extrn word_5648:word
extrn word_564A:word
extrn word_564C:word
extrn word_565A:word
extrn word_565C:word
extrn word_565E:word
extrn word_566C:word
extrn word_566E:word
extrn word_5670:word
extrn word_567E:word
extrn word_5680:word
extrn word_5682:word
extrn word_5690:word
extrn word_5692:word
extrn word_5694:word
extrn word_6BAB:word
extrn word_6BB1:word
extrn word_6BBE:word
extrn word_6BCA:word
extrn word_6BE2:word
extrn word_6BEB:word
extrn word_6BF1:word
extrn word_6BFA:word
extrn word_6BFC:word
extrn word_6C06:word
extrn word_6C08:word
extrn word_6C0A:word
extrn word_6C0C:word
extrn word_6C0E:word
extrn word_6C10:word
extrn word_6C9E:word
extrn word_6CA0:word
extrn word_6CA2:word
extrn word_78C4:word
extrn word_79CE:word
extrn word_7B04:word
extrn word_7B1B:word
extrn word_7CDF:word
extrn word_7D9B:word
extrn word_7DA9:word
extrn word_91F7:word
extrn word_91F9:word
extrn word_91FC:word
extrn word_91FF:word
extrn word_9201:word
extrn word_A30E:word
extrn word_A310:word
extrn word_A326:word
extrn word_A32E:word
extrn word_A331:word
extrn word_A333:word
extrn word_A336:word
extrn word_A338:word
extrn word_A33A:word
extrn word_A33C:word
extrn word_A33E:word
extrn word_A341:word
extrn word_A3F7:word
extrn word_A3FB:word
extrn word_A3FD:word
extrn word_A3FF:word
extrn word_A401:word
extrn word_A403:word
extrn word_A405:word
extrn word_A407:word
extrn word_A408:word
extrn word_A409:word
extrn word_A40B:word
extrn word_A40D:word
extrn word_A40E:word
extrn word_A40F:word
extrn word_A411:word
extrn word_A413:word
extrn word_A414:word
extrn word_A415:word
extrn word_A417:word
extrn word_A419:word
extrn word_A41A:word
extrn word_A41B:word
extrn word_A41D:word
extrn word_A41F:word
extrn word_A420:word
extrn word_A421:word
extrn word_A423:word
extrn word_A425:word
extrn word_A4F7:word
extrn word_A517:word
extrn word_A519:word
DATA ends
; end module interface
CODE segment word public 'CODE'
assume cs:CODE, ds:DATA, es:nothing, ss:nothing
; module interface (tools/symtab.py)
extrn ActivateResourceSlot:near
extrn AnimateTiles:near
extrn BlitTileToRing:near
extrn ClearStaggeredBlocks:near
extrn CopyRingWindowToPage:near
extrn CpuClass:byte
extrn DrawForegroundOverSprites:near
extrn DrawSpriteObjects:near
extrn EnableVideoOutput:near
extrn EnterLevelView:near
extrn FlipDisplayPage:near
extrn LoadSqzFile:near
extrn NextWindRandom:near
extrn OptionBits:word
extrn OptionVByte:byte
extrn PageInResource:near
extrn PollInputState:near
extrn PresentFrame:near
extrn RedrawOverlayTiles:near
extrn RevealLevelColumns:near
extrn RunCallbackMapScreen:near
extrn RunGameOverScreen:near
extrn RunIrisClose:near
extrn ScrollViewDown:near
extrn ScrollViewLeft:near
extrn ScrollViewRight:near
extrn ScrollViewUp:near
extrn SetCpuWriteMode:near
extrn SetLatchCopyMode:near
extrn SetLevelPalette:near
extrn SetLevelVideoMode:near
extrn ShowIntroPicture:near
extrn StartSoundEffect:near
extrn StepRandomSeed:near
extrn TryShowCredits:near
extrn TryShowIntroKeyed:near
extrn UpdateWindAndFlakes:near
extrn WaitForButtonPressRelease:near
extrn WaitVerticalRetrace:near
; end module interface

sub_454E:
    mov word ptr word_6CA2, 0FFFFh
    mov byte ptr byte_6CA4, 0FFh
    mov byte ptr byte_6CA5, 0FFh
    mov byte ptr byte_6CA6, 0FFh
    push ds
    push es
    call SetCpuWriteMode
    mov ax, 0A000h
    mov es, ax
    push word ptr DisplayPage
    mov di, word ptr ds:DrawPage
    mov ds, word ptr word_003D
    mov si, 7B0h
    add di, 1B80h
    mov dx, 3C4h
    mov ax, 102h
loc_4586:
    out dx, ax
    mov cx, 1CCh
    rep movsw
    sub di, 398h
    shl ah, 1
    cmp ah, 10h
    jne loc_4586
    call SetLatchCopyMode
    mov si, di
    pop di
    add di, 1B80h
    push es
    pop ds
    mov cx, 398h
    rep movsb
    pop es
    pop ds
    ret
loc_45AB:
    pop es
    pop ds
    ret
cvar_45AE label byte
; DATA: counter byte
    db 00h

sub_45AF:
    push ds
    push es
    cmp byte ptr ds:byte_6BE4, 0
    jne loc_45AB
    mov ax, word ptr word_6C0E
    mov dx, word ptr word_6C10
    mov bx, 61A8h
    div bx
    sub ax, word ptr word_6C0C
    je loc_45E0
    add word ptr word_6C0C, ax
    jmp short loc_45D3
loc_45D0:
    call sub_65D6
loc_45D3:
    dec al
    jns loc_45D0
    mov si, 4F1Ch
    mov ax, 0E3h
    call sub_8875
loc_45E0:
    call SetCpuWriteMode
    mov byte ptr cs:cvar_45AE, 0
    mov al, byte ptr byte_27D8
    cmp byte ptr byte_6CA4, al
    je loc_4606
    mov byte ptr byte_6CA4, al
    cmp al, 9
    jbe loc_45FB
    mov al, 9
loc_45FB:
    mov di, word ptr ds:DrawPage
    add di, 1CEDh
    call sub_473D
loc_4606:
    xor al, al
    mov dx, word ptr word_6C0E
    cmp word ptr word_6CA0, dx
    je loc_4614
    or al, 1
loc_4614:
    mov cx, word ptr word_6C10
    cmp word ptr word_6CA2, cx
    je loc_4620
    or al, 1
loc_4620:
    or al, al
    je loc_464C
    mov word ptr word_6CA0, dx
    mov word ptr word_6CA2, cx
    call sub_4803
    mov di, 1CF1h
    add di, word ptr ds:DrawPage
    mov si, 6F52h
loc_4639:
    lodsb
    cmp al, 24h
    je loc_4647
    sub al, 30h
    call sub_473D
    inc di
    inc di
    jmp short loc_4639
loc_4647:
    xor al, al
    call sub_473D
loc_464C:
    mov al, byte ptr byte_27D6
    cmp byte ptr byte_6CA5, al
    je loc_4683
    mov byte ptr byte_6CA5, al
    mov ah, al
    mov dl, 3
    mov di, 1D01h
    add di, word ptr ds:DrawPage
    or ah, ah
    je loc_4674
    mov al, 0Ah
loc_4669:
    call sub_473D
    inc di
    inc di
    dec dl
    dec ah
    jne loc_4669
loc_4674:
    or dl, dl
    je loc_4683
    mov al, 0Bh
loc_467A:
    call sub_473D
    inc di
    inc di
    dec dl
    jne loc_467A
loc_4683:
    cmp byte ptr ds:byte_6C00, 0
    je loc_469C
    mov byte ptr byte_6CA6, 0FFh
    mov ah, 1Fh
    test byte ptr SnowFrameTick, 1
    jne loc_46AA
    xor ah, ah
    jmp short loc_46AA
loc_469C:
    mov ah, byte ptr byte_6CA7
    cmp byte ptr byte_6CA6, ah
    je loc_46C7
    mov byte ptr byte_6CA6, ah
loc_46AA:
    call sub_46EE
    mov al, 0Ch
    mov si, 6F86h
loc_46B2:
    mov di, word ptr [si]
    add di, word ptr ds:DrawPage
    inc si
    inc si
    shr ah, 1
    jae loc_46C1
    call sub_473D
loc_46C1:
    inc al
    cmp al, 10h
    jbe loc_46B2
loc_46C7:
    cmp byte ptr cs:cvar_45AE, 0
    je loc_46EB
    call SetLatchCopyMode
    mov si, 1B80h
    mov di, si
    add si, word ptr DrawPage
    add di, word ptr ds:DisplayPage
    mov ax, 0A000h
    mov es, ax
    mov ds, ax
    mov cx, 398h
    rep movsb
loc_46EB:
    jmp near ptr loc_45AB

sub_46EE:
    push ax
    push bx
    push cx
    push dx
    push di
    push si
    push ds
    push es
    push bp
    mov ax, 0A000h
    mov es, ax
    mov di, word ptr ds:DrawPage
    add di, 1BA1h
    mov si, 7D1h
    mov ds, word ptr word_003D
    mov dx, 3C4h
    mov ax, 102h
loc_4711:
    out dx, ax
    mov bx, 17h
loc_4715:
    mov cx, 3
    rep movsw
    add di, 22h
    add si, 22h
    dec bx
    jne loc_4715
    sub di, 398h
    shl ah, 1
    cmp ah, 10h
    jne loc_4711
    inc byte ptr cs:cvar_45AE
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

sub_473D:
    push ax
    push cx
    push dx
    push si
    push ds
    push es
    mov dx, 0A000h
    mov es, dx
    mov ds, word ptr word_003D
    mov ah, 60h
    mul ah
    add ax, 0E60h
    add ax, 7B0h
    mov si, ax
    mov dx, 3C4h
    mov ax, 102h
loc_475E:
    out dx, ax
    mov cx, 0Ch
loc_4762:
    movsb
    movsb
    add di, 26h
    loop loc_4762
    sub di, 1E0h
    shl ah, 1
    cmp ah, 10h
    jne loc_475E
    inc byte ptr cs:cvar_45AE
    pop es
    pop ds
    pop si
    pop dx
    pop cx
    pop ax
    ret

sub_4780:
    push ax
    push cx
    push dx
    push si
    push ds
    push es
    mov ah, 58h
    mul ah
    add ax, 1C70h
    mov si, ax
    mov ax, 0A000h
    mov es, ax
    mov ds, word ptr word_003D
    mov dx, 3C4h
    mov ax, 102h
loc_479E:
    out dx, ax
    mov cx, 0Bh
loc_47A2:
    movsb
    movsb
    add di, 26h
    loop loc_47A2
    sub di, 1B8h
    shl ah, 1
    cmp ah, 10h
    jne loc_479E
    inc byte ptr cs:cvar_45AE
    pop es
    pop ds
    pop si
    pop dx
    pop cx
    pop ax
    ret

sub_47C0:
    push ax
    push cx
    push dx
    push si
    push ds
    push es
    mov dx, 0A000h
    mov es, dx
    cbw
    add ax, 0F1h
    shl ax, 1
    mov si, ax
    mov ax, word ptr [si+FrameDataOffsets]
    mov ds, word ptr [si+FrameDataSegments]
    add ax, 16h
    mov si, ax
    mov dx, 3C4h
    mov ax, 102h
loc_47E6:
    out dx, ax
    mov cx, 0Bh
loc_47EA:
    movsb
    movsb
    add di, 26h
    loop loc_47EA
    sub di, 1B8h
    shl ah, 1
    cmp ah, 10h
    jne loc_47E6
    pop es
    pop ds
    pop si
    pop dx
    pop cx
    pop ax
    ret

sub_4803:
    push dx
    push ax
    push bx
    push cx
    push bp
    push si
    mov ax, seg DATA
    mov ds, ax
    mov di, 6F82h
    mov si, 6F4Eh
    mov bp, 0Ah
loc_4817:
    mov bl, 30h
loc_4819:
    or cx, cx
    je loc_4831
    cmp cx, word ptr [di]
    ja loc_4828
    jne loc_4831
    cmp dx, word ptr [di+2]
    jb loc_4831
loc_4828:
    sub dx, word ptr [di+2]
    sbb cx, word ptr [di]
    inc bl
    jmp short loc_4819
loc_4831:
    cmp word ptr [di], 0
    jne loc_483B
    cmp dx, word ptr [di+2]
    jae loc_4828
loc_483B:
    mov byte ptr [si], bl
    inc si
    sub di, 4
    dec bp
    jne loc_4817
    mov byte ptr [si], 24h
    pop si
    pop bp
    pop cx
    pop bx
    pop ax
    pop dx
    ret

sub_484E:
    cmp byte ptr ds:byte_6BC5, 0
    jne loc_4858
    jmp near ptr loc_4906
loc_4858:
    cmp byte ptr ds:byte_6BC7, 0
    je loc_488C
    and word ptr word_4F20, 8000h
    mov dx, 30h
    mov ax, word ptr word_4F22
    or ax, ax
    jns loc_4871
    neg ax
loc_4871:
    cmp ax, 40h
    ja loc_487D
    inc dx
    cmp ax, 20h
    ja loc_487D
    inc dx
loc_487D:
    mov al, byte ptr byte_7B1A
    cmp al, 3
    jae loc_4888
    mov dl, 33h
    sub dl, al
loc_4888:
    or word ptr word_4F20, dx
loc_488C:
    mov si, 7B29h
    mov ax, word ptr word_4F20
    mov dl, ah
    and ah, 1Fh
    mov bl, byte ptr byte_27EA
    or bl, byte ptr byte_27EB
    jne loc_48B2
    mov bl, 1
    cmp byte ptr ds:byte_7B1A, 3
    je loc_48B2
    jb loc_48AE
    neg bl
loc_48AE:
    add byte ptr byte_7B1A, bl
loc_48B2:
    mov cx, word ptr [si]
    jcxz loc_4906
    cmp ax, cx
    jne loc_4901
    mov al, byte ptr [si+4]
    cbw
    mov cx, ax
    mov ax, word ptr [si+2]
    cmp ax, 79h
    jb loc_48D4
    mov bl, byte ptr byte_7B1A
    xor bh, bh
    shl bx, 1
    mov ax, word ptr [bx+word_7B1B]
loc_48D4:
    and dl, 80h
    je loc_48DD
    or ah, dl
    neg cx
loc_48DD:
    cmp byte ptr ds:byte_6BC8, 18h
    jbe loc_48E9
    cmp al, 79h
    jae loc_48E9
    inc ax
loc_48E9:
    mov word ptr word_4F0E, ax
    mov ax, word ptr PlayerX
    add ax, cx
    mov word ptr RenderSlots, ax
    mov al, byte ptr [si+5]
    cbw
    add ax, word ptr PlayerY
    mov word ptr word_4F0C, ax
    jmp short loc_4906
loc_4901:
    add si, 6
    jmp short loc_48B2
loc_4906:
    ret

sub_4907:
    mov si, 9107h
    mov di, 5570h
    mov bx, 7
    mov cx, 10h
    mov byte ptr byte_6BFE, 0
loc_4918:
    cmp word ptr [si+4], -1
    jne loc_4921
    jmp near ptr loc_4AF0
loc_4921:
    mov al, byte ptr [si+6]
    and al, 0Fh
    cmp al, 8
    je loc_492D
    jmp near ptr loc_49F3
loc_492D:
    mov word ptr word_6BFA, 0
    mov ax, word ptr [si+0Bh]
    sub word ptr [si+2], ax
    cmp byte ptr [si+0Ah], 0
    jne loc_496B
    mov ax, word ptr [si+0Bh]
    sub ax, 8
    jge loc_494D
    xor ax, ax
    mov byte ptr [si+7], 0
loc_494D:
    mov word ptr [si+0Bh], ax
    test byte ptr [si+6], 40h
    je loc_4969
    cmp byte ptr [si+7], 0
    jne loc_4961
    dec byte ptr [si+0Dh]
    jne loc_4969
loc_4961:
    mov byte ptr [si+0Ah], 1
    mov byte ptr [si+7], 0
loc_4969:
    jmp short loc_49EA
loc_496B:
    cmp byte ptr [si+0Ah], 1
    jne loc_49CF
    mov al, byte ptr [si+7]
    xor ah, ah
    cmp ax, 0C0h
    jge loc_497F
    add byte ptr [si+7], 8
loc_497F:
    push bx
    push cx
    mov cl, 4
    sar ax, cl
    mov word ptr word_6BFC, ax
    add word ptr [si+0Bh], ax
    mov es, word ptr LevelSegment
    mov bx, word ptr [si]
    sar bx, cl
    mov dx, word ptr [si+2]
    add dx, word ptr [si+0Bh]
    sar dx, cl
    mov al, byte ptr MapRows
    xor ah, ah
    dec ax
    sub ax, dx
    jae loc_49AE
    neg ax
    cmp ax, 3
    jb loc_49CB
    jmp short loc_49C3
loc_49AE:
    mov bh, dl
    mov bl, byte ptr es:[bx]
    xor bh, bh
    cmp byte ptr [bx+byte_7F5E], 0
    je loc_49CB
    cmp byte ptr [bx+byte_7F5E], 6
    je loc_49CB
loc_49C3:
    mov byte ptr [si+0Ah], 2
    mov byte ptr [si+0Dh], 16h
loc_49CB:
    pop cx
    pop bx
    jmp short loc_49EA
loc_49CF:
    cmp byte ptr [si+0Ah], 2
    jne loc_49EA
    test byte ptr [si+6], 40h
    jne loc_49EA
    dec byte ptr [si+0Dh]
    jne loc_49EA
    mov byte ptr [si+0Ah], 0
    mov al, byte ptr [si+9]
    mov byte ptr [si+0Dh], al
loc_49EA:
    mov ax, word ptr [si+0Bh]
    add word ptr [si+2], ax
    jmp near ptr loc_4A8B
loc_49F3:
    xor ax, ax
    mov word ptr word_6BFA, ax
    mov word ptr word_6BFC, ax
    cmp byte ptr [si+0Eh], 0
    jne loc_4A0D
    cmp byte ptr [si+7], 0
    jl loc_4A0D
    test byte ptr [si+6], 0C0h
    je loc_4A8B
loc_4A0D:
    mov al, byte ptr [si+0Eh]
    cmp byte ptr [si+7], al
    je loc_4A20
    mov dl, 1
    jge loc_4A1B
    neg dl
loc_4A1B:
    add al, dl
    mov byte ptr [si+0Eh], al
loc_4A20:
    mov ah, al
    mov dl, byte ptr [si+6]
    and dl, 7
    jne loc_4A2E
    xor al, al
    neg ah
loc_4A2E:
    cmp dl, 1
    jne loc_4A35
    neg ah
loc_4A35:
    cmp dl, 2
    jne loc_4A3C
    xor ah, ah
loc_4A3C:
    cmp dl, 3
    je loc_4A61
    cmp dl, 4
    jne loc_4A48
    xor al, al
loc_4A48:
    cmp dl, 5
    jne loc_4A4F
    neg al
loc_4A4F:
    cmp dl, 6
    jne loc_4A58
    neg al
    xor ah, ah
loc_4A58:
    cmp dl, 7
    jne loc_4A61
    neg al
    neg ah
loc_4A61:
    mov dl, ah
    cbw
    mov word ptr word_6BFA, ax
    add word ptr [si], ax
    mov al, dl
    cbw
    mov word ptr word_6BFC, ax
    add word ptr [si+2], ax
    mov al, byte ptr [si+0Eh]
    cmp byte ptr [si+7], al
    jne loc_4A8B
    mov ax, word ptr [si+0Ch]
    inc ax
    cmp word ptr [si+0Ah], ax
    jne loc_4A88
    neg word ptr [si+7]
    xor ax, ax
loc_4A88:
    mov word ptr [si+0Ch], ax
loc_4A8B:
    mov dx, word ptr CameraCol
    mov ax, word ptr [si]
    shl dx, 1
    shl dx, 1
    shl dx, 1
    shl dx, 1
    sub dx, ax
    neg dx
    cmp dx, 160h
    jge loc_4AF0
    cmp dx, -20h
    jle loc_4AF0
    mov word ptr [di], ax
    mov ax, word ptr [si+2]
    mov dx, word ptr CameraRow
    shl dx, 1
    shl dx, 1
    shl dx, 1
    shl dx, 1
    sub dx, ax
    neg dx
    cmp dx, 0D0h
    jge loc_4AF0
    cmp dx, -20h
    jle loc_4AF0
    mov word ptr [di+2], ax
    mov ax, word ptr [si+4]
    mov word ptr [di+4], ax
    mov word ptr [di+9], si
    cmp word ptr ds:word_4F2A, -10h
    jle loc_4AE4
    or byte ptr [si+6], 40h
    call sub_4B05
    jb loc_4AEC
loc_4AE4:
    and byte ptr [si+6], 0BFh
    sub word ptr [di+2], 2
loc_4AEC:
    add di, 12h
    dec bx
loc_4AF0:
    add si, 0Fh
    dec cx
    je loc_4AF9
    jmp near ptr loc_4918
loc_4AF9:
    mov word ptr [di+4], 0FFFFh
    add di, 12h
    dec bx
    jne loc_4AF9
    ret

sub_4B05:
    push ax
    push bx
    push si
    cmp byte ptr ds:byte_6BFE, 0
    jne loc_4B89
    mov ax, word ptr PlayerY
    cmp word ptr [di+2], ax
    jle loc_4B89
    mov si, 4F1Ch
    push word ptr PlayerY
    mov ax, word ptr word_6BFC
    or ax, ax
    jl loc_4B29
    add word ptr PlayerY, ax
loc_4B29:
    push word ptr [si+4]
    mov word ptr [si+4], 7
    call sub_8D7B
    pop word ptr [si+4]
    pop word ptr PlayerY
    jae loc_4B89
    mov byte ptr byte_6BFE, 1
    mov byte ptr byte_4F24, 0
    mov ax, word ptr word_6BFA
    add word ptr PlayerX, ax
    call sub_64DF
    mov bx, word ptr [di+4]
    and bh, 1Fh
    shl bx, 1
    mov al, byte ptr [bx+FrameHeights]
    xor ah, ah
    neg ax
    add ax, word ptr [di+2]
    cmp word ptr PlayerY, ax
    jle loc_4B76
    inc ax
    mov word ptr PlayerY, ax
    mov word ptr word_4F2A, 1
    jmp short loc_4B84
loc_4B76:
    mov ax, word ptr word_6BFC
    shl ax, 1
    shl ax, 1
    shl ax, 1
    shl ax, 1
    mov word ptr word_4F2A, ax
loc_4B84:
    stc
    pop si
    pop bx
    pop ax
    ret
loc_4B89:
    clc
    pop si
    pop bx
    pop ax
    ret

sub_4B8E:
    push es
    call SetCpuWriteMode
    mov dx, 3C4h
    mov ax, 0F02h
    out dx, ax
    mov dl, 0CEh
    mov ax, 1003h
    out dx, ax
    mov ax, 1
    out dx, ax
    mov si, 7DE6h
    mov bp, 14h
loc_4BA9:
    cmp word ptr [si], -1
    je loc_4C21
    mov cl, 4
    mov bx, 6F90h
    mov al, byte ptr [si+4]
    xlat
    sar al, 1
    sar al, 1
    imul byte ptr [si+5]
    sar ax, cl
    add word ptr [si], ax
    mov bx, 7090h
    mov al, byte ptr [si+4]
    xlat
    sar al, 1
    sar al, 1
    imul byte ptr [si+5]
    sar ax, cl
    add word ptr [si+2], ax
    mov ax, 0A000h
    mov es, ax
    mov al, byte ptr FineScroll
    cbw
    neg ax
    add ax, word ptr [si+2]
    mov dx, word ptr CameraRow
    shl dx, cl
    sub ax, dx
    cmp ax, 0B0h
    jae loc_4C1D
    mov ah, 28h
    mul ah
    mov di, ax
    mov ax, word ptr [si]
    mov dx, word ptr CameraCol
    shl dx, cl
    sub ax, dx
    cmp ax, 140h
    jae loc_4C1D
    mov cx, ax
    and cl, 7
    shr ax, 1
    shr ax, 1
    shr ax, 1
    add di, ax
    add di, word ptr ds:DrawPage
    mov al, 80h
    shr al, cl
    xchg byte ptr es:[di], al
loc_4C1D:
    mov word ptr [si], 0FFFFh
loc_4C21:
    add si, 6
    dec bp
    jne loc_4BA9
    mov dx, 3CEh
    mov ax, 1
    out dx, ax
    pop es
    ret

sub_4C30:
    cmp byte ptr ds:byte_6BEA, 1
    jb loc_4C68
    je loc_4C63
    xor ax, ax
    test byte ptr SnowFrameTick, 1
    je loc_4C5E
    cmp byte ptr ds:byte_4F27, 5
    je loc_4C55
    cmp byte ptr ds:byte_4F27, 20h
    je loc_4C55
    sub word ptr PlayerY, 3
loc_4C55:
    inc byte ptr byte_6BEA
    mov al, byte ptr byte_6BEA
    xor ah, ah
loc_4C5E:
    mov word ptr ShakeRows, ax
    jmp short loc_4C68
loc_4C63:
    xor ax, ax
    mov word ptr ShakeRows, ax
loc_4C68:
    ret

sub_4C69:
    xor ax, ax
    cmp byte ptr ds:byte_6BE6, 1
    je loc_4CBA
    jb loc_4C99
    mov bx, 2CF6h
    mov al, byte ptr CurrentLevel
    cmp al, 0Ah
    jb loc_4C8F
loc_4C7E:
    cmp byte ptr [bx], al
    je loc_4C85
    inc bx
    jmp short loc_4C7E
loc_4C85:
    sub bx, 2CF6h
    mov byte ptr CurrentLevel, bl
    jmp short loc_4CBA
loc_4C8F:
    xlat
    mov byte ptr CurrentLevel, al
    call ClearStaggeredBlocks
    jmp near ptr loc_4F65
loc_4C99:
    inc ax
    cmp byte ptr ds:byte_6BE4, 1
    jne loc_4CA4
    jmp near ptr loc_4F6C
loc_4CA4:
    cmp byte ptr ds:byte_6BE5, 1
    jne loc_4CAE
    jmp near ptr loc_5063
loc_4CAE:
    cmp byte ptr ds:byte_6BE5, 0FFh
    jne loc_4CB8
    jmp near ptr loc_5034
loc_4CB8:
    clc
    ret
loc_4CBA:
    cmp byte ptr ds:CurrentLevel, 0Ah
    jb loc_4CCB
    call ClearStaggeredBlocks
    inc byte ptr CurrentLevel
    jmp near ptr loc_4F65
loc_4CCB:
    cmp byte ptr ds:byte_6C04, 0
    je loc_4CE1
    mov byte ptr byte_6C01, 0
    mov byte ptr byte_6C02, 1
    mov byte ptr byte_6C03, 0
loc_4CE1:
    call RunIrisClose
    mov ax, 0Fh
    call ActivateResourceSlot
    mov ax, word ptr CameraRow
    shl ax, 1
    shl ax, 1
    shl ax, 1
    shl ax, 1
    sub word ptr PlayerY, ax
    mov ax, word ptr CameraCol
    shl ax, 1
    shl ax, 1
    shl ax, 1
    shl ax, 1
    sub word ptr PlayerX, ax
    mov word ptr CameraCol, 0
    mov word ptr CameraRow, 0
    mov word ptr word_4F25, 0
    mov word ptr word_4F22, 0
    call SetCpuWriteMode
    call sub_51A3
    mov byte ptr byte_6BC5, 0
    mov word ptr word_4F0E, 0FFFFh
    mov word ptr word_4F28, 7BA7h
loc_4D37:
    mov bx, word ptr word_4F28
    call sub_638B
    xor bp, bp
    mov ax, word ptr PlayerX
    mov dx, 3Ch
    mov bx, 2
    sub ax, dx
    jns loc_4D4F
    neg bx
loc_4D4F:
    cmp ax, 2
    jb loc_4D5B
    mov dx, word ptr PlayerX
    sub dx, bx
    inc bp
loc_4D5B:
    mov word ptr PlayerX, dx
    mov ax, word ptr PlayerY
    mov dx, 0AFh
    mov bx, 2
    sub ax, dx
    jns loc_4D6E
    neg bx
loc_4D6E:
    cmp ax, 2
    jb loc_4D7A
    mov dx, word ptr PlayerY
    sub dx, bx
    inc bp
loc_4D7A:
    mov word ptr PlayerY, dx
    call sub_520C
    call sub_51A3
    call DrawSpriteObjects
    call PresentFrame
    or bp, bp
    jne loc_4D37
    mov ax, 168h
    mov word ptr word_4F2E, ax
    mov word ptr word_4F30, 0AFh
    mov word ptr word_4F32, 64h
    mov word ptr word_4F40, ax
    mov word ptr word_4F42, 94h
    mov word ptr word_4F44, 62h
    mov word ptr word_4F52, ax
    mov word ptr word_4F54, 9Bh
    mov word ptr word_4F56, 68h
loc_4DBE:
    mov bx, word ptr word_4F28
    call sub_638B
    call sub_520C
    call sub_51F0
    call sub_51A3
    call DrawSpriteObjects
    call PresentFrame
    sub word ptr word_4F2E, 3
    sub word ptr word_4F40, 3
    sub word ptr word_4F52, 3
    cmp word ptr ds:word_4F2E, 9Bh
    ja loc_4DBE
    cmp word ptr ds:word_6C9E, 0
    jne loc_4DF5
    jmp near ptr loc_4F0E
loc_4DF5:
    mov word ptr word_4F28, 7C6Bh
    mov word ptr word_4F22, 40h
    mov byte ptr byte_4F24, 2
loc_4E06:
    mov bx, word ptr word_4F28
    call sub_638B
    call sub_520C
    call sub_51F0
    call sub_51A3
    call DrawSpriteObjects
    call PresentFrame
    call sub_6333
    mov ax, word ptr word_4F22
    sar ax, 1
    sar ax, 1
    sar ax, 1
    sar ax, 1
    add word ptr PlayerX, ax
    or ax, ax
    jne loc_4E06
    mov word ptr word_4F28, 7CAFh
    xor di, di
loc_4E3A:
    mov bx, word ptr word_4F28
    call sub_638B
    call sub_520C
    call sub_51F0
    call sub_51A3
    call DrawSpriteObjects
    call FlipDisplayPage
    call WaitVerticalRetrace
    mov si, 52E8h
    mov cx, 14h
    xor bp, bp
loc_4E5B:
    cmp word ptr [si+4], -1
    je loc_4EAB
    mov ax, word ptr [si+0Eh]
    cmp ax, 80h
    jae loc_4E6F
    add ax, 8
    mov word ptr [si+0Eh], ax
loc_4E6F:
    sar ax, 1
    sar ax, 1
    sar ax, 1
    sar ax, 1
    add word ptr [si+2], ax
    inc bp
    cmp word ptr [si+2], 91h
    jb loc_4EAB
    mov bx, word ptr [si+4]
    and bh, 1Fh
    sub bx, 6Eh
    mov bl, byte ptr [bx-5C8Bh]
    xor bh, bh
    shl bx, 1
    mov ax, word ptr [bx-5CADh]
    add word ptr word_6C0E, ax
    adc word ptr word_6C10, 0
    mov word ptr [si+4], 0FFFFh
    mov dx, 8
    call StartSoundEffect
loc_4EAB:
    add si, 12h
    loop loc_4E5B
    test word ptr SnowFrameTick, 7
    je loc_4EBA
    jmp short loc_4E3A
loc_4EBA:
    xor al, al
    jmp short loc_4ECC
loc_4EBE:
    cmp byte ptr [di+byte_6C12], 0
    jne loc_4ED5
    inc al
    cmp al, 71h
    jae loc_4F07
    inc di
loc_4ECC:
    cmp di, 71h
    jb loc_4EBE
    xor di, di
    jmp short loc_4EBE
loc_4ED5:
    mov ax, di
    add ax, 6Eh
    mov si, 52E8h
    mov cx, 14h
loc_4EE0:
    cmp word ptr [si+4], -1
    je loc_4EEE
    add si, 12h
    loop loc_4EE0
    jmp near ptr loc_4E3A
loc_4EEE:
    mov word ptr [si+4], ax
    mov word ptr [si], 9Bh
    mov word ptr [si+2], 0
    mov word ptr [si+0Eh], 0
    dec byte ptr [di+byte_6C12]
    inc di
    jmp near ptr loc_4E3A
loc_4F07:
    or bp, bp
    je loc_4F0E
    jmp near ptr loc_4E3A
loc_4F0E:
    call WaitVerticalRetrace
    call WaitVerticalRetrace
    mov word ptr word_4F28, 7BA7h
loc_4F1A:
    mov bx, word ptr word_4F28
    call sub_638B
    call sub_520C
    call sub_51F0
    call sub_51A3
    call DrawSpriteObjects
    call PresentFrame
    sub word ptr word_4F2E, 2
    sub word ptr word_4F40, 2
    sub word ptr word_4F52, 2
    jns loc_4F46
    mov ax, 3
    jmp short loc_4F49
loc_4F46:
    mov ax, 2
loc_4F49:
    add word ptr PlayerX, ax
    cmp word ptr ds:word_4F2E, -34h
    jg loc_4F1A
    cmp byte ptr ds:byte_2879, 0
    je loc_4F5E
    jmp near ptr loc_506C
loc_4F5E:
    inc byte ptr CurrentLevel
    call sub_51DF
loc_4F65:
    call sub_5237
    xor ax, ax
    stc
    ret
loc_4F6C:
    call sub_509D
    cmp byte ptr ds:byte_2879, 0
    je loc_4F79
    jmp near ptr loc_506C
loc_4F79:
    cmp byte ptr ds:byte_6BE5, 0
    je loc_4F83
    jmp near ptr loc_506C
loc_4F83:
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
    mov es, ax
    mov si, 8F1Dh
    mov di, 6C12h
    mov cx, 46h
loc_4F9A:
    mov ax, word ptr [si+4]
    stosw
    add si, 7
    loop loc_4F9A
    mov ax, 55AAh
    stosw
    mov si, 8C8Dh
    mov di, 0A2A8h
    mov cx, 50h
loc_4FB0:
    xor al, al
    cmp word ptr [si+3], -1
    je loc_4FB9
    inc ax
loc_4FB9:
    stosb
    add si, 5
    loop loc_4FB0
    call sub_5237
    mov ax, word ptr CheckpointX
    mov word ptr PlayerX, ax
    mov ax, word ptr CheckpointY
    mov word ptr PlayerY, ax
    call EnterLevelView
    mov di, 8F1Dh
    mov si, 6C12h
    mov cx, 46h
loc_4FDA:
    lodsw
    mov dx, word ptr [di+4]
    sub dx, 35h
    cmp dl, 0Dh
    je loc_5007
    cmp dl, 2Ch
    je loc_5007
    cmp dl, 41h
    je loc_5007
    cmp dl, 0A9h
    je loc_5007
    cmp dl, 0AAh
    je loc_5007
    cmp dl, 0B6h
    je loc_5007
    cmp dl, 0E0h
    je loc_5007
    mov word ptr [di+4], ax
loc_5007:
    add di, 7
    loop loc_4FDA
    mov di, 8C8Dh
    mov si, 0A2A8h
    mov cx, 50h
loc_5015:
    lodsb
    or al, al
    jne loc_501F
    mov word ptr [di+3], 0FFFFh
loc_501F:
    add di, 5
    loop loc_5015
    call sub_51DF
    pop bp
    pop es
    pop ds
    pop si
    pop di
    pop dx
    pop cx
    pop bx
    pop ax
    xor ax, ax
    clc
    ret
loc_5034:
    call SetLevelVideoMode
    mov ax, word ptr word_0039
    mov word ptr NextFreeSegment, ax
    mov dx, 6BA0h
    push ds
    push es
    call LoadSqzFile
    pop es
    pop ds
    mov word ptr LevelSegment, ax
    mov ds, word ptr LevelSegment
    xor si, si
    mov cl, 10h
    call sub_919F
    call WaitForButtonPressRelease
    call sub_9286
    call ShowIntroPicture
    mov ax, 1
    stc
    ret
loc_5063:
    mov ax, word ptr LevelSegment
    mov word ptr NextFreeSegment, ax
    call sub_509D
loc_506C:
    call TryShowIntroKeyed
    mov ax, 11h
    call ActivateResourceSlot
    call RunGameOverScreen
    mov word ptr word_4F20, 0Dh
    mov byte ptr CurrentLevel, 0
    mov word ptr word_6C0E, 0
    mov word ptr word_6C10, 0
    mov word ptr word_6C0C, 0
    call sub_51DF
    mov ax, 1
    stc
    ret

sub_509D:
    push ax
    push bx
    push cx
    push dx
    push di
    push si
    push ds
    push es
    push bp
    mov dx, 7
    call StartSoundEffect
    mov byte ptr byte_4F2D, 0
    mov word ptr word_4F20, 21h
    mov word ptr word_4F2A, 0Fh
    mov ax, word ptr PlayerX
    mov dx, word ptr CameraCol
    add dx, 0Ah
    shl dx, 1
    shl dx, 1
    shl dx, 1
    shl dx, 1
    cmp ax, dx
    mov ax, 5
    jb loc_50D8
    neg ax
loc_50D8:
    mov word ptr word_4F22, ax
    mov cx, 3Ch
loc_50DE:
    push cx
    mov word ptr word_4F0E, 0FFFFh
    call sub_581E
    call sub_6822
    call sub_6210
    call sub_60FE
    call sub_60DF
    call AnimateTiles
    call RedrawOverlayTiles
    call CopyRingWindowToPage
    call sub_4B8E
    call DrawSpriteObjects
    call DrawForegroundOverSprites
    call sub_54AB
    call UpdateWindAndFlakes
    call PresentFrame
    mov ax, word ptr word_4F22
    add word ptr PlayerX, ax
    mov ax, word ptr word_4F2A
    dec ax
    cmp ax, 0FFF0h
    jge loc_5122
    mov ax, 0FFF0h
loc_5122:
    mov word ptr word_4F2A, ax
    sub word ptr PlayerY, ax
    pop cx
    loop loc_50DE
    call ClearStaggeredBlocks
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

sub_5139:
    call SetCpuWriteMode
    mov di, 410h
    add di, word ptr ds:DrawPage
    mov si, 7D7Fh
    call sub_5190
    mov ax, 64h
    mov cx, word ptr CollectTotalMain
    add cx, word ptr CollectTotalDecor
    jcxz loc_5165
    mov ax, word ptr word_2A76
    add ax, word ptr word_2A7A
    mov ah, 64h
    mul ah
    xor dx, dx
    div cx
loc_5165:
    mov dx, ax
    xor cx, cx
    call sub_4803
    mov si, 6F55h
    mov di, 430h
    add di, word ptr ds:DrawPage
    cmp byte ptr [si], 30h
    jne loc_517C
    inc si
loc_517C:
    lodsb
    cmp al, 24h
    je loc_518A
    sub al, 30h
    call sub_4780
    inc di
    inc di
    jmp short loc_517C
loc_518A:
    mov al, 1Ah
    call sub_47C0
    ret

sub_5190:
    lodsb
    cmp al, 24h
    je loc_51A2
    cmp al, 20h
    je loc_519E
    sub al, 41h
    call sub_47C0
loc_519E:
    inc di
    inc di
    jmp short sub_5190
loc_51A2:
    ret

sub_51A3:
    call SetCpuWriteMode
    mov di, 230h
    add di, word ptr ds:DrawPage
    mov si, 7D8Fh
    call sub_5190
    mov dx, word ptr word_6C0E
    mov cx, word ptr word_6C10
    call sub_4803
    mov di, 23Ch
    add di, word ptr ds:DrawPage
    mov si, 6F52h
loc_51C8:
    lodsb
    cmp al, 24h
    je loc_51D6
    sub al, 30h
    call sub_4780
    inc di
    inc di
    jmp short loc_51C8
loc_51D6:
    xor al, al
    call sub_4780
    call sub_5139
    ret

sub_51DF:
    mov di, 6C12h
    mov cx, 71h
    xor al, al
    rep stosb
    mov word ptr word_6C9E, 0
    ret

sub_51F0:
    test byte ptr SnowFrameTick, 3
    jne loc_520B
    push ax
    mov ax, word ptr word_4F56
    and ah, 1Fh
    inc ax
    cmp ax, 6Eh
    jb loc_5207
    mov ax, 68h
loc_5207:
    mov word ptr word_4F56, ax
    pop ax
loc_520B:
    ret

sub_520C:
    push ax
    push cx
    push dx
    push di
    push es
    mov ax, 0A000h
    mov es, ax
    mov di, word ptr ds:DrawPage
    mov dx, 3C4h
    mov ax, 0F02h
    out dx, ax
    mov dl, 0CEh
    mov ax, 5
    out dx, ax
    mov al, 3
    out dx, ax
    xor ax, ax
    mov cx, 0FA0h
    rep stosw
    pop es
    pop di
    pop dx
    pop cx
    pop ax
    ret

sub_5237:
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
    mov di, 6BC4h
    mov cx, 48h
    mov al, 0
    rep stosb
    mov si, 9203h
    mov di, 815Eh
    mov cx, 10A5h
    rep movsb
    mov byte ptr byte_4F2C, al
    mov word ptr PrevCameraCol, 55AAh
    mov word ptr word_6BBE, 4F76h
    mov ax, 0FFFFh
    call sub_55FC
    call SetLevelPalette
    mov di, 6CA9h
    mov cx, 100h
loc_527A:
    call NextWindRandom
    mov dh, al
    call NextWindRandom
    mov ah, al
    stosw
    loop loc_527A
    mov di, 6EA9h
    mov ax, 55AAh
    mov cx, 50h
    rep stosw
    call sub_52D2
    mov cx, 78h
    mov di, 7DE6h
    mov ax, 0FFFFh
    rep stosb
    mov cx, 37h
    mov di, 7DAFh
    rep stosb
    mov byte ptr byte_27D6, 3
    mov word ptr word_A517, 0FFFFh
    mov bl, byte ptr CurrentLevel
    xor bh, bh
    shl bx, 1
    mov ax, word ptr [bx+LevelScriptTable]
    mov word ptr ScriptPointer, ax
    mov word ptr ScriptFrameCounter, 0
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

sub_52D2:
    push es
    push ds
    mov es, word ptr LevelSegment
    mov ds, word ptr NextFreeSegment
    xor si, si
loc_52DE:
    mov di, word ptr [si]
    cmp di, -1
    je loc_52FB
    mov ax, word ptr [si+2]
    add si, 4
loc_52EB:
    mov cl, ah
    push di
    rep movsb
    pop di
    add di, 100h
    dec al
    jne loc_52EB
    jmp short loc_52DE
loc_52FB:
    pop ds
    pop es
    ret

sub_52FE:
    mov ax, seg DATA
    mov ds, ax
    mov es, ax
    cmp byte ptr ds:byte_6BE1, 0
    je loc_5325
    cmp byte ptr ds:byte_6BC5, 0
    jne loc_5325
    call sub_549A
    mov si, 8367h
    mov cx, 14h
loc_531C:
    cmp word ptr [si], dx
    je loc_5326
    add si, 7
    loop loc_531C
loc_5325:
    ret
loc_5326:
    mov word ptr word_4F22, 0
    mov word ptr word_4F2A, 0
    call ClearStaggeredBlocks
    mov byte ptr FineScroll, 0
    push word ptr CameraLeftLimit
    push si
    mov word ptr CameraLeftLimit, 0ECh
    mov ax, word ptr [si+4]
    mov cl, 4
    mov dl, al
    xor dh, dh
    shl dx, cl
    mov word ptr PlayerX, dx
    mov dl, ah
    xor dh, dh
    shl dx, cl
    mov word ptr PlayerY, dx
    mov ax, word ptr [si+2]
loc_5361:
    cmp byte ptr CameraRow, ah
    je loc_5377
    jb loc_5370
    mov dl, 10h
    call ScrollViewUp
    jmp short loc_5361
loc_5370:
    mov dl, 10h
    call ScrollViewDown
    jmp short loc_5361
loc_5377:
    cmp byte ptr CameraCol, al
    je loc_5389
    jb loc_5384
    call ScrollViewLeft
    jmp short loc_5377
loc_5384:
    call ScrollViewRight
    jmp short loc_5377
loc_5389:
    pop si
    pop word ptr CameraLeftLimit
    mov al, byte ptr [si+6]
    mov byte ptr byte_6BD9, al
    mov byte ptr byte_6BE1, 0
    cmp byte ptr ds:CurrentLevel, 5
    jne loc_53D7
    and byte ptr LevelFlags, 1
    mov byte ptr byte_A324, 0
    mov byte ptr byte_A325, 5
    mov byte ptr word_A326, 0
    mov byte ptr byte_A328, 0
    mov byte ptr byte_A32A, 1
    mov byte ptr byte_A329, 0
    mov byte ptr byte_A32B, 6Eh
    mov byte ptr byte_A32C, 8
    mov di, 5570h
    mov cx, 69h
    mov al, 0FFh
    rep stosb
loc_53D7:
    call RedrawOverlayTiles
    call CopyRingWindowToPage
    call sub_4907
    call sub_8922
    call sub_6822
    call DrawSpriteObjects
    call DrawForegroundOverSprites
    call sub_54AB
    call UpdateWindAndFlakes
    call RevealLevelColumns
    ret

sub_53F6:
    call sub_549A
    mov si, 83F3h
    mov cx, 0Fh
loc_53FF:
    mov ax, word ptr [si+4]
    cmp ax, 0FFFFh
    je loc_5415
    cmp ax, 0FFFEh
    je loc_5427
    sub ax, dx
    neg ax
    cmp ax, 8
    jbe loc_541B
loc_5415:
    add si, 0Ah
    loop loc_53FF
    ret
loc_541B:
    mov word ptr [si+4], 0FFFEh
    mov byte ptr ds:[6BEAh], 7
    jmp short loc_5415
loc_5427:
    push cx
    push dx
    mov byte ptr ds:[6BEAh], 7
    test byte ptr ds:[6BD5h], 3
    jne loc_5495
    mov es, word ptr ds:[2DDAh]
    mov dl, byte ptr [si+2]
    mov dh, byte ptr [si+3]
    mov di, word ptr [si]
    mov bl, dl
    xor bh, bh
    sub di, 100h
loc_5449:
    mov ah, dl
loc_544B:
    mov al, byte ptr es:[di+100h]
    mov byte ptr es:[di], al
    call sub_653D
    inc di
    dec ah
    jne loc_544B
    sub di, bx
    add di, 100h
    dec dh
    jne loc_5449
    sub word ptr [si], 100h
    mov bx, word ptr [si+6]
    mov dh, dl
    mov bp, seg DATA
loc_5471:
    mov ds, word ptr ds:[2875h]
    mov al, byte ptr [bx]
    mov ds, bp
    mov byte ptr es:[di], al
    call sub_653D
    inc di
    inc bx
    dec dl
    jne loc_5471
    mov al, dh
    cbw
    sub word ptr [si+6], ax
    dec byte ptr [si+8]
    jne loc_5495
    mov word ptr [si+4], 0FFFFh
loc_5495:
    pop dx
    pop cx
    jmp near ptr loc_5415

sub_549A:
    mov cl, 4
    mov ax, word ptr PlayerY
    sar ax, cl
    mov dh, al
    mov ax, word ptr PlayerX
    sar ax, cl
    mov dl, al
    ret

sub_54AB:
    mov si, 6EA9h
    call SetCpuWriteMode
    mov dx, 3C4h
    mov ax, 0F02h
    out dx, ax
    mov dl, 0CEh
    mov ax, 1003h
    out dx, ax
loc_54BE:
    mov cl, 3
    mov ax, word ptr [si]
    cmp ax, 55AAh
    jne loc_54CA
    jmp near ptr loc_55E8
loc_54CA:
    dec byte ptr [si+6]
    js loc_54D2
    jmp near ptr loc_5573
loc_54D2:
    call StepRandomSeed
    and al, 7
    add al, 3
    mov byte ptr [si+6], al
    call NextWindRandom
    and al, 0Fh
    mov dx, word ptr [si]
    sar dx, cl
    cmp word ptr PlayerX, dx
    jge loc_54ED
    neg al
loc_54ED:
    test byte ptr [si+7], 7
    je loc_54F5
    neg al
loc_54F5:
    mov byte ptr byte_6BC0, al
    call NextWindRandom
    and al, 0Fh
    mov dx, word ptr [si+2]
    sar dx, cl
    add dx, 20h
    cmp word ptr PlayerY, dx
    jge loc_550D
    neg al
loc_550D:
    test byte ptr [si+7], 7
    je loc_5515
    neg al
loc_5515:
    mov byte ptr byte_6BC1, al
    call StepRandomSeed
    and al, 1Fh
    cmp byte ptr ds:byte_6BC0, 0
    jge loc_5526
    neg al
loc_5526:
    mov byte ptr [si+4], al
    cmp byte ptr ds:byte_6BC0, 0
    jne loc_5534
    mov al, 2
    jmp short loc_5550
loc_5534:
    mov bl, byte ptr byte_6BC0
    test bl, bl
    jns loc_553E
    neg bl
loc_553E:
    mov al, byte ptr byte_6BC1
    cbw
    test ax, ax
    jns loc_5548
    neg ax
loc_5548:
    shl ax, 1
    shl ax, 1
    shl ax, 1
    div bl
loc_5550:
    cmp al, 20h
    jb loc_5556
    mov al, 20h
loc_5556:
    cmp byte ptr ds:byte_6BC1, 0
    jge loc_555F
    neg al
loc_555F:
    mov byte ptr [si+5], al
    test byte ptr SnowFrameTick, 0Fh
    jne loc_5573
    mov al, byte ptr [si+7]
    and al, 7
    je loc_5573
    dec byte ptr [si+7]
loc_5573:
    mov al, byte ptr [si+4]
    cbw
    add word ptr [si], ax
    jno loc_557D
    sub word ptr [si], ax
loc_557D:
    mov al, byte ptr [si+5]
    cbw
    add word ptr [si+2], ax
    test byte ptr [si+6], 1
    jne loc_5597
    mov dx, 3CEh
    mov ax, 0
    out dx, ax
    mov ax, 101h
    out dx, ax
    jmp short loc_559E
loc_5597:
    mov dx, 3CEh
    mov ax, 1
    out dx, ax
loc_559E:
    mov ax, 0A000h
    mov es, ax
    mov ax, word ptr [si+2]
    sar ax, cl
    mov dx, word ptr CameraRow
    shl dx, cl
    shl dx, 1
    sub ax, dx
    cmp ax, 0B0h
    jae loc_55E8
    mov ah, 28h
    mul ah
    mov di, ax
    mov ax, word ptr [si]
    sar ax, cl
    mov dx, word ptr CameraCol
    shl dx, cl
    shl dx, 1
    sub ax, dx
    cmp ax, 140h
    jae loc_55E8
    mov cx, ax
    and cl, 7
    shr ax, 1
    shr ax, 1
    shr ax, 1
    add di, ax
    add di, word ptr ds:DrawPage
    mov al, 80h
    shr al, cl
    xchg byte ptr es:[di], al
loc_55E8:
    add si, 8
    cmp si, 6F49h
    jae loc_55F4
    jmp near ptr loc_54BE
loc_55F4:
    mov dx, 3CEh
    mov ax, 1
    out dx, ax
    ret

sub_55FC:
    push bx
    push di
    push si
    mov ax, seg DATA
    mov ds, ax
    mov es, ax
    mov di, 4F1Ch
    mov cx, 40Bh
    xor ax, ax
    rep stosw
    mov di, 4F0Ah
    mov cx, 74h
loc_5616:
    mov word ptr [di+4], 0FFFFh
    mov byte ptr [di+11h], 0
    add di, 12h
    loop loc_5616
    mov ax, word ptr LevelStartX
    mov word ptr PlayerX, ax
    mov ax, word ptr LevelStartY
    mov word ptr PlayerY, ax
    pop si
    pop di
    pop bx
    ret

sub_5634:
    mov byte ptr cs:cvar_6771, 1
    call sub_5649
    dec byte ptr cs:cvar_6771
    ret

sub_5643:
    mov byte ptr cs:cvar_6771, 0

sub_5649:
    mov ax, seg DATA
    mov ds, ax
    cmp byte ptr ds:byte_6BD9, 0
    jne loc_5662
    test byte ptr LevelFlags, 2
    jne loc_565F
    call sub_57A8
loc_565F:
    call sub_5663
loc_5662:
    ret

sub_5663:
    mov ax, seg DATA
    mov ds, ax
    cmp byte ptr cs:cvar_6771, 0
    jne loc_567D
    test byte ptr LevelFlags, 4
    je loc_567D
    mov dl, 1
    call ScrollViewDown
    ret
loc_567D:
    cmp word ptr ds:word_4F2A, 0
    jne loc_5689
    mov byte ptr VScrollActive, 0
loc_5689:
    mov dx, word ptr PlayerY
    mov cl, 4
    sar dx, cl
    sub dx, word ptr CameraRow
    cmp word ptr ds:word_4F2A, 0
    je loc_56AE
    mov ax, 3
    cmp dx, 9
    jge loc_56D9
    mov ax, 8
    cmp dx, 2
    jg loc_56E0
    jmp short loc_56D9
loc_56AE:
    test byte ptr LevelFlags, 1
    jne loc_56C2
    mov ax, 9
    cmp dx, 0Ah
    jge loc_56D9
    mov ax, 8
    jmp short loc_56D4
loc_56C2:
    mov ax, 7
    cmp dx, 8
    jge loc_56D9
    mov ax, 6
    cmp dx, 5
    jg loc_56E0
    jmp short loc_56D9
loc_56D4:
    cmp dx, 3
    jg loc_56E0
loc_56D9:
    mov word ptr word_6BF1, ax
    inc byte ptr VScrollActive
loc_56E0:
    cmp byte ptr ds:VScrollActive, 0
    jne loc_56EA
    jmp near ptr loc_57A7
loc_56EA:
    mov ax, word ptr LevelPropHeader
    add ax, 0Bh
    shl ax, cl
    cmp word ptr PlayerY, ax
    ja loc_5703
    mov bx, word ptr CameraRow
    dec bx
    cmp word ptr LevelPropHeader, bx
    jl loc_576A
loc_5703:
    cmp byte ptr ds:VScrollActive, 0
    jne loc_570D
    jmp near ptr loc_57A7
loc_570D:
    cmp word ptr word_6BF1, dx
    jne loc_5716
    jmp near ptr loc_57A2
loc_5716:
    jg loc_576A
    mov ax, word ptr LevelPropHeader
    mov dx, ax
    add dx, 0Bh
    shl dx, cl
    cmp word ptr PlayerY, dx
    ja loc_572E
    cmp word ptr CameraRow, ax
    ja loc_57A2
loc_572E:
    mov dl, 10h
    cmp byte ptr cs:cvar_6771, 0
    jne loc_5763
    mov ax, word ptr PlayerY
    sub al, byte ptr FineScroll
    sbb ah, 0
    mov dx, word ptr CameraRow
    add dx, word ptr word_6BF1
    shl dx, cl
    sub ax, dx
    mov bx, ax
    cmp byte ptr ds:GridDirtyFlag, 0
    jne loc_575D
    mov dl, byte ptr [bx+byte_78C6]
    jmp short loc_5763
loc_575D:
    add bx, word ptr word_78C4
    mov dl, byte ptr [bx]
loc_5763:
    call ScrollViewDown
    jb loc_57A2
    jmp short loc_57A7
loc_576A:
    mov dl, 10h
    cmp byte ptr cs:cvar_6771, 0
    jne loc_579D
    mov ax, word ptr PlayerY
    sub al, byte ptr FineScroll
    sbb ah, 0
    mov bx, word ptr CameraRow
    add bx, word ptr word_6BF1
    shl bx, cl
    sub bx, ax
    cmp byte ptr ds:GridDirtyFlag, 0
    jne loc_5797
    mov dl, byte ptr [bx+byte_78C6]
    jmp short loc_579D
loc_5797:
    add bx, word ptr word_78C4
    mov dl, byte ptr [bx]
loc_579D:
    call ScrollViewUp
    jae loc_57A7
loc_57A2:
    mov byte ptr VScrollActive, 0
loc_57A7:
    ret

sub_57A8:
    mov ax, seg DATA
    mov ds, ax
    mov dx, word ptr PlayerX
    mov cl, 4
    sar dx, cl
    sub dx, word ptr CameraCol
    cmp dx, 14h
    jae loc_57CC
    cmp byte ptr ds:byte_6BFE, 0
    jne loc_57CC
    cmp word ptr ds:word_4F22, 0
    je loc_57F0
loc_57CC:
    cmp byte ptr ds:HScrollDir, 0
    je loc_57F6
    cmp byte ptr ds:HScrollDir, 1
    je loc_57E6
    cmp dx, 0Fh
    jge loc_57F0
    call ScrollViewLeft
    jae loc_581D
    jmp short loc_57F0
loc_57E6:
    cmp dx, 5
    jle loc_57F0
    call ScrollViewRight
    jae loc_581D
loc_57F0:
    mov byte ptr HScrollDir, 0
    ret
loc_57F6:
    cmp word ptr ds:word_4F22, 0
    je loc_5806
    test byte ptr byte_4F23, 80h
    je loc_5813
    jmp short loc_580B
loc_5806:
    cmp dx, 0Ah
    jge loc_5813
loc_580B:
    mov al, 2
    cmp dx, 4
    jle loc_581A
    ret
loc_5813:
    mov al, 1
    cmp dx, 10h
    jl loc_581D
loc_581A:
    mov byte ptr HScrollDir, al
loc_581D:
    ret

sub_581E:
    mov si, word ptr word_6BBE
    mov cx, 5
loc_5825:
    mov ax, word ptr [si+4]
    and ah, 1Fh
    cmp ax, 0FFFFh
    je loc_583F
    inc ax
    dec word ptr [si+2]
    cmp ax, 3Ah
    jb loc_583C
    mov ax, 0FFFFh
loc_583C:
    mov word ptr [si+4], ax
loc_583F:
    mov bx, si
    sub si, 12h
    cmp si, 4F76h
    jae loc_584D
    mov si, 4FBEh
loc_584D:
    loop loc_5825
    ret

sub_5850:
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
    mov word ptr word_4F0E, 0FFFFh
    cmp byte ptr ds:byte_6BE4, 0
    jne loc_587C
    cmp byte ptr ds:byte_282F, 0
    je loc_587C
    call sub_65AF
    dec byte ptr byte_6BE4
    jmp near ptr loc_5A8C
loc_587C:
    cmp byte ptr ds:byte_2830, 0
    je loc_5892
    mov byte ptr byte_6BE5, 1
loc_5888:
    cmp byte ptr ds:byte_2830, 0
    je loc_5888
    jmp near ptr loc_5A8C
loc_5892:
    call TryShowCredits
    jb loc_589A
    call sub_454E
loc_589A:
    mov es, word ptr LevelSegment
    mov si, 4F1Ch
    call sub_6294
    call PollInputState
    mov dl, byte ptr byte_27ED
    or dl, byte ptr byte_27EC
    mov byte ptr byte_6BDB, dl
    mov dl, byte ptr byte_27EA
    or dl, byte ptr byte_27EB
    mov byte ptr byte_6BDC, dl
    cmp byte ptr ds:byte_27EC, 0
    je loc_58E2
    cmp byte ptr ds:byte_27ED, 0
    jne loc_58FC
    cmp word ptr ds:word_4F25, 1
    je loc_58FC
    mov word ptr word_4F25, 1
    mov word ptr word_6BEB, 0
    jmp short loc_58FC
loc_58E2:
    cmp byte ptr ds:byte_27ED, 0
    je loc_58FC
    cmp word ptr ds:word_4F25, -1
    je loc_58FC
    mov word ptr word_4F25, 0FFFFh
    mov word ptr word_6BEB, 0
loc_58FC:
    xor ah, ah
    mov al, byte ptr byte_27EC
    shr al, 1
    rcl ah, 1
    mov al, byte ptr byte_27ED
    shr al, 1
    rcl ah, 1
    mov al, byte ptr byte_27EA
    shr al, 1
    rcl ah, 1
    mov al, byte ptr byte_27EB
    shr al, 1
    rcl ah, 1
    mov al, byte ptr byte_27E8
    shr al, 1
    rcl ah, 1
    cmp byte ptr ds:byte_6BCD, 0
    je loc_592A
    xor ah, ah
loc_592A:
    mov bl, ah
    xor bh, bh
    mov al, byte ptr [bx+byte_7B7F]
    mov dl, byte ptr byte_4F2D
    mov byte ptr byte_4F1B, dl
    cmp dl, 16h
    jb loc_5941
    mov al, 8
loc_5941:
    cmp byte ptr byte_4F27, al
    je loc_5952
    mov word ptr word_6BEB, 0
    mov byte ptr byte_4F2C, 0
loc_5952:
    add word ptr word_6BEB, 1
    sbb word ptr word_6BEB, 0
    mov bl, al
    shl bx, 1
    cmp byte ptr ds:byte_6BC5, 0
    jne loc_596A
    jmp near ptr loc_5A0B
loc_596A:
    add bx, 40h
    and byte ptr byte_6BC7, 1
    jne loc_5977
    jmp near ptr loc_59FE
loc_5977:
    cmp byte ptr ds:byte_27EA, 0
    je loc_59B1
    cmp byte ptr ds:byte_7B1A, 6
    jne loc_5993
    cmp word ptr ds:word_4F2A, 10h
    jle loc_5997
    or byte ptr byte_6BC7, 2
    jmp short loc_5997
loc_5993:
    inc byte ptr byte_7B1A
loc_5997:
    cmp byte ptr ds:byte_6BC6, 0
    je loc_59B1
    dec byte ptr byte_6BC6
    cmp byte ptr ds:byte_7B1A, 4
    jb loc_5A0F
    mov word ptr word_4F2A, 0FFC0h
    jmp short loc_5A0F
loc_59B1:
    cmp byte ptr ds:byte_27EB, 0
    jne loc_59C4
    sub byte ptr byte_6BC6, 1
    adc byte ptr byte_6BC6, 0
    jmp short loc_5A0F
loc_59C4:
    cmp byte ptr ds:byte_7B1A, 0
    je loc_59CF
    dec byte ptr byte_7B1A
loc_59CF:
    or byte ptr byte_6BC7, 2
    cmp byte ptr ds:byte_7B1A, 1
    ja loc_5A0F
    push ax
    mov ax, word ptr word_4F22
    sar ax, 1
    sar ax, 1
    sar ax, 1
    sar ax, 1
    jns loc_59EB
    neg ax
loc_59EB:
    mov dl, al
    mov ah, 5
    mul ah
    cmp byte ptr byte_6BC6, al
    jae loc_59FB
    add byte ptr byte_6BC6, dl
loc_59FB:
    pop ax
    jmp short loc_5A0F
loc_59FE:
    cmp word ptr ds:word_4F2A, 0A0h
    jle loc_5A0B
    mov byte ptr byte_6BC7, 1
loc_5A0B:
    call word ptr [bx+dtbl_7D2F]
loc_5A0F:
    mov ax, word ptr word_4F22
    mov cl, 4
    sar ax, cl
    mov dx, word ptr PlayerX
    add ax, dx
    mov dx, word ptr CameraLeftLimit
    add dx, 14h
    shl dx, cl
    cmp dx, ax
    jle loc_5A36
    cmp ax, 8
    jl loc_5A36
    cmp ax, 0FF8h
    jge loc_5A36
    mov word ptr PlayerX, ax
loc_5A36:
    mov ax, word ptr word_4F2A
    mov cl, 4
    sar ax, cl
    add word ptr PlayerY, ax
    call sub_5A96
    call sub_484E
    mov dx, 1
    sub byte ptr byte_6BCE, dl
    adc byte ptr byte_6BCE, dh
    sub byte ptr byte_6BCD, dl
    adc byte ptr byte_6BCD, dh
    sub byte ptr byte_6BEA, dl
    adc byte ptr byte_6BEA, dh
    sub byte ptr byte_6BE8, dl
    adc byte ptr byte_6BE8, dh
    sub byte ptr byte_6BE4, dl
    adc byte ptr byte_6BE4, dh
    sub byte ptr byte_6BE1, dl
    adc byte ptr byte_6BE1, dh
    sub byte ptr byte_6C00, dl
    adc byte ptr byte_6C00, dh
    sub word ptr word_6BE2, 1
    adc word ptr word_6BE2, 0
loc_5A8C:
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

sub_5A96:
    push ax
    push cx
    push dx
    mov si, 4F1Ch
    mov ax, word ptr PlayerY
    mov cl, 4
    sar ax, cl
    dec ax
    xchg ah, al
    mov bx, word ptr word_4F20
    and bh, 1Fh
    shl bx, 1
    mov dh, byte ptr [bx+FrameHeights]
    mov bx, 9
    cmp word ptr ds:word_4F22, 0
    jg loc_5AC3
    jl loc_5AC1
    xor bx, bx
loc_5AC1:
    neg bx
loc_5AC3:
    push ax
    push bx
    mov bx, word ptr PlayerX
    sar bx, cl
    add bx, ax
    mov ax, word ptr PlayerY
    sar ax, cl
    sub ax, word ptr CameraRow
    jns loc_5ADA
    neg ax
loc_5ADA:
    cmp ax, 0Bh
    ja loc_5B18
    mov ax, word ptr PlayerX
    sar ax, cl
    sub ax, word ptr CameraCol
    jns loc_5AEC
    neg ax
loc_5AEC:
    cmp ax, 14h
    ja loc_5B18
    test byte ptr LevelFlags, 4
    je loc_5B03
    mov ax, word ptr CameraRow
    shl ax, cl
    cmp word ptr PlayerY, ax
    jl loc_5B18
loc_5B03:
    cmp word ptr ds:PlayerY, 0
    jl loc_5B29
    mov al, byte ptr MapRows
    xor ah, ah
    inc ax
    shl ax, cl
    cmp word ptr PlayerY, ax
    jle loc_5B29
loc_5B18:
    cmp byte ptr ds:CurrentLevel, 0Eh
    jne loc_5B26
    mov byte ptr byte_6BE5, 0FFh
    jmp short loc_5B29
loc_5B26:
    call sub_65AF
loc_5B29:
    mov byte ptr byte_6BF3, 0
    call sub_5B81
    cmp byte ptr ds:byte_6BF3, 1
    jne loc_5B54
    cmp byte ptr ds:byte_6BFE, 0
    je loc_5B44
    call sub_64DF
    jmp short loc_5B54
loc_5B44:
    call sub_63B5
    cmp word ptr ds:word_4F2A, 0
    jle loc_5B59
    inc byte ptr byte_6BD2
    jmp short loc_5B59
loc_5B54:
    mov byte ptr byte_6BD2, 0
loc_5B59:
    pop bx
    pop ax
    cmp word ptr ds:PlayerY, 0
    jle loc_5B7D
    add bx, word ptr PlayerX
    sar bx, cl
    add bx, ax
    call sub_5C92
    jmp short loc_5B72
loc_5B6F:
    call sub_5CAC
loc_5B72:
    sub bx, 100h
    jb loc_5B7D
    sub dh, 10h
    ja loc_5B6F
loc_5B7D:
    pop dx
    pop cx
    pop ax
    ret

sub_5B81:
    push ax
    push bx
    push bp
    cmp word ptr ds:PlayerY, -1
    jg loc_5B96
    call sub_63B5
    mov byte ptr byte_6BF3, 0FFh
    jmp near ptr loc_5C77
loc_5B96:
    push bx
    lea di, [bx+word_0100]
    mov ax, di
    cmp byte ptr MapRows, ah
    mov al, byte ptr es:[di]
    ja loc_5BA8
    xor al, al
loc_5BA8:
    mov bx, 7F5Eh
    xlat
    xor ah, ah
    mov bx, ax
    shl bx, 1
    cmp word ptr ds:word_6BAB, di
    je loc_5C04
    push ax
    push bx
    push di
    cmp word ptr ds:word_6BAB, 55AAh
    je loc_5BE8
    push di
    mov di, word ptr ds:word_6BAB
    mov bl, byte ptr es:[di]
    xor bh, bh
loc_5BCD:
    dec bx
    test byte ptr [bx-7FA2h], 20h
    jne loc_5BDE
    mov word ptr word_6BAB, 55AAh
    pop di
    jmp short loc_5BE8
loc_5BDE:
    mov al, bl
    mov byte ptr es:[di], al
    call sub_5C7B
    jmp short loc_5BCD
loc_5BE8:
    mov bl, byte ptr es:[di]
    xor bh, bh
    test byte ptr [bx-7FA2h], 20h
    je loc_5C01
    mov word ptr ds:word_6BAB, di
    inc bx
    mov al, bl
    mov byte ptr es:[di], al
    call sub_5C7B
loc_5C01:
    pop di
    pop bx
    pop ax
loc_5C04:
    call word ptr [bx+word_7D9B]
    pop bx
    sub bx, 100h
    jb loc_5C77
    cmp word ptr ds:word_4F2A, 0
    jg loc_5C77
    mov di, bx
    mov ah, byte ptr es:[bx]
    mov al, byte ptr es:[bx+100h]
    mov bx, 7E5Eh
    xlat
    xchg ah, al
    mov bx, 805Eh
    xlat
    and al, 0Fh
    mov bl, al
    xor bh, bh
    shl bx, 1
    push ax
    call word ptr [bx+word_7DA9]
    pop ax
    and ah, 1
    je loc_5C77
    cmp word ptr ds:PlayerY, 0
    jle loc_5C77
    mov dx, 0FFFFh
    mov bx, 7E5Eh
    cmp word ptr ds:word_4F22, 0
    jg loc_5C53
    neg dx
loc_5C53:
    push di
    add di, dx
    mov al, byte ptr es:[di+100h]
    xlat
    or al, al
    je loc_5C70
    neg dx
    add di, dx
    add di, dx
    mov al, byte ptr es:[di+100h]
    xlat
    or al, al
    jne loc_5C76
loc_5C70:
    shl dx, 1
    add word ptr PlayerX, dx
loc_5C76:
    pop di
loc_5C77:
    pop bp
    pop bx
    pop ax
    ret

sub_5C7B:
    cmp byte ptr [bx+TileClassTable], 1
    jb loc_5C8E
    mov byte ptr GridDirtyFlag, 1
    mov word ptr PrevCameraCol, 55AAh
    ret
loc_5C8E:
    call sub_653D
    ret

sub_5C92:
    push bx
    push dx
    push di
    mov di, bx
    mov al, byte ptr es:[bx]
    mov bx, 7E5Eh
    xlat
    xor ah, ah
    mov bx, ax
    shl bx, 1
    call word ptr [bx+dtbl_7D95]
    pop di
    pop dx
    pop bx
    ret

sub_5CAC:
    push bx
    push dx
    push di
    mov di, bx
    mov al, byte ptr es:[bx]
    mov bx, 7E5Eh
    xlat
    cmp al, 4
    je loc_5CC0
    cmp al, 2
    jne loc_5CCA
loc_5CC0:
    xor ah, ah
    mov bx, ax
    shl bx, 1
    call word ptr [bx+dtbl_7D95]
loc_5CCA:
    pop di
    pop dx
    pop bx
    ret

sub_5CCE:
    call sub_62EC
    call sub_6333
    call sub_6374
    call sub_638B
    ret

sub_5CDB:
    cmp byte ptr ds:byte_6BD0, 0
    je loc_5CE5
    jmp near ptr loc_5F93
loc_5CE5:
    push ax
    push bx
    push dx
    mov byte ptr byte_6BC8, 0
    call sub_62EC
    call sub_6333
    cmp byte ptr ds:byte_6BFE, 0
    jne loc_5D0E
    cmp word ptr ds:word_4F2A, 0
    je loc_5D0E
    cmp byte ptr ds:byte_6BD1, 4
    jbe loc_5D0B
    call sub_62EC
loc_5D0B:
    jmp near ptr loc_5E0D
loc_5D0E:
    mov ax, word ptr word_4F22
    or ax, ax
    jns loc_5D17
    neg ax
loc_5D17:
    cmp ax, 8
    jb loc_5D42
    mov al, byte ptr byte_4F21
    mov ah, byte ptr byte_4F23
    rol al, 1
    rol ah, 1
    and ax, 101h
    cmp al, ah
    je loc_5D31
    jmp near ptr loc_5DED
loc_5D31:
    mov bx, 24h
    mov al, 12h
    call sub_635D
    call sub_638B
    call sub_5E11
    jmp near ptr loc_5E08
loc_5D42:
    or ax, ax
    je loc_5D49
    jmp near ptr loc_5DED
loc_5D49:
    cmp byte ptr ds:byte_6BD3, 1Eh
    jb loc_5D73
    cmp byte ptr ds:byte_27E9, 0
    jne loc_5D83
    mov al, byte ptr byte_27EC
    and al, byte ptr byte_27ED
    jne loc_5D83
    sub byte ptr byte_6BD3, 3
    mov bx, 20h
    mov al, 10h
    call sub_635D
    call sub_638B
    jmp near ptr loc_5E08
loc_5D73:
    mov al, byte ptr byte_27EC
    and al, byte ptr byte_27ED
    jne loc_5D83
    cmp byte ptr ds:byte_27E9, 0
    je loc_5DC9
loc_5D83:
    cmp byte ptr ds:byte_6BFE, 0
    jne loc_5DC9
    mov bx, 26h
    mov al, 13h
    call sub_635D
    call sub_638B
    test byte ptr LevelFlags, 2
    jne loc_5E08
    cmp byte ptr ds:byte_6BD9, 0
    jne loc_5E08
    mov ax, word ptr PlayerX
    mov cl, 4
    sar ax, cl
    sub ax, word ptr CameraCol
    test byte ptr byte_4F21, 80h
    jne loc_5DBF
    cmp ax, 2
    jbe loc_5E08
    call ScrollViewRight
    jmp short loc_5E08
loc_5DBF:
    cmp ax, 11h
    jae loc_5E08
    call ScrollViewLeft
    jmp short loc_5E08
loc_5DC9:
    mov si, 79E0h
    mov ax, word ptr word_27F0
    and ax, 1FFh
loc_5DD2:
    cmp ax, word ptr [si]
    jb loc_5DED
    cmp ax, word ptr [si+2]
    jb loc_5DE0
    add si, 4
    jmp short loc_5DD2
loc_5DE0:
    mov bx, 22h
    mov al, 11h
    call sub_635D
    call sub_638B
    jmp short loc_5E08
loc_5DED:
    mov bx, word ptr [bx+word_7CDF]
    mov word ptr word_4F28, bx
    mov ax, word ptr [bx]
    mov dh, byte ptr word_4F25
    and dh, 80h
    or ah, dh
    mov word ptr word_4F20, ax
    mov byte ptr byte_4F2C, 0
loc_5E08:
    mov byte ptr byte_4F27, 0
loc_5E0D:
    pop dx
    pop bx
    pop ax
    ret

sub_5E11:
    test byte ptr SnowFrameTick, 3
    jne loc_5E40

sub_5E18:
    push ax
    push bx
    mov bx, word ptr word_6BBE
    mov ax, word ptr PlayerX
    mov word ptr [bx], ax
    mov ax, word ptr PlayerY
    mov word ptr [bx+2], ax
    mov word ptr [bx+4], 35h
    sub bx, 12h
    cmp bx, 4F76h
    jae loc_5E3A
    mov bx, 4FBEh
loc_5E3A:
    mov word ptr word_6BBE, bx
    pop bx
    pop ax
loc_5E40:
    ret

sub_5E41:
    push bx
    mov bx, word ptr word_6BBE
    mov word ptr [bx], ax
    mov word ptr [bx+2], dx
    mov word ptr [bx+4], 35h
    sub bx, 12h
    cmp bx, 4F76h
    jae loc_5E5C
    mov bx, 4FBEh
loc_5E5C:
    mov word ptr word_6BBE, bx
    pop bx
    ret

sub_5E62:
    cmp byte ptr ds:byte_6BD0, 0
    je loc_5E6C
    jmp near ptr loc_5F93
loc_5E6C:
    mov byte ptr byte_6BD3, 0
    mov byte ptr byte_6BE1, 4
    call sub_5EB7
    mov ax, word ptr word_4F22
    or ax, ax
    jns loc_5E82
    neg ax
loc_5E82:
    mov bp, 20h
    cmp ax, bp
    jbe loc_5E8C
    jmp near ptr sub_5CDB
loc_5E8C:
    call sub_62B1
    call sub_6374
    call sub_638B
    ret

sub_5E96:
    cmp byte ptr ds:byte_6BD0, 0
    je loc_5EA0
    jmp near ptr loc_5F93
loc_5EA0:
    mov byte ptr byte_6BC8, 0
    mov byte ptr byte_6BE1, 4
    call sub_6374
    call sub_638B
    call sub_6333
    call sub_5EB7
    ret

sub_5EB7:
    cmp byte ptr ds:byte_6BCE, 30h
    ja loc_5EC3
    add byte ptr byte_6BCE, 2
loc_5EC3:
    ret

sub_5EC4:
    cmp byte ptr ds:byte_6BD0, 0
    je loc_5ECE
    jmp near ptr loc_5F93
loc_5ECE:
    push ax
    mov ax, word ptr word_4F22
    or ax, ax
    jns loc_5ED8
    neg ax
loc_5ED8:
    cmp ax, 40h
    jl loc_5EF8
    cmp byte ptr ds:byte_6BC5, 0
    je loc_5EF8
    add byte ptr byte_6BC8, 1
    sbb byte ptr byte_6BC8, 0
    cmp byte ptr ds:byte_6BC8, 17h
    jne loc_5EF8
    call sub_5E18
loc_5EF8:
    pop ax
    add byte ptr byte_6BD3, 1
    sbb byte ptr byte_6BD3, 0
    mov bp, 50h
    call sub_62B1
    call sub_62EC
    call sub_6374
    call sub_638B
    ret

sub_5F13:
    cmp byte ptr ds:[6BC8h], 18h
    jb loc_5F41
    mov byte ptr ds:[6BC7h], 1
    mov byte ptr ds:[6BC6h], 18h
    sub word ptr ds:[4F1Eh], 3
    mov byte ptr ds:[6BC8h], 0
    jmp short loc_5F8C

sub_5F30:
    cmp byte ptr ds:byte_6BD0, 0
    jne loc_5F93
    cmp byte ptr ds:byte_6BE0, 0
    je loc_5F41
    jmp near ptr sub_5CDB
loc_5F41:
    mov byte ptr byte_6BFE, 0
    mov bl, byte ptr byte_6BD1
    xor bh, bh
    inc byte ptr byte_6BD1
    cmp bl, 9
    jae loc_5F6A
    shl bx, 1
    mov ax, word ptr [bx+word_79CE]
    cmp byte ptr ds:byte_6BC5, 0
    je loc_5F64
    sar ax, 1
loc_5F64:
    add word ptr word_4F2A, ax
    jmp short loc_5F70
loc_5F6A:
    mov bp, 0C0h
    call sub_6309
loc_5F70:
    mov bp, 30h
    cmp word ptr word_4F22, bp
    jb loc_5F7E
    call sub_6333
    jmp short loc_5F81
loc_5F7E:
    call sub_62B1
loc_5F81:
    mov bx, 4
    mov al, 2
    call sub_6374
    call sub_638B
loc_5F8C:
    call sub_62EC
    call sub_62EC
    ret
loc_5F93:
    mov al, byte ptr byte_4F27

sub_5F96:
    call sub_6374
    call sub_638B
    call sub_6333
    add byte ptr byte_6BD3, 1
    sbb byte ptr byte_6BD3, 0
    mov al, 5
    mul byte ptr byte_7B18
    mov bx, ax
    mov al, byte ptr [bx+byte_7B07]
    cmp byte ptr ds:byte_6BCE, 0
    je loc_5FC0
    shl al, 1
    shl al, 1
loc_5FC0:
    mov byte ptr byte_7B19, al
    mov al, byte ptr byte_6BCF
    and al, 40h
    xor al, 40h
    mov byte ptr byte_6BD0, al
    je loc_5FD2
    jmp near ptr loc_6081
loc_5FD2:
    mov al, byte ptr [bx+byte_7B06]
    mov byte ptr byte_6BCD, al
    mov dx, 5
    mov al, byte ptr byte_7B18
    or al, al
    je loc_5FEB
    mov dl, 0
    cmp al, 1
    je loc_5FEB
    mov dl, 0Ah
loc_5FEB:
    call StartSoundEffect
    xor dx, dx
    mov al, byte ptr byte_4F27
    cmp al, 6
    je loc_6004
    mov dx, 0FFE0h
    cmp al, 3
    je loc_6004
    mov dx, 0FFD0h
    call sub_5E11
loc_6004:
    cmp byte ptr ds:byte_6BFE, 0
    jne loc_600F
    add word ptr word_4F2A, dx
loc_600F:
    mov al, byte ptr [bx+byte_7B08]
    shr al, 1
    jae loc_6075
    call sub_627C
    jb loc_6075
    and al, 3
    mov byte ptr [si+8], al
    mov bx, word ptr [bx+word_7B04]
loc_6025:
    inc bx
    inc bx
    cmp word ptr [bx], 55AAh
    jne loc_6025
    add bx, 6
    mov word ptr [si+0Ch], bx
    mov ax, word ptr [bx]
    mov cx, word ptr [bx-4]
    mov bx, word ptr [bx-2]
    mov word ptr [si+0Eh], bx
    mov dh, byte ptr word_4F25
    and dh, 80h
    je loc_604B
    or ah, dh
    neg cx
loc_604B:
    mov word ptr [si+4], ax
    mov word ptr [si+6], cx
    mov ax, word ptr RenderSlots
    sar cx, 1
    sar cx, 1
    sar cx, 1
    sar cx, 1
    add ax, cx
    mov word ptr [si], ax
    mov ax, word ptr word_4F0C
    sar bx, 1
    sar bx, 1
    sar bx, 1
    sar bx, 1
    add ax, bx
    mov word ptr [si+2], ax
    mov ax, 0FFFFh
    jmp short loc_60AE
loc_6075:
    cmp byte ptr ds:byte_6BD2, 0
    je loc_6081
    mov ax, 0FFFFh
    jmp short loc_60AE
loc_6081:
    mov si, word ptr [bx+word_7B04]
    mov ax, word ptr word_4F20
    mov dh, ah
    and ah, 1Fh
    mov bx, 0FFF8h
loc_6090:
    add bx, 8
    mov cx, word ptr [bx+si]
    cmp cx, 55AAh
    je loc_60DE
    cmp cx, ax
    jne loc_6090
    mov ax, word ptr [bx+si+2]
    mov cx, word ptr [bx+si+4]
    and dh, 80h
    je loc_60AE
    or ah, dh
    neg cx
loc_60AE:
    mov word ptr word_4F0E, ax
    mov ax, word ptr PlayerX
    mov dx, word ptr word_4F22
    sar dx, 1
    sar dx, 1
    sar dx, 1
    sar dx, 1
    add ax, dx
    sub ax, cx
    mov word ptr RenderSlots, ax
    mov ax, word ptr PlayerY
    mov dx, word ptr word_4F2A
    sar dx, 1
    sar dx, 1
    sar dx, 1
    sar dx, 1
    add ax, dx
    sub ax, word ptr [bx+si+6]
    mov word ptr word_4F0C, ax
loc_60DE:
    ret

sub_60DF:
    mov si, 5450h
    mov cx, 10h
loc_60E5:
    cmp word ptr [si+4], -1
    je loc_60F8
    dec word ptr [si+2]
    dec word ptr [si+0Ch]
    jne loc_60F8
    mov word ptr [si+4], 0FFFFh
loc_60F8:
    add si, 12h
    loop loc_60E5
    ret

sub_60FE:
    mov si, 50A8h
    mov bp, 20h
    mov cl, 4
    mov es, word ptr LevelSegment
loc_610A:
    cmp word ptr [si+4], -1
    je loc_6136
    dec word ptr [si+0Ch]
    jg loc_6139
    je loc_6124
    cmp byte ptr [si+11h], 0
    jne loc_6136
    mov word ptr [si+4], 0FFFFh
    jmp short loc_6136
loc_6124:
    mov byte ptr [si+11h], 0Fh
    jmp near ptr loc_6206
loc_612B:
    cmp word ptr [si+0Ch], 32h
    jbe loc_6136
    mov word ptr [si+0Ch], 32h
loc_6136:
    jmp near ptr loc_6206
loc_6139:
    mov ax, word ptr [si+6]
    sar ax, cl
    add word ptr [si], ax
    jns loc_6149
    mov word ptr [si], 0
    neg word ptr [si+6]
loc_6149:
    cmp word ptr [si], 1000h
    jb loc_6152
    neg word ptr [si+6]
loc_6152:
    mov ax, word ptr [si+0Eh]
    mov dx, ax
    sar ax, cl
    add word ptr [si+2], ax
    add dx, 9
    cmp dx, 100h
    jge loc_6168
    mov word ptr [si+0Eh], dx
loc_6168:
    mov dx, word ptr [si+4]
    and dh, 1Fh
    cmp dx, 136h
    je loc_612B
    cmp dx, 134h
    je loc_612B
    cmp dx, 0E5h
    je loc_6136
    cmp dx, 12Ch
    je loc_612B
    cmp dx, 132h
    je loc_612B
    mov dx, word ptr [si]
    sar dx, cl
    mov bl, dl
    mov dx, word ptr [si+2]
    sar dx, cl
    mov bh, dl
    or ax, ax
    jle loc_61CC
    mov al, byte ptr es:[bx]
    mov bx, 7F5Eh
    xlat
    or al, al
    je loc_61E3
    neg word ptr [si+0Eh]
    sar word ptr [si+0Eh], 1
    xor dx, dx
    mov ax, word ptr [si+6]
    mov dx, 8
    or ax, ax
    jns loc_61BC
    neg dx
loc_61BC:
    sub ax, dx
    mov dx, ax
    xor dh, byte ptr [si+7]
    je loc_61C7
    xor ax, ax
loc_61C7:
    mov word ptr [si+6], ax
    jmp short loc_61E3
loc_61CC:
    mov al, byte ptr es:[bx-100h]
    mov bx, 7E5Eh
    xlat
    or al, al
    je loc_61E3
    neg word ptr [si+6]
    mov ax, word ptr [si+6]
    sar ax, cl
    add word ptr [si], ax
loc_61E3:
    test byte ptr SnowFrameTick, 1
    jne loc_6206
    cmp word ptr [si+6], 0
    je loc_6206
    mov ax, word ptr [si+4]
    mov dx, ax
    inc dx
    and ah, 1Fh
    cmp ax, 49h
    jb loc_6203
    ja loc_6206
    mov dx, 46h
loc_6203:
    mov word ptr [si+4], dx
loc_6206:
    add si, 12h
    dec bp
    je loc_620F
    jmp near ptr loc_610A
loc_620F:
    ret

sub_6210:
    mov si, 4F2Eh
    mov cx, 4
loc_6216:
    cmp word ptr [si+4], -1
    je loc_626C
    test byte ptr [si+5], 20h
    jne loc_6229
    mov word ptr [si+4], 0FFFFh
    jmp short loc_626C
loc_6229:
    mov ax, word ptr [si+6]
    sar ax, 1
    sar ax, 1
    sar ax, 1
    sar ax, 1
    add word ptr [si], ax
    mov ax, word ptr [si+0Eh]
    sar ax, 1
    sar ax, 1
    sar ax, 1
    sar ax, 1
    add word ptr [si+2], ax
    mov bx, word ptr [si+0Ch]
    mov ax, word ptr [bx]
    or ax, ax
    jns loc_6251
    add bx, ax
    mov ax, word ptr [bx]
loc_6251:
    inc bx
    inc bx
    mov word ptr [si+0Ch], bx
    mov bl, byte ptr [si+7]
    and bl, 80h
    or ah, bl
    mov word ptr [si+4], ax
    mov bl, byte ptr [si+8]
    xor bh, bh
    shl bx, 1
    call word ptr [bx+dtbl_79EC]
loc_626C:
    add si, 12h
    loop loc_6216
    ret

sub_6272:
    add word ptr [si+0Eh], 20h
    ret

sub_6277:
    sub word ptr [si+0Eh], 10h
    ret

sub_627C:
    push cx
    mov si, 4F2Eh
    mov cx, 4
loc_6283:
    cmp word ptr [si+4], -1
    je loc_6291
    add si, 12h
    loop loc_6283
    pop cx
    stc
    ret
loc_6291:
    pop cx
    clc
    ret

sub_6294:
    cmp byte ptr ds:byte_280D, 0
    je loc_62B0
loc_629B:
    cmp byte ptr ds:byte_280D, 0
    jne loc_629B
loc_62A2:
    cmp byte ptr ds:byte_280D, 0
    je loc_62A2
loc_62A9:
    cmp byte ptr ds:byte_280D, 0
    jne loc_62A9
loc_62B0:
    ret

sub_62B1:
    push ax
    push cx
    push dx
    push bp
    xor ax, ax
    xor dx, dx
    cmp byte ptr ds:byte_6BDB, 0
    je loc_62CB
    mov ax, word ptr word_4F25
    shl ax, 1
    shl ax, 1
    shl ax, 1
    shl ax, 1
loc_62CB:
    mov cl, byte ptr byte_4F24
    sar ax, cl
    mov dx, word ptr word_4F22
    add dx, ax
    cmp dx, bp
    jge loc_62E3
    neg bp
    cmp dx, bp
    jle loc_62E3
    mov bp, dx
loc_62E3:
    mov word ptr word_4F22, bp
    pop bp
    pop dx
    pop cx
    pop ax
    ret

sub_62EC:
    push ax
    mov ax, word ptr SnowWind
    shr ax, 1
    shr ax, 1
    shr ax, 1
    sub word ptr word_4F22, ax
    cmp word ptr ds:word_4F22, -60h
    jge loc_6307
    mov word ptr word_4F22, 0FFA0h
loc_6307:
    pop ax
    ret

sub_6309:
    push ax
    push cx
    push dx
    mov dx, word ptr word_4F2A
    mov ax, 10h
    cmp byte ptr ds:byte_6BC7, 1
    jne loc_6323
    mov ax, 4
    sar bp, 1
    sar bp, 1
    sar bp, 1
loc_6323:
    add dx, ax
    cmp dx, bp
    jge loc_632B
    mov bp, dx
loc_632B:
    mov word ptr word_4F2A, bp
    pop dx
    pop cx
    pop ax
    ret

sub_6333:
    push cx
    push dx
    xor ch, ch
    mov ax, word ptr word_4F22
    or ax, ax
    jns loc_6342
    neg ax
    inc ch
loc_6342:
    mov cl, byte ptr byte_4F24
    mov dx, 0Ch
    shr dx, cl
    sub ax, dx
    jae loc_6351
    xor ax, ax
loc_6351:
    or ch, ch
    je loc_6357
    neg ax
loc_6357:
    mov word ptr word_4F22, ax
    pop dx
    pop cx
    ret

sub_635D:
    cmp byte ptr byte_4F2C, al
    je loc_636F
    mov byte ptr byte_4F2C, al
    mov bx, word ptr [bx+word_7CDF]
    mov word ptr word_4F28, bx
    ret
loc_636F:
    mov bx, word ptr word_4F28
    ret

sub_6374:
    cmp byte ptr byte_4F27, al
    je loc_6386
    mov byte ptr byte_4F27, al
    mov bx, word ptr [bx+word_7CDF]
    mov word ptr word_4F28, bx
    ret
loc_6386:
    mov bx, word ptr word_4F28
    ret

sub_638B:
    push ax
    push bx
    push dx
    mov ax, word ptr [bx]
    or ax, ax
    jns loc_6398
    add bx, ax
    mov ax, word ptr [bx]
loc_6398:
    mov byte ptr byte_6BCF, ah
    and ah, 1Fh
    mov dh, byte ptr word_4F25
    and dh, 80h
    or ah, dh
    mov word ptr word_4F20, ax
    inc bx
    inc bx
    mov word ptr word_4F28, bx
    pop dx
    pop bx
    pop ax
    ret

sub_63B5:
    push ax
    push bp
    mov bp, 50h
    call sub_62B1
    mov bp, 0C0h
    call sub_6309
    cmp byte ptr ds:byte_6BC5, 0
    je loc_63D6
    mov al, 2Dh
    cmp word ptr ds:word_4F2A, 0
    jl loc_63F4
    inc ax
    jmp short loc_63F4
loc_63D6:
    cmp word ptr ds:word_4F2A, 0
    jle loc_63FE
    mov byte ptr byte_6BE0, 6
    cmp byte ptr ds:byte_6BD0, 0
    jne loc_63FE
    mov al, 0Ch
    cmp byte ptr ds:byte_6BD2, 0Ch
    jb loc_63F4
    inc al
loc_63F4:
    mov ah, byte ptr word_4F25
    and ah, 80h
    mov word ptr word_4F20, ax
loc_63FE:
    pop bp
    pop ax
    ret

sub_6401:
    or byte ptr byte_6BF3, 1
    ret

sub_6407:
    push ax
    mov ax, word ptr word_4F22
    sar ax, 1
    sar ax, 1
    sar ax, 1
    sar ax, 1
    sub word ptr PlayerX, ax
    mov word ptr word_4F22, 0
    pop ax
    ret

sub_641F:
    mov byte ptr byte_4F24, 0
    cmp word ptr ds:word_4F2A, 0
    jge loc_642D
    jmp short sub_6401
loc_642D:
    mov byte ptr byte_6BC7, 0
    and byte ptr PlayerY, 0F0h
    mov bl, byte ptr es:[di]
    xor bh, bh
    mov al, byte ptr [bx-71E3h]
    cbw
    or ax, ax
    je loc_645E
    call sub_661A
    mov bx, word ptr word_4F2A
    sar bx, 1
    sar bx, 1
    sar bx, 1
    sar bx, 1
    jle loc_6478
    cmp ax, bx
    jl loc_6478
    mov ax, bx
    jmp short loc_6478
loc_645E:
    mov bl, byte ptr es:[di-100h]
    xor bh, bh
    mov al, byte ptr [bx-71E3h]
    or al, al
    je loc_647C
    call sub_661A
    cmp ax, 10h
    jge loc_647C
    sub ax, 10h
loc_6478:
    add word ptr PlayerY, ax
loc_647C:
    cmp byte ptr ds:byte_6BD2, 4
    jbe loc_64D9
    call sub_5E18
    mov ax, word ptr PlayerY
    sub ax, word ptr word_6BCA
    cmp ax, 20h
    jl loc_64D9
    cmp word ptr ds:word_4F2A, 50h
    jl loc_64D9
    mov ax, word ptr PlayerY
    mov word ptr word_6BCA, ax
    cmp byte ptr ds:byte_6BD2, 14h
    jb loc_64B3
    cmp word ptr ds:word_4F2A, 0A0h
    jle loc_64B3
    mov byte ptr byte_6BEA, 8
loc_64B3:
    cmp byte ptr ds:byte_6BD2, 0Ah
    jbe loc_64D9
    test byte ptr LevelFlags, 1
    jne loc_64C7
    mov word ptr word_4F2A, 0FFE0h
loc_64C7:
    mov ax, word ptr word_4F20
    and ax, 0E000h
    or ax, 0Ch
    mov word ptr word_4F20, ax
    mov byte ptr byte_6BD2, 0
    ret
loc_64D9:
    mov word ptr word_4F2A, 0

sub_64DF:
    sub byte ptr byte_6BE0, 1
    adc byte ptr byte_6BE0, 0
    mov byte ptr byte_6BD1, 0
    mov byte ptr byte_6BF3, 2
    mov ax, word ptr PlayerY
    mov word ptr word_6BCA, ax
    ret
loc_64FA:
    mov si, 6EA9h
loc_64FD:
    cmp word ptr [si], 55AAh
    jne loc_6522
    mov cl, 3
    mov ax, word ptr PlayerX
    shl ax, cl
    mov word ptr [si], ax
    mov ax, word ptr PlayerY
    shl ax, cl
    mov word ptr [si+2], ax
    mov byte ptr [si+4], 0
    mov byte ptr [si+5], 0
    mov byte ptr [si+7], 0
    jmp short loc_652B
loc_6522:
    add si, 8
    cmp si, 6F49h
    jb loc_64FD
loc_652B:
    ret

sub_652C:
    mov bl, byte ptr es:[di]
    xor bh, bh
    test byte ptr [bx-7FA2h], 10h
    jne loc_64FA
    ret

sub_6539:
    call sub_6407
    ret

sub_653D:
    push ax
    push bx
    push cx
    push dx
    push di
    push si
    push ds
    push es
    push bp
    mov dx, di
    sub dl, byte ptr ds:[2DE4h]
    cmp dl, 14h
    jae loc_65A4
    sub dh, byte ptr ds:[2DE6h]
    cmp dh, 0Ch
    jae loc_65A4
    push ax
    mov bl, 50h
    mov bh, 2
    cmp byte ptr cs:OptionVByte, 3
    jb loc_656E
    call SetLatchCopyMode
    mov bl, 28h
    mov bh, 1
loc_656E:
    mov ax, di
    mov al, ah
    xor ah, ah
    mov dl, 0Ch
    div dl
    mov al, bl
    mul ah
    shl ax, 1
    shl ax, 1
    shl ax, 1
    shl ax, 1
    push ax
    mov ax, di
    xor ah, ah
    mov dl, 14h
    div dl
    mov al, ah
    xor ah, ah
    mov cl, bh
    shl ax, cl
    pop di
    add di, ax
    pop ax
    call BlitTileToRing
    mov byte ptr ds:[6BBDh], 1
    clc
    jmp short loc_65A5
loc_65A4:
    stc
loc_65A5:
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

sub_65AF:
    call sub_65B3
    ret

sub_65B3:
    cmp byte ptr ds:byte_6BE4, 0
    jne loc_65CF
    cmp byte ptr ds:byte_27D8, 0
    je loc_65D0
    dec byte ptr byte_27D8
    mov byte ptr byte_27D6, 0
    mov byte ptr byte_6BE4, 2
loc_65CF:
    ret
loc_65D0:
    mov byte ptr byte_6BE5, 1
    ret

sub_65D6:
    push si
    mov si, 2172h
    cmp byte ptr [si+byte_0666], 63h
    je loc_65ED
    jb loc_65E9
    mov byte ptr cs:[26FAh], 0CFh
loc_65E9:
    inc byte ptr [si+byte_0666]
loc_65ED:
    pop si
    ret

sub_65EF:
    cmp word ptr ds:word_4F2A, 0
    jne loc_6616
    mov al, byte ptr es:[di+100h]
    mov bx, 8E1Dh
    xlat
    or al, al
    je loc_6616
    call sub_661A
    cmp ax, 10h
    jge loc_6616
    add di, 100h
    add word ptr PlayerY, 10h
    jmp short sub_6641
loc_6616:
    call sub_6401
    ret

sub_661A:
    test al, 30h
    je loc_6640
    push bx
    push ax
    mov ax, word ptr PlayerX
    and ax, 0Fh
    mov bl, 3
    div bl
    pop bx
    test bl, 10h
    je loc_6637
    and bl, 0Fh
    add al, bl
    jmp short loc_663E
loc_6637:
    and bl, 0Fh
    neg al
    add al, bl
loc_663E:
    cbw
    pop bx
loc_6640:
    ret

sub_6641:
    call sub_641F
    ret

sub_6645:
    mov byte ptr byte_4F24, 0
    cmp byte ptr ds:byte_6BE1, 0
    jne loc_6654
    jmp near ptr sub_641F
loc_6654:
    jmp near ptr sub_6401

sub_6657:
    call sub_641F
    mov byte ptr byte_4F24, 1
    ret

sub_6660:
    call sub_641F
    mov byte ptr byte_4F24, 2
    ret

sub_6669:
    call sub_641F
    mov byte ptr byte_4F24, 3
    ret

sub_6672:
    ret

sub_6673:
    cmp word ptr ds:word_4F2A, 0
    je loc_668B
    mov word ptr word_4F2A, 0
    and word ptr PlayerY, 0FFF0h
    add word ptr PlayerY, 10h
    ret
loc_668B:
    mov al, byte ptr word_4F20
    cmp al, 0Ah
    je loc_66D2
    cmp al, 15h
    je loc_66D2
    push bx
    push di
    mov di, word ptr ds:word_6BB1
    mov cl, 4
    mov ax, word ptr [di+2]
    sar ax, cl
    mov bh, al
    mov ax, word ptr [di]
    sar ax, cl
    mov bl, al
    mov di, bx
    mov es, word ptr LevelSegment
    mov bx, 7E5Eh
    mov al, byte ptr es:[di]
    xlat
    and al, 1
    je loc_66D0
    mov dx, 10h
    mov al, byte ptr es:[di-1]
    xlat
    and al, 1
    jne loc_66CA
    neg dx
loc_66CA:
    mov di, word ptr ds:word_6BB1
    add word ptr [di], dx
loc_66D0:
    pop di
    pop bx
loc_66D2:
    ret

unused_66D3:
    push ax
    push bx
    push dx
    mov ax, word ptr [si]
    sub ax, word ptr [di]
    jns loc_66DE
    neg ax
loc_66DE:
    cmp ax, 40h
    jae loc_675B
    mov ax, word ptr [si+2]
    sub ax, word ptr [di+2]
    jns loc_66ED
    neg ax
loc_66ED:
    cmp ax, 46h
    jae loc_675B
    mov ax, word ptr [si+2]
    mov dx, word ptr [di+2]
    mov bx, word ptr [si+4]
    cmp ax, dx
    jge loc_6703
    xchg dx, ax
    mov bx, word ptr [di+4]
loc_6703:
    and bh, 1Fh
    shl bx, 1
    mov bl, byte ptr [bx+7191h]
    xor bh, bh
    sub bx, 3
    sub ax, bx
    cmp ax, dx
    jge loc_675B
    mov bx, word ptr [si+4]
    and bh, 1Fh
    shl bx, 1
    mov bp, bx
    mov al, byte ptr [bx+752Ah]
    xor ah, ah
    mov dx, word ptr [si]
    sub dx, ax
    mov bx, word ptr [di+4]
    and bh, 1Fh
    shl bx, 1
    mov ax, word ptr [di]
    sub al, byte ptr [bx+752Ah]
    sbb ah, 0
    mov bl, byte ptr [bx+7190h]
    shr bl, 1
    cmp ax, dx
    jl loc_674E
    xchg dx, ax
    mov bl, byte ptr ds:[bp+7190h]
    shr bl, 1
loc_674E:
    xor bh, bh
    add ax, bx
    cmp ax, dx
    jle loc_675B
    pop dx
    pop bx
    pop ax
    stc
    ret
loc_675B:
    pop dx
    pop bx
    pop ax
    clc
    ret

unused_6760:
    mov ax, word ptr [si]
    mov dx, 0Ch
    test byte ptr [si+5], 80h
    je loc_676E
    mov dx, 0FFF6h
loc_676E:
    add ax, dx
    ret
cvar_6771 label byte
; DATA: flag byte
    db 00h

sub_6772:
    mov al, byte ptr byte_6C01
    or al, byte ptr byte_6C02
    je loc_67D6
    inc byte ptr byte_6C03
    mov bl, byte ptr CurrentLevel
    xor bh, bh
    shl bx, 1
    mov si, word ptr [bx+LevelPaletteTable]
    mov dx, 3C8h
    xor al, al
    out dx, al
    mov bx, 0ACB7h
    mov cx, 30h
    xor bp, bp
    cmp byte ptr ds:byte_6C02, 0
    je loc_67A2
    xchg si, bx
loc_67A2:
    lodsb
    mov ah, byte ptr [bx]
    mov dl, byte ptr byte_6C03
    mov dh, dl
    sub al, ah
    jns loc_67B3
    neg al
    neg dh
loc_67B3:
    cmp al, dl
    ja loc_67BB
    mov al, ah
    jmp short loc_67C1
loc_67BB:
    mov al, byte ptr [si-1]
    sub al, dh
    inc bp
loc_67C1:
    mov dx, 3C9h
    out dx, al
    inc bx
    loop loc_67A2
    or bp, bp
    jne loc_67D6
    mov byte ptr byte_6C01, 0
    mov byte ptr byte_6C02, 0
loc_67D6:
    ret

sub_67D7:
    cmp byte ptr ds:byte_6CA7, 1Fh
    jne loc_680D
    mov ax, word ptr PlayerX
    mov word ptr word_A336, ax
    mov ax, word ptr PlayerY
    sub ax, 70h
    mov word ptr word_A338, ax
    mov ax, 6Eh
    mov word ptr word_A33A, ax
    xor ax, ax
    xor dx, dx
    mov cx, 1
    call sub_8D1B
    mov byte ptr byte_6CA7, 0
    mov byte ptr byte_6BFF, 1
    mov byte ptr byte_6C00, 2Ch
    ret
loc_680D:
    mov al, byte ptr byte_6CA8
    and al, 38h
    cmp al, 38h
    jne loc_6821
    and byte ptr byte_6CA8, 0C7h
    mov word ptr word_6BE2, 294h
loc_6821:
    ret

sub_6822:
    cmp byte ptr ds:byte_91FE, 0FFh
    je loc_682C
    call sub_70D7
loc_682C:
    cmp byte ptr ds:CurrentLevel, 5
    jne loc_6844
    test byte ptr LevelFlags, 0FEh
    jne loc_6844
    cmp word ptr ds:word_A326, 3
    je loc_6844
    call sub_6D34
loc_6844:
    cmp byte ptr ds:CurrentLevel, 9
    jne loc_684E
    call sub_6ADD
loc_684E:
    mov si, 4FD0h
    mov bp, 0Ch
    mov cl, 4
loc_6856:
    mov ax, word ptr [si+4]
    cmp ax, 0FFFFh
    jne loc_6861
    jmp near ptr loc_690A
loc_6861:
    mov ax, word ptr [si+0Ah]
    sar ax, cl
    add word ptr [si+2], ax
    mov ax, word ptr [si+8]
    cmp ax, 0FFFFh
    je loc_6875
    sar ax, cl
    add word ptr [si], ax
loc_6875:
    mov bx, word ptr [si+6]
    test byte ptr [bx+4], 8
    je loc_6881
    call sub_698C
loc_6881:
    mov bx, word ptr [si+0Ch]
loc_6884:
    mov dx, word ptr [bx]
    cmp dx, 0
    jge loc_688F
    add bx, dx
    jmp short loc_6884
loc_688F:
    mov ax, dx
    and dh, 1Fh
    and ah, 0E0h
    or ah, byte ptr word_6BE2
    mov byte ptr byte_A340, ah
    add dx, 138h
    cmp word ptr ds:word_6BE2, 0
    je loc_68D2
    cmp word ptr ds:word_6BE2, 7
    jne loc_68B6
    mov byte ptr byte_6BEA, 9
loc_68B6:
    mov di, 0A801h
    and dh, 1Fh
loc_68BC:
    cmp dx, word ptr [di]
    jb loc_68D2
    cmp dx, word ptr [di+2]
    jbe loc_68CA
    add di, 6
    jmp short loc_68BC
loc_68CA:
    mov dx, word ptr [di+4]
    add dx, 35h
    jmp short loc_68D7
loc_68D2:
    inc bx
    inc bx
    mov word ptr [si+0Ch], bx
loc_68D7:
    and dh, 1Fh
    mov al, byte ptr [si+9]
    and al, 80h
    or dh, al
    and word ptr [si+4], 6000h
    or word ptr [si+4], dx
    mov bx, word ptr [si+6]
    mov bl, byte ptr [bx+1]
    xor bh, bh
    shl bl, 1
    push ax
    push bx
    push cx
    push dx
    push di
    push si
    push ds
    push es
    push bp
    call word ptr cs:[bx+tbl_6AA9]
    pop bp
    pop es
    pop ds
    pop si
    pop di
    pop dx
    pop cx
    pop bx
    pop ax
loc_690A:
    add si, 12h
    dec bp
    je loc_6913
    jmp near ptr loc_6856
loc_6913:
    mov si, 8489h
loc_6916:
    cmp byte ptr [si], 32h
    jae loc_698B
    cmp word ptr [si+2], -1
    je loc_6984
    test byte ptr [si+4], 4
    jne loc_6984
    cmp byte ptr ds:byte_B198, 1
    je loc_6934
    test byte ptr [si+1], 80h
    jne loc_6984
loc_6934:
    mov bl, byte ptr [si+1]
    xor bh, bh
    shl bl, 1
    push ax
    push bx
    push cx
    push dx
    push di
    push si
    push ds
    push es
    push bp
    call word ptr cs:[bx+tbl_6AC3]
    pop bp
    pop es
    pop ds
    pop si
    pop di
    pop dx
    pop cx
    pop bx
    pop ax
    jb loc_6984
    mov bx, 0A86Fh
    mov ax, word ptr [si+2]
    sub ax, 138h
    mov dl, byte ptr [si+1]
    and dl, 7Fh
    xor dh, dh
loc_6965:
    inc bx
    inc bx
    cmp word ptr [bx], 7D01h
    jne loc_6965
    cmp word ptr [bx+2], dx
    jne loc_6965
    add bx, 4
loc_6975:
    cmp word ptr [bx], ax
    je loc_697D
    inc bx
    inc bx
    jmp short loc_6975
loc_697D:
    mov di, word ptr ds:word_A32E
    mov word ptr [di+0Ch], bx
loc_6984:
    mov al, byte ptr [si]
    cbw
    add si, ax
    jmp short loc_6916
loc_698B:
    ret

sub_698C:
    push ax
    push bx
    push cx
    push dx
    push es
    mov cl, 4
    mov ax, word ptr [si+2]
    sar ax, cl
    mov bh, al
    mov ax, word ptr [si]
    sar ax, cl
    mov bl, al
    mov es, word ptr LevelSegment
    mov dl, byte ptr es:[bx]
    mov dh, byte ptr es:[bx-100h]
    mov ax, word ptr [si+8]
    cmp ax, 0
    je loc_69BB
    mov ax, 1
    jg loc_69BB
    neg ax
loc_69BB:
    add bx, ax
    mov al, byte ptr es:[bx-100h]
    mov bx, 7E5Eh
    xlat
    mov bx, word ptr [si+6]
    test byte ptr [bx+4], 40h
    je loc_69F6
    test byte ptr [bx+4], 80h
    je loc_69F6
    or al, al
    je loc_69E3
    mov ax, word ptr [si+8]
    sar ax, cl
    sub word ptr [si], ax
    jmp near ptr loc_6A77
loc_69E3:
    call sub_8048
    mov word ptr [si+0Ah], 0
    and byte ptr [bx+4], 7Fh
    sub word ptr [si+2], 10h
    jmp near ptr loc_6A77
loc_69F6:
    or al, al
    je loc_6A21
    test byte ptr [bx+4], 40h
    je loc_6A17
    call sub_8058
    or byte ptr [bx+4], 80h
    mov word ptr [si+0Ah], 0FFF0h
    mov ax, word ptr [si+8]
    sar ax, 1
    sar ax, 1
    add word ptr [si], ax
    jmp short loc_6A77
loc_6A17:
    neg word ptr [si+8]
    mov ax, word ptr [si+8]
    sar ax, cl
    add word ptr [si], ax
loc_6A21:
    mov bx, 7F5Eh
    sub word ptr [si+2], 10h
    xchg dl, dh
    mov al, dl
    xlat
    or al, al
    jne loc_6A3E
    add word ptr [si+2], 10h
    mov dl, dh
    mov al, dl
    xlat
    or al, al
    je loc_6A6C
loc_6A3E:
    mov al, dl
    call sub_6A7D
    and byte ptr [si+2], 0F0h
    add word ptr [si+2], ax
    mov bx, word ptr [si+6]
    test byte ptr [bx+4], 20h
    jne loc_6A65
    mov ax, word ptr [si+0Ah]
    neg ax
    sar ax, 1
    mov dx, ax
    jns loc_6A60
    neg ax
loc_6A60:
    cmp ax, 20h
    ja loc_6A67
loc_6A65:
    xor dx, dx
loc_6A67:
    mov word ptr [si+0Ah], dx
    jmp short loc_6A77
loc_6A6C:
    cmp word ptr [si+0Ah], 100h
    jge loc_6A77
    add word ptr [si+0Ah], 10h
loc_6A77:
    pop es
    pop dx
    pop cx
    pop bx
    pop ax
    ret

sub_6A7D:
    push bx
    push dx
    mov bx, 8E1Dh
    xlat
    test al, 30h
    je loc_6AA5
    mov dx, ax
    mov ax, word ptr [si]
    and ax, 0Fh
    mov bl, 3
    div bl
    test dl, 10h
    je loc_6A9E
    and dl, 0Fh
    add al, dl
    jmp short loc_6AA5
loc_6A9E:
    and dl, 0Fh
    neg al
    add al, dl
loc_6AA5:
    cbw
    pop dx
    pop bx
    ret
tbl_6AA9 label word
    dw offset sub_7C90
    dw offset sub_7C8C
    dw offset sub_7C2D
    dw offset sub_7B91
    dw offset sub_7ADF
    dw offset sub_7A60
    dw offset sub_78EC
    dw offset sub_7898
    dw offset sub_77DE
    dw offset sub_773D
    dw offset sub_7665
    dw offset sub_760F
    dw offset sub_75C4
tbl_6AC3 label word
    dw offset sub_7F6C
    dw offset sub_7F26
    dw offset sub_7EE2
    dw offset sub_7ED8
    dw offset sub_7EBF
    dw offset sub_7EB5
    dw offset sub_7EB5
    dw offset sub_7EB5
    dw offset sub_7EB5
    dw offset sub_7E97
    dw offset sub_7D9B
    dw offset sub_7D6E
    dw offset sub_7D1B

sub_6ADD:
    mov ax, seg DATA
    mov ds, ax
    mov ax, 0A000h
    mov es, ax
    cmp word ptr ds:word_A517, -1
    jne loc_6B04
    mov word ptr word_A4F7, 0A4F9h
    mov word ptr word_A519, 18h
    mov byte ptr byte_A515, 0
    mov byte ptr byte_A516, 3
loc_6B04:
    mov cx, word ptr word_A519
    sar cx, 1
    sar cx, 1
    call sub_7585
    cmp word ptr ds:word_A519, 0
    jne loc_6B1C
    mov al, 2
    call sub_6C0D
    ret
loc_6B1C:
    cmp byte ptr ds:byte_A515, 0
    jne loc_6B43
    mov bx, word ptr word_A4F7
    mov ax, word ptr [bx]
    cmp ax, 0FFFFh
    jne loc_6B33
    add bx, word ptr [bx+2]
    mov ax, word ptr [bx]
loc_6B33:
    mov word ptr word_A517, ax
    mov ax, word ptr [bx+2]
    mov byte ptr byte_A515, al
    add bx, 4
    mov word ptr word_A4F7, bx
loc_6B43:
    mov si, 4F2Eh
    mov cx, 4
loc_6B49:
    cmp word ptr [si+4], -1
    je loc_6B8C
    mov ax, word ptr [si]
    sub ax, 0B9h
    cmp ax, 32h
    jae loc_6B8C
    mov ax, word ptr [si+2]
    sub ax, 1Eh
    cmp ax, 32h
    jae loc_6B8C
    mov word ptr word_A517, 0A534h
    sub byte ptr word_A519, 1
    adc byte ptr word_A519, 0
    jne loc_6B79
    call sub_6BDB
loc_6B79:
    inc byte ptr byte_A516
    and byte ptr byte_A516, 3
    jne loc_6B91
    mov word ptr word_A517, 0A540h
    jmp short loc_6B91
loc_6B8C:
    add si, 12h
    loop loc_6B49
loc_6B91:
    mov bx, word ptr word_A517
loc_6B95:
    mov al, byte ptr [bx]
    cmp al, 0FFh
    jne loc_6BA1
    call sub_6CA7
    inc bx
    jmp short loc_6B95
loc_6BA1:
    cmp al, 0FEh
    jne loc_6BAB
    call sub_6CF1
    inc bx
    jmp short loc_6B95
loc_6BAB:
    cmp al, 0FDh
    jne loc_6BB8
    mov dx, 2
    call StartSoundEffect
    inc bx
    jmp short loc_6B95
loc_6BB8:
    or al, al
    jns loc_6BD3
    cmp al, 3
    je loc_6BCA
    sub byte ptr byte_A515, 1
    adc byte ptr byte_A515, 0
loc_6BCA:
    cbw
    add bx, ax
    mov word ptr word_A517, bx
    jmp short loc_6B95
loc_6BD3:
    inc word ptr word_A517
    call sub_6C0D
    ret

sub_6BDB:
    mov ax, 0B9h
    mov word ptr word_A336, ax
    mov ax, 1Eh
    mov word ptr word_A338, ax
    mov cx, 4
    mov ax, 20h
    mov dx, 0FF60h
loc_6BF0:
    push cx
    mov cx, 1
    push ax
    mov word ptr word_A33A, 2137h
    pop ax
    call sub_8D1B
    neg ax
    js loc_6C09
    sub ax, 10h
    sub dx, 10h
loc_6C09:
    pop cx
    loop loc_6BF0
    ret

sub_6C0D:
    mov di, 3F40h
    add di, 18h
    call SetLatchCopyMode
    mov ah, 6
    mul ah
    mov bx, ax
    add bx, 14h
    mov ch, 7
loc_6C21:
    mov cl, 6
loc_6C23:
    push ds
    mov ds, word ptr LevelSegment
    mov al, byte ptr [bx]
    inc bx
    push es
    pop ds
    xor ah, ah
    shl ax, 1
    shl ax, 1
    shl ax, 1
    shl ax, 1
    shl ax, 1
    add ax, 5E80h
    mov si, ax
    movsb
    movsb
    add di, 26h
    movsb
    movsb
    add di, 26h
    movsb
    movsb
    add di, 26h
    movsb
    movsb
    add di, 26h
    movsb
    movsb
    add di, 26h
    movsb
    movsb
    add di, 26h
    movsb
    movsb
    add di, 26h
    movsb
    movsb
    add di, 26h
    movsb
    movsb
    add di, 26h
    movsb
    movsb
    add di, 26h
    movsb
    movsb
    add di, 26h
    movsb
    movsb
    add di, 26h
    movsb
    movsb
    add di, 26h
    movsb
    movsb
    add di, 26h
    movsb
    movsb
    add di, 26h
    movsb
    movsb
    add di, 26h
    sub di, 27Eh
    pop ds
    dec cl
    jne loc_6C23
    add bx, 0FAh
    add di, 274h
    dec ch
    je loc_6CA6
    jmp near ptr loc_6C21
loc_6CA6:
    ret

sub_6CA7:
    push cx
    push si
    mov si, 50A8h
    mov cx, 20h
loc_6CAF:
    cmp word ptr [si+4], -1
    je loc_6CBC
    add si, 12h
    loop loc_6CAF
    jmp short loc_6CEE
loc_6CBC:
    mov word ptr [si+4], 1CAh
    mov word ptr [si], 0C8h
    mov word ptr [si+2], 58h
    mov byte ptr [si+11h], 0
    mov word ptr [si+0Ch], 84h
    mov word ptr [si+9], 0FFFFh
    call NextWindRandom
    and ax, 0Fh
    shl ax, 1
    shl ax, 1
    shl ax, 1
    neg ax
    mov word ptr [si+6], ax
    mov word ptr [si+0Eh], 0
loc_6CEE:
    pop si
    pop cx
    ret

sub_6CF1:
    push cx
    push si
    mov si, 50A8h
    mov cx, 20h
loc_6CF9:
    cmp word ptr [si+4], -1
    je loc_6D06
    add si, 12h
    loop loc_6CF9
    jmp short loc_6D31
loc_6D06:
    mov word ptr [si+4], 1CBh
    call NextWindRandom
    and ax, 7Fh
    sub ax, 10h
    mov word ptr [si], ax
    mov word ptr [si+2], 0
    mov byte ptr [si+11h], 0
    mov word ptr [si+0Ch], 42h
    mov word ptr [si+9], 0FFFFh
    xor ax, ax
    mov word ptr [si+6], ax
    mov word ptr [si+0Eh], ax
loc_6D31:
    pop si
    pop cx
    ret

sub_6D34:
    mov cx, 2
    sub cx, word ptr word_A326
    shl cx, 1
    call sub_7585
    mov bx, 7DAFh
    mov si, 4F1Ch
    mov di, 55EEh
    mov cx, 5
loc_6D4C:
    mov bp, word ptr [bx+4]
    cmp bp, -1
    je loc_6DD2
    cmp byte ptr ds:byte_6BE4, 0
    jne loc_6D80
    call sub_8D7B
    jae loc_6D80
    mov byte ptr byte_4F2D, 2Ch
    mov byte ptr byte_6BD0, 0
    mov word ptr word_4F2A, 0FF80h
    mov byte ptr byte_4F24, 3
    mov word ptr word_4F22, 0FF80h
    call sub_824D
    jmp short loc_6DC9
loc_6D80:
    mov dx, word ptr [bx+6]
    sar dx, 1
    sar dx, 1
    sar dx, 1
    sar dx, 1
    add word ptr [bx+2], dx
    mov al, byte ptr [bx+0Ah]
    cbw
    add word ptr [bx+8], ax
    mov dx, word ptr [bx+8]
    cmp dx, 20h
    jge loc_6DA2
    cmp dx, -20h
    jge loc_6DA5
loc_6DA2:
    neg byte ptr [bx+0Ah]
loc_6DA5:
    sar dx, 1
    sar dx, 1
    sar dx, 1
    sar dx, 1
    je loc_6DB7
    mov ax, 1
    jg loc_6DB5
    inc ax
loc_6DB5:
    add bp, ax
loc_6DB7:
    add word ptr [bx], dx
    mov dx, word ptr [bx]
    mov word ptr [di], dx
    mov dx, word ptr [bx+2]
    mov word ptr [di+2], dx
    cmp dx, 7D8h
    jle loc_6DCF
loc_6DC9:
    mov bp, 0FFFFh
    mov word ptr [bx+4], bp
loc_6DCF:
    mov word ptr [di+4], bp
loc_6DD2:
    add di, 12h
    add bx, 0Bh
    dec cx
    je loc_6DDE
    jmp near ptr loc_6D4C
loc_6DDE:
    mov ax, 0FFFFh
    mov word ptr word_5670, ax
    mov word ptr word_565E, ax
    cmp word ptr ds:word_A326, 2
    jae loc_6E12
    mov al, 0Ch
    mul byte ptr byte_A329
    mov si, ax
    add si, 0A4B5h
    lodsw
    mov word ptr word_566C, ax
    lodsw
    mov word ptr word_566E, ax
    lodsw
    mov word ptr word_5670, ax
    lodsw
    mov word ptr word_565A, ax
    lodsw
    mov word ptr word_565C, ax
    lodsw
    mov word ptr word_565E, ax
loc_6E12:
    mov al, 0Ch
    mul byte ptr byte_A32A
    mov si, ax
    add si, 0A485h
    lodsw
    mov word ptr word_5690, ax
    lodsw
    mov word ptr word_5692, ax
    lodsw
    mov word ptr word_5694, ax
    lodsw
    mov word ptr word_567E, ax
    lodsw
    mov word ptr word_5680, ax
    lodsw
    mov word ptr word_5682, ax
    mov al, 6
    mul byte ptr byte_A328
    mov si, ax
    add si, 0A4E5h
    lodsw
    mov word ptr word_5648, ax
    lodsw
    mov word ptr word_564A, ax
    lodsw
    cmp word ptr ds:word_A326, 0
    je loc_6E55
    mov ax, 0FFFFh
loc_6E55:
    mov word ptr word_564C, ax
    test byte ptr SnowFrameTick, 3
    jne loc_6E6D
    mov al, byte ptr byte_A328
    inc al
    cmp al, 2
    jbe loc_6E6A
    xor al, al
loc_6E6A:
    mov byte ptr byte_A328, al
loc_6E6D:
    cmp byte ptr ds:byte_A324, 1
    jb loc_6E8B
    jne loc_6E84
    mov byte ptr byte_A329, 2
    dec byte ptr byte_A32A
    add byte ptr byte_A32B, 5
loc_6E84:
    dec byte ptr byte_A324
loc_6E88:
    jmp near ptr loc_6F8F
loc_6E8B:
    cmp byte ptr ds:byte_A32B, 0
    jne loc_6E88
    mov al, byte ptr byte_A329
    inc al
    cmp al, 2
    jbe loc_6E9D
    xor al, al
loc_6E9D:
    mov byte ptr byte_A329, al
    mov al, byte ptr byte_A32A
    inc al
    cmp al, 2
    jbe loc_6EAB
    xor al, al
loc_6EAB:
    mov byte ptr byte_A32A, al
    mov ah, 1
    cmp al, 1
    je loc_6F0F
    mov ah, 3
    mov byte ptr byte_6BEA, 4
    add word ptr PlayerX, 2
    push ax
    mov si, 7DAFh
    mov cx, 5
loc_6EC7:
    cmp word ptr [si+4], -1
    je loc_6ED4
    add si, 0Bh
    loop loc_6EC7
    jmp short loc_6F0E
loc_6ED4:
    mov word ptr [si+4], 1B3h
    mov ax, word ptr PlayerY
    sub ax, 96h
    mov word ptr [si+2], ax
    call NextWindRandom
    mov dl, al
    and al, 7Ch
    test al, 4
    je loc_6EEF
    neg al
loc_6EEF:
    cbw
    add ax, word ptr PlayerX
    mov word ptr [si], ax
    and dx, 3
    inc dx
    shl dx, 1
    shl dx, 1
    shl dx, 1
    shl dx, 1
    mov word ptr [si+6], dx
    mov word ptr [si+8], 0
    mov byte ptr [si+0Ah], 4
loc_6F0E:
    pop ax
loc_6F0F:
    mov byte ptr byte_A32B, ah
    dec byte ptr byte_A32C
    jne loc_6F3C
    call NextWindRandom
    and al, 0Fh
    shl al, 1
    shl al, 1
    shl al, 1
    mov byte ptr byte_A32C, al
    call NextWindRandom
    and al, 0Fh
    shl al, 1
    shl al, 1
    shl al, 1
    add al, 40h
    mov byte ptr byte_A32B, al
    mov byte ptr byte_A329, 0
loc_6F3C:
    cmp byte ptr ds:byte_6BE4, 0
    je loc_6F46
    jmp near ptr loc_70CB
loc_6F46:
    mov si, 4F1Ch
    mov di, 5690h
    call sub_8D7B
    jb loc_6F59
    mov di, 567Eh
    call sub_8D7B
    jae loc_6F8F
loc_6F59:
    cmp word ptr ds:PlayerY, 7BBh
    jg loc_6F69
    mov word ptr word_4F2A, 0FF60h
    jmp short loc_6FBB
loc_6F69:
    mov word ptr word_4F2A, 0FF60h
    mov byte ptr byte_4F2D, 2Ch
    mov byte ptr byte_6BD0, 0
    mov byte ptr byte_4F24, 3
    mov word ptr word_4F22, 0FF80h
    call sub_824D
    call sub_824D
    call sub_824D
    jmp short loc_6FBB
loc_6F8F:
    cmp byte ptr ds:byte_6BE4, 0
    je loc_6F99
    jmp near ptr loc_70CB
loc_6F99:
    mov si, 4F1Ch
    mov di, 5690h
    call sub_8D7B
    jae loc_6FBB
    cmp byte ptr ds:byte_A330, 0
    je loc_6FBB
    mov ax, 0FFC0h
    cmp byte ptr ds:byte_27EA, 0
    je loc_6FB8
    mov ax, 0FF80h
loc_6FB8:
    mov word ptr word_4F2A, ax
loc_6FBB:
    mov di, 4F0Ah
    mov si, 5648h
    mov al, 24h
    mul byte ptr word_A326
    add si, ax
    cmp word ptr [di+4], -1
    je loc_6FDE
    mov byte ptr byte_A312, 1
    call sub_8D7B
    mov byte ptr byte_A312, 0
    jb loc_7001
loc_6FDE:
    mov di, 4F2Eh
    mov cx, 4
loc_6FE4:
    cmp word ptr [di+4], -1
    je loc_6FF9
    mov byte ptr byte_A312, 1
    call sub_8D7B
    mov byte ptr byte_A312, 0
    jb loc_7001
loc_6FF9:
    add di, 12h
    loop loc_6FE4
    jmp near ptr loc_70A5
loc_7001:
    xor byte ptr [si+5], 40h
    cmp word ptr ds:word_A326, 0
    je loc_7010
    xor byte ptr [si-0Dh], 40h
loc_7010:
    mov word ptr [di+4], 0FFFFh
    mov byte ptr byte_A324, 6
    cmp word ptr ds:word_A326, 2
    jae loc_702B
    mov byte ptr byte_A329, 3
    mov byte ptr byte_A32A, 3
loc_702B:
    dec byte ptr byte_A325
    jne loc_70A5
    mov byte ptr byte_A325, 7
    inc word ptr word_A326
    cmp word ptr ds:word_A326, 3
    jne loc_70A5
    mov ax, word ptr word_5648
    sub ax, 0C8h
    mov word ptr word_A336, ax
    mov ax, word ptr word_564A
    mov word ptr word_A338, ax
    call sub_94F3
    add word ptr word_A336, 20h
    add word ptr word_A338, 20h
    call sub_94F3
    sub word ptr word_A336, 40h
    call sub_94F3
    sub word ptr word_A336, 20h
    sub word ptr word_A338, 40h
    call sub_94F3
    xor dx, dx
    xor ax, ax
    mov cx, 1
    mov word ptr word_A33A, 63h
    call sub_8D1B
    mov di, 55EEh
    mov cx, 5
loc_708B:
    mov word ptr [di+4], 0FFFFh
    add di, 12h
    loop loc_708B
    mov di, 5648h
    mov cx, 5
loc_709B:
    mov word ptr [di+4], 0FFFFh
    add di, 12h
    loop loc_709B
loc_70A5:
    cmp word ptr ds:PlayerX, 3D8h
    jle loc_70CB
    mov byte ptr byte_4F2D, 2Ch
    mov byte ptr byte_6BD0, 0
    mov word ptr word_4F2A, 0FF70h
    mov byte ptr byte_4F24, 3
    mov word ptr word_4F22, 0FF60h
    call sub_824D
loc_70CB:
    mov ax, 1
    sub byte ptr byte_A32B, al
    adc byte ptr byte_A32B, ah
    ret

sub_70D7:
    mov cx, word ptr word_91FC
    shr cx, 1
    shr cx, 1
    shr cx, 1
    mov di, word ptr ds:word_564C
    cmp di, -1
    je loc_70F7
    cmp byte ptr ds:byte_91FE, 6
    je loc_70F7
    test di, 2000h
    jne loc_70F9
loc_70F7:
    xor cx, cx
loc_70F9:
    call sub_7585
    mov cl, 4
    mov ax, word ptr word_6C06
    sar ax, cl
    add ax, word ptr word_91FF
    js loc_7118
    cmp word ptr word_91F7, ax
    jg loc_7118
    cmp word ptr word_91F9, ax
    jl loc_7118
    mov word ptr word_91FF, ax
loc_7118:
    mov ax, word ptr word_6C08
    sar ax, cl
    add word ptr word_9201, ax
    cmp word ptr ds:word_6C08, 0
    jl loc_7165
    mov bx, word ptr word_9201
    sar bx, cl
    mov ax, word ptr word_91FF
    sar ax, cl
    mov bh, bl
    mov bl, al
    mov es, word ptr LevelSegment
    mov al, byte ptr es:[bx]
    mov bx, 7F5Eh
    xlat
    or al, al
    je loc_7165
    and byte ptr word_9201, 0F0h
    cmp word ptr ds:word_6C08, 0
    je loc_7157
    mov byte ptr byte_6BEA, 7
loc_7157:
    mov word ptr word_6C08, 0
    mov word ptr word_6C06, 0
    jmp short loc_7172
loc_7165:
    cmp word ptr ds:word_6C08, 0E0h
    jge loc_7172
    add word ptr word_6C08, 10h
loc_7172:
    xor dx, dx
    mov ax, word ptr PlayerX
    cmp word ptr word_91FF, ax
    jae loc_717E
    inc dx
loc_717E:
    mov byte ptr byte_A3FA, dl
    mov ax, word ptr PlayerX
    sub ax, word ptr word_91FF
    jns loc_718D
    neg ax
loc_718D:
    mov word ptr word_A3FB, ax
    cmp ax, 190h
    jbe loc_7198
    jmp near ptr loc_7579
loc_7198:
    mov ax, word ptr PlayerY
    sub ax, word ptr word_9201
    jns loc_71A3
    neg ax
loc_71A3:
    cmp ax, 0FAh
    jbe loc_71AB
    jmp near ptr loc_7579
loc_71AB:
    cmp byte ptr ds:byte_91FE, 0
    jne loc_71D6
    cmp word ptr ds:word_A3FB, 0FAh
    jb loc_71C8
    mov byte ptr byte_6C05, 0
    mov word ptr word_A401, 0A427h
    jmp near ptr loc_7534
loc_71C8:
    mov word ptr word_A3F7, 0
    mov byte ptr byte_91FE, 1
    jmp near ptr loc_7534
loc_71D6:
    cmp byte ptr ds:byte_91FE, 1
    je loc_71E0
    jmp near ptr loc_7262
loc_71E0:
    cmp word ptr ds:word_A3FB, 0FAh
    jb loc_71F0
    mov byte ptr byte_91FE, 0
    jmp near ptr loc_7534
loc_71F0:
    inc word ptr word_A3F7
    cmp word ptr ds:word_91FC, 3Ch
    jae loc_7203
    mov byte ptr byte_91FE, 2
    jmp near ptr loc_7534
loc_7203:
    mov word ptr word_A401, 0A434h
    cmp word ptr ds:word_A3F7, 6Eh
    jb loc_7221
    mov byte ptr byte_91FE, 2
    call sub_757A
    mov word ptr word_A3F7, 0
    jmp near ptr loc_7534
loc_7221:
    cmp byte ptr ds:byte_6BD0, 0
    je loc_7235
    test byte ptr SnowFrameTick, 7
    jne loc_7235
    call sub_757A
    jmp near ptr loc_7534
loc_7235:
    cmp byte ptr ds:byte_6C05, 0Ah
    jb loc_724A
    mov byte ptr byte_91FE, 3
    mov word ptr word_A3F7, 0
    jmp near ptr loc_7534
loc_724A:
    cmp byte ptr ds:byte_6C05, 3
    ja loc_7254
    jmp near ptr loc_7534
loc_7254:
    mov byte ptr byte_91FE, 2
    mov word ptr word_A3F7, 0
    jmp near ptr loc_7534
loc_7262:
    cmp byte ptr ds:byte_91FE, 2
    je loc_726C
    jmp near ptr loc_7366
loc_726C:
    inc word ptr word_A3F7
    cmp word ptr ds:word_A3F7, 2Ch
    je loc_7299
    ja loc_72C4
    call sub_81B4
    cmp word ptr ds:word_A3FB, 4Bh
    jae loc_7290
    mov byte ptr byte_91FE, 3
    mov word ptr word_A3F7, 0
    jmp short loc_7296
loc_7290:
    mov word ptr word_A401, 0A43Bh
loc_7296:
    jmp near ptr loc_7534
loc_7299:
    call sub_81B4
    mov word ptr word_6C08, 0FF20h
    cmp word ptr ds:word_91FC, 28h
    jae loc_72BB
    cmp word ptr ds:word_A3FB, 50h
    jae loc_72BB
    mov byte ptr byte_91FE, 4
    mov word ptr word_A3F7, 0
loc_72BB:
    mov word ptr word_A401, 0A45Eh
    jmp near ptr loc_7534
loc_72C4:
    cmp word ptr ds:word_6C08, 0
    jle loc_72DB
    mov ax, 0A460h
    cmp word ptr ds:word_91FC, 64h
    jge loc_72D8
    mov ax, 0A462h
loc_72D8:
    mov word ptr word_A401, ax
loc_72DB:
    cmp word ptr ds:word_A3FB, 50h
    ja loc_72F5
    mov word ptr word_A3F7, 0
    mov byte ptr byte_91FE, 3
    mov byte ptr byte_6C05, 0Bh
    jmp near ptr loc_7534
loc_72F5:
    cmp word ptr ds:word_A3F7, 58h
    jae loc_72FF
    jmp near ptr loc_7534
loc_72FF:
    jne loc_7349
    mov al, byte ptr byte_91FB
    mov ah, 50h
    mul ah
    mov bx, ax
    mov word ptr word_A401, 0A45Eh
    mov ax, word ptr word_91FF
    sub ax, word ptr PlayerX
    push ax
    jns loc_731C
    neg ax
loc_731C:
    cmp ax, bx
    jbe loc_7328
    mov ax, 0FFA0h
    mov dx, 30h
    jmp short loc_7338
loc_7328:
    mov dl, 0Eh
    div dl
    xor ah, ah
    mov cl, 4
    shl ax, cl
    mov dx, ax
    shr dx, 1
    neg ax
loc_7338:
    mov word ptr word_6C08, ax
    pop ax
    or ax, ax
    jl loc_7342
    neg dx
loc_7342:
    mov word ptr word_6C06, dx
    jmp near ptr loc_7534
loc_7349:
    cmp word ptr ds:word_6C08, 0
    je loc_7353
    jmp near ptr loc_7534
loc_7353:
    mov byte ptr byte_6C05, 3
    mov word ptr word_A3F7, 0
    mov byte ptr byte_91FE, 1
    jmp near ptr loc_7534
loc_7366:
    cmp byte ptr ds:byte_91FE, 3
    je loc_7370
    jmp near ptr loc_741A
loc_7370:
    call sub_81B4
    inc word ptr word_A3F7
    mov dx, 42h
    mov ax, word ptr word_91FC
    sub ax, 32h
    jns loc_7384
    neg ax
loc_7384:
    cmp ax, 19h
    ja loc_738C
    mov dx, 9Ah
loc_738C:
    cmp word ptr word_A3F7, dx
    ja loc_740C
    mov ax, word ptr word_6C08
    or ax, ax
    je loc_73AD
    jg loc_73A4
    mov word ptr word_A401, 0A45Eh
    jmp near ptr loc_7534
loc_73A4:
    mov word ptr word_A401, 0A464h
    jmp near ptr loc_7534
loc_73AD:
    mov ax, word ptr word_A3FB
    cmp ax, 64h
    jae loc_73F1
    cmp ax, 19h
    jbe loc_73DA
    cmp ax, 23h
    jbe loc_73E3
    cmp byte ptr ds:byte_6C05, 0
    jne loc_73D1
    mov byte ptr byte_91FE, 7
    mov word ptr word_A3F7, 0
loc_73D1:
    mov word ptr word_A401, 0A46Bh
    jmp near ptr loc_7534
loc_73DA:
    mov word ptr word_A401, 0A470h
    jmp near ptr loc_7534
loc_73E3:
    mov byte ptr byte_91FE, 4
    mov word ptr word_A3F7, 0
    jmp near ptr loc_7534
loc_73F1:
    mov word ptr word_6C08, 0FFAFh
    mov dx, 50h
    mov ax, word ptr word_91FF
    cmp word ptr PlayerX, ax
    jg loc_7405
    neg dx
loc_7405:
    mov word ptr word_6C06, dx
    jmp near ptr loc_7534
loc_740C:
    mov word ptr word_A3F7, 0
    mov byte ptr byte_91FE, 2
    jmp near ptr loc_7534
loc_741A:
    cmp byte ptr ds:byte_91FE, 4
    jne loc_7452
    call sub_81B4
    inc word ptr word_A3F7
    cmp word ptr ds:word_A3F7, 42h
    jbe loc_743D
    mov byte ptr byte_91FE, 3
    mov word ptr word_A3F7, 0
    jmp near ptr loc_7534
loc_743D:
    mov word ptr word_A401, 0A475h
    cmp word ptr ds:word_6C08, 0
    jne loc_744F
    mov byte ptr byte_6BEA, 4
loc_744F:
    jmp near ptr loc_7534
loc_7452:
    cmp byte ptr ds:byte_91FE, 5
    jne loc_747A
    inc word ptr word_A3F7
    cmp word ptr ds:word_A3F7, 13h
    jbe loc_7471
    xor ax, ax
    mov word ptr word_A3F7, ax
    mov byte ptr byte_91FE, 1
    jmp near ptr loc_7534
loc_7471:
    mov word ptr word_A401, 0A47Eh
    jmp near ptr loc_7534
loc_747A:
    cmp byte ptr ds:byte_91FE, 6
    jne loc_74EA
    mov word ptr word_A401, 0A480h
    cmp word ptr ds:word_6C08, 0
    jge loc_7491
    jmp near ptr loc_7534
loc_7491:
    mov byte ptr byte_91FE, 0FFh
    mov ax, 0FFFFh
    mov word ptr word_564C, ax
    mov word ptr word_565E, ax
    mov word ptr word_5670, ax
    mov word ptr word_5682, ax
    mov word ptr word_5694, ax
    mov ax, word ptr word_5648
    mov word ptr word_A336, ax
    mov ax, word ptr word_564A
    mov word ptr word_A338, ax
    call sub_94F3
    add word ptr word_A336, 20h
    add word ptr word_A338, 20h
    call sub_94F3
    sub word ptr word_A336, 40h
    call sub_94F3
    sub word ptr word_A336, 20h
    sub word ptr word_A338, 40h
    call sub_94F3
    xor dx, dx
    xor ax, ax
    mov cx, 1
    mov word ptr word_A33A, 63h
    call sub_8D1B
    ret
loc_74EA:
    cmp byte ptr ds:byte_91FE, 7
    jne loc_7534
    inc word ptr word_A3F7
    cmp word ptr ds:word_A3F7, 2Ch
    jbe loc_7503
    mov byte ptr byte_91FE, 1
    jmp short loc_7534
loc_7503:
    mov word ptr word_A401, 0A45Eh
    cmp word ptr ds:word_6C08, 0
    je loc_751A
    jl loc_7534
    mov word ptr word_A401, 0A460h
    jmp short loc_7534
loc_751A:
    mov word ptr word_6C08, 0FF9Fh
    mov dx, 0FFD0h
    mov ax, word ptr word_91FF
    cmp word ptr PlayerX, ax
    jg loc_752E
    neg dx
loc_752E:
    mov word ptr word_6C06, dx
    jmp short loc_7534
loc_7534:
    mov ax, word ptr word_A401
    cmp word ptr word_6C0A, ax
    je loc_7543
    mov word ptr word_6C0A, ax
    mov word ptr word_A3FF, ax
loc_7543:
    mov bx, word ptr word_A3FF
    mov al, byte ptr [bx]
    or al, al
    jns loc_7554
    cbw
    add word ptr word_A3FF, ax
    jmp short loc_7543
loc_7554:
    mov byte ptr byte_A3F9, al
    and al, 0BFh
    inc word ptr word_A3FF
    mov ah, 14h
    mul ah
    mov si, ax
    add si, 0A571h
    mov ax, word ptr word_91FF
    mov word ptr word_A403, ax
    mov ax, word ptr word_9201
    mov word ptr word_A405, ax
    call sub_93B2
    call sub_80DE
loc_7579:
    ret

sub_757A:
    add byte ptr byte_6C05, 1
    sbb byte ptr byte_6C05, 0
    ret

sub_7585:
    mov si, 56A2h
    mov ax, 8
    mov dx, 0AAh
    cmp cx, 8
    jb loc_7596
    mov cx, 8
loc_7596:
    push cx
    jcxz loc_75B1
    mov ax, 0Dh
    call ActivateResourceSlot
loc_759F:
    mov word ptr [si+4], 135h
    mov word ptr [si], ax
    mov word ptr [si+2], dx
    add ax, 5
    add si, 12h
    loop loc_759F
loc_75B1:
    pop ax
    mov cx, 8
    sub cx, ax
    jcxz loc_75C3
loc_75B9:
    mov word ptr [si+4], 0FFFFh
    add si, 12h
    loop loc_75B9
loc_75C3:
    ret

sub_75C4:
    mov bx, word ptr [si+6]
    mov cl, 4
    mov al, byte ptr [si+0Eh]
    or al, al
    jne loc_75E8
    mov al, byte ptr [bx+0Dh]
    cbw
    mov dx, word ptr [si]
    cmp word ptr PlayerX, dx
    jge loc_75DE
    neg ax
loc_75DE:
    shl ax, cl
    mov word ptr [si+8], ax
    inc byte ptr [si+0Eh]
    jmp short loc_760E
loc_75E8:
    cmp al, 1
    jne loc_7607
    test byte ptr [si+5], 20h
    jne loc_7601
    inc byte ptr [bx+7]
    cmp byte ptr [bx+7], 9Ah
    jb loc_760E
    mov byte ptr [si+0Eh], 0FFh
    jmp short loc_760E
loc_7601:
    mov byte ptr [bx+7], 0
    jmp short loc_760E
loc_7607:
    cmp al, 0FFh
    jne loc_760E
    jmp near ptr loc_7CDA
loc_760E:
    ret

sub_760F:
    mov cl, 4
    call sub_8084
    mov al, byte ptr [si+0Eh]
    cmp al, 0
    jne loc_7649
    cmp byte ptr ds:byte_A340, 0
    je loc_7664
    mov byte ptr [si+0Eh], 1
    and byte ptr [bx+4], 0EFh
    mov al, byte ptr [bx+0Dh]
    cbw
    shl ax, cl
    mov dx, word ptr [si]
    cmp word ptr PlayerX, dx
    jg loc_763A
    neg ax
loc_763A:
    mov word ptr [si+8], ax
    mov al, byte ptr [bx+0Eh]
    cbw
    shl ax, cl
    neg ax
    mov word ptr [si+0Ah], ax
    ret
loc_7649:
    cmp al, 1
    jne loc_765D
    mov al, byte ptr [bx+0Fh]
    cbw
    shl ax, cl
    cmp word ptr [si+0Ah], ax
    jge loc_7664
    add word ptr [si+0Ah], 8
    ret
loc_765D:
    cmp al, 0FFh
    jne loc_7664
    jmp near ptr loc_7712
loc_7664:
    ret

sub_7665:
    call sub_8084
    mov al, byte ptr [si+0Eh]
    cmp byte ptr ds:CurrentLevel, 5
    jne loc_7685
    cmp byte ptr ds:byte_6BEA, 0
    je loc_7685
    cmp al, 3
    je loc_7685
    cmp al, 0FFh
    je loc_7685
    mov byte ptr [bx+7], 1
loc_7685:
    cmp al, 0
    jne loc_7698
    or byte ptr [bx+4], 18h
    cmp word ptr [si+0Ah], 0
    jne loc_7697
    mov byte ptr [si+0Eh], 1
loc_7697:
    ret
loc_7698:
    cmp al, 1
    jne loc_76B3
    cmp word ptr [si+0Ah], 0
    je loc_76A7
    mov byte ptr [si+0Eh], 0
    ret
loc_76A7:
    mov byte ptr [bx+7], 1Eh
    mov byte ptr [si+0Eh], 2
    call sub_8058
    ret
loc_76B3:
    cmp al, 2
    jne loc_7703
    mov ax, word ptr [si+8]
    or ax, ax
    jns loc_76C0
    neg ax
loc_76C0:
    cmp ax, 10h
    jae loc_76E1
    cmp byte ptr ds:byte_A340, 0
    je loc_7702
    mov byte ptr [bx+4], 0Fh
    mov dl, byte ptr [bx+0Dh]
    xor dh, dh
    mov ax, word ptr PlayerX
    cmp word ptr [si], ax
    jl loc_76DE
    neg dx
loc_76DE:
    mov word ptr [si+8], dx
loc_76E1:
    test byte ptr SnowFrameTick, 3
    jne loc_7702
    dec byte ptr [bx+7]
    jne loc_7702
    mov byte ptr [si+0Eh], 3
    mov word ptr [si+0Ah], 0
    mov cl, 0Fh
    sar word ptr [si+8], cl
    mov byte ptr [bx+4], 36h
    call sub_8058
loc_7702:
    ret
loc_7703:
    cmp al, 3
    jne loc_7712
    cmp byte ptr ds:byte_A340, 0
    je loc_7711
    jmp near ptr loc_7CFF
loc_7711:
    ret
loc_7712:
    cmp al, 0FFh
    jne loc_773C
    test byte ptr [bx+4], 1
    jne loc_771F
    jmp near ptr loc_7CFF
loc_771F:
    test byte ptr [si+5], 20h
    jne loc_7731
    mov ax, word ptr [si+2]
    cmp word ptr PlayerY, ax
    jge loc_7731
    jmp near ptr loc_7CFF
loc_7731:
    cmp word ptr [si+0Ah], 0F0h
    jge loc_773C
    add word ptr [si+0Ah], 0Fh
loc_773C:
    ret

sub_773D:
    mov bx, word ptr [si+6]
    test byte ptr [si+5], 20h
    jne loc_7785
    cmp byte ptr [si+0Eh], 0FFh
    je loc_7785
    mov ax, word ptr [si+2]
    sub ax, word ptr PlayerY
    jns loc_7757
    neg ax
loc_7757:
    cmp ax, 0BEh
    jae loc_777B
    mov ax, word ptr [bx+0Dh]
    mov dx, word ptr PlayerX
    cmp dx, ax
    jl loc_7773
    mov ax, word ptr [bx+0Fh]
    add ax, 1E0h
    cmp ax, dx
    jg loc_7785
    jmp short loc_777B
loc_7773:
    add dx, 1E0h
    cmp dx, ax
    jge loc_7785
loc_777B:
    mov word ptr [si+4], 0FFFFh
    and byte ptr [bx+4], 0FBh
    ret
loc_7785:
    mov al, byte ptr [si+0Eh]
    or al, al
    jne loc_77AE
    mov al, byte ptr [bx+11h]
    cbw
    mov word ptr [si+8], ax
    add ax, 3
    mov dl, byte ptr [bx+12h]
    xor dh, dh
    cmp ax, dx
    jg loc_77A2
    mov byte ptr [bx+11h], al
loc_77A2:
    mov ax, word ptr [si]
    cmp word ptr [bx+0Fh], ax
    jge loc_77DD
    mov byte ptr [si+0Eh], 1
    ret
loc_77AE:
    cmp al, 1
    jne loc_77D6
    mov al, byte ptr [bx+11h]
    cbw
    mov word ptr [si+8], ax
    sub ax, 3
    mov dl, byte ptr [bx+12h]
    xor dh, dh
    neg dx
    cmp ax, dx
    jl loc_77CA
    mov byte ptr [bx+11h], al
loc_77CA:
    mov ax, word ptr [si]
    cmp word ptr [bx+0Dh], ax
    jl loc_77DD
    mov byte ptr [si+0Eh], 0
    ret
loc_77D6:
    cmp al, 0FFh
    jne loc_77DD
    jmp near ptr loc_7CDA
loc_77DD:
    ret

sub_77DE:
    call sub_8084
    mov cl, 4
    cmp word ptr [si+8], 0
    jne loc_77F8
    mov dx, 1
    mov ax, word ptr PlayerX
    cmp word ptr [si], ax
    jle loc_77F5
    neg dx
loc_77F5:
    mov word ptr [si+8], dx
loc_77F8:
    mov al, byte ptr [si+0Eh]
    or al, al
    jne loc_7857
    call sub_8001
    jb loc_7825
    mov ax, word ptr PlayerX
    sub ax, word ptr [si]
    jns loc_780D
    neg ax
loc_780D:
    sar ax, cl
    cmp byte ptr [bx+0Dh], al
    jb loc_7825
    mov ax, word ptr PlayerY
    sub ax, word ptr [si+2]
    jns loc_781E
    neg ax
loc_781E:
    sar ax, cl
    cmp byte ptr [bx+10h], al
    jae loc_7827
loc_7825:
    jmp short loc_7897
loc_7827:
    mov al, byte ptr [bx+0Eh]
    cbw
    shl ax, cl
    neg ax
    mov word ptr [si+0Ah], ax
    mov al, byte ptr [bx+0Fh]
    cbw
    shl ax, cl
    mov dx, word ptr [si]
    cmp word ptr PlayerX, dx
    jg loc_7842
    neg ax
loc_7842:
    mov word ptr [si+8], ax
    mov byte ptr [si+0Eh], 0Ah
    mov al, byte ptr [bx+4]
    and al, 0D3h
    or al, 2Ch
    mov byte ptr [bx+4], al
    call sub_8058
    ret
loc_7857:
    cmp al, 0Ah
    jne loc_7869
    cmp word ptr [si+0Ah], 0
    jl loc_7897
    mov byte ptr [si+0Eh], 0Bh
    call sub_8058
    ret
loc_7869:
    cmp al, 0Bh
    jne loc_7887
    cmp word ptr [si+0Ah], 0
    jg loc_7897
    mov byte ptr [si+0Eh], 0Ch
    call sub_8048
    call sub_8048
    mov word ptr [si+8], 0
    mov byte ptr [bx+7], 0
    ret
loc_7887:
    cmp al, 0Ch
    jne loc_7890
    call sub_8001
    jae loc_7827
loc_7890:
    cmp al, 0FFh
    jne loc_7897
    jmp near ptr loc_7CDA
loc_7897:
    ret

sub_7898:
    call sub_8084
    mov cl, 4
    mov al, byte ptr [si+0Eh]
    or al, al
    jne loc_78DF
    mov dx, 1
    mov ax, word ptr PlayerX
    cmp word ptr [si], ax
    jle loc_78B0
    neg dx
loc_78B0:
    mov word ptr [si+8], dx
    mov ax, word ptr PlayerX
    sub ax, word ptr [si]
    jns loc_78BC
    neg ax
loc_78BC:
    sar ax, cl
    cmp byte ptr [bx+0Dh], al
    jb loc_78EB
    mov byte ptr [si+0Eh], 0Ah
    mov al, byte ptr [bx+0Eh]
    cbw
    shl ax, cl
    mov word ptr [si+0Ah], ax
    test byte ptr [si+9], 50h
    je loc_78D8
    neg ax
loc_78D8:
    mov word ptr [si+8], ax
    call sub_8058
    ret
loc_78DF:
    cmp al, 0Ah
    jne loc_78E4
    ret
loc_78E4:
    cmp al, 0FFh
    jne loc_78EB
    jmp near ptr loc_7CDA
loc_78EB:
    ret

sub_78EC:
    call sub_8084
    mov cl, 4
    mov al, byte ptr [si+0Eh]
    or al, al
    jne loc_7930
    call sub_7A50
    mov ax, word ptr PlayerX
    sub ax, word ptr [si]
    jns loc_7904
    neg ax
loc_7904:
    sar ax, cl
    cmp byte ptr [bx+0Dh], al
    jae loc_790E
    jmp near ptr loc_7A4F
loc_790E:
    mov byte ptr [si+0Eh], 1
    mov byte ptr [bx+10h], 18h
    mov byte ptr [bx+0Eh], 0
    mov byte ptr [bx+0Fh], 0
    mov cl, 3
    mov ax, word ptr [si]
    shl ax, cl
    mov word ptr [bx+11h], ax
    mov ax, word ptr [si+2]
    shl ax, cl
    mov word ptr [bx+13h], ax
    ret
loc_7930:
    cmp al, 1
    je loc_7937
    jmp near ptr loc_7A48
loc_7937:
    call sub_7A50
    mov ax, word ptr PlayerX
    sub ax, word ptr [si]
    mov bp, ax
    jns loc_7945
    neg ax
loc_7945:
    mul ax
    mov word ptr word_A30E, ax
    mov word ptr word_A310, dx
    xchg cx, ax
    mov ax, word ptr PlayerY
    sub ax, word ptr [si+2]
    jns loc_7959
    neg ax
loc_7959:
    mov cx, ax
    mul ax
    add word ptr word_A30E, ax
    adc word ptr word_A310, dx
    cmp word ptr ds:word_A310, 0
    jne loc_799F
    cmp word ptr ds:word_A30E, 2710h
    jae loc_799F
    cmp byte ptr ds:byte_6BD0, 0
    je loc_799F
    mov al, 40h
    cmp word ptr ds:word_A30E, 0E10h
    jae loc_7987
    mov al, 68h
loc_7987:
    cmp cx, 30h
    jae loc_7998
    or bp, bp
    jle loc_7992
    neg al
loc_7992:
    mov byte ptr [bx+0Eh], al
    jmp near ptr loc_7A28
loc_7998:
    mov byte ptr [bx+0Fh], 0C0h
    jmp near ptr loc_7A28
loc_799F:
    dec byte ptr [bx+10h]
    js loc_79A7
    jmp near ptr loc_7A28
loc_79A7:
    call StepRandomSeed
    and al, 0Fh
    add al, 3
    mov byte ptr [bx+10h], al
    call NextWindRandom
    and ah, 7
    and al, 0Fh
    add al, ah
    mov dx, word ptr [si]
    cmp word ptr PlayerX, dx
    jge loc_79C5
    neg al
loc_79C5:
    mov byte ptr byte_6BC0, al
    call NextWindRandom
    and al, 0Fh
    mov dx, word ptr [si+2]
    add dx, 32h
    cmp word ptr PlayerY, dx
    jge loc_79DB
    neg al
loc_79DB:
    mov byte ptr byte_6BC1, al
    call StepRandomSeed
    and al, 1Fh
    cmp byte ptr ds:byte_6BC0, 0
    jge loc_79EC
    neg al
loc_79EC:
    mov byte ptr [bx+0Eh], al
    cmp byte ptr ds:byte_6BC0, 0
    jne loc_79FA
    mov al, 2
    jmp short loc_7A16
loc_79FA:
    mov ch, byte ptr byte_6BC0
    test ch, ch
    jns loc_7A04
    neg ch
loc_7A04:
    mov al, byte ptr byte_6BC1
    cbw
    test ax, ax
    jns loc_7A0E
    neg ax
loc_7A0E:
    shl ax, 1
    shl ax, 1
    shl ax, 1
    div ch
loc_7A16:
    cmp al, 20h
    jb loc_7A1C
    mov al, 20h
loc_7A1C:
    cmp byte ptr ds:byte_6BC1, 0
    jge loc_7A25
    neg al
loc_7A25:
    mov byte ptr [bx+0Fh], al
loc_7A28:
    mov al, byte ptr [bx+0Eh]
    cbw
    add word ptr [bx+11h], ax
    mov al, byte ptr [bx+0Fh]
    cbw
    add word ptr [bx+13h], ax
    mov cl, 3
    mov ax, word ptr [bx+11h]
    sar ax, cl
    mov word ptr [si], ax
    mov ax, word ptr [bx+13h]
    sar ax, cl
    mov word ptr [si+2], ax
    ret
loc_7A48:
    cmp al, 0FFh
    jne loc_7A4F
    jmp near ptr loc_7CDA
loc_7A4F:
    ret

sub_7A50:
    mov dx, 1
    mov ax, word ptr PlayerX
    cmp word ptr [si], ax
    jle loc_7A5C
    neg dx
loc_7A5C:
    mov word ptr [si+8], dx
    ret

sub_7A60:
    call sub_8084
    mov cl, 4
    mov al, byte ptr [si+0Eh]
    or al, al
    jne loc_7AB7
    mov dx, 1
    mov ax, word ptr PlayerX
    cmp word ptr [si], ax
    jle loc_7A78
    neg dx
loc_7A78:
    mov word ptr [si+8], dx
    mov ax, word ptr PlayerX
    sub ax, word ptr [si]
    jns loc_7A84
    neg ax
loc_7A84:
    sar ax, cl
    cmp byte ptr [bx+0Dh], al
    jb loc_7ADE
    mov ax, word ptr PlayerY
    sub ax, word ptr [si+2]
    jns loc_7A95
    neg ax
loc_7A95:
    sar ax, cl
    cmp byte ptr [bx+0Eh], al
    jb loc_7ADE
    mov byte ptr [si+0Eh], 0Ah
    mov al, byte ptr [bx+0Fh]
    xor ah, ah
    shl ax, cl
    mov word ptr [si+0Ah], ax
    test byte ptr [si+9], 80h
    je loc_7AB2
    neg ax
loc_7AB2:
    mov word ptr [si+8], ax
    jmp short loc_7ADE
loc_7AB7:
    cmp al, 0Ah
    jne loc_7AD7
    mov ax, word ptr [si+2]
    sub ax, word ptr PlayerY
    jns loc_7AC6
    neg ax
loc_7AC6:
    cmp ax, 8
    ja loc_7ADE
    call sub_8058
    mov byte ptr [si+0Eh], 0Bh
    mov word ptr [si+0Ah], 0
loc_7AD7:
    cmp al, 0FFh
    jne loc_7ADE
    jmp near ptr loc_7CDA
loc_7ADE:
    ret

sub_7ADF:
    mov bx, word ptr [si+6]
    mov ax, word ptr [bx+9]
    mov dx, word ptr [bx+0Bh]
    call sub_8089
    mov al, byte ptr [si+0Eh]
    mov cl, 4
    cmp al, 0
    jne loc_7B15
    call sub_8001
    jb loc_7B2F
    mov dx, word ptr [si+2]
    sub dx, word ptr [bx+0Bh]
    push dx
    xor al, al
    call sub_7FD9
    pop dx
    cmp byte ptr [bx+0Dh], dl
    jbe loc_7B10
    add word ptr [si+2], 2
    ret
loc_7B10:
    mov byte ptr [si+0Eh], 1
    ret
loc_7B15:
    cmp al, 1
    jne loc_7B30
    call sub_7B53
    add byte ptr [bx+0Fh], 4
    mov al, byte ptr [bx+0Eh]
    cmp byte ptr [bx+0Fh], al
    jb loc_7B2F
    mov byte ptr [bx+0Fh], al
    mov byte ptr [si+0Eh], 2
loc_7B2F:
    ret
loc_7B30:
    cmp al, 2
    jne loc_7B4B
    call sub_7B53
    mov dl, 1
    cmp byte ptr [bx+0Fh], 0
    js loc_7B41
    neg dl
loc_7B41:
    add byte ptr [bx+10h], dl
    mov al, byte ptr [bx+10h]
    add byte ptr [bx+0Fh], al
    ret
loc_7B4B:
    cmp al, 0FFh
    jne loc_7B52
    jmp near ptr loc_7CDA
loc_7B52:
    ret

sub_7B53:
    mov al, byte ptr [bx+0Fh]
    xor ah, ah
    mov di, ax
    mov al, byte ptr [di+SineTable]
    sar al, 1
    sar al, 1
    mov ch, byte ptr [bx+0Dh]
    imul ch
    sar ax, cl
    add ax, word ptr [bx+9]
    mov word ptr [si], ax
    mov al, byte ptr [bx+0Fh]
    xor ah, ah
    mov di, ax
    mov al, byte ptr [di+byte_7090]
    sar al, 1
    sar al, 1
    imul ch
    sar ax, cl
    add ax, word ptr [bx+0Bh]
    mov word ptr [si+2], ax
    mov al, byte ptr [bx+0Fh]
    mov dl, byte ptr [bx+0Dh]
    call sub_7FD9
    ret

sub_7B91:
    call sub_8084
    mov al, byte ptr [si+0Eh]
    mov cl, 4
    cmp al, 0
    jne loc_7BC5
    call sub_8001
    jb loc_7BC4
    mov ax, word ptr [bx+9]
    sub ax, word ptr PlayerX
    jns loc_7BAD
    neg ax
loc_7BAD:
    sar ax, cl
    cmp byte ptr [bx+0Dh], al
    jb loc_7C2C
    and byte ptr [bx+4], 0EFh
    mov byte ptr [si+0Eh], 1
    mov word ptr [si+0Ah], 20h
    call sub_8058
loc_7BC4:
    ret
loc_7BC5:
    cmp al, 1
    jne loc_7C18
    mov dx, word ptr [si+2]
    sub dx, word ptr [bx+0Bh]
    xor al, al
    call sub_7FD9
    mov ax, word ptr [si+2]
    sar ax, cl
    mov bh, al
    mov ax, word ptr [si]
    sar ax, cl
    mov bl, al
    mov es, word ptr LevelSegment
    mov al, byte ptr es:[bx]
    mov bx, 7F5Eh
    xlat
    or al, al
    je loc_7C2C
    and byte ptr [si+2], 0F0h
    mov word ptr [si+0Ah], 0
    mov byte ptr [si+0Eh], 2
    mov bx, word ptr [si+6]
    or byte ptr [bx+4], 48h
    mov dx, 30h
    mov ax, word ptr [si]
    cmp word ptr PlayerX, ax
    jge loc_7C11
    neg dx
loc_7C11:
    mov word ptr [si+8], dx
    call sub_8058
    ret
loc_7C18:
    cmp al, 2
    jne loc_7C25
    cmp word ptr [si], 0
    jge loc_7C24
    neg word ptr [si+8]
loc_7C24:
    ret
loc_7C25:
    cmp al, 0FFh
    jne loc_7C2C
    jmp near ptr loc_7CDA
loc_7C2C:
    ret

sub_7C2D:
    call sub_8084
    mov cl, 4
    mov al, byte ptr [si+0Eh]
    cmp al, 2
    jae loc_7C85
    push ax
    mov dx, word ptr [si+2]
    sub dx, word ptr [bx+0Bh]
    xor al, al
    call sub_7FD9
    pop ax
    or al, al
    jne loc_7C66
    mov al, byte ptr [bx+0Eh]
    cbw
    shl ax, cl
    mov word ptr [si+0Ah], ax
    mov ax, word ptr [si+2]
    sub ax, word ptr [bx+0Bh]
    cmp byte ptr [bx+0Dh], al
    jge loc_7C8B
    mov byte ptr [si+0Eh], 1
    call sub_8058
    ret
loc_7C66:
    cmp al, 1
    jne loc_7C85
    mov al, byte ptr [bx+0Eh]
    cbw
    shl ax, cl
    neg ax
    mov word ptr [si+0Ah], ax
    mov ax, word ptr [si+2]
    sub ax, word ptr [bx+0Bh]
    jge loc_7C8B
    mov byte ptr [si+0Eh], 0
    call sub_8048
    ret
loc_7C85:
    cmp al, 0FFh
    jne loc_7C8B
    jmp short loc_7CDA
loc_7C8B:
    ret

sub_7C8C:
    call sub_8084
    ret

sub_7C90:
    call sub_8084
    mov al, byte ptr [si+0Eh]
    cmp al, 0
    jne loc_7CB4
    call sub_8001
    jb loc_7CB3
    mov ax, word ptr [si+2]
    sub ax, word ptr PlayerY
    cmp ax, 0B0h
    jge loc_7CFF
    or byte ptr [bx+4], 8
    mov byte ptr [si+0Eh], 1
loc_7CB3:
    ret
loc_7CB4:
    cmp al, 1
    jne loc_7CD5
    cmp word ptr [si+0Ah], 0
    jne loc_7CFE
    mov byte ptr [si+0Eh], 2
    mov dx, 20h
    mov ax, word ptr PlayerX
    cmp word ptr [si], ax
    jl loc_7CCE
    neg dx
loc_7CCE:
    mov word ptr [si+8], dx
    call sub_8058
    ret
loc_7CD5:
    cmp al, 2
    jne loc_7CDA
    ret
loc_7CDA:
    cmp al, 0FFh
    jne loc_7CFE
    test byte ptr [bx+4], 1
    je loc_7CFF
    test byte ptr [si+5], 20h
    jne loc_7CF3
    mov ax, word ptr [si+2]
    cmp word ptr PlayerY, ax
    jl loc_7CFF
loc_7CF3:
    cmp word ptr [si+0Ah], 0F0h
    jge loc_7CFE
    add word ptr [si+0Ah], 0Fh
loc_7CFE:
    ret
loc_7CFF:
    mov byte ptr [bx+7], 0
    mov bx, word ptr [si+6]
    mov word ptr [si+4], 0FFFFh
    and byte ptr [bx+4], 0FBh
    test byte ptr [bx+4], 2
    jne loc_7D1A
    mov word ptr [bx+2], 0FFFFh
loc_7D1A:
    ret

sub_7D1B:
    mov ax, word ptr [si+0Bh]
    cmp word ptr PlayerY, ax
    jle loc_7D6C
    mov ax, word ptr [si+9]
    sub ax, word ptr PlayerX
    jns loc_7D2F
    neg ax
loc_7D2F:
    cmp ax, 280h
    jge loc_7D6C
    cmp ax, 140h
    jge loc_7D49
    mov ax, word ptr PlayerY
    sub ax, word ptr [si+0Bh]
    cmp ax, 168h
    jge loc_7D6C
    cmp ax, 0B4h
    jle loc_7D6C
loc_7D49:
    add byte ptr [si+7], 1
    sbb byte ptr [si+7], 0
    mov al, byte ptr [si+7]
    shr al, 1
    shr al, 1
    cmp byte ptr [si+6], al
    ja loc_7D6C
    call sub_7F31
    jb loc_7D6C
    mov byte ptr [si+4], 8Fh
    mov byte ptr [si+7], 0
    clc
    ret
loc_7D6C:
    stc
    ret

sub_7D6E:
    add byte ptr [si+7], 1
    sbb byte ptr [si+7], 0
    mov al, byte ptr [si+7]
    shr al, 1
    shr al, 1
    cmp byte ptr [si+6], al
    ja loc_7D96
    call sub_7F26
    jb loc_7D96
    mov byte ptr [si+4], 37h
    call NextWindRandom
    and al, 3Fh
    cbw
    sub word ptr [di+2], ax
    clc
    ret
loc_7D96:
    stc
    ret
loc_7D98:
    jmp near ptr loc_7E95

sub_7D9B:
    cmp byte ptr ds:CurrentLevel, 5
    jne loc_7DB0
    cmp byte ptr ds:byte_6BEA, 0
    jne loc_7D98
    cmp word ptr ds:word_A326, 3
    je loc_7D98
loc_7DB0:
    add byte ptr [si+7], 1
    sbb byte ptr [si+7], 0
    mov al, byte ptr [si+7]
    shr al, 1
    shr al, 1
    mov ah, 0
    cmp byte ptr [si+6], al
    ja loc_7D98
    mov cl, 4
    mov ax, word ptr PlayerX
    sar ax, cl
    mov dx, word ptr [si+9]
    sub al, dl
    jb loc_7D98
    cmp byte ptr [si+0Bh], al
    jb loc_7D98
    mov ax, word ptr PlayerY
    sar ax, cl
    sub al, dh
    jb loc_7D98
    cmp byte ptr [si+0Ch], al
    jb loc_7D98
    call sub_806C
    jb loc_7D98
    mov byte ptr [di+10h], 0
    mov word ptr ds:word_A32E, di
    mov bx, word ptr word_A341
    mov ax, word ptr PlayerX
    mov dx, ax
    add ax, word ptr [bx-5CBDh]
    inc bx
    inc bx
    and bx, 0Fh
    mov word ptr word_A341, bx
    mov word ptr [di], ax
    cmp dx, ax
    mov dx, 0
    jge loc_7E15
    not dx
loc_7E15:
    mov word ptr [di+8], dx
    mov cl, 4
    mov ax, word ptr PlayerY
    sar ax, cl
    mov bh, al
    mov ax, word ptr [di]
    sar ax, cl
    mov bl, al
    add bh, 4
    mov bp, bx
    mov bx, 7F5Eh
    mov es, word ptr LevelSegment
    mov dh, byte ptr MapRows
    xor dl, dl
    mov ah, 0Ah
loc_7E3B:
    cmp bp, dx
    jae loc_7E5C
    mov al, byte ptr es:[bp]
    xlat
    or al, al
    je loc_7E5C
    mov al, byte ptr es:[bp-100h]
    xlat
    or al, al
    jne loc_7E5C
    mov al, byte ptr es:[bp-200h]
    xlat
    or al, al
    je loc_7E6C
loc_7E5C:
    sub bp, 100h
    cmp bp, 300h
    jl loc_7E6A
    dec ah
    jne loc_7E3B
loc_7E6A:
    jmp short loc_7E95
loc_7E6C:
    mov ax, bp
    mov al, ah
    xor ah, ah
    shl ax, cl
    mov word ptr [di+2], ax
    mov ax, word ptr [si+2]
    mov word ptr [di+4], ax
    mov word ptr [di+6], si
    mov byte ptr [si+4], 17h
    mov byte ptr [di+0Eh], 0
    mov word ptr [di+0Ah], 0
    mov al, byte ptr [si+5]
    mov byte ptr [di+0Fh], al
    clc
    ret
loc_7E95:
    stc
    ret

sub_7E97:
    mov byte ptr [si+11h], 0
    mov al, byte ptr [si+4]
    push ax
    call sub_7F26
    pop ax
    jb loc_7EBE
    or al, 5
    cmp byte ptr ds:CurrentLevel, 6
    jne loc_7EB0
    or al, 80h
loc_7EB0:
    mov byte ptr [si+4], al
    clc
    ret

sub_7EB5:
    call sub_7F26
    jb loc_7EBE
    mov byte ptr [si+4], 5
loc_7EBE:
    ret

sub_7EBF:
    call sub_7F26
    jb loc_7ED7
    mov byte ptr [si+4], 5
    mov byte ptr [si+0Fh], 0
    mov al, byte ptr [si+0Eh]
    mov byte ptr [si+0Fh], 0
    mov byte ptr [si+10h], 0
loc_7ED7:
    ret

sub_7ED8:
    call sub_7F26
    jb loc_7EE1
    mov byte ptr [si+4], 37h
loc_7EE1:
    ret

sub_7EE2:
    mov ax, word ptr [si+9]
    mov dx, word ptr [si+0Bh]
    call sub_8022
    jb loc_7F25
    call sub_806C
    jb loc_7F25
    mov byte ptr [di+10h], 0
    mov word ptr ds:word_A32E, di
    mov ax, word ptr [si+9]
    mov word ptr [di], ax
    mov ax, word ptr [si+0Bh]
    mov word ptr [di+2], ax
    mov ax, word ptr [si+2]
    mov word ptr [di+4], ax
    mov word ptr [di+6], si
    mov byte ptr [si+4], 5
    xor ax, ax
    mov word ptr [di+0Ah], ax
    mov word ptr [di+8], ax
    mov byte ptr [di+0Eh], 0
    mov al, byte ptr [si+5]
    mov byte ptr [di+0Fh], al
    clc
loc_7F25:
    ret

sub_7F26:
    mov ax, word ptr [si+9]
    mov dx, word ptr [si+0Bh]
    call sub_8022
    jb loc_7F6B

sub_7F31:
    call sub_806C
    jb loc_7F6B
    mov byte ptr [di+10h], 0
    mov word ptr ds:word_A32E, di
    mov ax, word ptr [si+9]
    mov word ptr [di], ax
    mov ax, word ptr [si+0Bh]
    mov word ptr [di+2], ax
    mov ax, word ptr [si+2]
    mov word ptr [di+4], ax
    mov word ptr [di+6], si
    mov byte ptr [si+4], 17h
    mov word ptr [di+8], 0
    mov word ptr [di+0Ah], 0
    mov byte ptr [di+0Eh], 0
    mov al, byte ptr [si+5]
    mov byte ptr [di+0Fh], al
    clc
loc_7F6B:
    ret

sub_7F6C:
    mov cl, 4
    mov ax, word ptr PlayerX
    sar ax, cl
    mov dx, word ptr [si+9]
    sub al, dl
    jb loc_7FD8
    cmp byte ptr [si+0Bh], al
    jb loc_7FD8
    mov ax, word ptr PlayerY
    sar ax, cl
    sub al, dh
    jb loc_7FD8
    cmp byte ptr [si+0Ch], al
    jb loc_7FD8
    call sub_806C
    jb loc_7FD8
    mov byte ptr [di+10h], 0
    mov word ptr ds:word_A32E, di
    mov dx, 0C0h
    xor byte ptr byte_6BCC, 1
    je loc_7FA6
    neg dx
loc_7FA6:
    mov ax, word ptr PlayerX
    add ax, dx
    mov word ptr [di], ax
    mov ax, word ptr PlayerY
    sub ax, 0B0h
    mov word ptr [di+2], ax
    mov ax, word ptr [si+2]
    mov word ptr [di+4], ax
    mov word ptr [di+6], si
    mov byte ptr [si+4], 7
    mov byte ptr [di+0Eh], 0
    mov word ptr [di+0Ah], 0
    mov word ptr [di+8], 0
    mov al, byte ptr [si+5]
    mov byte ptr [di+0Fh], al
    clc
loc_7FD8:
    ret

sub_7FD9:
    sar dl, 1
    sar dl, 1
    mov ah, dl
    mov ch, dl
    mov bp, 3
loc_7FE4:
    call sub_8014
    mov dx, word ptr [bx+9]
    mov word ptr [di], dx
    mov dx, word ptr [bx+0Bh]
    sub dx, 18h
    mov word ptr [di+2], dx
    mov byte ptr [di+5], ch
    mov byte ptr [di+4], al
    add ch, ah
    dec bp
    jne loc_7FE4
    ret

sub_8001:
    add byte ptr [bx+7], 1
    sbb byte ptr [bx+7], 0
    mov al, byte ptr [bx+7]
    shr al, 1
    shr al, 1
    cmp al, byte ptr [bx+6]
    ret

sub_8014:
    mov di, 7DE6h
loc_8017:
    cmp word ptr [di], -1
    je loc_8021
    add di, 6
    jmp short loc_8017
loc_8021:
    ret

sub_8022:
    mov cl, 4
    sar ax, cl
    sub ax, word ptr CameraCol
    cmp ax, 0FFFEh
    jl loc_8046
    cmp ax, 16h
    jg loc_8046
    sar dx, cl
    sub dx, word ptr CameraRow
    cmp dx, -2
    jl loc_8046
    cmp dx, 0Dh
    jg loc_8046
    clc
    ret
loc_8046:
    stc
    ret

sub_8048:
    push bx
    mov bx, word ptr [si+0Ch]
loc_804C:
    dec bx
    dec bx
    cmp word ptr [bx], 0
    jge loc_804C
    mov word ptr [si+0Ch], bx
    pop bx
    ret

sub_8058:
    push bx
    mov bx, word ptr [si+0Ch]
loc_805C:
    cmp word ptr [bx], 0
    jl loc_8065
    inc bx
    inc bx
    jmp short loc_805C
loc_8065:
    inc bx
    inc bx
    mov word ptr [si+0Ch], bx
    pop bx
    ret

sub_806C:
    push cx
    mov di, 4FD0h
    mov cx, 0Ch
loc_8073:
    cmp word ptr [di+4], -1
    je loc_8081
    add di, 12h
    loop loc_8073
    pop cx
    stc
    ret
loc_8081:
    pop cx
    clc
    ret

sub_8084:
    mov ax, word ptr [si]
    mov dx, word ptr [si+2]

sub_8089:
    mov bx, word ptr [si+6]
    cmp byte ptr [si+0Eh], 0FFh
    je loc_80CA
    test byte ptr [si+5], 20h
    jne loc_80CA
    sub ax, word ptr PlayerX
    jns loc_80A0
    neg ax
loc_80A0:
    cmp ax, 140h
    ja loc_80B4
    mov ax, dx
    sub ax, word ptr PlayerY
    jns loc_80AF
    neg ax
loc_80AF:
    cmp ax, 12Ch
    jbe loc_80CA
loc_80B4:
    cmp byte ptr [si+0Eh], 0Ah
    jb loc_80BD
    jmp near ptr loc_7CFF
loc_80BD:
    mov word ptr [si+4], 0FFFFh
    and byte ptr [bx+4], 0FBh
    mov byte ptr [bx+7], 0
loc_80CA:
    ret

sub_80CB:
    push si
    mov si, word ptr [di+0Ch]
loc_80CF:
    inc si
    inc si
    cmp word ptr [si], 7D00h
    jne loc_80CF
    inc si
    inc si
    mov word ptr [di+0Ch], si
    pop si
    ret

sub_80DE:
    mov si, 4F0Ah
    call sub_8182
    jb loc_80F7
    mov si, 4F2Eh
    mov cx, 4
loc_80EC:
    call sub_8182
    jb loc_80F7
    add si, 12h
    loop loc_80EC
    ret
loc_80F7:
    mov ax, word ptr SnowFrameTick
    mov dx, ax
    sub ax, word ptr word_A3FD
    jns loc_8104
    neg ax
loc_8104:
    cmp ax, 16h
    jbe loc_8181
    mov word ptr word_A3FD, dx
    mov dx, 8
    call StartSoundEffect
    or byte ptr byte_564D, 40h
    or byte ptr byte_565F, 40h
    or byte ptr byte_5671, 40h
    or byte ptr byte_5683, 40h
    or byte ptr byte_5695, 40h
    mov byte ptr byte_6BC5, 0
    call sub_757A
    mov al, byte ptr byte_7B19
    xor ah, ah
    sub word ptr word_91FC, ax
    jns loc_814C
    mov byte ptr byte_91FE, 6
    mov word ptr word_6C08, 0FF80h
    jmp short loc_8181
loc_814C:
    cmp al, 14h
    jbe loc_8176
    mov dx, 30h
    mov ax, word ptr PlayerX
    cmp word ptr word_91FF, ax
    jge loc_815E
    neg dx
loc_815E:
    mov word ptr word_6C06, dx
    mov ax, 0FFC0h
    cmp word ptr ds:word_6C08, 0
    jle loc_816E
    shl ax, 1
loc_816E:
    mov word ptr word_6C08, ax
    mov byte ptr byte_91FE, 5
loc_8176:
    mov word ptr word_A3F7, 0
    mov word ptr [si+4], 0FFFFh
loc_8181:
    ret

sub_8182:
    cmp word ptr [si+4], -1
    je loc_81B2
    mov di, word ptr ds:word_A423
    mov ax, word ptr [di+4]
    and ah, 1Fh
    cmp ax, 19Dh
    je loc_819C
    cmp ax, 19Ch
    jne loc_81A8
loc_819C:
    call sub_8D7B
    jae loc_81A8
    mov word ptr [si+4], 0FFFFh
    jmp short loc_81B2
loc_81A8:
    mov di, word ptr ds:word_A425
    call sub_8D7B
    ret

unused_81B0:
    stc
    ret
loc_81B2:
    clc
    ret

sub_81B4:
    cmp byte ptr ds:byte_6BE4, 0
    jne loc_81F2
    mov si, 4F1Ch
    mov di, word ptr ds:word_A421
    call sub_81F3
    mov di, word ptr ds:word_A423
    call sub_81F3
    mov di, word ptr ds:word_A425
    call sub_8D7B
    jae loc_81F2
    cmp byte ptr ds:byte_A330, 0
    je loc_81F2
    mov ax, 0FFC0h
    cmp byte ptr ds:byte_27EA, 0
    je loc_81EF
    mov ax, 0FF80h
    mov dx, 3
    call StartSoundEffect
loc_81EF:
    mov word ptr word_4F2A, ax
loc_81F2:
    ret

sub_81F3:
    cmp word ptr [di+4], -1
    je loc_81FF
    test byte ptr [di+5], 20h
    jne loc_8201
loc_81FF:
    jmp short loc_824C
loc_8201:
    call sub_8D7B
    jae loc_824C
    mov al, 4
    sub al, byte ptr byte_91FB
    sub byte ptr byte_6C05, al
    jns loc_8217
    mov byte ptr byte_6C05, 0
loc_8217:
    call sub_824D
    mov dx, 1
    call StartSoundEffect
    mov byte ptr byte_4F2D, 2Ch
    mov byte ptr byte_6BD0, 0
    mov word ptr word_4F2A, 0FF80h
    mov byte ptr byte_4F24, 3
    mov ax, 80h
    mov dx, word ptr word_91FF
    cmp word ptr PlayerX, dx
    jge loc_8244
    neg ax
loc_8244:
    mov word ptr word_4F22, ax
    mov byte ptr byte_6BC7, 0
loc_824C:
    ret

sub_824D:
    push ax
    push cx
    push dx
    dec byte ptr byte_6BC9
    jns loc_8264
    mov byte ptr byte_6BC9, 5
    dec byte ptr byte_27D6
    jns loc_8264
    call sub_65B3
loc_8264:
    mov ax, word ptr PlayerX
    mov word ptr word_A336, ax
    mov ax, word ptr PlayerY
    sub ax, 30h
    mov word ptr word_A338, ax
    mov cx, 1
    mov ax, 30h
    test byte ptr byte_6BC9, 1
    je loc_8282
    neg ax
loc_8282:
    mov dx, 0FF80h
    mov word ptr word_A33A, 2046h
    call sub_8D1B
    pop dx
    pop cx
    pop ax
    ret
loc_8292:
    jmp near ptr loc_83CE

sub_8295:
    cmp byte ptr ds:byte_4F2D, 0
    je loc_829F
    jmp near ptr loc_83D7
loc_829F:
    mov si, 4F1Ch
    mov di, 4FD0h
    mov cx, 0Ch
loc_82A8:
    cmp word ptr [di+4], -1
    je loc_8292
    test byte ptr [di+5], 20h
    je loc_8292
    mov bx, word ptr [di+6]
    test byte ptr [bx+4], 10h
    jne loc_8292
    cmp byte ptr [di+0Eh], 0FFh
    je loc_8292
    call sub_8D7B
    jae loc_8292
    cmp word ptr ds:word_6BE2, 0
    je loc_82D5
    call sub_8C72
    jmp near ptr loc_83CE
loc_82D5:
    cmp byte ptr ds:byte_A330, 0
    jne loc_82DF
    jmp near ptr loc_838A
loc_82DF:
    cmp byte ptr ds:byte_4F2B, 0
    jge loc_82E9
    jmp near ptr loc_838A
loc_82E9:
    cmp byte ptr ds:byte_6BC7, 0
    je loc_8348
    cmp word ptr ds:word_4F2A, 20h
    jle loc_8343
    mov al, byte ptr [di+10h]
    and al, 3
    mov dl, al
    cbw
    shl ax, 1
    add ax, 52h
    call sub_8875
    or byte ptr [di+5], 40h
    cmp dl, 2
    jne loc_8340
    mov byte ptr [di+0Eh], 0FFh
    call sub_80CB
    mov bx, word ptr [di+6]
    and byte ptr [bx+4], 0F7h
    mov word ptr [di+0Ah], 0FF38h
    mov ax, word ptr word_4F2A
    or ax, ax
    jns loc_832C
    neg ax
loc_832C:
    mov dx, word ptr PlayerX
    cmp word ptr [di], dx
    jg loc_8336
    neg ax
loc_8336:
    mov dx, ax
    shl dx, 1
    add ax, dx
    mov word ptr [di+8], ax
    ret
loc_8340:
    inc byte ptr [di+10h]
loc_8343:
    mov ax, 0FFA0h
    jmp short loc_837A
loc_8348:
    mov dx, 3
    call StartSoundEffect
    mov al, byte ptr [di+10h]
    shr al, 1
    shr al, 1
    cmp al, 0Bh
    je loc_836D
    add byte ptr [di+10h], 4
    inc ax
    mov bx, ax
    shr al, 1
    jb loc_836D
    xor bh, bh
    mov ax, word ptr [bx-5C1Bh]
    call sub_8875
loc_836D:
    mov ax, 0FFC0h
    cmp byte ptr ds:byte_27EA, 0
    je loc_837A
    mov ax, 0FF20h
loc_837A:
    mov word ptr word_4F2A, ax
    mov byte ptr byte_6BD2, 0
    mov ax, word ptr word_A331
    sub word ptr PlayerY, ax
    ret
loc_838A:
    mov dx, 9
    call StartSoundEffect
    mov di, word ptr [di+6]
    and byte ptr [di+4], 0FEh
    mov byte ptr byte_4F2D, 2Ch
    mov byte ptr byte_6BD0, 0
    mov word ptr word_4F2A, 0FF80h
    mov ax, word ptr word_4F22
    shl ax, 1
    shl ax, 1
    neg ax
    mov word ptr word_4F22, ax
    mov byte ptr byte_6BC7, 0
    cmp byte ptr ds:byte_6BC5, 0
    mov byte ptr byte_6BC5, 0
    jne loc_83CD
    dec byte ptr byte_27D6
    jns loc_83CD
    call sub_65B3
loc_83CD:
    ret
loc_83CE:
    add di, 12h
    dec cx
    je loc_83D7
    jmp near ptr loc_82A8
loc_83D7:
    mov si, 50A8h
    mov di, 4F1Ch
    mov cx, 34h
loc_83E0:
    cmp word ptr [si+4], -1
    jne loc_83E9
    jmp near ptr loc_860E
loc_83E9:
    cmp word ptr [si+0Ch], 0BCh
    jle loc_83F3
    jmp near ptr loc_860E
loc_83F3:
    test byte ptr [si+5], 20h
    jne loc_83FC
    jmp near ptr loc_860E
loc_83FC:
    call sub_8D7B
    jb loc_8404
    jmp near ptr loc_860E
loc_8404:
    mov ax, word ptr [si+4]
    and ah, 1Fh
    cmp ax, 0E5h
    jne loc_8412
loc_840F:
    jmp near ptr loc_860E
loc_8412:
    cmp ax, 12Ch
    je loc_840F
    cmp ax, 132h
    je loc_840F
    cmp ax, 134h
    je loc_840F
    cmp ax, 136h
    je loc_840F
    mov ax, 0FFFFh
    xchg word ptr [si+4], ax
    mov word ptr word_A33A, ax
    and ah, 1Fh
    cmp ax, 1CAh
    jne loc_843A
    jmp near ptr loc_8618
loc_843A:
    cmp ax, 1CBh
    jne loc_8442
    jmp near ptr loc_8618
loc_8442:
    sub ax, 35h
    cmp ax, 91h
    jne loc_844D
    jmp near ptr loc_885F
loc_844D:
    cmp word ptr ds:PlayerY, 20h
    jle loc_845C
    cmp ax, 0E2h
    jne loc_845C
    jmp near ptr loc_882A
loc_845C:
    cmp ax, 102h
    jne loc_8464
    jmp near ptr loc_8859
loc_8464:
    cmp ax, 0E4h
    jne loc_846C
    jmp near ptr loc_87FD
loc_846C:
    cmp ax, 0AEh
    jne loc_8474
    jmp near ptr loc_87E6
loc_8474:
    cmp ax, 0Dh
    jne loc_847C
    jmp near ptr loc_87AE
loc_847C:
    cmp ax, 0B6h
    jne loc_8484
    jmp near ptr loc_87BC
loc_8484:
    cmp ax, 2Ch
    jne loc_848C
    jmp near ptr loc_87CA
loc_848C:
    cmp ax, 0E0h
    jne loc_8494
    jmp near ptr loc_87D8
loc_8494:
    cmp ax, 14h
    ja loc_849C
    jmp near ptr loc_85DA
loc_849C:
    cmp ax, 2Ch
    ja loc_84A4
    jmp near ptr loc_8524
loc_84A4:
    cmp ax, 32h
    ja loc_84AC
    jmp near ptr loc_854F
loc_84AC:
    cmp ax, 40h
    ja loc_84B4
    jmp near ptr loc_8582
loc_84B4:
    cmp ax, 4Ah
    jbe loc_850A
    cmp ax, 0A6h
    ja loc_84C1
    jmp near ptr loc_85B0
loc_84C1:
    cmp ax, 0ADh
    je loc_84F6
    cmp ax, 0A7h
    je loc_84D0
    cmp ax, 0A8h
    jne loc_84D3
loc_84D0:
    jmp near ptr loc_864F
loc_84D3:
    cmp ax, 0A9h
    jne loc_84DB
    jmp near ptr loc_86B7
loc_84DB:
    cmp ax, 0AAh
    jne loc_84E3
    jmp near ptr loc_870A
loc_84E3:
    cmp ax, 0B5h
    jne loc_84EB
    jmp near ptr loc_876C
loc_84EB:
    cmp ax, 0B4h
    jne loc_84F3
    jmp near ptr loc_8790
loc_84F3:
    jmp near ptr loc_860E
loc_84F6:
    cmp byte ptr ds:byte_27D6, 3
    jae loc_8509
    inc byte ptr byte_27D6
    mov dx, 4
    call StartSoundEffect
    jmp short loc_853F
loc_8509:
    ret
loc_850A:
    mov dx, 8
    call StartSoundEffect
    cmp byte ptr ds:byte_6BC5, 0
    jne loc_84F3
    mov byte ptr byte_6BC5, 1
    mov ax, 0FFFFh
    mov byte ptr byte_4F27, al
    jmp short loc_853F
loc_8524:
    mov dx, 8
    call StartSoundEffect
    sub ax, 27h
    jb loc_84F3
    cmp al, 4
    ja loc_84F3
    push cx
    mov cl, al
    mov al, 1
    shl al, cl
    or byte ptr byte_6CA7, al
    pop cx
loc_853F:
    mov bx, word ptr [si+9]
    cmp bx, -1
    je loc_854C
    mov word ptr [bx+4], 0FFFFh
loc_854C:
    jmp near ptr loc_860E
loc_854F:
    mov dx, 8
    call StartSoundEffect
    sub ax, 2Dh
    push cx
    push si
    mov cl, al
    mov al, 1
    shl al, cl
    or byte ptr byte_6CA8, al
    cmp cl, 1
    jne loc_857E
    mov si, 8F1Dh
    mov cx, 46h
loc_856F:
    cmp word ptr [si+4], 116h
    jne loc_8579
    inc word ptr [si+4]
loc_8579:
    add si, 7
    loop loc_856F
loc_857E:
    pop si
    pop cx
    jmp short loc_853F
loc_8582:
    mov dx, 4
    call StartSoundEffect
    mov bx, word ptr [si+0Eh]
    cmp bx, 80h
    jl loc_85B6
    neg word ptr [si+0Eh]
    call NextWindRandom
    test al, 1
    mov ax, 20h
    je loc_85A5
    neg ax
    mov byte ptr byte_6BEA, 7
loc_85A5:
    mov word ptr [si+6], ax
    mov ax, word ptr word_A33A
    mov word ptr [si+4], ax
    jmp short loc_860E
loc_85B0:
    mov dx, 8
    call StartSoundEffect
loc_85B6:
    sub ax, 39h
    mov bx, ax
    inc byte ptr [bx+byte_6C12]
    inc word ptr word_6C9E
    mov al, byte ptr [bx-5C8Bh]
    xor ah, ah
    add ax, 4Ah
    mov bx, word ptr [si+9]
    cmp bx, -1
    je loc_85D8
    inc word ptr word_2A7A
loc_85D8:
    jmp short loc_860B
loc_85DA:
    mov dx, 8
    call StartSoundEffect
    mov bx, word ptr [si+9]
    cmp bx, -1
    je loc_85ED
    mov word ptr [bx+4], 0FFFFh
loc_85ED:
    inc byte ptr byte_6BC9
    cmp byte ptr ds:byte_6BC9, 6
    jb loc_860E
    cmp byte ptr ds:byte_27D6, 3
    je loc_860E
    inc byte ptr byte_27D6
    mov byte ptr byte_6BC9, 0
    mov ax, 0E2h
loc_860B:
    call sub_8875
loc_860E:
    add si, 12h
    dec cx
    je loc_8617
    jmp near ptr loc_83E0
loc_8617:
    ret
loc_8618:
    cmp byte ptr ds:byte_27D6, 1
    jae loc_8624
    call sub_65B3
    jmp short loc_860E
loc_8624:
    mov dx, 1
    call StartSoundEffect
    mov byte ptr byte_4F2D, 2Ch
    mov byte ptr byte_4F2C, 0
    mov byte ptr byte_4F27, 8
    dec byte ptr byte_27D6
    mov al, byte ptr byte_27D6
    push ax
    mov byte ptr byte_27D6, 1
    call sub_867E
    pop ax
    mov byte ptr byte_27D6, al
    jmp short loc_860E
loc_864F:
    mov dx, 1
    call StartSoundEffect
    mov byte ptr byte_4F2D, 2Ch
    mov byte ptr byte_4F2C, 0
    mov byte ptr byte_4F27, 8
    call sub_867E
    mov byte ptr byte_6BEA, 7
    mov bx, word ptr [si+9]
    cmp bx, -1
    je loc_8679
    mov word ptr [bx+4], 0FFFFh
loc_8679:
    mov ax, 0E4h
    jmp short loc_860B

sub_867E:
    push cx
    mov ax, word ptr PlayerX
    mov word ptr word_A336, ax
    mov ax, word ptr PlayerY
    sub ax, 30h
    mov word ptr word_A338, ax
    mov al, 6
    mul byte ptr byte_27D6
    mov cx, ax
    add cl, byte ptr byte_6BC9
    je loc_86B5
    mov ax, 30h
    mov dx, 0FF80h
    mov byte ptr byte_27D6, 0
    mov byte ptr byte_6BC9, 0
    mov word ptr word_A33A, 2046h
    call sub_8D1B
loc_86B5:
    pop cx
    ret
loc_86B7:
    mov dx, 0
    call StartSoundEffect
    push ax
    push bx
    push cx
    push dx
    push di
    push si
    push ds
    push es
    push bp
    mov di, 4FD0h
    mov cx, 0Ch
loc_86CC:
    cmp word ptr [di+4], -1
    je loc_86E4
    mov bx, word ptr [di+6]
    test byte ptr [bx+4], 10h
    jne loc_86E4
    test byte ptr [di+5], 20h
    je loc_86E4
    call sub_8C72
loc_86E4:
    add di, 12h
    loop loc_86CC
    mov byte ptr byte_6BEA, 9
    pop bp
    pop es
    pop ds
    pop si
    pop di
    pop dx
    pop cx
    pop bx
    pop ax
    mov bx, word ptr [si+9]
    cmp bx, -1
    je loc_8704
    mov word ptr [bx+4], 0FFFFh
loc_8704:
    mov ax, 0E6h
    jmp near ptr loc_860B
loc_870A:
    mov dx, 0
    call StartSoundEffect
    push ax
    push bx
    push cx
    push dx
    push di
    push si
    push ds
    push es
    push bp
    mov di, 4FD0h
    mov cx, 0Ch
loc_871F:
    cmp word ptr [di+4], -1
    je loc_874B
    mov bx, word ptr [di+6]
    test byte ptr [bx+4], 10h
    jne loc_874B
    test byte ptr [di+5], 20h
    je loc_874B
    mov ax, word ptr [di]
    mov word ptr word_A336, ax
    mov ax, word ptr [di+2]
    mov word ptr word_A338, ax
    mov byte ptr [di+0Eh], 0FFh
    mov word ptr [di+4], 0FFFFh
    call sub_94F3
loc_874B:
    add di, 12h
    loop loc_871F
    pop bp
    pop es
    pop ds
    pop si
    pop di
    pop dx
    pop cx
    pop bx
    pop ax
    mov bx, word ptr [si+9]
    cmp bx, -1
    je loc_8766
    mov word ptr [bx+4], 0FFFFh
loc_8766:
    mov ax, 0E7h
    jmp near ptr loc_860B
loc_876C:
    cmp byte ptr ds:byte_6C04, 1
    je loc_878D
    mov dx, 1
    call StartSoundEffect
    mov byte ptr byte_6C02, 0
    mov byte ptr byte_6C01, 1
    mov byte ptr byte_6C03, 0
    mov byte ptr byte_6C04, 1
loc_878D:
    jmp near ptr loc_853F
loc_8790:
    cmp byte ptr ds:byte_6C04, 0
    je loc_87AB
    mov byte ptr byte_6C01, 0
    mov byte ptr byte_6C02, 1
    mov byte ptr byte_6C03, 0
    mov byte ptr byte_6C04, 0
loc_87AB:
    jmp near ptr loc_853F
loc_87AE:
    mov dx, 8
    call StartSoundEffect
    mov byte ptr byte_7B18, 0
    jmp near ptr loc_853F
loc_87BC:
    mov dx, 8
    call StartSoundEffect
    mov byte ptr byte_7B18, 1
    jmp near ptr loc_853F
loc_87CA:
    mov dx, 8
    call StartSoundEffect
    mov byte ptr byte_7B18, 2
    jmp near ptr loc_853F
loc_87D8:
    mov dx, 8
    call StartSoundEffect
    mov byte ptr byte_7B18, 3
    jmp near ptr loc_853F
loc_87E6:
    mov dx, 4
    call StartSoundEffect
    call sub_65D6
    push si
    mov ax, 0E3h
    mov si, 4F1Ch
    call sub_8875
    pop si
    jmp near ptr loc_853F
loc_87FD:
    mov ax, word ptr PlayerX
    mov word ptr CheckpointX, ax
    mov ax, word ptr PlayerY
    mov word ptr CheckpointY, ax
    mov di, 8F1Dh
    mov cx, 46h
loc_880F:
    cmp word ptr [di+4], 118h
    jne loc_8819
    inc word ptr [di+4]
loc_8819:
    add di, 7
    loop loc_880F
    mov bx, word ptr [si+9]
    cmp bx, -1
    je loc_8829
    dec word ptr [bx+4]
loc_8829:
    ret
loc_882A:
    mov al, byte ptr CurrentLevel
    cmp al, 2
    jne loc_8838
    mov byte ptr CurrentLevel, 0Ch
    jmp short loc_8841
loc_8838:
    cmp al, 0Dh
    jne loc_8841
    mov byte ptr CurrentLevel, 2
loc_8841:
    cmp al, 6
    jne loc_884A
    mov byte ptr CurrentLevel, 0Eh
loc_884A:
    cmp al, 0Fh
    jne loc_8853
    mov byte ptr CurrentLevel, 6
loc_8853:
    mov byte ptr byte_6BE6, 1
    ret
loc_8859:
    mov byte ptr byte_6BE6, 0FFh
    ret
loc_885F:
    push cx
    push si
    mov si, 6EA9h
    mov cx, 14h
loc_8867:
    mov byte ptr [si+7], 7
    add si, 8
    loop loc_8867
    pop si
    pop cx
    jmp near ptr loc_85B0

sub_8875:
    push di
    push cx
    push ax
    push bx
    mov bx, ax
    sub bx, 4Ah
    jb loc_8894
    cmp bx, 10h
    ja loc_8894
    shl bx, 1
    mov bx, word ptr [bx-5CADh]
    add word ptr word_6C0E, bx
    adc word ptr word_6C10, 0
loc_8894:
    mov cx, 10h
    mov di, 5450h
loc_889A:
    cmp word ptr [di+4], -1
    je loc_88A7
    add di, 12h
    loop loc_889A
    jmp short loc_88D2
loc_88A7:
    mov word ptr [di+4], ax
    mov ax, word ptr [si]
    mov word ptr [di], ax
    mov ax, word ptr [si+2]
    mov word ptr [di+2], ax
    mov word ptr [di+0Ch], 2Ch
    mov word ptr ds:word_A33E, di
    cmp si, 50A8h
    jb loc_88D2
    push di
    mov di, word ptr [si+9]
    cmp di, -1
    je loc_88D1
    mov word ptr [di+4], 0FFFFh
loc_88D1:
    pop di
loc_88D2:
    pop bx
    pop ax
    pop cx
    pop di
    ret

sub_88D7:
    mov si, 4F2Eh
    mov cx, 4
    mov byte ptr byte_A312, 1
loc_88E2:
    cmp word ptr [si+4], -1
    je loc_88F0
    call sub_8C21
    jb loc_88F0
    call sub_899E
loc_88F0:
    add si, 12h
    loop loc_88E2
    cmp byte ptr ds:byte_6BC5, 0
    jne loc_891C
    mov si, 4F0Ah
    cmp word ptr [si+4], -1
    je loc_891C
    call sub_8C21
    jb loc_890F
    call sub_899E
    jae loc_891C
loc_890F:
    cmp word ptr ds:word_4F2A, 0
    je loc_891C
    mov word ptr word_4F2A, 0FFB0h
loc_891C:
    mov byte ptr byte_A312, 0
    ret

sub_8922:
    mov di, 52E8h
    mov si, 8F1Dh
    mov dx, 46h
    mov bx, 14h
    mov cl, 4
loc_8930:
    cmp word ptr [si+4], -1
    je loc_898C
    mov ax, word ptr [si]
    sar ax, cl
    sub ax, word ptr CameraCol
    jb loc_898C
    cmp ax, 16h
    jg loc_898C
    mov ax, word ptr [si+2]
    sar ax, cl
    sub ax, word ptr CameraRow
    jb loc_898C
    cmp ax, 2Bh
    jg loc_898C
    mov ax, word ptr [si]
    mov word ptr [di], ax
    xor ax, ax
    test byte ptr SnowFrameTick, 1
    jne loc_8974
    mov al, byte ptr [si+6]
    cbw
    inc ax
    mov byte ptr [si+6], al
    cmp al, 4
    jl loc_8974
    neg ax
    inc ax
    mov byte ptr [si+6], al
loc_8974:
    add ax, word ptr [si+2]
    mov word ptr [si+2], ax
    mov word ptr [di+2], ax
    mov ax, word ptr [si+4]
    mov word ptr [di+4], ax
    mov word ptr [di+9], si
    add di, 12h
    dec bx
    je loc_899D
loc_898C:
    add si, 7
    dec dx
    jne loc_8930
loc_8992:
    mov word ptr [di+4], 0FFFFh
    add di, 12h
    dec bx
    jne loc_8992
loc_899D:
    ret

sub_899E:
    push cx
    mov di, 8C8Dh
    mov cl, 4
    mov ax, word ptr [si]
    sar ax, cl
    mov bp, word ptr [si+2]
    sub bp, 10h
    mov cx, 50h
loc_89B1:
    mov dx, word ptr [di+3]
    cmp dx, -1
    jne loc_89BC
    jmp near ptr loc_8A4E
loc_89BC:
    mov bl, dl
    sub bl, al
    jns loc_89C4
    neg bl
loc_89C4:
    cmp bl, 1
    jbe loc_89CC
    jmp near ptr loc_8A4E
loc_89CC:
    mov bl, dh
    xor bh, bh
    shl bx, 1
    shl bx, 1
    shl bx, 1
    shl bx, 1
    sub bx, bp
    jns loc_89DE
    neg bx
loc_89DE:
    cmp bx, 10h
    jae loc_8A4E
    mov word ptr [si+4], 0FFFFh
    mov dx, word ptr [di+3]
    call sub_8A5A
    jae loc_8A4E
    mov ax, ds
    mov es, ax
    mov cx, 50h
    mov di, 0A2A8h
    mov al, 0
    rep stosb
    mov bp, sp
loc_8A00:
    mov cx, 50h
    mov si, 8C8Dh
loc_8A06:
    mov ax, word ptr [si+3]
    cmp ax, 0FFFFh
    je loc_8A3F
    cmp ax, dx
    je loc_8A3F
    sub al, dl
    jns loc_8A18
    neg al
loc_8A18:
    cmp al, 1
    ja loc_8A3F
    sub ah, dh
    jns loc_8A22
    neg ah
loc_8A22:
    cmp ah, 1
    ja loc_8A3F
    mov di, 50h
    sub di, cx
    cmp byte ptr [di-5D58h], 0
    jne loc_8A3F
    inc byte ptr [di-5D58h]
    push word ptr [si+3]
    mov di, si
    call sub_8B6E
loc_8A3F:
    add si, 5
    loop loc_8A06
    cmp sp, bp
    je loc_8A4B
    pop dx
    jmp short loc_8A00
loc_8A4B:
    pop cx
    stc
    ret
loc_8A4E:
    add di, 5
    dec cx
    je loc_8A57
    jmp near ptr loc_89B1
loc_8A57:
    clc
    pop cx
    ret

sub_8A5A:
    push ax
    push bx
    push cx
    push dx
    push di
    push si
    push ds
    push es
    push bp
    push dx
    mov cl, 4
    mov ax, word ptr [di+3]
    mov dl, ah
    xor dh, dh
    shl dx, cl
    xor ah, ah
    shl ax, cl
    add ax, 8
    add dx, 0Ch
    call sub_5E41
    pop dx
    test byte ptr [di+2], 80h
    je loc_8AB1
    dec byte ptr [di+2]
    jns loc_8A8B
    jmp near ptr loc_8BE8
loc_8A8B:
    push cx
    push dx
    call sub_8BF6
loc_8A90:
    call NextWindRandom
    and ax, 7
    je loc_8A90
    add ax, 6Eh
    mov word ptr word_A33A, ax
    mov ax, 0
    mov dx, 0
    sub word ptr word_A338, 70h
    call sub_8D1B
    pop dx
    pop cx
    jmp near ptr loc_8B77
loc_8AB1:
    mov ax, word ptr SnowFrameTick
    push ax
    sub ax, word ptr word_A33C
    jns loc_8ABD
    neg ax
loc_8ABD:
    cmp ax, 6
    jae loc_8AC6
    pop ax
    jmp near ptr loc_8BEB
loc_8AC6:
    pop word ptr word_A33C
    push dx
    push cx
    call sub_8BF6
    test byte ptr [di+2], 40h
    je loc_8B40
    cmp byte ptr [di+2], 40h
    jne loc_8B19
    mov byte ptr [di+2], 0
    mov cx, 132h
    mov al, byte ptr CurrentLevel
    cmp al, 3
    je loc_8AF7
    mov cx, 12Ch
    cmp al, 7
    je loc_8AF7
    cmp al, 6
    je loc_8AF7
    mov cx, 134h
loc_8AF7:
    mov word ptr word_A33A, cx
    mov ax, 20h
    mov dx, 0FFD0h
    mov cx, 2
    call sub_8D1B
    mov ax, 136h
    cmp byte ptr ds:CurrentLevel, 8
    je loc_8B43
    mov ax, 0E5h
    mov cx, 4
    jmp short loc_8B43
loc_8B19:
    mov cx, 132h
    mov al, byte ptr CurrentLevel
    cmp al, 3
    je loc_8B31
    mov cx, 12Ch
    cmp al, 7
    je loc_8B31
    cmp al, 6
    je loc_8B31
    mov cx, 134h
loc_8B31:
    mov word ptr word_A33A, cx
    mov ax, 30h
    mov dx, 0FFA0h
    mov cx, 4
    jmp short loc_8B61
loc_8B40:
    call sub_8C13
loc_8B43:
    mov word ptr word_A33A, ax
    mov ax, 30h
    cmp si, 4F1Ch
    jae loc_8B56
    cmp word ptr ds:word_4F25, 0
    jmp short loc_8B5A
loc_8B56:
    cmp word ptr [si+6], 0
loc_8B5A:
    jl loc_8B5E
    neg ax
loc_8B5E:
    mov dx, 0FF90h
loc_8B61:
    call sub_8D1B
    pop cx
    pop dx
    sub byte ptr [di+2], 1
    jb loc_8B77
    jmp short loc_8BEB

sub_8B6E:
    push ax
    push bx
    push cx
    push dx
    push di
    push si
    push ds
    push es
    push bp
loc_8B77:
    inc word ptr word_2A76
    mov ax, 0FFFFh
    xchg word ptr [di+3], ax
    mov bl, byte ptr [di+1]
    mov es, word ptr LevelSegment
    mov di, ax
    mov byte ptr es:[di], bl
    sub al, byte ptr CameraCol
    jb loc_8BE8
    cmp al, 14h
    jae loc_8BE8
    sub ah, byte ptr CameraRow
    jb loc_8BE8
    cmp ah, 0Ch
    jae loc_8BE8
    push bx
    call SetLatchCopyMode
    mov bl, 28h
    mov bh, 1
    mov ax, di
    mov al, ah
    xor ah, ah
    mov dl, 0Ch
    div dl
    mov al, bl
    mul ah
    shl ax, 1
    shl ax, 1
    shl ax, 1
    shl ax, 1
    push ax
    mov ax, di
    xor ah, ah
    mov dl, 14h
    div dl
    mov al, ah
    xor ah, ah
    mov cl, bh
    shl ax, cl
    pop di
    add di, ax
    pop ax
    call BlitTileToRing
    mov byte ptr AnimVisibleFlag, 1
    mov byte ptr GridDirtyFlag, 1
    mov word ptr PrevCameraCol, 55AAh
loc_8BE8:
    stc
    jmp short loc_8BEC
loc_8BEB:
    clc
loc_8BEC:
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

sub_8BF6:
    mov ax, word ptr [di+3]
    mov cl, 4
    mov dl, al
    xor dh, dh
    shl dx, cl
    mov word ptr word_A336, dx
    mov dl, ah
    xor dh, dh
    shl dx, cl
    mov word ptr word_A338, dx
    mov cx, 1
    ret

sub_8C13:
    call NextWindRandom
    and ax, 7Fh
    cmp al, 5Fh
    jae sub_8C13
    add ax, 2080h
    ret

sub_8C21:
    mov di, 4FD0h
    mov dx, 0Ch
loc_8C27:
    cmp word ptr [di+4], -1
    je loc_8C68
    cmp byte ptr [di+0Eh], 0FFh
    je loc_8C68
    mov bx, word ptr [di+6]
    test byte ptr [bx+4], 10h
    jne loc_8C68
    call sub_8D7B
    jae loc_8C68
    or byte ptr [di+5], 40h
    mov al, byte ptr byte_7B19
    sub byte ptr [di+0Fh], al
    jae loc_8C58
    mov dx, 2
    call StartSoundEffect
    call sub_8C72
    jmp short loc_8C61
loc_8C58:
    mov ax, word ptr [di+8]
    sar ax, 1
    sar ax, 1
    sub word ptr [di], ax
loc_8C61:
    mov word ptr [si+4], 0FFFFh
    stc
    ret
loc_8C68:
    add di, 12h
    dec dx
    je loc_8C70
    jmp short loc_8C27
loc_8C70:
    clc
    ret

sub_8C72:
    mov al, byte ptr [bx+8]
    cbw
    add ax, 4Ah
    push bx
    mov bl, byte ptr [di+10h]
    shr bl, 1
    shr bl, 1
    shr bl, 1
    and bx, 7
    mov dl, byte ptr [bx-5C0Fh]
    xor dh, dh
    pop bx
    push si
    mov si, di
    push word ptr [si]
    push word ptr [si+2]
    push di
loc_8C96:
    call sub_8875
    mov di, word ptr ds:word_A33E
    push dx
    shl dx, 1
    shl dx, 1
    sub word ptr [di+0Ch], dx
    pop dx
    add word ptr [si+2], 7
    add word ptr [si], 9
    dec dx
    jne loc_8C96
    pop di
    pop word ptr [si+2]
    pop word ptr [si]
    pop si
    mov byte ptr [di+0Eh], 0FFh
    test byte ptr [bx+4], 1
    jne loc_8CE1
    push cx
    mov ax, word ptr [di]
    mov word ptr word_A336, ax
    mov ax, word ptr [di+2]
    mov word ptr word_A338, ax
    mov cx, 6
    mov ax, 30h
    mov dx, 0FF80h
    mov word ptr word_A33A, 2046h
    call sub_8D1B
    pop cx
    ret
loc_8CE1:
    call sub_80CB
    mov al, byte ptr [bx+4]
    mov ah, al
    and ah, 0F7h
    and al, 0C8h
    cmp al, 88h
    je loc_8CF5
    mov byte ptr [bx+4], ah
loc_8CF5:
    mov al, byte ptr byte_7B19
    xor ah, ah
    cmp ax, 19h
    jbe loc_8D02
    mov ax, 19h
loc_8D02:
    neg ax
    shl ax, 1
    shl ax, 1
    shl ax, 1
    mov word ptr [di+0Ah], ax
    sar ax, 1
    test byte ptr [si+5], 80h
    jne loc_8D17
    neg ax
loc_8D17:
    mov word ptr [di+8], ax
    ret

sub_8D1B:
    push ax
    push bx
    push cx
    push di
    push bp
    xor di, di
    mov bx, 50A8h
loc_8D25:
    cmp word ptr [bx+4], -1
    jne loc_8D6C
    mov bp, word ptr word_A33A
    mov word ptr [bx+4], bp
    mov byte ptr [bx+11h], 0
    mov word ptr [bx+0Ch], 0C6h
    mov bp, word ptr word_A336
    mov word ptr [bx], bp
    mov bp, word ptr word_A338
    mov word ptr [bx+2], bp
    mov word ptr [bx+6], ax
    mov word ptr [bx+0Eh], dx
    mov word ptr [bx+9], 0FFFFh
    neg ax
    inc di
    test di, 1
    jne loc_8D69
    cmp di, 0Ch
    jbe loc_8D63
    xor ax, ax
loc_8D63:
    sub ax, 10h
    sub dx, 10h
loc_8D69:
    dec cx
    je loc_8D75
loc_8D6C:
    add bx, 12h
    cmp bx, 52E8h
    jb loc_8D25
loc_8D75:
    pop bp
    pop di
    pop cx
    pop bx
    pop ax
    ret

sub_8D7B:
    push ax
    push bx
    push dx
    push si
    push di
    push bp
    mov byte ptr byte_A330, 0
    mov ax, word ptr [si]
    sub ax, word ptr [di]
    jns loc_8D8E
    neg ax
loc_8D8E:
    cmp ax, 40h
    jb loc_8D96
    jmp near ptr loc_8E3D
loc_8D96:
    mov ax, word ptr [si+2]
    sub ax, word ptr [di+2]
    jns loc_8DA0
    neg ax
loc_8DA0:
    cmp ax, 46h
    jb loc_8DA8
    jmp near ptr loc_8E3D
loc_8DA8:
    mov ax, word ptr [si+2]
    mov dx, word ptr [di+2]
    mov bx, word ptr [si+4]
    cmp ax, dx
    jge loc_8DBB
    mov bx, word ptr [di+4]
    xchg dx, ax
    xchg si, di
loc_8DBB:
    and bh, 1Fh
    shl bx, 1
    mov bl, byte ptr [bx+FrameHeights]
    xor bh, bh
    sub ax, bx
    cmp ax, dx
    jge loc_8E3D
    cmp byte ptr ds:byte_A312, 0
    jne loc_8DF1
    sub dx, ax
    cmp word ptr ds:word_4F2A, 80h
    jge loc_8DE9
    shr bx, 1
    cmp dx, bx
    ja loc_8DF1
    cmp si, 4F1Ch
    je loc_8DF1
loc_8DE9:
    inc byte ptr byte_A330
    mov word ptr word_A331, dx
loc_8DF1:
    mov bx, word ptr [si+4]
    and bh, 1Fh
    shl bx, 1
    mov bp, bx
    mov al, byte ptr [bx+FrameHotX]
    xor ah, ah
    mov dx, word ptr [si]
    sub dx, ax
    mov bx, word ptr [di+4]
    and bh, 1Fh
    shl bx, 1
    mov ax, word ptr [di]
    sub al, byte ptr [bx+FrameHotX]
    sbb ah, 0
    mov bl, byte ptr [bx+FrameWidths]
    cmp ax, dx
    jl loc_8E24
    xchg dx, ax
    mov bl, byte ptr ds:[bp+FrameWidths]
loc_8E24:
    xor bh, bh
    cmp byte ptr ds:byte_A312, 0
    jne loc_8E2F
    sar bx, 1
loc_8E2F:
    add ax, bx
    cmp ax, dx
    jle loc_8E3D
    pop bp
    pop di
    pop si
    pop dx
    pop bx
    pop ax
    stc
    ret
loc_8E3D:
    pop bp
    pop di
    pop si
    pop dx
    pop bx
    pop ax
    clc
    ret

sub_8E45:
    mov ax, seg DATA
    mov ds, ax
    xor ax, ax
    mov byte ptr byte_2879, al
    mov word ptr word_287A, ax
    mov byte ptr byte_287C, al
    mov byte ptr byte_287D, al
    mov cx, 8
    call PageInResource
    mov word ptr LevelSegment, ax
    mov ds, word ptr LevelSegment
    xor si, si
    mov cl, 0A0h
    call sub_919F
    mov ax, seg DATA
    mov ds, ax
    xor ax, ax
    mov word ptr word_27F0, ax
    mov word ptr word_27F2, ax
    xor al, al
loc_8E7B:
    cmp byte ptr byte_27F6, al
    jne loc_8EC9
    cmp byte ptr byte_27F7, al
    jne loc_8EDE
    mov al, byte ptr byte_282D
    or al, byte ptr byte_2810
    jne loc_8EC9
    cmp word ptr ds:word_27F0, 10Eh
    jb loc_8E7B
    call sub_9286
    mov cx, 0Ch
    call PageInResource
    mov word ptr LevelSegment, ax
    mov ds, ax
    xor si, si
    call sub_9155
    mov ax, seg DATA
    mov ds, ax
    call sub_8F2D
    call sub_9286
    mov byte ptr byte_2879, 1
    mov al, byte ptr ds:byte_083E
    mov byte ptr CurrentLevel, al
    mov al, byte ptr byte_083D
    mov byte ptr byte_B198, al
    jmp short loc_8F29
loc_8EC9:
    mov byte ptr CurrentLevel, 0
    call SetLevelVideoMode
    mov dx, 991Fh
    call RunCallbackMapScreen
    mov byte ptr CurrentLevel, 0
    jmp short loc_8EFD
loc_8EDE:
    mov byte ptr CurrentLevel, 0
    call SetLevelVideoMode
    mov dx, 9985h
    mov byte ptr CurrentLevel, 0FFh
    call RunCallbackMapScreen
    cmp byte ptr ds:CurrentLevel, 0
    jge loc_8EFB
    jmp near ptr sub_8E45
loc_8EFB:
    jmp short loc_8EFD
loc_8EFD:
    mov cx, 400h
    mov di, 3Fh
    mov ax, seg DATA
    mov es, ax
    mov ax, 55AAh
    rep stosw
    dec di
    mov al, byte ptr CurrentLevel
    stosb
    mov al, byte ptr ExpertFlag
    mov byte ptr byte_B198, al
    mov byte ptr byte_083D, al
    cmp byte ptr cs:CpuClass, 3
    jb loc_8F29
    mov word ptr word_78C4, 78C6h
loc_8F29:
    call sub_9286
    ret

sub_8F2D:
    call SetLatchCopyMode
    mov ax, 0A000h
    mov ds, ax
    mov es, ax
    xor si, si
    mov di, 3F40h
    mov cx, 1F40h
    rep movsb
    mov ax, seg DATA
    mov ds, ax
    mov byte ptr byte_6C01, 0
    mov byte ptr byte_6C02, 1
    mov byte ptr byte_6C03, 0
    mov byte ptr FineScroll, 0
    xor ax, ax
    mov word ptr CameraCol, ax
    mov word ptr CameraRow, ax
    mov word ptr PlayerX, ax
    mov word ptr PlayerY, 0A9h
    mov word ptr word_4F25, ax
    mov word ptr word_4F22, ax
    mov word ptr word_4F28, 7BA7h
    mov word ptr word_4F2E, 30h
    mov word ptr word_4F30, 0A9h
    mov word ptr word_4F37, ax
    mov word ptr word_4F34, ax
    mov word ptr word_4F3A, 0AB4Bh
    mov byte ptr byte_6BC5, 0
    xor bp, bp
loc_8F96:
    inc bp
    mov bx, word ptr word_4F28
    call sub_638B
    mov si, 4F2Eh
    call sub_9047
    call sub_9071
    call DrawSpriteObjects
    call PresentFrame
    mov ax, 5
    add word ptr PlayerX, ax
    test bp, 3
    jne loc_8FBB
    dec ax
loc_8FBB:
    add word ptr word_4F2E, ax
    test byte ptr byte_4F21, 20h
    jne loc_8F96
    push ds
    pop es
    mov word ptr word_4F37, 0FFFFh
    mov word ptr word_4F3C, 0
    mov word ptr word_4F25, 0FFFFh
    mov si, 4F2Eh
    lea di, [si+12h]
    mov cx, 3Fh
    rep movsw
    mov si, 4F2Eh
    mov cx, 3
    mov ax, word ptr PlayerX
    mov dx, word ptr PlayerY
loc_8FF2:
    add ax, 1Eh
    sub dx, 3
    mov word ptr [si], ax
    mov word ptr [si+2], dx
    add si, 12h
    loop loc_8FF2
loc_9002:
    mov bx, word ptr word_4F28
    call sub_638B
    mov cx, 3
    mov si, 4F2Eh
loc_900F:
    sub word ptr [si], 5
    call sub_9047
    cmp word ptr [si+2], 0A9h
    jbe loc_901F
    neg word ptr [si+0Eh]
loc_901F:
    add word ptr [si+0Eh], 8
    mov ax, word ptr [si+0Eh]
    sar ax, 1
    sar ax, 1
    sar ax, 1
    sar ax, 1
    add word ptr [si+2], ax
    add si, 12h
    loop loc_900F
    call sub_9071
    call DrawSpriteObjects
    call PresentFrame
    sub word ptr PlayerX, 7
    jns loc_9002
    ret

sub_9047:
    push ax
    push bx
    push dx
    mov bx, word ptr [si+0Ch]
    mov ax, word ptr [bx]
    or ax, ax
    jns loc_9057
    add bx, ax
    mov ax, word ptr [bx]
loc_9057:
    and ah, 1Fh
    add ax, 138h
    mov dh, byte ptr [si+9]
    and dh, 80h
    or ah, dh
    mov word ptr [si+4], ax
    inc bx
    inc bx
    mov word ptr [si+0Ch], bx
    pop dx
    pop bx
    pop ax
    ret

sub_9071:
    push ax
    push cx
    push si
    push di
    call SetLatchCopyMode
    mov si, 3F40h
    mov di, word ptr ds:DrawPage
    mov ax, 0A000h
    mov ds, ax
    mov es, ax
    mov cx, 1F40h
    rep movsb
    pop di
    pop si
    pop cx
    pop ax
    ret

sub_9090:
    mov ax, seg DATA
    mov ds, ax
    push ds
    mov dx, 0A2F8h
    push ds
    push es
    call LoadSqzFile
    pop es
    pop ds
    mov word ptr NextFreeSegment, ax
    mov word ptr LevelSegment, ax
    mov cl, 0FFh
    call sub_919F
    pop ds
    mov ax, word ptr NextFreeSegment
    add ax, 1030h
    mov ds, ax
    mov ax, 0A000h
    mov es, ax
    xor di, di
    xor si, si
    mov cx, 3A20h
    rep movsw
    mov ax, seg DATA
    mov ds, ax
    mov es, word ptr NextFreeSegment
    xor di, di
    mov dx, 3C7h
    mov al, 0
    out dx, al
    mov dl, 0C9h
    mov cx, 300h
loc_90D8:
    in al, dx
    and al, 3Fh
    stosb
    loop loc_90D8
loc_90DE:
    mov dl, 0C8h
    mov al, 0
    out dx, al
    inc dx
    mov si, 0ACE7h
    xor di, di
    xor bp, bp
    mov cx, 300h
loc_90EE:
    xor ah, ah
    mov bh, byte ptr [si]
    mov al, byte ptr es:[di]
    sub bh, al
    je loc_9110
    jns loc_90FD
    neg bh
loc_90FD:
    mov ah, 2
    cmp bh, ah
    jae loc_9108
    inc bp
    mov al, byte ptr [si]
    jmp short loc_9110
loc_9108:
    cmp byte ptr [si], al
    ja loc_910E
    neg ah
loc_910E:
    add al, ah
loc_9110:
    stosb
    inc si
    out dx, al
    inc bl
    cmp bl, 52h
    jb loc_911F
    call WaitVerticalRetrace
    xor bl, bl
loc_911F:
    loop loc_90EE
    or bp, bp
    jne loc_90DE
    mov ax, seg DATA
    mov ds, ax
    ret

sub_912B:
    mov ax, seg DATA
    mov ds, ax
    push ds
    mov dx, 0A304h
    push ds
    push es
    call LoadSqzFile
    pop es
    pop ds
    mov word ptr NextFreeSegment, ax
    mov word ptr LevelSegment, ax
    mov cl, 10h
    call sub_919F
    mov cx, 46h
loc_9149:
    call WaitVerticalRetrace
    loop loc_9149
    call sub_9286
    pop ds
    ret
cvar_9153 label byte
; DATA: bound and index bytes
    db 00h
cvar_9154 label byte
; UNKNOWN: not yet classified as code or data
    db 00h

sub_9155:
    push ds
    call SetLevelVideoMode
    call WaitVerticalRetrace
    mov dx, 3C8h
    mov al, 0
    out dx, al
    inc dx
    mov cx, 300h
loc_9166:
    out dx, al
    loop loc_9166
    mov ax, 0A000h
    mov es, ax
    mov dx, 3C4h
    mov al, 2
    out dx, al
    inc dx
    mov al, 1
loc_9177:
    out dx, al
    mov cx, 0FA0h
    xor di, di
    rep movsw
    shl al, 1
    cmp al, 10h
    jb loc_9177
    xor bp, bp
    lea di, [si+30h]
    mov ax, ds
    mov es, ax
    mov al, 0
    mov cx, 2D0h
    rep stosb
    mov si, ds
    add si, 7D0h
    mov ds, si
    jmp short loc_91CF

sub_919F:
    mov byte ptr cs:cvar_9153, cl
    push ds
    mov ax, 13h
    int 10h
    call WaitVerticalRetrace
    mov dx, 3C8h
    mov al, 0
    out dx, al
    inc dx
    mov cx, 300h
loc_91B7:
    out dx, al
    loop loc_91B7
    call sub_926E
    mov cx, 7D00h
    rep movsw
    call WaitVerticalRetrace
    call EnableVideoOutput
    mov si, ds
    sub si, 30h
    mov ds, si
loc_91CF:
    mov byte ptr cs:cvar_9154, 0
    xor si, si
    xor bp, bp
loc_91D9:
    test byte ptr cs:cvar_9154, 1Fh
    jne loc_91E4
    call WaitVerticalRetrace
loc_91E4:
    test word ptr cs:OptionBits, 1
    je loc_925E
    mov dx, 3C7h
    mov al, byte ptr cs:cvar_9154
    out dx, al
    mov dl, 0C9h
    jmp short loc_91F9
loc_91F9:
    in al, dx
    and al, 3Fh
    mov ah, al
    jmp short loc_9200
loc_9200:
    in al, dx
    and al, 3Fh
    mov bl, al
    jmp short loc_9207
loc_9207:
    in al, dx
    and al, 3Fh
    mov bh, al
    mov dl, 0C8h
    mov al, byte ptr cs:cvar_9154
    out dx, al
    inc dx
    mov al, ah
    cmp byte ptr [si], al
    jbe loc_9221
    cmp al, 3Dh
    ja loc_9221
    add al, 2
    inc bp
loc_9221:
    out dx, al
    mov al, bl
    cmp byte ptr [si+1], al
    jbe loc_9230
    cmp al, 3Dh
    ja loc_9230
    add al, 2
    inc bp
loc_9230:
    out dx, al
    mov al, bh
    cmp byte ptr [si+2], al
    jbe loc_923F
    cmp al, 3Dh
    ja loc_923F
    add al, 2
    inc bp
loc_923F:
    out dx, al
    add si, 3
    inc byte ptr cs:cvar_9154
    mov al, byte ptr cs:cvar_9153
    cmp byte ptr cs:cvar_9154, al
    jae loc_9255
    jmp short loc_91D9
loc_9255:
    or bp, bp
    je loc_925C
    jmp near ptr loc_91CF
loc_925C:
    jmp short loc_926C
loc_925E:
    mov dx, 3C8h
    mov al, 0
    out dx, al
    inc dx
    mov cx, 1E0h
loc_9268:
    lodsb
    out dx, al
    loop loc_9268
loc_926C:
    pop ds
    ret

sub_926E:
    mov ax, seg DATA
    mov ds, ax
    mov si, word ptr LevelSegment
    add si, 30h
    mov ds, si
    xor si, si
    mov ax, 0A000h
    mov es, ax
    xor di, di
    ret

sub_9286:
    test word ptr cs:OptionBits, 1
    jne loc_9292
    jmp near ptr loc_932E
loc_9292:
    mov al, byte ptr cs:OptionVByte
    cmp al, 5
    je loc_929B
    ret
loc_929B:
    mov byte ptr cs:cvar_9154, 0
    xor bp, bp
loc_92A3:
    test byte ptr cs:cvar_9154, 1Fh
    jne loc_92AE
    call WaitVerticalRetrace
loc_92AE:
    mov dx, 3C7h
    mov al, byte ptr cs:cvar_9154
    out dx, al
    mov dl, 0C9h
    jmp short loc_92BA
loc_92BA:
    in al, dx
    and al, 3Fh
    mov ah, al
    jmp short loc_92C1
loc_92C1:
    in al, dx
    and al, 3Fh
    mov bl, al
    jmp short loc_92C8
loc_92C8:
    in al, dx
    and al, 3Fh
    mov bh, al
    mov dl, 0C8h
    mov al, byte ptr cs:cvar_9154
    out dx, al
    inc dx
    mov al, ah
    sub al, 1
    adc al, 0
    sub al, 1
    adc al, 0
    sub al, 1
    adc al, 0
    sub al, 1
    adc al, 0
    or bp, ax
    out dx, al
    mov al, bl
    sub al, 1
    adc al, 0
    sub al, 1
    adc al, 0
    sub al, 1
    adc al, 0
    sub al, 1
    adc al, 0
    or bp, ax
    out dx, al
    mov al, bh
    sub al, 1
    adc al, 0
    sub al, 1
    adc al, 0
    sub al, 1
    adc al, 0
    sub al, 1
    adc al, 0
    or bp, ax
    out dx, al
    inc byte ptr cs:cvar_9154
    mov al, byte ptr cs:cvar_9153
    cmp byte ptr cs:cvar_9154, al
    jae loc_9327
    jmp near ptr loc_92A3
loc_9327:
    or bp, bp
    je loc_932E
    jmp near ptr sub_9286
loc_932E:
    ret

sub_932F:
    push bx
    push cx
    push dx
    push di
    push si
    push ds
    push es
    push ax
    mov dx, seg DATA
    mov ds, dx
    cmp byte ptr ds:byte_A335, 0
    jne loc_939B
    xor dx, dx
    mov ax, 0F000h
    mov es, ax
    mov si, 0FFF0h
loc_934D:
    add dl, byte ptr es:[si]
    sbb dh, byte ptr es:[si]
    inc si
    jne loc_934D
    mov ax, 0C000h
loc_9359:
    mov es, ax
    cmp word ptr es:[0], 0AA55h
    jne loc_9384
    xor si, si
    mov cx, 80h
loc_9369:
    add dl, byte ptr es:[si]
    xor dh, byte ptr es:[si]
    inc si
    loop loc_9369
    mov bh, byte ptr es:[2]
    xor bl, bl
    shr bx, 1
    shr bx, 1
    shr bx, 1
    add ax, bx
    sub ah, 4
loc_9384:
    add ah, 4
    cmp ah, 0F0h
    jb loc_9359
    or dx, dx
    jne loc_9392
    mov dl, 20h
loc_9392:
    mov word ptr word_A333, dx
    mov byte ptr byte_A335, 1
loc_939B:
    pop ax
    xor ax, 55A3h
    mul word ptr word_A333
    mov cl, byte ptr cs:CpuClass
    rol ax, cl
    pop es
    pop ds
    pop si
    pop di
    pop dx
    pop cx
    pop bx
    ret

sub_93B2:
    mov ax, seg DATA
    mov ds, ax
    mov es, ax
    call sub_93F6
    mov di, 5648h
    mov dx, 5
loc_93C2:
    mov bx, word ptr [si]
    cmp bx, 0A415h
    jne loc_93CE
    mov word ptr ds:word_A423, di
loc_93CE:
    cmp bx, 0A41Bh
    jne loc_93D8
    mov word ptr ds:word_A421, di
loc_93D8:
    cmp bx, 0A40Fh
    jne loc_93E2
    mov word ptr ds:word_A425, di
loc_93E2:
    inc si
    inc si
    mov ax, word ptr [bx]
    stosw
    mov ax, word ptr [bx+2]
    stosw
    mov ax, word ptr [bx+4]
    stosw
    add di, 0Ch
    dec dx
    jne loc_93C2
    ret

sub_93F6:
    push ax
    push bx
    push dx
    lodsw
    mov word ptr word_A407, ax
    mov dx, ax
    lodsw
    mov word ptr word_A40D, ax
    call sub_94DC
    mov dl, ah
    cbw
    cmp byte ptr ds:byte_A3FA, 0
    je loc_9412
    neg ax
loc_9412:
    add ax, word ptr word_A403
    mov word ptr word_A409, ax
    mov al, dl
    cbw
    add ax, word ptr word_A405
    mov word ptr word_A40B, ax
    mov dx, word ptr word_A40D
    lodsw
    mov word ptr word_A413, ax
    call sub_94DC
    mov dl, ah
    cbw
    cmp byte ptr ds:byte_A3FA, 0
    je loc_943A
    neg ax
loc_943A:
    add ax, word ptr word_A409
    mov word ptr word_A40F, ax
    mov al, dl
    cbw
    add ax, word ptr word_A40B
    mov word ptr word_A411, ax
    mov dx, word ptr word_A40D
    lodsw
    mov word ptr word_A419, ax
    call sub_94DC
    mov dl, ah
    cbw
    cmp byte ptr ds:byte_A3FA, 0
    je loc_9462
    neg ax
loc_9462:
    add ax, word ptr word_A409
    mov word ptr word_A415, ax
    mov al, dl
    cbw
    add ax, word ptr word_A40B
    mov word ptr word_A417, ax
    mov dx, word ptr word_A40D
    lodsw
    mov word ptr word_A41F, ax
    call sub_94DC
    mov dl, ah
    cbw
    cmp byte ptr ds:byte_A3FA, 0
    je loc_948A
    neg ax
loc_948A:
    add ax, word ptr word_A409
    mov word ptr word_A41B, ax
    mov al, dl
    cbw
    add ax, word ptr word_A40B
    mov word ptr word_A41D, ax
    test byte ptr byte_A3F9, 40h
    je loc_94B3
    add word ptr word_A40B, 2
    inc word ptr word_A411
    inc word ptr word_A41D
    inc word ptr word_A417
loc_94B3:
    cmp byte ptr ds:byte_A3FA, 0
    je loc_94D8
    xor word ptr word_A408, 80h
    xor word ptr word_A40E, 80h
    xor word ptr word_A414, 80h
    xor word ptr word_A41A, 80h
    xor word ptr word_A420, 80h
loc_94D8:
    pop dx
    pop bx
    pop ax
    ret

sub_94DC:
    push bx
    mov bx, 0A6EDh
loc_94E0:
    cmp word ptr [bx], dx
    jne loc_94EE
    cmp word ptr [bx+2], ax
    jne loc_94EE
    mov ax, word ptr [bx+4]
    pop bx
    ret
loc_94EE:
    add bx, 6
    jmp short loc_94E0

sub_94F3:
    push ax
    push dx
    push cx
    mov cx, 4
    mov ax, 20h
    mov dx, 0FF60h
loc_94FF:
    push cx
    mov cx, 1
    push ax
    call sub_8C13
    mov word ptr word_A33A, ax
    pop ax
    call sub_8D1B
    neg ax
    js loc_9518
    sub ax, 10h
    sub dx, 10h
loc_9518:
    pop cx
    loop loc_94FF
    pop cx
    pop dx
    pop ax
    ret
CODE ends
DATA segment word public 'DATA'
DATA ends
end
