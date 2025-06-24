/*
 * VMA419 LED Matrix Display Driver Implementation
 * 
 * This file contains the implementation of the VMA419 32x16 LED matrix display driver.
 * The driver supports 4-phase multiplexing using hardware SPI for optimal performance.
 * 
 * Key Features:
 * - 32x16 pixel resolution per panel
 * - 4-phase row multiplexing (rows 0,4,8,12 → 1,5,9,13 → 2,6,10,14 → 3,7,11,15)
 * - Row remapping for correct physical display
 * - Hardware SPI for fast data transmission
 * - Multiple graphics modes (NORMAL, INVERSE, TOGGLE, OR, NOR)
 * 
 * Hardware Interface:
 * - Hardware SPI pins (MOSI, SCK) for data transmission
 * - A and B pins for row selection (2-bit binary)
 * - LATCH pin for transferring data to output latches
 * - OE (Output Enable) pin for display brightness control
 * 
 * Memory Layout:
 * - Uses DMD419-compatible memory layout for compatibility
 * - 1 bit per pixel (1=LED ON, 0=LED OFF)
 * - Frame buffer organized by panels and rows
 * 
 */

#include "vma419.h"
#include <stdlib.h> 
#include <string.h> 
#include <util/delay.h>

// ===============================================
// HELPER MACROS FOR PIN CONTROL
// ===============================================
#define PIN_MODE_OUTPUT(ddr_reg, pin_mask) (*(ddr_reg) |= (pin_mask))
#define PIN_SET_HIGH(port_reg, pin_mask)   (*(port_reg) |= (pin_mask))
#define PIN_SET_LOW(port_reg, pin_mask)    (*(port_reg) &= ~(pin_mask))

// ===============================================
// HARDWARE SPI FUNCTIONS
// ===============================================

/**
 * Initialize hardware SPI for VMA419 display communication
 * Configures ATmega16 SPI peripheral for optimal performance
 */
static void spi_init(void) {
    // Set SPI pins as outputs (MOSI=PB5, SCK=PB7, SS=PB4)
    DDRB |= (1 << PB5) | (1 << PB7) | (1 << PB4);
    
    // Configure SPI Control Register for maximum speed
    SPCR = (1 << SPE) |     // SPI Enable
           (1 << MSTR) |    // Master mode
           (0 << CPOL) |    // Clock polarity: idle low
           (0 << CPHA) |    // Clock phase: sample on leading edge
           (0 << SPR1) |    // SPI Speed: fosc/4 (fastest)
           (0 << SPR0);
    
    // Standard SPI timing (no double speed)
    SPSR &= ~(1 << SPI2X);
    
    // Set SS pin high (not used but good practice)
    PORTB |= (1 << PB4);
}

/**
 * Send one byte via hardware SPI to the VMA419 display
 * Uses ATmega16 SPI peripheral for fast data transmission
 * 
 * @param data Byte to transmit (MSB first)
 */
static void spi_transfer(uint8_t data) {
    // Hardware SPI transfer
    SPDR = data;                        // Load data into SPI Data Register
    while (!(SPSR & (1 << SPIF)));      // Wait for transmission complete
    // Reading SPSR and SPDR clears the SPIF flag automatically
    volatile uint8_t dummy = SPDR;      // Clear SPIF by reading SPDR
    (void)dummy;                        // Suppress unused variable warning
}



/**
 * Initialize VMA419 display driver
 * 
 * This function sets up the display structure, allocates frame buffer memory,
 * configures GPIO pins, and initializes the SPI communication.
 * 
 * @param disp Pointer to VMA419 display structure to initialize
 * @param pin_config Pointer to pin configuration structure
 * @param panels_wide Number of panels horizontally (usually 1)
 * @param panels_high Number of panels vertically (usually 1)
 * @return 0 on success, -1 on failure
 */
