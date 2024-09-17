/* ---------------------------------------------------------------------------
  serial9_atmega_32u.cpp - hardware specific support for serial9

  Currently only the __AVR_ATmega32U4__ is supported
*/
#include "Arduino.h"

#if defined(__AVR_ATmega32U4__)
  #define TXC TXC1
  #define RXC RXC1
  #define RXEN RXEN1
  #define TXEN TXEN1
  #define RXCIE RXCIE1
  #define UDRIE UDRIE1
  #define U2X U2X1
  #define UPE UPE1
  #define UDRE UDRE1
  #define UCSZ0 UCSZ10
  #define UCSZ1 UCSZ11
  #define UCSZ2 UCSZ12
  #define TXCIE TXCIE1
  #define TXB8 TXB81
  #define RXB8 RXB81

  #define UBRRH UBRR1H
  #define UBRRL UBRR1L

  #define UCSRA UCSR1A
  #define UCSRB UCSR1B
  #define UCSRC UCSR1C
  #define UDR UDR1

  #define RE_ (2)
  #define DE  (3)
#else
  #error This library currently only works with ATmega32U4
#endif

// UCSRA has three bits that are R/W - we need to be sure we are writing
// the correct value to the other bits when writing to a specific bit!
//
// UCSRA:0 MPCM - Multi-processor Communication Mode - set to 0
// UCSRA:1 U2X  - USART speed - set depending on baud rate
// UCSRA:6 TCX  - Transmisison complete - set to CLEAR this bit
//
// In our use case, we write to the UCSRA register in two cases:
//
// 1. When we set the baud rate
// 2. When we need to clear the TXC bit
//
// If we assume that the baud rate only changes when any transmission
// is complete, we can preset the ucsra_shadow variable with the correct
// value and always use the shadow copy when writing to UCSRA.
//
static uint8_t ucsra_shadow = bit(TXC);

void serial9_set_8bit_mode(void)
{
  UCSRB &= ~bit(UCSZ2);
}

void serial9_set_9bit_mode(void)
{
  UCSRB |= bit(UCSZ2);
}

void serial9_set_baud(uint32_t baud)
{
  uint16_t baud_setting = (F_CPU / 4 / baud - 1) / 2;
  ucsra_shadow |= bit(U2X);

  // Hardcoded exception for 57600 for compatibility with the bootloader
  // shipped with the Duemilanove and previous boards and the firmware
  // on the 8U2 on the Uno and Mega 2560. Also, The baud_setting cannot
  // be > 4095, so switch back to non-u2x mode if the baud rate is too
  // low.
  if (((F_CPU == 16000000UL) && (baud == 57600)) || (baud_setting >4095))
  {
    baud_setting = (F_CPU / 8 / baud - 1) / 2;
    ucsra_shadow &= ~bit(U2X);
  }

  UCSRA = ucsra_shadow;

  // assign the baud_setting, a.k.a. ubrr (USART Baud Rate Register)
  UBRRH = baud_setting >> 8;
  UBRRL = baud_setting;

  digitalWrite(DE, LOW);
  digitalWrite(RE_, LOW);
}

void serial9_start(void)
{
  // Disable interrupts, set 9bit mode, enable rx/tx - this completely
  // overwrites any previous UCSRB bits
  UCSRB |= bit(TXEN) | bit(RXEN);

  // Enable 8bit size in UCSRC and leave the other bits alone (Parity, etc)
  UCSRC = bit(UCSZ0) | bit(UCSZ1);

  // Set the DE and RE_ pins to output
  pinMode(DE, OUTPUT);
  pinMode(RE_, OUTPUT);
}

void serial9_stop(void)
{
  // Turn off RX and TX
  UCSRB &= ~(bit(TXEN) | bit(RXEN));

  // Set the DE and RE_ pins to input
  pinMode(DE, INPUT);
  pinMode(RE_, INPUT);
}

void serial9_talk(void)
{
  digitalWrite(DE, HIGH);
}

// NOTE: Set RE_ HIGH to prevent a glitch on the RX line before
//       you set the DE to LOW - then set RE_ low to enable
//       receiving again

void serial9_listen(void)
{
  digitalWrite(RE_, HIGH);
  digitalWrite(DE, LOW);
  digitalWrite(RE_, LOW);
}

void serial9_offline(void)
{
  digitalWrite(DE, LOW);
  digitalWrite(RE_, HIGH);
}

bool serial9_rx_available(void)
{
  return (bool)(UCSRA & bit(RXC));
}

uint16_t serial9_read(void)
{
  if (!(bool)(UCSRA & bit(RXC))) {
    return -1;
  } else if ((bool)(UCSRB & bit(RXB8))) {
    return UDR | bit(8);
  } else {
    return UDR;
  }
}

bool serial9_tx_busy(void)
{
  return 0 == (UCSRA & bit(UDRE));
}

bool serial9_tx_complete(void)
{
  return (bool)(UCSRA & bit(TXC));
}

void serial9_write(uint16_t data)
{
  if ((bool)(UCSRA & bit(TXC))) {
      UCSRA = ucsra_shadow;
  }

  if (data & bit(8)) {
      UCSRB |= bit(TXB8);
  } else {
      UCSRB &= ~bit(TXB8);
  }

  UDR = (uint8_t)(data & 0xff);
}
