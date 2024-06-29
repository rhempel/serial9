#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "Arduino.h"

size_t MockSerial::write(unsigned char c)
{
    mock().actualCall("write").onObject(this).withParameter("c", c);
    return mock().intReturnValue();
}

uint16_t MockSerial::read(void)
{
    mock().actualCall("read").onObject(this);
    return mock().unsignedIntReturnValue();
}

unsigned int MockSerial::available(void)
{
    mock().actualCall("available").onObject(this);
    return mock().intReturnValue();
}

void serial9_set_baud(uint32_t baud)
{
    mock().actualCall("serial9_set_baud").withParameter("baud", baud);
}

void serial9_start(void)
{
    mock().actualCall("serial9_start");
}

void serial9_stop(void)
{
    mock().actualCall("serial9_stop");
}

void serial9_talk(void)
{
    mock().actualCall("serial9_talk");
}

void serial9_listen(void)
{
    mock().actualCall("serial9_listen");
}

void serial9_offline(void)
{
    mock().actualCall("serial9_offline");
}

bool serial9_rx_available(void)
{
    mock().actualCall("serial9_rx_available");
    return mock().boolReturnValue();
}

uint16_t serial9_read(void)
{
    mock().actualCall("serial9_read");
    return mock().intReturnValue();
}

bool serial9_tx_busy(void)
{
    mock().actualCall("serial9_tx_busy");
    return mock().boolReturnValue();
}

bool serial9_tx_complete(void)
{
    mock().actualCall("serial9_tx_complete");
    return mock().boolReturnValue();
}

void serial9_write(uint16_t data)
{
    mock().actualCall("serial9_write").withParameter("data", data);
}