int vma419_init(VMA419_Display* disp, VMA419_PinConfig* pin_config, uint8_t panels_wide, uint8_t panels_high) {
    // Validate input parameters
    if (!disp || !pin_config || panels_wide == 0 || panels_high == 0) {
        return -1; // Invalid arguments
    }
    
    // Copy pin configuration and display dimensions
    disp->pins = *pin_config;
    disp->panels_wide = panels_wide;
    disp->panels_high = panels_high;
    disp->total_width_pixels = (uint16_t)panels_wide * VMA419_PIXELS_ACROSS_PER_PANEL;
    disp->total_height_pixels = (uint16_t)panels_high * VMA419_PIXELS_DOWN_PER_PANEL;

    // Calculate frame buffer size using DMD419-compatible formula
    uint16_t displays_total = panels_wide * panels_high;
    disp->frame_buffer_size = displays_total * VMA419_RAM_SIZE_BYTES;

    // Allocate frame buffer memory
    disp->frame_buffer = (uint8_t*)malloc(disp->frame_buffer_size);
    if (!disp->frame_buffer) {
        return -1; // Memory allocation failed
    }    // Configure GPIO pins as outputs
    PIN_MODE_OUTPUT(disp->pins.oe_port_ddr, disp->pins.oe_pin_mask);
    PIN_MODE_OUTPUT(disp->pins.a_port_ddr, disp->pins.a_pin_mask);
    PIN_MODE_OUTPUT(disp->pins.b_port_ddr, disp->pins.b_pin_mask);
    PIN_MODE_OUTPUT(disp->pins.latch_clk_port_ddr, disp->pins.latch_clk_pin_mask);

    // Set initial pin states
    PIN_SET_HIGH(disp->pins.oe_port_out, disp->pins.oe_pin_mask); // OE high = display disabled
    PIN_SET_LOW(disp->pins.a_port_out, disp->pins.a_pin_mask);    // Row select A = 0
    PIN_SET_LOW(disp->pins.b_port_out, disp->pins.b_pin_mask);    // Row select B = 0
    PIN_SET_LOW(disp->pins.latch_clk_port_out, disp->pins.latch_clk_pin_mask); // Latch low

    // Initialize hardware SPI and clear display
    spi_init();
    vma419_clear(disp);
    disp->scan_cycle = 0; // Start with first scan phase

    return 0; // Success
}

/**
 * Deinitialize VMA419 display driver
 * 
 * Frees allocated memory and cleans up resources.
 * 
 * @param disp Pointer to VMA419 display structure
 */
void vma419_deinit(VMA419_Display* disp) {
    if (disp && disp->frame_buffer) {
        free(disp->frame_buffer);
        disp->frame_buffer = NULL;
        disp->frame_buffer_size = 0;
    }
}

/**
 * Clear the display buffer (turn off all LEDs)
 * 
 * Sets all pixels in the frame buffer to 0 (LED OFF state).
 * Call vma419_scan_display_quarter() to update the physical display.
 * 
 * @param disp Pointer to VMA419 display structure
 */
void vma419_clear(VMA419_Display* disp) {
    if (disp && disp->frame_buffer) {
        // VMA419 uses 1=LED ON, 0=LED OFF
        // So clearing means setting all bits to 0
        memset(disp->frame_buffer, 0x00, disp->frame_buffer_size);
    }
}

/**
 * Remap logical rows to physical rows for VMA419 display
 * 
 * The VMA419 has a specific row mapping pattern:
 * Logical row 0→3, 1→0, 2→1, 3→2, then repeats for each group of 4 rows
 * This function converts logical row coordinates to physical row coordinates.
 * 
 * @param logical_y Logical row number (0-15)
 * @return Physical row number (0-15)
 */
static uint16_t vma419_remap_row(uint16_t logical_y) {
    // Group-based remapping for 16-row display
    uint16_t group = logical_y / 4;  // Which group of 4 rows (0, 1, 2, 3)
    uint16_t offset = logical_y % 4; // Position within the group
    
    uint16_t remapped_offset;
    switch (offset) {
        case 0: remapped_offset = 3; break; // 0→3, 4→7, 8→11, 12→15
        case 1: remapped_offset = 0; break; // 1→0, 5→4, 9→8, 13→12  
        case 2: remapped_offset = 1; break; // 2→1, 6→5, 10→9, 14→13
        case 3: remapped_offset = 2; break; // 3→2, 7→6, 11→10, 15→14
        default: remapped_offset = offset; break;
    }
    
    uint16_t physical_y = group * 4 + remapped_offset;
    
    // Boundary check
    if (physical_y >= VMA419_PIXELS_DOWN_PER_PANEL) {
        return logical_y; // Return original if out of bounds
    }
    
    return physical_y;
}

