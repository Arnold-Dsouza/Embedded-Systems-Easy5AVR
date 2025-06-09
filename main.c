#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "vma419.h"

// --- Global Display Structures ---
VMA419_Display dmd_display;
VMA419_PinConfig dmd_pins = {
    .spi_clk_port_ddr  = &DDRB, .spi_clk_port_out  = &PORTB, .spi_clk_pin_mask  = (1 << PB7), // User: CLK -> PB7 (SPI SCK)
    .spi_data_port_ddr = &DDRB, .spi_data_port_out = &PORTB, .spi_data_pin_mask = (1 << PB5), // User: R -> PB5 (SPI MOSI)

    // Row Select Pins
    .a_port_ddr        = &DDRA, .a_port_out        = &PORTA, .a_pin_mask        = (1 << PA1), // User: A -> PA1
    .b_port_ddr        = &DDRA, .b_port_out        = &PORTA, .b_pin_mask        = (1 << PA2), // User: B -> PA2

    // Latch/Strobe Clock Pin
    .latch_clk_port_ddr= &DDRA, .latch_clk_port_out= &PORTA, .latch_clk_pin_mask= (1 << PA4), // User: SCLK -> PA4 (Latch/STB)

    // Output Enable Pin
    .oe_port_ddr       = &DDRD, .oe_port_out       = &PORTD, .oe_pin_mask       = (1 << PD7)  // User: OE -> PD7
};

int main(void) {
    // Initialize the display (1 panel wide, 1 panel high)
    if (vma419_init(&dmd_display, &dmd_pins, 1, 1) != 0) {
        while(1); // Initialization failed
    }    vma419_clear(&dmd_display); // Clear the display buffer first
    
    // Simple test: Set first 8 pixels in top row to ON
    for(uint8_t x = 0; x < 8; x++) {
        vma419_set_pixel(&dmd_display, x, 0, 1);  // 1 = pixel ON, 0 = pixel OFF
    }    
    while(1) {
        // Cycle through all 4 scan phases rapidly
        for(uint8_t cycle = 0; cycle < 4; cycle++) {
            dmd_display.scan_cycle = cycle;
            vma419_scan_display_quarter(&dmd_display);
            _delay_ms(1); 
        }
    }
    
    return 0; // Should not be reached
}