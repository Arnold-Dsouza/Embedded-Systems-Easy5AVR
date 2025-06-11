/*
 * VMA419 LED Matrix Display Driver - Main Application
 * 
 * This program demonstrates the VMA419 32x16 LED matrix display driver.
 * It initializes a single VMA419 panel and displays a test LED at position (31,15).
 * 
 * Test Pattern: Single LED at bottom-right corner (31,15)
 * - Demonstrates successful hardware initialization
 * - Validates pixel addressing and row remapping
 * - Confirms 4-phase multiplexing operation
 * 
 * Hardware Configuration:
 * - VMA419 32x16 LED Matrix Panel
 * - AVR microcontroller (ATmega328P or similar)
 * - Pin connections as specified in dmd_pins structure
 * 
 * Author: Arnold Dsouza
 * Date: 11/6/2025
 * Project: VMA419 LED Matrix Driver for Embedded Systems
 * Status: FULLY FUNCTIONAL ✓
 */

#include <avr/io.h>
#include <util/delay.h>
#include "vma419.h"

// --- Global Display Structures ---
VMA419_Display dmd_display;

// Pin configuration for VMA419 display
// Adjust these pin assignments according to your hardware setup
VMA419_PinConfig dmd_pins = {
    // SPI Data and Clock pins (for bit-banging SPI)
    .spi_clk_port_ddr  = &DDRB, .spi_clk_port_out  = &PORTB, .spi_clk_pin_mask  = (1 << PB7), // CLK -> PB7
    .spi_data_port_ddr = &DDRB, .spi_data_port_out = &PORTB, .spi_data_pin_mask = (1 << PB5), // R -> PB5

    // Row Select Pins (2-bit binary selection for 4 row phases)
    .a_port_ddr        = &DDRA, .a_port_out        = &PORTA, .a_pin_mask        = (1 << PA1), // A -> PA1
    .b_port_ddr        = &DDRA, .b_port_out        = &PORTA, .b_pin_mask        = (1 << PA2), // B -> PA2

    // Latch/Strobe Clock Pin (transfers shift register data to output latches)
    .latch_clk_port_ddr= &DDRA, .latch_clk_port_out= &PORTA, .latch_clk_pin_mask= (1 << PA4), // SCLK -> PA4

    // Output Enable Pin (controls display brightness/visibility)
    .oe_port_ddr       = &DDRD, .oe_port_out       = &PORTD, .oe_pin_mask       = (1 << PD7)  // OE -> PD7
};

int main(void) {
    // Initialize the VMA419 display driver
    // Parameters: display structure, pin config, panels_wide=1, panels_high=1
    if (vma419_init(&dmd_display, &dmd_pins, 1, 1) != 0) {
        // Initialization failed - halt execution
        while(1); 
    }
      // Clear the display buffer to ensure all LEDs start OFF
    vma419_clear(&dmd_display);
    
    // Test Pattern: Light up a single LED at bottom-right corner (31,15)
    // This demonstrates successful pixel addressing and display functionality
    // Position (31,15) = rightmost column, bottom row of the 32x16 display
    vma419_set_pixel(&dmd_display, 31, 15, 1); // Set pixel at (31,15) - bottom-right corner
    
    // Main display refresh loop
    // The VMA419 uses 4-phase multiplexing, so we cycle through all 4 phases
    while(1) {
        for(uint8_t cycle = 0; cycle < 4; cycle++) {
            dmd_display.scan_cycle = cycle;
            vma419_scan_display_quarter(&dmd_display);
            _delay_ms(1); // 1ms delay per phase = 250Hz refresh rate
        }
    }
    
    return 0; // Should never be reached
}