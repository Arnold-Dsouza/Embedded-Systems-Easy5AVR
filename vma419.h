#ifndef VMA419_H
#define VMA419_H

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h> // For uint8_t etc.

// Define panel characteristics
#define VMA419_PIXELS_ACROSS_PER_PANEL 32
#define VMA419_PIXELS_DOWN_PER_PANEL   16

// Hardware abstraction structure for pin configuration
// OE: Output Enable (active low)
// A, B: Row select pins
// SPI_CLK: SPI Clock (SCK)
// SPI_DATA: SPI Data (MOSI)
// LATCH_CLK: Storage Register Clock / Latch (SCLK on DMD419)
typedef struct {
    volatile uint8_t* oe_port_ddr;      // Data Direction Register for OE pin
    volatile uint8_t* oe_port_out;    // Output Register for OE pin
    uint8_t oe_pin_mask;            // Bit mask for OE pin

    volatile uint8_t* a_port_ddr;
    volatile uint8_t* a_port_out;
    uint8_t a_pin_mask;

    volatile uint8_t* b_port_ddr;
    volatile uint8_t* b_port_out;
    uint8_t b_pin_mask;

    volatile uint8_t* spi_clk_port_ddr; // SCK
    volatile uint8_t* spi_clk_port_out;
    uint8_t spi_clk_pin_mask;

    volatile uint8_t* spi_data_port_ddr; // MOSI
    volatile uint8_t* spi_data_port_out;
    uint8_t spi_data_pin_mask;

    volatile uint8_t* latch_clk_port_ddr;
    volatile uint8_t* latch_clk_port_out;
    uint8_t latch_clk_pin_mask;
} VMA419_PinConfig;

// Display control structure
typedef struct {
    VMA419_PinConfig pins;        // Pin configuration
    uint8_t panels_wide;        // Number of panels horizontally
    uint8_t panels_high;        // Number of panels vertically
    uint16_t total_width_pixels;  // Total display width in pixels
    uint16_t total_height_pixels; // Total display height in pixels
    uint8_t* frame_buffer;      // 1D array for pixel data
    uint16_t frame_buffer_size; // Size of frame_buffer in bytes
    uint8_t scan_cycle;         // Current phase of the 4-phase display scan (0-3)
} VMA419_Display;

// Core display functions
// Initializes the display structure, pins, and SPI.
// Returns 0 on success, -1 on failure (e.g., memory allocation).
int vma419_init(VMA419_Display* disp, VMA419_PinConfig* pin_config, uint8_t panels_wide, uint8_t panels_high);

// Clears the display buffer (all pixels off).
void vma419_clear(VMA419_Display* disp);

// Sets a pixel in the display buffer.
// (x, y) are 0-indexed coordinates.
// color: 1 for ON, 0 for OFF.
void vma419_set_pixel(VMA419_Display* disp, uint16_t x, uint16_t y, uint8_t color);

// Scans one quarter of the display. Call this repeatedly (e.g., from a timer interrupt).
void vma419_scan_display_quarter(VMA419_Display* disp);

// Optional: Timer setup for automatic refresh
void vma419_setup_timer_for_refresh(void (*refresh_func_ptr)(void));

// Deallocates resources used by the display
void vma419_deinit(VMA419_Display* disp);

#endif // VMA419_H