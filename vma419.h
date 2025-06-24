#ifndef VMA419_H
#define VMA419_H

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

//==============================================================================
// VMA419 LED MATRIX DISPLAY DRIVER LIBRARY
//==============================================================================
// 
// What this library does:
// This is the complete software driver to control a VMA419 LED matrix display.
// Think of it as the "brain" that tells each LED when to turn on or off.
//
// What kind of display this works with:
// - A 32x16 grid of red LEDs (that's 512 individual lights!)
// - Each LED can be either ON (bright) or OFF (dark)
// - Perfect for showing text, simple graphics, or animations
//
// How it talks to the display:
// - Uses SPI communication (like a very fast serial connection)
// - Controls which rows light up using special timing
// - Updates the display 250 times per second (so fast you don't see flicker)
//
// Technical details for the curious:
// - Works with AVR microcontrollers (like Arduino)
// - Uses 4-phase multiplexing (shows 1/4 of the display at a time, very quickly)
// - Memory efficient: only needs 64 bytes to store the whole image
//
// Simple rule: 1 = LED ON (bright), 0 = LED OFF (dark)
//==============================================================================

//------------------------------------------------------------------------------
// DISPLAY SIZE AND BASIC SETTINGS
//------------------------------------------------------------------------------
// These numbers define the size and capabilities of your LED display

#define VMA419_PIXELS_ACROSS_PER_PANEL 32    // How many LEDs wide (columns)
#define VMA419_PIXELS_DOWN_PER_PANEL   16    // How many LEDs tall (rows)
#define VMA419_BITSPERPIXEL             1    // Each LED needs 1 bit (ON or OFF)

// Memory calculation: How much RAM we need to store the entire display image
// Math: (32 pixels ÷ 8 bits per byte) × 16 rows = 4 × 16 = 64 bytes total
// That's tiny! Your phone's camera takes pictures that are millions of bytes
#define VMA419_RAM_SIZE_BYTES          ((VMA419_PIXELS_ACROSS_PER_PANEL*VMA419_BITSPERPIXEL/8)*VMA419_PIXELS_DOWN_PER_PANEL)

//------------------------------------------------------------------------------
// SPECIAL DRAWING MODES (for cool graphics effects)
//------------------------------------------------------------------------------
// These let you do more than just turn LEDs on or off - you can create special effects!

#define VMA419_GRAPHICS_NORMAL    0    // Normal mode: 1=turn LED on, 0=turn LED off
#define VMA419_GRAPHICS_INVERSE   1    // Opposite mode: 1=turn LED off, 0=turn LED on  
#define VMA419_GRAPHICS_TOGGLE    2    // Flip mode: 1=flip LED state (on becomes off, off becomes on)
#define VMA419_GRAPHICS_OR        3    // Add-only mode: can only turn LEDs on, never off
#define VMA419_GRAPHICS_NOR       4    // Subtract-only mode: can only turn LEDs off, never on

//------------------------------------------------------------------------------
// PIXEL LOOKUP TABLE (makes the code run faster)
//------------------------------------------------------------------------------
// This is a pre-calculated table that helps us quickly find which bit controls which LED.
// Think of it like a cheat sheet - instead of doing math every time, we just look up the answer.
//
// How it works: Each byte stores 8 pixels (8 LEDs). This table tells us which bit 
// in the byte controls which LED position.
// 
// Example: If we want to control LED at position 3, we look up position 3 in this table
// and get 0x10, which means "use bit 4 of the byte" (because 0x10 = binary 00010000)

static const uint8_t vma419_pixel_lookup_table[8] = {
    0x80,   // Position 0 uses bit 7 (leftmost bit: 10000000)
    0x40,   // Position 1 uses bit 6 (next bit:    01000000)
    0x20,   // Position 2 uses bit 5 (next bit:    00100000)
    0x10,   // Position 3 uses bit 4 (next bit:    00010000)
    0x08,   // Position 4 uses bit 3 (next bit:    00001000)
    0x04,   // Position 5 uses bit 2 (next bit:    00000100)
    0x02,   // Position 6 uses bit 1 (next bit:    00000010)
    0x01    // Position 7 uses bit 0 (rightmost:   00000001)
};

//------------------------------------------------------------------------------
// HARDWARE WIRING CONFIGURATION
//------------------------------------------------------------------------------
// This structure is like a wiring diagram in code form. It tells the software
// exactly which pins on your microcontroller connect to which wires on the LED display.
//
// Why we need so many pins:
// The LED display isn't smart - it needs the microcontroller to tell it:
// - What data to show (SPI pins)
// - Which rows to light up (A, B pins) 
// - When to update (latch pin)
// - When to turn on/off (output enable pin)
//
// Each pin needs 3 pieces of information:
// 1. DDR register: Sets the pin as input or output
// 2. PORT register: Controls the voltage on the pin (high or low)
// 3. Pin mask: Which specific pin number we're using

