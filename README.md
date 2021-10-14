# Marchesa Bot (or the boring name of CockatriceTournamentBot)

Marchesa Bot (aka tricebot) is a FOSS to make hosting tournaments on cocaktrice
easier for tournament organisers. It can create games, save replays and has an
integration with [Squire Bot](https://gitalb.com/monarch3). It currently requires
players to use cockatrice beta to see the games that it makes.

## Build status
[![CodeQL - Build and Test (Ubuntu)](https://github.com/djpiper28/CockatriceTournamentBot/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/djpiper28/CockatriceTournamentBot/actions/workflows/codeql-analysis.yml)
[![codecov](https://codecov.io/gh/djpiper28/CockatriceTournamentBot/branch/main/graph/badge.svg?token=EFPY6BDV96)](https://codecov.io/gh/djpiper28/CockatriceTournamentBot)
(Linux only due to pthread and unistd dependancies)

## Features
 - Automatic game creation
 - Automatic replay downloads
 - Ability to kick players
 - Automatic player/deck verification

### Player Deck Verification
This is when the bot can automatically kick players who are not welcome and tell
playeres when they load the wrong deck. Both are entirely optional. The bot gets
sent the list of players and decks from the api client, (i.e:
[Squire Bot](https://github.com/TylerBloom/SquireBot)) and then when a player who
is not meant to be in the game joins they can be kicked automatically by the bot.

## Interfacing with the bot
The bot will host an https server (with self signed keys probably) and apps that
interface with the bot will use that to connect and send commands which will then
get a response (see src/helppage.gen.html for details). A sample implementation is
in python in a file called `tricebot.py`.

Do note empty games are deleted after 30 minutes by this bot.

## Configuring The Bot
The file (config.conf) should be in this format:

```yaml
#Cockatrice username
username=username
#Cockatrice password
password=yourpassword
#Cockatrice server address you must include the port
serveraddress=ws://server.cockatrice.us:4748
#Auth token for the bot
authtoken=nGpzR/KspN6ry7jG8CU4bonN2aujzfJa
#SSL certificate
certfile=fullchain.pem
#SSL certificate key
certkeyfile=privkey.pem
#IP address to bind the API server to, you must include the port
bindAddr=https://127.0.0.1:8000
#Client ID for the bot, set to your discord bot ID if -DDISCORD=1
clientID=id
#Room to join
roomName=Magic
#Folder to save replays to, also effects the URL that replays are servered on
replayFolder=replays
#Rate limit in max messages per second
ratelimit=5
#Base of the external URL
externURL=https://tricebot.co.uk
```
Change the data for what you want. (is whitespace sensitive)
For duplicate property tags, the last line of the tag is used i.e:
```yaml
username=not this
username=this is used
...
```

You can add comments by starting a line with a hashtag (#) i.e:
```yaml
#this is a comment
username=jeff
...
```

If the bot starts and any of the values are not set in the configuration file it
will print an error message, if the values are invalid then the program may crash,
I will work on a fix to that issue at some point soon. If there any issues with
configuring the bot then do feel free to ask but dont share you password :).

The clientID is the client ID of the discord bot attched, use -DDISCORD=0 if you
are not using a discord bot and set the client ID to your IP or random noise. If
discord is set to 1, then the index page of the bot will be an invite link for
that bot (perms are set to admin by default).

### certfile and certkeyfile
Generate some ssl keys and slap them in the folder with the executable and then
change the config file to have the correct names. The api server does not support
CA certificates because it is not web-facing. Ideal configuration would have the
bot running on the same machine as the program that uses it.

## Compiling
Install dependencies (ubuntu package names):
`cmake g++ or clang++ libmbedtls-dev libprotobuf-dev protobuf-compiler libcppunit-dev`
The tests use `cppunit` and `gcovr`.

Create build directory and use cmake:
```sh
mkdir build
cd build
cmake .. # -DCMAKE_BUILD_TYPE=Debug
cmake --build . # -j x
```

### Tests
Compiling with build type TEST will create the test executable then you can run
it with `make coverage` or gcov to get coverage if you want.
```sh
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=TEST
cmake --build . # -j x
./CockatriceTournamentBotTests #runs tests
make coverage #runs tests and gets coverage
```

## Help it is borked
Create an issue or look through issues for your problem, if the program crashes or 
hangs, please provide debug information, see below for help with that.

### Providing Debug Information
Compile a debug build with 
```sh
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . # -j x
```
Please copy and paste the logs from before the crash/hangand put them in your 
issue, then (optional) run with a debugger and get the full backtrace of the 
program. Make sure that the program is called with an argument
of `3` to get some verbose mongoose logs.

## Contributing
#### Commits and Code Coverage Graph
(because I can)
[![commits](https://codecov.io/gh/djpiper28/CockatriceTournamentBot/branch/main/graphs/commits.svg)](https://codecov.io/gh/djpiper28/CockatriceTournamentBot)

### Code Styling
Where possible, code should be ISO C with exceptions for protobuf or cppunit
library binds.

#### Indentation And New Lines
 - Indentation should be with spaces not tabs (4 spaces).
 - Functions should have each arg on separate lines that are indented to the function name's end.
i.e:
```c
sampleFunction(argOne,
               argTwo);
```
 - Structs should be indented to one level worth. 
 - When two or more variables in a struct have the same type they should be defined on separate lines.
i.e:
```c
struct a = {
    int a;
    char *b,
          c;
};
```

#### Tests
Tests are written in mostly C but use the cppunit library as needed with
classes named in camelCase. Tests are compiled by automatically and can be
ran with `ctest` or `./CockatriceTournamentBotTests`.

[![tests](https://codecov.io/gh/djpiper28/CockatriceTournamentBot/branch/main/graphs/sunburst.svg)](https://codecov.io/gh/djpiper28/CockatriceTournamentBot)

#### Compiler Directives
 - All defines should be named in SCREAMING_SNAKE_CASE
 - All compiler directives should be at the top of the code if possible
 - The `#ifndef #define` should be used to guard against multiple header inclusions

#### Naming Conventions
 - camelCase for function names and variable names
 - PascalCase for struct names
 - snake_case for file cxx file names
 - camelCase for python file names

#### Comments
 - Please add comments

#### Constants
 - #define them please as `const` is cringe

## TODO:
- allow multi-room gaming
