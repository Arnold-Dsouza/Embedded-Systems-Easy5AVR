/*--------------------------------------------------------------------------------------

 DMD419.cpp - Function and support library for the Freetronics DMD419, a 512 LED matrix display
           panel arranged in a 32 x 16 layout.

 Copyright (C) 2011 Marc Alexander (info <at> freetronics <dot> com)

 Note that the DMD419 library uses the SPI port for the fastest, low overhead writing to the
 display. Keep an eye on conflicts if there are any other devices running from the same
 SPI port, and that the chip select on those devices is correctly set to be inactive
 when the DMD419 is being written to.

 ---

 This program is free software: you can redistribute it and/or modify it under the terms
 of the version 3 GNU General Public License as published by the Free Software Foundation.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with this program.
 If not, see <http://www.gnu.org/licenses/>.

--------------------------------------------------------------------------------------*/
#include "DMD419.h"

/*--------------------------------------------------------------------------------------
 Setup and instantiation of DMD419 library
 Note this currently uses the SPI port for the fastest performance to the DMD419, be
 careful of possible conflicts with other SPI port devices
--------------------------------------------------------------------------------------*/
DMD419::DMD419(byte panelsWide, byte panelsHigh)
{
    uint16_t ui;
    DisplaysWide=panelsWide;
    DisplaysHigh=panelsHigh;
    DisplaysTotal=DisplaysWide*DisplaysHigh;
    row1 = DisplaysTotal<<4;
    row2 = DisplaysTotal<<5;
    row3 = ((DisplaysTotal<<2)*3)<<2;
    bDMD419ScreenRAM = (byte *) malloc(DisplaysTotal*DMD419_RAM_SIZE_BYTES);

    // initialize the SPI port
    SPI.begin();		// probably don't need this since it inits the port pins only, which we do just below with the appropriate DMD419 interface setup
    SPI.setBitOrder(MSBFIRST);	//
    SPI.setDataMode(SPI_MODE0);	// CPOL=0, CPHA=0
    SPI.setClockDivider(SPI_CLOCK_DIV4);	// system clock / 4 = 4MHz SPI CLK to shift registers. If using a short cable, can put SPI_CLOCK_DIV2 here for 2x faster updates

    digitalWrite(PIN_DMD419_A, LOW);	// 
    digitalWrite(PIN_DMD419_B, LOW);	// 
    digitalWrite(PIN_DMD419_CLK, LOW);	// 
    digitalWrite(PIN_DMD419_SCLK, LOW);	// 
    digitalWrite(PIN_DMD419_R_DATA, HIGH);	// 
    digitalWrite(PIN_DMD419_nOE, LOW);	//

    pinMode(PIN_DMD419_A, OUTPUT);	//
    pinMode(PIN_DMD419_B, OUTPUT);	//
    pinMode(PIN_DMD419_CLK, OUTPUT);	//
    pinMode(PIN_DMD419_SCLK, OUTPUT);	//
    pinMode(PIN_DMD419_R_DATA, OUTPUT);	//
    pinMode(PIN_DMD419_nOE, OUTPUT);	//

    clearScreen(true);

    // init the scan line/ram pointer to the required start point
    bDMD419Byte = 0;
}

//DMD419::~DMD419()
//{
//   // nothing needed here
//}

