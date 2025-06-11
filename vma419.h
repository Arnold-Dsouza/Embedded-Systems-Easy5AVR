#ifndef VMA419_H
#define VMA419_H

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

//==============================================================================
// VMA419 LED MATRIX DISPLAY DRIVER LIBRARY
//==============================================================================
// This library provides a complete driver for the VMA419 32x16 LED matrix display.
// Compatible with AVR microcontrollers using bit-banged SPI communication.
//
// Hardware Specifications:
// - Display Size: 32 columns × 16 rows = 512 total LEDs
// - Color: Monochrome (Red LEDs)
// - Multiplex: 4-phase row scanning (4 rows active simultaneously)
// - Interface: SPI-like serial data + row select pins + control pins
// - Memory Layout: Based on DMD419 addressing scheme
//
// Polarity: VMA419 uses POSITIVE logic (1 = LED ON, 0 = LED OFF)
//==============================================================================

//------------------------------------------------------------------------------
// PANEL CHARACTERISTICS
//------------------------------------------------------------------------------
#define VMA419_PIXELS_ACROSS_PER_PANEL 32    // Number of columns per panel
#define VMA419_PIXELS_DOWN_PER_PANEL   16    // Number of rows per panel
#define VMA419_BITSPERPIXEL             1    // Monochrome: 1 bit per pixel

// Frame buffer size calculation: (32 pixels ÷ 8 bits/byte) × 16 rows = 64 bytes per panel
#define VMA419_RAM_SIZE_BYTES          ((VMA419_PIXELS_ACROSS_PER_PANEL*VMA419_BITSPERPIXEL/8)*VMA419_PIXELS_DOWN_PER_PANEL)

//------------------------------------------------------------------------------
// GRAPHICS MODES (for advanced pixel operations)
//------------------------------------------------------------------------------
#define VMA419_GRAPHICS_NORMAL    0    // Normal: Set pixel if value=1, clear if value=0
#define VMA419_GRAPHICS_INVERSE   1    // Inverse: Set pixel if value=0, clear if value=1
#define VMA419_GRAPHICS_TOGGLE    2    // Toggle: Flip pixel state if value=1
#define VMA419_GRAPHICS_OR        3    // OR: Only turn pixels ON, never OFF
#define VMA419_GRAPHICS_NOR       4    // NOR: Only turn ON pixels OFF, never ON

//------------------------------------------------------------------------------
// PIXEL LOOKUP TABLE (for fast bit manipulation)
//------------------------------------------------------------------------------
// This table converts column position (0-7) within a byte to the corresponding bit mask.
// Used for setting/clearing individual pixels within a byte of the frame buffer.
// Example: column 0 → bit 7 (0x80), column 7 → bit 0 (0x01)
static const uint8_t vma419_pixel_lookup_table[8] = {
    0x80,   // Column 0 → bit 7 (MSB)
    0x40,   // Column 1 → bit 6
    0x20,   // Column 2 → bit 5
    0x10,   // Column 3 → bit 4
    0x08,   // Column 4 → bit 3
    0x04,   // Column 5 → bit 2
    0x02,   // Column 6 → bit 1
    0x01    // Column 7 → bit 0 (LSB)
};

//------------------------------------------------------------------------------
// HARDWARE PIN CONFIGURATION STRUCTURE
//------------------------------------------------------------------------------
// This structure defines all the hardware pins needed to control the VMA419 display.
// Each pin requires: Data Direction Register (DDR), Output Port Register, and Pin Mask.
//
// Required Pins:
// - OE (Output Enable): Active LOW, controls display brightness/enable
// - A, B: Row selection pins (2-bit address for 4-phase multiplexing)
// - SPI_CLK: Serial clock for data transmission
// - SPI_DATA: Serial data line (MOSI equivalent)
// - LATCH_CLK: Storage register clock (latches data into display drivers)
typedef struct {
    // Output Enable Pin (Active LOW - LOW=display ON, HIGH=display OFF)
    volatile uint8_t* oe_port_ddr;      // Data Direction Register
    volatile uint8_t* oe_port_out;      // Output Port Register
    uint8_t oe_pin_mask;                // Pin bit mask (e.g., (1<<PD7))

    // Row Selection Pin A (LSB of 2-bit row address)
    volatile uint8_t* a_port_ddr;
    volatile uint8_t* a_port_out;
    uint8_t a_pin_mask;

    // Row Selection Pin B (MSB of 2-bit row address)
    volatile uint8_t* b_port_ddr;
    volatile uint8_t* b_port_out;
    uint8_t b_pin_mask;

    // SPI Clock Pin (Serial Clock - data is clocked on rising edge)
    volatile uint8_t* spi_clk_port_ddr;
    volatile uint8_t* spi_clk_port_out;
    uint8_t spi_clk_pin_mask;

    // SPI Data Pin (Serial Data - MOSI equivalent, MSB first transmission)
    volatile uint8_t* spi_data_port_ddr;
    volatile uint8_t* spi_data_port_out;
    uint8_t spi_data_pin_mask;

    // Latch Clock Pin (Storage Register Clock - latches SPI data into display drivers)
    volatile uint8_t* latch_clk_port_ddr;
    volatile uint8_t* latch_clk_port_out;
    uint8_t latch_clk_pin_mask;
} VMA419_PinConfig;

