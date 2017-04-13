		cpu	8048
		relaxed	on

		; 6MHz clock = 400kHz (1/15) cycle speed = 2.5us per cycle

		; 41.(6) cycles per bit for 9600 speed
		; 42 cycles: 3 cycles (or 0.7%) error
		; ACHTUNG! 2 stop bits !!!1111

		; P24 -- 'prog' input pin
		; P26 -- txd output pin
		; P27 -- rxd input pin


		org	0
		sel	mb0
		jmp	init
		;org	3
		jmp	$103
run:
		jmp	$100	;go to main program
		;org	7
		jmp	$107
init:
		dis	i
		dis	tcnti
		sel	rb0

		; set 8243 to inputs (8155 set to inputs by reset)
		movd	a,P4
		movd	a,P5
		movd	a,P6
		movd	a,P7

		; set P27..P24 to inputs/highs,
		; P23..P20 to %1000
		mov	a,#$F8
		outl	P2,a

		; get state of P24 ('prog' pin)
		in	a,P2
		jb4	run ;if high -- go to main program

		;copy program to MOVX+PROG ram
		clr	a
		mov	r0,a
cpyloop:
		movp	a,@a
		movx	@r0,a ;p23..p20 is %1000

		inc	r0
		mov	a,r0
		jnz	cpyloop

		; go to address $800
		sel	mb1
		jmp	boot

boot:		;boot itself, works from MOVX/MOVP RAM memory at $800



		;print hello message
		mov	r5,#hello&255
prmsg:		mov	a,r5
		movp	a,@a
		jz	prmsg_end
		call	sendbyte
		inc	r5
		jmp	prmsg
prmsg_end:


		;receive command
wbyte:		call	recvbyte

		xrl	a,#'j'
		jnz	no_jump		; j
		mov	a,#'j'
		call	sendbyte
		sel	mb0
		jmp	$100

no_jump		xrl	a,#'j'!'w'
		jnz	no_write	; w
;write
		call	rbyte
		jf1	boot
		orl	a,#$F0
		outl	P2,a
		call	rbyte
		jf1	boot
		mov	r0,a
		call	rbyte
		jf1	boot

		mov	r4,a
		in	a,P2 ;check for address range (must be 0100..07ff)
		anl	a,#$0F
		jz	boot
		jb3	boot
		mov	a,r4

		movx	@r0,a

		mov	r5,#ok&255
		jmp	prmsg


no_write	xrl	a,#'w'!'r'
		jnz	boot		; r
;read
		call	rbyte
		jf1	boot
		orl	a,#$F0
		outl	P2,a
		call	rbyte
		jf1	boot
		mov	r0,a
		movx	a,@r0
		call	sbyte
		mov	r5,#enter&255
		jmp	prmsg



		


rbyte:		;receive hex byte (2 hex values) to A. F1=0 if ok, F1=1 if error
		clr	F1
		call	rhex
		jf1	rbyte_error
		swap	a
		mov	r4,a
		call	rhex
		orl	a,r4
rbyte_error	ret


rhex:		;receive hex to A. inverts F1 if error. 
		call	recvbyte
		;compare to be hex: 0..9, A..F (only uppercase) or hex: $30..$39, $41..$46
		add	a,#-'0' ;now is is 0..9, $11..$16
		
		jb7	rhex_error ;check for $20..$ff
		jb6	rhex_error
		jb5	rhex_error
		
		jb4	rhex_af
rhex_09		; 0..9 OK, A..F fail
		add	a,#-10
		jc	rhex_error
		add	a,#10
		ret

rhex_af		;$11..$16 OK, $17..$1F fail
		add	a,#-$17
		jc	rhex_error
		add	a,#$17-$11+$0A ; to hex digit A..F
		ret

rhex_error:	cpl	F1
		ret



hello:		db	"boot48",10,0
ok:		db	"ok"
enter:		db	10,0

;-------------------------------------------------------------------------------
recvbyte:	;receive byte to A
		; kills: r5,r6,r7

		in	a,P2
		jb7	recvbyte	;wait for start bit
		; jitter from the beginning of start bit: +3..+6 cycles

		mov	r6,#9 ; 2 
		              ; we receive start bit as another data bit, discarding it at the end
recvbyte_loop:
		mov	r7,#5 ; 2
		djnz	r7,$  ; 10

		xch	a,r5	;1
		in	a,P2	;2
		rlc	a	;1
		xch	a,r5	;1
		rrc	a	;1

		mov	r7,#9	;2
		djnz	r7,$	;18
		nop		;1

		djnz	r6,recvbyte_loop ;2

		; we return at the near end of last information bit,
		; we have near 1 uart bittime (stopbit) to parse the received byte
		ret



sbyte:		;send hex byte
		mov	r4,a
		swap	a
		call	shex
		mov	a,r4
shex:
		anl	a,#$0F
		add	a,#-10
		jnc	shex_09
		add	a,#'A'-'0'-10
shex_09:	add	a,#10+'0'

;-------------------------------------------------------------------------------
sendbyte:	;sends byte from A at 9600 baud (6MHz)
		; kills a,r6,r7

		clr	c
		mov	r6,#11

sendbyte_loop:	jc	sbl_1
		anl	P2,#$BF
sbl_1:		jnc	sbl_2
		orl	P2,#$40
sbl_2:		; 6 cycles
		clr	c	;1
		cpl	c	;1
		rrc	a	;1

		mov	r7,#14	;2
		djnz	r7,$	;28
		nop		;1

		djnz	r6,sendbyte_loop	;2

		ret

