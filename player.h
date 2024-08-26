#ifndef _PLAYER_H_
#define _PLAYER_H_

#include "card.h"

enum player_decision{
  CALL = 0x01,
  RAISE = 0x02,
  FOLD = 0x04,
  CHECK = 0x08
};

class player{
  private:
    const char* player_name;
    int player_num;
    int remaining_money;

    Card* card1;
    Card* card2;
  public:
    player();

    Card* get_first_card();
    Card* get_second_card();

    void set_initial_card(Card* card1, Card* card2);
    void set_name(char* name);
    void set_bet(int money);

    int get_money();
    int get_current_round_bet();
    
};

#endif
