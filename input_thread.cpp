#include <iostream>
#include <iomanip>
#include <string.h>
#include <stdlib.h>
#include <cstring>
#include <thread>

#include "input_thread.h"

input_thread* input_thread::instance = NULL;

input_thread* input_thread::get_instance(){
  if(instance == NULL){
    instance = new input_thread();
  }
  return instance;
}

void input_thread::set_listener(observer* ob){
  p_listener = ob;
}

void input_thread::start(){
  std::thread thread_read(&input_thread::read, this);
  thread_read.join();
}

void input_thread::read(){
  char* input = new char[1000];
  // Stop on EOF instead of spinning on the last input forever.
  while(std::cin >> std::setw(1000) >> input){
    p_listener->handle_input(input); 
  }
  delete[] input;
}
