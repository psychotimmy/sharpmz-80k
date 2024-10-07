/* Sharp MZ-80K emulator virtual tape handling routines */
/* Tim Holyoake, October 2024 */
#include "sharpmz.h"

uint8_t header[TAPEHEADER];    // Tape headers are always 128 bytes
uint8_t body[TAPEBODY];        // Maximum storage is 48K - 49152 bytes

static FATFS fs;               // File system pointer for sd card

/* Mount the sd card */
uint8_t tapeinit()
{
  FRESULT res;
  sleep_ms(1000);
  // Mount the sd card filesystem
  res=f_mount(&fs, "", 1);
  if (res != FR_OK) {
    SHOW("sd card mount failed with status %d\n",res);
    return(0);
  }
  SHOW("sd card mounted ok\n");
  return(1);
}

/* Preload a tape file into the header/body memory ready for LOAD */
int16_t tapeloader(int16_t n)
{
  FIL fp;
  DIR dp;
  FILINFO fno;
  FRESULT res;
  uint bytesread,bodylen,dc;

  res=f_opendir(&dp,"/");	/* Open the root directory on the sd card */
  if (res) {
    SHOW("Error on directory open for /, status is %d\n",res);
    return(-1);
  }

  dc=0;                         
  while (dc <= n) {             /* Find the nth file in the directory */
    res=f_readdir(&dp, &fno);   /* (i.e. the nth file on the 'tape')  */
    if (res != FR_OK || fno.fname[0] == 0) {
      /* We're at the end of the tape or an error has happened */
      /* Return with no change to the preloaded file */
      SHOW("End of tape, status is %d\n",res);
      f_closedir(&dp);
      return(-1);
    }
    if (fno.fattrib & AM_DIR)
      SHOW("Ignoring directory %s\n",fno.fname);
    else 
      ++dc;                      /* Increment the file count */
  }
  f_closedir(&dp);               /* Close the directory pointer */
  
  // We now have the next file on the tape - preload it
  res=f_open(&fp,fno.fname,FA_READ|FA_OPEN_EXISTING);
  if (res) {
    SHOW("Error on file open for %s, status is %d\n",fno.fname,res);
    return(-1);
  }
  
  // MZ-80K tape headers are always 128 bytes
  f_read(&fp,header,128,&bytesread);
  if (bytesread != 128) {
    SHOW("Error on header read - only read %d of 128 bytes\n",bytesread);
    f_close(&fp);
    return(-1);
  }

  // Work out how many bytes to read from the header - stored in locations
  // header[19] and header[18] (msb, lsb)
  bodylen=((header[19]<<8)&0xFF00)|header[18];
  SHOW("Tape body length for tape %d is %d\n",n,bodylen);
  f_read(&fp,body,bodylen,&bytesread);
  if (bytesread != bodylen) {
    SHOW("Error on body read - only read %d of %d bytes\n",bytesread,bodylen);
    f_close(&fp);
    return(-1);
  }

  // Update the preloaded tape name in the emulator status area. Note that
  // this is the name stored in the header, NOT the actual file name on
  // the SD card.
  uint8_t spos;
  for (spos=40;spos<80;spos++) 
    mzemustatus[spos]=0x00;
  mzemustatus[40]=0x0e;  //N
  mzemustatus[41]=0x85;  //e
  mzemustatus[42]=0x98;  //x
  mzemustatus[43]=0x94;  //t
  mzemustatus[44]=0x00;  //<space>
  mzemustatus[45]=0x94;  //t
  mzemustatus[46]=0x81;  //a
  mzemustatus[47]=0x90;  //p
  mzemustatus[48]=0x85;  //e
  mzemustatus[49]=0x00;  //<space
  mzemustatus[50]=0x89;  //i
  mzemustatus[51]=0x93;  //s
  mzemustatus[52]=0x4f;  //:
  mzemustatus[53]=0x00;  //<space>
  spos=54;
  // Tape name terminates with 0x0d or is 17 characters long
  while ((header[spos-53] != 0x0d) && (spos < 71)) {
    mzemustatus[spos]=mzascii2display(header[spos-53]);
    ++spos;
  }

  // We've read the tape successfully if we get here
  SHOW("Successful preload of %s\n",fno.fname);
  f_close(&fp);
  return(n);     /* Return the file number loaded - matches requested */
}
