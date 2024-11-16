org 0x0

; Tell the assembler to compile 16 bit code
bits 16

start:
    ; Print hello world message
    mov si, msg_hello
    call puts

; Halt indefinitely
halt:
	cli ; Disable interrupts so that there is no way to get out of the halt
	hlt

;	Print character to the screen.
;	Parameters:
;		ds:si: string address
;
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
	; The interrupt excpets the character to print to be in the al register
	int 0x10

	; Loop
	jmp .loop

.done:
	; When done restore the register values and return
	pop ax
	pop si
	ret


; Define end line escape sequence
%define ENDL 0x0D, 0x0A

; Store the strings
msg_hello: db 'Hello, world! Its me, the KERNEL!', ENDL, 0
