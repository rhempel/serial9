#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "serial9.h"
#include "Arduino.h"

Serial9 *s9;
MockSerial Serial;

TEST_GROUP(Serial9)
{

    void setup()
    {
        s9 = new Serial9();
    }

    void teardown()
    {
        delete s9;
        mock().clear();
    }
};

TEST(Serial9, begin)
{
//  GIVEN: An uninitialized serial9 object 
//  WHEN:  The begin() method is called
//  THEN:  The default baud rate is set
//         The low level interface is started
//         The device is put in listen mode

    mock().expectOneCall("serial9_set_baud").withParameter("baud", 9600);
    mock().expectOneCall("serial9_start");
    mock().expectOneCall("serial9_listen");

    s9->begin(9600);

    mock().checkExpectations();
}

TEST(Serial9, end)
{
//  GIVEN: A serial9 object 
//  WHEN:  The end() method is called
//  THEN:  The low level interface is stopped

    mock().expectOneCall("serial9_stop");

    s9->end();

    mock().checkExpectations();
}

TEST(Serial9, idle)
{
//  GIVEN: An initialized serial9 object 
//  WHEN:  There is no data available from the USB or 485 ports
//         And the transmitter is not busy
//         And any pending 485 transmission is not complete
//  THEN:  We do nothing

    mock().expectOneCall("serial9_rx_available").andReturnValue(false);
    mock().expectOneCall("serial9_tx_busy").andReturnValue(false);
    mock().expectOneCall("available").onObject(&Serial).andReturnValue(0);
    mock().expectOneCall("serial9_tx_complete").andReturnValue(false);

    s9->loop();

    mock().checkExpectations();
}

TEST(Serial9, idle_tx_complete_not_writing)
{
//  GIVEN: An initialized serial9 object 
//  WHEN:  There is no data available from the USB or 485 ports
//         And the transmitter is not busy
//         And any pending 485 transmission is complete
//  THEN:  We do nothing

    mock().expectOneCall("serial9_rx_available").andReturnValue(false);
    mock().expectOneCall("serial9_tx_busy").andReturnValue(false);
    mock().expectOneCall("available").onObject(&Serial).andReturnValue(0);
    mock().expectOneCall("serial9_tx_complete").andReturnValue(true);

    s9->loop();

    mock().checkExpectations();
}

TEST(Serial9, serial9_available_non_escape_8bit_data)
{
//  GIVEN: Idle system with available data on serial9
//  WHEN:  A character with bit9 low is available on serial9
//  THEN:  The byte is written to the Serial object

    mock().expectOneCall("serial9_rx_available").andReturnValue(true);
    mock().expectOneCall("serial9_read").andReturnValue(0xaa);
    mock().expectOneCall("write").onObject(&Serial).withParameter("c", 0xaa).andReturnValue(0x01);

    s9->loop();

//  GIVEN: One or more bytes have been written to the Serial object
//  WHEN:  No characters are available from Serial
//  THEN:  Nothing else happens

    mock().expectOneCall("serial9_rx_available").andReturnValue(false);
    mock().expectOneCall("serial9_tx_busy").andReturnValue(false);
    mock().expectOneCall("available").onObject(&Serial).andReturnValue(0);
    mock().expectOneCall("serial9_tx_complete").andReturnValue(true);

    s9->loop();

    mock().checkExpectations();
}

TEST(Serial9, serial9_available_escape_8bit_data)
{
//  GIVEN: Idle system with available data on serial9
//  WHEN:  A character with bit9 low is available on serial9
//         and it is the ESCAPE character
//  THEN:  The ESCAPE byte is written to the Serial object, twice

    mock().expectOneCall("serial9_rx_available").andReturnValue(true);
    mock().expectOneCall("serial9_read").andReturnValue(0xff);
    mock().expectOneCall("write").onObject(&Serial).withParameter("c", 0xff).andReturnValue(0x01);
    mock().expectOneCall("write").onObject(&Serial).withParameter("c", 0xff).andReturnValue(0x01);

    s9->loop();

//  GIVEN: One or more bytes have been written to the Serial object
//  WHEN:  No characters are available from Serial
//  THEN:  Nothing else happens

    mock().expectOneCall("serial9_rx_available").andReturnValue(false);
    mock().expectOneCall("serial9_tx_busy").andReturnValue(false);
    mock().expectOneCall("available").onObject(&Serial).andReturnValue(0);
    mock().expectOneCall("serial9_tx_complete").andReturnValue(true);

    s9->loop();

    mock().checkExpectations();
}

