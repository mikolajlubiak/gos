; Offset all address values by 0x7C00 - where BIOS in legacy boot expects the program to be
org 0x7C00

; Tell the assembler to compile 16 bit code
bits 16



;
;	FAT12 header
;
jmp short main
nop

; BIOS parameter block
bdb_oem:						db "MSWIN4.1"
bdb_bytes_per_sector:			dw 512
bdb_sectors_per_cluster:		db 1
bdb_reserved_sectors:			dw 1
bdb_fat_count:					db 2
bdb_dir_entries:				dw 0E0h
bdb_total_sectors:				dw 2880
bdb_media_descriptor_type:		db 0F0h
bdb_sectors_per_fat:			dw 9
bdb_sectors_per_track:			dw 18
bdb_head_count:					dw 2
bdb_hidden_sectors:				dd 0
bdb_large_sectors:				dd 0

; Extended boot record
ebr_drive_number:				db 0
								db 0
ebr_signature:					db 29h
ebr_volume_id:					db 1h, 2h, 3h, 4h
ebr_volume_label:				db "GALL OS INC"
ebr_system_id:					db "FAT12   "



;
;	Instructions
;

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


main:
	; Clear registers
	xor ax, ax
	mov ds, ax
	mov es, ax
	mov ss, ax

	; Set stack pointer to the bottom address of the program
	mov sp, 0x7C00

	; BIOS should store the driver number in the dl register
	mov [ebr_drive_number], dl

	; Populate the disk_read parameters
	mov ax, 1 ; LBA = 1, second sector
	mov cl, 1 ; Read 1 sector
	mov bx, 0x7E00 ; Store the data below instructions

	; Read the data
	call disk_read

	; Halt
	jmp halt


; Halt indefinitely
halt:
	cli ; Disable interrupts so that there is no way to get out of the halt
	hlt

; Wait for a key press, then reboot
wait_key_and_reboot:
	; Wait the a key press
	mov ah, 0
	int 16h

	; Reboot
	jmp 0FFFFh:0

	; Halt
	jmp halt



;
;	Disk routines
;

; Print error message and reboot
floppy_error:
	; Print the error message
	mov si, msg_floppy_error
	call puts

	jmp wait_key_and_reboot


;	Convert LBA address to CHS address
;	Parameters:
;		ax: LBA address
;	Retuns:
;		cx[0:5]: sector number
;		cx[6:15]: cylinder number
;		dh: head number
;
lba_to_chs:
	; Preserve register
	push ax
	push dx

	;
	;	sector number
	;
	; ax = LBA / bdb_sectors_per_track
	; cx = dx = LBA % bdb_sectors_per_track + 1
	;
	xor dx, dx
	div word [bdb_sectors_per_track]
	inc dx
	mov cx, dx

	;
	;	cylinder and head number
	;
	; ax = ax / bdb_head_count =
	; = (LBA / bdb_sectors_per_track) / bdb_head_count = cylinder number
	; dh = dx = ax % bdb_head_count =
	; = (LBA / bdb_sectors_per_track) % bdb_head_count = head number
	;
	xor dx, dx
	div word [bdb_head_count]
	mov dh, dl	; dl is lower 8 bits of dx

	;			CX
	;	CH		+		CL
	;
	;	CH + CL[8:6] = cylinder numcer
	;	CL[0:5] = sector number
	mov ch, al	; al is lower 8 bits of ax
	shl ah, 6	; ah is upper 8 bits of ax
	or cl, ah


	; Restore registers
	pop ax
	mov dl, al
	pop ax
	ret


;
;	Read from disk
;	Parameters:
;		ax: LBA address
;		cl: number of sectors to read
;		dl: driver number
;		es:bx: memory address to store the data
;
disk_read:
	; Preserve registers
	push ax
	push bx
	push cx
	push dx
	push di
	push cx ; Save CL

	; Convert the address
	call lba_to_chs

	; Pop the previously pushed CX. Write CL to AL
	pop ax

	; Set flag to read disk sectors
	mov ah, 02h

	; Retry the read operation 3 times. Floppy disks can be pretty unreliable.
	mov di, 3

.retry
	; Save all registers, we don't know what BIOS will override
	pusha

	; Set carry flag, some BIOSes don't do that automatically
	stc

	; Interrupt
	int 13h

	; If the carry flag is unset the operation succeeded
	jnc .done

	; If the operation failed restore the registers and reset the floppy controller
	popa
	call disk_reset

	; Retry until di is not zero: 3 times
	dec di
	test di, di
	jnz .retry

.fail:
	; Failed 3 times
	jmp floppy_error

; If succeeded, restore the registers
.done:
	popa

	; Restore registers and return
	pop di
	pop dx
	pop cx
	pop bx
	pop ax
	ret


;
;	Disk reset
;	Parameters:
;		dl: driver number
;
disk_reset:
	; Preserve registers
	pusha

	; Reset the floppy
	mov ah, 0
	stc
	int 13h

	; If failed print error and reboot
	jc floppy_error

	; Restore the registers and return
	popa
	ret



; Define end line escape sequence
%define ENDL 0x0D, 0x0A

; Store the string
msg_floppy_error: 		db "Read from floppy failed 3 times. KERNEL PAAAAAAAAANIC! or something... idk", ENDL, 0


; Add padding until we reach 510 bytes
times 510-($-$$) db 0

; Set the last two bytes to the signature
dw 0AA55h
