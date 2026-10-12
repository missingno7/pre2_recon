; Prehistorik 2 (PRE2.EXE), link module 4 of 4
; module interface (tools/symtab.py)
public sub_9520
public sub_96D5
public sub_9B23
DATA segment word public 'DATA'
extrn byte_27E8:byte
extrn byte_2874:byte
extrn byte_2879:byte
extrn byte_2D8A:byte
extrn byte_6BC4:byte
extrn byte_6BD5:byte
extrn byte_6BE5:byte
extrn byte_B1A5:byte
extrn byte_B1AA:byte
extrn byte_B1AB:byte
extrn byte_B1B2:byte
extrn byte_B1BB:byte
extrn word_003D:word
extrn word_1F18:word
extrn word_27F0:word
extrn word_2875:word
extrn word_2DD8:word
extrn word_2DE4:word
extrn word_2DE6:word
extrn word_5088:word
extrn word_62DA:word
extrn word_667A:word
extrn word_6BF8:word
extrn word_7522:word
extrn word_B199:word
extrn word_B19B:word
extrn word_B19D:word
extrn word_B19F:word
extrn word_B1A1:word
extrn word_B1A3:word
extrn word_B1A8:word
extrn word_B1AE:word
DATA ends
; end module interface
CODE segment word public 'CODE'
assume cs:CODE, ds:DATA, es:nothing, ss:nothing
; module interface (tools/symtab.py)
extrn sub_02CC:near
extrn sub_0492:near
extrn sub_0DC1:near
extrn sub_26FA:near
extrn sub_3034:near
extrn sub_3042:near
extrn sub_39DF:near
extrn sub_44FB:near
extrn sub_452B:near
extrn sub_453B:near
extrn sub_9286:near
extrn sub_932F:near
; end module interface

sub_9520:
    mov ax, seg DATA
    mov ds, ax
    mov es, ax
    call sub_3042
    mov ax, 2
    call sub_02CC
    mov cx, 5
    call sub_0492
    mov ax, seg DATA
    mov ds, ax
    mov es, word ptr word_2875
    mov bl, byte ptr byte_2D8A
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
    mov word ptr word_B19F, ax
    mov word ptr word_B199, ax
    mov word ptr word_B19B, ax
    mov word ptr word_B19D, 8
    push es
    push ds
    pop es
    mov dx, 0B0E8h
    xor bx, bx
    mov ax, 1012h
    mov cx, 10h
    int 10h
    pop es
    call sub_3034
    call sub_452B
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
    mul word ptr word_B19F
    mov bx, word ptr word_B19D
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
    xchg word ptr word_B1A1, bx
    mov word ptr word_B1A3, bx
    call sub_452B
    sti
    call sub_0DC1
    call sub_990D
    cli
    mov dx, 3C4h
    mov al, 2
    out dx, al
    mov dx, 3C0h
    mov al, 33h
    out dx, al
    mov al, byte ptr word_B19D
    and al, 7
    out dx, al
    mov dx, word ptr word_B19D
    inc word ptr word_B19D
    test dl, 7
    jne loc_969C
    mov bp, 1FFFh
    mov ds, word ptr word_2875
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
    cmp word ptr ds:word_B19D, 280h
    jae loc_96AE
    jmp near ptr loc_9613
loc_96AE:
    sti
    mov cx, 46h
loc_96B2:
    call sub_990D
    call sub_0DC1
    cmp byte ptr ds:byte_27E8, 0
    jne loc_96C1
    loop loc_96B2
loc_96C1:
    mov ax, seg DATA
    mov ds, ax
    sti
    call sub_9286
    mov byte ptr byte_2874, 0
    mov byte ptr byte_6BE5, 0
    ret

sub_96D5:
    mov ax, seg DATA
    mov ds, ax
    mov es, ax
    mov word ptr word_B1AE, dx
    mov ax, 1
    call sub_02CC
    call sub_3042
    mov cx, 6
    call sub_0492
    mov ax, 0A000h
    mov es, ax
    mov ds, word ptr word_2875
    xor si, si
    xor di, di
    call sub_452B
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
    mov es, word ptr word_2875
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
    mov word ptr word_B19D, ax
    mov word ptr word_B19F, ax
    mov word ptr word_B199, ax
    mov word ptr word_B19B, ax
    mov word ptr word_B1A8, ax
    mov byte ptr byte_B1B2, al
    mov byte ptr byte_B1AA, 0
    mov byte ptr byte_B1AB, 4
    mov byte ptr byte_B1A5, 2
    mov byte ptr byte_B1BB, 1
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
    mul word ptr word_B19F
    mov bx, word ptr word_B19D
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
    xchg word ptr word_B1A1, bx
    mov word ptr word_B1A3, bx
    call sub_452B
    sti
    call sub_9900
    cli
    mov dx, 3C4h
    mov al, 2
    out dx, al
    cmp byte ptr ds:byte_B1BB, 0
    mov byte ptr byte_B1BB, 0
    jne loc_97F8
    mov dx, 3C0h
    mov al, 33h
    out dx, al
    mov al, byte ptr word_B19D
    and al, 7
    out dx, al
