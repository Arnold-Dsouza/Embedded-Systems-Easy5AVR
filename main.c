/*
 * ===================================================================
 * LED Matrix Display Controller - Main Program
 * ===================================================================
 * 
 * What this program does:
 * - Controls a 32x16 LED matrix display to show scrolling text
 * - Shows the FESB university logo on startup
 * - Accepts messages from computer via UART (serial connection)
 * - Responds to button presses to control text speed and position
 * 
 * How it works:
 * 1. Shows FESB logo for 10 seconds when powered on
 * 2. Displays "WELCOME ERASMUS STUDENTS" scrolling text
 * 3. You can type new messages via serial terminal
 * 4. Physical buttons control speed and text position
 * 
 * Hardware needed:
 * - VMA419 32x16 red LED matrix panel
 * - 5 push buttons connected to pins PC0, PC1, PC2, PC6, PC7
 * - USB/serial connection to computer
 * 
 * Created by: Arnold Dsouza
 * Date: November 6, 2025
 * Status: Working perfectly! ✓
 */

#define F_CPU 8000000UL   

#include <avr/io.h>        // Basic input/output functions for AVR chips
#include <util/delay.h>    // Functions to create time delays
#include <string.h>        // Text manipulation functions (strlen, strcpy, etc.)
#include <avr/interrupt.h> // Functions to handle interrupts
#include "vma419.h"        // Our custom LED matrix driver
#include "VMA419_Font.h"   // Font data for displaying text
#include "fesb_logo.h"     // University logo bitmap data

#define BAUD 9600         // Communication speed: 9600 bits per second
#define MYUBRR ((F_CPU / (16UL * BAUD)) - 1) // Math to calculate baud rate = 51

// ===============================================
// MAIN VARIABLES - THE IMPORTANT STUFF
// ===============================================
VMA419_Display dmd_display;  // This controls our LED matrix

// Settings for the scrolling text
char scroll_text[32] = "WELCOME ERASMUS STUDENTS";  // The message to display
int16_t scroll_position = 32;    // Where the text starts (off the right side)
uint8_t scroll_speed = 30;       // How fast it scrolls (lower = faster)
int8_t scroll_direction = -1;    // Which way: -1 = right to left, 1 = left to right
int8_t text_y_offset = 4;        // How high up the text appears (0 = top, 15 = bottom)
// ===============================================
// SERIAL COMMUNICATION BUFFER
// ===============================================
// This stores messages typed from the computer until we're ready to process them
#define UART_BUFFER_SIZE 64  // Can store up to 64 characters at once
volatile char uart_rx_buffer[UART_BUFFER_SIZE];  // Where incoming characters go
volatile uint8_t uart_rx_head = 0;     // Points to where next character goes
volatile uint8_t uart_rx_tail = 0;     // Points to first unread character  
volatile uint8_t uart_message_ready = 0;  // Flag: 1 = complete message ready, 0 = still typing
char uart_message[32];  // Holds the final complete message

// ===============================================
// FUNCTIONS TO TALK TO THE COMPUTER
// ===============================================

// Set up the serial communication system
void USART_Init(unsigned int ubrr) {
    // Configure the baud rate (how fast we communicate)
    UBRRH = (unsigned char)(ubrr >> 8);  // High byte of baud rate
    UBRRL = (unsigned char)ubrr;         // Low byte of baud rate
    
    // Turn on the serial communication features we need
    UCSRB = (1 << RXEN) | (1 << TXEN) | (1 << RXCIE); // Enable receive, transmit, and receive interrupt
    
    // Set the data format: 8 data bits, no parity bit, 1 stop bit
    UCSRC = (1 << URSEL) | (1 << UCSZ1) | (1 << UCSZ0);
    
    // Enable interrupts so we can automatically handle incoming messages
    sei();
}

// Send a single character to the computer
void USART_Transmit(char data) {
    while (!(UCSRA & (1 << UDRE))); // Wait until the transmit buffer is ready
    UDR = data; // Put the character in the transmit register
}

