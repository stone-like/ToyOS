ORG 0x7c00
BITS 16

CODE_SEG equ gdt_code -gdt_start
DATA_SEG equ gdt_data -gdt_start

_start:
    jmp short start
    nop
 times 33 db 0 ; fake BIOS Parameter Block

start:
    jmp 0:step2


; Read Disk

; AH = 02h
; AL = number of sectors to read (must be nonzero)
; CH = low eight bits of cylinder number
; CL = sector number 1-63 (bits 0-5)
; high two bits of cylinder (bits 6-7, hard disk only)
; DH = head number
; DL = drive number (bit 7 set for hard disk)
; ES:BX -> data buffer

; Return:
; CF set on error
; if AH = 11h (corrected ECC error), AL = burst length
; CF clear if successful
; AH = status (see #00234)
; AL = number of sectors transferred (only valid if CF set for some
; BIOSes)

step2:
    cli ;Clear Interrupts
    mov ax,0x00
    mov ds,ax
    mov es,ax
    mov ss,ax
    mov sp,0x7c00
    sti ;Enables Interrupts

.load_protected:
    cli
    lgdt[gdt_descriptor]
    mov eax,cr0
    or eax,0x1
    mov cr0,eax
    jmp CODE_SEG:load32 ;JUMP and SET CODE SEG 
    ; cr0で32bitに入っているのでgdtを使う方式
    ;CODE_SEG=0x8で、0x8にはlgdtでロードしたgdtTableの[1]があり、gdt_codeがある
    ; gdt_codeは0x0を指し、load32は間接なので、0x0+load32+orgとなる
    
    ;対して、CODE_SEG:0x0100000は0x0100000が絶対なのでorgは絡まず、
    ;0x0 + 0x0100000となる





; GDT
gdt_start:
gdt_null:
    dd 0x0
    dd 0x0

;AccessByteをcodeとdataで変える必要があるのはcodeとdataでprivilage0にするための方法がcode,dataでちょっと違うから
; offset 0x8,startAdress 0x0
gdt_code:     ;CS SHOULD POINT TO THIS
    dw 0xffff ; Segment limit first 0-15 bits  
    dw 0      ; Base first 0-15bits
    db 0      ; Base 16-26bits
    db 0x9a   ; AccessByte
    db 11001111b ; High 4bit flags and low 4 bit flags
    db 0      ; Base 24-31 bits

; offset 0x10,startAdress 0x0
gdt_data:     ;DS,SS,ES,FS,GS SHOULD POINT TO THIS
    dw 0xffff ; Segment limit first 0-15 bits  
    dw 0      ; Base first 0-15bits
    db 0      ; Base 16-26bits
    db 0x92   ; AccessByte
    db 11001111b ; High 4bit flags and low 4 bit flags
    db 0      ; Base 24-31 bits


gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start -1
    dd gdt_start

[BITS 32]
load32:
; load Kernel into memory
    mov eax,1 ;何セクタ目か,0はブートセクタ
    mov ecx,100 ;どれくらいのセクタをloadするか
    mov edi,0x0100000 ; メモリの1M地点
    ;1セクタ目から/bin/kernel.bin(kernelのコードをまとめてコンパイルしたもの)があるので
    ;それをロード

    call ata_lba_read
    jmp CODE_SEG:0x0100000

ata_lba_read:
    mov ebx, eax, ; Backup the LBA
    ; Send the highest 8 bits of the lba to hard disk controller
    shr eax, 24
    or eax, 0xE0 ; Select the  master drive
    mov dx, 0x1F6
    out dx, al
    ; Finished sending the highest 8 bits of the lba

    ; Send the total sectors to read
    mov eax, ecx
    mov dx, 0x1F2
    out dx, al
    ; Finished sending the total sectors to read

    ; Send more bits of the LBA
    mov eax, ebx ; Restore the backup LBA
    mov dx, 0x1F3
    out dx, al
    ; Finished sending more bits of the LBA

    ; Send more bits of the LBA
    mov dx, 0x1F4
    mov eax, ebx ; Restore the backup LBA
    shr eax, 8
    out dx, al
    ; Finished sending more bits of the LBA

    ; Send upper 16 bits of the LBA
    mov dx, 0x1F5
    mov eax, ebx ; Restore the backup LBA
    shr eax, 16
    out dx, al
    ; Finished sending upper 16 bits of the LBA

    mov dx, 0x1f7
    mov al, 0x20
    out dx, al

    ; Enable the A20 line
    in al, 0x92
    or al, 2
    out 0x92, al
    ; Read all sectors into memory
.next_sector:
    push ecx
; Checking if we need to read
.try_again:
    mov dx, 0x1f7
    in al, dx
    test al, 8
    jz .try_again

; We need to read 256 words at a time
    mov ecx, 256
    mov dx, 0x1F0
    rep insw  ;rep inswはediにecx Wordをdxポートinputする,
    ;今は16bitなので1word=2byteとなり256word=512byte=1セクタ
    pop ecx
    loop .next_sector
    ; End of reading sectors into memory
    ret


times 510-($-$$) db 0
dw 0xAA55 
;littelEndian
