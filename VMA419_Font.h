/*
 * VMA419_Font.h - Simple Font System for VMA419 LED Matrix Display
 * 
 * This file provides a complete font rendering system for VMA419 displays.
 * It includes font data and all necessary functions in a single header file.
 * 
 * Features:
 * - 5x7 pixel font with 95 printable ASCII characters
 * - Simple API: select font, draw characters and strings
 * - Compatible with existing VMA419 driver
 * - Optimized for embedded systems
 * 
 * Usage:
 *   #include "VMA419_Font.h"
 *   vma419_font_init(&display);
 *   vma419_font_draw_string(&display, 0, 0, "Hello");
 * 
 * Author: Arnold Dsouza
 * Date: June 12, 2025
 */

#ifndef VMA419_FONT_H
#define VMA419_FONT_H

#include <avr/pgmspace.h>
#include <stdint.h>
#include "vma419.h"

//==============================================================================
// FONT DATA - 5x7 SYSTEM FONT
//==============================================================================

// Font characteristics
#define VMA419_FONT_WIDTH  5
#define VMA419_FONT_HEIGHT 7
#define VMA419_FONT_FIRST_CHAR 32  // Space character
#define VMA419_FONT_CHAR_COUNT 95  // Characters 32-126

// 5x7 Font Data - Each character is 5 bytes (5 columns)
// Bit 0 = top row, Bit 6 = bottom row of 7-pixel high character
const uint8_t vma419_font_5x7[] PROGMEM = {
    // Character 32 (0x20): ' ' (space)
    0x00, 0x00, 0x00, 0x00, 0x00,
    
    // Character 33 (0x21): '!'
    0x00, 0x00, 0x5F, 0x00, 0x00,
    
    // Character 34 (0x22): '"'
    0x00, 0x07, 0x00, 0x07, 0x00,
    
    // Character 35 (0x23): '#'
    0x14, 0x7F, 0x14, 0x7F, 0x14,
    
    // Character 36 (0x24): '$'
    0x24, 0x2A, 0x7F, 0x2A, 0x12,
    
    // Character 37 (0x25): '%'
    0x23, 0x13, 0x08, 0x64, 0x62,
    
    // Character 38 (0x26): '&'
    0x36, 0x49, 0x55, 0x22, 0x50,
    
    // Character 39 (0x27): '''
    0x00, 0x05, 0x03, 0x00, 0x00,
    
    // Character 40 (0x28): '('
    0x00, 0x1C, 0x22, 0x41, 0x00,
    
    // Character 41 (0x29): ')'
    0x00, 0x41, 0x22, 0x1C, 0x00,
    
    // Character 42 (0x2A): '*'
    0x08, 0x2A, 0x1C, 0x2A, 0x08,
    
    // Character 43 (0x2B): '+'
    0x08, 0x08, 0x3E, 0x08, 0x08,
    
    // Character 44 (0x2C): ','
    0x00, 0x50, 0x30, 0x00, 0x00,
    
    // Character 45 (0x2D): '-'
    0x08, 0x08, 0x08, 0x08, 0x08,
    
    // Character 46 (0x2E): '.'
    0x00, 0x60, 0x60, 0x00, 0x00,
    
    // Character 47 (0x2F): '/'
    0x20, 0x10, 0x08, 0x04, 0x02,
    
    // Character 48 (0x30): '0'
    0x3E, 0x51, 0x49, 0x45, 0x3E,
    
    // Character 49 (0x31): '1'
    0x00, 0x42, 0x7F, 0x40, 0x00,
    
    // Character 50 (0x32): '2'
    0x42, 0x61, 0x51, 0x49, 0x46,
    
    // Character 51 (0x33): '3'
    0x21, 0x41, 0x45, 0x4B, 0x31,
    
    // Character 52 (0x34): '4'
    0x18, 0x14, 0x12, 0x7F, 0x10,
    
    // Character 53 (0x35): '5'
    0x27, 0x45, 0x45, 0x45, 0x39,
    
    // Character 54 (0x36): '6'
    0x3C, 0x4A, 0x49, 0x49, 0x30,
    
    // Character 55 (0x37): '7'
    0x01, 0x71, 0x09, 0x05, 0x03,
    
    // Character 56 (0x38): '8'
    0x36, 0x49, 0x49, 0x49, 0x36,
    
    // Character 57 (0x39): '9'
    0x06, 0x49, 0x49, 0x29, 0x1E,
    
    // Character 58 (0x3A): ':'
    0x00, 0x36, 0x36, 0x00, 0x00,
    
    // Character 59 (0x3B): ';'
    0x00, 0x56, 0x36, 0x00, 0x00,
    
    // Character 60 (0x3C): '<'
    0x00, 0x08, 0x14, 0x22, 0x41,
    
    // Character 61 (0x3D): '='
    0x14, 0x14, 0x14, 0x14, 0x14,
    
    // Character 62 (0x3E): '>'
    0x41, 0x22, 0x14, 0x08, 0x00,
    
    // Character 63 (0x3F): '?'
    0x02, 0x01, 0x51, 0x09, 0x06,
    
    // Character 64 (0x40): '@'
    0x32, 0x49, 0x79, 0x41, 0x3E,
    
    // Character 65 (0x41): 'A'
    0x7E, 0x11, 0x11, 0x11, 0x7E,
    
    // Character 66 (0x42): 'B'
    0x7F, 0x49, 0x49, 0x49, 0x36,
    
    // Character 67 (0x43): 'C'
    0x3E, 0x41, 0x41, 0x41, 0x22,
    
    // Character 68 (0x44): 'D'
    0x7F, 0x41, 0x41, 0x22, 0x1C,
    
    // Character 69 (0x45): 'E'
    0x7F, 0x49, 0x49, 0x49, 0x41,
    
    // Character 70 (0x46): 'F'
    0x7F, 0x09, 0x09, 0x01, 0x01,
    
    // Character 71 (0x47): 'G'
    0x3E, 0x41, 0x41, 0x51, 0x32,
    
    // Character 72 (0x48): 'H'
    0x7F, 0x08, 0x08, 0x08, 0x7F,
    
    // Character 73 (0x49): 'I'
    0x00, 0x41, 0x7F, 0x41, 0x00,
    
    // Character 74 (0x4A): 'J'
    0x20, 0x40, 0x41, 0x3F, 0x01,
    
    // Character 75 (0x4B): 'K'
    0x7F, 0x08, 0x14, 0x22, 0x41,
    
    // Character 76 (0x4C): 'L'
    0x7F, 0x40, 0x40, 0x40, 0x40,
    
    // Character 77 (0x4D): 'M'
    0x7F, 0x02, 0x04, 0x02, 0x7F,
    
    // Character 78 (0x4E): 'N'
    0x7F, 0x04, 0x08, 0x10, 0x7F,
    
    // Character 79 (0x4F): 'O'
    0x3E, 0x41, 0x41, 0x41, 0x3E,
    
    // Character 80 (0x50): 'P'
    0x7F, 0x09, 0x09, 0x09, 0x06,
    
    // Character 81 (0x51): 'Q'
    0x3E, 0x41, 0x51, 0x21, 0x5E,
    
    // Character 82 (0x52): 'R'
    0x7F, 0x09, 0x19, 0x29, 0x46,
    
    // Character 83 (0x53): 'S'
    0x46, 0x49, 0x49, 0x49, 0x31,
    
    // Character 84 (0x54): 'T'
    0x01, 0x01, 0x7F, 0x01, 0x01,
    
    // Character 85 (0x55): 'U'
    0x3F, 0x40, 0x40, 0x40, 0x3F,
    
    // Character 86 (0x56): 'V'
    0x1F, 0x20, 0x40, 0x20, 0x1F,
    
    // Character 87 (0x57): 'W'
    0x7F, 0x20, 0x18, 0x20, 0x7F,
    
    // Character 88 (0x58): 'X'
    0x63, 0x14, 0x08, 0x14, 0x63,
    
    // Character 89 (0x59): 'Y'
    0x03, 0x04, 0x78, 0x04, 0x03,
    
    // Character 90 (0x5A): 'Z'
    0x61, 0x51, 0x49, 0x45, 0x43,
    
    // Character 91 (0x5B): '['
    0x00, 0x00, 0x7F, 0x41, 0x41,
    
    // Character 92 (0x5C): '\'
    0x02, 0x04, 0x08, 0x10, 0x20,
    
    // Character 93 (0x5D): ']'
    0x41, 0x41, 0x7F, 0x00, 0x00,
    
    // Character 94 (0x5E): '^'
    0x04, 0x02, 0x01, 0x02, 0x04,
    
    // Character 95 (0x5F): '_'
    0x40, 0x40, 0x40, 0x40, 0x40,
    
    // Character 96 (0x60): '`'
    0x00, 0x01, 0x02, 0x04, 0x00,
    
    // Character 97 (0x61): 'a'
    0x20, 0x54, 0x54, 0x54, 0x78,
    
    // Character 98 (0x62): 'b'
    0x7F, 0x48, 0x44, 0x44, 0x38,
    
    // Character 99 (0x63): 'c'
    0x38, 0x44, 0x44, 0x44, 0x20,
    
    // Character 100 (0x64): 'd'
    0x38, 0x44, 0x44, 0x48, 0x7F,
    
    // Character 101 (0x65): 'e'
    0x38, 0x54, 0x54, 0x54, 0x18,
    
    // Character 102 (0x66): 'f'
    0x08, 0x7E, 0x09, 0x01, 0x02,
    
    // Character 103 (0x67): 'g'
    0x08, 0x14, 0x54, 0x54, 0x3C,
    
    // Character 104 (0x68): 'h'
    0x7F, 0x08, 0x04, 0x04, 0x78,
    
    // Character 105 (0x69): 'i'
    0x00, 0x44, 0x7D, 0x40, 0x00,
    
    // Character 106 (0x6A): 'j'
    0x20, 0x40, 0x44, 0x3D, 0x00,
    
    // Character 107 (0x6B): 'k'
    0x00, 0x7F, 0x10, 0x28, 0x44,
    
    // Character 108 (0x6C): 'l'
    0x00, 0x41, 0x7F, 0x40, 0x00,
    
    // Character 109 (0x6D): 'm'
    0x7C, 0x04, 0x18, 0x04, 0x78,
    
    // Character 110 (0x6E): 'n'
    0x7C, 0x08, 0x04, 0x04, 0x78,
    
    // Character 111 (0x6F): 'o'
    0x38, 0x44, 0x44, 0x44, 0x38,
    
    // Character 112 (0x70): 'p'
    0x7C, 0x14, 0x14, 0x14, 0x08,
    
    // Character 113 (0x71): 'q'
    0x08, 0x14, 0x14, 0x18, 0x7C,
    
    // Character 114 (0x72): 'r'
    0x7C, 0x08, 0x04, 0x04, 0x08,
    
    // Character 115 (0x73): 's'
    0x48, 0x54, 0x54, 0x54, 0x20,
    
    // Character 116 (0x74): 't'
    0x04, 0x3F, 0x44, 0x40, 0x20,
    
    // Character 117 (0x75): 'u'
    0x3C, 0x40, 0x40, 0x20, 0x7C,
    
    // Character 118 (0x76): 'v'
    0x1C, 0x20, 0x40, 0x20, 0x1C,
    
    // Character 119 (0x77): 'w'
    0x3C, 0x40, 0x30, 0x40, 0x3C,
    
    // Character 120 (0x78): 'x'
    0x44, 0x28, 0x10, 0x28, 0x44,
    
    // Character 121 (0x79): 'y'
    0x0C, 0x50, 0x50, 0x50, 0x3C,
    
    // Character 122 (0x7A): 'z'
    0x44, 0x64, 0x54, 0x4C, 0x44,
    
    // Character 123 (0x7B): '{'
    0x00, 0x08, 0x36, 0x41, 0x00,
    
    // Character 124 (0x7C): '|'
    0x00, 0x00, 0x7F, 0x00, 0x00,
    
    // Character 125 (0x7D): '}'
    0x00, 0x41, 0x36, 0x08, 0x00,
    
    // Character 126 (0x7E): '~'
    0x08, 0x08, 0x2A, 0x1C, 0x08
};

