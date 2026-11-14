#include <cstdio>
#include <string>
#include "test_framework.h"
#include "ui.h"

static json my_turn_state(){
  return json::parse(R"({
    "type":"state","stage":"flop","you":1,"turn":1,"hand":3,"pot":42,"current_bet":20,
    "dealer":0,"small_blind":1,"big_blind":2,"blinds":[1,2],"turn_time_left":25,
    "board":["7d","3d","Ks"],
    "seats":[
      {"seat":0,"name":"Alice","stack":180,"bet":20,"status":"active","cards":null},
      {"seat":1,"name":"Bob","stack":190,"bet":10,"status":"active","cards":["Ad","Td"]},
      {"seat":2,"name":"小明","stack":198,"bet":0,"status":"folded","cards":null}
    ],
    "your_hand":"High card",
    "actions":{"fold":true,"check":false,"call":10,"raise":{"min":40,"max":200}}
  })");
}

TEST(card_text_formats_rank_and_suit){
  CHECK(card_text("Ad", false) == "A♦");
  CHECK(card_text("Ts", false) == "10♠");
  CHECK(card_text("2c", false) == "2♣");
  CHECK(card_text("Kh", true) == "\033[31mK♥\033[0m");
  CHECK(card_text("??", false) == "??");
}

TEST(display_width_handles_color_and_cjk){
  CHECK_EQ(display_width("Bob"), 3);
  CHECK_EQ(display_width("\033[31mK♥\033[0m"), 2);
  CHECK_EQ(display_width("小明"), 4);
  CHECK(pad_right("小明", 6) == "小明  ");
}

TEST(render_shows_board_pot_seats_and_turn){
  std::deque<std::string> log = {"Alice raises to 20", "Flop: 7♦ 3♦ K♠"};
  std::string screen = render_table(my_turn_state(), log, "", false);
  CHECK(screen.find("Hand #3") != std::string::npos);
  CHECK(screen.find("FLOP") != std::string::npos);
  CHECK(screen.find("7♦") != std::string::npos);
  CHECK(screen.find("Pot: 42") != std::string::npos);
  CHECK(screen.find("Bob (you) SB") != std::string::npos);
  CHECK(screen.find("A♦  10♦") != std::string::npos);  // own cards
  CHECK(screen.find("## ##") != std::string::npos);              // Alice's hidden cards
  CHECK(screen.find("folded") != std::string::npos);
  CHECK(screen.find("Alice raises to 20") != std::string::npos);
  CHECK(screen.find("Your turn (25s)") != std::string::npos);
  CHECK(screen.find("[c]all 10") != std::string::npos);
  CHECK(screen.find("[r <40-200>]") != std::string::npos);
  CHECK(screen.find("Your best hand: High card") != std::string::npos);
}

TEST(parse_command_builds_actions){
  json st = my_turn_state();
  CHECK(parse_command("f", st).message == json({{"type", "action"}, {"action", "fold"}}));
  CHECK(parse_command("c", st).message["action"] == "call");
  command_result raise = parse_command("r 60", st);
  CHECK(raise.message["action"] == "raise");
  CHECK_EQ(raise.message["amount"].get<int>(), 60);
  command_result allin = parse_command("a", st);
  CHECK_EQ(allin.message["amount"].get<int>(), 200);
  CHECK(parse_command("q", st).quit);
  CHECK(parse_command("?", st).help);
  CHECK(parse_command("", st).message.is_null());
}

TEST(parse_command_rejects_bad_input_locally){
  json st = my_turn_state();
  CHECK(parse_command("k", st).error.find("can't check") != std::string::npos);
  CHECK(parse_command("r", st).error.find("Raise to how much") != std::string::npos);
  CHECK(parse_command("r 30", st).error == "Raise must be to between 40 and 200");
  CHECK(parse_command("r abc", st).message.is_null());
  CHECK(parse_command("s", st).error == "A hand is already running");
  CHECK(parse_command("xyz", st).error.find("Unknown command") == 0);

  st["turn"] = 0;
  CHECK(parse_command("c", st).error == "It is not your turn");
  st["stage"] = "waiting";
  CHECK(parse_command("c", st).error.find("No hand is running") == 0);
  CHECK(parse_command("s", st).message == json({{"type", "start"}}));
}

