#!/usr/bin/python3
#
# host programmer for 8048 retro board.
# uses pyserial package.

import io,sys,argparse
import serial
import time











# arguments parser based on argparse package
#
# -r  -- just reset the board using DTR signal, other args are ignored. -j can be added to send 'j' afterwards.
# -nr -- do not reset board with DTR before programming. test for 'boot48' string will be executed anyway,
#        test must pass to continue programming. Default is reset board before programming.
# -nv -- do not commit verification pass after all data is programmed. Default is to perform verification.
# -b  -- switch to binary mode. Default is intelhex mode, when addresses are defined by ihex file and are inside --offs and --size bounds.
#        for binary files, data is always taken from the very beginning of the file and placed from --offs to the ROM, up to --size bytes.
# -j  -- Send 'j' command to the bootloader, which causes it to run ROM from the address 0x100 (main firmware). Default is to leave the device
#        in bootloader.
#
#
#
#
#
#
#
#
#
#
def argument_parser():

	def auto_int(x):
		return int(x,0)
	
	
	parser=argparse.ArgumentParser()

	parser.add_argument('-r',  action='store_true', help='Just reset the board using DTR signal, all other arguments will be ignored')
	parser.add_argument('-nr', action='store_true', help='Do not reset the board with DTR before actual action starts. Default is resetting the board')
	parser.add_argument('-nv', action='store_true', help='Do not verify the written data just after the programming. Default is to verify everything programmed')
	parser.add_argument('-b',  action='store_true', help='Switch to binary mode. Default is intelhex mode. Binary file is always used from the beginning, while intelhex is used according to addresses within.')
	parser.add_argument('-j',  action='store_true', help='Perform \'j\' command (run ROM from 0x100) just after read/write actions are finished. Default is no \'j\', device is left in programming mode.')

	parser.add_argument('--offs', action='store', type=auto_int, default=256,  help='Offset inside ROM. Default is 0x100. Addresses 0x000-0x0FF couldn\'t be written.')
	parser.add_argument('--size', action='store', type=auto_int, default=1792, help='Size to use from the offset. Default is 0x700.')

	parser.add_argument('--read',  action='store', help='File to read the contents of the ROM into. Executed before --write')
	parser.add_argument('--write', action='store', help='File to write to the ROM. Executed after --read')
	parser.add_argument('--check', action='store', help='File to check against ROM contents. Exclusive with both --read and --write')

	parser.add_argument('--port', action='store', help='Serial port to open')

	args=parser.parse_args()

	return args


# just simple args validator
def validate_args(args):

	if args.port==None:
		sys.stderr.write('serial port not given!\n')
		sys.exit(1)

	if args.r==True:
		return

	if args.check==None and args.read==None and args.write==None and args.j==False:
		sys.stderr.write('nothing to do!\n')
		sys.exit(1)

	if args.check!=None and ( args.read!=None or args.write!=None ):
		sys.stderr.write('--check arg must be exclusive with both --read and --write!\n');
		sys.exit(1)

	if args.offs<(0 if args.write==None else 256) or (args.offs+args.size)>2048:
		sys.stderr.write('combination of --offs and --size goes out of allowed range!\n');
		sys.exit(1)

	if args.size<1:
		sys.stderr.write('--size is less than 1!\n')
		sys.exit(1)

	return
	


# perform tasks required by args
def perform(args,rd_file,wr_file,chk_file):


	# parse input file (if any)
	if wr_file!=None or chk_file!=None:
		write_data = parse_write_file(args.b,args.offs,args.size,(chk_file if chk_file!=None else wr_file))


	# open serial port
	serport = serial.Serial()
	#
	serport.port = args.port
	serport.baudrate = 9600
	serport.bytesize = serial.EIGHTBITS
	serport.parity   = serial.PARITY_NONE
	serport.stopbits = serial.STOPBITS_TWO
	serport.timeout  = 0.1
	serport.xonxoff  = False
	serport.rtscts   = False
	serport.dsrdtr   = False
	#
	if args.r==True:
		dtr = 1 # make reset
	elif args.nr==True:
		dtr = 0 # don't make reset
	else:
		dtr = 1 # make reset
	#
	try:
		serport.open()
	except serial.serialutil.SerialException:
		sys.stderr.write('can\'t open serial port!\n')
		sys.exit(1)

	if dtr==1:
		print('Resetting 8048 board...')

	serport.setDTR(True if dtr==0 else False)


	# now port is opened, if dtr was 1 (reset activated), wait at least 0.1 s by timeouting read().
	# assuming that no read data arrives while dtr=1!

	if dtr==1:
		while len(serport.read(size=256))>0:
			pass

	# at least, 0.1 s (serport.timeout) is passed now (in case of no -nr argument), release dtr and continue with the task
	serport.setDTR(True)


	# check whether boot is working (check for 'boot48' reply)
	if args.r==False or args.j==True:
		check_boot_connection(serport)


	# do read task, if needed
	if args.r!=True and (args.read!=None or args.check!=None):
		read_data = read_contents(serport,args.offs,args.size)
	else:
		read_data = None


	# do write task, if needed
	if args.r!=True and args.write!=None:
		program_contents(serport,write_data)


	# do verify task, if needed
	if args.r!=True and args.write!=None and args.nv!=True:
		verify_contents(serport,write_data)

	# do check task, if needed
	if args.r!=True and args.check!=None:
		compare_contents(write_data,read_data)


	# do 'j' command if needed
	if args.j==True:
		serport.write(b'j')
		
		r=serport.read(size=1)
		if len(r)!=1 or r!=b'j':
			sys.stderr.write('warning: sending \'j\' command failed: no response!')
	
	# close port
	serport.close()


	# writeback read file, if needed
	if args.r!=True and args.read!=None:
		write_contents(args.b,args.offs,args.size,read_data,rd_file)


	print('Success!')




