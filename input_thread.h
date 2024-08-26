#ifndef _INPUT_THREAD_H_
#define _INPUT_THREAD_H_

#include "observer.h"

class input_thread{
  private:
    input_thread(){};
    static input_thread* instance;
    observer* p_listener = NULL;
  public:
    static input_thread* get_instance();
    ~input_thread(){};
    void start();
    void set_listener(observer* ob);
    void read();
};

#endif
