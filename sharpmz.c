/* Tim Holyoake, 25th August 2024 */

#include "sharpmz.h"

uint8_t mzuserram[URAMSIZE];		// MZ-80K monitor and user RAM
uint8_t mzvram[VRAMSIZE];		// MZ-80K video RAM
uint8_t mzemustatus[EMUSSIZE];          // Emulator status area

z80 mzcpu;			        // Z80 CPU context 
volatile void* unusedv;
volatile z80*  unusedz;

static uint8_t tapeselected=0;

/* Write a byte to RAM or an output device */
void mem_write(void* unusedv, uint16_t addr, uint8_t value)
{
	/* Can't write to monitor ROM or into FD ROM space */
	if ((addr < 0x1000) || (addr > 0xEFFF )) return;
	if (addr < 0xD000) {
		mzuserram[addr-0x1000] = value;
                return;
	}
	if (addr < 0xD400) {
		mzvram[addr-0xD000] = value;
                return;
	}
        if ((addr >= 0xD400) && (addr < 0xE000)) {
		SHOW("Weird write 0x%04x 0x%02x\n",addr,value);
                return;
        }
	if (addr<0xE004) {
                /* This is output - use the 8255 emulation */
		write8255(addr,value);
		return;
	}
	if (addr<0xE008) {
                /* This is output - use the 8253 emulation */
		wr8253(addr,value);
		return;
	}
	if (addr<0xE009) {
		wrE008(value);
		return;
	}

        SHOW("** Writing 0x%02x unused address at 0x%04x **\n",value,addr);
	// Should never be able to get here!
	return;
}

/* Read a byte from memory or input device */
uint8_t mem_read(void* unusedv, uint16_t addr)
{
	if (addr < 0x1000) {
		//SHOW("Mon read 0x%04x is 0x%02x\n",addr,mzmonitor[addr]);
		return mzmonitor[addr];
	}
	if (addr < 0xD000) {
		//SHOW("User RAM read 0x%04x is 0x%02x\n",addr,mzuserram[addr-4096]);
		return mzuserram[addr-0x1000];
	}
	if (addr < 0xD400) {
		//SHOW("VRAM read 0x%04x is 0x%02x\n",addr,mzvram[addr-0xD000]);
		return mzvram[addr-0xD000];
	}
        if ((addr >= 0xD400) && (addr < 0xE000)) {
		SHOW("Reading weird address 0x%04x\n",addr);
                return(0xC7);
        }
        if (addr < 0xE004) {
		return(read8255(addr));
	}
        if (addr < 0xE007) {
		return(rd8253(addr));
	}
        if (addr < 0xE008) {
                SHOW("Address 0x%04x unused\n",addr);
          	return(0xC7); // Unused - RST00 
	}
	if (addr < 0xE009) {
		return(rdE008());
	}

        SHOW("** Reading unused address at 0x%04x **\n",addr);
	return(0xC7); // Unused - RST00
}

/* SIO write to device */
void io_write(z80* unusedz, uint8_t addr, uint8_t val)
{
	/* SIO not used by MZ-80K, so should never get here */
	SHOW("Error: In io_write at 0x%04x with value 0x%02x\n",addr,val);
	return;
}

/* SIO read from device */
uint8_t io_read(z80* unusedz, uint8_t addr)
{
	/* SIO not used by MZ-80K, so should never get here */
	SHOW("Error: In io_read at 0x%04x\n",addr);
       	return(0);
}

