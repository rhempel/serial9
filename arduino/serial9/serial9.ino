/* ---------------------------------------------------------------------------
Serial9 - converts an 8 bit data stream from the USB serial port to a 9 bit
          data stream on the hardware serial port of a AVR_LEONARDO
          compatible Arduino device

See README and LICENCE for more information
*/

#include "Arduino.h"

#if !defined(HAVE_CDCSERIAL)
  #error This project requires an Arduino with USB-CDC capabilities
#endif

#if !defined(ARDUINO_AVR_LEONARDO)
  #error This project requires an AVR_LEONARDO compatible board (Pro Micro)
#endif

#include "serial9.h"

Serial9 s9;

// the setup function runs once when you press reset or power the board
void setup() {
  s9.begin(9600);
}

// the loop function runs over and over again forever
void loop() {
   s9.loop();
}
