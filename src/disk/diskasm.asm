bits 32

global disk_read_sector
global disk_write_sector

%define DISK_STATE      0x8100
%define DISK_BUFFER     0x8200
%define DISK_GDT        0x8400
%define DISK_TRAMPOLINE 0x8500

; DISK_STATE:
; +00  DAP
; +10  LBA
; +14  protected-mode buffer
; +18  result
; +19  operation (0 = read, 1 = write)
; +1A  BIOS status
; +1C  protected-mode ESP
; +20  protected-mode EFLAGS
; +24  protected-mode CR0
; +28  protected-mode return EIP
; +2C  protected-mode return CS

disk_read_sector:
    pushfd
    pop eax
    mov [DISK_STATE + 0x20], eax

    push ebp
    mov ebp, esp
    push ebx
    push esi
    push edi

    mov [DISK_STATE + 0x1C], esp

    mov eax, [ebp + 8]
    mov [DISK_STATE + 0x10], eax

    mov eax, [ebp + 12]
    mov [DISK_STATE + 0x14], eax

    mov byte [DISK_STATE + 0x19], 0
    jmp disk_prepare

disk_write_sector:
    pushfd
    pop eax
    mov [DISK_STATE + 0x20], eax

    push ebp
    mov ebp, esp
    push ebx
    push esi
    push edi

    mov [DISK_STATE + 0x1C], esp

    mov eax, [ebp + 8]
    mov [DISK_STATE + 0x10], eax

    mov eax, [ebp + 12]
    mov [DISK_STATE + 0x14], eax

    mov byte [DISK_STATE + 0x19], 1

    mov esi, eax
    mov edi, DISK_BUFFER
    mov ecx, 256
    cld
    rep movsw

disk_prepare:
    mov dword [DISK_STATE + 0x28], protected_mode_return
    mov word [DISK_STATE + 0x2C], 0x08

    mov esi, rm_start
    mov edi, DISK_TRAMPOLINE
    mov ecx, rm_end - rm_start
    cld
    rep movsb

    cli

    mov eax, cr0
    mov [DISK_STATE + 0x24], eax
    and eax, 0xFFFFFFFE
    mov cr0, eax

    ; 16-bit far jump to 0000:8500.
    ; 66 = operand-size override
    ; EA = far jump
    db 0x66, 0xEA
    dw DISK_TRAMPOLINE
    dw 0x0000

bits 16

rm_start:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7000

    ; Build flat 32-bit GDT in low memory.
    mov dword [DISK_GDT + 0x00], 0
    mov dword [DISK_GDT + 0x04], 0

    mov dword [DISK_GDT + 0x08], 0x0000FFFF
    mov dword [DISK_GDT + 0x0C], 0x00CF9A00

    mov dword [DISK_GDT + 0x10], 0x0000FFFF
    mov dword [DISK_GDT + 0x14], 0x00CF9200

    ; GDTR at 0x8418.
    mov word [DISK_GDT + 0x18], 23
    mov dword [DISK_GDT + 0x1A], DISK_GDT

    ; Build DAP.
    mov byte [DISK_STATE + 0x00], 0x10
    mov byte [DISK_STATE + 0x01], 0
    mov word [DISK_STATE + 0x02], 1
    mov word [DISK_STATE + 0x04], DISK_BUFFER
    mov word [DISK_STATE + 0x06], 0

    mov eax, [DISK_STATE + 0x10]
    mov [DISK_STATE + 0x08], eax
    mov dword [DISK_STATE + 0x0C], 0

    xor dx, dx
    mov dl, [0x8000]
    mov si, DISK_STATE

    ; BIOS needs interrupts enabled.
    sti

    cmp byte [DISK_STATE + 0x19], 0
    jne rm_write

rm_read:
    mov ah, 0x42
    int 0x13
    jmp rm_result

rm_write:
    mov ah, 0x43
    mov al, 0
    int 0x13

rm_result:
    mov [DISK_STATE + 0x1A], ah

    cli

    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7000

    jc rm_error

    mov byte [DISK_STATE + 0x18], 0
    jmp rm_leave

rm_error:
    mov byte [DISK_STATE + 0x18], 1

rm_leave:
    lgdt [DISK_GDT + 0x18]

    mov eax, [DISK_STATE + 0x24]
    or eax, 1
    mov cr0, eax

    ; Far indirect jump to:
    ; protected_mode_return : 0x08
    ;
    ; 66 FF /5 = 32-bit far indirect jump
    ; pointer is at DISK_STATE + 0x28.
    db 0x66, 0xFF, 0x2E
    dw DISK_STATE + 0x28

rm_end:

bits 32

protected_mode_return:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax

    mov esp, [DISK_STATE + 0x1C]

    cmp byte [DISK_STATE + 0x18], 0
    jne .done

    cmp byte [DISK_STATE + 0x19], 0
    jne .done

    mov esi, DISK_BUFFER
    mov edi, [DISK_STATE + 0x14]
    mov ecx, 256
    cld
    rep movsw

.done:
    movzx eax, byte [DISK_STATE + 0x18]

    mov edx, eax
    push dword [DISK_STATE + 0x20]
    popfd
    mov eax, edx

    pop edi
    pop esi
    pop ebx
    pop ebp
    ret
