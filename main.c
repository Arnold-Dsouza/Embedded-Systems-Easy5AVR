/*
 * VMA419 LED Matrix Display Driver - Main Application
 * 
 * This program demonstrates the VMA419 32x16 LED matrix display driver.
 * It initializes a single VMA419 panel and displays a test LED at position (31,15).
 * 
 * Test Pattern: Single LED at bottom-right Corner (31,15)
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

// UART Configuration for Internal 8MHz Oscillator
#define F_CPU 8000000UL   // 8MHz internal oscillator

#include <avr/io.h>
#include <util/delay.h>
#include <string.h>
#include <stdio.h>  // For sprintf function
#include <avr/interrupt.h>
#include "vma419.h"
#include "VMA419_Font.h"
#include "fesb_logo.h"
#define BAUD 9600         // Baud rate
#define MYUBRR ((F_CPU / (16UL * BAUD)) - 1) // Calculate UART baud rate = 51

// Alternative baud rate values for different configurations:
// For 8MHz internal oscillator: MYUBRR = 51 (current setting)
// For 16MHz external crystal: MYUBRR = 103
// For 1MHz internal oscillator: MYUBRR = 6

// --- Global Display Structures ---
VMA419_Display dmd_display;

// --- Scrolling Text Variables ---
char scroll_text[32] = "WELCOME ERASMUS STUDENTS";  // Reduced from 64 to save RAM
int16_t scroll_position = 32;  // Start position (off-screen right)
uint8_t scroll_speed = 30;    // Delay in milliseconds between scroll steps
int8_t scroll_direction = -1; // Direction: -1 = right to left (default), 1 = left to right
int8_t text_y_offset = 4;     // Vertical position (0-15 for 16-pixel height display)

// --- UART Circular Buffer Variables ---
#define UART_BUFFER_SIZE 64  // Reduced from 128 to save RAM
volatile char uart_rx_buffer[UART_BUFFER_SIZE];
volatile uint8_t uart_rx_head = 0;
volatile uint8_t uart_rx_tail = 0;
volatile uint8_t uart_message_ready = 0;
char uart_message[32];  // Reduced from 64 to save RAM

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
    while (!(UCSRA & (1 << RXC))); 
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
    
    // Reset scroll position to start from right
    scroll_position = 32;
    
    // Send confirmation via UART
    USART_SendString("Updated: ");
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
    USART_Init(MYUBRR);    // Initialize PC0 and PC1 as inputs with pull-up resistors
    // PC0: Speed Up button, PC1: Speed Down button
    // PC2: Toggle Direction button
    // PC6: Text Up button, PC7: Text Down button
    DDRC &= ~((1 << PC0) | (1 << PC1) | (1 << PC2) | (1 << PC6) | (1 << PC7));  // Set as inputs
    PORTC |= (1 << PC0) | (1 << PC1) | (1 << PC2) | (1 << PC6) | (1 << PC7);    // Enable pull-up resistors
    
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
      // Display current speed and direction information
    USART_SendString("Speed: ");
    // Simple digit conversion without sprintf to save memory
    if (scroll_speed >= 100) {
        USART_Transmit('0' + (scroll_speed / 100));
        USART_Transmit('0' + ((scroll_speed % 100) / 10));
        USART_Transmit('0' + (scroll_speed % 10));
    } else if (scroll_speed >= 10) {
        USART_Transmit('0' + (scroll_speed / 10));
        USART_Transmit('0' + (scroll_speed % 10));
    } else {
        USART_Transmit('0' + scroll_speed);
    }
    USART_SendString("\r\nDirection: ");
    USART_SendString((scroll_direction < 0) ? "R>L" : "L>R");
    USART_SendString("\r\n");
    
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
    
    // ========================================================================
    // DISPLAY FESB LOGO FOR 10 SECONDS
    // ========================================================================
    USART_SendString("Displaying FESB Logo for 10 seconds...\r\n");
    fesb_logo_show_for_duration(&dmd_display, 10);    USART_SendString("FESB Logo display complete. Starting scrolling text...\r\n");
    USART_SendString("> ");
    
    // Clear display after logo and prepare for scrolling text
    vma419_clear(&dmd_display);
    
    // Add stabilization delay after logo display
    _delay_ms(200);    // Re-initialize button pins to ensure proper state after logo display
    // (Logo display might have affected pin states)
    DDRC &= ~((1 << PC0) | (1 << PC1) | (1 << PC2) | (1 << PC6) | (1 << PC7));  // Set as inputs
    PORTC |= (1 << PC0) | (1 << PC1) | (1 << PC2) | (1 << PC6) | (1 << PC7);    // Enable pull-up resistors
    
    // Wait for button pins to stabilize
    _delay_ms(50);
      // Variables for UART message processing
    char new_message[32]; // Reduced buffer size
    uint16_t refresh_counter = 0;
    uint16_t button_status_counter = 0; // Counter for periodic button status report    // Initialize button states properly after FESB logo display
    // Read current button states to initialize previous states
    uint8_t button_pc0_prev = (PINC & (1 << PC0)) >> PC0; // Previous state of PC0 (Speed Up)
    uint8_t button_pc1_prev = (PINC & (1 << PC1)) >> PC1; // Previous state of PC1 (Speed Down)
    uint8_t button_pc2_prev = (PINC & (1 << PC2)) >> PC2; // Previous state of PC2 (Toggle Direction)
    uint8_t button_pc6_prev = (PINC & (1 << PC6)) >> PC6; // Previous state of PC6 (Text Up)
    uint8_t button_pc7_prev = (PINC & (1 << PC7)) >> PC7; // Previous state of PC7 (Text Down)
    uint8_t button_debounce_timer = 0; // Timer for button debounce    // Debug: Send initial button states via UART (simplified)
    USART_SendString("Buttons: ");
    USART_Transmit('0' + button_pc0_prev);
    USART_Transmit('0' + button_pc1_prev);
    USART_Transmit('0' + button_pc2_prev);
    USART_Transmit('0' + button_pc6_prev);
    USART_Transmit('0' + button_pc7_prev);
    USART_SendString("\r\n");
    
    // Send button status message
    USART_SendString("Controls: PC0=Speed+, PC1=Speed-, PC2=ToggleDir, PC6=Up, PC7=Down\r\n");
    USART_SendString("> ");
    
    // Main display refresh loop with UART message handling
    // The VMA419 uses 4-phase multiplexing, so we cycle through all 4 phases
    while(1) {
        // Check for new UART message
        if (uart_message_available()) {
            uart_get_message(new_message, sizeof(new_message));
            updateDisplayMessage(new_message); // Update LED display
        }        // Process button inputs with proper debouncing
        // Only check buttons when not in debounce period
        if (button_debounce_timer == 0) {
            // Read current button states (0 = pressed, 1 = released due to pull-up)
            uint8_t button_pc0_current = (PINC & (1 << PC0)) >> PC0;
            uint8_t button_pc1_current = (PINC & (1 << PC1)) >> PC1;
            uint8_t button_pc2_current = (PINC & (1 << PC2)) >> PC2;
            uint8_t button_pc6_current = (PINC & (1 << PC6)) >> PC6;
            uint8_t button_pc7_current = (PINC & (1 << PC7)) >> PC7;

            // Debug: Detect any button state changes (simplified)
            if (button_pc0_current != button_pc0_prev || button_pc1_current != button_pc1_prev ||
                button_pc2_current != button_pc2_prev ||
                button_pc6_current != button_pc6_prev || button_pc7_current != button_pc7_prev) {
                USART_SendString("Btn: ");
                USART_Transmit('0' + button_pc0_current);
                USART_Transmit('0' + button_pc1_current);
                USART_Transmit('0' + button_pc2_current);
                USART_Transmit('0' + button_pc6_current);
                USART_Transmit('0' + button_pc7_current);
                USART_SendString("\r\n");
            }
              // PC0: Speed Up button (decrease delay value)
            if (button_pc0_prev == 1 && button_pc0_current == 0) {
                // Button just pressed - increase speed (lower delay)
                if (scroll_speed > 5) {
                    scroll_speed -= 5;
                    USART_SendString("Speed+: ");
                    USART_Transmit('0' + (scroll_speed / 10));
                    USART_Transmit('0' + (scroll_speed % 10));
                    USART_SendString("\r\n> ");
                } else {
                    USART_SendString("Speed MAX\r\n> ");
                }
                button_debounce_timer = 50; // Set debounce timer (50 * 4ms = 200ms)
            }
            
            // PC1: Speed Down button (increase delay value)
            if (button_pc1_prev == 1 && button_pc1_current == 0) {
                // Button just pressed - decrease speed (higher delay)
                if (scroll_speed < 100) {
                    scroll_speed += 5;
                    USART_SendString("Speed-: ");
                    USART_Transmit('0' + (scroll_speed / 10));
                    USART_Transmit('0' + (scroll_speed % 10));
                    USART_SendString("\r\n> ");
                } else {
                    USART_SendString("Speed MIN\r\n> ");
                }
                button_debounce_timer = 50; // Set debounce timer
            }            // PC2: Toggle Direction button
            if (button_pc2_prev == 1 && button_pc2_current == 0) {
                // Button just pressed - toggle direction
                scroll_direction = -scroll_direction; // Toggle between 1 and -1
                
                // Reset scroll position for new direction
                if (scroll_direction < 0) {
                    // Right to left
                    scroll_position = 32;
                    USART_SendString("Dir: R>L\r\n> ");
                } else {
                    // Left to right
                    scroll_position = -strlen(scroll_text) * 6;
                    USART_SendString("Dir: L>R\r\n> ");
                }
                button_debounce_timer = 50; // Set debounce timer
            }
            
            // PC6: Text Up button (decrease Y offset)
            if (button_pc6_prev == 1 && button_pc6_current == 0) {
                // Button just pressed - move text up
                if (text_y_offset > 0) {
                    text_y_offset--;
                    USART_SendString("Text Up: Y=");
                    USART_Transmit('0' + (text_y_offset / 10));
                    USART_Transmit('0' + (text_y_offset % 10));
                    USART_SendString("\r\n> ");
                } else {
                    USART_SendString("Text at TOP\r\n> ");
                }
                button_debounce_timer = 50; // Set debounce timer
            }
            
            // PC7: Text Down button (increase Y offset)
            if (button_pc7_prev == 1 && button_pc7_current == 0) {
                // Button just pressed - move text down
                if (text_y_offset < 15) {
                    text_y_offset++;
                    USART_SendString("Text Down: Y=");
                    USART_Transmit('0' + (text_y_offset / 10));
                    USART_Transmit('0' + (text_y_offset % 10));
                    USART_SendString("\r\n> ");
                } else {
                    USART_SendString("Text at BOTTOM\r\n> ");
                }
                button_debounce_timer = 50; // Set debounce timer
            }            // Update previous button states
            button_pc0_prev = button_pc0_current;
            button_pc1_prev = button_pc1_current;
            button_pc2_prev = button_pc2_current;
            button_pc6_prev = button_pc6_current;
            button_pc7_prev = button_pc7_current;} else {
            // Decrement debounce timer
            button_debounce_timer--;
        }        // Periodic button status report (every 1000 refresh cycles ~ 4 seconds)
        button_status_counter++;
        if (button_status_counter >= 1000) {
            button_status_counter = 0;
            // Read current button states for status report
            uint8_t pc0_state = (PINC & (1 << PC0)) >> PC0;
            uint8_t pc1_state = (PINC & (1 << PC1)) >> PC1;
            uint8_t pc2_state = (PINC & (1 << PC2)) >> PC2;
            uint8_t pc6_state = (PINC & (1 << PC6)) >> PC6;
            uint8_t pc7_state = (PINC & (1 << PC7)) >> PC7;
            
            USART_SendString("Status: ");
            USART_Transmit('0' + pc0_state);
            USART_Transmit('0' + pc1_state);
            USART_Transmit('0' + pc2_state);
            USART_Transmit('0' + pc6_state);
            USART_Transmit('0' + pc7_state);
            USART_SendString("\r\n> ");
        }// Clear display buffer
        vma419_clear(&dmd_display);
        
        // Draw scrolling text with variable Y offset
        vma419_font_draw_string(&dmd_display, scroll_position, text_y_offset, scroll_text);
        
        // Refresh display (4-phase multiplexing)
        for(uint8_t cycle = 0; cycle < 4; cycle++) {
            dmd_display.scan_cycle = cycle;
            vma419_scan_display_quarter(&dmd_display);
            _delay_ms(1); // 1ms delay per phase = 250Hz refresh rate
        }          // Update scroll position every few refresh cycles
        refresh_counter++;
        if(refresh_counter >= scroll_speed) {
            refresh_counter = 0;
            
            // Update position based on direction
            scroll_position += scroll_direction;
            
            // Calculate text width: each character is 6 pixels wide (5 + 1 spacing)
            int16_t text_width = strlen(scroll_text) * 6;
            
            // Handle position wrapping based on direction
            if (scroll_direction < 0) {
                // Right to left (default) - reset when text has scrolled off left side
                if(scroll_position < -text_width) {
                    scroll_position = 32; // Start from right edge again
                }
            } else {
                // Left to right - reset when text has scrolled off right side
                if(scroll_position > 32) {
                    scroll_position = -text_width; // Start from left edge
                }
            }
        }
    }

    return 0;
}