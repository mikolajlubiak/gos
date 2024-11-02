; Offset all address values by 0x7C00 - where BIOS in legacy boot expects the program to be
org 0x7C00

; Tell the assembler to compile 16 bit code
bits 16

start:
	jmp main

;	Print character to the screen.
;	Expects string address in ds:si
puts:
	; Preserve register values
	push si
	push ax

.loop:
	; Load value from address ds:si into al
	lodsb

	; Don't change the value, update CPU flags
	or al, al

	; If current character is string terminator, end the loop
	jz .done

	; Set flag to write to the screen
	mov ah, 0x0e

	; Set default write mode
	mov bh, 0

	; Interrupt
	int 0x10

	; Loop
	jmp .loop

.done:
	; When done restore the register values and return
	pop ax
	pop si
	ret


main:
	; Clear registers
	mov ax, 0
	mov ds, ax
	mov es, ax
	mov ss, ax

	; Set stack pointer to the bottom address of the program
	mov sp, 0x7C00

	; Store string pointer in ds:si
	mov si, msg_hello_world

	; Call the procedure
	call puts

	; Halt
	hlt

; Infinite loop in case hlt doesn't work
.halt:
	jmp .halt


; Define end line escape sequence
%define ENDL 0x0D, 0x0A

; Store the string
msg_hello_world: db "Hello, world!", ENDL, 0


; Add padding until we reach 510 bytes
times 510-($-$$) db 0

; Set the last two bytes to the signature
dw 0AA55h
