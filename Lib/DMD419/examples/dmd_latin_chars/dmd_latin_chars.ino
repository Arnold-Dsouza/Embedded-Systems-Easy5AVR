/*--------------------------------------------------------------------------------------

 dmd_latin_chars
   Demo and example Latin-1 encoding project for the Freetronics DMD, a 512 LED matrix display
   panel arranged in a 32 x 16 layout.

See http://www.freetronics.com/dmd for resources and a getting started guide.

This example code is in the public domain. It demonstrates using the ISO-8859-1
(Latin-1) extended character set. Thanks to Xavier Seignard for contributing
Latin-1 support.

*******************************
HOW TO ENTER Latin-1 CHARACTERS
*******************************

Unfortunately entering Latin-1 characters like Ã or è is not as simple as just
typing them. Arduino Sketches are saved in Unicode UTF-8 format, but the DMD library
does not understand Unicode (it's too complex.)

To enter the characters as Latin-1, look at the codepage layout here and look for
the hexadecimal digit that corresponds to the character you want.
https://en.wikipedia.org/wiki/ISO/IEC_8859-1#Codepage_layout

For example, á (lowercase a, rising diacritical mark) has hex value 00E0 in Latin-1.
To translate this into a constant string, replace the leading 00 with \x - so the
string could be "The Portugese for rapid is r\xE0pido".

To be safe, the string may also need to be separated in its own quote marks - ie
"The Portugese for rapid is r""\xE0""pido"

When you compile the sketch, the compiler will join all these strings up into one
long string - however without the quotes around the \x it may try to include additional
characters as part of the hexadecimal sequence.

/*--------------------------------------------------------------------------------------
  Includes
--------------------------------------------------------------------------------------*/
#include <SPI.h>        //SPI.h must be included as DMD is written by SPI (the IDE complains otherwise)
#include <DMD419.h>        //
#include <TimerOne.h>   //
#include "SystemFont5x7.h"
#include "Arial_Black_16_ISO_8859_1.h"

//Fire up the DMD library as dmd
#define DISPLAYS_ACROSS 1
#define DISPLAYS_DOWN 1
DMD419 dmd(DISPLAYS_ACROSS, DISPLAYS_DOWN);

/*--------------------------------------------------------------------------------------
  Interrupt handler for Timer1 (TimerOne) driven DMD refresh scanning, this gets
  called at the period set in Timer1.initialize();
--------------------------------------------------------------------------------------*/
void ScanDMD()
{ 
  dmd.scanDisplayBySPI();
}

/*--------------------------------------------------------------------------------------
  setup
  Called by the Arduino architecture before the main loop begins
--------------------------------------------------------------------------------------*/
void setup(void)
{

   //initialize TimerOne's interrupt/CPU usage used to scan and refresh the display
   Timer1.initialize( 3000 );           //period in microseconds to call ScanDMD. Anything longer than 5000 (5ms) and you can see flicker.
   Timer1.attachInterrupt( ScanDMD );   //attach the Timer1 interrupt to ScanDMD which goes to dmd.scanDisplayBySPI()

   //clear/init the DMD pixels held in RAM
   dmd.clearScreen( true );   //true is normal (all pixels off), false is negative (all pixels on)
  Serial.begin(115200);
}

/*--------------------------------------------------------------------------------------
  loop
  Arduino architecture main loop
--------------------------------------------------------------------------------------*/
void loop(void)
{
   dmd.clearScreen( true );
   dmd.selectFont(Arial_Black_16_ISO_8859_1);
   // Français, Österreich, Magyarország
   const char *MSG = "Fran""\xe7""ais, ""\xd6""sterreich, Magyarorsz""\xe1""g";
   dmd.drawMarquee(MSG,strlen(MSG),(32*DISPLAYS_ACROSS)-1,0);
   long start=millis();
   long timer=start;
   while(1){
     if ((timer+30) < millis()) {
       dmd.stepMarquee(-1,0);
       timer=millis();
     }
   }
}

