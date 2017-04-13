#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "ihexread.h"
#include "cpu8048.h"

#define ROM_SIZE 2048
#define EXT_SIZE 256

uint8_t ROM[2048];
uint8_t EXT[256];

struct state_48 s;


#define UART_SNDBF_SIZE 256
#define UART_SNDBF_MASK 255

volatile uint8_t uart_sndbf[UART_SNDBF_SIZE];

volatile unsigned int uart_sndbf_head=0;
volatile unsigned int uart_sndbf_tail=0;


volatile uint8_t uart_send_bit;


void uart_recv(unsigned int cycle, uint8_t bit);
uint8_t uart_send(unsigned int cycle);

int uart_try_send(char *);
void uart_poll_stdin(void);

int main(int argc, char ** argv)
{
	int i;
	FILE * hex;


	for(i=0;i<ROM_SIZE;i++)
		ROM[i] = 0x01;
	//
	ROM[0] = 0x24; ROM[1] = 0x00; // jmp $100
	ROM[3] = 0x24; ROM[4] = 0x03; // jmp $103
	ROM[7] = 0x24; ROM[8] = 0x07; // jmp $107


	if( argc<=1 )
	{
		printf("please give argument!\n");
		exit(1);
	}

	hex=fopen(argv[1],"r");
	if( !hex )
	{
		printf("%s(): can't open hex file %s!\n",__func__,argv[1]);
		exit(1);
	}
	ihex_read(hex,ROM,0,2048);


	fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);


	// start execution
	reset_48(&s);
	//
	while(1)
	{
		if( argc<=2 ) dump_48(&s, stderr);
		step_48(&s);
		uart_recv(s.cycle_count, (s.port_latches[2]&0x40));
		uart_send_bit = uart_send(s.cycle_count);

		uart_poll_stdin();
	}








	fclose(hex);

	return 0;
}



uint8_t read_rom_48(uint16_t addr)
{
	if( 0<=addr && addr<ROM_SIZE )
		return ROM[addr];
	else if( ROM_SIZE<=addr && addr<(ROM_SIZE+EXT_SIZE) )
		return EXT[addr-ROM_SIZE];
	else
	{
		printf("%s(): invalid address %x\n",__func__,addr);
		exit(1);
	}
}

uint8_t read_ext_48(uint16_t addr)
{
	if( 0<=addr && addr<ROM_SIZE )
		return ROM[addr];
	else if( ROM_SIZE<=addr && addr<(ROM_SIZE+EXT_SIZE) )
		return EXT[addr-ROM_SIZE];
	else
	{
		printf("%s(): invalid address %x\n",__func__,addr);
		exit(1);
	}
}

void    write_ext_48(uint16_t addr, uint8_t data)
{
	if( ROM_SIZE<=addr && addr<(ROM_SIZE+EXT_SIZE) )
	{
		EXT[addr-ROM_SIZE]=data;
	}
	else if( 0<=addr && addr<ROM_SIZE )
	{
//		printf("%s(): ROM write addr=%04x, data=%02x\n",__func__,addr,data);
	}
	else
	{
		printf("%s(): invalid address %x\n",__func__,addr);
		exit(1);
	}
}


void set_dir(unsigned int cycle_count, uint8_t port, uint8_t data)
{}

void out_port(unsigned int cycle_count, uint8_t port, uint8_t data)
{
	if( port==2 )
	{
//printf("%s(): data=%02x\n",__func__,data);
		//uart_recv(cycle_count, (data&0x40));
	}
}

uint8_t in_port(unsigned int cycle_count, uint8_t port)
{
	uint8_t val;

	val = s.port_latches[port];
	
	if( port==2 )
	{
		//return uart_send_bit ? 0xEF : 0x6F; // RxD is bit 7, /PROG is bit 4
		val &= (uart_send_bit ? 0xEF : 0x6F);
	}

	return val;
}



