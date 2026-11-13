#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <cstring>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
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

void cli_transport::set_ip_addr(char* addr){
  memset(ip_addr, 0, 16);
  strncpy(ip_addr, addr, 15);
}

void cli_transport::set_port(int p){
  port = p;
}

int cli_transport::start_client(){
  client_socket = socket(AF_INET, SOCK_STREAM, 0); 

  // specifying address 
  sockaddr_in serverAddress; 
  memset(&serverAddress, 0, sizeof(serverAddress));
  serverAddress.sin_family = AF_INET; 
  serverAddress.sin_port = htons(port); 
  serverAddress.sin_addr.s_addr = inet_addr(ip_addr); 

  // sending connection request 
  if(connect(client_socket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
    printf("Could not connect to %s:%d\n", ip_addr, port);
    exit(1);
  }

  read_with_thread(client_socket);
  return 0; 
}

// TCP is a byte stream: one recv() can return half a frame or several frames,
// so bytes are kept in `buf` until a whole frame is there.
void cli_transport::read(int client_socket){
  char buf[1024];
  int hasrecvd = 0;

  while(true){
    int n = recv(client_socket, buf + hasrecvd, sizeof(buf) - hasrecvd, 0);
    if(n <= 0){
      printf("Disconnected from server\n");
      exit(0);
    }
    hasrecvd += n;

    while(hasrecvd >= 4){
      uint32_t header;
      memcpy(&header, buf, 4);
      header = ntohl(header);
      int length = (header >> 16) & 0xFF;
      int frame_size = length + 4;
      if(hasrecvd < frame_size){
        break; // wait for the rest of this frame
      }

      message_content* msg_packet = deserialize(buf);
      p_listener->handle_message(msg_packet);
      delete msg_packet;

      hasrecvd -= frame_size;
      memmove(buf, buf + frame_size, hasrecvd);
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
  if(length < 0 || length > 255){
    printf("Message too long, not sent\n");
    return;
  }
  char packet[4 + 255];
  uint32_t header = ((uint32_t)(command & 0xFF) << 24) | ((uint32_t)length << 16);
  uint32_t n_header = htonl(header);
  memcpy(packet, &n_header, 4);
  if(length > 0){
    memcpy(packet + 4, buf, length);
  }

  if(send_msg(packet, length + 4) == -1){
    printf("Error sending\n");
  }
}

void cli_transport::read_with_thread(int client_socket){
  std::thread thread_read(&cli_transport::read, this, client_socket);
  thread_read.detach();
}

message_content* cli_transport::deserialize(const char* buffer){
  uint32_t header;
  memcpy(&header, buffer, 4);
  header = ntohl(header);
  int length = (header >> 16) & 0xFF;
  int command = (header >> 24) & 0xFF;

  message_content* packet = new message_content();
  packet->set_command(command);
  // Even commands carry text/bytes, odd commands carry an optional int.
  if(command % 2 == 0){
    packet->set_charmessage((char*)buffer + 4, length);
  }
  else if(length >= 4){
    uint32_t value;
    memcpy(&value, buffer + 4, 4);
    packet->set_intmessage((int)ntohl(value));
  }
  return packet;
}

void cli_transport::disconnect(){
  if(client_socket != -1){
    close(client_socket);
    client_socket = -1;
  }
}

// Despite the name this is a hex dump, used for debug logging.
char* cli_transport::convert_buffer_to_base64(char* message, int length){
  static const char digits[] = "0123456789ABCDEF";
  char* msg = new char[2*length + 1];
  for(int i = 0; i < length; i++){
    unsigned char byte = message[i];
    msg[2*i] = digits[byte >> 4];
    msg[2*i+1] = digits[byte & 0x0F];
  }
  msg[2*length] = 0;
  return msg;
}
