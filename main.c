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
#include <string.h>
#include <avr/interrupt.h>
#include "vma419.h"
#include "VMA419_Font.h"

// UART Configuration for Internal 8MHz Oscillator
#define F_CPU 8000000UL   // 8MHz internal oscillator
#define BAUD 9600         // Baud rate
#define MYUBRR ((F_CPU / (16UL * BAUD)) - 1) // Calculate UART baud rate = 51

// Alternative baud rate values for different configurations:
// For 8MHz internal oscillator: MYUBRR = 51 (current setting)
// For 16MHz external crystal: MYUBRR = 103
// For 1MHz internal oscillator: MYUBRR = 6

// --- Global Display Structures ---
VMA419_Display dmd_display;

// --- Scrolling Text Variables ---
char scroll_text[64] = "HELLO WORLD ";  // Text to scroll (increased buffer size)
int16_t scroll_position = 32;  // Start position (off-screen right)
uint8_t scroll_speed = 30;    // Delay in milliseconds between scroll steps

// --- Button Control Variables ---
uint8_t scroll_direction = 0;  // 0 = right to left, 1 = left to right
int16_t text_y_position = 4;   // Vertical position of text (default middle)
uint8_t brightness_level = 3;  // Brightness level (0-7, higher = brighter)
uint8_t button_debounce = 0;   // Simple debounce counter

// Speed and position limits
#define SPEED_MIN 10
#define SPEED_MAX 100
#define Y_POS_MIN 0
#define Y_POS_MAX 9  // Keep text within display bounds (16 pixels - 7 font height)
#define BRIGHTNESS_MIN 0
#define BRIGHTNESS_MAX 7

// --- UART Circular Buffer Variables ---
#define UART_BUFFER_SIZE 128
volatile char uart_rx_buffer[UART_BUFFER_SIZE];
volatile uint8_t uart_rx_head = 0;
volatile uint8_t uart_rx_tail = 0;
volatile uint8_t uart_message_ready = 0;
char uart_message[64];  // Final message buffer

// UART Communication Functions
void USART_Init(unsigned int ubrr) {
    // Set baud rate registers (both high and low bytes)
    UBRRH = (unsigned char)(ubrr >> 8);
    UBRRL = (unsigned char)ubrr; 
    
    // Enable USART receive, transmit, and receive interrupt
    UCSRB = (1 << RXEN) | (1 << TXEN) | (1 << RXCIE); 
    
    // Set frame format: 8-bit, no parity, 1 stop bit
    UCSRC = (1 << URSEL) | (1 << UCSZ1) | (1 << UCSZ0);
    
    // Enable global interrupts
    sei();
}

void USART_Transmit(char data) {
    while (!(UCSRA & (1 << UDRE))); // Wait until transmit buffer is empty
    UDR = data; // Load character to UART data register
}

void USART_SendString(const char* str) {
    while (*str) { // Transmit until null terminator
        USART_Transmit(*str++); // Transmit character and move to next
    }
}

char USART_Receive(void) {
    while (!(UCSRA & (1 << RXC))); // Wait until character is received
    return UDR; // Return the received character
}

uint8_t USART_DataAvailable(void) {
    return (UCSRA & (1 << RXC)); // Return 1 if data is available
}

// UART Interrupt Service Routine
ISR(USART_RXC_vect) {
    char received_char = UDR;
    
    // Echo the character back to terminal
    USART_Transmit(received_char);
    
    // Handle different characters
    if (received_char == '\r' || received_char == '\n') {
        // End of message - process the received data
        if (uart_rx_head != uart_rx_tail) {
            // Copy buffer content to message
            uint8_t msg_index = 0;
            uint8_t temp_tail = uart_rx_tail;
            
            while (temp_tail != uart_rx_head && msg_index < sizeof(uart_message) - 1) {
                uart_message[msg_index] = uart_rx_buffer[temp_tail];
                temp_tail = (temp_tail + 1) % UART_BUFFER_SIZE;
                msg_index++;
            }
            
            uart_message[msg_index] = '\0'; // Null terminate
            uart_message_ready = 1;
            
            // Reset buffer pointers
            uart_rx_head = 0;
            uart_rx_tail = 0;
            
            // Send newline
            USART_Transmit('\r');
            USART_Transmit('\n');
        }
    }
    else if (received_char == 8 || received_char == 127) {
        // Backspace or Delete - remove last character
        if (uart_rx_head != uart_rx_tail) {
            uart_rx_head = (uart_rx_head - 1 + UART_BUFFER_SIZE) % UART_BUFFER_SIZE;
            // Send backspace sequence to terminal
            USART_Transmit('\b');
            USART_Transmit(' ');
            USART_Transmit('\b');
        }
    }
    else if (received_char >= 32 && received_char <= 126) {
        // Printable character - add to buffer
        uint8_t next_head = (uart_rx_head + 1) % UART_BUFFER_SIZE;
        
        // Check if buffer is not full
        if (next_head != uart_rx_tail) {
            uart_rx_buffer[uart_rx_head] = received_char;
            uart_rx_head = next_head;
        }
    }
    // Ignore other control characters
}

