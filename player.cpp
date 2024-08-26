#include <iostream>
#include <string.h>
#include <stdlib.h>
#include "card.h"
#include "player.h"

player::player(){
  card1 = NULL;
  card2 = NULL;
}
  
void player::set_name(char* name){
  player_name = name;
}

void player::set_bet(int money){
  remaining_money = money;
}

Card* player::get_first_card(){
  return card1;
}

Card* player::get_second_card(){
  return card2;
}

int player::get_money(){
  return remaining_money;
}

void player::set_initial_card(Card* c1, Card* c2){
  card1 = c1; 
  card2 = c2;
}