/**
 * Set a single pixel in the display buffer
 * 
 * This is the simplified pixel setting function for basic on/off control.
 * 
 * @param disp Pointer to VMA419 display structure
 * @param x X coordinate (0 to total_width_pixels-1)
 * @param y Y coordinate (0 to total_height_pixels-1) 
 * @param color Pixel state (1=ON, 0=OFF)
 */
void vma419_set_pixel(VMA419_Display* disp, uint16_t x, uint16_t y, uint8_t color) {
    if (!disp || !disp->frame_buffer || x >= disp->total_width_pixels || y >= disp->total_height_pixels) {
        return; // Invalid parameters or out of bounds
    }
    
    // Apply row remapping to convert logical to physical coordinates
    uint16_t physical_y = vma419_remap_row(y);

    // Calculate memory layout using DMD419-compatible addressing
    uint16_t panel = (x / VMA419_PIXELS_ACROSS_PER_PANEL) + (disp->panels_wide * (physical_y / VMA419_PIXELS_DOWN_PER_PANEL));
    uint16_t bX = (x % VMA419_PIXELS_ACROSS_PER_PANEL) + (panel << 5);  // panel * 32
    uint16_t bY = physical_y % VMA419_PIXELS_DOWN_PER_PANEL;
    
    // Calculate byte index in frame buffer
    uint16_t displays_total = disp->panels_wide * disp->panels_high;
    uint16_t byte_index = bX / 8 + bY * (displays_total << 2);
    
    // Get bit mask for this pixel position
    uint8_t bit_mask = vma419_pixel_lookup_table[bX & 0x07];
    
    // Set or clear the pixel bit
    if (byte_index < disp->frame_buffer_size) {
        if (color) { // Pixel ON (VMA419: 1 = LED ON)
            disp->frame_buffer[byte_index] |= bit_mask;
        } else { // Pixel OFF (VMA419: 0 = LED OFF)
            disp->frame_buffer[byte_index] &= ~bit_mask;
        }
    }
}

/**
 * Advanced pixel writing function with graphics modes
 * 
 * This function provides advanced pixel manipulation with various graphics modes
 * including normal, inverse, toggle, OR, and NOR operations.
 * 
 * @param disp Pointer to VMA419 display structure
 * @param x X coordinate (0 to total_width_pixels-1)
 * @param y Y coordinate (0 to total_height_pixels-1)
 * @param graphics_mode Graphics operation mode (NORMAL, INVERSE, TOGGLE, OR, NOR)
 * @param pixel Pixel value (1 or 0)
 */
void vma419_write_pixel(VMA419_Display* disp, uint16_t x, uint16_t y, uint8_t graphics_mode, uint8_t pixel) {
    if (!disp || !disp->frame_buffer || x >= disp->total_width_pixels || y >= disp->total_height_pixels) {
        return; // Invalid parameters or out of bounds
    }

    // Apply row remapping to convert logical to physical coordinates
    uint16_t physical_y = vma419_remap_row(y);

    // Calculate memory layout using DMD419-compatible addressing
    uint16_t panel = (x / VMA419_PIXELS_ACROSS_PER_PANEL) + (disp->panels_wide * (physical_y / VMA419_PIXELS_DOWN_PER_PANEL));
    uint16_t bX = (x % VMA419_PIXELS_ACROSS_PER_PANEL) + (panel << 5);  // panel * 32
    uint16_t bY = physical_y % VMA419_PIXELS_DOWN_PER_PANEL;
    
    // Calculate byte index in frame buffer
    uint16_t displays_total = disp->panels_wide * disp->panels_high;
    uint16_t ram_pointer = bX / 8 + bY * (displays_total << 2);
    
    // Get bit mask for this pixel position
    uint8_t lookup = vma419_pixel_lookup_table[bX & 0x07];
    
    // Boundary check
    if (ram_pointer >= disp->frame_buffer_size) {
        return;
    }
    
    // Apply graphics mode logic (VMA419: 1=LED ON, 0=LED OFF)
    switch (graphics_mode) {
        case VMA419_GRAPHICS_NORMAL:
            if (pixel == 1) {
                disp->frame_buffer[ram_pointer] |= lookup;   // Set bit = LED ON
            } else {
                disp->frame_buffer[ram_pointer] &= ~lookup;  // Clear bit = LED OFF
            }
            break;
            
        case VMA419_GRAPHICS_INVERSE:
            if (pixel == 0) {
                disp->frame_buffer[ram_pointer] |= lookup;   // Set bit = LED ON
            } else {
                disp->frame_buffer[ram_pointer] &= ~lookup;  // Clear bit = LED OFF
            }
            break;
            
        case VMA419_GRAPHICS_TOGGLE:
            if (pixel == 1) {
                if ((disp->frame_buffer[ram_pointer] & lookup) != 0) {
                    disp->frame_buffer[ram_pointer] &= ~lookup;  // LED was ON, turn OFF
                } else {
                    disp->frame_buffer[ram_pointer] |= lookup;   // LED was OFF, turn ON
                }
            }
            break;
            
        case VMA419_GRAPHICS_OR:
            // Only set pixels ON (OR operation)
            if (pixel == 1) {
                disp->frame_buffer[ram_pointer] |= lookup;
            }
            break;
            
        case VMA419_GRAPHICS_NOR:
            // Only clear pixels that are currently ON
            if ((pixel == 1) && ((disp->frame_buffer[ram_pointer] & lookup) != 0)) {
                disp->frame_buffer[ram_pointer] &= ~lookup;
            }
            break;
    }
}/**
 * Select row pair for multiplexed scanning
 * 
 * The VMA419 uses 4-phase multiplexing where rows are grouped into pairs:
 * Phase 0: rows 0,4,8,12  | Phase 1: rows 1,5,9,13
 * Phase 2: rows 2,6,10,14 | Phase 3: rows 3,7,11,15
 * 
 * Row selection is controlled by A and B pins in binary:
 * A=0,B=0 (0): rows 3,7,11,15 | A=1,B=0 (1): rows 0,4,8,12
 * A=0,B=1 (2): rows 1,5,9,13  | A=1,B=1 (3): rows 2,6,10,14
 * 
 * @param disp Pointer to VMA419 display structure
 * @param row_pair Row pair selection (0-3)
 */
