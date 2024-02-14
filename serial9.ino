/*

*/

#include "Arduino.h"

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