TEST(Serial9, serial9_available_9bit_data)
{
//  GIVEN: Idle system with available data on serial9
//  WHEN:  A character with bit9 high is available on serial9
//  THEN:  The ESCAPE byte is written to the Serial object
//         The SERIAL9_HIGH command is written to the Serial object
//         The lower 8 bits of the character are written to the Serial object

    mock().expectOneCall("serial9_rx_available").andReturnValue(true);
    mock().expectOneCall("serial9_read").andReturnValue(0x01aa);
    mock().expectOneCall("write").onObject(&Serial).withParameter("c", 0xff).andReturnValue(0x01);
    mock().expectOneCall("write").onObject(&Serial).withParameter("c", 0x01).andReturnValue(0x01);
    mock().expectOneCall("write").onObject(&Serial).withParameter("c", 0xaa).andReturnValue(0x01);

    s9->loop();

//  GIVEN: One or more bytes have been written to the Serial object
//  WHEN:  No additional characters are available from Serial
//  THEN:  Nothing else happens

    mock().expectOneCall("serial9_rx_available").andReturnValue(false);
    mock().expectOneCall("serial9_tx_busy").andReturnValue(false);
    mock().expectOneCall("available").onObject(&Serial).andReturnValue(0);
    mock().expectOneCall("serial9_tx_complete").andReturnValue(true);

    s9->loop();

    mock().checkExpectations();
}

TEST(Serial9, serial_available_non_escape_data)
{
//  GIVEN: Idle system with available data on Serial
//  WHEN:  A non-ESCAPE character is received from Serial
//  THEN:  The character is written to the serial9 object

    mock().expectOneCall("serial9_rx_available").andReturnValue(false);
    mock().expectOneCall("serial9_tx_busy").andReturnValue(false);
    mock().expectOneCall("available").onObject(&Serial).andReturnValue(1);
    mock().expectOneCall("read").onObject(&Serial).andReturnValue(0xaa);

//  THEN:  The serial9 object is placed into talk mode (half duplex)
//         The character is written to the serial9 object

    mock().expectOneCall("serial9_talk");
    mock().expectOneCall("serial9_write").withParameter("data", 0xaa);

    s9->loop();

//  GIVEN: A character has been written to the serial9 object
//  WHEN:  The loop is executed and the serial9 transmitter is busy
//  THEN:  Nothing happens

    mock().expectOneCall("serial9_rx_available").andReturnValue(false);
    mock().expectOneCall("serial9_tx_busy").andReturnValue(true);

    s9->loop();

//  GIVEN: One or more bytes have been written to the Serial object
//  WHEN:  No additional characters are available from Serial
//         The loop is executed and the serial9 transmitter is not busy
//  THEN:  The serial9 object is placed into listen mode (half duplex)

    mock().expectOneCall("serial9_rx_available").andReturnValue(false);
    mock().expectOneCall("serial9_tx_busy").andReturnValue(false);
    mock().expectOneCall("available").onObject(&Serial).andReturnValue(0);
    mock().expectOneCall("serial9_tx_complete").andReturnValue(true);
    mock().expectOneCall("serial9_listen");

    s9->loop();

    mock().checkExpectations();
}

