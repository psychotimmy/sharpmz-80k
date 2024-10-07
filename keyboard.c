#include "sharpmz.h"

// The Sharp MZ-80K Keyboard Map based on SUC magazine article, July 2001.
// Corrected Row 5,key data bit 0x10 to be ; not : as per original article.

// Key data bit  0   1   2   3   4   5   6   7  
//          hex  01  02  04  08  10  20  40  80
//             The key data bit is 0 if pressed

//               !   #   %   '   )   +
// Row 0         1   3   5   7   9   -   G2  G4

//               "   $   &   (   pi
// Row 1         2   4   6   8   0   G1  G3  G5

//               <   <-  ]   @   :   *
// Row 2         Qq  Ee  Tt  Uu  Oo  =   G7  G9

//               >   [   \   ?   ^
// Row 3         Ww  Rr  Yy  Ii  Pp  G6  G8  G10

//               SPD DIA 
// Row 4         Aa  Dd  Gg  Jj  Ll  £   G12 G14

//               HRT CLB
// Row 5         Ss  Ff  Hh  Kk  ;   G11 G13 G15

//               ->                  SML
// Row 6         Zz  Cc  Bb  Mm  .   CAP G17 G19

//               v
// Row 7         Xx  Vv  Nn  ,   /   G16 G18 G20

//                   INS --- RC
// Row 8         LSH DEL --- LC  CR  RSH G22 G24

//               CLR     UP      ---
// Row 9         HOM SPC DWN BRK --- G21 G23 G25

// SML/CAP key is a toggle. If SML, then the 3rd character on
// the key (either lower case or a graphic) is input.

// Design decision 1 - map usb lower case keys to upper case
// to better mimic the way the MZ-80K keyboard works.

/* Globals */

uint8_t processkey[KBDROWS] = {		// Return keyboard characters
0xFF,0xFF,0xFF,0xFF,0xFF,               // All 0xFF means no key to process
0xFF,0xFF,0xFF,0xFF,0xFF
};

static uint8_t smlcapled = 0;

/* Convert USB key press to the MZ-80K keyboard map, 
   then store in the processkey[] global (read on portB by the 8255) */
