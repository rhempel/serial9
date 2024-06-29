// This just provides an isolated definition for the Arduino
// Serial class that can be used for mocking.

class MockSerial
{
  public:
    size_t write(unsigned char c);
    uint16_t read(void);
    unsigned int available(void);
};

// There is a single instance of MockSerial called Serial somewhere ...

extern MockSerial Serial;