TEST(Serial9, serial_available_escaped_9bit_data)
{
//  GIVEN: Idle system with available data on Serial
//  WHEN:  An ESCAPE character is received from Serial
//  THEN:  ...

    mock().expectOneCall("serial9_rx_available").andReturnValue(false);
    mock().expectOneCall("serial9_tx_busy").andReturnValue(false);
    mock().expectOneCall("available").onObject(&Serial).andReturnValue(1);
    mock().expectOneCall("read").onObject(&Serial).andReturnValue(0xff);

//  THEN:  The serial9 object is placed into talk mode (half duplex)

    mock().expectOneCall("serial9_talk");

    s9->loop();

//  GIVEN: An ESCAPE character has been recieved from Serial
//  WHEN:  A SERIAL9_HIGH character is received from Serial
//  THEN:  Nothing happens until the next character is read

    mock().expectOneCall("serial9_rx_available").andReturnValue(false);
    mock().expectOneCall("serial9_tx_busy").andReturnValue(false);
    mock().expectOneCall("available").onObject(&Serial).andReturnValue(1);
    mock().expectOneCall("read").onObject(&Serial).andReturnValue(0x01);

    s9->loop();

//  GIVEN: SERIAL9_HIGH character is received from Serial
//  WHEN:  Any other character is received from Serial
//  THEN:  The character is sent to serial9 with the 9th bit set

    mock().expectOneCall("serial9_rx_available").andReturnValue(false);
    mock().expectOneCall("serial9_tx_busy").andReturnValue(false);
    mock().expectOneCall("available").onObject(&Serial).andReturnValue(1);
    mock().expectOneCall("read").onObject(&Serial).andReturnValue(0xaa);
    mock().expectOneCall("serial9_write").withParameter("data", 0x01aa);

    s9->loop();

//  GIVEN: A character has been written to the serial9 object
//  WHEN:  The loop is executed and the serial9 transmitter is busy
//  THEN:  Nothing happens

    mock().expectOneCall("serial9_rx_available").andReturnValue(false);
    mock().expectOneCall("serial9_tx_busy").andReturnValue(true);

    s9->loop();

//  GIVEN: One or more bytes have been written to the Serial object
//  WHEN:  No additional characters are available from Serial
//         The loop is executed and the serial9 transmitter is not busy
//  THEN:  The serial9 object is placed into listen mode (half duplex)

    mock().expectOneCall("serial9_rx_available").andReturnValue(false);
    mock().expectOneCall("serial9_tx_busy").andReturnValue(false);
    mock().expectOneCall("available").onObject(&Serial).andReturnValue(0);
    mock().expectOneCall("serial9_tx_complete").andReturnValue(true);
    mock().expectOneCall("serial9_listen");

    s9->loop();

    mock().checkExpectations();
}

TEST(Serial9, serial_available_escaped_escape_data)
{
//  GIVEN: Idle system with available data on Serial
//  WHEN:  An ESCAPE character is received from Serial
//  THEN:  ...

    mock().expectOneCall("serial9_rx_available").andReturnValue(false);
    mock().expectOneCall("serial9_tx_busy").andReturnValue(false);
    mock().expectOneCall("available").onObject(&Serial).andReturnValue(1);
    mock().expectOneCall("read").onObject(&Serial).andReturnValue(0xff);

//  THEN:  The serial9 object is placed into talk mode (half duplex)

    mock().expectOneCall("serial9_talk");

    s9->loop();

//  GIVEN: An ESCAPE character has been recieved from Serial
//  WHEN:  An ESCAPE character is received from Serial
//  THEN:  The ESCAPE character is sent to serial9 with the 9th bit clear

    mock().expectOneCall("serial9_rx_available").andReturnValue(false);
    mock().expectOneCall("serial9_tx_busy").andReturnValue(false);
    mock().expectOneCall("available").onObject(&Serial).andReturnValue(1);
    mock().expectOneCall("read").onObject(&Serial).andReturnValue(0xff);
    mock().expectOneCall("serial9_write").withParameter("data", 0x00ff);

    s9->loop();

//  GIVEN: One or more bytes have been written to the Serial object
//  WHEN:  No additional characters are available from Serial
//         The loop is executed and the serial9 transmitter is not busy
//  THEN:  The serial9 object is placed into listen mode (half duplex)

    mock().expectOneCall("serial9_rx_available").andReturnValue(false);
    mock().expectOneCall("serial9_tx_busy").andReturnValue(false);
    mock().expectOneCall("available").onObject(&Serial).andReturnValue(0);
    mock().expectOneCall("serial9_tx_complete").andReturnValue(true);
    mock().expectOneCall("serial9_listen");

    s9->loop();

    mock().checkExpectations();
}

