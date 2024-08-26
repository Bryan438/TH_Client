#ifndef _OBSERVER_H_
#define _OBSERVER_H_

#include "message_content.h"

class observer{
  private:
  public:
    observer();
    virtual void handle_message(message_content* msg) = 0;
    virtual void handle_input(char* input) = 0;
};

#endif

