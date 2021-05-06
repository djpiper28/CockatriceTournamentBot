# Marchesa Bot (or the boring name of CockatriceTournamentBot)

## Build status
[![CodeQL - Build and Test (Ubuntu)](https://github.com/djpiper28/CockatriceTournamentBot/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/djpiper28/CockatriceTournamentBot/actions/workflows/codeql-analysis.yml)

(Linux only due to pthread and unistd dependancies)

## Interfacing with the bot
The bot will host an https server (with self signed keys probably) and apps that
interface with the bot will use that to connect and send commands which will then
get a response (see src/helppage.gen.html for details). A sample implementation is
in python in a file called `tricebot.py`.

Do note empty games are deleted after 30 minutes by this bot.

## Configuring The Bot
The file (config.conf) should be in this format:

```py
username=username
password=yourpassword
serveraddress=ws://server.cockatrice.us:4748
authtoken=nGpzR/KspN6ry7jG8CU4bonN2aujzfJa
certfile=fullchain.pem
certkeyfile=privkey.pem
bindAddr=https://127.0.0.1:8000
clientID=id
roomName=Magic
replayFolder=replays
```
Change the data for what you want. (is whitespace sensitive)
For duplicate property tags, the last line of the tag is used i.e:

You can add comments by starting a line with a hashtag (#) i.e:
```py
#this is a comment
username=jeff
...
```

The clientID is the client ID of the discord bot attched, use -DDISCORD=0 if you
are not using a discord bot and set the client ID to your IP or random noise. If
discord is set to 1, then the index page of the bot will be an invite link for
that bot (perms are set to admin by default).

```py
username=not this
username=this is used

```

### certfile and certkeyfile
Generate some ssl keys and slap them in the folder with the executable and then
change the config file to have the correct names. The api server does not support
CA certificates because it is not web-facing. Ideal configuration would have the
bot running on the same machine as the program that uses it.

## Compiling
Install dependencies (ubuntu package names):
`cmake g++ or clang++ libmbedtls-dev libprotobuf-dev protobuf-compiler libcppunit-dev`

Create build directory and use cmake:
```sh
mkdir build
cd build
cmake .. # -DCMAKE_BUILD_TYPE=Debug
cmake --build . # -j x
```
Tests are compiled with this program by default, to runthese tests use the command 
`ctest` after compiling.

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

## Code Styling
Where possible, code should be ISO C with exceptions for protobuf or cppunit
library binds.

### Indentation And New Lines
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

### Tests
Tests are written in mostly C but use the cppunit library as needed with
classes named in camelCase. Tests are compiled by automatically and can be
ran with `ctest` or `./CockatriceTournamentBotTests`.

### Compiler Directives
 - All defines should be named in SCREAMING_SNAKE_CASE
 - All compiler directives should be at the top of the code if possible
 - The `#ifndef #define` should be used to guard against multiple header inclusions

### Naming Conventions
 - camelCase for function names and variable names
 - PascalCase for struct names
 - snake_case for file cxx file names
 - camelCase for python file names

### Comments
 - Please add comments

### Constants
 - #define them please as `const` is cringe

## TODO:
- add tricebot rate limit (wip)
- allow multi-room gaming
