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

static json alone_with_bots(){
  return json::parse(R"({
    "type":"state","stage":"waiting","you":0,"turn":-1,"hand":4,"pot":0,"current_bet":0,
    "dealer":1,"small_blind":-1,"big_blind":-1,"blinds":[1,2],"solo":true,"game_over":false,
    "board":[],
    "seats":[
      {"seat":0,"name":"Bryan","stack":230,"bet":0,"status":"ready","cards":null,"bot":false},
      {"seat":1,"name":"RL 1","stack":170,"bet":0,"status":"ready","cards":null,"bot":true,"house":true,"style":"rl"},
      {"seat":2,"name":"Robo","stack":200,"bet":0,"status":"ready","cards":null,"bot":true}
    ]
  })");
}

TEST(bot_commands_add_and_remove_bots){
  json st = alone_with_bots();
  CHECK(parse_command("bot", st).message == json({{"type", "add_bot"}, {"style", "rl"}, {"count", 1}}));
  CHECK(parse_command("bot equity 2", st).message == json({{"type", "add_bot"}, {"style", "equity"}, {"count", 2}}));
  CHECK(parse_command("BOT 3 Rules", st).message == json({{"type", "add_bot"}, {"style", "rules"}, {"count", 3}}));
  CHECK(parse_command("bot magic", st).error.find("Unknown bot style") == 0);
  CHECK(parse_command("bot 0", st).error == "Add 1 to 11 bots at a time");

  CHECK(parse_command("kick rl 1", st).message == json({{"type", "kick"}, {"seat", 1}}));
  CHECK(parse_command("rm RL 1", st).message["seat"] == 1);
  CHECK(parse_command("remove Robo", st).message == json({{"type", "kick"}, {"seat", 2}}));
  CHECK(parse_command("kick Bryan", st).error == "To get up yourself, type leave");
  CHECK(parse_command("kick Nobody", st).error == "Nobody at the table is called \"Nobody\"");
  CHECK(parse_command("kick", st).error.find("Remove whom") == 0);
  CHECK(parse_command("rebuy", st).message == json({{"type", "rebuy"}}));
}

TEST(only_the_host_deals_adds_bots_and_kicks){
  json st = alone_with_bots();
  st["host"] = 0;
  CHECK(parse_command("s", st).message == json({{"type", "start"}}));
  st["host"] = 2;                               // someone else runs the table
  CHECK(parse_command("s", st).error == "Only the host (Robo) can do that");
  CHECK(parse_command("bot", st).error == "Only the host (Robo) can do that");
  CHECK(parse_command("kick RL 1", st).error == "Only the host (Robo) can do that");
  CHECK(parse_command("rebuy", st).message == json({{"type", "rebuy"}}));
  std::deque<std::string> events;
  std::string out = render_table(st, events, "", false);
  CHECK(out.find("Robo [bot] [host]") != std::string::npos);
  CHECK(out.find("Waiting for Robo, the host, to deal") != std::string::npos);
  CHECK(out.find("Type s to start") == std::string::npos);
  st["seats"][0]["stack"] = 0;
  out = render_table(st, events, "", false);
  CHECK(out.find("type rebuy to buy in again") != std::string::npos);
}

TEST(end_new_vote_and_leave_commands){
  json st = alone_with_bots();
  CHECK(parse_command("end", st).message == json({{"type", "end_game"}}));
  CHECK(parse_command("new", st).message == json({{"type", "new_game"}}));
  st["solo"] = false;                           // other people: the server puts it to a vote
  CHECK(parse_command("end", st).message == json({{"type", "end_game"}}));
  CHECK(parse_command("restart", st).message == json({{"type", "new_game"}}));
  CHECK(parse_command("yes", st).error == "There is no vote to answer");
  st["vote"] = {{"kind", "end"}, {"by", "Ann"}, {"agreed", {"Ann"}}, {"waiting", {"Bryan"}}, {"seconds_left", 42}};
  CHECK(parse_command("yes", st).message == json({{"type", "vote"}, {"agree", true}}));
  CHECK(parse_command("n", st).message == json({{"type", "vote"}, {"agree", false}}));
  CHECK(parse_command("leave", st).message == json({{"type", "leave"}}));
  st["game_over"] = true;
  CHECK(parse_command("new", st).message == json({{"type", "new_game"}}));
  CHECK(parse_command("end", st).error.find("already over") != std::string::npos);
}

TEST(render_shows_an_open_vote){
  json st = alone_with_bots();
  st["solo"] = false;
  st["vote"] = {{"kind", "new"}, {"by", "Ann"}, {"agreed", {"Ann"}}, {"waiting", {"Bryan"}}, {"seconds_left", 42}};
  std::deque<std::string> events;
  std::string out = render_table(st, events, "", false);
  CHECK(out.find("Vote: Ann wants to start over (42s). Waiting for Bryan. Type yes or no") != std::string::npos);
  CHECK(out.find("Only bots with you") == std::string::npos);
}