//==============================================================================
// FONT RENDERING FUNCTIONS
//==============================================================================

/**
 * Initialize font system (call this once)
 * @param disp Pointer to VMA419 display structure
 */
static inline void vma419_font_init(VMA419_Display* disp) {
    // Font system is ready to use - no initialization needed
    (void)disp; // Suppress unused parameter warning
}

/**
 * Draw a single character
 * @param disp Pointer to VMA419 display structure
 * @param x X coordinate (column)
 * @param y Y coordinate (row)
 * @param c Character to draw
 * @return Width of character drawn (always 5 for this font)
 */
static inline uint8_t vma419_font_draw_char(VMA419_Display* disp, int16_t x, int16_t y, char c) {
    // Check if character is in valid range
    if (c < VMA419_FONT_FIRST_CHAR || c >= (VMA419_FONT_FIRST_CHAR + VMA419_FONT_CHAR_COUNT)) {
        return 0; // Character not available
    }
    
    // Check bounds
    if (x >= 32 || y >= 16) return 0;
    if ((x + VMA419_FONT_WIDTH) < 0 || (y + VMA419_FONT_HEIGHT) < 0) return VMA419_FONT_WIDTH;
    
    // Calculate character index and data offset
    uint8_t char_index = c - VMA419_FONT_FIRST_CHAR;
    uint16_t data_offset = char_index * VMA419_FONT_WIDTH;
    
    // Draw character column by column
    for (uint8_t col = 0; col < VMA419_FONT_WIDTH; col++) {
        int16_t pixel_x = x + col;
        if (pixel_x >= 0 && pixel_x < 32) {
            // Read column data from PROGMEM
            uint8_t column_data = pgm_read_byte(&vma419_font_5x7[data_offset + col]);
            
            // Draw pixels in this column
            for (uint8_t row = 0; row < VMA419_FONT_HEIGHT; row++) {
                int16_t pixel_y = y + row;
                if (pixel_y >= 0 && pixel_y < 16) {
                    if (column_data & (1 << row)) {
                        vma419_set_pixel(disp, pixel_x, pixel_y, 1);
                    }
                }
            }
        }
    }
    
    return VMA419_FONT_WIDTH;
}

