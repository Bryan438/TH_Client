#include <algorithm>
#include <cctype>
#include <sstream>
#include "ui.h"

static const char* RED = "\033[31m";
static const char* BOLD = "\033[1m";
static const char* DIM = "\033[2m";
static const char* GREEN = "\033[32m";
static const char* YELLOW = "\033[33m";
static const char* RESET = "\033[0m";

static std::string paint(const std::string& text, const char* code, bool color){
  return color ? std::string(code) + text + RESET : text;
}

std::string card_text(const std::string& code, bool color){
  if(code.size() != 2){
    return code;
  }
  std::string rank = code[0] == 'T' ? "10" : std::string(1, code[0]);
  switch(code[1]){
    case 's': return rank + "♠";
    case 'c': return rank + "♣";
    case 'h': return paint(rank + "♥", RED, color);
    case 'd': return paint(rank + "♦", RED, color);
    default: return code;
  }
}

int display_width(const std::string& s){
  int width = 0;
  size_t i = 0;
  while(i < s.size()){
    unsigned char c = s[i];
    if(c == 0x1b){                       // ANSI escape: skip to the final letter
      while(i < s.size() && !std::isalpha((unsigned char)s[i])) i++;
      i++;
      continue;
    }
    unsigned cp = 0;
    int len = 1;
    if(c < 0x80){ cp = c; }
    else if((c >> 5) == 0x6){ cp = c & 0x1f; len = 2; }
    else if((c >> 4) == 0xe){ cp = c & 0x0f; len = 3; }
    else if((c >> 3) == 0x1e){ cp = c & 0x07; len = 4; }
    for(int k = 1; k < len && i + k < s.size(); k++){
      cp = (cp << 6) | (s[i + k] & 0x3f);
    }
    i += len;
    bool wide = (cp >= 0x1100 && cp <= 0x115f) || (cp >= 0x2e80 && cp <= 0xa4cf) ||
                (cp >= 0xac00 && cp <= 0xd7a3) || (cp >= 0xf900 && cp <= 0xfaff) ||
                (cp >= 0xfe30 && cp <= 0xfe4f) || (cp >= 0xff00 && cp <= 0xff60) ||
                (cp >= 0xffe0 && cp <= 0xffe6) || (cp >= 0x1f300 && cp <= 0x1faff);
    width += wide ? 2 : 1;
  }
  return width;
}

std::string pad_right(const std::string& s, int width){
  int w = display_width(s);
  return w >= width ? s : s + std::string(width - w, ' ');
}

static std::string upper(std::string s){
  for(char& c : s) c = std::toupper((unsigned char)c);
  return s;
}

static int get_int(const json& j, const char* key, int fallback = -1){
  return j.contains(key) && j[key].is_number_integer() ? j[key].get<int>() : fallback;
}

static std::string seat_name(const json& state, int seat){
  if(seat < 0 || !state.contains("seats") || seat >= (int)state["seats"].size()){
    return "?";
  }
  return state["seats"][seat]["name"].get<std::string>();
}

