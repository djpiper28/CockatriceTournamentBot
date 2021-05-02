# Marchesa Bot (or the boring name of CockatriceTournamentBot)

## Build status
[![CodeQL](https://github.com/djpiper28/CockatriceTournamentBot/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/djpiper28/CockatriceTournamentBot/actions/workflows/codeql-analysis.yml)

(Linux only due to pthread and unistd dependancies)

## Interfacing with the bot
The bot will host an https server (with self signed keys probably) and apps that
interface with the bot will use that to connect and send commands which will then
get a response (see src/helppage.gen.html for details). A sample implementation is
in python in a file called `tricebot.py`.

Do note empty games are deleted after 30 minutes by this bot.

## Config file
The file (config.conf) should be in this format:

```yaml
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

The clientID is the client ID of the discord bot attched, use -DDISCORD=0 if you
are not using a discord bot and set the client ID to your IP or random noise. If
discord is set to 1, then the index page of the bot will be an invite link for
that bot (perms are set to admin by default).

```yaml
username=not this
username=this is used

```

### certfile and certkeyfile
Generate some ssl keys and slap them in the folder with the executable and then
change the config file to have the correct names. The api server does not support
CA certificates because it is not web-facing. Ideal configuration would have the
bot running on the same machine as the program that uses it.

## Compiling
Install dependencies:
`cmake g++ libmbedtls-dev libprotobuf-dev protobuf-compiler`

Create build directory and use cmake:
```sh
mkdir build
cd build
cmake .. # -DCMAKE_BUILD_TYPE=Debug
cmake --build . # -j x
```
Tests are compiled with this program by default, to run
these tests use the command `ctest`.

## Help it is borked
Create an issue or look through issues for your problem.

### The program is crashing or hanging
Compile with 
```sh
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . # -j x
``` 
then run with a debugger and get the full backtrace of the program. 
Make sure that the program is called with an argument
of `3` to get some verbose mongoose logs.

## TODO:
- add tricebot rate limit (wip)
- allow multi-room gaming
