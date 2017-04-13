#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "cpu8048.h"


// resets the CPU
void reset_48(struct state_48 * s)
{
	int i;


	s->opcode = (-1);

	s->cycle_count = 0;

	s->reg_pc = 0;

	s->flag_mb = 0;
	
	s->flag_f1 = 0;

	s->reg_psw = 0x08;

	s->reg_a = 0;

	s->in_irq = 0;

	s->int_n_pin = 1;
	s->i_ena     = 0;

	s->t_req  = 0;
	s->t_flag = 0;
	s->t_ena  = 0;

	s->t0_clk_ena = 0;

	s->timer     = 0;
	s->prescaler = 0;
	s->tmr_mode  = 0;
	s->tmr_on    = 0;

	s->t0_pin = 0;

	s->t1_pin      = 0;
	s->t1_pin_prev = 0;


	for(i=0;i<sizeof(s->mem);i++)
		s->mem[i] = 0;
	
	for(i=0;i<8;i++)
	{
		s->port_latches[i] = 0xFF;
		s->port_dir[i] = 0;
	}
}


// makes single step, returns number of cycles spent (1 or 2 if normal command, up to 4 if the interrupt also entered)
int step_48(struct state_48 * s)
{
	uint8_t opcode, opcode2;

	uint8_t  arg,a8;
	uint16_t res16, addr;

	uint8_t tmr_inc;

	unsigned int cycles = 1; // intra-command cycle counter

	int i;
	


	// some interrupt logic
	if( !s->t_ena )
		s->t_req = 0;


	// check interrupt conditions
	if( !s->in_irq && ((s->i_ena && !s->int_n_pin) || (s->t_ena && s->t_req)) )
	{
		push_state(s);

		s->reg_pc = (s->i_ena && !s->int_n_pin) ? 3 : 7; // /INT has priority

		s->in_irq = 1;

		if( s->reg_pc == 7 ) // timer interrupt
			s->t_req = 0;

		cycles += 2; // interrupt entry count
	}

	opcode = fetch_48(s);

	s->opcode = opcode;


	switch(opcode)
	{
	///////////////////////////////////////////////////////////////////////////////
		case 0x03:		// ADD A,#imm
		case 0x13:		// ADDC A,#imm
			cycles++;
		case 0x68 ... 0x6F:	// ADD A,Rn
		case 0x60 ... 0x61:	// ADD A,@Rn
		case 0x78 ... 0x7F:	// ADDC A,Rn
		case 0x70 ... 0x71:	// ADDC A,@Rn
		
			arg = get_arg_48(s, opcode);

			res16=add(s->reg_a, arg, ( (opcode & 0x10) && (s->reg_psw & C_MASK) ) ? 1 : 0);
			
			set_c(s,res16>>8);
			set_ac(s,s->reg_a,arg,res16);
			
			s->reg_a = res16 & 255;
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0x53:		// ANL A,#imm
			cycles++;
		case 0x58 ... 0x5F:	// ANL A,Rn
		case 0x50 ... 0x51:	// ANL A,@Rn

			s->reg_a = s->reg_a & get_arg_48(s, opcode);
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0x98:		// ANL BUS,#imm
		case 0x99 ... 0x9A:	// ANL Pp,#imm (p=1..2)
		case 0x9C ... 0x9F:	// ANLD Pp,A (p=4..7, coded 0..3)
			cycles++;

			arg = (opcode & 4) ? s->reg_a : fetch_48(s);

			arg &= get_port_latches(s,opcode);

			set_port_latches(s,opcode,arg);
			set_port_dir(s,opcode,0xFF);
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0x14: case 0x34: case 0x54: case 0x74: // CALL 
		case 0x94: case 0xB4: case 0xD4: case 0xF4:
			cycles++;
			
			addr = ((opcode<<3) & 0x0700) | (fetch_48(s) & 255) | ( (s->flag_mb && !s->in_irq) ? 0x0800 : 0x0000);

			push_state(s);

			s->reg_pc = addr;
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0x27:		// CLR A
			s->reg_a = 0;
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0x97:		// CLR C
			s->reg_psw &= (~C_MASK);
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0xA5:		// CLR F1
			s->flag_f1 = 0;
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0x85:		// CLR F0
			s->reg_psw &= (~F0_MASK);
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0x37:		// CPL A
			s->reg_a ^= 0xFF;
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0xA7:		// CPL C
			s->reg_psw ^= C_MASK;
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0x95:		// CPL F0
			s->reg_psw ^= F0_MASK;
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0xB5:		// CPL F1
			s->flag_f1 ^= 1;
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0x57:		// DA A
			// original docs seem to describe DA A
			//printf("step_48(): DA A not yet done!\n");
			//exit(1);

			arg = 0;
			if( (s->reg_psw&AC_MASK) || (s->reg_a&15)>9 )
				arg += 0x06;

			if( (s->reg_psw&C_MASK) || (s->reg_a>>4)>9 || ( (s->reg_a&15)>9 && (s->reg_a>>4)==9 ) )
			{
				arg += 0x60;
				s->reg_psw |= C_MASK;
			}

			s->reg_a += arg;
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0x07:		// DEC A
			s->reg_a--;
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0xC8 ... 0xCF:	// DEC Rn
			set_reg_data(s,opcode,get_reg_data(s,opcode)-1);
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0x15:		// DIS I
			s->i_ena = 0;
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0x35:		// DIS TCNTI
			s->t_ena = 0;
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0xE8 ... 0xEF:	// DJNZ Rn,addr
			cycles++;

			addr = fetch_48(s);

			set_reg_data(s,opcode,arg=(get_reg_data(s,opcode)-1));

			if( arg )
				jump_8(s,addr);
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0x05:		// EN I
			s->i_ena = 1;
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0x25:		// EN TCNTI
			s->t_ena = 1;
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0x75:		// ENT0 CLK
			s->t0_clk_ena = 1;
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0x08:		// INS A,BUS
		case 0x09 ... 0x0A:	// IN A,Pp, p=1..2
		case 0x0C ... 0x0F:	// MOVD A,Pp (p=4..7, coded 0..3)
			cycles++;

			set_port_dir(s,opcode,0x00);

			s->reg_a = get_port_pins(s,opcode);
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0x17:		// INC A
			s->reg_a++;
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0x18 ... 0x1F:	// INC Rn
			set_reg_data(s,opcode,get_reg_data(s,opcode)+1);
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0x10 ... 0x11:	// INC @Rn
			arg = get_reg_data(s,opcode);
			s->mem[arg]++;
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0x12: case 0x32: case 0x52: case 0x72: // JBn addr
		case 0x92: case 0xB2: case 0xD2: case 0xF2:
			cycles++;

			arg = fetch_48(s);

			if( s->reg_a & (1<<((opcode>>5) & 7)) )
				jump_8(s,arg);
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0xF6:		// JC addr
			cycles++;

			arg = fetch_48(s);

			if( s->reg_psw & C_MASK )
				jump_8(s,arg);
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0xB6:		// JF0 addr
			cycles++;

			arg = fetch_48(s);

			if( s->reg_psw & F0_MASK )
				jump_8(s,arg);
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0x76:		// JF1 addr
			cycles++;

			arg = fetch_48(s);

			if( s->flag_f1 )
				jump_8(s,arg);
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0x04: case 0x24: case 0x44: case 0x64:
		case 0x84: case 0xA4: case 0xC4: case 0xE4: // JMP addr
			cycles++;

			s->reg_pc = ((opcode<<3) & 0x0700) | (fetch_48(s) & 255) | ( (s->flag_mb && !s->in_irq) ? 0x0800 : 0x0000);
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0xB3:		// JMPP @A
			cycles++;

			addr = (s->reg_pc & 0xFF00) | (s->reg_a & 0x00FF);

			arg = read_rom_48(addr);

			s->reg_pc = (s->reg_pc & 0xFF00) | (arg & 0x00FF);
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0xE6:		// JNC addr
			cycles++;

			arg = fetch_48(s);

			if( !(s->reg_psw & C_MASK) )
				jump_8(s,arg);
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0x86:		// JNI addr
			cycles++;

			arg = fetch_48(s);

			if( !s->int_n_pin )
				jump_8(s,arg);
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0x26:		// JNT0 addr
		case 0x36:		// JT0 addr
			cycles++;

			arg = fetch_48(s);

			if( !((opcode & 0x10) ^ (s->t0_pin<<4)) )
				jump_8(s,arg);
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0x46:		// JNT1 addr
		case 0x56:		// JT1 addr
			cycles++;

			arg = fetch_48(s);

			if( !((opcode & 0x10) ^ (s->t1_pin<<4)) )
				jump_8(s,arg);
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0x96:		// JNZ addr
		case 0xC6:		// JZ addr
			cycles++;

			arg = fetch_48(s);

			if( (opcode==0x96) ? s->reg_a : (!s->reg_a) )
				jump_8(s,arg);
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0x16:		// JTF addr
			cycles++;

			arg = fetch_48(s);

			if( s->t_flag )
			{
				jump_8(s,arg);
				s->t_flag = 0;
			}
		break;

		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0x23:		// MOV A,#imm
			cycles++;
		case 0xF8 ... 0xFF:	// MOV A,Rn
		case 0xF0 ... 0xF1:	// MOV A,@Rn
			
			s->reg_a = get_arg_48(s,opcode);
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0xC7:		// MOV A,PSW
			s->reg_a = s->reg_psw;
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0x42:		// MOV A,T
			s->reg_a = s->timer;
		break;
		
	///////////////////////////////////////////////////////////////////////////////
		case 0x62:		// MOV T,A
			s->timer = s->reg_a;
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0xD7:		// MOV PSW,A
			s->reg_psw = s->reg_a | 0x08;
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0xA8 ... 0xAF:	// MOV Rn,A
			set_reg_data(s,opcode,s->reg_a);
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0xB8 ... 0xBF:	// MOV Rn,#imm
			cycles++;
			
			arg = fetch_48(s);

			set_reg_data(s,opcode,arg);
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0xA0 ... 0xA1:	// MOV @Rn,A
			// orig manual says it is 2-cycle instruction?????
			s->mem[get_reg_data(s,opcode)] = s->reg_a;
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0xB0 ... 0xB1:	// MOV @Rn,#imm
			cycles++;

			arg = fetch_48(s);

			s->mem[get_reg_data(s,opcode)] = arg;
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0xA3:		// MOVP A,@A
			cycles++; 

			s->reg_a = read_rom_48( (s->reg_pc & 0xFF00) | (s->reg_a & 0x00FF) );
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0xE3:		// MOVP3 A,@A  (???) 0xE2 in Stashin, Urusov (???), 0xE3 in asl
			cycles++; 

			s->reg_a = read_rom_48( 0x0300 | (s->reg_a & 0x00FF) );
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0x80 ... 0x81:	// MOVX A,@Rn
			cycles++;

			arg = get_reg_data(s,opcode);

			s->reg_a = read_ext_48( ((s->port_latches[2]<<8) & 0x0F00) | (arg & 0x00FF) );
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0x90 ... 0x91:	// MOVX @Rn,A
			cycles++;

			arg = get_reg_data(s,opcode);
			
			write_ext_48( ((s->port_latches[2]<<8) & 0x0F00) | (arg & 0x00FF), s->reg_a );
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0x00:		// NOP
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0x43:		// ORL A,#imm
			cycles++;
		case 0x48 ... 0x4F:	// ORL A,Rn
		case 0x40 ... 0x41:	// ORL A,@Rn

			s->reg_a = s->reg_a | get_arg_48(s, opcode);
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0x88:		// ORL BUS,#imm
		case 0x89 ... 0x8A:	// ORL Pp,#imm (p=1..2)
		case 0x8C ... 0x8F:	// ORLD Pp,A (p=4..7, coded 0..3)
			cycles++;

			arg = (opcode & 4) ? s->reg_a : fetch_48(s);

			arg |= get_port_latches(s,opcode);

			set_port_latches(s,opcode,arg);
			set_port_dir(s,opcode,0xFF);
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0x02:		// OUTL BUS,A
		case 0x39 ... 0x3A:	// OUTL Pp,A (p=1..2)
		case 0x3C ... 0x3F:	// MOVD Pp,A (p=4..7, coded 0..3)
			opcode2 = (opcode==0x02) ? 0x38 : opcode;
			cycles++;

			set_port_latches(s,opcode2,s->reg_a);
			set_port_dir(s,opcode2,0xFF);
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0x83:		// RET
		case 0x93:		// RETR
			cycles++;

			pop_state(s, (opcode & 0x10));

			if( opcode==0x93 )
				s->in_irq = 0;
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0xE7:		// RL A
			s->reg_a = ((s->reg_a<<1) & 0xFE) | ((s->reg_a>>7) & 0x01);
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0xF7:		// RLC A
			arg = ((s->reg_a<<1) & 0xFE) | ((s->reg_psw & C_MASK) ? 0x01 : 0x00);
			
			s->reg_psw &= (~C_MASK);
			s->reg_psw |= (s->reg_a & 0x80) ? C_MASK : 0;
			s->reg_a = arg;
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0x77:		// RR A
			s->reg_a = ((s->reg_a>>1) & 0x7F) | ((s->reg_a<<7) & 0x80);
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0x67:		// RRC A
			arg = ((s->reg_a>>1) & 0x7F) | ((s->reg_psw & C_MASK) ? 0x80 : 0x00);
			
			s->reg_psw &= (~C_MASK);
			s->reg_psw |= (s->reg_a & 0x01) ? C_MASK : 0;
			s->reg_a = arg;
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0xE5:		// SEL MB0
		case 0xF5:		// SEL MB1
			s->flag_mb = (opcode & 0x10) ? 1 : 0;
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0xC5:		// SEL RB0
		case 0xD5:		// SEL RB1
			s->reg_psw &= (~RB_MASK);
			s->reg_psw |= (opcode & 0x10) ? RB_MASK : 0;
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0x65:		// STOP TCNT
			s->tmr_on = 0;
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0x45:		// STRT CNT
			s->tmr_mode = 1;
			s->tmr_on   = 1;
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0x55:		// STRT T
			s->tmr_mode = 0;
			s->tmr_on   = 1;
			s->prescaler = 0;
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0x47:		// SWAP A
			s->reg_a = ((s->reg_a<<4) & 0xF0) | ((s->reg_a>>4) & 0x0F);
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0x28 ... 0x2F:	// XCH A,Rn
			arg = get_reg_data(s,opcode);
			set_reg_data(s,opcode,s->reg_a);
			s->reg_a = arg;
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0x20 ... 0x21:	// XCH A,@Rn
			a8 = get_reg_data(s,opcode);
			arg = s->mem[a8];
			s->mem[a8] = s->reg_a;
			s->reg_a = arg;
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0x30 ... 0x31:	// XCHD A,@Rn
			a8 = get_reg_data(s,opcode);
			arg = s->mem[a8];
			s->mem[a8] = (s->mem[a8] & 0xF0) | (s->reg_a & 0x0F);
			s->reg_a = (s->reg_a & 0xF0) | (arg & 0x0F);
		break;

	///////////////////////////////////////////////////////////////////////////////
		case 0xD3:		// XRL A,#imm
			cycles++;
		case 0xD8 ... 0xDF:	// XRL A,Rn
		case 0xD0 ... 0xD1:	// XRL A,@Rn
			s->reg_a = s->reg_a ^ get_arg_48(s,opcode);	
		break;

	///////////////////////////////////////////////////////////////////////////////
		default:
			fprintf(stderr,"step_48(): unknown opcode=%02x\n",opcode);
			exit(1);
	}








	for(i=0;i<cycles;i++)
	{
		// timer working
		// prescaler
		s->prescaler = (s->prescaler + 1)&31;
		// timer
		if( s->tmr_on )
		{
			tmr_inc = 0;
        
			if( s->tmr_mode ) // counter
			{
				if( !s->t1_pin && s->t1_pin_prev ) tmr_inc = 1;
			}
			else // timer
			{
				if( !s->prescaler ) tmr_inc = 1;
			}
        
			if( tmr_inc )
			{
				s->timer++;
				if( !s->timer )
				{
					s->t_req  = 1;
					s->t_flag = 1;
				}
			}
		}
		//
		s->t1_pin_prev = s->t1_pin;
	}



	s->cycle_count += cycles;

	return cycles;
}