/*--------------------------------------------------------------------------------------
 Set or clear a pixel at the x and y location (0,0 is the top left corner)
--------------------------------------------------------------------------------------*/
void
 DMD419::writePixel(unsigned int bX, unsigned int bY, byte bGraphicsMode, byte bPixel)
{
    unsigned int uiDMD419RAMPointer;

    if (bX >= (DMD419_PIXELS_ACROSS*DisplaysWide) || bY >= (DMD419_PIXELS_DOWN * DisplaysHigh)) {
	    return;
    }
    byte panel=(bX/DMD419_PIXELS_ACROSS) + (DisplaysWide*(bY/DMD419_PIXELS_DOWN));
    bX=(bX % DMD419_PIXELS_ACROSS) + (panel<<5);
    bY=bY % DMD419_PIXELS_DOWN;
    //set pointer to DMD419 RAM byte to be modified
    uiDMD419RAMPointer = bX/8 + bY*(DisplaysTotal<<2);

    byte lookup = bPixelLookupTable[bX & 0x07];

    switch (bGraphicsMode) {
    case GRAPHICS_NORMAL:
	    if (bPixel == true)
		bDMD419ScreenRAM[uiDMD419RAMPointer] &= ~lookup;	// zero bit is pixel on
	    else
		bDMD419ScreenRAM[uiDMD419RAMPointer] |= lookup;	// one bit is pixel off
	    break;
    case GRAPHICS_INVERSE:
	    if (bPixel == false)
		    bDMD419ScreenRAM[uiDMD419RAMPointer] &= ~lookup;	// zero bit is pixel on
	    else
		    bDMD419ScreenRAM[uiDMD419RAMPointer] |= lookup;	// one bit is pixel off
	    break;
    case GRAPHICS_TOGGLE:
	    if (bPixel == true) {
		if ((bDMD419ScreenRAM[uiDMD419RAMPointer] & lookup) == 0)
		    bDMD419ScreenRAM[uiDMD419RAMPointer] |= lookup;	// one bit is pixel off
		else
		    bDMD419ScreenRAM[uiDMD419RAMPointer] &= ~lookup;	// one bit is pixel off
	    }
	    break;
    case GRAPHICS_OR:
	    //only set pixels on
	    if (bPixel == true)
		    bDMD419ScreenRAM[uiDMD419RAMPointer] &= ~lookup;	// zero bit is pixel on
	    break;
    case GRAPHICS_NOR:
	    //only clear on pixels
	    if ((bPixel == true) &&
		    ((bDMD419ScreenRAM[uiDMD419RAMPointer] & lookup) == 0))
		    bDMD419ScreenRAM[uiDMD419RAMPointer] |= lookup;	// one bit is pixel off
	    break;
    }

}

void DMD419::drawString(int bX, int bY, const char *bChars, byte length,
		     byte bGraphicsMode)
{
    if (bX >= (DMD419_PIXELS_ACROSS*DisplaysWide) || bY >= DMD419_PIXELS_DOWN * DisplaysHigh)
	return;
    uint8_t height = pgm_read_byte(this->Font + FONT_HEIGHT);
    if (bY+height<0) return;

    int strWidth = 0;
	this->drawLine(bX -1 , bY, bX -1 , bY + height, GRAPHICS_INVERSE);

    for (int i = 0; i < length; i++) {
        int charWide = this->drawChar(bX+strWidth, bY, bChars[i], bGraphicsMode);
	    if (charWide > 0) {
	        strWidth += charWide ;
	        this->drawLine(bX + strWidth , bY, bX + strWidth , bY + height, GRAPHICS_INVERSE);
            strWidth++;
        } else if (charWide < 0) {
            return;
        }
        if ((bX + strWidth) >= DMD419_PIXELS_ACROSS * DisplaysWide || bY >= DMD419_PIXELS_DOWN * DisplaysHigh) return;
    }
}

void DMD419::drawMarquee(const char *bChars, byte length, int left, int top)
{
    marqueeWidth = 0;
    for (int i = 0; i < length; i++) {
	    marqueeText[i] = bChars[i];
	    marqueeWidth += charWidth(bChars[i]) + 1;
    }
    marqueeHeight=pgm_read_byte(this->Font + FONT_HEIGHT);
    marqueeText[length] = '\0';
    marqueeOffsetY = top;
    marqueeOffsetX = left;
    marqueeLength = length;
    drawString(marqueeOffsetX, marqueeOffsetY, marqueeText, marqueeLength,
	   GRAPHICS_NORMAL);
}

