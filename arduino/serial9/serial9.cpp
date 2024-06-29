/* ---------------------------------------------------------------------------
  serial9.cpp - support for 9 bit serial data using the physical UART

  There are a couple of things to keep in mind ...

  1. The only thing this device has to do is transfer characters between
     the USB serial port and the UART. Therefore we use the Arduino loop()
     to continuously poll both devices - this eliminates the need for any
     buiffering.

  2. The ATmega32U UART is 9 bit capbable, but the current character MUST
     be completely finished before changing the state of the 9th bit.

  3. The Arduino USB-CDC librbary uses a blocking send when returning
     data back to the host. To make things worse, when there are no more
     endpoints available, the send function BLOCKS with a timeout of
     250 msec, which results in no characters being transmitted during
     this time.

     To prevent this from happening, the host MUST read the incoming
     serial port as often as possible.
*/

#include <stddef.h>
#include <stdint.h>

#include "serial9.h"
#include "Arduino.h"

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

// Here is where the escape protocol is defined ...
//
#define SERIAL9_BIT9 (0x0100) // Mask for the 9th bit

#define SERIAL9_ESCAPE (0xff)
#define SERIAL9_HIGH (0x01) // The next byte is sent with BIT9 high

#define SERIAL9_BAUD_300 (0x10)
#define SERIAL9_BAUD_600 (0x11)
#define SERIAL9_BAUD_1200 (0x12)
#define SERIAL9_BAUD_2400 (0x13)
#define SERIAL9_BAUD_4800 (0x14)
#define SERIAL9_BAUD_9600 (0x15)
#define SERIAL9_BAUD_19200 (0x16)
#define SERIAL9_BAUD_38400 (0x17)
#define SERIAL9_BAUD_57600 (0x18)
#define SERIAL9_BAUD_115200 (0x19)

#ifndef SERIAL9_BUFFER_SIZE
  #define SERIAL9_BUFFER_SIZE (32)
#endif

void Serial9::loop(void)
{
  // Highest priority is checking to see if a character is available
  // in the hardware serial device, and sending the data back to the
  // host using the USB9_ESCAPE sequence if necessary.

  // A character has been received by the UART

  if (serial9_rx_available()) {

    // Check if the 9th bit is set and send the possibly ESCAPED data
    // back to the host. Yes, we could factor out the data write at
    // the end of each of the three conditions, but it's easier to
    // understand if we don't
    //
    uint16_t rx_data = serial9_read();

    if ((bool)(rx_data & SERIAL9_BIT9)) {
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
    // No point getting more from Serial if we are still
    // busy transmitting on serail9 :-)
    //
    DO_NOTHING;

  // The UART is ready to send a character, is there USB Serial data?

  } else if (Serial.available() > 0) {

    uint16_t tx_data = Serial.read();

    // Force the interface into talk mode, we will be writing
    // a character soon
    //
    if (_writing) {
        // Do nothing
        DO_NOTHING;
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

      // Most of the time the next state will be SERIAL9_STATE_IDLE
      // so we set it here - override if necessary!
      //
      tx_state = SERIAL9_STATE_IDLE;

      if (SERIAL9_HIGH == tx_data) {
        tx_state = SERIAL9_STATE_HIGH;

      } else if (SERIAL9_ESCAPE == tx_data) {
        // It's an escaped ESCAPE character, just send it
        serial9_write(tx_data);

      } else if (SERIAL9_BAUD_300 == tx_data) {
        serial9_set_baud(300);

      } else if (SERIAL9_BAUD_600 == tx_data) {
        serial9_set_baud(600);

      } else if (SERIAL9_BAUD_1200 == tx_data) {
        serial9_set_baud(1200);

      } else if (SERIAL9_BAUD_2400 == tx_data) {
        serial9_set_baud(2400);

      } else if (SERIAL9_BAUD_4800 == tx_data) {
        serial9_set_baud(4800);

      } else if (SERIAL9_BAUD_9600 == tx_data) {
        serial9_set_baud(9600);

      } else if (SERIAL9_BAUD_19200 == tx_data) {
        serial9_set_baud(19200);

      } else if (SERIAL9_BAUD_38400 == tx_data) {
        serial9_set_baud(38400);

      } else if (SERIAL9_BAUD_57600 == tx_data) {
        serial9_set_baud(57600);

      } else if (SERIAL9_BAUD_115200 == tx_data) {
        serial9_set_baud(115200);

      } else {
        // illegal character - ignore it
        tx_state = SERIAL9_STATE_IDLE;
      }
      break;

    case SERIAL9_STATE_HIGH:
        // It's a character that should be sent with the 9th bit high
        tx_state = SERIAL9_STATE_IDLE;
        serial9_write(tx_data | SERIAL9_BIT9);
        break;

    default:
       // Weird state when we got this character, ignore it and
       // force the IDLE state
       //
       DO_NOTHING;
       tx_state = SERIAL9_STATE_IDLE;
    }

  // The UART has completed the current character and there are no
  // incoming charaters available from the USB - force the interface
  // into the listen state if we were writing

  } else if (serial9_tx_complete()) {

    // Force listen mode, we are no longer writing
    if (_writing) {
      _writing = false;
      serial9_listen();
    } else {
      // Do nothing
      DO_NOTHING;
    }

    tx_state = SERIAL9_STATE_IDLE;

  // No other cases to cover ... we are done

  } else {
    // Do nothing ...
    DO_NOTHING;
  }
}