void dump_48(struct state_48 * s, FILE * stream)
{
	int i;

	fprintf(stream, "dmp: pc=%04x, op=%02x, psw=%02x, a=%02x",s->reg_pc, read_rom_48(s->reg_pc), s->reg_psw, s->reg_a);

	for(i=0;i<8;i++)
	{
		fprintf(stream, ", r%1d=%02x",i,(s->reg_psw & RB_MASK) ? s->mem[i+24] : s->mem[i]);
	}
	
	fprintf(stream, "\n");
}

















uint8_t get_reg_data(struct state_48 * s, uint8_t regnum)
{
	uint8_t regaddr = regnum & 7;

	regaddr += (s->reg_psw & RB_MASK) ? 24 : 0;

	return s->mem[regaddr];
}

void set_reg_data(struct state_48 * s, uint8_t regnum, uint8_t data)
{
	uint8_t regaddr = regnum & 7;

	regaddr += (s->reg_psw & RB_MASK) ? 24 : 0;

	s->mem[regaddr] = data;
}



// adds two bytes with carry, returns 16bit result
uint16_t add(uint8_t a, uint8_t b, uint8_t carry_in)
{
	uint16_t res;

	res = a;
	res += b;
	res += carry_in ? 1 : 0;

	return res;
}


// subtracts b from a with carry, returns 16bit result
/*
uint16_t sub(uint8_t a, uint8_t b, uint8_t carry_in)
{
	uint16_t res;

	res = a;
	res -= b;
	res -= carry_in ? 1 : 0;

	return res;
}*/

