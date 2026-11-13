# Texas Hold'em Client

Terminal client for the [Texas Hold'em server](https://github.com/Bryan438/Texas_Holdem_1).
Shows the whole table and updates after every action: board, pot, every player's
stack, bet and status, your cards, your best hand, whose turn it is, and a game log.

## Build & Run
Needs a C++17 compiler on macOS or Linux (on Windows use WSL). On a Mac: `xcode-select --install`.
```bash
make
./thclient <server ip or host> [port] [name]
./thclient 192.168.1.20 4000 Bryan
```

## Commands
| Type | Does |
|---|---|
| `s` | start the next hand |
| `f` | fold |
| `k` | check |
| `c` | call |
| `r 20` | raise to 20 (total bet this round) |
| `a` | all-in |
| `?` | help |
| `q` | quit |

If you lose the connection, run the client again with the same name to get your seat and chips back.
Set `NO_COLOR=1` to turn colors off.

## Test
```bash
make test
```
Covers card formatting, column widths (including Chinese names), the table screen,
and turning typed commands into protocol messages.

JSON: [nlohmann/json](https://github.com/nlohmann/json) v3.11.3, vendored in `third_party/` (MIT).
