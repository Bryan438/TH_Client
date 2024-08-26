#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <cstring>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <thread>

#include "cli_transport.h"
#include "message_content.h"

cli_transport* cli_transport::instance = NULL;

cli_transport* cli_transport::get_instance(){
  if(instance == NULL){
    instance = new cli_transport();
  }
  return instance;
}

cli_transport::cli_transport(){
  client_socket = -1;
}

int cli_transport::start_client(){
  client_socket = socket(AF_INET, SOCK_STREAM, 0); 

  // specifying address 
  sockaddr_in serverAddress; 
  serverAddress.sin_family = AF_INET; 
  serverAddress.sin_port = htons(4000); 
  serverAddress.sin_addr.s_addr = INADDR_ANY; 

  // sending connection request 
  connect(client_socket, (struct sockaddr*)&serverAddress, 
      sizeof(serverAddress)); 

  read_with_thread(client_socket);
  return 0; 
}

void cli_transport::read(int client_socket){
  int n = -1;
  char buf[1000];
  int flag;
  int hasrecvd = 0;

  while(true){
    do{
      n = recv(client_socket, buf + hasrecvd, sizeof(buf) - hasrecvd, 0);
      hasrecvd += n;
      flag = ioctl(client_socket, FIONREAD, &flag);
      printf("recv from server, n = %d, flag = %d\n", n, flag);
    } while(flag > 0);

    if(n <= 0){
      printf("error\n");
      break;
    }
    while(hasrecvd >= 4){
      uint32_t header = ntohl(*((int*) buf));
      header >>= 20;
      uint32_t length = header & 0x0FF;
      header >>= 8;
      uint32_t command = header;

      if(hasrecvd >= length + 4){
        char* base64buf = convert_buffer_to_base64(buf, length + 4);
        printf("recvd buf = %s\n", base64buf);
        delete[] base64buf;
        
        message_content* msg_packet = deserialize(buf);
        p_listener->handle_message(msg_packet);
        hasrecvd -= (length + 4);
        if(hasrecvd > 0){
          memcpy(buf, buf + length + 4, hasrecvd);
        }
      }
    }
  }
}

int cli_transport::send_msg(char* message, int length){
  if(send(client_socket, message, length, 0) == -1){
    printf("Error sending");
    return -1;
  }
  return 1;
}

void cli_transport::set_listener(observer* ob){
  p_listener = ob;
}


void cli_transport::serialize(int command, int length, char* buf){
  char packet[length + 4];
  int operation = 0;
  memset(packet, 0, length+4);

  command <<= 28;
  operation = command | operation;
  int length_cpy = length << 20;
  operation = length_cpy | operation;
  int n_operation = htonl(operation);
  memcpy(packet, &n_operation, 4);
  memcpy(packet+4, buf, length);

  char* message_content = convert_buffer_to_base64(packet, length + 4);
  printf("send message = %s\n", message_content);
  delete[] message_content;

  if(send_msg(packet, length + 4) == -1){
    printf("Error sending");
  }
}

void cli_transport::read_with_thread(int client_socket){
  std::thread thread_read(&cli_transport::read, this, client_socket);
  thread_read.detach();
}

message_content* cli_transport::deserialize(const char* buffer){
  char message[50];
  int intmessage = 0;
  memset(message, 0, 30);

  uint32_t header = ntohl(*((int*) buffer));
  header >>= 20;
  uint32_t length = header & 0x0FF;
  header >>= 8;
  uint32_t command = header;
  memcpy(message, buffer+4, length);
  message_content* packet = new message_content();
  packet->set_command(command);
  if(command%2 == 0){
    packet->set_charmessage(message, length);
  }
  else{
    packet->set_intmessage(intmessage);
  }
  return packet;
}


void cli_transport::disconnect(){
  if(client_socket != -1){
    close(client_socket);
    client_socket = -1;
  }
}

char* cli_transport::convert_buffer_to_base64(char* message, int length){
  char* msg = new char[2*length + 1];
  memset(msg, 0, length*2);
  for(int i = 0; i < length; i++){
    char one_message = message[i];
    char first_four = 0;
    first_four = (one_message >> 4);
    char base64first_four = first_four & 0x0F;
    if(base64first_four > 9){
      msg[2*i] = 'A' + base64first_four - 10;
    }
    else{
      msg[2*i] =  base64first_four + '0';
    }
    char base64last_four  = one_message & 0x0F;
    if(base64last_four > 9){
      msg[2*i+1] = 'A' + base64last_four - 10;
    }
    else{
      msg[2*i+1] =  base64last_four + '0';
    }
  }
  msg[2*length + 1] = 0;
  return msg;
}