// UART receive at 9600 baud (41 2/3 cycles per bit)
void uart_recv(unsigned int curr_cycle, uint8_t curr_bit) // input is cycle count and TxD bit (1 or 0)
{
	static int init = 1;

	static unsigned int prev_cycle;
	static uint8_t prev_bit;

	const float BAUD_TIME = (41.66666666666666666666666666);


/*	static int state;
	const int ST_IDLE=0;
	const int ST_STRT=1;
	const int ST_BITS=2;
	const int ST_STOP=3;
*/
	static enum { ST_IDLE, ST_STRT, ST_BITS, ST_STOP } state;


	static int uart_cycles;
	static int bitcount;
	static int word;


	unsigned int cycle;
	uint8_t bit;


	if( init )
	{
		prev_cycle = curr_cycle;
		init = 0;
		state = ST_IDLE;
	}


//printf("%s(): cycle=%d, bit=%02x\n",__func__,curr_cycle,!!curr_bit);


	if( curr_cycle==prev_cycle )
	{
		prev_bit = curr_bit;
		return;
	}



	cycle = init ? 0 : 1;
	init = 0;

	while( cycle<=(curr_cycle-prev_cycle) )
	{
		bit = (cycle==(curr_cycle-prev_cycle)) ? curr_bit : prev_bit;
//printf("%s(): cycle=%d, bit=%02x, state=%d\n",__func__,cycle,bit,state);
		switch(state)
		{
		case ST_IDLE:
			if( !bit )
			{
				uart_cycles=0;
				state=ST_STRT;
			}
		break;
		case ST_STRT:
			uart_cycles++;
			if( uart_cycles>=(BAUD_TIME/2.0) )
			{
				if( bit ) // wrong start bit
				{
					state = ST_IDLE;
//printf("%s(): wrong start bit!\n",__func__);
				}
				else // start bit OK
				{
					state = ST_BITS;
					bitcount = 1;
					word = 0;
				}
			}
		break;
		case ST_BITS:
			uart_cycles++;
			if( uart_cycles>=(BAUD_TIME*(0.5+bitcount)) )
			{
				bitcount++;
				// collect bits
				word >>= 1;
				word |= bit ? 0x80 : 0;

				if( bitcount>=9 )
				{
					state = ST_STOP;
				}
			}
		break;
		case ST_STOP:
			uart_cycles++;
			if( uart_cycles>=(BAUD_TIME*9.5) )
			{
				if( bit ) // stop bit OK
				{
					fprintf(stdout,"%c",word);fflush(stdout);
				}
//else printf("%s(): wrong stop bit!\n",__func__);

				state = ST_IDLE;
			}
		break;
		default:
		break;
		}

		cycle++;
	}
//printf("%s(): end of while loop\n",__func__);


	prev_cycle = curr_cycle;
	prev_bit   = curr_bit;
}


uint8_t uart_send(unsigned int curr_cycle)
{ // sends bytes from uart_sndbuf. called as CPU inputs from uart pin, returns its required state (0 or !0)

	static int init = 1;

	static unsigned int prev_cycle;

	const float BAUD_TIME = (41.66666666666666666666666666);

	static enum { ST_IDLE, ST_STRT, ST_BITS, ST_STOP } state;

	static int uart_cycles;
	static int bitcount;
	static int word;

	unsigned int cycle;
	static uint8_t bit=1;

	if( init )
	{
		prev_cycle = curr_cycle;
		init = 0;
		state = ST_IDLE;
		bit = 1;
	}

	cycle = init ? 0 : 1;
	init = 0;

	while( cycle<=(curr_cycle-prev_cycle) )
	{
//printf("%s(): cycle=%d, bit=%02x, state=%d\n",__func__,cycle,bit,state);
		switch(state)
		{
		case ST_IDLE:
			// see if there are bytes to send
			if( uart_sndbf_head!=uart_sndbf_tail )
			{
				word = uart_sndbf[uart_sndbf_tail];
				uart_sndbf_tail = (uart_sndbf_tail>=(UART_SNDBF_SIZE-1)) ? 0 : (uart_sndbf_tail+1);
				bitcount = 1;
				state = ST_BITS;
				uart_cycles=0;
				bit = 0;
//printf("%s(): got byte to send: %c\n",__func__,word);
			}
		break;
		case ST_BITS:
			uart_cycles++;
			if( uart_cycles>=(BAUD_TIME*((float)bitcount)) )
			{
				bitcount++;
				
				bit = (word&1);
				word >>= 1;
				word |= 0x80;

				if( bitcount>=11 )
				{
					state = ST_IDLE;
				}
			}
		break;
		default:
		break;
		}

		cycle++;
	}
//printf("%s(): end of while loop\n",__func__);

	prev_cycle = curr_cycle;

	return bit;
}

int uart_try_send(char * str)
{ // try sending given string by adding it to the uart_sndbuf, if ever possible.
  // returns 0 of failed to send (nothing added to uart_sndbuf), 1 if all string is sent.

	size_t stsz;
	unsigned int bffree;
	int i;


	if( !str )
		return 0;

	stsz=strlen(str);
	if( !stsz )
		return 0;


	bffree = UART_SNDBF_SIZE - ((uart_sndbf_head - uart_sndbf_tail) & UART_SNDBF_MASK);

	if( stsz<bffree ) // max bytes in fifo 255 (!), not 256
	{
		for(i=0;i<stsz;i++)
		{
			uart_sndbf[uart_sndbf_head] = str[i];
			uart_sndbf_head = (uart_sndbf_head>=(UART_SNDBF_SIZE-1)) ? 0 : (uart_sndbf_head+1);
//printf("%s(): sent byte %c to fifo\n",__func__,str[i]);
		}

		return 1;
	}
	
	return 0;
}


void uart_poll_stdin(void)
{
	char ts[2];
	ssize_t r;

	while( (r=read(0,ts,1))>0 )
	{
		ts[1]=0;
//printf("char: %s\n",ts);
		uart_try_send(ts);
	}

	if( r<0 && errno!=EWOULDBLOCK && errno!=EAGAIN )
	{
		printf("%s(): read() error! %d\n",__func__,errno);
		exit(1);
	}
}