static void select_row_pair(VMA419_Display* disp, uint8_t row_pair) {
    // VMA419 row selection mapping (determined through testing)
    uint8_t select_value = row_pair;
    
    // Set A pin (LSB)
    if (select_value & 0x01) {
        PIN_SET_HIGH(disp->pins.a_port_out, disp->pins.a_pin_mask);
    } else {
        PIN_SET_LOW(disp->pins.a_port_out, disp->pins.a_pin_mask);
    }

    // Set B pin (MSB)  
    if (select_value & 0x02) {
        PIN_SET_HIGH(disp->pins.b_port_out, disp->pins.b_pin_mask);
    } else {
        PIN_SET_LOW(disp->pins.b_port_out, disp->pins.b_pin_mask);
    }
}

/**
 * Scan and display one quarter of the VMA419 display
 * 
 * This function implements the core 4-phase multiplexing logic for the VMA419.
 * It sends data for 4 rows simultaneously using shift registers, then latches
 * the data and enables the selected row group.
 * 
 * The VMA419 multiplexing works as follows:
 * - 16 rows are divided into 4 groups of 4 rows each
 * - Each scan cycle displays one group: (0,4,8,12), (1,5,9,13), (2,6,10,14), (3,7,11,15)
 * - Data for all 4 rows in the group is sent via SPI in a specific pattern
 * - Row selection pins A,B select which group is active
 * 
 * @param disp Pointer to VMA419 display structure
 */
