bits 16

section _TEXT class=CODE

;
;   Divide a 64bit number by a 32bit number
;   Parameters:
;       Stack:  
;               uint64_t dividend,
;               uint32_t divisor,
;               uint64_t *quotient_out,
;               uint32_t *remainder_out
;
global _x86_div64_32
_x86_div64_32:
    ; Enter
    push bp
    mov bp, sp

    ; Preserve BX
    push bx

    ; [bp + 0] - old call frame
    ; [bp + 2] - return address
    ; [bp + 4] - first argument (dividend)
    ; [bp + 12] - second argument (divisor)
    ; [bp + 16] - third argument (quotient_out)
    ; [bp + 18] - fourth argument (remainder_out)

    ; Divide upper 32 bits
    mov eax, [bp + 8]   ; eax = upper 32 bits of dividend
    mov ecx, [bp + 12]  ; ecx = divisor
    xor edx, edx
    div ecx             ; eax = quot, edx = remainder

    ; Store the upper 32 bits of quotient
    mov bx, [bp + 16]
    mov [bx + 4], eax

    ; Divide upper 32 bits
    mov eax, [bp + 4]   ; eax = upper 32 bits of dividend
                        ; edx = old remainder
    div ecx

    ; Store results
    mov [bx], eax       ; store the lower 32 bits of quotient
    mov bx, [bp + 18]   ; store the remainder
    mov [bx], edx

    ; Restore BX
    pop bx

    ; Leave
    mov sp, bp
    pop bp

    ; Return
    ret


;
;   Print a character to the screen
;   Parameters:
;       Stack:
;               char character,
;               uint8_t page
;
global _x86_Video_WriteCharTeletype
_x86_Video_WriteCharTeletype:
    ; Enter
    push bp
    mov bp, sp

    ; Preserve BX
    push bx

    ; [bp + 0] - old call frame
    ; [bp + 2] - return address
    ; [bp + 4] - first argument (character)
    ; [bp + 6] - second argument (page)
    mov ah, 0Eh
    mov al, [bp + 4]
    mov bh, [bp + 6]

    ; Interrupt
    int 10h

    ; Restore BX
    pop bx

    ; Leave
    mov sp, bp
    pop bp

    ; Return
    ret


;
;   Resets the disk
;   Parameters:
;       Stack:
;               uint8_t drive number
;
global _x86_Disk_Reset
_x86_Disk_Reset:
    ; Enter
    push bp
    mov bp, sp

    ; [bp + 0] - old call frame
    ; [bp + 2] - return address
    ; [bp + 4] - drive number

    ; Setup registers
    mov dl, [bp+4]  ; Disk number

    ; Interrupt
    mov ah, 0       ; Reset disk
    stc             ; Set the carry flag to 1
    int 13h         ; Interrupt

    ; Return
    ; BIOS sets carry flag to 0 if succeeded
    mov ax, 1
    sbb ax, 0   ; Subtract with borrow to subtract the carry flag
                ; Returns 1 for success and 0 for failure

    ; Leave
    mov sp, bp
    pop bp

    ; Return
    ret


