# content of test_class.py
import pytest

from serial9 import Serial9

class TestDevice():
    __test__ = False

    def __init__(self):
        self._tx_buffer = b""
        self._rx_buffer = b""

    def rx(self):
        d = self._rx_buffer
        self._rx_buffer = b""
        return d

    def tx(self, d):
        self._tx_buffer += d

def test_initialization_with_no_connection():
    # Given: No initial conditions
    # When: A Serial9 instance is created with no connection device
    # Then: The default connection is None
    #       and the initial state is SERIAL9_STATE_IDLE
    #       and the loopback buffer is empty
    #
    s9 = Serial9()

    assert s9._conn == None
    assert s9._rx_state == Serial9.SERIAL9_STATE_IDLE
    assert s9._loopback_buffer == b""

def test_tx8_with_no_connection():
    # Given: No initial conditions
    # When: A Serial9 instance is created with no connection device
    #       and we try to transmit any data as 8 bit
    # Then: The excption is caught
    #       The loopback buffer has the transmitted data
    #
    s9 = Serial9()

    test_string = b"hello"
    result_string = test_string

    s9.tx8(test_string)

    assert s9._loopback_buffer == result_string

def test_tx9_with_no_connection():
    # Given: No initial conditions
    # When: A Serial9 instance is created with no connection device
    #       and we try to transmit any data as 9 bit
    # Then: The excption is caught
    #       The loopback buffer has the transmitted data
    #
    s9 = Serial9()

    test_string = b"hello"
    result_list = [[0xff, 0x01, c] for c in test_string]
    result_string = bytes(c for sublist in result_list for c in sublist)

    s9.tx9(test_string)

    assert s9._loopback_buffer == result_string

def test_rx_with_no_connection():
    # Given: No initial conditions
    # When: A Serial9 instance is created with no connection device
    #       and we try to receive any data
    # Then: The excption is caught
    #       The loopback buffer has the transmitted data
    #
    s9 = Serial9()

    test_list = [[0xff, 0x01, c] for c in b"hello"]
    test_string = bytes(c for sublist in test_list for c in sublist)

    result_list = [0x100 + c for c in b"hello"]

    s9._loopback_buffer = test_string

    assert result_list == s9.rx()

def test_initialization_with_test_device():
    # Given: No initial conditions
    # When: A Serial9 instance is created with a test device
    # Then: The default connection is the test device
    #       and the initial state is SERIAL9_STATE_IDLE
    #       and the loopback buffer is empty
    #
    test_device = TestDevice()

    s9 = Serial9(test_device)

    assert s9._conn == test_device
    assert s9._rx_state == Serial9.SERIAL9_STATE_IDLE
    assert s9._loopback_buffer == b""

def test_tx8_empty_string():
    # Given: Serial9 instance initialized with a TestDevice
    # When: We transmit a null string as 8 bit data
    # Then: The test device data buffer has a null string.
    #
    test_device = TestDevice()
    s9 = Serial9(test_device)

    test_string = bytes([])
    result_string = test_string

    assert b"" == test_device._tx_buffer
    s9.tx8(test_string)
    assert result_string == test_device._tx_buffer

def test_tx8_not_ff_string():
    # Given: Serial9 instance initialized with a TestDevice
    # When: We transmit a string with NO 0xff character as 8 bit data
    # Then: The test device data buffer contains the same string
    #
    test_device = TestDevice()
    s9 = Serial9(test_device)

    test_string = bytes([c for c in range(0,0xff)])
    result_string = test_string

    assert b"" == test_device._tx_buffer
    s9.tx8(test_string)
    assert result_string == test_device._tx_buffer

def test_tx8_single_ff_string():
    # Given: Serial9 instance initialized with a TestDevice
    # When: We transmit a string with a single 0xff character as 8 bit data
    # Then: The test device data buffer contains the same string with each
    #       0xff character replaced by two 0xff characters.
    #
    test_device = TestDevice()
    s9 = Serial9(test_device)

    test_string = bytes([0xff])
    result_string = bytes([0xff, 0xff])

    assert b"" == test_device._tx_buffer
    s9.tx8(test_string)
    assert result_string == test_device._tx_buffer

def test_tx8_leading_ff_string():
    # Given: Serial9 instance initialized with a TestDevice
    # When: We transmit a string with a single 0xff character followed
    #       by many non 0xff characters as 8 bit data
    # Then: The test device data buffer contains the same string with each
    #       0xff character replaced by two 0xff characters.
    #
    test_device = TestDevice()
    s9 = Serial9(test_device)

    test_string = bytes([0xff] + [c for c in range(0,0xff)])
    result_string = bytes([0xff]) + test_string

    assert b"" == test_device._tx_buffer
    s9.tx8(test_string)
    assert result_string == test_device._tx_buffer

