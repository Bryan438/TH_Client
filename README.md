# Texas Hold'em Client

[![CI](https://github.com/Bryan438/TH_Client/actions/workflows/ci.yml/badge.svg)](https://github.com/Bryan438/TH_Client/actions/workflows/ci.yml)

Terminal client for the [Texas Hold'em server](https://github.com/Bryan438/Texas_Holdem_1).
Shows the whole table and updates after every action: board, pot, every player's
stack, bet and status, your cards, your best hand, whose turn it is, and a game log.
The turn timer and the next-hand countdown tick down live, and whatever you are typing
stays on the input line when the screen redraws.

## Build & Run
Needs a C++17 compiler on macOS or Linux (on Windows use WSL). On a Mac: `xcode-select --install`.
```bash
make
./thclient <server ip or host> [port] [name] [table]
./thclient 192.168.1.20                # asks for your name, then shows the tables
./thclient 192.168.1.20 4000 Bryan     # sits down at the main table at once
./thclient 192.168.1.20 4000 Bryan 3   # ...or at table 3
```

## Picking a table
Without a name on the command line, the client asks for one and then lists the server's
tables: number, name, blinds, starting chips, how many are seated, whether a hand is
running, the host, and `[password]` for a private table.

| Type | Does |
|---|---|
| a table's number | sit down there (a private table asks for its password) |
| Enter | sit down at the main table (table 1) |
| `new Friday 2/5 pw=owl` | open a table and sit down as its host: name, blinds 1/2, 2/5 or 5/10 (default 1/2), `pw=` makes it private |
| `q` | quit |

`leave` at a table brings you back to this list. Needs a server with the lobby
([Texas_Holdem_1](https://github.com/Bryan438/Texas_Holdem_1) PR 20 or later); an older
server has only its one table, and the client sits there as before.

## Commands
| Type | Does |
|---|---|
| `s` | start the next hand (the host) |
| `f` | fold |
| `k` | check |
| `c` | call |
| `r 20` | raise to 20 (total bet this round) |
| `a` | all-in |
| `h` | show the last hand street by street (`h` again to go back) |
| `bot` | the host adds a bot that the server plays (`bot equity 2`: two equity bots; styles rl, equity, rules, calling, random; default one rl bot) |
| `kick NAME` (or `remove NAME`) | the host sends someone away: a bot, or a person (who folds if in a hand) |
| `rebuy` | out of chips: buy in again for the starting chips (counted in the results) |
| `end` | end the game and show who is up and who is down. Alone with bots it happens at once; with other people it asks them, and happens once everyone agrees |
| `new` | start a new game: everyone back to the starting chips (same rule; no vote after `end`) |
| `yes` / `no` | answer a vote to end or restart (one no drops it) |
| `leave` | get up from the table: a hand you are in is folded. Your chips are kept for this game: joining again with the same name brings them back. You are back at the list of tables |
| `?` | help |
| `q` | quit (your seat is kept for a while, like a dropped connection) |

The first person to sit down is the host, marked `[host]`: only they deal, add bots and
`kick`. When the host leaves, or is away longer than the server's grace period, the next
person takes over. Bots are marked on the table: `[rl bot]` for one added with `bot`, `[bot]` for one that
joined over the network (`thbot`). A player who drops out during a hand shows as
`away 25s`: their cards wait that long for them to come back. These need a server from
[Texas_Holdem_1](https://github.com/Bryan438/Texas_Holdem_1) PR #13 or later (bots: #15).

If you lose the connection, run the client again with the same name to get your seat and chips back.
This works because the server gives each player a secret token, which the client saves in
`~/.th_client_tokens.json`; someone else using your name cannot take your seat.
Editing keys: Backspace, Ctrl-U clears the line, Ctrl-D (on an empty line) or `q` quits.
Set `NO_COLOR=1` to turn colors off.

## Test
```bash
make test
```
Covers card formatting, column widths (including Chinese names), the table screen and its
countdowns, the last-hand view, and turning typed commands into protocol messages. GitHub Actions runs the build and
the tests on Linux and macOS for every push and pull request.

JSON: [nlohmann/json](https://github.com/nlohmann/json) v3.11.3, vendored in `third_party/` (MIT).
