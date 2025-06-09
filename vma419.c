#include "vma419.h"
#include <stdlib.h> // For malloc, free
#include <string.h> // For memset
#include <avr/interrupt.h> // For timer interrupts if used

// --- Private Helper Macros for Pin Control ---
#define PIN_MODE_OUTPUT(ddr_reg, pin_mask) (*(ddr_reg) |= (pin_mask))
#define PIN_SET_HIGH(port_reg, pin_mask)   (*(port_reg) |= (pin_mask))
#define PIN_SET_LOW(port_reg, pin_mask)    (*(port_reg) &= ~(pin_mask))

// --- Private SPI Functions ---
static VMA419_Display* g_spi_display = NULL; // Global pointer for bit-banging

static void spi_init(VMA419_Display* disp) {
    g_spi_display = disp; // Store display pointer for bit-banging
    
}

static void spi_transfer(uint8_t data) {
    if (!g_spi_display) return;
    
    // Bit-bang SPI transfer
    for (uint8_t bit = 0; bit < 8; bit++) {
        // Set data line based on MSB
        if (data & 0x80) {
            PIN_SET_HIGH(g_spi_display->pins.spi_data_port_out, g_spi_display->pins.spi_data_pin_mask);
        } else {
            PIN_SET_LOW(g_spi_display->pins.spi_data_port_out, g_spi_display->pins.spi_data_pin_mask);
        }
          // Clock pulse
        PIN_SET_HIGH(g_spi_display->pins.spi_clk_port_out, g_spi_display->pins.spi_clk_pin_mask);
        _delay_us(2); // Increased delay for clock high
        PIN_SET_LOW(g_spi_display->pins.spi_clk_port_out, g_spi_display->pins.spi_clk_pin_mask);
        _delay_us(2); // Increased delay for clock low
        
        data <<= 1; // Shift to next bit
    }
}



int vma419_init(VMA419_Display* disp, VMA419_PinConfig* pin_config, uint8_t panels_wide, uint8_t panels_high) {
    if (!disp || !pin_config || panels_wide == 0 || panels_high == 0) {
        return -1; // Invalid arguments
    }

    disp->pins = *pin_config; // Copy pin configuration
    disp->panels_wide = panels_wide;
    disp->panels_high = panels_high;
    disp->total_width_pixels = (uint16_t)panels_wide * VMA419_PIXELS_ACROSS_PER_PANEL;
    disp->total_height_pixels = (uint16_t)panels_high * VMA419_PIXELS_DOWN_PER_PANEL;

    // Calculate frame buffer size: (total_width * total_height) / 8 bits_per_byte
    // The DMD stores data per row, so it's (width_in_bytes_per_panel * panels_wide) * total_height_rows
    disp->frame_buffer_size = ( ( ( (uint16_t)VMA419_PIXELS_ACROSS_PER_PANEL / 8U) * disp->panels_wide ) * disp->total_height_pixels );

    disp->frame_buffer = (uint8_t*)malloc(disp->frame_buffer_size);
    if (!disp->frame_buffer) {
        return -1; // Memory allocation failed
    }

    // Initialize pin directions
    PIN_MODE_OUTPUT(disp->pins.oe_port_ddr, disp->pins.oe_pin_mask);
    PIN_MODE_OUTPUT(disp->pins.a_port_ddr, disp->pins.a_pin_mask);
    PIN_MODE_OUTPUT(disp->pins.b_port_ddr, disp->pins.b_pin_mask);
    PIN_MODE_OUTPUT(disp->pins.spi_clk_port_ddr, disp->pins.spi_clk_pin_mask);
    PIN_MODE_OUTPUT(disp->pins.spi_data_port_ddr, disp->pins.spi_data_pin_mask);
    PIN_MODE_OUTPUT(disp->pins.latch_clk_port_ddr, disp->pins.latch_clk_pin_mask);

    // Set initial pin states
    PIN_SET_HIGH(disp->pins.oe_port_out, disp->pins.oe_pin_mask); // OE is active low, so high disables output
    PIN_SET_LOW(disp->pins.a_port_out, disp->pins.a_pin_mask);
    PIN_SET_LOW(disp->pins.b_port_out, disp->pins.b_pin_mask);
    PIN_SET_LOW(disp->pins.spi_clk_port_out, disp->pins.spi_clk_pin_mask);    PIN_SET_LOW(disp->pins.spi_data_port_out, disp->pins.spi_data_pin_mask);
    PIN_SET_LOW(disp->pins.latch_clk_port_out, disp->pins.latch_clk_pin_mask);

    spi_init(disp); // Pass display pointer
    vma419_clear(disp);
    disp->scan_cycle = 0;

    return 0; // Success
}