def test_tx8_trailing_ff_string():
    # Given: Serial9 instance initialized with a TestDevice
    # When: We transmit a string with many non 0xff characters followed
    #       by a single 0xff character as 8 bit data
    # Then: The test device data buffer contains the same string with each
    #       0xff character replaced by two 0xff characters.
    #
    test_device = TestDevice()
    s9 = Serial9(test_device)

    test_string = bytes([c for c in range(0,0x100)])
    result_string = test_string + bytes([0xff])

    assert b"" == test_device._tx_buffer
    s9.tx8(test_string)
    assert result_string == test_device._tx_buffer

def test_tx8_alternating_ff_string():
    # Given: Serial9 instance initialized with a TestDevice
    # When: We transmit a string with leading, trailing, and every other
    #       character an 0xff as 8 bit data
    # Then: The test device data buffer contains the same string with each
    #       0xff character replaced by two 0xff characters.
    #
    test_device = TestDevice()
    s9 = Serial9(test_device)

    test_string = bytes([0xff, 0x00, 0xff, 0x01, 0xff, 0x11, 0xff,])
    result_string = bytes([0xff, 0xff, 0x00, 0xff, 0xff, 0x01, 0xff, 0xff, 0x11, 0xff, 0xff,])

    assert b"" == test_device._tx_buffer
    s9.tx8(test_string)
    assert result_string == test_device._tx_buffer

def test_tx8_all_ff_string():
    # Given: Serial9 instance initialized with a TestDevice
    # When: We transmit a string with all 0xff as 8 bit data
    # Then: The test device data buffer contains the same string with each
    #       0xff character replaced by two 0xff characters.
    #
    test_device = TestDevice()
    s9 = Serial9(test_device)

    test_string = bytes([0xff, 0xff, 0xff, 0xff,])
    result_string = bytes([0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,])

    assert b"" == test_device._tx_buffer
    s9.tx8(test_string)
    assert result_string == test_device._tx_buffer

def test_tx9_empty_string():
    # Given: Serial9 instance initialized with a TestDevice
    # When: We transmit a null string as 9 bit data
    # Then: The test device data buffer has a null string.
    #
    test_device = TestDevice()
    s9 = Serial9(test_device)

    assert b"" == test_device._tx_buffer
    s9.tx9(b"")
    assert b"" == test_device._tx_buffer

def test_tx9_single_not_ff_string():
    # Given: Serial9 instance initialized with a TestDevice
    # When: We transmit a string with NO 0xff character as 9 bit data
    # Then: The test device data buffer contains the same string with
    #       every byte prefixed by 0xff 0x01
    #
    test_device = TestDevice()
    s9 = Serial9(test_device)

    test_string = bytes([0x80])
    result_string = bytes([0xff, 0x01, 0x80,])

    assert b"" == test_device._tx_buffer
    s9.tx9(test_string)
    assert result_string == test_device._tx_buffer

def test_tx9_multiple_not_ff_string():
    # Given: Serial9 instance initialized with a TestDevice
    # When: We transmit a string with NO 0xff character as 9 bit data
    # Then: The test device data buffer contains the same string with
    #       every byte prefixed by 0xff 0x01
    #
    test_device = TestDevice()
    s9 = Serial9(test_device)

    test_string = bytes([0x00, 0x01, 0x02, 0xfe])
    result_string = bytes([0xff, 0x01, 0x00, 0xff, 0x01, 0x01, 0xff, 0x01, 0x02, 0xff, 0x01, 0xfe])

    assert b"" == test_device._tx_buffer
    s9.tx9(test_string)
    assert result_string == test_device._tx_buffer

def test_tx9_long_not_ff_string():
    # Given: Serial9 instance initialized with a TestDevice
    # When: We transmit a long string with NO 0xff character as 9 bit data
    # Then: The test device data buffer contains the same string with
    #       every byte prefixed by 0xff 0x01
    #
    test_device = TestDevice()
    s9 = Serial9(test_device)

    test_string = bytes([c for c in range(0,255)])
    result_list = [[0xff, 0x01, c] for c in range(0,255)]
    result_string = bytes(c for sublist in result_list for c in sublist)


    assert b"" == test_device._tx_buffer
    s9.tx9(test_string)
    assert result_string == test_device._tx_buffer

def test_tx9_single_ff_string():
    # Given: Serial9 instance initialized with a TestDevice
    # When: We transmit a string with a single 0xff character as 9 bit data
    # Then: The test device data buffer contains the same string with
    #       every byte replaced by 0xff 0x01 0xff 
    #
    test_device = TestDevice()
    s9 = Serial9(test_device)

    test_string = bytes([0xff])
    result_string = bytes([0xff, 0x01, 0xff,])

    assert b"" == test_device._tx_buffer
    s9.tx9(test_string)
    assert result_string == test_device._tx_buffer

