;xorshift procedure



RND:		;random number generator
		;
		; nonzero SEED is in r4 r5 r6 r7
		; returns value in A
		; r3 killed

;; C code is:
;	static uint8_t h,l,d,e;
;	uint8_t hh,ll;

;	ll = l ^ (l<<3);
;	hh = d ^ (d<<1);
;	ll = ll ^ hh ^ (hh>>1);
;	hh = e;

;	d=h;e=l;
;	h=hh;l=ll;

;	return l;

		;r4 - H
		;r5 - L
		;r6 - D
		;r7 - E

		mov	a,r6
		add	a,r6
		xrl	a,r6
		mov	r3,a	;hh=d^(d<<1)

		clr	c
		rrc	a
		xrl	a,r3
		mov	r3,a ;hh=hh^(hh>>1)

		mov	a,r5
		rl	a
		rl	a
		rl	a
		anl	a,#$F8
		xrl	a,r5
		xrl	a,r3 ;hh=hh^l^(l<<3)
		
	;r6<r4<r7<r5<hh

		xch	a,r5
		xch	a,r7
		xch	a,r4
		xch	a,r6

		mov	a,r5


		ret	;return l
		;result also in r5 (must be stored)

