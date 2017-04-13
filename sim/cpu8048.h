#ifndef CPU8048_H
#define CPU8048_H


#define PC_MASK (0x0FFF) /* full space is 4kB */
#define PC_INC  (0x07FF) /* increments only within 2kB */

#define SP_MASK (0x07) /* within PSW */
#define SP_INIT (0x00) /* after reset */


#define F0_MASK (0x20) /* within PSW */

#define C_MASK  (0x80) /* within PSW */

#define AC_MASK (0x40)

#define RB_MASK (0x10)



struct state_48
{
	int opcode; // negative value if no opcode yet fetched, as after the reset, otherwise contains positive 8-bit value

	unsigned int cycle_count; // counts CPU cycles from the reset

	uint8_t  reg_a;
	uint8_t  reg_psw;	
	uint16_t reg_pc;

	uint8_t  flag_f1;
	uint8_t  flag_mb;

	uint8_t  in_irq; // whether executing interrupt code (from entry up to RETR)

	uint8_t  t0_pin;
	uint8_t  t1_pin;

	uint8_t  int_n_pin; // /INT interrupt pin state (also JNI command). must be updated when pins are changing as it is level-sensitive
	uint8_t  t_req; // 'TCNT' interrupt request
	uint8_t  t_flag; // flag for JTF command

	uint8_t  i_ena; // 'I' interrupt enable
	uint8_t  t_ena; // 'TCNT' interrupt enable

	uint8_t  t0_clk_ena;

	uint8_t  prescaler;
	uint8_t  timer;

	uint8_t  tmr_mode; // 0 -- timer, 1 -- counter

	uint8_t  tmr_on; // 0 -- stopped, 1 -- started

	uint8_t  t1_pin_prev;

	uint8_t  mem[256]; // cpu-local memory (max possible size)

	uint8_t  port_latches[8]; // ports 4..7 are 4bit in reality
	
	uint8_t  port_dir    [8]; // shows port directions: 0xFF -- output, 0x00 -- input.
	                          // valid for port0 aka BUS and ports 4..7,
	                          // ports 1-2 are OD+pullup type and those bits,
	                          // although maintained, shouldn't be used
};



// external functions (must be implemented in the main code)
//
// memory read/write
uint8_t read_rom_48(uint16_t addr);
uint8_t read_ext_48(uint16_t addr);
void    write_ext_48(uint16_t addr, uint8_t data);
//
// ports read/write
void   out_port(unsigned int cycle_count, uint8_t portnum, uint8_t data); // output data to port latches
uint8_t in_port(unsigned int cycle_count, uint8_t portnum); // in data from port pins
void    set_dir(unsigned int cycle_count, uint8_t portnum, uint8_t data); // used for PORT0 (aka BUS) and for PORTS4..7, that are truly bidirectional.
                                                                         // Ports 1-2 (and 3? :) are quasi-bidir, so when this function is called for them, it should be ignored.
                                                                        // 0xFF is for output, 0x00 is for input


// usable functions (use them to perform emulation)
void reset_48(struct state_48 * s);
int step_48(struct state_48 * s);
void dump_48(struct state_48 * s, FILE *);


// internal functions
uint8_t fetch_48(struct state_48 * s);

uint8_t get_reg_data(struct state_48 * s, uint8_t regnum);
void    set_reg_data(struct state_48 * s, uint8_t regnum, uint8_t data);

void set_c(struct state_48 * s, uint8_t byte);
void set_ac(struct state_48 * s, uint8_t a, uint8_t b, uint8_t res);

uint16_t add(uint8_t a, uint8_t b, uint8_t carry_in);
//uint16_t sub(uint8_t a, uint8_t b, uint8_t carry_in);

uint8_t get_arg_48(struct state_48 * s, uint8_t opcode);

uint8_t get_port_latches(struct state_48 * s, uint8_t portnum);
void    set_port_latches(struct state_48 * s, uint8_t portnum, uint8_t data);
uint8_t get_port_pins   (struct state_48 * s, uint8_t portnum);

void    set_port_dir    (struct state_48 * s, uint8_t portnum, uint8_t data); // 0 -- input, 0xFF -- output


void     push(struct state_48 * s, uint16_t data);
uint16_t pop (struct state_48 * s);

void push_state(struct state_48 * s);
void pop_state (struct state_48 * s, uint8_t update_state);

void jump_8(struct state_48 * s, uint8_t byte);

#endif // CPU8048_H