;
;   Read data from disk
;   Parameters:
;       Stack:
;               uint8_t drive number,
;               uint16_t cylinder number,
;               uint16_t sector number,
;               uint16_t head number,
;               uint8_t sectors to read count,
;               uint8_t far* data buffer
;
global _x86_Disk_Read
_x86_Disk_Read:
    ; Enter
    push bp
    mov bp, sp

    ; Preserve ES and BX
    push es
    push bx

    ; [bp + 0] - old call frame
    ; [bp + 2] - return address
    ; [bp + 4] - drive number
    ; [bp + 6] - cylinder number
    ; [bp + 8] - sector number
    ; [bp + 10] - head number
    ; [bp + 12] - sectors to read count
    ; [bp + 14] - data buffer pointer

    ; Setup registers
    mov dl, [bp+4]  ; Disk number
    mov ch, [bp+6]  ; Lower 8 bits of cylinder number

    mov cl, [bp+7]  ; Upper 2 bits of cylinder number
    shl cl, 6

    mov al, [bp+8] ; Lower 6 bits are sector number
    and al, 3Fh
    xor cl, al

    mov dh, [bp+10]  ; Head number

    mov al, [bp+12] ; Sectors to read

    ; Far data buffer pointer in ES:BX
    mov bx, [bp+16] ; Upper 2 bits of far pointer
    mov es, bx
    mov bx, [bp+14] ; Lower 2 bits of far pointer

    ; Interrupt
    mov ah, 02h     ; Read from disk
    stc             ; Set the carry flag to 1
    int 13h         ; Interrupt

    ; Return
    ; BIOS sets carry flag to 0 if succeeded
    mov ax, 1
    sbb ax, 0   ; Subtract with borrow to subtract the carry flag
                ; Returns 1 for success and 0 for failure

    ; Restore BX and ES
    pop bx
    pop es

    ; Leave
    mov sp, bp
    pop bp

    ; Return
    ret


;
;   Get drive parameters
;   Parameters:
;       Stack:
;               uint8_t drive number,
;               uint8_t* drive type,
;               uint16_t* cylinder count,
;               uint16_t* sector count,
;               uint16_t* head count
;
global _x86_Disk_GetDriveParams
_x86_Disk_GetDriveParams:
    ; Enter
    push bp
    mov bp, sp

    ; Preserve ES, BX, SI and DI
    push es
    push bx
    push si
    push di

    ; [bp + 0] - old call frame
    ; [bp + 2] - return address
    ; [bp + 4] - drive number
    ; [bp + 6] - drive type pointer
    ; [bp + 8] - cylinder count pointer
    ; [bp + 10] - sector count pointer
    ; [bp + 12] - head count pointer

    ; Setup registers
    mov dl, [bp+4]  ; Drive number
    mov di, 0       ; 0000h:0000h to guard against BIOS bugs
    mov es, di

    ; Interrupt
    mov ah, 08h     ; Get drive parameters
    stc             ; Set the carry flag to 1
    int 13h         ; Interrupt

    ; Setup output
    mov si, [bp+6]  ; Drive type
    mov [si], bl

    ; Cylinder count
    mov bl, ch      ; Lower bits
    mov bh, cl      ; Upper 2 bits
    shr bh, 6
    mov si, [bp+8]
    mov [si], bx

    ; Sector count
    ;xor ch, ch      ; Zero out bits 7 to 15
    and cl, 3Fh     ; 
    mov si, [bp+10]
    mov [si], cl

    mov si, [bp+12] ; Head count
    mov [si], dh

    ; Return
    ; BIOS sets carry flag to 0 if succeeded
    mov ax, 1
    sbb ax, 0   ; Subtract with borrow to subtract the carry flag
                ; Returns 1 for success and 0 for failure

    ; Restore DI, SI, BX and ES
    pop di
    pop si
    pop bx
    pop es

    ; Leave
    mov sp, bp
    pop bp

    ; Return
    ret

;========================================================================
;==     Name:           U4D                                            ==
;==     Operation:      Unsigned 4 byte divide                         ==
;==     Inputs:         DX;AX   Dividend                               ==
;==                     CX;BX   Divisor                                ==
;==     Outputs:        DX;AX   Quotient                               ==
;==                     CX;BX   Remainder                              ==
;==     Volatile:       none                                           ==
;========================================================================
global __U4D
__U4D:
    shl edx, 16     ; Move DX to upper half of EDX
    mov dx, ax      ; EDX = dividend
    mov eax, edx    ; EAX = dividend
    xor edx, edx

    shl ecx, 16     ; Move CX to upper half of ECX
    mov cx, bx      ; ECX = divisor

    ; Divide
    ; EAX = quotient
    ; EDX = remainder
    div ecx

    ; CX:BX = remainder
    mov ebx, edx
    mov ecx, edx
    shr ecx, 16

    ; DX:AX = quotient
    mov edx, eax
    shr edx, 16

    ; Return
    ret