/* Convert a Sharp 'ASCII' character to a display character */
/* Painful and incomplete - but good enough for version 1!  */
uint8_t mzascii2display(uint8_t ascii)
{
  uint8_t displaychar;

  switch(ascii) {
    case 0x21: displaychar=0x61;   //!
               break;
    case 0x22: displaychar=0x62;   //"
               break;
    case 0x23: displaychar=0x63;   //#
               break;
    case 0x24: displaychar=0x64;   //$
               break;
    case 0x25: displaychar=0x65;   //%
               break;
    case 0x26: displaychar=0x66;   //&
               break;
    case 0x27: displaychar=0x67;   //'
               break;
    case 0x28: displaychar=0x68;   //(
               break;
    case 0x29: displaychar=0x69;   //)
               break;
    case 0x2a: displaychar=0x6b;   //*
               break;
    case 0x2b: displaychar=0x6a;   //+
               break;
    case 0x2c: displaychar=0x2f;   //,
               break;
    case 0x2d: displaychar=0x2a;   //-
               break;
    case 0x2e: displaychar=0x2e;   //.
               break;
    case 0x2f: displaychar=0x2d;   ///
               break;
    case 0x30: displaychar=0x20;   //0
               break;
    case 0x31: displaychar=0x21;   //1
               break;
    case 0x32: displaychar=0x22;   //2
               break;
    case 0x33: displaychar=0x23;   //3
               break;
    case 0x34: displaychar=0x24;   //4
               break;
    case 0x35: displaychar=0x25;   //5
               break;
    case 0x36: displaychar=0x26;   //6
               break;
    case 0x37: displaychar=0x27;   //7
               break;
    case 0x38: displaychar=0x28;   //8
               break;
    case 0x39: displaychar=0x29;   //9
               break;
    case 0x3a: displaychar=0x4f;   //:
               break;
    case 0x3b: displaychar=0x2c;   //;
               break;
    case 0x3c: displaychar=0x51;   //<
               break;
    case 0x3d: displaychar=0x2b;   //=
               break;
    case 0x3e: displaychar=0x57;   //>
               break;
    case 0x3f: displaychar=0x49;   //?
               break;
    case 0x40: displaychar=0x55;   //@
               break;
    case 0x41: displaychar=0x01;   //A
               break;
    case 0x42: displaychar=0x02;   //B
               break;
    case 0x43: displaychar=0x03;   //C
               break;
    case 0x44: displaychar=0x04;   //D
               break;
    case 0x45: displaychar=0x05;   //E
               break;
    case 0x46: displaychar=0x06;   //F
               break;
    case 0x47: displaychar=0x07;   //G
               break;
    case 0x48: displaychar=0x08;   //H
               break;
    case 0x49: displaychar=0x09;   //I
               break;
    case 0x4a: displaychar=0x0a;   //J
               break;
    case 0x4b: displaychar=0x0b;   //K
               break;
    case 0x4c: displaychar=0x0c;   //L
               break;
    case 0x4d: displaychar=0x0d;   //M
               break;
    case 0x4e: displaychar=0x0e;   //N
               break;
    case 0x4f: displaychar=0x0f;   //O
               break;
    case 0x50: displaychar=0x10;   //P
               break;
    case 0x51: displaychar=0x11;   //Q
               break;
    case 0x52: displaychar=0x12;   //R
               break;
    case 0x53: displaychar=0x13;   //S
               break;
    case 0x54: displaychar=0x14;   //T
               break;
    case 0x55: displaychar=0x15;   //U
               break;
    case 0x56: displaychar=0x16;   //V
               break;
    case 0x57: displaychar=0x17;   //W
               break;
    case 0x58: displaychar=0x18;   //X
               break;
    case 0x59: displaychar=0x19;   //Y
               break;
    case 0x5a: displaychar=0x1a;   //Z
               break;
    case 0x5b: displaychar=0x52;   //[
               break;
    case 0x5c: displaychar=0x59;   //\
               break;
    case 0x5d: displaychar=0x54;   //]
               break;
    case 0x92: displaychar=0x85;   //e
               break;
    case 0x96: displaychar=0x94;   //t
               break;
    case 0x97: displaychar=0x87;   //g
               break;
    case 0x98: displaychar=0x88;   //h
               break;
    case 0x9a: displaychar=0x82;   //b
               break;
    case 0x9b: displaychar=0x98;   //x
               break;
    case 0x9c: displaychar=0x84;   //d
               break;
    case 0x9d: displaychar=0x92;   //r
               break;
    case 0x9e: displaychar=0x90;   //p
               break;
    case 0x9f: displaychar=0x83;   //c
               break;
    case 0xa0: displaychar=0x91;   //q
               break;
    case 0xa1: displaychar=0x81;   //a
               break;
    case 0xa2: displaychar=0x9a;   //z
               break;
    case 0xa3: displaychar=0x97;   //w
               break;
    case 0xa4: displaychar=0x93;   //s
               break;
    case 0xa5: displaychar=0x95;   //u
               break;
    case 0xa6: displaychar=0x89;   //i
               break;
    case 0xa9: displaychar=0x8b;   //k
               break;
    case 0xaa: displaychar=0x86;   //f
               break;
    case 0xab: displaychar=0x96;   //v
               break;
    case 0xaf: displaychar=0x8a;   //j
               break;
    case 0xb0: displaychar=0x8e;   //n
               break;
    case 0xb3: displaychar=0x8d;   //m
               break;
    case 0xb7: displaychar=0x8f;   //o
               break;
    case 0xb8: displaychar=0x8c;   //l
               break;
    case 0xbd: displaychar=0x99;   //y
               break;
    case 0xff: displaychar=0x60;   //pi
               break;
    default:   displaychar=0x00;   //<space> for anything not defined 
               break;
  }

  return(displaychar);
}