//------------------------------------------------------------------------------
// DISPLAY CONTROL STRUCTURE
//------------------------------------------------------------------------------
// Main structure that holds all display state and configuration data.
// This includes hardware pin configuration, panel layout, frame buffer, and scan state.
typedef struct {
    VMA419_PinConfig pins;          // Hardware pin configuration
    uint8_t panels_wide;            // Number of panels horizontally (typically 1)
    uint8_t panels_high;            // Number of panels vertically (typically 1)
    uint16_t total_width_pixels;    // Total display width in pixels (panels_wide × 32)
    uint16_t total_height_pixels;   // Total display height in pixels (panels_high × 16)
    uint8_t* frame_buffer;          // Frame buffer memory (dynamically allocated)
    uint16_t frame_buffer_size;     // Frame buffer size in bytes
    uint8_t scan_cycle;             // Current scan phase (0-3 for 4-phase multiplexing)
} VMA419_Display;

//==============================================================================
// PUBLIC API FUNCTIONS
//==============================================================================

//------------------------------------------------------------------------------
// CORE DISPLAY FUNCTIONS
//------------------------------------------------------------------------------

/**
 * @brief Initialize the VMA419 display system
 * @param disp Pointer to display structure to initialize
 * @param pin_config Pointer to pin configuration structure
 * @param panels_wide Number of panels horizontally (typically 1)
 * @param panels_high Number of panels vertically (typically 1)
 * @return 0 on success, -1 on failure (invalid parameters or memory allocation error)
 * 
 * This function:
 * - Configures all GPIO pins as outputs
 * - Allocates frame buffer memory
 * - Initializes SPI communication
 * - Clears the display
 * - Sets initial scan state
 */
int vma419_init(VMA419_Display* disp, VMA419_PinConfig* pin_config, uint8_t panels_wide, uint8_t panels_high);

/**
 * @brief Clear the entire display (turn all LEDs OFF)
 * @param disp Pointer to initialized display structure
 * 
 * Sets all bits in the frame buffer to 0 (VMA419 uses 1=ON, 0=OFF polarity)
 */
void vma419_clear(VMA419_Display* disp);

/**
 * @brief Set or clear a single pixel
 * @param disp Pointer to initialized display structure
 * @param x Column coordinate (0-31 for single panel)
 * @param y Row coordinate (0-15 for single panel)  
 * @param color 1 = LED ON, 0 = LED OFF
 * 
 * Note: Uses logical coordinates that are automatically mapped to physical display rows
 */
void vma419_set_pixel(VMA419_Display* disp, uint16_t x, uint16_t y, uint8_t color);

/**
 * @brief Advanced pixel setting with graphics modes
 * @param disp Pointer to initialized display structure
 * @param x Column coordinate
 * @param y Row coordinate
 * @param graphics_mode Graphics operation mode (VMA419_GRAPHICS_NORMAL, etc.)
 * @param pixel 1 = pixel value true, 0 = pixel value false
 * 
 * Supports advanced operations like XOR, OR, toggle for graphics effects
 */
void vma419_write_pixel(VMA419_Display* disp, uint16_t x, uint16_t y, uint8_t graphics_mode, uint8_t pixel);

/**
 * @brief Scan one quarter of the display (must be called continuously)
 * @param disp Pointer to initialized display structure
 * 
 * This function handles the 4-phase multiplexing by:
 * 1. Selecting the appropriate row group via A,B pins
 * 2. Transmitting pixel data for all 4 active rows via SPI
 * 3. Latching the data into the display drivers
 * 4. Enabling the display output
 * 
 * Must be called in a loop, cycling through scan_cycle 0-3 for full display refresh
 */
void vma419_scan_display_quarter(VMA419_Display* disp);

/**
 * @brief Deallocate display resources
 * @param disp Pointer to display structure to deinitialize
 * 
 * Frees the dynamically allocated frame buffer memory
 */
void vma419_deinit(VMA419_Display* disp);

#endif // VMA419_H