loc_97F8:
    call word ptr word_B1AE
    mov ax, seg DATA
    mov ds, ax
    call sub_453B
    mov al, byte ptr word_B199
    mov ah, byte ptr word_B19D
    and ax, 808h
    cmp al, ah
    je loc_9836
    mov si, word ptr word_B1A1
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
    mov dx, word ptr word_B19F
    sub dx, word ptr word_B19B
    je loc_9877
    js loc_984D
    mov si, word ptr word_B1A1
    dec si
    lea di, [si+word_1F18]
    jmp short loc_9860
loc_984D:
    neg dx
    mov si, word ptr word_B1A1
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
    cmp byte ptr ds:byte_B1B2, 0
    jne loc_9881
    jmp near ptr loc_97A8
loc_9881:
    sti
    call sub_9286
    ret

sub_9886:
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
    jmp short sub_9886
loc_98FF:
    ret

sub_9900:
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

sub_990D:
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

sub_991F:
    call sub_9AE0
    mov byte ptr ds:[0B1ABh], 4
    mov bx, 0B180h
    mov word ptr ds:[0B1A6h], 0C38h
    call sub_9886
    mov word ptr ds:[0B1A6h], 1185h
    mov byte ptr ds:[0B1ABh], 3
    mov bx, 0B185h
    cmp byte ptr ds:[0B197h], 0
    je loc_994B
    mov bx, 0B18Eh
loc_994B:
    call sub_9886
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

sub_9985:
    call sub_9AE0
    mov byte ptr ds:[0B1ABh], 3
    mov bx, 0B175h
    mov word ptr ds:[0B1A6h], 0AF2h
    call sub_9886
    mov byte ptr ds:[0B1ABh], 4
    mov bx, 0B170h
    mov word ptr ds:[0B1A6h], 12C9h
    call sub_9886
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

sub_9AE0:
    mov bp, 1FFFh
    mov ax, word ptr ds:[0B19Dh]
    sub ax, 4
    xchg word ptr ds:[0B19Dh], ax
    mov word ptr ds:[0B199h], ax
    add byte ptr ds:[0B1A5h], 1
    cmp byte ptr cs:[5], 0
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

sub_9B23:
    mov ax, seg DATA
    mov ds, ax
    mov es, ax
    xor ax, ax
    cmp byte ptr byte_2879, al
    je loc_9B35
    jmp near ptr loc_9C86
loc_9B35:
    mov word ptr word_2DE4, ax
    mov word ptr word_2DE6, ax
    mov word ptr word_6BF8, ax
    mov byte ptr byte_6BC4, al
    mov cx, 0Bh
    call sub_0492
    mov ax, 0A000h
    mov es, ax
    mov di, 3F40h
    mov ds, word ptr word_2875
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
    call sub_39DF
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
    call sub_39DF
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
    call sub_9CC0
    call sub_9C87
    call sub_26FA
    call sub_44FB
    call sub_0DC1
    call sub_990D
    cmp word ptr ds:word_27F0, 276h
    jae loc_9C83
    cmp byte ptr ds:byte_27E8, 0
    je loc_9C62
loc_9C83:
    call sub_9286
loc_9C86:
    ret

sub_9C87:
    push ax
    push bx
    push cx
    push dx
    push di
    push si
    push ds
    push es
    push bp
    call sub_453B
    mov ax, 0A000h
    mov es, ax
    mov di, word ptr ds:word_2DD8
    mov dx, 3C4h
    mov ax, 0F02h
    out dx, ax
    mov al, 28h
    mul byte ptr byte_6BC4
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

sub_9CC0:
    push ax
    push bx
    push cx
    push dx
    push di
    push si
    cmp byte ptr ds:byte_6BC4, 0B9h
    jae loc_9CD1
    inc byte ptr byte_6BC4
loc_9CD1:
    test byte ptr byte_6BD5, 3
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
    call sub_9DDA
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
    call sub_9DF0
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

sub_9DDA:
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

sub_9DF0:
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
