/* ---------------------------------------------------------------------------
  serial9.h - support for 9 bit serial data using the physical UART

  Based on https://github.com/WestfW/Duino-hacks/tree/master/Serial9_test

  Data is transferred to the host over USB using escaped 8 bit data.

    ESC 01  - Next byte should be sent with bit 9 high
    ESC ESC - Next byte to send is ESC
    ESC xxx - All other combinations are illegal - ignore
    dd      - Next byte to send with bit 9 low

  This is implemented as a trivial state machine.

  Based on https://gist.github.com/benVolatiles/c0aa455d4ace0761537818efbaba40ff

  Uses the minimal rign_buffer library.
 */

#ifndef SERIAL9_H
#define SERIAL9_H

#ifndef SERIAL9_BUFFER_SIZE
  #define SERIAL9_BUFFER_SIZE (32)
#endif

class Serial9 // : public Stream
{
  private:
    bool _writing;

    enum serial9_state_e { SERIAL9_STATE_IDLE,
                           SERIAL9_STATE_ESCAPE,
                           SERIAL9_STATE_HIGH, };

    enum serial9_state_e tx_state;

  public:
    Serial9();
    ~Serial9();

    void begin(uint32_t baud);
    void end(void);
    void loop(void);
};

#endif // SERIAL9_H