// Send a whole text message to the computer
void USART_SendString(const char* str) {
    while (*str) { // Keep going until we hit the end of the text (null character)
        USART_Transmit(*str++); // Send this character and move to the next
    }
}

// ===============================================
// AUTOMATIC MESSAGE HANDLER (INTERRUPT FUNCTION)
// ===============================================
// This function runs automatically whenever a character arrives from the computer
// It's like having a secretary that collects your mail while you're busy with other work
ISR(USART_RXC_vect) {
    char received_char = UDR;  // Get the character that just arrived
    
    // Echo it back to the computer so the user can see what they typed
    USART_Transmit(received_char);
    
    // Now decide what to do with this character
    if (received_char == '\r' || received_char == '\n') {
        // User pressed Enter - they're done typing their message
        if (uart_rx_head != uart_rx_tail) {  // Make sure there's actually something in the buffer
            // Copy everything from our temporary buffer into the final message
            uint8_t msg_index = 0;
            uint8_t temp_tail = uart_rx_tail;
            
            // Copy character by character until we reach the end
            while (temp_tail != uart_rx_head && msg_index < sizeof(uart_message) - 1) {
                uart_message[msg_index] = uart_rx_buffer[temp_tail];
                temp_tail = (temp_tail + 1) % UART_BUFFER_SIZE;  // Move to next position (wrapping around if needed)
                msg_index++;
            }
            
            uart_message[msg_index] = '\0'; // Add null terminator to mark end of string
            uart_message_ready = 1;         // Signal that a complete message is ready
            
            // Clear the buffer for the next message
            uart_rx_head = 0;
            uart_rx_tail = 0;
            
            // Send a new line to the computer terminal
            USART_Transmit('\r');
            USART_Transmit('\n');
        }
    }
    else if (received_char == 8 || received_char == 127) {
        // User pressed Backspace or Delete - remove the last character they typed
        if (uart_rx_head != uart_rx_tail) {  // Only if there's something to delete
            uart_rx_head = (uart_rx_head - 1 + UART_BUFFER_SIZE) % UART_BUFFER_SIZE;
            // Send backspace sequence to the computer so it erases on screen too
            USART_Transmit('\b');  // Move cursor back
            USART_Transmit(' ');   // Overwrite with space
            USART_Transmit('\b');  // Move cursor back again
        }
    }
    else if (received_char >= 32 && received_char <= 126) {
        // It's a normal printable character (letters, numbers, symbols)
        uint8_t next_head = (uart_rx_head + 1) % UART_BUFFER_SIZE;
        
        // Make sure our buffer isn't full
        if (next_head != uart_rx_tail) {
            uart_rx_buffer[uart_rx_head] = received_char;  // Store the character
            uart_rx_head = next_head;                      // Move to next position
        }
        // If buffer is full, we just ignore the character (could add error handling here)
    }
    // Ignore any other control characters (like Ctrl+C, weird escape sequences, etc.)
}

// ===============================================
// HELPER FUNCTIONS FOR MESSAGE HANDLING
// ===============================================

// Check if someone has finished typing a complete message
uint8_t uart_message_available(void) {
    return uart_message_ready;  // Returns 1 if message is ready, 0 if not
}

// Get the complete message and mark it as read
void uart_get_message(char* buffer, uint8_t max_length) {
    if (uart_message_ready) {
        strncpy(buffer, uart_message, max_length - 1);  // Copy the message safely
        buffer[max_length - 1] = '\0';                  // Make sure it ends properly
        uart_message_ready = 0;                         // Mark as read
    }
}