// sets carry in PSW
void set_c(struct state_48 * s, uint8_t byte)
{
	s->reg_psw = byte ? (s->reg_psw|C_MASK) : (s->reg_psw&(~C_MASK));
}

// sets halfcarry in PSW
void set_ac(struct state_48 * s, uint8_t a, uint8_t b, uint8_t res)
{
	s->reg_psw =  ( ( (a&b) | (a&(~res)) | (b&(~res)) ) & 8 ) ?
	                      (s->reg_psw |   AC_MASK )           :
	                      (s->reg_psw & (~AC_MASK))           ;
}


uint8_t fetch_48(struct state_48 * s)
{
	uint16_t addr;

	addr = s->reg_pc & PC_MASK;

	s->reg_pc = (  s->reg_pc    & (~PC_INC) ) |
	            ( (s->reg_pc+1) &   PC_INC  ) ;

	return read_rom_48(addr); 
}


uint8_t get_arg_48(struct state_48 * s, uint8_t opcode)
{
	uint8_t arg;

	switch( opcode & 0x0F )
	{
		case 0x03:
			arg = fetch_48(s);
		break;
		
		case 0x08 ... 0x0F:
		case 0x00 ... 0x01:
			arg = get_reg_data(s,opcode);
			if( !(opcode&0x08) )
				arg = s->mem[arg];
		break;

		default:
			printf("get_arg_48(): wrong opcode!\n");
			exit(1);
	}

	return arg;
}



