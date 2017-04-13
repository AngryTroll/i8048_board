; test RAM Memory.
;
;done for i8048 retro board.
; 
; requirements:
; 0x800..0x8FF -- external RAM (also tested)


; stack: starts from 0 (s3..s0=0), increments at pushes (CALLs).
; stack is empty (if sp=8, push will be made at addr 8), incrementing (at push)



		;stack:
		;8-9, A-B -- call area
		;C..17 -- free






		cpu	8048
		relaxed	on



	;memory locations
maxmem	equ	$0C
stseed	equ	maxmem+1 ;d..10
count	equ	stseed+4  ;11
istrt	equ	count+2; ;13 -- first free memory

	;registers
	; r4 r5 r6 r7 -- RNG


		; 6MHz clock = 400kHz (1/15) cycle speed = 2.5us per cycle

		org	$100
		sel	rb0
		dis	i
		dis	tcnti


		;init RNG with memory contents
		mov	r0,#8
		mov	r2,#(64-8)/4 ;8--63 only
irng
		mov	a,@r0
		inc	r0
		addc	a,r4
		mov	r4,a
		mov	a,@r0
		inc	r0
		addc	a,r5
		mov	r5,a
		mov	a,@r0
		inc	r0
		addc	a,r6
		mov	r6,a
		mov	a,@r0
		inc	r0
		addc	a,r7
		mov	r7,a

		djnz	r2,irng
		;check if zero
		orl	a,r4
		orl	a,r5
		orl	a,r6
		jnz	irng_nozero
		inc	r4
irng_nozero


		;determine internal memory size
		mov	r0,#255
		mov	@r0,#255
		mov	r0,#127
		mov	@r0,#127
		mov	r0,#63
		mov	@r0,#63
		mov	r0,#255
		mov	a,@r0
		mov	r0,#maxmem
		mov	@r0,a ;maxmem - address of last byte of internal memory

		;init pass counter
		mov	r0,#count
		clr	a
		mov	@r0,a
		inc	r0
		mov	@r0,a


		;init external memory address
		anl	P2,#$F0
		orl	P2,#$08



		;main testing loop...
LOOP:

		;store SEED
		mov	r1,#stseed
		mov	a,r4
		mov	@r1,a
		inc	r1
		mov	a,r5
		mov	@r1,a
		inc	r1
		mov	a,r6
		mov	@r1,a
		inc	r1
		mov	a,r7
		mov	@r1,a


		;fill internal memory
		mov	r0,#maxmem
		mov	a,@r0
		inc	a
		mov	r2,a

		mov	r0,#istrt
fill_l:		call	RND
		mov	@r0,a
		inc	r0
		mov	a,r0
		xrl	a,r2
		jnz	fill_l

		;fill external memory
		mov	r0,#0
fill_e:		call	RND
		movx	@r0,a
		inc	r0
		mov	a,r0
		jnz	fill_e



		;restore SEED
		;mov	r1,#stseed
		mov	a,@r1
		mov	r7,a
		dec	r1
		mov	a,@r1
		mov	r6,a
		dec	r1
		mov	a,@r1
		mov	r5,a
		dec	r1
		mov	a,@r1
		mov	r4,a


		;check internal memory

		mov	r0,#istrt
check_l:	call	RND
		mov	a,@r0
		inc	r0
		xrl	a,r5
		jnz	error
		mov	a,r0
		xrl	a,r2
		jnz	check_l

		;check external memory
		mov	r0,#0
check_e:	call	RND
		movx	a,@r0
		xrl	a,r5
		jnz	error
		inc	r0
		mov	a,r0
		jnz	check_e


		;were no errors: increment and print pass counter
		mov	r0,#count
		mov	a,#1
		add	a,@r0
		da	a
		mov	@r0,a
		inc	r0
		clr	a
		addc	a,@r0
		da	a
		mov	@r0,a

		mov	a,@r0
		call	sendhex
		dec	r0
		mov	a,@r0
		call	sendhex

		mov	a,#10
		call	sendbyte

		jmp	LOOP

error:		mov	a,#'e'
		call	sendbyte
		jmp	error



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