/**
 * Draw a text string
 * @param disp Pointer to VMA419 display structure
 * @param x Starting X coordinate
 * @param y Starting Y coordinate
 * @param str Null-terminated string to draw
 */
static inline void vma419_font_draw_string(VMA419_Display* disp, int16_t x, int16_t y, const char* str) {
    if (!disp || !str) return;
    
    int16_t cursor_x = x;
    
    while (*str && cursor_x < 32) {
        uint8_t char_width = vma419_font_draw_char(disp, cursor_x, y, *str);
        cursor_x += char_width + 1; // Add 1 pixel spacing between characters
        str++;
    }
}

/**
 * Draw a single digit number (0-9)
 * @param disp Pointer to VMA419 display structure
 * @param x X coordinate
 * @param y Y coordinate
 * @param digit Single digit (0-9)
 */
static inline void vma419_font_draw_digit(VMA419_Display* disp, int16_t x, int16_t y, uint8_t digit) {
    if (digit <= 9) {
        vma419_font_draw_char(disp, x, y, '0' + digit);
    }
}

/**
 * Draw a two-digit number (00-99)
 * @param disp Pointer to VMA419 display structure
 * @param x X coordinate
 * @param y Y coordinate
 * @param number Two-digit number (0-99)
 */
static inline void vma419_font_draw_number_2d(VMA419_Display* disp, int16_t x, int16_t y, uint8_t number) {
    if (number > 99) number = 99;
    
    vma419_font_draw_digit(disp, x, y, number / 10);      // Tens digit
    vma419_font_draw_digit(disp, x + 6, y, number % 10);  // Ones digit
}

/**
 * Draw text centered on the display
 * @param disp Pointer to VMA419 display structure
 * @param y Y coordinate (row)
 * @param str String to center
 */
static inline void vma419_font_draw_string_centered(VMA419_Display* disp, int16_t y, const char* str) {
    if (!str) return;
    
    // Calculate string width
    uint8_t len = 0;
    const char* p = str;
    while (*p++) len++;
    
    // Calculate total pixel width: (chars * 5) + (spaces * 1)
    uint8_t total_width = (len * VMA419_FONT_WIDTH) + (len - 1);
    
    // Center the string
    int16_t start_x = (32 - total_width) / 2;
    if (start_x < 0) start_x = 0;
    
    vma419_font_draw_string(disp, start_x, y, str);
}

#endif // VMA419_FONT_H
