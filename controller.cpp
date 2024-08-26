#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>


#include "card.h"
#include "player.h"
#include "cli_transport.h"
#include "message_content.h"
#include "observer.h"
#include "controller.h"
#include "input_thread.h"

int my_strlen(char* str){
  int i = 0;
  if(!str){
    return -1;
  }
  while(str[i] != 0){
    i++;
  }
  return i;
}

controller::controller(){
  player1 = new player();
}

void controller::start(){
  int decision;
  char player_name[50];
  char player_decision[10];
  int bet; 

  memset(player_name, 0, 50);
  memset(player_decision, 0, 50);
  cli_transport::get_instance()->set_listener(this);
  cli_transport::get_instance()->start_client();

  input_thread::get_instance()->set_listener(this);
  input_thread::get_instance()->start();
}

void controller::handle_input(char* input){
  printf("input : %s\n", input);
  switch(current_state){
    case INIT:
      switch(input[0]){
        case '4':
          printf("Enter name \n");
          current_state = INIT_WAITING_NAME;
          break;
        case '6':
          current_state = START;
          printf("Game start \n");
          cli_transport::get_instance()->serialize(7, 0, 0);
          break;
        default:
          break;
      }
      break;
    case INIT_WAITING_NAME:
      cli_transport::get_instance()->serialize(4, my_strlen(input), input);
      current_state = INIT;
      break;

    case INIT_WAITING_BET:
      //TODO
      current_state = INIT;
      break;

    case START:
      break;

    case PREFLOP_WAITING_ACTION:
      {
        int valid_status = -1;
        char decision = -1;

        valid_status = check_valid_input(decision_condition[0], input[0]);

        switch(valid_status){
          case -1:
            printf("Invalid input\n");
            break;
          case RAISE:
            current_state = PREFLOP_WAITING_RAISE;

            printf("New bet\n");
            memcpy(result_list + 1, input, 1);
            break;
          default:
            current_state = PREFLOP;

            memcpy(result_list + 1, input, 1);
            cli_transport::get_instance()->serialize(10, 6, result_list); 
            break;
        } 
      }
      break;

    case PREFLOP_WAITING_RAISE:
      {
        //Get raise input
        int new_amount = atoi(input);
        printf("New amount = %d\n", new_amount);
        if(new_amount < h_low || new_amount > h_high){
          printf("Wrong Input\n");
          break;
        }

        current_state = PREFLOP;

        int n_new_amount = htonl(new_amount);
        memcpy(result_list + 2, &n_new_amount, 4);
        cli_transport::get_instance()->serialize(10, 6, result_list); 
      }
      break;

    default:
      break;
  }
}

void controller::handle_message(message_content* message){

  switch(current_state){
    case INIT:
    case INIT_WAITING_NAME:
    case INIT_WAITING_BET:
    case START:
      {
        switch(message->get_command()){
          case 6:
            {
              current_state = PREFLOP;

              char cardmsg[10];
              int num1 = 0;
              int num2 = 0;
              memset(cardmsg, 0, 10);
              memcpy(cardmsg, message->get_charmessage(), 10);

              memcpy(&num1, cardmsg, 4);
              memcpy(&num2, cardmsg + 5, 4);

              int h_num1 = ntohl(num1);
              int h_num2 = ntohl(num2);

              Card* card1 = new Card((card_suit)cardmsg[4], h_num1);
              Card* card2 = new Card((card_suit)cardmsg[9], h_num2);
              card1->show();
              card2->show();
              player1->set_initial_card(card1, card2);
            }
            break;
          default:
            break;
        }
      }
      break;
    case PREFLOP:
      {
        switch(message->get_command()){
          case 8:
            {
              current_state = PREFLOP_WAITING_ACTION;

              memset(decision_condition, 0, 9);
              memcpy(decision_condition, message->get_charmessage(), 9);
              int high = 0;
              int low = 0;
              memcpy(&high, decision_condition + 1, 4);
              h_high = ntohl(high);
              memcpy(&low, decision_condition + 5, 4);
              h_low = ntohl(low);

              memset(result_list, 0, 6);
              memcpy(result_list, decision_condition, 1);

              if(decision_condition[0] & FOLD){
                printf(" [F]old ");
              } 
              if(decision_condition[0] & RAISE){
                printf(" [R]aise ");
              } 
              if(decision_condition[0] & CALL){
                printf(" [C]all ");
              } 
              if(decision_condition[0] & CHECK){
                printf(" c[H]eck ");
              }

              printf("\n");

            }
            break;
          case 3:
            {
              current_state = FLOP;
              cli_transport::get_instance()->serialize(1, 0, 0); 
            }
          default:
            break;
        }
      }
      break;

    case FLOP:
      {
      }
      break;

    default:
      break;
  }

  switch(message->get_command()){
    case 2:
      {
        printf("%s\n", message->get_charmessage());
        break;
      }
    case 4:
      break;
    case 5:
      break;
    case 8:
      break;
    default:
      break;
  } 
}

int controller::check_valid_input(int combined, char input){
  int decision = 0;
  switch(input){
    case 'F':
    case 'f':
      decision = FOLD;
      break;
    case 'R':
    case 'r':
      decision = RAISE;
      break;
    case 'c':
    case 'C':
      decision = CALL;
      break;
    case 'H':
    case 'h':
      decision = CHECK;
      break;
    default:
      break;
  }
  return decision & combined ? decision : -1;
}


