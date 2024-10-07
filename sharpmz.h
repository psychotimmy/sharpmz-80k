// A Sharp MZ-80K emulator for the Raspberry Pi Pico
// Release 1 - Written August - October 2024
//
// The license and copyright notice below apply to all files that make up
// this emulator, including documentation, excepting the z80 core, fatfs,
// and sdcard libraries which have compatible open source licenses. 
//
// The contents of the SP-1002 Monitor and Character ROM are Copyright (c)
// 1979 Sharp Corporation and may be found in the source file sharpcorp.c

// MIT License

// Copyright (c) 2024 Tim Holyoake

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <tusb.h>
#include "pico.h"
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/binary_info.h"
#include "pico/util/datetime.h"
#include "pico/scanvideo.h"
#include "pico/sync.h"
#include "pico/scanvideo/composable_scanline.h"
#include "hardware/gpio.h"
#include "hardware/rtc.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "fatfs/ffconf.h"
#include "fatfs/ff.h"
#include "sdcard/sdcard.h"
#include "sdcard/pio_spi.h"
#include "zazu80/z80.h"

/* Low-level debugging code macros */
//#define USBDEBUGOUTPUT
#undef USBDEBUGOUTOUT

#ifdef USBDEBUGOUTPUT
#define SHOW printf
#else
#define SHOW //
#endif

/* Sharp MZ-80K memory locations */

#define MROMSIZE        4096  //   4   Kbytes Monitor ROM
#define CROMSIZE        2048  //   2   Kbytes Character ROM
#define URAMSIZE 	49152 //   0.5 Kbytes Monitor + 48 Kbytes User RAM
#define VRAMSIZE	1024  //   1   Kbyte  Video RAM
#define FRAMSIZE        1024  //   1   Kbyte  FD ROM (not used at present)

/***************************************************/
/* Sharp MZ-80K memory map summary                 */
/*                                                 */
/* 0x0000 - 0x0FFF  Monitor ROM SP-1002 (or other) */
/*      0 - 4095    4096 bytes                     */
/* 0x1000 - 0x11FF  Monitor stack and work area    */
/*   4096 - 4607    512 bytes                      */
/* 0x1200 - 0xCFFF  User program area (inc. langs) */
/*   4608 - 53247   48640 bytes                    */
/* 0xD000 - 0xDFFF  Video device control area      */
/*  53248 -  57343     First 1024 bytes is VRAM    */
/*                     1000 bytes used for display */
/*                     Remaining 24 bytes "spare"  */
/*                     0xD400-0xDFFF unused addrs  */
/* 0xE000 - 0xEFFF  8255/8253 device control area  */
/*  57344 - 61439      Only first few addrs used   */
/* 0xF000 - 0xFFFF  FD controller ROM (if present) */
/*  61440 - 65535      Only first 1 Kbytes used    */
/*                                                 */
/***************************************************/

/* Pico gpio pin definitions */
#define PICOTONE1 27    // The VGA board uses pins 27 & 28 for sound (pwm)
#define PICOTONE2 28

/* MZ-80K keyboard */
#define KBDROWS 10     // There are 10 rows sensed on the keyboard

/* Emulator status information display - uses the last 40 scanlines */
/* equivalent to 5 rows of 40 characters */
#define EMUSSIZE 200   // Up to 200 bytes of status info stored

/* Maximum tape header and body sizes in bytes */
#define TAPEHEADER	128
#define TAPEBODY	49152

/* sharpmz.c */
extern z80 mzcpu;
extern uint8_t mzuserram[URAMSIZE];
extern uint8_t mzvram[VRAMSIZE];
extern uint8_t mzemustatus[EMUSSIZE];
extern uint8_t mzascii2display(uint8_t);
extern void picoled(uint8_t);

/* sharpcorp.c */
extern const uint8_t mzmonitor[MROMSIZE];
extern const uint8_t cgrom[CROMSIZE];

/* keyboard.c */
extern uint8_t processkey[KBDROWS];
extern void mzmapkey(int32_t*, int8_t);

/* cassette.c */
extern uint8_t cread(void);

/* tapes.c */
extern uint8_t header[TAPEHEADER];
extern uint8_t body[TAPEBODY];
extern uint8_t tapeinit(void);
extern int16_t tapeloader(int16_t);

/* vgadisplay.c */
extern void vga_main(void);

/* 8255.c */
extern uint8_t portA;
extern uint8_t portC;
extern uint8_t cmotor;
extern uint8_t csense;
extern uint8_t vgate;
extern uint8_t vblank;
extern uint8_t read8255(uint16_t addr);
extern void write8255(uint16_t addr, uint8_t data);

/* 8253.c */
extern void p8253_init(void);
extern uint8_t rd8253(uint16_t addr);
extern void wr8253(uint16_t addr, uint8_t data);
extern uint8_t rdE008();
extern void wrE008(uint8_t data);
