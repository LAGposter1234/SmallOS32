bits 16
org 0x7c00

start:
    cli
    xor ax, ax
    mov ds, ax
    mov [boot_drive], dl
    mov [0x8000], dl

    mov si, boot_msg
    call print_msg

    ; load kernel
    mov ax, 0x1000
    mov es, ax ; segment
    mov ah, 02h ; read sectors
    mov al, 20h ; 32 of them
    mov ch, 0 ; cylander 0
    mov cl, 2 ; sector 2? ig?
    mov dh, 0 ; head 0
    mov dl, [boot_drive] ; drive
    mov bx, 0 ; offset
    int 13h ; me fav interrupt (totally)

    jc error ; jump if error

    mov ax, 0012h
    int 10h

    cli

    lgdt [gdtr]

    mov eax, cr0
    or al, 1
    mov cr0, eax

    jmp 08h:protected_mode

    jmp $

error:
    mov si, error_msg
    call print_msg
    cli
    hlt

; arg1 - si
print_msg:
.start:
    mov al, [si]
    cmp al, 0
    je .done
    ; it is not 0
    ; print the character in al
    mov ah, 0x0e
    int 0x10
    inc si
    jmp .start
.done:
    ret

boot_msg:
    db "SmallOS 32", 0Dh, 0Ah, 0

error_msg:
    db "Disk Error!", 0Dh, 0Ah, 0

boot_drive:
    db 0

gdt:
    dq 0

    ; 32-bit code: base 0, limit 4 GiB
    dq 0x00CF9A000000FFFF

    ; 32-bit data: base 0, limit 4 GiB
    dq 0x00CF92000000FFFF

gdt_end:

gdtr:
    dw gdt_end - gdt - 1
    dd gdt

bits 32

protected_mode:
    mov ax, 10h
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax

    mov esp, 0x90000

    jmp 08h:0x10000

times 446-($-$$) db 0

db 0x80
db 0x01, 0x01, 0x00
db 0xDA
db 0xFE, 0xFF, 0xFF
dd 0x00000001
dd 0x0000003F

times 48 db 0

dw 0xAA55
