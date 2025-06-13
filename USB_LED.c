
#include <avr/io.h>
#include <util/delay.h>
#include <string.h>

#define BAUD 9600 //baud rate
#define MYUBRR ((F_CPU / (16UL * BAUD)) - 1) //calculate speed of transmition

void USART_Init(unsigned int ubrr) {
    //UBRRH = (unsigned char)(ubrr >> 8);
    //UBRRL = (unsigned char)ubrr;    //set lower and higher bits of UART register to set the speed
    UBRRL = (unsigned char)51; 
    UCSRB = (1 << RXEN) | (1 << TXEN); // turn on USART receive. turn on USART transmition 
    UCSRC = (1 << URSEL) | (1 << UCSZ1) | (1 << UCSZ0);  // select register UCSRC-usart control. 8-bit, no parity, 1 stop
}

void USART_Transmit(char data) {
    while (!(UCSRA & (1 << UDRE))); //wait until receiver buffpr is empty
    UDR = data; //load charater to data register of UART
}

void USART_SendString(const char* str) {
    while (*str) { //transmit as long as the last character is not 0
        USART_Transmit(*str++); //transmit character and switch to the next one
    }
}

char USART_Receive(void) {
    while (!(UCSRA & (1 << RXC))); // Wait until u receive character
    return UDR; // return the received chracter from the data register
}

void checkCommand(const char* command) {
    USART_SendString("Received: ");
    USART_SendString(command);
    USART_SendString("\r\n");

    if (strcmp(command, "LED_ON") == 0) {
        PORTA |= (1 << PA1);
        USART_SendString("LED is ON\r\n");
    } else if (strcmp(command, "LED_OFF") == 0) {
        PORTA &= ~(1 << PA1);
        USART_SendString("LED is OFF\r\n");
    } else {
        USART_SendString("Unknown command\r\n");
    }
}

int main(void) {
    USART_Init(MYUBRR);
    DDRA |= (1 << PA1); 

    char buffer[16]; //buffer for received characters.
    uint8_t i = 0;  //variable to switch to the next characters

    while (1) {
        
        char c = USART_Receive(); //Receive the character
        
        if (c == '\r' || c == '\n') { //if its CR or LF
            buffer[i] = '\0';  //finish the string with null 
            if (i > 0) { // if theres something in buffer
                checkCommand(buffer); // check buffer command
                i = 0; // i back to 0. Ready to save new command to the beggining of the buffer (?)
            }
        } else if (i < sizeof(buffer) - 1) { // if buffer is not full (-1 cause of null at the end of the string)
            buffer[i++] = c; //add received character to the buffer
        }
    }
}
