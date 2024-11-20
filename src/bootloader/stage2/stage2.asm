bits 16
section _ENTRY class=code
extern _cstart_
global entry

entry:
	cli
	mov ax, ds
	mov ss, ax
	mov sp, 0
	mov bp, sp
	sti

	; Expect boot driver number in DL, pass it to cstart function
	xor dh, dh
	push dx
	call _cstart_

	cli
	hlt