std::string render_table(const json& state, const std::deque<std::string>& log,
                         const std::string& notice, bool color){
  std::ostringstream out;
  std::string stage = state.value("stage", "waiting");
  int you = get_int(state, "you");
  int turn = get_int(state, "turn");
  int dealer = get_int(state, "dealer");
  int sb = get_int(state, "small_blind");
  int bb = get_int(state, "big_blind");
  bool waiting = stage == "waiting";

  // Header
  std::string blinds = "";
  if(state.contains("blinds") && state["blinds"].is_array() && state["blinds"].size() == 2){
    blinds = std::to_string(state["blinds"][0].get<int>()) + "/" + std::to_string(state["blinds"][1].get<int>());
  }
  out << paint("=== Texas Hold'em ===", BOLD, color);
  if(get_int(state, "hand", 0) > 0){
    out << "  Hand #" << get_int(state, "hand", 0);
  }
  out << "  " << (waiting ? "WAITING" : upper(stage)) << "  Blinds " << blinds << "\n\n";

  // Board and pot
  out << "  Board: ";
  int shown = 0;
  if(state.contains("board")){
    for(const json& c : state["board"]){
      out << pad_right(card_text(c.get<std::string>(), color), 4);
      shown++;
    }
  }
  for(; shown < 5; shown++){
    out << pad_right(paint("--", DIM, color), 4);
  }
  out << "   Pot: " << paint(std::to_string(get_int(state, "pot", 0)), BOLD, color);
  if(get_int(state, "current_bet", 0) > 0){
    out << "   To call: " << get_int(state, "current_bet", 0);
  }
  out << "\n\n";

  // Seats
  out << "     " << pad_right("Player", 22) << pad_right("Stack", 8) << pad_right("Bet", 6)
      << pad_right("Cards", 10) << "Status\n";
  if(state.contains("seats")){
    for(const json& s : state["seats"]){
      int i = s["seat"].get<int>();
      std::string status = s.value("status", "");
      std::string name = s["name"].get<std::string>();

      std::string tags;
      if(!waiting || dealer == i){
        if(i == dealer) tags += " D";
        if(i == sb) tags += " SB";
        if(i == bb) tags += " BB";
      }
      std::string who = name + (i == you ? " (you)" : "") + tags;

      std::string cards;
      if(s.contains("cards") && s["cards"].is_array()){
        for(const json& c : s["cards"]){
          cards += pad_right(card_text(c.get<std::string>(), color), 4);
        }
      }
      else if(status == "active" || status == "allin"){
        cards = paint("## ##", DIM, color);
      }

      std::string status_text;
      if(status == "allin") status_text = paint("ALL-IN", YELLOW, color);
      else if(status == "folded") status_text = "folded";
      else if(status == "sitting_out") status_text = "next hand";
      else if(status == "disconnected") status_text = "offline";
      else if(status == "busted") status_text = "busted";
      if(s.contains("hand") && s["hand"].is_string()){
        status_text += (status_text.empty() ? "" : " ") + paint(s["hand"].get<std::string>(), GREEN, color);
      }

      int bet = s.value("bet", 0);
      std::string line = std::string(i == turn ? " >> " : "    ") + " " +
                         pad_right(who, 22) + pad_right(std::to_string(s.value("stack", 0)), 8) +
                         pad_right(bet > 0 ? std::to_string(bet) : "", 6) + pad_right(cards, 10) + status_text;
      bool dim = status == "folded" || status == "disconnected" || status == "sitting_out" || status == "busted";
      if(i == turn) out << paint(line, BOLD, color) << "\n";
      else if(dim) out << paint(line, DIM, color) << "\n";
      else out << line << "\n";
    }
  }
  out << "\n";

  // Result of the last hand
  if(state.contains("result") && state["result"].is_array()){
    for(const json& r : state["result"]){
      std::string hand = r.value("hand", "");
      out << "  " << paint(r["name"].get<std::string>() + " wins " + std::to_string(r["amount"].get<int>()) +
                           (hand.empty() ? "" : " with " + hand), GREEN, color) << "\n";
    }
    out << "\n";
  }

  // Recent events
  if(!log.empty()){
    for(const std::string& line : log){
      out << paint("  | " + line, DIM, color) << "\n";
    }
    out << "\n";
  }

  if(state.contains("your_hand") && state["your_hand"].is_string() && !waiting){
    out << "  Your best hand: " << state["your_hand"].get<std::string>() << "\n";
  }

  // What happens next
  std::string timer = "";
  if(state.contains("turn_time_left") && state["turn_time_left"].is_number_integer()){
    timer = " (" + std::to_string(state["turn_time_left"].get<int>()) + "s)";
  }
  if(waiting){
    if(state.contains("next_hand_in") && state["next_hand_in"].is_number_integer()){
      out << "  Next hand in " << paint(std::to_string(state["next_hand_in"].get<int>()) + "s", BOLD, color)
          << " (type " << paint("s", BOLD, color) << " to start now), ? for help, q to quit\n";
    }
    else{
      out << "  Type " << paint("s", BOLD, color) << " to start the next hand, h for the last hand, ? for help, q to quit\n";
    }
  }
  else if(turn == you && state.contains("actions")){
    const json& a = state["actions"];
    std::string opts = "[f]old  ";
    if(a.value("check", false)) opts += "[k] check  ";
    if(a.value("call", 0) > 0) opts += "[c]all " + std::to_string(a["call"].get<int>()) + "  ";
    if(a.contains("raise") && a["raise"].is_object()){
      opts += "[r <" + std::to_string(a["raise"]["min"].get<int>()) + "-" +
              std::to_string(a["raise"]["max"].get<int>()) + ">] raise to  ";
    }
    opts += "[a]ll-in";
    out << paint("  Your turn" + timer + ": ", YELLOW, color) << opts << "\n";
  }
  else{
    out << "  Waiting for " << seat_name(state, turn) << timer << "...\n";
  }

  if(!notice.empty()){
    out << paint("  " + notice, RED, color) << "\n";
  }
  return out.str();
}