boolean DMD419::stepMarquee(int amountX, int amountY)
{
    boolean ret=false;
    marqueeOffsetX += amountX;
    marqueeOffsetY += amountY;

    if (marqueeOffsetX < -marqueeWidth) {
	    marqueeOffsetX = DMD419_PIXELS_ACROSS * DisplaysWide;
	    clearScreen(true);
        ret=true;
    } else if (marqueeOffsetX > DMD419_PIXELS_ACROSS * DisplaysWide) {
	    marqueeOffsetX = -marqueeWidth;
	    clearScreen(true);
        ret=true;
    }
    
        
    if (marqueeOffsetY < -marqueeHeight) {
	    marqueeOffsetY = DMD419_PIXELS_DOWN * DisplaysHigh;
	    clearScreen(true);
        ret=true;
    } else if (marqueeOffsetY > DMD419_PIXELS_DOWN * DisplaysHigh) {
	    marqueeOffsetY = -marqueeHeight;
	    clearScreen(true);
        ret=true;
    }

    // Special case horizontal scrolling to improve speed
    if (amountY==0 && amountX==-1) {
        // Shift entire screen one bit
        for (int i=0; i<DMD419_RAM_SIZE_BYTES*DisplaysTotal;i++) {
            if ((i%(DisplaysWide*4)) == (DisplaysWide*4) -1) {
                bDMD419ScreenRAM[i]=(bDMD419ScreenRAM[i]<<1)+1;
            } else {
                bDMD419ScreenRAM[i]=(bDMD419ScreenRAM[i]<<1) + ((bDMD419ScreenRAM[i+1] & 0x80) >>7);
            }
        }

        // Redraw last char on screen
        int strWidth=marqueeOffsetX;
        for (byte i=0; i < marqueeLength; i++) {
            int wide = charWidth(marqueeText[i]);
            if (strWidth+wide >= DisplaysWide*DMD419_PIXELS_ACROSS) {
                drawChar(strWidth, marqueeOffsetY,marqueeText[i],GRAPHICS_NORMAL);
                return ret;
            }
            strWidth += wide+1;
        }
    } else if (amountY==0 && amountX==1) {
        // Shift entire screen one bit
        for (int i=(DMD419_RAM_SIZE_BYTES*DisplaysTotal)-1; i>=0;i--) {
            if ((i%(DisplaysWide*4)) == 0) {
                bDMD419ScreenRAM[i]=(bDMD419ScreenRAM[i]>>1)+128;
            } else {
                bDMD419ScreenRAM[i]=(bDMD419ScreenRAM[i]>>1) + ((bDMD419ScreenRAM[i-1] & 1) <<7);
            }
        }

        // Redraw last char on screen
        int strWidth=marqueeOffsetX;
        for (byte i=0; i < marqueeLength; i++) {
            int wide = charWidth(marqueeText[i]);
            if (strWidth+wide >= 0) {
                drawChar(strWidth, marqueeOffsetY,marqueeText[i],GRAPHICS_NORMAL);
                return ret;
            }
            strWidth += wide+1;
        }
    } else {
        drawString(marqueeOffsetX, marqueeOffsetY, marqueeText, marqueeLength,
	       GRAPHICS_NORMAL);
    }

    return ret;
}


/*--------------------------------------------------------------------------------------
 Clear the screen in DMD419 RAM
--------------------------------------------------------------------------------------*/
void DMD419::clearScreen(byte bNormal)
{
    if (bNormal) // clear all pixels
        memset(bDMD419ScreenRAM,0xFF,DMD419_RAM_SIZE_BYTES*DisplaysTotal);
    else // set all pixels
        memset(bDMD419ScreenRAM,0x00,DMD419_RAM_SIZE_BYTES*DisplaysTotal);
}

/*--------------------------------------------------------------------------------------
 Draw or clear a line from x1,y1 to x2,y2
--------------------------------------------------------------------------------------*/
void DMD419::drawLine(int x1, int y1, int x2, int y2, byte bGraphicsMode)
{
    int dy = y2 - y1;
    int dx = x2 - x1;
    int stepx, stepy;

    if (dy < 0) {
	    dy = -dy;
	    stepy = -1;
    } else {
	    stepy = 1;
    }
    if (dx < 0) {
	    dx = -dx;
	    stepx = -1;
    } else {
	    stepx = 1;
    }
    dy <<= 1;			// dy is now 2*dy
    dx <<= 1;			// dx is now 2*dx

    writePixel(x1, y1, bGraphicsMode, true);
    if (dx > dy) {
	    int fraction = dy - (dx >> 1);	// same as 2*dy - dx
	    while (x1 != x2) {
	        if (fraction >= 0) {
		        y1 += stepy;
		        fraction -= dx;	// same as fraction -= 2*dx
	        }
	        x1 += stepx;
	        fraction += dy;	// same as fraction -= 2*dy
	        writePixel(x1, y1, bGraphicsMode, true);
	    }
    } else {
	    int fraction = dx - (dy >> 1);
	    while (y1 != y2) {
	        if (fraction >= 0) {
		        x1 += stepx;
		        fraction -= dy;
	        }
	        y1 += stepy;
	        fraction += dx;
	        writePixel(x1, y1, bGraphicsMode, true);
	    }
    }
}

