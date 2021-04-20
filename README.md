# Marchesa Bot (or the boring name of CockatriceTournamentBot)
(Linux only due to pthread and unistd dependancies)

## Interfacing with it 
The bot will host an https server (with self signed keys probably) and apps that
interface with the bot will use that to connect and send commands which will then
get a response (see src/helppage.gen.html for details). A sample implementation is
in python in a file called `tricebot.py`.

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
clientID=test
roomName=Magic
replayFolder=replays
```
Change the data for what you want. (is whitespace sensitive)
For duplicate property tags, the last line of the tag is used i.e:

```yaml
username=not this
username=this is used

```

## certfile and certkeyfile
Generate some ssl keys and slap them in the folder with the executable and then
change the config file to have the correct names. The api server does not support
CA certificates because it is not web-facing. Ideal configuration would have the
bot running on the same machine as the program that uses it.

## Help it is borked
compile with `make build-debug` and then run it with a debugger and get the full
backtrace of the program. Make sure that the program is called with an argument of
`3` to get some verbose mongoose logs.

## TODO:
make it make a log or something idk
