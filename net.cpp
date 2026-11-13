#include <cstring>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <thread>
#include <vector>
#include "net.h"
#include "line_buffer.h"

connection::~connection(){
  if(sock >= 0){
    close(sock);
  }
}

bool connection::open(const std::string& host, int port, std::string& error){
  addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  addrinfo* found = nullptr;
  int rc = getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &found);
  if(rc != 0){
    error = std::string("cannot resolve ") + host + ": " + gai_strerror(rc);
    return false;
  }
  for(addrinfo* a = found; a != nullptr; a = a->ai_next){
    int s = socket(a->ai_family, a->ai_socktype, a->ai_protocol);
    if(s < 0){
      continue;
    }
    if(connect(s, a->ai_addr, a->ai_addrlen) == 0){
      sock = s;
      break;
    }
    close(s);
  }
  freeaddrinfo(found);
  if(sock < 0){
    error = "cannot connect to " + host + ":" + std::to_string(port) + " (is the server running?)";
    return false;
  }
  return true;
}

void connection::start_reader(std::function<void(const std::string&)> on_line, std::function<void()> on_close){
  int s = sock;
  std::thread([s, on_line, on_close]{
    line_buffer buffer;
    char chunk[4096];
    std::vector<std::string> lines;
    while(true){
      int n = recv(s, chunk, sizeof(chunk), 0);
      if(n <= 0){
        break;
      }
      lines.clear();
      bool ok = buffer.feed(chunk, n, lines);
      for(const std::string& line : lines){
        on_line(line);
      }
      if(!ok){
        break;
      }
    }
    on_close();
  }).detach();
}

bool connection::send_line(const std::string& line){
  std::string data = line + "\n";
  size_t sent = 0;
  while(sent < data.size()){
    ssize_t n = send(sock, data.data() + sent, data.size() - sent, 0);
    if(n <= 0){
      return false;
    }
    sent += n;
  }
  return true;
}