def test_tx9_leading_ff_string():
    # Given: Serial9 instance initialized with a TestDevice
    # When: We transmit a string with a single 0xff character followed
    #       by many non 0xff characters as 9 bit data
    # Then: The test device data buffer contains the same string with
    #       every byte replaced by 0xff 0x01 0xff 
    #
    test_device = TestDevice()
    s9 = Serial9(test_device)

    test_string = bytes([0xff, 0x01, 0x02, 0x03,])
    result_string = bytes([0xff, 0x01, 0xff, 0xff, 0x01, 0x01, 0xff, 0x01, 0x02, 0xff, 0x01, 0x03,])

    assert b"" == test_device._tx_buffer
    s9.tx9(test_string)
    assert result_string == test_device._tx_buffer

def test_tx9_trailing_ff_string():
    # Given: Serial9 instance initialized with a TestDevice
    # When: We transmit a string with many non 0xff characters followed
    #       by a single 0xff character as 8 bit data
    # Then: The test device data buffer contains the same string with
    #       every byte replaced by 0xff 0x01 0xff 
    #
    test_device = TestDevice()
    s9 = Serial9(test_device)

    test_string = bytes([0x01, 0x02, 0x03, 0xff,])
    result_string = bytes([0xff, 0x01, 0x01, 0xff, 0x01, 0x02, 0xff, 0x01, 0x03, 0xff, 0x01, 0xff,])

    assert b"" == test_device._tx_buffer
    s9.tx9(test_string)
    assert result_string == test_device._tx_buffer

def test_tx9_alternating_ff_string():
    # Given: Serial9 instance initialized with a TestDevice
    # When: We transmit a string with leading, trailing, and every other
    #       character an 0xff as 9 bit data
    # Then: The test device data buffer contains the same string with
    #       every byte replaced by 0xff 0x01 0xff 
    #
    test_device = TestDevice()
    s9 = Serial9(test_device)

    test_string = bytes([0xff, 0x00, 0xff, 0x01, 0xff, 0x11, 0xff,])
    result_string = bytes([0xff, 0x01, 0xff, 0xff, 0x01, 0x00,
                           0xff, 0x01, 0xff, 0xff, 0x01, 0x01, 
                           0xff, 0x01, 0xff, 0xff, 0x01, 0x11, 
                           0xff, 0x01, 0xff,])

    assert b"" == test_device._tx_buffer
    s9.tx9(test_string)
    assert result_string == test_device._tx_buffer

def test_tx9_all_ff_string():
    # Given: Serial9 instance initialized with a TestDevice
    # When: We transmit a string with all 0xff as 8 bit data
    # Then: The test device data buffer contains the same string with
    #       every byte replaced by 0xff 0x01 0xff 
    #
    test_device = TestDevice()
    s9 = Serial9(test_device)

    test_string = bytes([0xff, 0xff, 0xff, 0xff,])
    result_string = bytes([0xff, 0x01, 0xff, 0xff, 0x01, 0xff,
                           0xff, 0x01, 0xff, 0xff, 0x01, 0xff,])

    assert b"" == test_device._tx_buffer
    s9.tx9(test_string)
    assert result_string == test_device._tx_buffer

def test_rx_empty_string():
    # Given: Serial9 instance initialized with a TestDevice
    # When: The test device has a null string in the buffer
    # Then: We recieve an empty list
    #       and the state is SERIAL9_STATE_IDLE
    #
    test_device = TestDevice()
    s9 = Serial9(test_device)

    test_string = b""
    result_bytes = []

    test_device._rx_buffer = test_string
    assert result_bytes == s9.rx()
    assert s9._rx_state == Serial9.SERIAL9_STATE_IDLE

def test_rx_not_ff_8bit_string():
    # Given: Serial9 instance initialized with a TestDevice
    # When: The test device has a non ff string in the buffer
    # Then: We receive the same string as a list of 8 bit values
    #       and the state is SERIAL9_STATE_IDLE
    #
    test_device = TestDevice()
    s9 = Serial9(test_device)

    test_string = bytes([c for c in range(0,0xff)])
    result_bytes = [c for c in range(0,0xff)]

    test_device._rx_buffer = test_string
    assert result_bytes == s9.rx()
    assert s9._rx_state == Serial9.SERIAL9_STATE_IDLE

