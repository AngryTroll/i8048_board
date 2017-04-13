all: tst_ram.hex tst_ram.bin dumprnd.hex dumprnd.bin


tst_ram.p: tst_ram.s xorshift.s Makefile
	asl -L tst_ram.s

dumprnd.p: dumprnd.s xorshift.s Makefile
	asl -L dumprnd.s


tst_ram.hex: tst_ram.p Makefile
	p2hex tst_ram.p

tst_ram.bin: tst_ram.p Makefile
	p2bin -l 255 -r '$$-$$' tst_ram.p


dumprnd.hex: dumprnd.p Makefile
	p2hex dumprnd.p

dumprnd.bin: dumprnd.p Makefile
	p2bin -l 255 -r '$$-$$' dumprnd.p



clean:
	rm *.p *.lst *.hex *.bin