# checks wheter boot firmware is functioning.
def check_boot_connection(serport):
	
	#first see if we've already received (or just receiving) 'boot48' string

	boot48 = serport.read(size=256)

	if len(boot48)<7:
		#nothing or crap received: it is possible when we are not resetting board. So then send some 'bad' string, which will cause board to send 'boot48' again
		serport.write(b'\n')
		boot48 = serport.read(size=256)

	if len(boot48)<7 or len(boot48)>8 or boot48[:6]!=b'boot48':
		#size of 'boot48' is 6 bytes, plus 1 or 2 lineend bytes. Otherwise, crap is certainly received, which is unrecoverable
		sys.stderr.write('can\'t communicate with boot firmware!\n')
		sys.exit(1)



################################################################################
# parse file to be written to flash.
def parse_write_file(b,offs,size,filehandle):


	write_contents = []


	if b==True: # binary file

		try:
			array = filehandle.read()
		except (OSError,IOError):
			sys.stderr.write("can't read data from --write or --check file!\n")
			sys.exit(1)

		file_size = len(array)

		if file_size<1:
			sys.stderr.write("--write or --check file has zero length!\n")
			sys.exit(1)

		my_size=min(size,file_size)

		j=0
		for i in range(offs,offs+my_size):
			write_contents.append((i,array[j]))
			j=j+1

	else: # ihex file
		sys.stderr.write("intelhex yet not supported\n")
		sys.exit(1)


	return write_contents

# reads data from ROM
def read_contents(ser,offs,size):

	print('Reading data from boot48...')


	read_data = []

	for i in range(offs,offs+size):

		addr='r{0:04X}'.format(i)
		baddr=bytes(addr,'utf-8')
		ser.write(baddr)

		result=ser.read(size=3) # read answer

		if len(result)!=3:
			sys.stderr.write("error parsing reply to read command!\n")
			sys.exit(1)

		try:
			byte=int(result[0:2],16)
		except ValueError:
			sys.stderr.write("error parsing reply to read command!\n")
			sys.exit(1)
	

		read_data.append((i,byte))

	return read_data

# writes data to file
def write_contents(b,offs,size,read_data,rd_file):


	if b==True: # binary file

		#assume that read_data is sequential (has no holes)

		b=bytearray(1)

		for i in read_data:
			
			b[0] = i[1]

			rd_file.write(b)


	else: # intelhex
		sys.stderr.write("intelhex yet not supported\n")
		sys.exit(1)
		

# programs data to ROM
def program_contents(ser,write_data):

	print('Programming data to boot48...')
	
	
	for el in write_data:

		(addr,data)=el

		wrcmd='w{0:04X}{1:02X}'.format(addr,data)
		bwrcmd=bytes(wrcmd,'utf-8')

		ser.write(bwrcmd)

		result=ser.read(size=3)

		if len(result)!=3 or result!=b'ok\n':
			sys.stderr.write("error parsing reply to write command!\n")
			sys.exit(1)

# verify ROM contents against data previously written
def verify_contents(ser,write_data):

	print('Verifying data just written to boot48...')


	for el in write_data:

		(addr,data)=el

		rdcmd='r{0:04X}'.format(addr)
		brdcmd=bytes(rdcmd,'utf-8')

		ser.write(brdcmd)

		result=ser.read(size=3)

		if len(result)!=3:
			sys.stderr.write("error parsing reply to verify command!\n")
			sys.exit(1)

		try:
			byte=int(result[0:2],16)
		except ValueError:
			sys.stderr.write("error parsing reply to verify command!\n")
			sys.exit(1)

		if byte!=data:
			sys.stderr.write("First verify error at {0:04X}: must be {1:02X}, was {2:02X}!\n".format(addr,data,byte))
			sys.exit(1)


################################################################################
#
###################
# main() function #
###################
#
def main():

	args=argument_parser()
	print(args)
	validate_args(args)



	# opening files, if given
	read_file=None
	write_file=None
	check_file=None
	#
	if args.read!=None:
		try:
			read_file = open(args.read,('wt' if args.b==False else 'wb'))
		except (OSError,IOError):
			if read_file==None:
				sys.stderr.write('can\'t open --read file!\n')
				sys.exit(1)
	
	if args.write!=None:
		try:
			write_file = open(args.write,('rt' if args.b==False else 'rb'))
		except (OSError,IOError):
			if write_file==None:
				sys.stderr.write('can\'t open --write file!\n')
				sys.exit(1)

	if args.check!=None:
		try:
			check_file = open(args.check,('rt' if args.b==False else 'rb'))
		except (OSError,IOError):
			if check_file==None:
				sys.stderr.write('can\'t open --check file!\n')
				sys.exit(1)




	perform(args,read_file,write_file,check_file)




	# close files
	if read_file!=None:
		read_file.close()
	if write_file!=None:
		write_file.close()
	if check_file!=None:
		check_file.close()

	sys.exit(0)

if __name__=='__main__':
	main()

