#ifndef _CARD_H_
#define _CARD_H_

enum card_suit{
  Diamond = 1,
  Spade = 2,
  Heart = 3,
  Club = 4
};

class Card{
  private:
    int number;
    card_suit suit;
  public:
    Card(card_suit suit, int number);
    Card(){};
    void show();
    void set_number(int number);
    card_suit get_suit();
    int get_number();
};

#endif

