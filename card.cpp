#include <iostream>
#include <string.h>
#include "card.h"

Card::Card(card_suit s, int n){
  suit = s;
  number = n;
}

void Card::show(){
  const char* str_suit = NULL;
  switch(suit){
    case Diamond:
      str_suit = "Diamond";
      break;
    case Heart:
      str_suit = "Heart";
      break;
    case Club:
      str_suit = "Club";
      break;
    case Spade:
      str_suit = "Spade";
      break;
    default:
      str_suit = "Unknown";
      break;
  }
  printf("suit : %d\n str_suit : %s\n number : %d\n", suit, str_suit, number);
}

card_suit Card::get_suit(){
  return suit;
}

int Card::get_number(){
  return number;
}