// Update what's shown on the LED display with a new message
void updateDisplayMessage(const char* message) {
    // Clear out the old message completely
    memset(scroll_text, 0, sizeof(scroll_text));
    
    // Figure out how much of the new message we can fit
    uint8_t msg_len = strlen(message);
    uint8_t max_copy_len = sizeof(scroll_text) - 2; // Leave room for space and null terminator
    
    if (msg_len > max_copy_len) {
        msg_len = max_copy_len; // Truncate if the message is too long
    }
    
    // Copy the new message
    memcpy(scroll_text, message, msg_len);
    
    // Add a space at the end for smooth scrolling (so text doesn't run together)
    scroll_text[msg_len] = ' ';
    scroll_text[msg_len + 1] = '\0';
    
    // Start scrolling from the right side again
    scroll_position = 32;
    
    // Let the user know we got their message
    USART_SendString("Updated: ");
    USART_SendString(scroll_text);
    USART_SendString("\r\n> ");
}

// ===============================================
// HARDWARE WIRING CONFIGURATION
// ===============================================
// This tells the program which pins on the microcontroller connect to which wires on the LED display
// Think of this like a wiring diagram in code form

VMA419_PinConfig dmd_pins = {
    // SPI Communication Pins (these send data to the display)
    .spi_clk_port_ddr  = &DDRB, .spi_clk_port_out  = &PORTB, .spi_clk_pin_mask  = (1 << PB7), // Clock signal (PB7)
    .spi_data_port_ddr = &DDRB, .spi_data_port_out = &PORTB, .spi_data_pin_mask = (1 << PB5), // Data signal (PB5)

    // Row Selection Pins (these choose which rows of LEDs to light up)
    .a_port_ddr        = &DDRA, .a_port_out        = &PORTA, .a_pin_mask        = (1 << PA1), // Row select A (PA1)
    .b_port_ddr        = &DDRA, .b_port_out        = &PORTA, .b_pin_mask        = (1 << PA2), // Row select B (PA2)

    // Control Pins
    .latch_clk_port_ddr= &DDRA, .latch_clk_port_out= &PORTA, .latch_clk_pin_mask= (1 << PA4), // Latch data to display (PA4)
    .oe_port_ddr       = &DDRD, .oe_port_out       = &PORTD, .oe_pin_mask       = (1 << PD7)  // Enable display output (PD7)
};