void vma419_deinit(VMA419_Display* disp) {
    if (disp && disp->frame_buffer) {
        free(disp->frame_buffer);
        disp->frame_buffer = NULL;
    }
    // Optionally, could reset SPI and pin states here if needed
}

void vma419_clear(VMA419_Display* disp) {
    if (disp && disp->frame_buffer) {
        // Pixels are ON when the bit is 0, OFF when 1 (as per original DMD419 library)
        // So, to clear (all OFF), set all bits to 1.
        memset(disp->frame_buffer, 0x00, disp->frame_buffer_size);  // Try 0x00 instead of 0xFF
    }
}

void vma419_set_pixel(VMA419_Display* disp, uint16_t x, uint16_t y, uint8_t color) {
    if (!disp || !disp->frame_buffer || x >= disp->total_width_pixels || y >= disp->total_height_pixels) {
        return;
    }

    // Calculate the byte index and bit mask for the pixel
    // Each panel is 32 pixels (4 bytes) wide.
    // Data is stored row by row, with multiple panels concatenated horizontally for each row segment.
    uint16_t panel_x_offset = (x / VMA419_PIXELS_ACROSS_PER_PANEL) * (VMA419_PIXELS_ACROSS_PER_PANEL / 8);
    uint16_t x_in_panel = x % VMA419_PIXELS_ACROSS_PER_PANEL;
    uint16_t byte_in_row = panel_x_offset + (x_in_panel / 8);
    
    uint16_t bytes_per_logical_row = (disp->total_width_pixels / 8);
    uint16_t byte_index = (y * bytes_per_logical_row) + byte_in_row;
    
    uint8_t bit_mask = 0x80 >> (x_in_panel % 8);    if (byte_index < disp->frame_buffer_size) {
        if (color) { // Pixel ON (bit = 1)  - Try inverting this logic
            disp->frame_buffer[byte_index] |= bit_mask;
        } else { // Pixel OFF (bit = 0)
            disp->frame_buffer[byte_index] &= ~bit_mask;
        }
    }
}

// Selects the DMD row pair (0-3 for a 16-pixel high display, multiplexed in 4 phases)
static void select_row_pair(VMA419_Display* disp, uint8_t row_pair) {
    // Row selection logic (A and B pins)
    // The VMA419 LED matrix seems to have inverted row selection mapping:
    // Row Pair | B | A | Physical Rows
    // ---------|---|---|---------------
    // 0 (0,4)  | H | H | rows 0, 4, 8, 12
    // 1 (1,5)  | H | L | rows 1, 5, 9, 13  
    // 2 (2,6)  | L | H | rows 2, 6, 10, 14
    // 3 (3,7)  | L | L | rows 3, 7, 11, 15
    
    // Invert the row_pair logic for correct mapping
    uint8_t inverted_pair = 4 - row_pair;
    
    if (inverted_pair & 0x01) { // Check LSB for A pin
        PIN_SET_HIGH(disp->pins.a_port_out, disp->pins.a_pin_mask);
    } else {
        PIN_SET_LOW(disp->pins.a_port_out, disp->pins.a_pin_mask);
    }

    if (inverted_pair & 0x02) { // Check MSB for B pin
        PIN_SET_HIGH(disp->pins.b_port_out, disp->pins.b_pin_mask);
    } else {
        PIN_SET_LOW(disp->pins.b_port_out, disp->pins.b_pin_mask);
    }
}

