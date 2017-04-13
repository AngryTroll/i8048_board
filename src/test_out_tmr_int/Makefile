all: tst_oti.hex tst_oti.bin


tst_oti.p: tst_oti.s Makefile
	asl -L tst_oti.s

tst_oti.hex: tst_oti.p Makefile
	p2hex tst_oti.p

tst_oti.bin: tst_oti.p Makefile
	p2bin -l 255 -r '$$-$$' tst_oti.p


clean:
	rm *.p *.lst *.hex *.bin

