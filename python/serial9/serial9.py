# -----------------------------------------------------------------------------
"""Connects an 8 bit data stream to a 9 bit serial device

This allows us to run the control program on another PC or in a Docker
instance and leave the host to conenct to the physical device.

It may seem like overkill, but it provides the flexibility to implement the
port interface to a socket, physical serial port, a loopback device
or even a unit test interface.

The ``conn`` device used to initialize a ``Serial9`` instance must provide
at least these interface functions:

- tx(bytes)
- bytes rx()

Physical Interface Construction
===============================

This package is intended to interface with a physical device made up
of these two components:

- `Arduino ProMicro (USB-C)`_ - Atmega32U4 (USB-C) with USB and separate Serial/GPIO pins
- `Serial to 485`_ - An adapter for logic level serial to RS485 voltage

If you want the ProMicro with MicroUSB then choose:

- `Arduino ProMicro (MicroUSB)`_ - Atmega32U4 (MicroUSB) with USB and separate Serial/GPIO pins

With these two boards, the Arduino Development IDE, and the `Serial9`_ firmware, we can
make a little adapter that takes 8 bit data from the USB serial port and reliably
transforms it to 9 bit serial data for the RS-485 bus. See :numref:`serial9wiring`

.. _serial9wiring:
.. figure:: ./images/Serial9Wiring.jpg
    :alt: How to connect the ProMicro to the RS485 Adapter
    :align: Center

    Wiring to connect the ProMicro to the RS485 Adapter

.. _`Arduino ProMicro (USB-C)`: https://www.amazon.com/HiLetgo-ATmega32U4-Headers-Compitable-Arduino/dp/B09KGY2NWT
.. _`Arduino ProMicro (MicroUSB)`: https://www.amazon.com/HiLetgo-Atmega32U4-Bootloadered-Development-Microcontroller/dp/B01MTU9GOB
.. _`Serial to 485`: https://www.amazon.ca/Garosa-Converter-Raspberry-Integrated-Circuits/dp/B07X3JX7SY
.. _`Serial9`: https://github.com/rhempel/serial9

Public API
==========

.. automethod:: serial9.Serial9.tx8
.. automethod:: serial9.Serial9.tx9
.. automethod:: serial9.Serial9.rx

Optional API
============

The ``Serial9`` firmware has optional escape sequences for changing the baud rate of
the serial interface - the data rate between the host and the ``Serial9`` device
is not affected. This allows the ``Serial9`` device to be used at any supported baud
rate, instead fo just the default of 9600 .

.. automethod:: serial9.Serial9.set_baud

Encoding 9 Bit Data for an 8 Bit Interface
==========================================

The `Serial9`_ firmware runs on the Arduino ProMicro and translates between 8 and
9 bit data using a simple state machine. Rather than showing a traditional state diagram, which
implies how to implement the code, we will try to use Extended BNF "railroad diagrams" to
show how to construct (and decode) the escape sequences for communicating with the ProMicro
device. 

.. uml::
    :caption: EBNF Railroad Diagrams for ``Serial9`` Communication
    :align: center

    @startebnf
    Send_Data = Non_Escape | Escaped_Escape | Bit_9_High;
    Escaped_Escape = Escape, Escape;
    Bit_9_High = Escape, 0x01, Character;

    Escape = "0xff";
    Non_Escape = "0x00 - 0xfe";
    Character = "0x00 - 0xff";
    @endebnf

The `Serial9`_ firmware has an optional set of escape sequences for setting the baud rate. They
are not passed to the physical serial interface.

.. uml::
    :caption: EBNF Railroad Diagrams for ``Serial9`` Baud Rate Change
    :align: center

    @startebnf
    Change_Baud = Escape, ( Baud_300 | Baud_600 | Baud_1200 | Baud_2400 | Baud_4800 | Baud_9600 | Baud_19200 | Baud_38400 | Baud_57600 | Baud_115200);

    Baud_300 = 0x10;
    Baud_600 = 0x11;
    Baud_1200 = 0x12;
    Baud_2400 = 0x13;
    Baud_4800 = 0x14;
    Baud_9600 = 0x15;
    Baud_19200 = 0x16;
    Baud_38400 = 0x17;
    Baud_57600 = 0x18;
    Baud_115200 = 0x19;
    Escape = "0xff";
    @endebnf
"""
# -----------------------------------------------------------------------------

import sys
import re
import time
import logging

import serial
import serial.tools.list_ports