std::string help_text(){
  return "Commands: s = start hand, f = fold, k = check, c = call, "
         "r 20 = raise to 20, a = all-in, h = last hand, q = quit";
}

static std::string cards_text(const json& cards, bool color){
  std::string out;
  for(const json& c : cards){
    if(!out.empty()) out += " ";
    out += card_text(c.get<std::string>(), color);
  }
  return out;
}

static std::string action_text(const json& a){
  std::string action = a.value("action", "");
  int amount = a.value("amount", 0);
  std::string text;
  if(action == "small_blind") text = "posts small blind " + std::to_string(amount);
  else if(action == "big_blind") text = "posts big blind " + std::to_string(amount);
  else if(action == "fold") text = "folds";
  else if(action == "check") text = "checks";
  else if(action == "call") text = "calls " + std::to_string(amount);
  else if(action == "bet") text = "bets " + std::to_string(a.value("to", amount));
  else if(action == "raise") text = "raises to " + std::to_string(a.value("to", amount));
  else text = action;
  if(a.value("allin", false)) text += " (all-in)";
  std::string reason = a.value("reason", "");
  if(reason == "timeout") text += " (ran out of time)";
  else if(reason == "disconnected") text += " (disconnected)";
  return a.value("name", "?") + " " + text;
}

std::string render_hand_summary(const json& summary, const std::string& notice, bool color){
  std::ostringstream out;
  int you = get_int(summary, "you");
  std::string blinds;
  if(summary.contains("blinds") && summary["blinds"].size() == 2){
    blinds = std::to_string(summary["blinds"][0].get<int>()) + "/" + std::to_string(summary["blinds"][1].get<int>());
  }
  out << paint("=== Last hand: #" + std::to_string(get_int(summary, "hand", 0)) + " ===", BOLD, color)
      << "  Blinds " << blinds << "  Pot " << get_int(summary, "pot", 0) << "\n\n";

  // Players: start stack and result
  for(const json& p : summary.value("players", json::array())){
    int net = p.value("net", 0);
    std::string result = net > 0 ? paint("+" + std::to_string(net), GREEN, color)
                       : net < 0 ? std::to_string(net) : "0";
    std::string who = p.value("name", "?") + (p.value("seat", -1) == you ? " (you)" : "");
    out << "  " << pad_right(who, 22) << pad_right("start " + std::to_string(p.value("stack", 0)), 12)
        << pad_right(result, 8);
    if(p.contains("cards")){
      out << "  " << cards_text(p["cards"], color);
    }
    out << "\n";
  }
  out << "\n";

  // Streets: board cards as they came, then that street's actions
  const json board = summary.value("board", json::array());
  const json actions = summary.value("actions", json::array());
  const char* streets[] = {"preflop", "flop", "turn", "river"};
  for(int i = 0; i < 4; i++){
    std::string street = streets[i];
    size_t board_needed = i == 0 ? 0 : (size_t)(i + 2);    // flop 3, turn 4, river 5
    bool reached = board.size() >= board_needed;
    bool any = false;
    for(const json& a : actions) any = any || a.value("street", "") == street;
    if(!reached || (!any && i == 0)){
      continue;
    }
    std::string title = street;
    title[0] = std::toupper((unsigned char)title[0]);
    out << "  " << paint(title, BOLD, color);
    if(i == 1) out << "   " << cards_text(json(std::vector<json>(board.begin(), board.begin() + 3)), color);
    if(i >= 2) out << "   " << cards_text(json::array({board[i + 1]}), color);
    out << "\n";
    for(const json& a : actions){
      if(a.value("street", "") == street){
        out << "    " << action_text(a) << "\n";
      }
    }
  }

  const json shown = summary.value("showdown", json::array());
  if(!shown.empty()){
    out << "  " << paint("Showdown", BOLD, color) << "\n";
    for(const json& s : shown){
      out << "    " << s.value("name", "?") << " shows " << cards_text(s["cards"], color)
          << " - " << s.value("hand", "") << "\n";
    }
  }
  out << "  " << paint("Result", BOLD, color) << "\n";
  for(const json& w : summary.value("winners", json::array())){
    out << "    " << paint(w.value("name", "?") + " wins " + std::to_string(w.value("won", 0)), GREEN, color) << "\n";
  }
  out << "\n  Type " << paint("h", BOLD, color) << " to go back to the table\n";
  if(!notice.empty()){
    out << paint("  " + notice, RED, color) << "\n";
  }
  return out.str();
}

command_result parse_command(const std::string& line, const json& state){
  command_result r;
  std::istringstream in(line);
  std::string word;
  in >> word;
  std::transform(word.begin(), word.end(), word.begin(), [](unsigned char c){ return std::tolower(c); });
  if(word.empty()){
    return r;
  }
  if(word == "q" || word == "quit" || word == "exit"){
    r.quit = true;
    return r;
  }
  if(word == "?" || word == "help"){
    r.help = true;
    return r;
  }
  if(word == "h" || word == "history"){
    r.review = true;
    return r;
  }

  std::string stage = state.value("stage", "waiting");
  if(word == "s" || word == "start"){
    if(stage != "waiting"){
      r.error = "A hand is already running";
    }
    else{
      r.message = {{"type", "start"}};
    }
    return r;
  }

  bool is_action = word == "f" || word == "fold" || word == "k" || word == "check" ||
                   word == "c" || word == "call" || word == "r" || word == "raise" || word == "a" || word == "allin";
  if(!is_action){
    r.error = "Unknown command \"" + word + "\". " + help_text();
    return r;
  }
  if(stage == "waiting"){
    r.error = "No hand is running. Type s to start one";
    return r;
  }
  if(get_int(state, "turn") != get_int(state, "you") || !state.contains("actions")){
    r.error = "It is not your turn";
    return r;
  }

  const json& a = state["actions"];
  int call_amount = a.value("call", 0);
  bool can_raise = a.contains("raise") && a["raise"].is_object();
  auto action = [](const std::string& name){ return json{{"type", "action"}, {"action", name}}; };

  if(word == "f" || word == "fold"){
    r.message = action("fold");
  }
  else if(word == "k" || word == "check"){
    if(!a.value("check", false)){
      r.error = "You can't check: " + std::to_string(call_amount) + " to call (c to call)";
    }
    else{
      r.message = action("check");
    }
  }
  else if(word == "c" || word == "call"){
    if(call_amount == 0){
      r.error = "Nothing to call (k to check)";
    }
    else{
      r.message = action("call");
    }
  }
  else if(word == "a" || word == "allin"){
    if(can_raise){
      r.message = action("raise");
      r.message["amount"] = a["raise"]["max"].get<int>();
    }
    else if(call_amount > 0){
      r.message = action("call");   // calling uses the whole stack
    }
    else{
      r.message = action("check");
    }
  }
  else{
    if(!can_raise){
      r.error = "You can't raise, only call or fold";
      return r;
    }
    int lo = a["raise"]["min"].get<int>();
    int hi = a["raise"]["max"].get<int>();
    long amount = 0;
    std::string rest;
    if(!(in >> rest) || rest.find_first_not_of("0123456789") != std::string::npos || rest.size() > 9){
      r.error = "Raise to how much? e.g. r " + std::to_string(lo) + " (from " + std::to_string(lo) +
                " to " + std::to_string(hi) + ")";
      return r;
    }
    amount = std::stol(rest);
    if(amount < lo || amount > hi){
      r.error = "Raise must be to between " + std::to_string(lo) + " and " + std::to_string(hi);
      return r;
    }
    r.message = action("raise");
    r.message["amount"] = (int)amount;
  }
  return r;
}
