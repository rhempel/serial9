Welcome to Serial9's documentation!
=======================================

The ``Serial9`` project is a tool to generate a 9 bit data stream from
a standard serial USB (CDC) device. This is useful when you need to
interface with something like an RS-485 bus that has a wakeup bit.

You might be able to coax a standard FTDI device to mess around with
the parity bit - but in my experience this is unreliable at best.

To buld this device you will also need an Arduino IDE - just open the
`arduino/serial9/serial9.ino` file in the IDE, build, and download to
the ProMicro device.

There is also a `serial9.py` Python library that you can use to interface
to the USB device in a platform independent way. The API is described below.

.. toctree::
   :maxdepth: 3
   :caption: Quick Links

:mod:`serial9` -- Interface To 9 Bit SerialDdevice
--------------------------------------------------

.. automodule:: serial9
    :no-members:
    :special-members: