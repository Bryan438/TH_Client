#include "message_content.h"
#include <string.h>
#include <stdlib.h>

message_content::message_content(){
  charmessage = NULL;
}

message_content::~message_content(){
  if(charmessage != NULL){
    delete charmessage;
  }
} 

void message_content::set_charmessage(char* msg, int len){
  if(charmessage != NULL){
    delete charmessage;
  }
  length = len;
  charmessage = new char[length+1];
  memcpy(charmessage, msg, len);
  charmessage[len+1] = 0;
}   

void message_content::set_intmessage(int msg){
  intmessage = msg;
}   

void message_content::set_command(int cmd){
  command = cmd;
} 

int message_content::get_length(){
  return length;
}

char* message_content::get_charmessage(){
  return charmessage;
}

int message_content::get_intmessage(){
  return intmessage;
} 

int message_content::get_command(){
  return command;
}