/*--------------------------------------------------------------------------------------
 Draw or clear a circle of radius r at x,y centre
--------------------------------------------------------------------------------------*/
void DMD419::drawCircle(int xCenter, int yCenter, int radius,
		     byte bGraphicsMode)
{
    int x = 0;
    int y = radius;
    int p = (5 - radius * 4) / 4;

    drawCircleSub(xCenter, yCenter, x, y, bGraphicsMode);
    while (x < y) {
	    x++;
	    if (p < 0) {
	        p += 2 * x + 1;
	    } else {
	        y--;
	        p += 2 * (x - y) + 1;
	    }
	    drawCircleSub(xCenter, yCenter, x, y, bGraphicsMode);
    }
}

void DMD419::drawCircleSub(int cx, int cy, int x, int y, byte bGraphicsMode)
{

    if (x == 0) {
	    writePixel(cx, cy + y, bGraphicsMode, true);
	    writePixel(cx, cy - y, bGraphicsMode, true);
	    writePixel(cx + y, cy, bGraphicsMode, true);
	    writePixel(cx - y, cy, bGraphicsMode, true);
    } else if (x == y) {
	    writePixel(cx + x, cy + y, bGraphicsMode, true);
	    writePixel(cx - x, cy + y, bGraphicsMode, true);
	    writePixel(cx + x, cy - y, bGraphicsMode, true);
	    writePixel(cx - x, cy - y, bGraphicsMode, true);
    } else if (x < y) {
	    writePixel(cx + x, cy + y, bGraphicsMode, true);
	    writePixel(cx - x, cy + y, bGraphicsMode, true);
	    writePixel(cx + x, cy - y, bGraphicsMode, true);
	    writePixel(cx - x, cy - y, bGraphicsMode, true);
	    writePixel(cx + y, cy + x, bGraphicsMode, true);
	    writePixel(cx - y, cy + x, bGraphicsMode, true);
	    writePixel(cx + y, cy - x, bGraphicsMode, true);
	    writePixel(cx - y, cy - x, bGraphicsMode, true);
    }
}

/*--------------------------------------------------------------------------------------
 Draw or clear a box(rectangle) with a single pixel border
--------------------------------------------------------------------------------------*/
void DMD419::drawBox(int x1, int y1, int x2, int y2, byte bGraphicsMode)
{
    drawLine(x1, y1, x2, y1, bGraphicsMode);
    drawLine(x2, y1, x2, y2, bGraphicsMode);
    drawLine(x2, y2, x1, y2, bGraphicsMode);
    drawLine(x1, y2, x1, y1, bGraphicsMode);
}

/*--------------------------------------------------------------------------------------
 Draw or clear a filled box(rectangle) with a single pixel border
--------------------------------------------------------------------------------------*/
void DMD419::drawFilledBox(int x1, int y1, int x2, int y2,
			byte bGraphicsMode)
{
    for (int b = x1; b <= x2; b++) {
	    drawLine(b, y1, b, y2, bGraphicsMode);
    }
}

