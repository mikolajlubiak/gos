bits 16

section _TEXT class=CODE

;
;   Print a character to the screen
;   Parameters:
;       Stack: character, page
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
