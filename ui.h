#ifndef _UI_H_
#define _UI_H_

#include <deque>
#include <string>
#include "third_party/json.hpp"

using json = nlohmann::json;

// Pure functions: no I/O, so they can be unit tested.

// "Td" -> "10♦" (red when color is on). Unknown codes are returned as-is.
std::string card_text(const std::string& code, bool color);

// Columns a string takes in a terminal: ANSI color codes count 0,
// CJK characters count 2.
int display_width(const std::string& s);
std::string pad_right(const std::string& s, int width);

// The whole screen for one state message.
// `chat`: the last lines people said at the table ("Ann: nice hand").
std::string render_table(const json& state, const std::deque<std::string>& log,
                         const std::string& notice, bool color,
                         const std::deque<std::string>& chat = std::deque<std::string>());

// A chat message ("chat" from the server) as one line, "Ann: nice hand",
// with anything that could move the cursor or change colours taken out.
std::string chat_line(const json& msg);

struct command_result{
  json message;        // to send to the server (null if nothing to send)
  std::string error;   // shown to the player instead
  bool quit = false;
  bool help = false;
  bool review = false; // show / hide the last hand
};

// The last finished hand ("hand_summary" message), street by street.
std::string render_hand_summary(const json& summary, const std::string& notice, bool color);

// Turn what the player typed into a server message, checking it against the
// actions the last state allowed. The server checks again anyway.
command_result parse_command(const std::string& line, const json& state);

std::string help_text();

// ---- The lobby: picking a table before sitting down.

// What the player typed at the table list.
struct lobby_choice{
  int table = -1;             // sit down here (-1 = nothing chosen)
  bool needs_password = false;
  json create;                // a create_table message to send (null if none)
  std::string password;       // the new table's password, to sit down with
  std::string error;          // shown instead
  bool quit = false;
};

// "" = the main table, "3" = table 3, "new Friday 2/5 pw=sesame" opens a table
// (blinds 1/2, 2/5 or 5/10; pw= makes it private), "q" quits.
lobby_choice parse_lobby_choice(const std::string& line, const json& lobby, const std::string& name);

// The list of tables, for a player who has not sat down yet.
std::string render_lobby(const json& lobby, const std::string& name, const std::string& notice, bool color);

#endif