TEST(Serial9, serial_available_escaped_set_baud_data)
{
    struct baud_test_t {
        unsigned char baud_char;
        uint32_t baud_rate;
    } baud_test[] = { {0x10,    300}
                    , {0x11,    600}
                    , {0x12,   1200}
                    , {0x13,   2400}
                    , {0x14,   4800}
                    , {0x15,   9600}
                    , {0x16,  19200}
                    , {0x17,  38400}
                    , {0x18,  57600}
                    , {0x19, 115200}
                    };

    int i;

    for (i=0; i<(sizeof(baud_test)/sizeof(baud_test_t)); ++i) {

        //  GIVEN: Idle system with available data on Serial
        //  WHEN:  An ESCAPE character is received from Serial
        //  THEN:  ...

        mock().expectOneCall("serial9_rx_available").andReturnValue(false);
        mock().expectOneCall("serial9_tx_busy").andReturnValue(false);
        mock().expectOneCall("available").onObject(&Serial).andReturnValue(1);
        mock().expectOneCall("read").onObject(&Serial).andReturnValue(0xff);
    
        //  THEN:  The serial9 object is placed into talk mode (half duplex)
    
        mock().expectOneCall("serial9_talk");
    
        s9->loop();
    
        //  GIVEN: An ESCAPE character has been recieved from Serial
        //  WHEN:  A SET_BAUD character is received from Serial
        //  THEN:  The baud rate is updated
    
        mock().expectOneCall("serial9_rx_available").andReturnValue(false);
        mock().expectOneCall("serial9_tx_busy").andReturnValue(false);
        mock().expectOneCall("available").onObject(&Serial).andReturnValue(1);
        mock().expectOneCall("read").onObject(&Serial).andReturnValue(baud_test[i].baud_char);
        mock().expectOneCall("serial9_set_baud").withParameter("baud", baud_test[i].baud_rate);

        s9->loop();

        //  GIVEN: The baud rate has been changed
        //  WHEN:  No additional characters are available from Serial
        //         The loop is executed and the serial9 transmitter is not busy
        //  THEN:  The serial9 object is placed into listen mode (half duplex)
    
        mock().expectOneCall("serial9_rx_available").andReturnValue(false);
        mock().expectOneCall("serial9_tx_busy").andReturnValue(false);
        mock().expectOneCall("available").onObject(&Serial).andReturnValue(0);
        mock().expectOneCall("serial9_tx_complete").andReturnValue(true);
        mock().expectOneCall("serial9_listen");
    
        s9->loop();

        mock().checkExpectations();
    }
}

TEST(Serial9, serial_available_escaped_unknown)
{
    //  GIVEN: Idle system with available data on Serial
    //  WHEN:  An ESCAPE character is received from Serial
    //  THEN:  ...
    mock().expectOneCall("serial9_rx_available").andReturnValue(false);
    mock().expectOneCall("serial9_tx_busy").andReturnValue(false);
    mock().expectOneCall("available").onObject(&Serial).andReturnValue(1);
    mock().expectOneCall("read").onObject(&Serial).andReturnValue(0xff);

    //  THEN:  The serial9 object is placed into talk mode (half duplex)

    mock().expectOneCall("serial9_talk");

    s9->loop();

    //  GIVEN: An ESCAPE character has been recieved from Serial
    //  WHEN:  An UNKNOWN character is received from Serial
    //  THEN:  nothing happens

    mock().expectOneCall("serial9_rx_available").andReturnValue(false);
    mock().expectOneCall("serial9_tx_busy").andReturnValue(false);
    mock().expectOneCall("available").onObject(&Serial).andReturnValue(1);
    mock().expectOneCall("read").onObject(&Serial).andReturnValue(0xaa);

    s9->loop();

    //  GIVEN: Nothing has happened
    //  WHEN:  No additional characters are available from Serial
    //         The loop is executed and the serial9 transmitter is not busy
    //  THEN:  The serial9 object is placed into listen mode (half duplex)

    mock().expectOneCall("serial9_rx_available").andReturnValue(false);
    mock().expectOneCall("serial9_tx_busy").andReturnValue(false);
    mock().expectOneCall("available").onObject(&Serial).andReturnValue(0);
    mock().expectOneCall("serial9_tx_complete").andReturnValue(true);
    mock().expectOneCall("serial9_listen");

    s9->loop();
    mock().checkExpectations();
}

// NOTE: DO we need a test to see what happens if we get a regular or escaped character after
//       a baud rate change? The reason is that we put the system in talk mode, and expect
//       the next charaters(s) to be written, or if no writing is happening that the
//       sytstem goes back in listen mode (it does)