// Check if a complete message has been received
uint8_t uart_message_available(void) {
    return uart_message_ready;
}

// Get the received message and clear the flag
void uart_get_message(char* buffer, uint8_t max_length) {
    if (uart_message_ready) {
        strncpy(buffer, uart_message, max_length - 1);
        buffer[max_length - 1] = '\0';
        uart_message_ready = 0;
    }
}

// Button Control Functions
void init_buttons(void) {
    // Configure PC0-PC7 as inputs with pull-up resistors
    DDRC = 0x00;   // All PORTC pins as inputs
    PORTC = 0xFF;  // Enable pull-up resistors on all pins
}

void check_buttons(void) {
    // Simple debouncing: only check buttons every few cycles
    button_debounce++;
    if (button_debounce < 10) return;
    button_debounce = 0;
    
    uint8_t button_state = ~PINC;  // Invert because buttons pull to ground
    
    // PC0 - Increase Speed
    if (button_state & (1 << PC0)) {
        if (scroll_speed > SPEED_MIN) {
            scroll_speed -= 5;  // Decrease delay = increase speed
            USART_SendString("Speed increased\r\n> ");
        }
    }
    
    // PC1 - Decrease Speed
    if (button_state & (1 << PC1)) {
        if (scroll_speed < SPEED_MAX) {
            scroll_speed += 5;  // Increase delay = decrease speed
            USART_SendString("Speed decreased\r\n> ");
        }
    }
    
    // PC2 - Direction: Left to Right
    if (button_state & (1 << PC2)) {
        scroll_direction = 1;
        // Reset position for new direction
        scroll_position = -((int16_t)strlen(scroll_text) * 6);
        USART_SendString("Direction: Left to Right\r\n> ");
    }
    
    // PC3 - Direction: Right to Left
    if (button_state & (1 << PC3)) {
        scroll_direction = 0;
        // Reset position for new direction
        scroll_position = 32;
        USART_SendString("Direction: Right to Left\r\n> ");
    }
    
    // PC4 - Move text upwards
    if (button_state & (1 << PC4)) {
        if (text_y_position > Y_POS_MIN) {
            text_y_position--;
            USART_SendString("Text moved up\r\n> ");
        }
    }
    
    // PC5 - Move text downwards
    if (button_state & (1 << PC5)) {
        if (text_y_position < Y_POS_MAX) {
            text_y_position++;
            USART_SendString("Text moved down\r\n> ");
        }
    }
    
    // PC6 - Brightness Down
    if (button_state & (1 << PC6)) {
        if (brightness_level > BRIGHTNESS_MIN) {
            brightness_level--;
            USART_SendString("Brightness decreased\r\n> ");
        }
    }
    
    // PC7 - Brightness Up
    if (button_state & (1 << PC7)) {
        if (brightness_level < BRIGHTNESS_MAX) {
            brightness_level++;
            USART_SendString("Brightness increased\r\n> ");
        }
    }
}

void updateDisplayMessage(const char* message) {
    // Clear the scroll text and copy new message
    memset(scroll_text, 0, sizeof(scroll_text));
    
    // Copy the full message, leaving space for trailing space and null terminator
    uint8_t msg_len = strlen(message);
    uint8_t max_copy_len = sizeof(scroll_text) - 2; // Reserve 2 bytes for space + null
    
    if (msg_len > max_copy_len) {
        msg_len = max_copy_len; // Truncate if too long
    }
    
    // Copy the message
    memcpy(scroll_text, message, msg_len);
    
    // Add trailing space for smooth scrolling
    scroll_text[msg_len] = ' ';
    scroll_text[msg_len + 1] = '\0';
    
    // Reset scroll position based on direction
    if (scroll_direction == 0) {
        // Right to left: start from right edge
        scroll_position = 32;
    } else {
        // Left to right: start from left edge (negative position)
        int16_t text_width = strlen(scroll_text) * 6;
        scroll_position = -text_width;
    }
    
    // Send confirmation via UART
    USART_SendString("Message updated: ");
    USART_SendString(scroll_text);
    USART_SendString("\r\n> ");
}