def test_rx_single_ff_string():
    # Given: Serial9 instance initialized with a TestDevice
    # When: The test device has just ff string in the buffer
    # Then: We recieve an empty list
    #       and the state is SERIAL9_STATE_ESCAPE
    #
    test_device = TestDevice()
    s9 = Serial9(test_device)

    test_string = bytes([0xff])
    result_bytes = []

    test_device._rx_buffer = test_string
    assert result_bytes == s9.rx()
    assert s9._rx_state == Serial9.SERIAL9_STATE_ESCAPE

def test_rx_single_ff_01_string():
    # Given: Serial9 instance initialized with a TestDevice
    # When: The test device has just ff 01 string in the buffer
    # Then: We recieve an empty list
    #       and the state is SERIAL9_STATE_HIGH
    #
    test_device = TestDevice()
    s9 = Serial9(test_device)

    test_string = bytes([0xff, 0x01])
    result_bytes = []

    test_device._rx_buffer = test_string
    assert result_bytes == s9.rx()
    assert s9._rx_state == Serial9.SERIAL9_STATE_HIGH

def test_rx_single_ff_01_00_string():
    # Given: Serial9 instance initialized with a TestDevice
    # When: The test device has just ff 01 string in the buffer
    # Then: We recieve 0x00 with the 9th bit high
    #       and the state is SERIAL9_STATE_IDLE
    #
    test_device = TestDevice()
    s9 = Serial9(test_device)

    test_string = bytes([0xff, 0x01, 0x00])
    result_bytes = [0x100]

    test_device._rx_buffer = test_string
    assert result_bytes == s9.rx()
    assert s9._rx_state == Serial9.SERIAL9_STATE_IDLE

def test_rx_single_ff_ff_string():
    # Given: Serial9 instance initialized with a TestDevice
    # When: The test device has just ff ff string in the buffer
    # Then: We recieve 0xff
    #       and the state is SERIAL9_STATE_IDLE
    #
    test_device = TestDevice()
    s9 = Serial9(test_device)

    test_string = bytes([0xff, 0xff])
    result_bytes = [0xff]

    test_device._rx_buffer = test_string
    assert result_bytes == s9.rx()
    assert s9._rx_state == Serial9.SERIAL9_STATE_IDLE

def test_rx_single_ff_02_unhandled_escape_string():
    # Given: Serial9 instance initialized with a TestDevice
    # When: The test device has just ff 02 string in the buffer
    # Then: We recieve an empty list
    #       and the state is SERIAL9_STATE_IDLE
    #
    test_device = TestDevice()
    s9 = Serial9(test_device)

    test_string = bytes([0xff, 0x02])
    result_bytes = []

    test_device._rx_buffer = test_string
    assert result_bytes == s9.rx()
    assert s9._rx_state == Serial9.SERIAL9_STATE_IDLE

def test_rx_multi_byte_9_bit_string():
    # Given: Serial9 instance initialized with a TestDevice
    # When: The test device has a multi-byte 9 bit string in the buffer
    # Then: We recieve the bytes with 9th bit high
    #       and the state is SERIAL9_STATE_IDLE
    #
    # Note thos 
    test_device = TestDevice()
    s9 = Serial9(test_device)

    test_string = bytes([0xff, 0x01, 0x00, 0xff, 0x01, 0x055,
                         0xff, 0x01, 0xaa, 0xff, 0x01, 0x0ff,])
    result_bytes = [0x100, 0x155, 0x1aa, 0x1ff,]

    test_device._rx_buffer = test_string
    assert result_bytes == s9.rx()
    assert s9._rx_state == Serial9.SERIAL9_STATE_IDLE

def test_rx_bad_state():
    # Given: Serial9 instance initialized with a TestDevice
    # When: The test device has one or more characters in the buffer
    #       and the rx state is invlaid (for whatever reason)
    # Then: The byte being processed during the bad state is returned
    #       and an error is logged
    #       and the state is SERIAL9_STATE_IDLE
    #
    # Note thos 
    test_device = TestDevice()
    s9 = Serial9(test_device)

    test_string = bytes([0x00, 0x01, 0x02,])
    result_bytes = [0x00, 0x01, 0x02,]

    test_device._rx_buffer = test_string
    s9._rx_state = 0x55

    assert result_bytes == s9.rx()
    assert s9._rx_state == Serial9.SERIAL9_STATE_IDLE

def test_set_baud():
    # Given: Serial9 instance initialized with a TestDevice
    # When: The function is called with ANY integer value
    # Then: The test device data buffer contains a 2 character
    #       string that is 0xff and the integer (mod 256)
    #
    test_device = TestDevice()
    s9 = Serial9(test_device)

    result_string = bytes([0xff, 0x17])

    s9.set_baud(Serial9.SERIAL_9_BAUD_38400)

    assert result_string == test_device._tx_buffer