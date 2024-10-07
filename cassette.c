/* Sharp MZ-80K tape handling        */
/* Tim Holyoake, August-October 2024 */

#include "sharpmz.h"

#define LONGPULSE  1     /* cread() returns high for a long pulse */
#define SHORTPULSE 0     /*                 low for a short pulse */

#define RBGAP_L 120      /* Big tape gap length in bits - read  */
#define WBGAP_L 22000    /* Big tape gap length in bits - write */
#define RSGAP_L 120      /* Small tape gap length - read        */
#define WSGAP_L 11000    /* Small tape gap length - write       */
#define BTM_L  80        /* Big tape mark length                */
#define STM_L  40        /* Small tape mark length              */
#define L_L    1         /* One long pulse length               */
#define S256_L 256       /* 256 short pulses length             */
#define HDR_L  1024      /* Header 128 bytes, 1024 bits         */
#define CHK_L  16        /* Checksum 2 bytes, 16 bits           */

static uint8_t  crstate=0;  // Holds tape state for cread()
static uint8_t  cwstate=0;  // Holds tape state for cwrite()

/* MZ-80K tapes always have a 128 byte header, followed by a body */

// Tape format is as follows (long pulse = 1, short pulse =0): 

// ** Header preamble **
// bgap - big gap - 22,000 short pulses (only need to send >100 when reading)
// btm -  big tape mark - 40 long, 40 short
// l - 1 long pulse

// ** Tape header **
// hdr - tape header - 128 bytes (1024 bits).
// chkh - header checksum - 2 bytes.
// l - 1 long pulse
// 256s - 256 short pulses
// hdrc - a copy of the tape header
// chkh - a copy of the header checksum
// l - 1 long pulse

// ** Body preamble **
// sgap - small gap - 11,000 short pulses (only need to send >100 when reading)
// stm - small tape mark - 20 long, 20 short
// l - 1 long pulse

// ** Tape body **
// file - variable length, set in header.
// chkf - file checksum - 2 bytes.
// l - 1 long pulse
// 256s - 256 short pulses
// filec - a copy of file
// chkf - a copy of the file checksum
// l - 1 long pulse

// If the header and body are read successfully on the first attempt, the 
// process ends and the second copy is not read. This implementation assumes
// the first read is always successful, as we're using .mzf files rather
// than a real tape.

/* Read an MZ-80K format tape one bit at a time */
/* Pseudo finite state machine implementation */

