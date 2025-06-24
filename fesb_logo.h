/*
 * FESB Logo Bitmap Data for VMA419 32x16 LED Matrix Display
 * 
 * This file contains the FESB (Faculty of Electrical Engineering, 
 * Mechanical Engineering and Naval Architecture) logo bitmap data
 * optimized for display on a 32x16 LED matrix.
 * 
 * The logo has been stylized to fit the 32x16 resolution while
 * maintaining the recognizable FESB letter forms.
 * 
 */

#ifndef FESB_LOGO_H
#define FESB_LOGO_H

#include <stdint.h>
#include "vma419.h"

// FESB Logo dimensions
#define FESB_LOGO_WIDTH  32
#define FESB_LOGO_HEIGHT 16

// FESB Logo bitmap data (32x16 pixels = 64 bytes)
// Recreated based on the actual FESB university logo
// The real logo features bold, modern block letters with specific styling
// Each byte represents 8 horizontal pixels, MSB = leftmost pixel
// Bit 1 = LED ON, Bit 0 = LED OFF
const uint8_t fesb_logo_bitmap[FESB_LOGO_HEIGHT][FESB_LOGO_WIDTH/8] = {
~0xff, ~0xff, ~0xff, ~0xff, ~0xff, ~0xff, ~0xff, ~0xff, ~0xe0, ~0x40, ~0x80, ~0x07, ~0xc0, ~0x80, ~0x00, ~0x03, 
~0x87, ~0x8f, ~0x0d, ~0xe3, ~0x8f, ~0x9f, ~0x1f, ~0xf1, ~0x9f, ~0xbf, ~0x1f, ~0xe3, ~0xa0, ~0x40, ~0x02, ~0x03, 
~0xc0, ~0x80, ~0x81, ~0x03, ~0x87, ~0x8f, ~0xf1, ~0xe3, ~0x8f, ~0x9f, ~0xf9, ~0xf1, ~0x8f, ~0x8d, ~0xe1, ~0xe3, 
~0x8f, ~0x80, ~0x01, ~0x03, ~0x8f, ~0xc0, ~0x02, ~0x07, ~0xff, ~0xff, ~0xff, ~0xff, ~0xff, ~0xff, ~0xff, ~0xff
};

/**
 * Display the FESB logo on the VMA419 LED matrix
 * 
 * This function draws the complete FESB logo bitmap to the display buffer.
 * The logo is designed to fill the entire 32x16 display area.
 * 
 * @param disp Pointer to VMA419 display structure
 */
void fesb_logo_display(VMA419_Display* disp);

/**
 * Display FESB logo for a specified duration with flashing effect
 * 
 * This function displays the FESB logo in two phases:
 * Phase 1: Shows logo solid for 10 seconds
 * Phase 2: Flashes logo at 0.5Hz (1 sec ON, 1 sec OFF) for 10 seconds
 * Total duration: 20 seconds
 * 
 * @param disp Pointer to VMA419 display structure  
 * @param duration_seconds Duration parameter (currently fixed at 20 seconds total)
 */
void fesb_logo_show_for_duration(VMA419_Display* disp, uint8_t duration_seconds);

//==============================================================================
// IMPLEMENTATION
//==============================================================================

void fesb_logo_display(VMA419_Display* disp) {
    if (!disp) return;
    
    // Clear the display buffer first
    vma419_clear(disp);
    
    // Draw the FESB logo pixel by pixel
    for (uint8_t row = 0; row < FESB_LOGO_HEIGHT; row++) {
        for (uint8_t byte_col = 0; byte_col < (FESB_LOGO_WIDTH/8); byte_col++) {            // Read bitmap byte from regular memory
            uint8_t bitmap_byte = fesb_logo_bitmap[row][byte_col];
            
            // Process each bit in the byte (8 pixels)
            for (uint8_t bit = 0; bit < 8; bit++) {
                uint8_t x = byte_col * 8 + bit;
                uint8_t y = row;
                
                // Extract pixel value (MSB first)
                uint8_t pixel = (bitmap_byte >> (7 - bit)) & 0x01;
                
                // Set pixel in display buffer
                vma419_set_pixel(disp, x, y, pixel);
            }
        }
    }
}

void fesb_logo_show_for_duration(VMA419_Display* disp, uint8_t duration_seconds) {
    if (!disp) return;
    
    // Phase 1: Display the logo solid for 10 seconds
    fesb_logo_display(disp);
    
    // Calculate refresh cycles for 10 seconds solid display
    // At 250Hz refresh rate (4ms per complete cycle)
    uint16_t solid_cycles = 10 * 250; // 10 seconds * 250 cycles/sec = 2500 cycles
    
    // Maintain solid display for 10 seconds
    for (uint16_t cycle_count = 0; cycle_count < solid_cycles; cycle_count++) {
        // Perform one complete 4-phase refresh cycle (4ms total)
        for (uint8_t phase = 0; phase < 4; phase++) {
            disp->scan_cycle = phase;
            vma419_scan_display_quarter(disp);
            _delay_ms(1); // 1ms per phase = 250Hz refresh rate
        }
    }
    
    // Phase 2: Flash the logo at 0.5Hz for 10 seconds
    // 0.5Hz = 0.5 cycles per second = 2 seconds per complete cycle
    // Each cycle: 1 second ON, 1 second OFF
    // For 10 seconds: 5 complete flash cycles
    
    uint16_t cycles_per_second = 250; // 250 refresh cycles = 1 second
    
    for (uint8_t flash_cycle = 0; flash_cycle < 5; flash_cycle++) {
        // Logo ON for 1 second
        fesb_logo_display(disp); // Draw logo
        for (uint16_t cycle_count = 0; cycle_count < cycles_per_second; cycle_count++) {
            for (uint8_t phase = 0; phase < 4; phase++) {
                disp->scan_cycle = phase;
                vma419_scan_display_quarter(disp);
                _delay_ms(1);
            }
        }
        
        // Logo OFF for 1 second
        vma419_clear(disp); // Clear display (logo off)
        for (uint16_t cycle_count = 0; cycle_count < cycles_per_second; cycle_count++) {
            for (uint8_t phase = 0; phase < 4; phase++) {
                disp->scan_cycle = phase;
                vma419_scan_display_quarter(disp);
                _delay_ms(1);
            }
        }
    }
}

#endif // FESB_LOGO_H