TEST(render_marks_bots_and_shows_how_to_add_them){
  json st = alone_with_bots();
  std::string screen = render_table(st, {}, "", false);
  CHECK(screen.find("RL 1 [rl bot] D") != std::string::npos);
  CHECK(screen.find("Robo [bot]") != std::string::npos);
  CHECK(screen.find("Bryan (you)") != std::string::npos && screen.find("Bryan (you) [") == std::string::npos);
  CHECK(screen.find("end = end the game") != std::string::npos);
  CHECK(screen.find("to add a bot") == std::string::npos);    // three players ready: no need
  st["seats"].erase(1);
  st["seats"].erase(1);
  CHECK(render_table(st, {}, "", false).find("Type bot to add a bot") != std::string::npos);
}

TEST(render_shows_standings_after_the_game_ends){
  json st = alone_with_bots();
  st["game_over"] = true;
  st["standings"] = json::parse(R"([
    {"seat":0,"name":"Bryan","stack":230,"net":30,"bot":false},
    {"seat":2,"name":"Robo","stack":200,"net":0,"bot":true},
    {"seat":1,"name":"RL 1","stack":170,"net":-30,"bot":true}
  ])");
  std::string screen = render_table(st, {}, "", false);
  CHECK(screen.find("GAME OVER") != std::string::npos);
  CHECK(screen.find("Results") != std::string::npos);
  CHECK(screen.find("Bryan (you)") < screen.find("RL 1  "));
  CHECK(screen.find("+30") != std::string::npos && screen.find("-30") != std::string::npos);
  CHECK(screen.find("Type new to start a new game") != std::string::npos);
}

TEST(render_shows_a_player_who_dropped_out_mid_hand){
  json st = my_turn_state();
  st["seats"][0]["status"] = "disconnected";
  st["seats"][0]["reconnect_in"] = 22;
  CHECK(render_table(st, {}, "", false).find("away 22s") != std::string::npos);
  st["seats"][0].erase("reconnect_in");
  CHECK(render_table(st, {}, "", false).find("offline") != std::string::npos);
}

TEST(hand_summary_shows_a_returned_bet){
  json sum = sample_summary();
  sum["returned"] = json::parse(R"([{"seat":0,"name":"Alice","amount":40}])");
  CHECK(render_hand_summary(sum, "", false).find("Uncalled bet of 40 returned to Alice") != std::string::npos);
}

static json two_tables(){
  return json::parse(R"({"type":"lobby","max_tables":20,"can_create":true,"tables":[
    {"id":1,"name":"Main table","main":true,"private":false,"blinds":[1,2],"chips":200,"seats":3,"people":2,"max":12,"playing":true,"host":"Ann"},
    {"id":4,"name":"Friday","main":false,"private":true,"blinds":[2,5],"chips":500,"seats":1,"people":1,"max":12,"playing":false,"host":"Bob"}]})");
}

TEST(picking_a_table_from_the_lobby){
  json lobby = two_tables();
  CHECK_EQ(parse_lobby_choice("", lobby, "Cat").table, 1);         // Enter: the main table
  lobby_choice main = parse_lobby_choice("1", lobby, "Cat");
  CHECK(main.table == 1 && !main.needs_password);
  lobby_choice friday = parse_lobby_choice(" 4 ", lobby, "Cat");
  CHECK(friday.table == 4 && friday.needs_password);
  CHECK(parse_lobby_choice("7", lobby, "Cat").error.find("no table 7") != std::string::npos);
  CHECK(parse_lobby_choice("q", lobby, "Cat").quit);
  CHECK(!parse_lobby_choice("hello", lobby, "Cat").error.empty());

  lobby_choice opened = parse_lobby_choice("new Late night 5/10 pw=owl", lobby, "Cat");
  CHECK(opened.create == json({{"type", "create_table"}, {"name", "Late night"}, {"blinds", {5, 10}}, {"password", "owl"}}));
  CHECK(opened.password == "owl");
  CHECK(parse_lobby_choice("new", lobby, "Cat").create == json({{"type", "create_table"}, {"name", "Cat's table"}}));
}

TEST(render_lobby_lists_the_tables){
  std::string out = render_lobby(two_tables(), "Cat", "", false);
  CHECK(out.find("Main table") != std::string::npos);
  CHECK(out.find("2/5") != std::string::npos);
  CHECK(out.find("500 chips") != std::string::npos);
  CHECK(out.find("1/12 seated") != std::string::npos);
  CHECK(out.find("host Bob  [password]") != std::string::npos);
  CHECK(out.find("playing") != std::string::npos);
  CHECK(out.find("Hello Cat") != std::string::npos);
  CHECK(render_lobby(json(), "Cat", "", false).find("Waiting for the list") != std::string::npos);
}

TEST(render_names_the_table){
  json st = alone_with_bots();
  std::deque<std::string> events;
  CHECK(render_table(st, events, "", false).find("Friday") == std::string::npos);
  st["table"] = {{"id", 4}, {"name", "Friday"}};
  CHECK(render_table(st, events, "", false).find("=== Texas Hold'em ===  Friday") != std::string::npos);
}