uint8_t get_port_latches(struct state_48 * s, uint8_t portnum)
{
	return s->port_latches[portnum & 7];
}

void    set_port_latches(struct state_48 * s, uint8_t portnum, uint8_t data)
{
	s->port_latches[portnum & 7] = data;

	out_port(s->cycle_count, portnum & 7, data);
}

uint8_t get_port_pins   (struct state_48 * s, uint8_t portnum)
{
	return in_port(s->cycle_count, portnum & 7);
}

void    set_port_dir    (struct state_48 * s, uint8_t portnum, uint8_t data)
{
	s->port_dir[portnum & 7] = data;

	set_dir(s->cycle_count, portnum & 7, data);
}

// post-increment (initially sp=0, addresses 8 and 9 used)
void    push(struct state_48 * s, uint16_t data)
{
	uint8_t sp;

	sp = (s->reg_psw & 7)*2 + 8;

	s->mem[sp+0] = data & 255;
	s->mem[sp+1] = data>>8;

	// increment SP in PSW
	s->reg_psw = (s->reg_psw & 0xF8) | ((s->reg_psw+1) & 0x07);
}

// pre-decrement
uint16_t pop (struct state_48 * s)
{
	uint8_t sp;

	// decrement SP in PSW
	s->reg_psw = (s->reg_psw & 0xF8) | ((s->reg_psw-1) & 0x07);

	sp = (s->reg_psw & 7)*2 + 8;
	
	return (s->mem[sp+1]<<8) | s->mem[sp+0];
}


// first PC.lsb is pushed, then high.psw|PC.msb
void    push_state(struct state_48 * s)
{
	uint16_t state;

	// make state
	state = ((s->reg_psw<<8) & 0xF000) | (s->reg_pc & PC_MASK);

	push(s,state);
}

void    pop_state (struct state_48 * s, uint8_t update_state)
{
	uint16_t state = pop(s);

	s->reg_pc = state & PC_MASK;

	if( update_state )
	{
		s->reg_psw = (s->reg_psw & 0x0F) | ((state>>8) & 0xF0);
	}
}


void jump_8(struct state_48 * s, uint8_t byte)
{
	s->reg_pc = (s->reg_pc & 0xFF00) | (byte & 0x00FF);
}