void vma419_scan_display_quarter(VMA419_Display* disp) {
    if (!disp || !disp->frame_buffer) return;

    // Disable display output during data transfer
    PIN_SET_HIGH(disp->pins.oe_port_out, disp->pins.oe_pin_mask);

    // Select the current row group for this scan cycle
    select_row_pair(disp, disp->scan_cycle);
    
    // Calculate addressing parameters (DMD419-compatible)
    uint16_t displays_total = disp->panels_wide * disp->panels_high;
    uint16_t rowsize = displays_total << 2;  // displays_total * 4 bytes per panel row
    uint16_t offset = rowsize * disp->scan_cycle;
    
    // Calculate row offset addresses for 4-row multiplexing
    // These match the DMD419 row addressing pattern
    uint16_t row1 = displays_total << 4;    // displays_total * 16
    uint16_t row2 = displays_total << 5;    // displays_total * 32  
    uint16_t row3 = displays_total * 48;    // displays_total * 48
    
    // Send data for all panels in the specific DMD419 SPI pattern
    // Each panel sends 16 bytes in this exact order for proper display
    for (uint16_t panel = 0; panel < displays_total; panel++) {
        // Send 16 bytes per panel in DMD419 order
        spi_transfer(disp->frame_buffer[offset + 0 + row3]);
        spi_transfer(disp->frame_buffer[offset + 0 + row2]);
        spi_transfer(disp->frame_buffer[offset + 1 + row3]);
        spi_transfer(disp->frame_buffer[offset + 1 + row2]);
        spi_transfer(disp->frame_buffer[offset + 0 + row1]);
        spi_transfer(disp->frame_buffer[offset + 0]);
        spi_transfer(disp->frame_buffer[offset + 1 + row1]);
        spi_transfer(disp->frame_buffer[offset + 1]);
        spi_transfer(disp->frame_buffer[offset + 2 + row3]);
        spi_transfer(disp->frame_buffer[offset + 2 + row2]);
        spi_transfer(disp->frame_buffer[offset + 3 + row3]);
        spi_transfer(disp->frame_buffer[offset + 3 + row2]);
        spi_transfer(disp->frame_buffer[offset + 2 + row1]);
        spi_transfer(disp->frame_buffer[offset + 2]);
        spi_transfer(disp->frame_buffer[offset + 3 + row1]);
        spi_transfer(disp->frame_buffer[offset + 3]);
        
        // Move to next panel's data
        offset += 4;
    }

    // Latch the data from shift registers to output latches
    PIN_SET_HIGH(disp->pins.latch_clk_port_out, disp->pins.latch_clk_pin_mask);
    _delay_us(10); // Latch pulse duration
    PIN_SET_LOW(disp->pins.latch_clk_port_out, disp->pins.latch_clk_pin_mask);

    // Enable display output for the selected row group
    PIN_SET_LOW(disp->pins.oe_port_out, disp->pins.oe_pin_mask);
}

/*
 * =============================================================================
 * VMA419 IMPLEMENTATION SUMMARY
 * =============================================================================
 * 
 * This implementation provides a complete VMA419 32x16 LED matrix display driver
 * optimized for AVR microcontrollers with the following key features:
 * 
 * DISPLAY ARCHITECTURE:
 * - 32×16 monochrome LED matrix (512 total LEDs)
 * - 4-phase row multiplexing (4 rows active simultaneously)
 * - DMD419-compatible memory layout for easy porting
 * - Row remapping function handles VMA419-specific physical layout
 * 
 * MULTIPLEXING PHASES:
 * Phase 0: Rows 0,4,8,12   | Phase 1: Rows 1,5,9,13
 * Phase 2: Rows 2,6,10,14  | Phase 3: Rows 3,7,11,15
 * 
 * MEMORY ORGANIZATION:
 * - 64 bytes frame buffer per panel (32×16 ÷ 8 bits/byte)
 * - 1 bit per pixel (1=LED ON, 0=LED OFF)
 * - Column-major bit ordering within bytes
 * - Supports multiple panels (extends horizontally/vertically)
 *  * HARDWARE INTERFACE:
 * - SPI_DATA (MOSI): Serial data transmission via hardware SPI
 * - SPI_CLK (SCK): Serial clock via hardware SPI
 * - A, B: 2-bit row selection (binary encoding)
 * - LATCH: Transfers shift register data to output latches
 * - OE: Output enable (active LOW, controls display brightness)
 * 
 * USAGE PATTERN:
 * 1. Call vma419_init() once to initialize
 * 2. Use vma419_set_pixel() or vma419_write_pixel() to draw
 * 3. Continuously call vma419_scan_display_quarter() in main loop
 *    cycling through scan_cycle 0-3 for full display refresh
 * * PERFORMANCE NOTES:
 * - Recommended refresh rate: 200-500 Hz (1-2.5ms per phase)
 * - Hardware SPI: ~1μs per byte (optimized data transfer)
 * - Full scan cycle: ~250μs for single panel with hardware SPI
 * - Memory usage: 64 bytes RAM per panel + structure overhead
 * 
 * COMPATIBILITY:
 * - Based on DMD419 library architecture
 * - Portable across AVR microcontroller families
 * - Pin assignments fully configurable
 * - Hardware SPI implementation for optimal performance
 * 
 * Author: Arnold Dsouza - Embedded Systems Course Project
 * Date: June 13, 2025
 * Version: 2.0 - Hardware SPI Implementation ✓ OPTIMIZED PERFORMANCE
 * =============================================================================
 */