/*--------------------------------------------------------------------------------------
 Draw the selected test pattern
--------------------------------------------------------------------------------------*/
void DMD419::drawTestPattern(byte bPattern)
{
    unsigned int ui;

    int numPixels=DisplaysTotal * DMD419_PIXELS_ACROSS * DMD419_PIXELS_DOWN;
    int pixelsWide=DMD419_PIXELS_ACROSS*DisplaysWide;
    for (ui = 0; ui < numPixels; ui++) {
	    switch (bPattern) {
	    case PATTERN_ALT_0:	// every alternate pixel, first pixel on
		    if ((ui & pixelsWide) == 0)
		        //even row
		        writePixel((ui & (pixelsWide-1)), ((ui & ~(pixelsWide-1)) / pixelsWide), GRAPHICS_NORMAL, ui & 1);
		    else
		        //odd row
		        writePixel((ui & (pixelsWide-1)), ((ui & ~(pixelsWide-1)) / pixelsWide), GRAPHICS_NORMAL, !(ui & 1));
		    break;
	    case PATTERN_ALT_1:	// every alternate pixel, first pixel off
		    if ((ui & pixelsWide) == 0)
		        //even row
		        writePixel((ui & (pixelsWide-1)), ((ui & ~(pixelsWide-1)) / pixelsWide), GRAPHICS_NORMAL, !(ui & 1));
		    else
		        //odd row
		        writePixel((ui & (pixelsWide-1)), ((ui & ~(pixelsWide-1)) / pixelsWide), GRAPHICS_NORMAL, ui & 1);
		    break;
	    case PATTERN_STRIPE_0:	// vertical stripes, first stripe on
		    writePixel((ui & (pixelsWide-1)), ((ui & ~(pixelsWide-1)) / pixelsWide), GRAPHICS_NORMAL, ui & 1);
		    break;
	    case PATTERN_STRIPE_1:	// vertical stripes, first stripe off
		    writePixel((ui & (pixelsWide-1)), ((ui & ~(pixelsWide-1)) / pixelsWide), GRAPHICS_NORMAL, !(ui & 1));
		    break;
        }
    }
}

/*--------------------------------------------------------------------------------------
 Scan the dot matrix LED panel display, from the RAM mirror out to the display hardware.
 Call 4 times to scan the whole display which is made up of 4 interleaved rows within the 16 total rows.
 Insert the calls to this function into the main loop for the highest call rate, or from a timer interrupt
--------------------------------------------------------------------------------------*/
void DMD419::scanDisplayBySPI()
{
    //if PIN_OTHER_SPI_nCS is in use during a DMD419 scan request then scanDisplayBySPI() will exit without conflict! (and skip that scan)
    if( digitalRead( PIN_OTHER_SPI_nCS ) == HIGH )
    {
        //SPI transfer pixels to the display hardware shift registers
        int rowsize=DisplaysTotal<<2;
        int offset=rowsize * bDMD419Byte;
        for (int i=0;i<DisplaysTotal;i++) {
            SPI.transfer(~bDMD419ScreenRAM[offset+0+row3]);    
            SPI.transfer(~bDMD419ScreenRAM[offset+0+row2]);
            SPI.transfer(~bDMD419ScreenRAM[offset+1+row3]);
            SPI.transfer(~bDMD419ScreenRAM[offset+1+row2]);
            SPI.transfer(~bDMD419ScreenRAM[offset+0+row1]);
            SPI.transfer(~bDMD419ScreenRAM[offset+0]);
            SPI.transfer(~bDMD419ScreenRAM[offset+1+row1]);
            SPI.transfer(~bDMD419ScreenRAM[offset+1]);
            SPI.transfer(~bDMD419ScreenRAM[offset+2+row3]);    
            SPI.transfer(~bDMD419ScreenRAM[offset+2+row2]);
            SPI.transfer(~bDMD419ScreenRAM[offset+3+row3]);
            SPI.transfer(~bDMD419ScreenRAM[offset+3+row2]);
            SPI.transfer(~bDMD419ScreenRAM[offset+2+row1]);
            SPI.transfer(~bDMD419ScreenRAM[offset+2]);
            SPI.transfer(~bDMD419ScreenRAM[offset+3+row1]);
            SPI.transfer(~bDMD419ScreenRAM[offset+3]);
        }

        OE_DMD419_ROWS_OFF();
        LATCH_DMD419_SHIFT_REG_TO_OUTPUT();
        switch (bDMD419Byte) {
        case 0:			// row 1, 5, 9, 13 were clocked out
            LIGHT_DMD419_ROW_01_05_09_13();
            bDMD419Byte=1;
            break;
        case 1:			// row 2, 6, 10, 14 were clocked out
            LIGHT_DMD419_ROW_02_06_10_14();
            bDMD419Byte=2;
            break;
        case 2:			// row 3, 7, 11, 15 were clocked out
            LIGHT_DMD419_ROW_03_07_11_15();
            bDMD419Byte=3;
            break;
        case 3:			// row 4, 8, 12, 16 were clocked out
            LIGHT_DMD419_ROW_04_08_12_16();
            bDMD419Byte=0;
            break;
        }
        OE_DMD419_ROWS_ON();
    }
}