void mzmapkey(int32_t *usbc, int8_t ncodes) 
{
  if (ncodes==1) {
    switch(usbc[0]) {

      case 0x03: processkey[8]=0x01^0xFF; //shift break (ctrl C)
                 processkey[9]=0x08^0xFF;
                 break;

      /* Unshifted keys */
      case 0x08: processkey[8]=0x02^0xFF; //<DEL>   (USB backspace, ctrl H)
                 break;
      case 0x09: processkey[9]=0x08^0xFF; //<BREAK> (USB Tab key)
                 break;
      case 0x0d: processkey[8]=0x10^0xFF; //<CR>
                 break;
      case 0x20: processkey[9]=0x02^0xFF; //<SPACE>
                 break;
      case 0x21: processkey[8]=0x01^0xFF; //!
                 processkey[0]=0x01^0xFF;
                 break;
      case 0x22: processkey[8]=0x01^0xFF; //"
                 processkey[1]=0x01^0xFF;
                 break;
      case 0x23: processkey[8]=0x01^0xFF; //#
                 processkey[0]=0x02^0xFF;
                 break;
      case 0x24: processkey[8]=0x01^0xFF; //$
                 processkey[1]=0x02^0xFF;
                 break;
      case 0x25: processkey[8]=0x01^0xFF; //%
                 processkey[0]=0x04^0xFF;
                 break;
      case 0x26: processkey[8]=0x01^0xFF; //&
                 processkey[1]=0x04^0xFF;
                 break;
      case 0x27: processkey[8]=0x01^0xFF; //'
                 processkey[0]=0x08^0xFF;
                 break;
      case 0x28: processkey[8]=0x01^0xFF; //(
                 processkey[1]=0x08^0xFF;
                 break;
      case 0x29: processkey[8]=0x01^0xFF; //)
                 processkey[0]=0x10^0xFF;
                 break;
      case 0x2a: processkey[8]=0x01^0xFF; //*
                 processkey[2]=0x20^0xFF;
		 break;
      case 0x2b: processkey[8]=0x01^0xFF; //+
                 processkey[0]=0x20^0xFF;
	 	 break;
      case 0x2c: processkey[7]=0x08^0xFF; //,
                 break;
      case 0x2d: processkey[0]=0x20^0xFF; //-
                 break;
      case 0x2e: processkey[6]=0x10^0xFF; //.
                 break;
      case 0x2f: processkey[7]=0x10^0xFF; ///
                 break;
      case 0x30: processkey[1]=0x10^0xFF; //0  
                 break;
      case 0x31: processkey[0]=0x01^0xFF; //1  
                 break;
      case 0x32: processkey[1]=0x01^0xFF; //2  
                 break;
      case 0x33: processkey[0]=0x02^0xFF; //3  
                 break;
      case 0x34: processkey[1]=0x02^0xFF; //4  
                 break;
      case 0x35: processkey[0]=0x04^0xFF; //5  
                 break;
      case 0x36: processkey[1]=0x04^0xFF; //6  
                 break;
      case 0x37: processkey[0]=0x08^0xFF; //7  
                 break;
      case 0x38: processkey[1]=0x08^0xFF; //8  
                 break;
      case 0x39: processkey[0]=0x10^0xFF; //9  
                 break;

      /* Shifted characters - often graphics */
      case 0x3a: processkey[8]=0x01^0xFF; //:
                 processkey[2]=0x10^0xFF;
                 break;
      case 0x3b: processkey[5]=0x10^0xFF; //;
                 break;
      case 0x3c: processkey[8]=0x01^0xFF; //<
                 processkey[2]=0x01^0xFF;
                 break;
      case 0x3d: processkey[2]=0x20^0xFF; //=
                 break;
      case 0x3e: processkey[8]=0x01^0xFF; //>
                 processkey[3]=0x01^0xFF;
                 break;
      case 0x3f: processkey[8]=0x01^0xFF; //?
                 processkey[3]=0x08^0xFF;
                 break;
      case 0x41: processkey[8]=0x01^0xFF; //spade (a)
                 processkey[4]=0x01^0xFF;
                 break;
      case 0x42: processkey[8]=0x01^0xFF; //diagonal fill top right (b)
                 processkey[6]=0x04^0xFF;
                 break;
      case 0x43: processkey[8]=0x01^0xFF; //filled block (c)
                 processkey[6]=0x02^0xFF;
                 break;
      case 0x44: processkey[8]=0x01^0xFF; //diamond (d)
                 processkey[4]=0x02^0xFF;
                 break;
      case 0x45: processkey[8]=0x01^0xFF; //left arrow (e)
                 processkey[2]=0x02^0xFF;
                 break;
      case 0x46: processkey[8]=0x01^0xFF; //club (f)
                 processkey[5]=0x02^0xFF;
                 break;
      case 0x47: processkey[8]=0x01^0xFF; //filled circle (g)
                 processkey[4]=0x04^0xFF;
                 break;
      case 0x48: processkey[8]=0x01^0xFF; //circle (h)
                 processkey[5]=0x04^0xFF;
                 break;
      case 0x49: processkey[8]=0x01^0xFF; //? (i)
                 processkey[3]=0x08^0xFF;
                 break;
      case 0x4a: processkey[8]=0x01^0xFF; //circle with filled border (j)
                 processkey[4]=0x08^0xFF;
                 break;
      case 0x4b: processkey[8]=0x01^0xFF; //lower right arc (k)
                 processkey[5]=0x08^0xFF;
                 break;
      case 0x4c: processkey[8]=0x01^0xFF; //lower left arc (l)
                 processkey[4]=0x10^0xFF;
                 break;
      case 0x4d: processkey[8]=0x01^0xFF; //diagonal fill lower left (m)
                 processkey[6]=0x08^0xFF;
                 break;
      case 0x4e: processkey[8]=0x01^0xFF; //diagonal fill lower right (n)
                 processkey[7]=0x04^0xFF;
                 break;
      case 0x4f: processkey[8]=0x01^0xFF; //: (o)
                 processkey[2]=0x10^0xFF;
                 break;
      case 0x50: processkey[8]=0x01^0xFF; //up arrow (p)
                 processkey[3]=0x10^0xFF;
                 break;
      case 0x51: processkey[8]=0x01^0xFF; //< (q)
                 processkey[2]=0x01^0xFF;
                 break;
      case 0x52: processkey[8]=0x01^0xFF; //[ (r)
                 processkey[3]=0x02^0xFF;
                 break;
      case 0x53: processkey[8]=0x01^0xFF; //heart (s)
                 processkey[5]=0x01^0xFF;
                 break;
      case 0x54: processkey[8]=0x01^0xFF; //] (t)
                 processkey[2]=0x04^0xFF;
                 break;
      case 0x55: processkey[8]=0x01^0xFF; //@ (u)
                 processkey[2]=0x08^0xFF;
                 break;
      case 0x56: processkey[8]=0x01^0xFF; //diagonal fill upper left (v)
                 processkey[7]=0x02^0xFF;
                 break;
      case 0x57: processkey[8]=0x01^0xFF; //> (w)
                 processkey[3]=0x01^0xFF;
                 break;
      case 0x58: processkey[8]=0x01^0xFF; //down arrow (x)
                 processkey[7]=0x01^0xFF;
                 break;
      case 0x59: processkey[8]=0x01^0xFF; //\ (y)
                 processkey[3]=0x04^0xFF;
                 break;
      case 0x5a: processkey[8]=0x01^0xFF; //right arrow (z)
                 processkey[6]=0x01^0xFF;
                 break;

      case 0x5c: processkey[8]=0x01^0xFF; //backslash
                 processkey[3]=0x04^0xFF;
                 break;
      case 0x5e: processkey[8]=0x01^0xFF; //pi
                 processkey[1]=0x10^0xFF;
		 break;

      /* Unshifted keys - letters */
      case 0x61: processkey[4]=0x01^0xFF; //A  
                 break;
      case 0x62: processkey[6]=0x04^0xFF; //B  
                 break;
      case 0x63: processkey[6]=0x02^0xFF; //C  
                 break;
      case 0x64: processkey[4]=0x02^0xFF; //D
                 break;
      case 0x65: processkey[2]=0x02^0xFF; //E
                 break;
      case 0x66: processkey[5]=0x02^0xFF; //F
                 break;
      case 0x67: processkey[4]=0x04^0xFF; //G
                 break;
      case 0x68: processkey[5]=0x04^0xFF; //H
                 break;
      case 0x69: processkey[3]=0x08^0xFF; //I
                 break;
      case 0x6A: processkey[4]=0x08^0xFF; //J
                 break;
      case 0x6B: processkey[5]=0x08^0xFF; //K
                 break;
      case 0x6C: processkey[4]=0x10^0xFF; //L
                 break;
      case 0x6D: processkey[6]=0x08^0xFF; //M
                 break;
      case 0x6E: processkey[7]=0x04^0xFF; //N
                 break;
      case 0x6F: processkey[2]=0x10^0xFF; //O
                 break;
      case 0x70: processkey[3]=0x10^0xFF; //P
                 break;
      case 0x71: processkey[2]=0x01^0xFF; //Q
                 break;
      case 0x72: processkey[3]=0x02^0xFF; //R
                 break;
      case 0x73: processkey[5]=0x01^0xFF; //S
                 break;
      case 0x74: processkey[2]=0x04^0xFF; //T
                 break;
      case 0x75: processkey[2]=0x08^0xFF; //U
                 break;
      case 0x76: processkey[7]=0x02^0xFF; //V
                 break;
      case 0x77: processkey[3]=0x01^0xFF; //W
                 break;
      case 0x78: processkey[7]=0x01^0xFF; //X
                 break;
      case 0x79: processkey[3]=0x04^0xFF; //Y
                 break;
      case 0x7a: processkey[6]=0x01^0xFF; //Z
                 break;

      default: break;                     // Anything we don't understand
                                          // is ignored
    }
  }

  if ((ncodes==3) && (usbc[0]==0x1B)) {
    if (usbc[1]==0x5B) {
      switch(usbc[2]) {
        case 0x41: processkey[8]=0x01^0xFF; //up arrow
                   processkey[9]=0x04^0xFF;
                   break;
        case 0x42: processkey[9]=0x04^0xFF; //down arrow
                   break;
        case 0x43: processkey[8]=0x08^0xFF; //left arrow
                   break;
        case 0x44: processkey[8]=0x09^0xFF; //right arrow
                   break;
        default:   break;                   // Ignore unknown codes
      }
    }
    if (usbc[1]==0x4F) {
      switch(usbc[2]) {
        case 0x46: processkey[8]=0x01^0xFF;  //clear screen (CLR) 
                   processkey[9]=0x01^0xFF;
                   break;
        default:   break;                    //Ignore unknown codes
      }
    }
  }

  if ((ncodes==4) && (usbc[0]==0x1B) &&
      (usbc[1]==0x5B) && (usbc[3]==0x7E)) {
    switch(usbc[2]) {
      case 0x31: processkey[9]=0x01^0xFF;  //home (HOME)
                 break;
      case 0x32: processkey[8]=0x03^0xFF;  //insert (INS)
                 break;
      case 0x33: processkey[8]=0x02^0xFF;  //delete (DEL)
                 break;

      // SML/CAPS toggle. portC bit2 is 1 at boot (green led).
      // When latched, this sets portC bit 2 to 0 (red led) and affects
      // the characters displayed - e.g. A becomes a. Even though the
      // SML/CAPS key is latched on the MZ-80K keyboard, it's treated as
      // a shifted character, hence the need to set processkey[8].
      // picoled() is used to turn the inbuilt pico led on (SML) or off.

      case 0x35: if ((portC>>2)&0x01)      //SML/CAPS toggle
                   processkey[8]=0x01^0xFF;
                 processkey[6]=0x20^0xFF;  
                 smlcapled=!smlcapled;
                 picoled(smlcapled);
                 break;
      case 0x36: processkey[9]=0x08^0xFF;  //break (BREAK)
                 break;
      default:   break;                    //Ignore unknown codes
    }
  }

  return;
}