# -----------------------------------------------------------------------------
class Serial9():

    SERIAL9_ESCAPE = 0xff
    SERIAL9_HIGH = 0x01

    SERIAL9_STATE_IDLE = 0x00
    SERIAL9_STATE_ESCAPE = 0x01
    SERIAL9_STATE_HIGH = 0x02

    SERIAL_9_BAUD_300 = 0x10
    SERIAL_9_BAUD_600 = 0x11
    SERIAL_9_BAUD_1200 = 0x12
    SERIAL_9_BAUD_2400 = 0x13
    SERIAL_9_BAUD_4800 = 0x14
    SERIAL_9_BAUD_9600 = 0x15
    SERIAL_9_BAUD_19200 = 0x16
    SERIAL_9_BAUD_38400 = 0x17
    SERIAL_9_BAUD_57600 = 0x18
    SERIAL_9_BAUD_115200 = 0x19

    def __init__(self, conn=None):

        self.logger = logging.getLogger(__name__)
        self._conn = conn
        self._rx_state = self.SERIAL9_STATE_IDLE
        self._loopback_buffer = b""

    def tx8(self, s):
        '''Send string to target with bit 9 low in all bytes, handle escape character

        Parameters:
            s (bytes): Data to be sent to the target

        '''

        self.logger.debug(f"tx8 {s}")
        d = re.sub(b"\xff", b"\xff\xff", s, flags=re.DOTALL)
        try:
            self._conn.tx(d)
        except:
            self._loopback_buffer += d

    def _escape_9(self, m):
        return b"\xff\x01" + m.group(0)

    def tx9(self, s):
        '''Send string to target with bit 9 high in all bytes

        Parameters:
            s (bytes): Data to be sent to the target
        '''
        self.logger.debug(f"tx9 {s}")
        d = re.sub(b".", self._escape_9, s, flags=re.DOTALL)
        try:
            self._conn.tx(d)
        except:
            self._loopback_buffer += d

    def rx(self):
        '''Return the data from the target as a list of integers

        If the data was received from the target with bit 9 low, the
        integer is in the range of ``0x0000`` to ``0x00ff``.

        If the data was received from the target with bit 9 high, the
        integer is in the range of ``0x0100`` to ``0x01ff``.

        Returns:
            [ integer, ... ] 
        '''

        try:
            raw_data = self._conn.rx()
        except:
            raw_data = self._loopback_buffer
            self._loopback_buffer = b""

        d = []

        for c in raw_data:
            if self._rx_state == self.SERIAL9_STATE_IDLE:
                if self.SERIAL9_ESCAPE == c:
                    self._rx_state = self.SERIAL9_STATE_ESCAPE
                else:
                    # It's an unescaped character, just append it
                    d.append(c)

            elif self._rx_state == self.SERIAL9_STATE_ESCAPE:
                if self.SERIAL9_HIGH == c:
                    self._rx_state = self.SERIAL9_STATE_HIGH

                elif self.SERIAL9_ESCAPE == c:
                    # It's an escaped ESCAPE character, just append it
                    self._rx_state = self.SERIAL9_STATE_IDLE
                    d.append(c)

                else:
                    # It's an illegal character - ignore it
                    self._rx_state = self.SERIAL9_STATE_IDLE

            elif self._rx_state == self.SERIAL9_STATE_HIGH:
                     # It's a character with the 9th bit high, append it
                     self._rx_state = self.SERIAL9_STATE_IDLE
                     d.append(c + 0x100)

            else:
                self._rx_state = self.SERIAL9_STATE_IDLE
                d.append(c)
                self.logger.error("Unhandled state")

        self.logger.debug(f"rx loop return {d}")
        return d

    def set_baud(self, baud):
        '''Send baud rate change escape sequence to the target

        Parameters:
            baud (int): One of the SERIAL_9_BAUD_xxx constants
        '''
        self._conn.tx(bytes([self.SERIAL9_ESCAPE, baud]))

# -----------------------------------------------------------------------------
def serial9(conn=None): # pragma no cover
    # If port is None, search for the first Arduino ProMicro
    # VID:PID=2341:8036 SER=
    if port:
        self.port = port
    else:
        for p in sorted(serial.tools.list_ports.comports()):
            if ((p.vid == 0x2341) and (p.pid == 0x8036)):
                self.logger.info('Found an Arduino ProMicro device')
                self.port = p.device
                break
            else:
                self.logger.info('Setting self.port to None')
                self.port = None
                pass

    s9 = Serial9(port)

    if isinstance(conn, socket.socket):
        logger.info(f'sock type matches')

    while True:
        s9.tx9(b"\x02")
        s9.tx8(b"abc\xff\x01\xff123")
        s9.tx9(b"\xFF")
        time.sleep(.008)
        s9.rx()

# -----------------------------------------------------------------------------
if __name__ == "__main__": # pragma no cover
    if 1 == len(sys.argv):
        serial9()
    elif 2 == len(sys.argv):
        serial9(sys.argv[1])
    else:
        logging.info(f"Too many parameters: {sys.argv[2:]}")
