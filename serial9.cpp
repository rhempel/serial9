/* ---------------------------------------------------------------------------
  serial9.cpp - support for 9 bit serial data using the physical UART

  There are a couple of things to keep in mind ...

  1. The only thing this device has to do is transfer characters between
     the USB serial device and the UART. Therefore we use the Arduino loop()
     to continuously poll both devices - this eliminates the need for any
     buiffering.

  1. The ATmega32U UART is 9 bit capbable, but the current character MUST
     be completely finished before changing the state of the 9th bit.

  2. There is no need for buffereing the data from the host - we only read
     a new character from the host when the 
*/
#include "Arduino.h"
#include "serial9.h"

// Move this to a private H file
#if defined(__AVR_ATmega32U4__)
  #define TXC0 TXC1
  #define RXC0 RXC1
  #define RXEN0 RXEN1
  #define TXEN0 TXEN1
  #define RXCIE0 RXCIE1
  #define UDRIE0 UDRIE1
  #define U2X0 U2X1
  #define UPE0 UPE1
  #define UDRE0 UDRE1
  #define UCSZ00 UCSZ10
  #define UCSZ01 UCSZ11
  #define UCSZ02 UCSZ12
  #define TXCIE0 TXCIE1
  #define TXB80 TXB81
  #define RXB80 RXB81
#else
  #error This library currently only works with ATmega32U4
#endif

#define RE_ (2)
#define DE  (3)

Serial9::Serial9()
{
#if defined(UBRR1H)
    _ubrrh = &UBRR1H;
    _ubrrl = &UBRR1L;

    _ucsra = &UCSR1A;
    _ucsrb = &UCSR1B;
    _ucsrc = &UCSR1C;
    _udr = &UDR1;

    rx_state = SERIAL9_STATE_IDLE;
#else
  #error Currently only Seria1 with ATmega32U4 is supported
#endif
}

Serial9::~Serial9() {}

void Serial9::begin(uint32_t baud)
{
  uint16_t baud_setting = (F_CPU / 4 / baud - 1) / 2;
  *_ucsra = 1 << U2X0;

  // hardcoded exception for 57600 for compatibility with the bootloader
  // shipped with the Duemilanove and previous boards and the firmware
  // on the 8U2 on the Uno and Mega 2560. Also, The baud_setting cannot
  // be > 4095, so switch back to non-u2x mode if the baud rate is too
  // low.
  if (((F_CPU == 16000000UL) && (baud == 57600)) || (baud_setting >4095))
  {
    *_ucsra = 0;
    baud_setting = (F_CPU / 8 / baud - 1) / 2;
  }

  // assign the baud_setting, a.k.a. ubrr (USART Baud Rate Register)
  *_ubrrh = baud_setting >> 8;
  *_ubrrl = baud_setting;

  // disable interrupts, set 9bit mode, enable rx/tx
  //  This completely overwrites any previous UCSRB bits
  *_ucsrb = (1 << UCSZ02) | (1 << TXEN0) | (1 << RXEN0) | // 9bits, enabled
            (0 << UDRIE0) | (0 << TXCIE0) | (0 << RXCIE0); // ints off.

  // Enable 9bit size in UCSRC.  Leave the other bits alone (Parity, etc)
  *_ucsrc |= (1 << UCSZ00) | (1 << UCSZ01);

  _written = false;

  pinMode(DE, OUTPUT);   // Default DE to LOW so we are not transmitting
  digitalWrite(DE, LOW);

  pinMode(RE_, OUTPUT);   // Default RE_ to LOW so we are always listening
  digitalWrite(RE_, LOW);
}

void Serial9::end()
{
  // wait for transmission of outgoing data
  *_ucsrb &= ~((1 << TXEN0) | (1 << RXEN0) | // 9bits, enabled
               (0 << UDRIE0) | (0 << TXCIE0) | (0 << RXCIE0)); // ints off.
}

#define SERIAL9_ESCAPE (0xff)
#define SERIAL9_HIGH   (0x01)

// NOTE: Break this long loop handler into separate components

static bool bit9;

void Serial9::loop(void)
{
  // Highest priority is checking to see if a character is available
  // in the hardware serial device, and sending the data back to the
  // host using the USB9_ESCAPE sequence if necessary.

  uint8_t status = *_ucsra;
  
  // A character has been received by the UART

  if (status & (1 << RXC0)) {

    // Check if the 9th bit is set and send the possibly ESCAPED data
    // back to the host

    bool bit9_hi = (0 != (*_ucsrb & (1 << RXB80)));
    uint8_t data = *_udr;

    if (bit9_hi) {
      Serial.write(SERIAL9_ESCAPE);
      Serial.write(SERIAL9_HIGH);
      Serial.write(data);
    } else if (SERIAL9_ESCAPE == data) {
      Serial.write(SERIAL9_ESCAPE);
      Serial.write(data);
    } else {
      Serial.write(data);
    }
  
  // The UART is NOT ready to send a character, do nothing

  } else if (0 == (status & (1 << UDRE0))) {
    
  // The UART is ready to send a character, is there Serial data?

  } else if (Serial.available() > 0) {

    uint8_t b = Serial.read();

    // Force DE high, we will be writing a character soon
    if (_written) {
      // Do nothing
    } else {
        _written = true;
        *_ucsra |= (1 << TXC0); // Write a 1 to clear the TXC0 bit
        digitalWrite(DE, HIGH);
    }

    switch (rx_state) {

    case SERIAL9_STATE_IDLE:

      if (SERIAL9_ESCAPE == b) {
        rx_state = SERIAL9_STATE_ESCAPE;

      } else {
        // It's a regular character, just send it
        *_ucsrb &= ~(1 << TXB80);
        *_udr = b;
      }
      break;

    case SERIAL9_STATE_ESCAPE:

      if (SERIAL9_HIGH == b) {
        rx_state = SERIAL9_STATE_HIGH;

      } else if (SERIAL9_ESCAPE == b) {
        // It's an escaped ESCAPE character, just send it
        rx_state = SERIAL9_STATE_IDLE;
        *_ucsrb &= ~(1 << TXB80);
        *_udr = b;

      } else {
        // illegal character - ignore it
        rx_state = SERIAL9_STATE_IDLE;
      }
      break;

     case SERIAL9_STATE_HIGH:
       // It's a character that should be sent with the 9th bit high
       rx_state = SERIAL9_STATE_IDLE;
       *_ucsrb |= (1 << TXB80);
       *_udr = b;
       break;       
     }

  // The UART has completed the current character and there are no
  // incoming charaters availablke from the USB - force DE low

  } else if (0 != (status & (1 << TXC0))) {

    // Force DE high, we will be writing a character soon
    if (_written) {
      _written = false;
      digitalWrite(DE, LOW);
    } else {
      // Do nothing
    }

  // No other cases to cover ... we are done

  } else {
    // Do nothing ...
  }
}