void DMD419::selectFont(const uint8_t * font)
{
    this->Font = font;
}


int DMD419::drawChar(const int bX, const int bY, const unsigned char letter, byte bGraphicsMode)
{
    if (bX > (DMD419_PIXELS_ACROSS*DisplaysWide) || bY > (DMD419_PIXELS_DOWN*DisplaysHigh) ) return -1;
    unsigned char c = letter;
    uint8_t height = pgm_read_byte(this->Font + FONT_HEIGHT);
    if (c == ' ') {
	    int charWide = charWidth(' ');
	    this->drawFilledBox(bX, bY, bX + charWide, bY + height, GRAPHICS_INVERSE);
	    return charWide;
    }
    uint8_t width = 0;
    uint8_t bytes = (height + 7) / 8;

    uint8_t firstChar = pgm_read_byte(this->Font + FONT_FIRST_CHAR);
    uint8_t charCount = pgm_read_byte(this->Font + FONT_CHAR_COUNT);

    uint16_t index = 0;

    if (c < firstChar || c >= (firstChar + charCount)) return 0;
    c -= firstChar;

    if (pgm_read_byte(this->Font + FONT_LENGTH) == 0
	    && pgm_read_byte(this->Font + FONT_LENGTH + 1) == 0) {
	    // zero length is flag indicating fixed width font (array does not contain width data entries)
	    width = pgm_read_byte(this->Font + FONT_FIXED_WIDTH);
	    index = c * bytes * width + FONT_WIDTH_TABLE;
    } else {
	    // variable width font, read width data, to get the index
	    for (uint8_t i = 0; i < c; i++) {
	        index += pgm_read_byte(this->Font + FONT_WIDTH_TABLE + i);
	    }
	    index = index * bytes + charCount + FONT_WIDTH_TABLE;
	    width = pgm_read_byte(this->Font + FONT_WIDTH_TABLE + c);
    }
    if (bX < -width || bY < -height) return width;

    // last but not least, draw the character
    for (uint8_t j = 0; j < width; j++) { // Width
	    for (uint8_t i = bytes - 1; i < 254; i--) { // Vertical Bytes
	        uint8_t data = pgm_read_byte(this->Font + index + j + (i * width));
		    int offset = (i * 8);
		    if ((i == bytes - 1) && bytes > 1) {
		        offset = height - 8;
            }
	        for (uint8_t k = 0; k < 8; k++) { // Vertical bits
		        if ((offset+k >= i*8) && (offset+k <= height)) {
		            if (data & (1 << k)) {
			            writePixel(bX + j, bY + offset + k, bGraphicsMode, true);
		            } else {
			            writePixel(bX + j, bY + offset + k, bGraphicsMode, false);
		            }
		        }
	        }
	    }
    }
    return width;
}

int DMD419::charWidth(const unsigned char letter)
{
    unsigned char c = letter;
    // Space is often not included in font so use width of 'n'
    if (c == ' ') c = 'n';
    uint8_t width = 0;

    uint8_t firstChar = pgm_read_byte(this->Font + FONT_FIRST_CHAR);
    uint8_t charCount = pgm_read_byte(this->Font + FONT_CHAR_COUNT);

    uint16_t index = 0;

    if (c < firstChar || c >= (firstChar + charCount)) {
	    return 0;
    }
    c -= firstChar;

    if (pgm_read_byte(this->Font + FONT_LENGTH) == 0
	&& pgm_read_byte(this->Font + FONT_LENGTH + 1) == 0) {
	    // zero length is flag indicating fixed width font (array does not contain width data entries)
	    width = pgm_read_byte(this->Font + FONT_FIXED_WIDTH);
    } else {
	    // variable width font, read width data
	    width = pgm_read_byte(this->Font + FONT_WIDTH_TABLE + c);
    }
    return width;
}
