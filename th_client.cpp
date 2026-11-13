#include <cstdio>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <mutex>
#include <string>
#include <unistd.h>
#include "net.h"
#include "ui.h"

// Usage: ./thclient <server ip or host> [port] [name]
namespace {

std::mutex lock;
json state;                       // last full table state from the server
std::deque<std::string> events;   // recent events, newest last
std::string notice;               // error / hint shown at the bottom
int my_seat = -1;
bool color = true;
const size_t MAX_EVENTS = 8;

// Caller holds `lock`.
void redraw(){
  std::string screen;
  if(state.is_null()){
    screen = "=== Texas Hold'em ===\n\n";
    if(!notice.empty()) screen += "  " + notice + "\n";
    if(my_seat < 0) screen += "Your name: ";
    std::cout << screen << std::flush;
    return;
  }
  std::cout << "\033[2J\033[H" << render_table(state, events, notice, color) << "> " << std::flush;
}

void on_line(const std::string& line){
  json msg = json::parse(line, nullptr, false);
  if(msg.is_discarded() || !msg.is_object()){
    return;
  }
  std::string type = msg.value("type", "");
  std::lock_guard<std::mutex> guard(lock);
  if(type == "welcome"){
    my_seat = msg.value("seat", -1);
    notice = "";
  }
  else if(type == "state"){
    // A hint about an earlier moment (e.g. "not your turn") is stale once the
    // game has moved on. Errors arrive just before the state they belong to,
    // so keep the notice if hand, stage and turn are unchanged.
    auto key = [](const json& s){
      return s.is_null() ? json() : json::array({s.value("hand", 0), s.value("stage", ""), s.value("turn", -1)});
    };
    if(key(msg) != key(state)){
      notice = "";
    }
    state = msg;
    redraw();
  }
  else if(type == "event"){
    events.push_back(msg.value("text", ""));
    while(events.size() > MAX_EVENTS) events.pop_front();
  }
  else if(type == "error"){
    notice = msg.value("message", "error");
    if(my_seat < 0) notice += ". Try another name.";
    redraw();
  }
}

}

int main(int argc, char* argv[]){
  if(argc < 2){
    printf("Usage: %s <server ip or host> [port] [name]\n", argv[0]);
    return 1;
  }
  std::string host = argv[1];
  int port = argc > 2 ? atoi(argv[2]) : 4000;
  color = isatty(STDOUT_FILENO) && getenv("NO_COLOR") == NULL;

  connection conn;
  std::string error;
  if(!conn.open(host, port, error)){
    printf("%s\n", error.c_str());
    return 1;
  }
  conn.start_reader(on_line, []{
    std::lock_guard<std::mutex> guard(lock);
    std::cout << "\nDisconnected from server.\n" << std::flush;
    _exit(0);
  });

  if(argc > 3){
    conn.send_line(json({{"type", "join"}, {"name", argv[3]}}).dump());
  }
  else{
    std::lock_guard<std::mutex> guard(lock);
    redraw();
  }

  std::string line;
  while(std::getline(std::cin, line)){
    json to_send;
    {
      std::lock_guard<std::mutex> guard(lock);
      if(my_seat < 0){
        // Not seated yet: whatever was typed is the name.
        if(!line.empty()){
          to_send = {{"type", "join"}, {"name", line}};
        }
      }
      else{
        command_result r = parse_command(line, state);
        if(r.quit){
          break;
        }
        notice = r.help ? help_text() : r.error;
        to_send = r.message;
        if(to_send.is_null()){
          redraw();
        }
      }
    }
    if(!to_send.is_null() && !conn.send_line(to_send.dump())){
      break;
    }
  }
  return 0;
}
