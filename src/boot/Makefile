all: boot.s Makefile
	asl -L boot.s
	p2hex boot.p
#	p2bin -r '$$-$$' boot.p 
	p2bin -l 255 -r 0-2047 boot.p 

clean:
	rm *.p *.lst *.hex *.bin

