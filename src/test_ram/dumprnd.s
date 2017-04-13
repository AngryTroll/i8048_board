; test RAM Memory.
;
; dump xorshift values


		;
		;stack:
		;8-9, A-B -- call area
		;C..17 -- free






		cpu	8048
		relaxed	on



	;memory locations

	;registers
	; r4 r5 r6 r7 -- RNG


		; 6MHz clock = 400kHz (1/15) cycle speed = 2.5us per cycle

		org	$100
		sel	rb0
		dis	i
		dis	tcnti


		;init RNG seed with 1
		clr	a
		mov	r4,a
		mov	r5,a
		mov	r6,a
		mov	r7,a
		inc	r4




		;main dump loop...
LOOP:
		mov	a,#10
		call	sendbyte
		mov	r0,#40
loop2:
		call	RND
		call	sendhex

		djnz	r0,loop2

		jmp	LOOP


sendhex:	;send hex byte
		mov	r1,a
		swap	a
		call	shex
		mov	a,r1
shex:
		anl	a,#$0F
		add	a,#-10
		jnc	shex_09
		add	a,#'A'-'0'-10
shex_09:	add	a,#10+'0'

;-------------------------------------------------------------------------------
sendbyte:	;sends byte from A at 9600 baud (6MHz)
		; kills a,r2,r3

		clr	c
		mov	r2,#11

sendbyte_loop:	jc	sbl_1
		anl	P2,#$BF
sbl_1:		jnc	sbl_2
		orl	P2,#$40
sbl_2:		; 6 cycles
		clr	c	;1
		cpl	c	;1
		rrc	a	;1

		mov	r3,#14	;2
		djnz	r3,$	;28
		nop		;1

		djnz	r2,sendbyte_loop	;2

		ret



		include	"xorshift.s"

