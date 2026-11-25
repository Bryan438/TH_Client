#include <algorithm>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <termios.h>
#include <thread>
#include <unistd.h>
#include "net.h"
#include "ui.h"

// Usage: ./thclient <server ip or host> [port] [name] [table]
namespace {

std::mutex lock;
json state;                       // last full table state from the server
std::chrono::steady_clock::time_point state_time;  // when it arrived
std::deque<std::string> events;   // recent events, newest last
std::string notice;               // error / hint shown at the bottom
std::string input;                // what the player is typing (raw mode only)
json last_summary;                // last finished hand, for "h"
bool reviewing = false;           // showing last_summary instead of the table
int my_seat = -1;
bool color = true;
bool interactive = false;         // stdin is a terminal and we manage the input line
std::string server_key;           // "host:port", for saved tokens
const size_t MAX_EVENTS = 8;

// Before sitting down: the name, the list of tables, the table being joined.
std::string my_name;
json lobby_info;                  // last "lobby" message
int want_table = 1;               // the table joined (1 = the main one)
int password_for = -1;            // asking for this private table's password
std::string new_table_password;   // a table we opened: sit down there with this
bool creating = false;
connection* server = nullptr;

// ------------------------------------------------------------ reconnect tokens
// The server gives every new player a secret token. It is saved here so that
// after a crash or a dropped connection, running the client again with the
// same name takes the old seat (and chips) back.

std::string token_file(){
  const char* home = getenv("HOME");
  return std::string(home ? home : ".") + "/.th_client_tokens.json";
}

json load_tokens(){
  std::ifstream in(token_file());
  json tokens = json::parse(in, nullptr, false);
  return tokens.is_object() ? tokens : json::object();
}

// A seat's token is per table; the main table keeps the older key, so a seat
// taken before there were tables is still found.
std::string token_key(int table, const std::string& name){
  return table == 1 ? server_key + ":" + name : server_key + ":" + std::to_string(table) + ":" + name;
}

void save_token(int table, const std::string& name, const std::string& token){
  json tokens = load_tokens();
  tokens[token_key(table, name)] = token;
  std::ofstream out(token_file());
  out << tokens.dump(2) << "\n";
}

json join_message(const std::string& name, int table, const std::string& password = ""){
  json msg = {{"type", "join"}, {"name", name}, {"table", table}};
  if(!password.empty()) msg["password"] = password;
  json tokens = load_tokens();
  std::string key = token_key(table, name);
  if(tokens.contains(key) && tokens[key].is_string()){
    msg["token"] = tokens[key];
  }
  return msg;
}

// ------------------------------------------------------------ terminal

termios original_termios;
bool raw_mode = false;

void restore_terminal(){
  if(raw_mode){
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios);
    raw_mode = false;
  }
}

void on_signal(int){
  restore_terminal();
  _exit(130);
}

// Read keys one at a time without echo, so the screen can be redrawn at any
// moment and the half-typed command is simply drawn again.
void enable_raw_mode(){
  if(tcgetattr(STDIN_FILENO, &original_termios) != 0){
    return;
  }
  termios t = original_termios;
  t.c_lflag &= ~(ICANON | ECHO);
  t.c_cc[VMIN] = 1;
  t.c_cc[VTIME] = 0;
  if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &t) == 0){
    raw_mode = true;
    atexit(restore_terminal);
    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);
  }
}

// ------------------------------------------------------------ drawing

// The state with its countdowns moved forward by the time since it arrived.
json current_view(){
  json view = state;
  long elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                   std::chrono::steady_clock::now() - state_time).count();
  for(const char* key : {"turn_time_left", "next_hand_in"}){
    if(view.contains(key) && view[key].is_number_integer()){
      view[key] = std::max(0L, view[key].get<long>() - elapsed);
    }
  }
  return view;
}

std::string prompt(){
  if(my_seat >= 0) return "> ";
  if(my_name.empty()) return "Your name: ";
  return password_for >= 0 ? "Password for table " + std::to_string(password_for) + ": " : "Table: ";
}

// Caller holds `lock`.
void redraw(){
  std::string screen = "\033[2J\033[H";
  if(state.is_null() && !my_name.empty()){
    screen += render_lobby(lobby_info, my_name, notice, color);
  }
  else if(state.is_null()){
    screen += "=== Texas Hold'em ===\n\n";
    if(!notice.empty()) screen += "  " + notice + "\n";
  }
  else if(reviewing && !last_summary.is_null()){
    screen += render_hand_summary(last_summary, notice, color);
  }
  else{
    screen += render_table(current_view(), events, notice, color);
  }
  std::cout << screen << prompt() << input << std::flush;
}

// Caller holds `lock`. Only the input line changed: no need to clear the screen.
void redraw_input_line(){
  std::cout << "\r\033[K" << prompt() << input << std::flush;
}

bool has_countdown(){
  return !state.is_null() &&
         ((state.contains("turn_time_left") && state["turn_time_left"].is_number_integer()) ||
          (state.contains("next_hand_in") && state["next_hand_in"].is_number_integer()));
}

// ------------------------------------------------------------ messages

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
    if(msg.contains("token") && msg["token"].is_string()){
      save_token(want_table, msg.value("name", ""), msg["token"].get<std::string>());
    }
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
    state_time = std::chrono::steady_clock::now();
    if(msg.value("stage", "waiting") != "waiting"){
      reviewing = false;   // a hand is running: back to the table
    }
    redraw();
  }
  else if(type == "hand_summary"){
    last_summary = msg;
  }
  else if(type == "event"){
    events.push_back(msg.value("text", ""));
    while(events.size() > MAX_EVENTS) events.pop_front();
  }
  else if(type == "left"){
    // Up from the table: by "leave", or the server removed us.
    my_seat = -1;
    state = json();
    reviewing = false;
    std::string by = msg.value("by", std::string(""));
    std::string reason = msg.value("reason", "");
    notice = reason == "closed" ? "That table closed."
           : reason != "kicked" ? "You left the table."
           : by.empty() ? "The server removed you from the table."
                        : by + " removed you from the table.";
    redraw();
  }
  else if(type == "lobby"){
    lobby_info = msg;
    if(my_seat < 0) redraw();
  }
  else if(type == "table_created"){
    // We opened it: sit down there, as its host.
    if(creating && server){
      creating = false;
      want_table = msg.value("table", 1);
      server->send_line(join_message(my_name, want_table, new_table_password).dump());
    }
  }
  else if(type == "error"){
    notice = msg.value("message", "error");
    if(my_seat < 0){
      creating = false;
      password_for = -1;
      // A name the table will not take: ask for another one.
      if(msg.value("code", "").rfind("name_", 0) == 0){
        my_name.clear();
        notice += ". Try another name.";
      }
    }
    redraw();
  }
}

// Handle one finished line. Returns false to quit.
bool handle_line(const std::string& line, connection& conn){
  json to_send;
  {
    std::lock_guard<std::mutex> guard(lock);
    if(my_seat < 0 && my_name.empty()){
      // First the name...
      if(!line.empty()){
        my_name = line;
        notice.clear();
      }
    }
    else if(my_seat < 0 && password_for >= 0){
      // ...a private table's password...
      want_table = password_for;
      password_for = -1;
      to_send = join_message(my_name, want_table, line);
    }
    else if(my_seat < 0){
      // ...then a table from the list.
      lobby_choice c = parse_lobby_choice(line, lobby_info, my_name);
      if(c.quit){
        return false;
      }
      notice = c.error;
      if(!c.create.is_null()){
        creating = true;
        new_table_password = c.password;
        to_send = c.create;
      }
      else if(c.table >= 0 && c.needs_password){
        password_for = c.table;
      }
      else if(c.table >= 0){
        want_table = c.table;
        to_send = join_message(my_name, want_table);
      }
    }
    else{
      command_result r = parse_command(line, state);
      if(r.quit){
        return false;
      }
      notice = r.help ? help_text() : r.error;
      to_send = r.message;
      if(r.review){
        if(last_summary.is_null()){
          notice = "No hand has finished yet";
        }
        else{
          reviewing = !reviewing;
        }
      }
      else if(!to_send.is_null()){
        reviewing = false;
      }
    }
    redraw();
  }
  return to_send.is_null() || conn.send_line(to_send.dump());
}

// Remove the last UTF-8 character (a Chinese character is 3 bytes).
void erase_last_char(std::string& s){
  while(!s.empty()){
    unsigned char c = s.back();
    s.pop_back();
    if((c & 0xC0) != 0x80){
      break;
    }
  }
}

void read_keys(connection& conn){
  char c;
  while(read(STDIN_FILENO, &c, 1) == 1){
    if(c == 0x1b){                            // arrow keys etc.: skip "ESC [ X"
      char seq;
      if(read(STDIN_FILENO, &seq, 1) == 1 && seq == '['){
        if(read(STDIN_FILENO, &seq, 1) != 1) return;
      }
      continue;
    }
    std::string line;
    bool submit = false;
    {
      std::lock_guard<std::mutex> guard(lock);
      if(c == '\n' || c == '\r'){
        line = input;
        input.clear();
        submit = true;
      }
      else if(c == 0x7f || c == 0x08){        // backspace
        erase_last_char(input);
      }
      else if(c == 0x15){                     // Ctrl-U: clear the line
        input.clear();
      }
      else if(c == 0x04){                     // Ctrl-D: quit on an empty line
        if(input.empty()) return;
      }
      else if((unsigned char)c >= 0x20 && input.size() < 200){
        input += c;
      }
      if(!submit){
        redraw_input_line();
      }
    }
    if(submit && !handle_line(line, conn)){
      return;
    }
  }
}

}

int main(int argc, char* argv[]){
  if(argc < 2){
    printf("Usage: %s <server ip or host> [port] [name] [table]\n"
           "  With a name, sits down at once: at the main table, or at the table given.\n"
           "  Without one, asks for it and shows the tables to pick from.\n", argv[0]);
    return 1;
  }
  std::string host = argv[1];
  int port = argc > 2 ? atoi(argv[2]) : 4000;
  server_key = host + ":" + std::to_string(port);
  color = isatty(STDOUT_FILENO) && getenv("NO_COLOR") == NULL;
  interactive = isatty(STDIN_FILENO) && isatty(STDOUT_FILENO);

  connection conn;
  std::string error;
  if(!conn.open(host, port, error)){
    printf("%s\n", error.c_str());
    return 1;
  }
  server = &conn;
  conn.start_reader(on_line, []{
    std::lock_guard<std::mutex> guard(lock);
    restore_terminal();
    std::cout << "\nDisconnected from server.\n" << std::flush;
    _exit(0);
  });

  if(interactive){
    enable_raw_mode();
  }

  {
    std::lock_guard<std::mutex> guard(lock);
    if(argc > 3){
      my_name = argv[3];
      want_table = argc > 4 ? std::max(1, atoi(argv[4])) : 1;
      conn.send_line(join_message(my_name, want_table).dump());
    }
    else{
      redraw();
    }
  }

  // Redraw every second while a countdown is running.
  std::thread([]{
    while(true){
      std::this_thread::sleep_for(std::chrono::seconds(1));
      std::lock_guard<std::mutex> guard(lock);
      if(has_countdown()){
        redraw();
      }
    }
  }).detach();

  if(raw_mode){
    read_keys(conn);
  }
  else{
    // Not a terminal (e.g. input piped from a script): plain lines.
    std::string line;
    while(std::getline(std::cin, line) && handle_line(line, conn)){
    }
  }

  {
    std::lock_guard<std::mutex> guard(lock);
    restore_terminal();
    std::cout << "\n" << std::flush;
  }
  _exit(0);  // the reader and timer threads are still running
}