void vma419_scan_display_quarter(VMA419_Display* disp) {
    if (!disp || !disp->frame_buffer) return;

    PIN_SET_HIGH(disp->pins.oe_port_out, disp->pins.oe_pin_mask); // Disable display output

    select_row_pair(disp, disp->scan_cycle);

    // Calculate the starting offset in the frame buffer for the current scan cycle.
    // Each scan cycle handles 1/4 of the rows.
    // For a 16-pixel high display, each cycle handles 4 effective rows.
    // The data for these 4 rows is interleaved in the original DMD419 RAM structure.
    // Example: scan_cycle 0 -> rows 0, 4, 8, 12
    //          scan_cycle 1 -> rows 1, 5, 9, 13 etc.
    // The frame_buffer here is organized linearly: row0_data, row1_data, ...
    // We need to pick data for the rows corresponding to the current scan_cycle.

    uint16_t bytes_per_logical_row = (disp->total_width_pixels / 8);

    for (uint8_t y_segment = 0; y_segment < disp->panels_high * (VMA419_PIXELS_DOWN_PER_PANEL / 4); ++y_segment) {
        // Determine the actual row in the display based on y_segment and scan_cycle
        uint16_t current_physical_row = (y_segment * 4) + disp->scan_cycle;
        if (current_physical_row >= disp->total_height_pixels) continue;

        uint8_t* row_data_ptr = disp->frame_buffer + (current_physical_row * bytes_per_logical_row);

        for (uint16_t byte_col = 0; byte_col < bytes_per_logical_row; ++byte_col) {
            spi_transfer(row_data_ptr[byte_col]);
        }
    }    // Latch data
    PIN_SET_HIGH(disp->pins.latch_clk_port_out, disp->pins.latch_clk_pin_mask);
    _delay_us(10); // Longer latch pulse
    PIN_SET_LOW(disp->pins.latch_clk_port_out, disp->pins.latch_clk_pin_mask);    PIN_SET_LOW(disp->pins.oe_port_out, disp->pins.oe_pin_mask); // Enable display output

    // Note: scan_cycle is now managed externally in main loop
}

// --- Optional Timer Setup ---
// This is a basic example. You'll need to configure the timer specific to your AVR.
// For example, using Timer1 in CTC mode.
static void (*g_refresh_callback)(void) = NULL;

#if defined (__AVR_ATmega328P__) || defined (__AVR_ATmega168__) // Example for common Arduinos
ISR(TIMER1_COMPA_vect) {
    if (g_refresh_callback) {
        g_refresh_callback();
    }
}
#endif

void vma419_setup_timer_for_refresh(void (*refresh_func_ptr)(void)) {
    g_refresh_callback = refresh_func_ptr;

    // Example: Setup Timer1 for ~200Hz refresh rate (adjust OCR1A and prescaler)
    // Assumes F_CPU = 16MHz. For 200Hz, period is 5ms.
    // T = (Prescaler / F_CPU) * (OCR1A + 1)
    // OCR1A + 1 = (T * F_CPU) / Prescaler
    // OCR1A + 1 = (0.005 * 16,000,000) / 64 = 80,000 / 64 = 1250
    // OCR1A = 1249

#if defined (__AVR_ATmega328P__) || defined (__AVR_ATmega168__)
    cli(); // Disable global interrupts
    TCCR1A = 0; // Set entire TCCR1A register to 0
    TCCR1B = 0; // Same for TCCR1B
    TCNT1  = 0; // Initialize counter value to 0

    OCR1A = 1249; // Compare match register (16MHz/64/1250 = 200Hz)
    TCCR1B |= (1 << WGM12); // CTC mode
    TCCR1B |= (1 << CS11) | (1 << CS10); // Set CS11 and CS10 bits for 64 prescaler
    TIMSK1 |= (1 << OCIE1A); // Enable timer compare interrupt
    sei(); // Enable global interrupts
#else
    // Implement timer setup for your specific AVR microcontroller
#endif
}