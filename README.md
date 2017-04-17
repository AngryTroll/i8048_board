# i8048_board
retro i8048 microcontroller board

directory structure:

pcad/ --  schematics and PCB in pcad2006 format, also contains gerber files (could be viewed by gerbv software) and some photos

sim/ -- simple C simulator, was used for developing of boot software before the board was available

src/ -- some source code, particularly, some tests and serial port python programmer


Features:
 - 8035/8039/8048/8049 (both nmos and cmos) compatible (on the photos there is KP1816BE35 -- soviet clone of 8035 chip)
 
 - IO expander chips: 82(c)43 and 81c55 IO/RAM/timer
 
 - total 12+16+22 = 50 IO pins, one 8bit timer, one 14bit timer, all available at the connectors

 - T0, T1, /INT signals of CPU available at the connectors
 
 - 2K code flash, 256 bytes of movx-RAM. Flash is both hardware and software write-protected

 - usage of 27(c)16 is also possible
 
 - boot code that allows in-system programmability via cheap USB-to-serial converter (/src/boot)
 
 - merged memory, code could be accessed via MOVX command as well as execution from movx-RAM is possible.
   This gives extra flexibility in table accesses as well as allows flash programming during CPU execution.
 
 - 6 MHz (0.4 MIPS) speed
 
 - flexible configurable interrupts from 81c55 timer
 
 - configurable clock source for 81c55 timer
 
 - extra NAND gate and flipflop available at the connectors

 - assembly language tests: 
   * /src/tst_oti: tests pins in output mode and 81c55 timer in square wave generator mode
   * /src/tst_ram: simple PRNG-based memory test
   * (more to come)

 - host programmer software, written in python (/src/host_pgm)

 - Easy shield board developing, prototype boards of different sizes (specifically, 7x9 cm and 8x12 cm) could be used.

 - retro application areas:
   * calculator (with additional shield board)
   * chess player (either with shield board or via serial port)
   * clock/timer with 7-segment or dotted displays
   * (anything imaginable)





Errata:
 - lousy reset circuit that is unable to generate reset at short power outages.
   This connected with the inherent ability of 8035 chips to continue write accesses
   during reset assertions, could easily lead to boot area EEPROM corruption.
 Workaround: deliberately use RO/RW jumper, set RW only during programming operations.

