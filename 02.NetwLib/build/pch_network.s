;
; File generated by cc65 v 2.19 - Git 734a76c
;
	.fopt		compiler,"cc65 v 2.19 - Git 734a76c"
	.setcpu		"6502"
	.smart		on
	.autoimport	on
	.case		on
	.debuginfo	off
	.importzp	sp, sreg, regsave, regbank
	.importzp	tmp1, tmp2, tmp3, tmp4, ptr1, ptr2, ptr3, ptr4
	.macpack	longbranch
	.import		_printf
	.import		_ip65_init
	.export		_print_ip
	.export		_check_uthernet

.segment	"RODATA"

S0001:
	.byte	$25,$64,$2E,$25,$64,$2E,$25,$64,$2E,$25,$64,$0A,$00

; ---------------------------------------------------------------
; void __near__ print_ip (unsigned long ip)
; ---------------------------------------------------------------

.segment	"CODE"

.proc	_print_ip: near

.segment	"CODE"

	jsr     pusheax
	lda     sp
	ldx     sp+1
	jsr     pushax
	lda     #<(S0001)
	ldx     #>(S0001)
	jsr     pushax
	ldy     #$03
	lda     (sp),y
	sta     ptr1+1
	dey
	lda     (sp),y
	sta     ptr1
	ldx     #$00
	lda     (ptr1,x)
	jsr     pusha0
	ldy     #$05
	lda     (sp),y
	tax
	dey
	lda     (sp),y
	ldy     #$01
	sta     ptr1
	stx     ptr1+1
	lda     (ptr1),y
	jsr     pusha0
	ldy     #$07
	lda     (sp),y
	tax
	dey
	lda     (sp),y
	ldy     #$02
	sta     ptr1
	stx     ptr1+1
	lda     (ptr1),y
	jsr     pusha0
	ldy     #$09
	lda     (sp),y
	tax
	dey
	lda     (sp),y
	ldy     #$03
	sta     ptr1
	stx     ptr1+1
	lda     (ptr1),y
	jsr     pusha0
	ldy     #$0A
	jsr     _printf
	jmp     incsp6

.endproc

; ---------------------------------------------------------------
; unsigned int __near__ check_uthernet (void)
; ---------------------------------------------------------------

.segment	"CODE"

.proc	_check_uthernet: near

.segment	"CODE"

	ldy     #$12
	jsr     subysp
	ldy     #$00
	lda     #$FF
	sta     (sp),y
	iny
	sta     (sp),y
	iny
	lda     #$00
	sta     (sp),y
	iny
	sta     (sp),y
L0002:	ldy     #$03
	lda     (sp),y
	cmp     #$00
	bne     L0006
	dey
	lda     (sp),y
	cmp     #$07
L0006:	bcs     L0003
	ldy     #$03
	lda     (sp),y
	sta     tmp1
	dey
	lda     (sp),y
	asl     a
	rol     tmp1
	ldx     tmp1
	clc
	adc     #$04
	bcc     L0007
	inx
	clc
L0007:	adc     sp
	tay
	txa
	adc     sp+1
	tax
	tya
	jsr     pushax
	ldy     #$04
	lda     (sp),y
	jsr     _ip65_init
	ldy     #$00
	jsr     staxspidx
	ldy     #$03
	lda     (sp),y
	sta     tmp1
	dey
	lda     (sp),y
	asl     a
	rol     tmp1
	ldx     tmp1
	clc
	adc     #$04
	bcc     L0009
	inx
	clc
L0009:	adc     sp
	sta     ptr1
	txa
	adc     sp+1
	sta     ptr1+1
	dey
	lda     (ptr1),y
	tax
	dey
	lda     (ptr1),y
	cpx     #$00
	bne     L0004
	cmp     #$00
	bne     L0004
	ldy     #$03
	lda     (sp),y
	tax
	dey
	lda     (sp),y
	jsr     stax0sp
	jmp     L0003
L0004:	ldy     #$02
	ldx     #$00
	lda     #$01
	jsr     addeqysp
	jmp     L0002
L0003:	ldy     #$01
	lda     (sp),y
	tax
	dey
	lda     (sp),y
	ldy     #$12
	jmp     addysp

.endproc

