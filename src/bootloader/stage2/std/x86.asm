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