/* Clear the last 40 scanlines (emulator status area) */
void mzclearstatus()
{
  for (uint8_t i=0;i<EMUSSIZE;i++)
    mzemustatus[i]=0x00;
}

/* Turn the LED on the pico on or off */
void picoled(uint8_t state)
{
  // state == 1 is on, 0 is off.
  gpio_put(PICO_DEFAULT_LED_PIN, state);
  return;
}

/* Sharp MZ-80K emulator main loop */
int main() 
{
	int32_t  usbc[8]; // USB keyboard buffer
        int8_t   ncodes;  // Number of codes to process in usb keyboard buffer

	//set_sys_clock_khz(200000,true);
        // Set system clock to 175Mhz - see also CMakeLists.txt
        set_sys_clock_pll(1050000000,6,1);
	stdio_init_all();

        gpio_init(PICO_DEFAULT_LED_PIN); // Init onboard pico LED (GPIO 25).
	gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

	sleep_ms(2000);

        SHOW("\n");
	sleep_ms(250);
	SHOW("Hello! My friend\n");
	sleep_ms(250);
	SHOW("Hello! My computer\n");
	sleep_ms(250);
        SHOW("\n");

        // Initialise mzuserram
        uint16_t i; 
        for (i=0;i<URAMSIZE;i++) mzuserram[i]=0x00;

        // Initialise mzemustatus area (bottom 40 scanlines)
        mzclearstatus();

        // Initialise 8253 PIT
        p8253_init();
	SHOW("8253 PIT initialised\n");

        // Initialise the Z80 processor
	z80_init(&mzcpu);
        mzcpu.read_byte = mem_read;
        mzcpu.write_byte = mem_write;
        mzcpu.port_in = io_read;
        mzcpu.port_out = io_write;
        mzcpu.pc = 0x0000;
	SHOW("Z80 processor initialised\n");

	// Initialise USB keyboard
        uint8_t toggle=1;
        picoled(toggle);  // Pico onboard led flashes if no keyboard
	while (!tud_cdc_connected()) { 
          toggle=!toggle;  
          picoled(toggle);
	  sleep_ms(200); 
        }
	SHOW("USB keyboard connected\n");
        picoled(0);

        // Mount the sd card to act as a tape source
        uint8_t tapestatus;
        tapestatus=tapeinit(); 
        tapeselected=tapeloader(3); 
        SHOW("Tape %d selected\n",tapeselected);
        ++tapeselected;

        // Start VGA output on the second core
        multicore_launch_core1(vga_main);
	SHOW("VGA output started on second core\n\n");

	for(;;) {

          z80_step(&mzcpu);		  // Execute next instructions

	  usbc[0]=tud_cdc_read_char();    // Read keyboard, no wait
	  if (usbc[0] != -1) {
            SHOW("Key pressed %x\n",usbc[0]);
            ncodes=1;
            while((usbc[ncodes]=tud_cdc_read_char()) != -1) { 
              SHOW("Key pressed %x\n",usbc[ncodes]);
              ++ncodes;
            }
            if ((usbc[0]==0x7e) && (ncodes==1)) {
              // Preselect next tape (~)
              SHOW("Hello tape\n");
              tapeselected=tapeloader(tapeselected);
              if (tapeselected == -1) 
                tapeselected=0;
              else
                tapeselected++;
            }
            if ((usbc[0]==0x7c) && (ncodes==1)) 
              mzclearstatus();          // Clear status area (|)

	    mzmapkey(usbc,ncodes);	// Map to MZ-80K keyboard
          }
	}

	return(0); // No return from here
}