uint8_t cread()
{
                             // Used to calculate the bit to output from tape
  uint8_t bitshift;          // to the MZ-80K when reading the header or body
  static uint16_t chkbits;   // Tracks number of long pulses sent in the header
                             // or body to enable the checksum to be calculated
                             // MUST be a 16 bit unsigned value
  static uint16_t bodylen;   // Length of tape body as declared in the header
  static uint8_t longsent;   // Tracks if a long pulse has been sent before 
                             // each new byte of the header, checksums and body
  static uint8_t checksum[2];// Stores the calculated checksum
  static uint8_t hilo;       // Used for the 1 -> tape bit read -> 0 logic
  static uint32_t secbits;   // Tracks where we are in the current tape section

  if (cmotor==0) {
    if (crstate==0) {
      return(LONGPULSE); // Motor is off and we're not reading a tape
    }
    else {               // Motor is off and we've part read a tape
      hilo=0;            // Reset hilo counter for next time motor is on
      return(LONGPULSE);
    }
  }

  // If we reach here, the motor is running and sense has been triggered

  // To mimic a tape being read, we need to surround each bit sent with a
  // high bit and a low bit. The lines below do this in the simplest way
  // possible ... by using modulo 3 arithmetic

  hilo=(hilo+1)%3;           // Sequence is 1, followed by the tape bit (0/1),
  if (hilo<2) return(hilo);  // followed by 0 until end of tape is reached

  // Initialise local statics if state is 0, transition to state 1
  if (crstate==0) {
    secbits=0;
    chkbits=0;
    longsent=0;
    crstate=1;
    // We don't return here - always fall through to state 1 immediately
  }
 
  /* Header preamble - bgap, btm, l - state 1 */
  // Note - 22,000 pulses in a real bgap, but anything > 100 will work
  // when a tape is being read (writing is different!) 
  if (crstate==1) {
    if (secbits<RBGAP_L) {
      ++secbits;
      return(SHORTPULSE);
    }
    if (secbits<RBGAP_L+(BTM_L/2)) {  /* First half of btm is long pulses */
      ++secbits;
      return(LONGPULSE);
    }
    if (secbits<RBGAP_L+BTM_L) {
      ++secbits;
      return(SHORTPULSE);
    }
    secbits=0;
    crstate=2;
    return(LONGPULSE);
  }

  /* First copy of the header */
  if (crstate==2) {
    if (secbits<HDR_L) {
      /* One LONGPULSE is sent before every byte of the header */
      if (((secbits%8)==0) && (longsent==0)) {
        /* Note - we don't increment secbits here */
        longsent=1;
        return(LONGPULSE);
      }
      longsent=0;
      /* Bytes are sent starting with bit 7 (msb) */
      bitshift=secbits%8;
      if (((header[secbits++/8]<<bitshift)&0x80) == 0x80) {
        ++chkbits; // Increment the long pulse count for calculating chkh
        return(LONGPULSE);
      }
      return(SHORTPULSE);
    }
    /* At the end of the header, move onto checksum state (3) */
    secbits=0;
    crstate=3;
  }

  /* Header checksum - state 3 */
  if (crstate==3) {
    if (secbits<CHK_L) {
      if ((secbits==0)&&(chkbits>0)) {
        // Need to calculate the header checksum
        // Note as chkbits is a uint16_t, we don't need to do modulo 2^16
        // as this will be taken care of for us
        checksum[0]=(chkbits>>8)&0xFF; /* MSB of the checksum */
        checksum[1]=chkbits&0xFF;      /* LSB of the checksum */
        SHOW("Header checksum is 0x%04x 0x%02x 0x%02x\n",chkbits,checksum[0],checksum[1]);
        // Reset chkbits for the next time a checksum is calculated
        chkbits=0;
      }
      if (((secbits % 8) == 0) && (longsent == 0)) {
        /* Note - we don't increment secbits here */
        longsent = 1;
        return(LONGPULSE);
      }
      /* Reset the longsent flag */
      longsent = 0;
      /* Bytes are sent starting with the MSB */
      bitshift=secbits%8;
      if (((checksum[secbits++/8]<<bitshift)&0x80) == 0x80) {
        return(LONGPULSE);
      }
      return(SHORTPULSE);
    }
    /* At the end of the checksum */
    /* Note - current assumption is that this is correct */
    /* Reasonable - as this isn't a real cassette tape */
    /* Saves a little time and complexity */
    secbits=0;
    crstate=7; 
  }

  /* States 4,5 and 6 are only required if the header checksum failed */
  /* The emulator uses .mzf files and assumes the copy of the header */
  /* is not required, but these states are reserved in case the ability */
  /* to use .wav tape copies is ever implemented */
  /* State 4 - one long, 256 short pulses */
  /* State 5 - header copy */
  /* State 6 - header checksum copy */

  /* We now have a long pulse, short gap, short tape mark and long pulse */
  /* before we finally get to the tape body */

  if (crstate==7) {
    if (secbits<L_L) {
      ++secbits;
      return(LONGPULSE);
    }
    if (secbits<L_L+RSGAP_L) {
      ++secbits;
      return(SHORTPULSE);
    }
    if (secbits<L_L+RSGAP_L+(STM_L/2)) {
      ++secbits;
      return(LONGPULSE);
    }
    if (secbits<L_L+RSGAP_L+STM_L) {
      ++secbits;
      return(SHORTPULSE);
    }
    if (secbits<L_L+RSGAP_L+STM_L+L_L) {
      ++secbits;
      return(LONGPULSE);
    }
    secbits=0;
    /* Tape body - length is calculated from the values stored by the header */
    /* in memory locations 0x1103 and 0x1102 from the 20th & 19th values     */
    /* found in the header - i.e. header[19] (msb) and header[18] (lsb).     */
    bodylen=((header[19]<<8)&0xFF00)|header[18];
    SHOW("Body length is 0x%04x (%d) bytes\n",bodylen,bodylen);
    SHOW("Transition to state 8 - program data\n");
    crstate=8;
  }

  /* Process the tape body - state 8 */
  if (crstate==8) {
    if (secbits<(bodylen*8)) {     // 1 byte = 8 bits to transmit
      /* One LONGPULSE is sent before every byte of the header */
      if (((secbits%8)==0) && (longsent==0)) {
        /* Note - we don't increment secbits here */
        longsent=1;
        return(LONGPULSE);
      }
      longsent=0;
      /* Bytes are sent starting with bit 7 (msb) */
      bitshift=secbits%8;
      if (((body[secbits++/8]<<bitshift)&0x80) == 0x80) {
        ++chkbits; // Increment the long pulse count for calculating chkb
        return(LONGPULSE);
      }
      return(SHORTPULSE);
    }
    /* At the end of the body, move onto checksum state (9) */
    SHOW("Transition to state 9 - program checksum\n");
    SHOW("%d bits processed\n",secbits);
    SHOW("%d bytes processed\n",secbits/8);
    secbits=0;
    crstate=9;
  }

  /* Body checksum - state 9 */
  if (crstate==9) {
    if (secbits<CHK_L) {
      if ((secbits==0)&&(chkbits>0)) {
        // Need to calculate the body checksum
        // Note as chkbits is a uint16_t, we don't need to do modulo 2^16
        // as this will be taken care of for us
        checksum[0]=(chkbits>>8)&0xFF; /* MSB of the checksum */
        checksum[1]=chkbits&0xFF;      /* LSB of the checksum */
        SHOW("Body checksum 0x%02x 0x%02x from 0x%04x\n",checksum[0],checksum[1],chkbits);
        // Reset chkbits for the next time a checksum is calculated
        chkbits=0;
      }
      if (((secbits % 8) == 0) && (longsent == 0)) {
        /* Note - we don't increment secbits here */
        longsent = 1;
        return(LONGPULSE);
      }
      /* Reset the longsent flag */
      longsent = 0;
      /* Bytes are sent starting with the MSB */
      bitshift=secbits%8;
      if (((checksum[secbits++/8]<<bitshift)&0x80) == 0x80) {
        return(LONGPULSE);
      }
      return(SHORTPULSE);
    }
    /* At the end of the checksum stop */
    /* Assumes copy of program data is not needed */
    SHOW("Transition to state 13 - stop\n");
    secbits=0;
    crstate=13; 
  }

  /* States 10,11 and 12 are only needed if the program body has failed */
  /* to checksum correctly, so are not required for .mzf files */
  /* State 10 - a long pulse followed by 256 short */
  /* State 11 - a copy of the body */
  /* State 12 - a copy of the body checksum */

  if (crstate==13) {
  /* At the end of the body checksum, send final stop bit */
      crstate=0;
      hilo=0;
      SHOW("Final stop bit sent\n");
      return(LONGPULSE);
  }

  /* Catch any errors - shouldn't happen, but ...        */
  SHOW("Error in cread() - unknown state %d\n",crstate);
  /* Reset state to 0, hilo to 0, motor off */
  crstate=0;
  cmotor=0;
  hilo=0;
  return(LONGPULSE);
}