TEST(render_shows_next_hand_countdown){
  json st = my_turn_state();
  st["stage"] = "waiting";
  st["turn"] = -1;
  st.erase("actions");
  st["next_hand_in"] = 4;
  CHECK(render_table(st, {}, "", false).find("Next hand in 4s (type s to start now)") != std::string::npos);
  st["next_hand_in"] = nullptr;
  CHECK(render_table(st, {}, "", false).find("Type s to start the next hand") != std::string::npos);
}

static json sample_summary(){
  return json::parse(R"({
    "type":"hand_summary","hand":3,"you":1,"blinds":[1,2],"pot":26,"dealer":0,
    "board":["7d","3d","Ks","2c","Ah"],
    "players":[
      {"seat":0,"name":"Alice","stack":100,"end_stack":114,"net":14},
      {"seat":1,"name":"Bob","stack":100,"end_stack":88,"net":-12,"cards":["Qd","Jd"]},
      {"seat":2,"name":"Cat","stack":100,"end_stack":98,"net":-2}
    ],
    "actions":[
      {"street":"preflop","seat":1,"name":"Bob","action":"small_blind","amount":1},
      {"street":"preflop","seat":2,"name":"Cat","action":"big_blind","amount":2},
      {"street":"preflop","seat":0,"name":"Alice","action":"raise","amount":6,"to":6},
      {"street":"preflop","seat":1,"name":"Bob","action":"call","amount":5},
      {"street":"preflop","seat":2,"name":"Cat","action":"fold","amount":0,"reason":"timeout"},
      {"street":"flop","seat":1,"name":"Bob","action":"check","amount":0},
      {"street":"flop","seat":0,"name":"Alice","action":"bet","amount":6,"to":6},
      {"street":"flop","seat":1,"name":"Bob","action":"call","amount":6,"allin":true}
    ],
    "showdown":[
      {"seat":0,"name":"Alice","cards":["Ac","Kd"],"hand":"Two pair"},
      {"seat":1,"name":"Bob","cards":["Qd","Jd"],"hand":"High card"}
    ],
    "winners":[{"seat":0,"name":"Alice","won":26}]
  })");
}

TEST(hand_summary_shows_streets_actions_and_result){
  std::string s = render_hand_summary(sample_summary(), "", false);
  CHECK(s.find("Last hand: #3") != std::string::npos);
  CHECK(s.find("Bob (you)") != std::string::npos);
  CHECK(s.find("Q♦ J♦") != std::string::npos);               // own cards
  CHECK(s.find("+14") != std::string::npos);
  CHECK(s.find("Bob posts small blind 1") != std::string::npos);
  CHECK(s.find("Alice raises to 6") != std::string::npos);
  CHECK(s.find("Cat folds (ran out of time)") != std::string::npos);
  CHECK(s.find("Flop   7♦ 3♦ K♠") != std::string::npos);
  CHECK(s.find("Turn   2♣") != std::string::npos);
  CHECK(s.find("River   A♥") != std::string::npos);
  CHECK(s.find("Bob calls 6 (all-in)") != std::string::npos);
  CHECK(s.find("Alice shows A♣ K♦ - Two pair") != std::string::npos);
  CHECK(s.find("Alice wins 26") != std::string::npos);
  CHECK(s.find("Preflop") < s.find("Flop"));
}

TEST(hand_summary_skips_streets_not_reached){
  json summary = sample_summary();
  summary["board"] = json::array();
  summary["showdown"] = json::array();
  std::string s = render_hand_summary(summary, "", false);
  CHECK(s.find("Flop") == std::string::npos);
  CHECK(s.find("Showdown") == std::string::npos);
}

TEST(parse_command_h_is_history_not_check){
  json st = my_turn_state();
  CHECK(parse_command("h", st).review);
  CHECK(parse_command("h", st).message.is_null());
  st["stage"] = "waiting";
  CHECK(parse_command("history", st).review);
}
