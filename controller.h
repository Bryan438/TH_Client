enum state{
  INIT = 1,
  INIT_WAITING_NAME = 2,
  INIT_WAITING_BET = 3,
  START = 4,

  PREFLOP = 5,
  PREFLOP_WAITING_ACTION = 6, 
  PREFLOP_WAITING_RAISE = 7,

  FLOP = 8,
  FLOP_WAITING_ACTION = 9
};

#include "player.h"
#include "message_content.h"
#include "observer.h"


class controller: public observer{
  private:
    player* player1;
    state current_state = INIT;
    int h_high = 0;
    int h_low = 0;
    char decision_condition[9];
    char result_list[6];
  public:
    controller();
    ~controller(){};
    void start();
    virtual void handle_input(char* input);
    virtual void handle_message(message_content* msg);
    int check_valid_input(int combined, char input);
};