typedef struct {
    // Output Enable Pin: The master on/off switch for the entire display
    // When this pin is LOW, the display shows the image
    // When this pin is HIGH, the display goes dark (even if data is still there)
    volatile uint8_t* oe_port_ddr;      // Direction register (make it an output)
    volatile uint8_t* oe_port_out;      // Output register (control high/low)
    uint8_t oe_pin_mask;                // Which pin (example: (1<<PD7) means pin PD7)

    // Row Selection Pin A: Chooses which group of rows to light up
    // Part of a 2-bit address system (A is the low bit)
    volatile uint8_t* a_port_ddr;       // Direction register
    volatile uint8_t* a_port_out;       // Output register  
    uint8_t a_pin_mask;                 // Pin mask

    // Row Selection Pin B: The other part of the row selection
    // Part of a 2-bit address system (B is the high bit)
    // Together, A and B can select 4 different row groups (00, 01, 10, 11)
    volatile uint8_t* b_port_ddr;       // Direction register
    volatile uint8_t* b_port_out;       // Output register
    uint8_t b_pin_mask;                 // Pin mask

    // SPI Clock Pin: The timing signal for data transmission
    // This pin pulses high and low to tell the display "here comes the next bit of data"
    volatile uint8_t* spi_clk_port_ddr; // Direction register
    volatile uint8_t* spi_clk_port_out; // Output register
    uint8_t spi_clk_pin_mask;           // Pin mask

    // SPI Data Pin: The actual image data
    // This pin carries the 1s and 0s that represent which LEDs should be on or off
    // Data is sent one bit at a time, synchronized with the clock pin
    volatile uint8_t* spi_data_port_ddr; // Direction register
    volatile uint8_t* spi_data_port_out; // Output register
    uint8_t spi_data_pin_mask;          // Pin mask

    // Latch Clock Pin: The "update display now" signal
    // After sending all the data bits, pulse this pin to actually update what's shown
    // Think of it like pressing "Enter" after typing - it makes the changes visible
    volatile uint8_t* latch_clk_port_ddr; // Direction register
    volatile uint8_t* latch_clk_port_out; // Output register
    uint8_t latch_clk_pin_mask;          // Pin mask
} VMA419_PinConfig;

//------------------------------------------------------------------------------
// MAIN DISPLAY CONTROL STRUCTURE  
//------------------------------------------------------------------------------
// This is the "brain" of the display system. It holds everything the software needs
// to know about your display and its current state.
//
// Think of this like the control panel for the display - it contains:
// - The wiring information (how it's connected)
// - The size information (how big it is)  
// - The image memory (what's currently being shown)
// - The timing information (which part is being refreshed right now)

typedef struct {
    VMA419_PinConfig pins;          // The wiring diagram (which pins connect where)
    
    uint8_t panels_wide;            // How many display panels side-by-side (usually just 1)
    uint8_t panels_high;            // How many display panels stacked up (usually just 1)
    
    uint16_t total_width_pixels;    // Total width in LEDs (panels_wide × 32)
    uint16_t total_height_pixels;   // Total height in LEDs (panels_high × 16)
    
    uint8_t* frame_buffer;          // The image memory - this holds what's currently shown
                                    // Each bit represents one LED (1=on, 0=off)
    uint16_t frame_buffer_size;     // How many bytes the image memory uses
    
    uint8_t scan_cycle;             // Which row group is being displayed right now (0, 1, 2, or 3)
                                    // This cycles through 0→1→2→3→0→1→2→3... very quickly
} VMA419_Display;

//==============================================================================
// FUNCTIONS YOU CAN USE (THE PUBLIC API)
//==============================================================================
// These are all the functions available for you to control the LED display.
// Think of these as the "remote control buttons" for your display.

//------------------------------------------------------------------------------
// BASIC DISPLAY FUNCTIONS (start here!)
//------------------------------------------------------------------------------

/**
 * SET UP THE DISPLAY SYSTEM (call this first!)
 * 
 * This is like plugging in and turning on your TV for the first time.
 * You must call this function before you can do anything else with the display.
 * 
 * What you need to provide:
 * @param disp - A pointer to your display structure (the "control panel")
 * @param pin_config - A pointer to your wiring configuration (the "wiring diagram") 
 * @param panels_wide - How many displays side-by-side (usually 1)
 * @param panels_high - How many displays stacked up (usually 1)
 * 
 * What it returns:
 * @return 0 if everything worked perfectly
 * @return -1 if something went wrong (bad parameters or not enough memory)
 * 
 * What this function does behind the scenes:
 * - Sets up all the pins as outputs (so they can control the display)
 * - Allocates memory to store the image (the "frame buffer")
 * - Initializes the communication system
 * - Clears the display (turns all LEDs off)
 * - Gets ready for the first image update
 */
int vma419_init(VMA419_Display* disp, VMA419_PinConfig* pin_config, uint8_t panels_wide, uint8_t panels_high);

/**
 * TURN OFF ALL THE LEDS (make the display blank)
 * 
 * This is like pressing the "clear screen" button. After calling this function,
 * your display will be completely dark - all 512 LEDs will be turned off.
 * 
 * When to use this:
 * - Before drawing a new image (to start with a clean slate)
 * - To quickly hide everything on the display
 * - When you want to start over with a blank screen
 * 
 * @param disp - Pointer to your initialized display structure
 * 
 * Technical note: This sets all bits in the frame buffer to 0, which means
 * "LED OFF" in the VMA419's system (remember: 1=ON, 0=OFF)
 */
