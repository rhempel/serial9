
## Serial9 - support for 9 bit serial data using the physical UART

Based on https://github.com/WestfW/Duino-hacks/tree/master/Serial9_test
     and https://gist.github.com/benVolatiles/c0aa455d4ace0761537818efbaba40ff

Data is transferred to the hardware serial port from the USB serial interface
using escaped 8 bit data:

```
  ESC 01 0xdd  - Send 0x1dd
  ESC ESC      - Next byte to send is 0x0ESC
  ESC 0x1[0-9] - Set baud rate - see below for details
  0xdd         - Send 0x0dd
```

The same process in reverse sends 9 bit data from the hardware serial
port to the USB serial interface:

```
    0x0dd   - As long as dd != ESC send is dd
    0x0ESC  - Send ESC ESC
    0x1dd   - Send 0xFF 0x01 0xdd
```

  This is implemented as a trivial state machine.
  
## Usage

This is a standalone Arduino project that is intended to use an
Arduino LEONARDO or equivalent ATMEGA 32U based device, such as
the ProMicro. If you want to add RS485 support, you will need an
adapter board.

Just import the ardiono/serial9.ino file into your Arduino IDE, then
build and download.

## Python library

There is a Python 3.x compatible library. You will need to install
the pyserial package to be able to find and communicate with the
serial9 device.

## Future?
  NOTE: For flexibility in the future, consider adding additional escape
        codes to support:

        1. Disable listen mode when transmitting
        3. ???