// Pin configuration for VMA419 display
// Hardware SPI configuration for ATmega16
VMA419_PinConfig dmd_pins = {
    // Hardware SPI pins (ATmega16 SPI peripheral)
    .spi_clk_port_ddr  = &DDRB, .spi_clk_port_out  = &PORTB, .spi_clk_pin_mask  = (1 << PB7), // SCK -> PB7 (Hardware SPI Clock)
    .spi_data_port_ddr = &DDRB, .spi_data_port_out = &PORTB, .spi_data_pin_mask = (1 << PB5), // MOSI -> PB5 (Hardware SPI Data)

    // Row Select Pins (2-bit binary selection for 4 row phases)
    .a_port_ddr        = &DDRA, .a_port_out        = &PORTA, .a_pin_mask        = (1 << PA1), // A -> PA1
    .b_port_ddr        = &DDRA, .b_port_out        = &PORTA, .b_pin_mask        = (1 << PA2), // B -> PA2

    // Latch/Strobe Clock Pin (transfers shift register data to output latches)
    .latch_clk_port_ddr= &DDRA, .latch_clk_port_out= &PORTA, .latch_clk_pin_mask= (1 << PA4), // SCLK -> PA4

    // Output Enable Pin (controls display brightness/visibility)
    .oe_port_ddr       = &DDRD, .oe_port_out       = &PORTD, .oe_pin_mask       = (1 << PD7)  // OE -> PD7
};

int main(void) {
    // Add startup delay for system stabilization
    _delay_ms(100);
    
    // Initialize UART communication
    USART_Init(MYUBRR);
    
    // Wait for UART to stabilize
    _delay_ms(100);
    
    // Send simple test character first
    USART_Transmit('A');
    _delay_ms(10);
    USART_Transmit('\r');
    USART_Transmit('\n');
      // Send welcome message
    USART_SendString("VMA419 LED Display - UART Control Ready!\r\n");
    USART_SendString("Type your message and press Enter to display on LED matrix\r\n");
    USART_SendString("\r\nButton Controls (PC0-PC7):\r\n");
    USART_SendString("PC0: Speed Up    | PC1: Speed Down\r\n");
    USART_SendString("PC2: Left->Right | PC3: Right->Left\r\n");
    USART_SendString("PC4: Move Up     | PC5: Move Down\r\n");
    USART_SendString("PC6: Dim         | PC7: Brighten\r\n");
    USART_SendString("> ");
    
    // Initialize the VMA419 display driver
    // Parameters: display structure, pin config, panels_wide=1, panels_high=1
    if (vma419_init(&dmd_display, &dmd_pins, 1, 1) != 0) {
        // Initialization failed - halt execution
        USART_SendString("ERROR: Display initialization failed!\r\n");
        while(1);
    }
    
    // Clear the display buffer to ensure all LEDs start OFF
    vma419_clear(&dmd_display);
      // Initialize font system
    vma419_font_init(&dmd_display);
    
    // Initialize button system
    init_buttons();
    
    // Variables for UART message processing
    char new_message[64]; // Buffer for complete messages
    uint16_t refresh_counter = 0;
      // Main display refresh loop with UART message handling
    // The VMA419 uses 4-phase multiplexing, so we cycle through all 4 phases
    while(1) {
        // Check for new UART message
        if (uart_message_available()) {
            uart_get_message(new_message, sizeof(new_message));
            updateDisplayMessage(new_message); // Update LED display
        }
        
        // Check button inputs
        check_buttons();
        
        // Clear display buffer
        vma419_clear(&dmd_display);
        
        // Draw scrolling text at the specified vertical position
        vma419_font_draw_string(&dmd_display, scroll_position, text_y_position, scroll_text);
          // Refresh display (4-phase multiplexing with brightness control)
        for(uint8_t cycle = 0; cycle < 4; cycle++) {
            dmd_display.scan_cycle = cycle;
            vma419_scan_display_quarter(&dmd_display);
            
            // Brightness control: vary the display ON time
            // Higher brightness_level = longer ON time
            uint8_t on_time = brightness_level + 1; // 1-8 range
            for(uint8_t i = 0; i < on_time; i++) {
                _delay_us(125); // Base delay unit (125μs × 8 = 1ms max)
            }
        }
          // Update scroll position every few refresh cycles
        refresh_counter++;
        if(refresh_counter >= scroll_speed) {
            refresh_counter = 0;
            
            // Calculate text width: each character is 6 pixels wide (5 + 1 spacing)
            int16_t text_width = strlen(scroll_text) * 6;
            
            if (scroll_direction == 0) {
                // Right to left scrolling
                scroll_position--;
                // Reset position when text has scrolled completely off screen
                if(scroll_position < -text_width) {
                    scroll_position = 32; // Start from right edge again
                }
            } else {
                // Left to right scrolling
                scroll_position++;
                // Reset position when text has scrolled completely off screen
                if(scroll_position > 32) {
                    scroll_position = -text_width; // Start from left edge again
                }
            }
        }
    }
    
    return 0; // Should never be reached
}