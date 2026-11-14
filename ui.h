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
std::string render_table(const json& state, const std::deque<std::string>& log,
                         const std::string& notice, bool color);

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

#endif
