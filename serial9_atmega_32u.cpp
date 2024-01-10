/* ---------------------------------------------------------------------------
  serial9_atmega_32u.cpp - hardwarte specific support for serial9
  
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

void serial9_set_baud(uint32_t baud)
{
  uint16_t baud_setting = (F_CPU / 4 / baud - 1) / 2;
  UCSRA = 1 << U2X;

  // hardcoded exception for 57600 for compatibility with the bootloader
  // shipped with the Duemilanove and previous boards and the firmware
  // on the 8U2 on the Uno and Mega 2560. Also, The baud_setting cannot
  // be > 4095, so switch back to non-u2x mode if the baud rate is too
  // low.
  if (((F_CPU == 16000000UL) && (baud == 57600)) || (baud_setting >4095))
  {
    UCSRA = 0;
    baud_setting = (F_CPU / 8 / baud - 1) / 2;
  }

  // assign the baud_setting, a.k.a. ubrr (USART Baud Rate Register)
  UBRRH = baud_setting >> 8;
  UBRRL = baud_setting;
}

void serial9_start(void)
{
  // Disable interrupts, set 9bit mode, enable rx/tx - this completely
  // overwrites any previous UCSRB bits
  UCSRB = bit(UCSZ2) | bit(TXEN) | bit(RXEN);

  // Enable 9bit size in UCSRC and leave the other bits alone (Parity, etc)
  UCSRC |= bit(UCSZ0) | bit(UCSZ1);

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
  UCSRA |= bit(TXC); // Write a 1 to clear the TXC0 bit (it's not always set)
}

void serial9_listen(void)
{
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
  if ((bool)(UCSRB & bit(RXB8))) {
    return UDR | bit(9);
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
  if (data & bit(9)) {
      UCSRB |= bit(TXB8);    
  } else {
      UCSRB &= ~bit(TXB8);
  }
  
  UDR = (uint8_t)(data & 0xff);
}
