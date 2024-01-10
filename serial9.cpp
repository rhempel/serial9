/* ---------------------------------------------------------------------------
  serial9.cpp - support for 9 bit serial data using the physical UART

  There are a couple of things to keep in mind ...

  1. The only thing this device has to do is transfer characters between
     the USB serial port and the UART. Therefore we use the Arduino loop()
     to continuously poll both devices - this eliminates the need for any
     buiffering.

  2. The ATmega32U UART is 9 bit capbable, but the current character MUST
     be completely finished before changing the state of the 9th bit.
*/

#include "Arduino.h"
#include "serial9.h"

#if !defined(HAVE_CDCSERIAL)
  #error This project requires an Arduino with USB-CDC capabilities
#endif

#if !defined(ARDUINO_AVR_LEONARDO)
  #error This project requires an AVR_LEONARDO compatible board (Pro Micro)
#endif

extern void serial9_set_baud(uint32_t baud);
extern void serial9_start(void);
extern void serial9_stop(void);

extern void serial9_talk(void);
extern void serial9_listen(void);
extern void serial9_offline(void);

extern bool serial9_rx_available(void);
extern uint16_t serial9_read(void);

extern bool serial9_tx_busy(void);
extern bool serial9_tx_complete(void);
extern void serial9_write(uint16_t data);

Serial9::Serial9()
{
  tx_state = SERIAL9_STATE_IDLE;
  _writing = false;
}

Serial9::~Serial9() {}

void Serial9::begin(uint32_t baud)
{
  serial9_set_baud(baud);
  serial9_start();
  serial9_listen();
}

void Serial9::end()
{
  serial9_stop();
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

  // A character has been received by the UART

  if (serial9_rx_available()) {

    // Check if the 9th bit is set and send the possibly ESCAPED data
    // back to the host

    uint16_t rx_data = serial9_read();

    if ((bool)(rx_data & bit(9))) {
      Serial.write(SERIAL9_ESCAPE);
      Serial.write(SERIAL9_HIGH);
      Serial.write((uint8_t)(rx_data & 0xff));
    } else if (SERIAL9_ESCAPE == rx_data) {
      Serial.write(SERIAL9_ESCAPE);
      Serial.write((uint8_t)(rx_data & 0xff));
    } else {
      Serial.write((uint8_t)(rx_data & 0xff));
    }

  // The UART is NOT ready to send a character, do nothing

  } else if (serial9_tx_busy()) {

  // The UART is ready to send a character, is there USB Serial data?

  } else if (Serial.available() > 0) {

    uint16_t tx_data = Serial.read();

    // Force DE high, we will be writing a character soon
    if (_writing) {
      // Do nothing
    } else {
        _writing = true;
        serial9_talk();
    }

    switch (tx_state) {

    case SERIAL9_STATE_IDLE:

      if (SERIAL9_ESCAPE == tx_data) {
        tx_state = SERIAL9_STATE_ESCAPE;

      } else {
        serial9_write(tx_data);
      }
      break;

    case SERIAL9_STATE_ESCAPE:

      if (SERIAL9_HIGH == tx_data) {
        tx_state = SERIAL9_STATE_HIGH;

      } else if (SERIAL9_ESCAPE == tx_data) {
        // It's an escaped ESCAPE character, just send it
        tx_state = SERIAL9_STATE_IDLE;
        serial9_write(tx_data);

      } else {
        // illegal character - ignore it
        tx_state = SERIAL9_STATE_IDLE;
      }
      break;

     case SERIAL9_STATE_HIGH:
       // It's a character that should be sent with the 9th bit high
       tx_state = SERIAL9_STATE_IDLE;
       serial9_write(tx_data | bit(9));
       break;
     }

  // The UART has completed the current character and there are no
  // incoming charaters availablke from the USB - force DE low

  } else if (serial9_tx_complete()) {

    // Force listen mode, we are no longer writing
    if (_writing) {
      _writing = false;
      serial9_listen();
    } else {
      // Do nothing
    }

  // No other cases to cover ... we are done

  } else {
    // Do nothing ...
  }
}