void vma419_clear(VMA419_Display* disp);

/**
 * TURN A SINGLE LED ON OR OFF (like drawing one pixel)
 * 
 * This lets you control just one LED out of the 512 on your display.
 * It's like having a very precise pencil that can draw individual dots.
 * 
 * How to use it:
 * @param disp - Pointer to your display structure  
 * @param x - Which column (0 to 31, left to right)
 * @param y - Which row (0 to 15, top to bottom)
 * @param color - What to do: 1 = turn LED on (bright), 0 = turn LED off (dark)
 * 
 * Examples:
 * vma419_set_pixel(&my_display, 0, 0, 1);    // Turn on top-left LED
 * vma419_set_pixel(&my_display, 31, 15, 1);  // Turn on bottom-right LED  
 * vma419_set_pixel(&my_display, 16, 8, 0);   // Turn off center LED
 * 
 * Important: The coordinates are automatically mapped to the correct physical
 * location on the display, so you don't need to worry about the internal wiring.
 */
void vma419_set_pixel(VMA419_Display* disp, uint16_t x, uint16_t y, uint8_t color);

/**
 * ADVANCED PIXEL CONTROL (for special effects and graphics)
 * 
 * This is like vma419_set_pixel() but with superpowers! Instead of just turning
 * LEDs on or off, you can do fancy operations like flipping their state or
 * only turning them on (never off).
 * 
 * Perfect for:
 * - Creating animations with flipping effects
 * - Drawing graphics that only add light (never subtract)
 * - Making patterns that interact with existing images
 * 
 * How to use it:
 * @param disp - Pointer to your display structure
 * @param x - Which column (0 to 31)  
 * @param y - Which row (0 to 15)
 * @param graphics_mode - What kind of operation to do:
 *   - VMA419_GRAPHICS_NORMAL: Normal mode (same as vma419_set_pixel)
 *   - VMA419_GRAPHICS_INVERSE: Opposite mode (1=off, 0=on)
 *   - VMA419_GRAPHICS_TOGGLE: Flip mode (1=flip the LED state)
 *   - VMA419_GRAPHICS_OR: Add-only (1=turn on, 0=leave alone)
 *   - VMA419_GRAPHICS_NOR: Subtract-only (1=turn off, 0=leave alone)
 * @param pixel - The value to use (1 = true, 0 = false)
 * 
 * Example: Create a blinking effect by toggling pixels
 * vma419_write_pixel(&display, 16, 8, VMA419_GRAPHICS_TOGGLE, 1);
 */
void vma419_write_pixel(VMA419_Display* disp, uint16_t x, uint16_t y, uint8_t graphics_mode, uint8_t pixel);

/**
 * REFRESH THE DISPLAY (the most important function!)
 * 
 * This is the "heartbeat" of your display system. You MUST call this function
 * continuously in a loop, or your display won't show anything!
 * 
 * Why this is needed:
 * The display can't show all 16 rows at once due to hardware limitations.
 * Instead, it shows 4 rows at a time, cycling through them very quickly:
 * - Phase 0: Show rows 0, 4, 8, 12
 * - Phase 1: Show rows 1, 5, 9, 13  
 * - Phase 2: Show rows 2, 6, 10, 14
 * - Phase 3: Show rows 3, 7, 11, 15
 * 
 * When you cycle through these phases fast enough (250+ times per second),
 * your eye sees a complete, flicker-free image!
 * 
 * How to use it:
 * @param disp - Pointer to your display structure
 * 
 * Typical usage pattern:
 * while(1) {
 *     for(int phase = 0; phase < 4; phase++) {
 *         disp.scan_cycle = phase;
 *         vma419_scan_display_quarter(&disp);
 *         delay(1ms);  // 1ms per phase = 250Hz refresh rate
 *     }
 * }
 * 
 * What this function does:
 * 1. Selects the right row group using the A and B pins
 * 2. Sends the pixel data for those rows via SPI
 * 3. Latches the data into the display drivers
 * 4. Enables the display output to show the result
 */
void vma419_scan_display_quarter(VMA419_Display* disp);

/**
 * CLEAN UP AND FREE MEMORY (call this when you're done)
 * 
 * This is like properly shutting down your computer - it cleans up any memory
 * that was allocated and makes sure everything is tidy before your program ends.
 * 
 * When to use this:
 * - When your program is about to end
 * - When you want to stop using the display
 * - Before re-initializing with different settings
 * 
 * @param disp - Pointer to the display structure to clean up
 * 
 * What it does:
 * - Frees the frame buffer memory that was allocated during initialization
 * - Resets the display structure to a safe state
 * 
 * Note: After calling this, you'll need to call vma419_init() again if you
 * want to use the display again.
 */
void vma419_deinit(VMA419_Display* disp);

#endif // VMA419_H