// ===============================================
// MAIN PROGRAM - THIS IS WHERE EVERYTHING STARTS
// ===============================================
int main(void) {
    // Wait a moment for the electronics to settle down when first powered on
    _delay_ms(100);
    
    // Set up communication with the computer
    USART_Init(MYUBRR);

    // Set up the control buttons (PC0, PC1, PC2, PC6, PC7 as inputs with pull-up resistors)
    // PC0: Speed Up button    | PC1: Speed Down button
    // PC2: Change Direction   | PC6: Move Text Up     | PC7: Move Text Down
    DDRC &= ~((1 << PC0) | (1 << PC1) | (1 << PC2) | (1 << PC6) | (1 << PC7));  // Make them inputs
    PORTC |= (1 << PC0) | (1 << PC1) | (1 << PC2) | (1 << PC6) | (1 << PC7);    // Turn on pull-up resistors
    
    // Give the serial connection time to stabilize
    _delay_ms(100);
    
    // Send a test character to make sure communication is working
    USART_Transmit('A');
    _delay_ms(10);
    USART_Transmit('\r');
    USART_Transmit('\n');
      // Send welcome messages to the computer terminal
    USART_SendString("VMA419 LED Display - UART Control Ready!\r\n");
    USART_SendString("Type your message and press Enter to display on LED matrix\r\n");

    // Tell the user about current settings
    USART_SendString("Speed: ");
    // Convert the speed number to text (doing this manually to save memory)
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
    USART_SendString("\r\n> ");
    
    // Initialize the LED matrix display system
    if (vma419_init(&dmd_display, &dmd_pins, 1, 1) != 0) {
        // If initialization fails, tell the user and stop the program
        USART_SendString("ERROR: Display initialization failed!\r\n");
        while(1);  // Infinite loop - program stops here
    }

    // Start with a blank display
    vma419_clear(&dmd_display);
    
    // Set up the font system for displaying text
    vma419_font_init(&dmd_display);
    
    // ===============================================
    // SHOW UNIVERSITY LOGO ON STARTUP
    // ===============================================
    USART_SendString("Displaying FESB Logo for 10 seconds...\r\n");
    fesb_logo_show_for_duration(&dmd_display, 10);

    USART_SendString("FESB Logo display complete. Starting scrolling text...\r\n");
    USART_SendString("> ");
    
    // Clear the display and get ready for scrolling text
    vma419_clear(&dmd_display);
    
    // Wait a moment for everything to settle
    _delay_ms(200);

    // Make sure the button pins are still set up correctly (logo display might have changed them)
    DDRC &= ~((1 << PC0) | (1 << PC1) | (1 << PC2) | (1 << PC6) | (1 << PC7));  // Inputs
    PORTC |= (1 << PC0) | (1 << PC1) | (1 << PC2) | (1 << PC6) | (1 << PC7);    // Pull-ups enabled
    
    // Give the buttons time to stabilize
    _delay_ms(50);    // ===============================================
    // SET UP VARIABLES FOR THE MAIN LOOP
    // ===============================================
    char new_message[32]; // Buffer to hold new messages from the computer
    uint16_t refresh_counter = 0;        // Counts how many times we've refreshed the display
    uint16_t button_status_counter = 0;  // Counts cycles for periodic status reports

    // Read the current state of all buttons to initialize our tracking variables
    // (Buttons read as 1 when not pressed, 0 when pressed due to pull-up resistors)
    uint8_t button_pc0_prev = (PINC & (1 << PC0)) >> PC0; // Speed Up button state
    uint8_t button_pc1_prev = (PINC & (1 << PC1)) >> PC1; // Speed Down button state
    uint8_t button_pc2_prev = (PINC & (1 << PC2)) >> PC2; // Direction Toggle button state
    uint8_t button_pc6_prev = (PINC & (1 << PC6)) >> PC6; // Text Up button state
    uint8_t button_pc7_prev = (PINC & (1 << PC7)) >> PC7; // Text Down button state
    uint8_t button_debounce_timer = 0; // Prevents button bouncing (false multiple presses)
    
    // Tell the user how to use the buttons
    USART_SendString("Controls: PC0=Speed+, PC1=Speed-, PC2=ToggleDir, PC6=Up, PC7=Down\r\n");
    USART_SendString("> ");
    
    // ===============================================
    // MAIN LOOP - THIS RUNS FOREVER
    // ===============================================
    // This is the heart of the program - it keeps running and handles:
    // 1. New messages from the computer
    // 2. Button presses
    // 3. Updating the LED display with scrolling text
    while(1) {
        // Check if someone sent us a new message via the computer
        if (uart_message_available()) {
            uart_get_message(new_message, sizeof(new_message));
            updateDisplayMessage(new_message); // Update what's shown on the LED display
        }

        // ===============================================
        // HANDLE BUTTON PRESSES
        // ===============================================
        // Only check buttons when we're not in the debounce period
        // (Debouncing prevents false button presses from electrical noise)
        if (button_debounce_timer == 0) {
            // Read the current state of all buttons
            uint8_t button_pc0_current = (PINC & (1 << PC0)) >> PC0;
            uint8_t button_pc1_current = (PINC & (1 << PC1)) >> PC1;
            uint8_t button_pc2_current = (PINC & (1 << PC2)) >> PC2;
            uint8_t button_pc6_current = (PINC & (1 << PC6)) >> PC6;
            uint8_t button_pc7_current = (PINC & (1 << PC7)) >> PC7;

          
            
            if (button_pc0_prev == 1 && button_pc0_current == 0) {
                if (scroll_speed > 5) {
                    scroll_speed -= 5;  // Make it faster
                    USART_SendString("Speed+: ");
                    USART_Transmit('0' + (scroll_speed / 10));
                    USART_Transmit('0' + (scroll_speed % 10));
                    USART_SendString("\r\n> ");
                } else {
                    USART_SendString("Speed MAX\r\n> ");
                }
                button_debounce_timer = 50; // 200ms debounce
            }
            
            // PC1: Speed Down button (makes text scroll slower)
            if (button_pc1_prev == 1 && button_pc1_current == 0) {
                if (scroll_speed < 100) {
                    scroll_speed += 5;  // Make it slower
                    USART_SendString("Speed-: ");
                    USART_Transmit('0' + (scroll_speed / 10));
                    USART_Transmit('0' + (scroll_speed % 10));
                    USART_SendString("\r\n> ");
                } else {
                    USART_SendString("Speed MIN\r\n> ");
                }
                button_debounce_timer = 50;
            }            // PC2: Direction Toggle button
            if (button_pc2_prev == 1 && button_pc2_current == 0) {
                scroll_direction = -scroll_direction; // Toggle between -1 and 1
                
                // Restart scrolling from the appropriate side
                if (scroll_direction < 0) {
                    scroll_position = 32;  // Right to left: start from right
                    USART_SendString("Dir: R>L\r\n> ");
                } else {
                    scroll_position = -strlen(scroll_text) * 6;  // Left to right: start from left
                    USART_SendString("Dir: L>R\r\n> ");
                }
                button_debounce_timer = 50;
            }
            
            // PC6: Text Up button
            if (button_pc6_prev == 1 && button_pc6_current == 0) {
                if (text_y_offset > 0) {
                    text_y_offset--;
                    USART_SendString("Text Up: Y=");
                    USART_Transmit('0' + (text_y_offset / 10));
                    USART_Transmit('0' + (text_y_offset % 10));
                    USART_SendString("\r\n> ");
                } else {
                    USART_SendString("Text at TOP\r\n> ");
                }
                button_debounce_timer = 50;
            }
            
            // PC7: Text Down button
            if (button_pc7_prev == 1 && button_pc7_current == 0) {
                if (text_y_offset < 15) {
                    text_y_offset++;
                    USART_SendString("Text Down: Y=");
                    USART_Transmit('0' + (text_y_offset / 10));
                    USART_Transmit('0' + (text_y_offset % 10));
                    USART_SendString("\r\n> ");
                } else {
                    USART_SendString("Text at BOTTOM\r\n> ");
                }
                button_debounce_timer = 50;
            }

            // Remember the current button states for next time (to detect when they change)
            button_pc0_prev = button_pc0_current;
            button_pc1_prev = button_pc1_current;
            button_pc2_prev = button_pc2_current;
            button_pc6_prev = button_pc6_current;
            button_pc7_prev = button_pc7_current;
        } else {
            // We're in debounce period - just count down the timer
            button_debounce_timer--;
        }

        // ===============================================
        // UPDATE THE LED DISPLAY
        // ===============================================
        // Clear the display and draw the current text
        vma419_clear(&dmd_display);
        vma419_font_draw_string(&dmd_display, scroll_position, text_y_offset, scroll_text);
        
        // Refresh the display using 4-phase multiplexing (1ms per phase = 250Hz refresh rate)
        for(uint8_t cycle = 0; cycle < 4; cycle++) {
            dmd_display.scan_cycle = cycle;
            vma419_scan_display_quarter(&dmd_display);
            _delay_ms(1);
        }        // ===============================================
        // UPDATE SCROLLING POSITION
        // ===============================================
        refresh_counter++;
        if(refresh_counter >= scroll_speed) {
            refresh_counter = 0;
            scroll_position += scroll_direction;  // Move text one pixel
            
            // Calculate text width (each character is 6 pixels wide)
            int16_t text_width = strlen(scroll_text) * 6;
            
            // Wrap around when text scrolls off the edge
            if (scroll_direction < 0) {
                // Right to left - restart from right when text disappears off left
                if(scroll_position < -text_width) {
                    scroll_position = 32;
                }
            } else {
                // Left to right - restart from left when text disappears off right
                if(scroll_position > 32) {
                    scroll_position = -text_width;
                }
            }
        }
    } // End of main loop

    return 0; // Program should never reach here, but good practice to include
}