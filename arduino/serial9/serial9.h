/* ---------------------------------------------------------------------------
  serial9.h - support for 9 bit serial data using the physical UART

  See README and LICENCE for more information
 */

#ifndef SERIAL9_H
#define SERIAL9_H

enum serial9_state_e { SERIAL9_STATE_IDLE,
                       SERIAL9_STATE_ESCAPE,
                       SERIAL9_STATE_HIGH,
                     };

class Serial9 // : public Stream
{
  private:
    bool _writing;

    enum serial9_state_e tx_state;

  public:
    Serial9();
    ~Serial9();

    void begin(uint32_t baud);
    void end(void);
    void loop(void);
};

// This macro is used to provide code coverage for empty cases
// or conditional clauses - when we are compiling normally
// it evaluates to nothing 

#ifdef GCOV
  #define DO_NOTHING { int do_nothing = 0; }
#else
  #define DO_NOTHING
#endif 

#endif // SERIAL9_H
