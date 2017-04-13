; test Output, Timers, Interrupt
;
;done for i8048 retro board.
; 
; requirements:
; J7 is closed (P27 to 7474 /R)
; J1 is closed (7474 /Q to /INT)
; J4 is open (nothing on T1)
; J3 is closed to the left (T_OUT to 7474 C)
; J2 is closed to the left (T0 to T_IN)
; 
; outputs some test signal on every pin of:
; - P1 of 8048
; - P4..P7 of 8243
; - PA, PB and PC of 8155
;
; T_OUT of 8155 outputs sweeped square wave from 1MHz down to 122Hz and back



		cpu	8048
		relaxed	on



obuf	equ	32	;6 bytes

tmrflag	equ	obuf+6
intflag	equ	tmrflag+1

ostate	equ	intflag+1 ;output state

tval	equ	ostate+1 ;8155 timer value
tdir	equ	tval+2	 ;direction and state



		; 6MHz clock = 400kHz (1/15) cycle speed = 2.5us per cycle

		org	$100
		jmp	init
		
		org	$103
		jmp	extint




		org	$107
;;;;;;;;;;;;;;;;;;;
; TIMER INTERRUPT ;
;;;;;;;;;;;;;;;;;;;
		jtf	tmr_proc
		retr
tmr_proc:
		sel	rb1
		mov	r7,a

		;set timer count period
		mov	a,#$E0
		mov	T,a

		;set interrupt flag
		mov	r0,#tmrflag
		mov	a,@r0
		dec	a
		mov	@r0,a

		;enable external int
		en	i

		mov	a,r7
		retr





extint:	
		sel	rb1
		mov	r7,a

		;clear interrupt ff by P25
		anl	P2,#$DF
		orl	P2,#$20

		;set interrupt flag
		mov	r0,#intflag
		mov	a,@r0
		dec	a
		mov	@r0,a

		;disable further ints
		dis	i

		mov	a,r7
		retr



init:		sel	rb0

		;disable interrupts
		dis	i
		dis	tcnti

		;enable clock output
		ent0	clk

		;clear ram
		mov	r0,#63
i_clram:	mov	@r0,#0
		dec	r0
		mov	a,r0
		jb5	i_clram


		;P2 port outputs all ones in high part
		orl	P2,#$F0


		;configure 8155 to outputs and to run timer in square wave mode
		anl	P2,#$F0
		orl	P2,#$09
		mov	r0,#$00
		mov	a,#$4F ;tmr stop, no interrupts, all pins to output
		movx	@r0,a
		mov	r0,#$04
		mov	a,#$FF
		movx	@r0,a
		inc	r0
		mov	a,#$7F ;tmp in square wave mode
		movx	@r0,a
		mov	r0,#$00
		mov	a,#$CF ;start timer
		movx	@r0,a 



		;enable timer interrupts
		strt	t
		en	tcnti

main_loop:
		mov	r0,#tmrflag
		mov	a,@r0
		jz	no_tmr
		inc	@r0
		call	output_proc

no_tmr
		mov	r0,#intflag
		mov	a,@r0
		jz	no_int
		inc	@r0
		call	int_proc
no_int
		jmp	main_loop


output_proc:
		;output data to different IO ports
		mov	r0,#obuf
		mov	a,@r0
		inc	r0
		outl	P1,a

		mov	a,@r0
		inc	r0
		movd	P4,a
		swap	a
		movd	P5,a

		mov	a,@r0
		inc	r0
		movd	P6,a
		swap	a
		movd	P7,a

		;8155 ports
		anl	P2,#$F0
		orl	P2,#$09
		mov	r1,#1

		mov	a,@r0
		inc	r0
		movx	@r1,a
		inc	r1
		mov	a,@r0
		inc	r0
		movx	@r1,a
		inc	r1
		mov	a,@r0
		movx	@r1,a



		; rotate states
		mov	r0,#obuf
		mov	r1,#ostate
		mov	a,@r1

		jz	tstate_0
		dec	a
		jz	tstate_1
		dec	a
		jz	tstate_2
		dec	a
		jz	tstate_3
		dec	a
		jz	tstate_4
		dec	a
		jz	tstate_5
		mov	@r1,#0
		ret

tstate_0	;clear obuf
		mov	r7,#6
tst0_l:		mov	@r0,#0
		inc	r0
		djnz	r7,tst0_l
		
		inc	@r1
		ret

tstate_1	;shift in single one
		mov	@r0,#1

		inc	@r1
		ret

tstate_2	;shift one through the bytes
		mov	r7,#6
		clr	c
tst2_l:		mov	a,@r0
		rlc	a
		mov	@r0,a
		inc	r0
		djnz	r7,tst2_l

		jnc	tst2_r
		inc	@r1
tst2_r		ret

tstate_3	;fill with all ones
		mov	r7,#6
tst3_l		mov	@r0,#$FF
		inc	r0
		djnz	r7,tst3_l
		inc	@r1
		ret

tstate_4	;shift in single zero
		mov	@r0,#$FE
		inc	@r1
		ret

tstate_5	;shift zero through bytes
		mov	r7,#6
		clr	c
		cpl	c
tst5_l:		mov	a,@r0
		rlc	a
		mov	@r0,a
		inc	r0
		djnz	r7,tst5_l
		jc	tst5_r
		mov	@r1,#0
tst5_r		ret



		;interrupt proc (changes 8155 timer settings)
int_proc:
		mov	r0,#tval


		mov	a,@r0
		mov	r2,a ;r2 - low
		inc	r0
		mov	a,@r0 ;a - high

		;make add/sub value by shifting right by 2
		clr	c
		rrc	a
		mov	r3,a
		mov	a,r2
		rrc	a
		mov	r2,a
		clr	c
		mov	a,r3
		rrc	a
		mov	r3,a
		mov	a,r2
		rrc	a
		mov	r2,a

		orl	a,r3	;see if it's zero and set to one if yes
		jnz	taddv_nz
		mov	r2,#1
taddv_nz
		dec	r0	;r0 points to low tval value

		;see what to do: add or subtract
		mov	r1,#tdir
		mov	a,@r1
		jz	tadd
tsub
		;sub r3:r2 from tval
		mov	a,r2
		cpl	a
		add	a,#1
		mov	r2,a
		mov	a,r3
		cpl	a
		addc	a,#0
		mov	r3,a

tadd:		;add r3:r2 to tval
		mov	a,r2
		add	a,@r0
		mov	@r0,a
		inc	r0
		mov	a,r3
		addc	a,@r0
		mov	@r0,a
		dec	r0
		anl	a,#$C0
		nop
		nop
		jz	taddsub_end

taddsub_ovf:	;overflow...
		
		clr	a
		cpl	a
		xrl	a,@r1 ;zero - add, FF - sub
		mov	@r1,a
		;new value: FF sub, 00 add
		
		mov	@r0,a
		inc	r0
		anl	a,#$3F
		mov	@r0,a
		dec	r0

taddsub_end:	;now put value into timer: r0 points to tval.low

		anl	P2,#$F0
		orl	P2,#$09
		mov	r1,#$04
		mov	a,@r0
		movx	@r1,a
		inc	r0
		inc	r1
		mov	a,@r0
		anl	a,#$3F
		orl	a,#$40 ;mode = square wave
		movx	@r1,a

		mov	r1,#$00
		mov	a,#$CF
		movx	@r1,a ;actualize new values


		ret

