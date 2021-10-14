#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "bot_conf.h"
#include "api_server.h"
#include "trice_structs.h"
#include "bot.h"
#include "version.h"
#include "cmd_queue.h"
#include "commands.pb.h"
#include "command_leave_game.pb.h"
#include "room_commands.pb.h"
#include "player_deck_info.h"
#include "commands.h"

#include "command_kick_from_game.pb.h"
#include "command_game_say.pb.h"

struct tournamentBot {
    struct Config config;
    struct triceBot b;
    struct tb_apiServer server;
    int running;
};

void stopAll(struct tournamentBot *bot) {
    printf("[INFO]: Stopping bot\n");

    bot->running = 0;
    tb_stopServer(&bot->server);
    stopBot(&bot->b);

    tb_freeServer(&bot->server);
    freeBot(&bot->b);
}

#if DEBUG
#define MACRO_DEBUG_FOR_EVENT(fn, type)\
void DebugFor##fn (struct triceBot *b, type event) {\
    time_t rawtime;\
    struct tm info;\
    char buffer[80];\
    time(&rawtime);\
    localtime_r(&rawtime , &info);\
    strftime(buffer, 80, "%x - %H:%M:%S %Z", &info);\
    \
    printf("[DEBUG] (%s) %s: %s\n",#fn , buffer, event.DebugString().c_str());\
}

#define MACRO_DEBUG_FOR_GAME_EVENT(fn,type)\
void DebugFor##fn (struct triceBot *b, struct game, type event, int pid) {\
    time_t rawtime;\
    struct tm info;\
    char buffer[80];\
    time(&rawtime);\
    localtime_r(&rawtime , &info);\
    strftime(buffer, 80, "%x - %H:%M:%S %Z", &info);\
    \
    printf("[DEBUG] %s: %s\n", buffer, #fn);\
}

#define MACRO_DEBUG_FOR_STATE_CHANGE(fn)\
void DebugFor##fn (struct triceBot *b) {\
    time_t rawtime;\
    struct tm info;\
    char buffer[80];\
    time(&rawtime);\
    localtime_r(&rawtime , &info);\
    strftime(buffer, 80, "%x - %H:%M:%S %Z", &info);\
    \
    printf("[DEBUG] %s: %s\n", buffer, #fn);\
}

//Server events
MACRO_DEBUG_FOR_EVENT(onEventServerIdentifictaion,
                      Event_ServerIdentification)
MACRO_DEBUG_FOR_EVENT(onEventServerCompleteList,
                      Event_ServerCompleteList)
MACRO_DEBUG_FOR_EVENT(onEventServerMessage,
                      Event_ServerMessage)
MACRO_DEBUG_FOR_EVENT(onEventServerShutdown,
                      Event_ServerShutdown)
MACRO_DEBUG_FOR_EVENT(onEventConnectionClosed,
                      Event_ConnectionClosed)
MACRO_DEBUG_FOR_EVENT(onEventUserMessage,
                      Event_UserMessage)
MACRO_DEBUG_FOR_EVENT(onEventListRooms,
                      Event_ListRooms)
MACRO_DEBUG_FOR_EVENT(onEventAddToList,
                      Event_AddToList)
MACRO_DEBUG_FOR_EVENT(onEventRemoveFromList,
                      Event_RemoveFromList)
MACRO_DEBUG_FOR_EVENT(onEventUserJoined,
                      Event_UserJoined)
MACRO_DEBUG_FOR_EVENT(onEventUserLeft,
                      Event_UserLeft)
MACRO_DEBUG_FOR_EVENT(onEventGameJoined,
                      Event_GameJoined)
MACRO_DEBUG_FOR_EVENT(onEventNotifyUser,
                      Event_NotifyUser)
MACRO_DEBUG_FOR_EVENT(onEventReplayAdded,
                      Event_ReplayAdded)

//Room events
MACRO_DEBUG_FOR_EVENT(onEventJoinRoom,
                      Event_JoinRoom)
MACRO_DEBUG_FOR_EVENT(onEventLeaveRoom,
                      Event_LeaveRoom)
MACRO_DEBUG_FOR_EVENT(onEventRoomSay,
                      Event_RoomSay)

//State changes
MACRO_DEBUG_FOR_STATE_CHANGE(onBotDisconnect)
MACRO_DEBUG_FOR_STATE_CHANGE(onBotConnect)
MACRO_DEBUG_FOR_STATE_CHANGE(onBotConnectionError)
MACRO_DEBUG_FOR_STATE_CHANGE(onBotLogin)
MACRO_DEBUG_FOR_STATE_CHANGE(onReplayDownload)

#define MACRO_DEBUG_FOR_EVENT_CALL(fn) set_##fn(&DebugFor##fn, b);

void addDebugFunctions(struct triceBot *b) {
    MACRO_DEBUG_FOR_EVENT_CALL(onEventServerIdentifictaion)
    MACRO_DEBUG_FOR_EVENT_CALL(onEventServerCompleteList)
    MACRO_DEBUG_FOR_EVENT_CALL(onEventServerMessage)
    MACRO_DEBUG_FOR_EVENT_CALL(onEventServerShutdown)
    MACRO_DEBUG_FOR_EVENT_CALL(onEventConnectionClosed)
    MACRO_DEBUG_FOR_EVENT_CALL(onEventUserMessage)
    MACRO_DEBUG_FOR_EVENT_CALL(onEventListRooms)
    MACRO_DEBUG_FOR_EVENT_CALL(onEventAddToList)
    MACRO_DEBUG_FOR_EVENT_CALL(onEventRemoveFromList)
    MACRO_DEBUG_FOR_EVENT_CALL(onEventUserJoined)
    MACRO_DEBUG_FOR_EVENT_CALL(onEventUserLeft)
    MACRO_DEBUG_FOR_EVENT_CALL(onEventGameJoined)
    MACRO_DEBUG_FOR_EVENT_CALL(onEventNotifyUser)
    MACRO_DEBUG_FOR_EVENT_CALL(onEventReplayAdded)

    //Room events
    MACRO_DEBUG_FOR_EVENT_CALL(onEventJoinRoom)
    MACRO_DEBUG_FOR_EVENT_CALL(onEventLeaveRoom)
    MACRO_DEBUG_FOR_EVENT_CALL(onEventRoomSay)

    //State changes
    MACRO_DEBUG_FOR_EVENT_CALL(onBotDisconnect)
    MACRO_DEBUG_FOR_EVENT_CALL(onBotConnect)
    MACRO_DEBUG_FOR_EVENT_CALL(onBotConnectionError)
    MACRO_DEBUG_FOR_EVENT_CALL(onBotLogin)
    MACRO_DEBUG_FOR_EVENT_CALL(onReplayDownload)
}

#endif

int baseLen = strlen("Player @ was kicked, as they were not expected in this "
"game. This action was taken automatically. "
"If you do not think you should have been kicked checked "
"your cockatrice name was put into the bot correctly. "
"Then contact your tournament organiser (they can disable "
"this check), finally if that doesn't help please raise an "
"issue at:  if this was an error. Expected players: ") + strlen(GITHUB_REPO);

void playerJoin(struct triceBot *b,
                struct game g,
                Event_Join event,
                int pid) {
    // Get player deck info
    struct playerDeckInfo *pdi = (struct playerDeckInfo *) g.gameData.gameDataPtr;
    if (pdi != NULL && event.has_player_properties()) {
        ServerInfo_PlayerProperties pp = event.player_properties();
        if ((!(pp.spectator() || pp.judge())) && pp.has_user_info()) {
            ServerInfo_User user = pp.user_info();

            // Check if player is allowed
            int allowed = isPlayerAllowed((char *) user.name().c_str(), pid, g);

            // If they are not then kick them
            if (!allowed) {
                char *messageBuffer = (char *) malloc(sizeof(char) *
                    (baseLen + (1 + g.playerCount) * (PLAYER_NAME_LENGTH + 1)));
                snprintf(messageBuffer,
                         512,
                         "Player @%s was kicked, as they were not expected in this "
                         "game. This action was taken automatically. "
                         "If you do not think you should have been kicked checked "
                         "your cockatrice name was put into the bot correctly. "
                         "Then contact your tournament organiser (they can disable "
                         "this check), finally if that doesn't help please raise an "
                         "issue at: %s if this was an error. Expected players: ",
                         user.name().c_str(),
                         GITHUB_REPO);
                printf("[INFO]: Player %s was kicked from game %d.\n",
                       user.name().c_str(),
                       g.gameID);

                for (int i = 0; i < g.playerCount; i++) {
                    strncat(messageBuffer, pdi[i].playerName, PLAYER_NAME_LENGTH);
                    strncat(messageBuffer, " ", 2);
                }

                Command_GameSay gameSayCmd;
                gameSayCmd.set_message(messageBuffer);

                CommandContainer cont;
                GameCommand *gc = cont.add_game_command();
                gc->MutableExtension(Command_GameSay::ext)
                    ->CopyFrom(gameSayCmd);

                struct pendingCommand *cmd = prepCmd(b,
                                                     cont,
                                                     g.gameID,
                                                     b->magicRoomID);

                enq(cmd, &b->sendQueue);
                free(messageBuffer);

                Command_KickFromGame kickCommand;
                kickCommand.set_player_id(pid);

                CommandContainer cont2;
                GameCommand *gc2 = cont2.add_game_command();
                gc2->MutableExtension(Command_KickFromGame::ext)
                    ->CopyFrom(kickCommand);

                cmd = prepCmd(b,
                              cont2,
                              g.gameID,
                              b->magicRoomID);

                enq(cmd, &b->sendQueue);
            }
        }
    }
}

void playerLeave(struct triceBot *b,
                 struct game g,
                 Event_Leave event,
                 int pid) {
    for (int i = 0; i < g.playerCount; i++) {
        if(g.playerArr[i].playerID == pid) {
            clearPlayerSlot(g.playerArr[i].playerID, g);
        }
    }
}

void playerPropertyChange(struct triceBot *b,
                          struct game g,
                          Event_PlayerPropertiesChanged event,
                          int pid) {
    struct playerDeckInfo *pdi = (struct playerDeckInfo *) g.gameData.gameDataPtr;
    if (pdi != NULL && event.has_player_properties() && pid != -1) {
        ServerInfo_PlayerProperties pp = event.player_properties();
        // Check if the deck has been changed
        if (pp.has_deck_hash()) {
            char *deckHash = (char *) pp.deck_hash().c_str();
            int allowed = isPlayerDeckAllowed(deckHash, pid, g);

            int plrArrayIndex = -1;
            for (int i = 0; plrArrayIndex == -1 && i < g.playerCount; i++) {
                if (g.playerArr[i].playerID == pid) {
                    plrArrayIndex = i;
                }
            }

            int pdiIndex = -1;
            for (int i = 0; pdiIndex == -1 && i < g.playerCount; i++) {
                if (pdi[i].playerUsingSlot == pid) {
                    pdiIndex = i;
                }
            }


            // If the hash is not allowed then tell the user
            if (plrArrayIndex != -1 && pdiIndex != -1 && !allowed) {
                char *space = " ";
                int spaceLen = strlen(space);
                int length = 512 + (DECK_HASH_LENGTH + spaceLen) * pdi[pdiIndex].deckCount;
                char *messageBuffer = (char *) malloc(sizeof(char) * length);
                if (pdi[pdiIndex].deckCount == 1) {
                    snprintf(messageBuffer,
                             512,
                             "@%s, you loaded a deck with hash '%s', which is not "
                             "expected. Please load the deck with hash: ",
                             g.playerArr[plrArrayIndex].playerName,
                             deckHash);
                } else {
                    snprintf(messageBuffer,
                            512,
                            "@%s, you loaded a deck with hash '%s', which is not "
                            "expected. Please load a deck with of these hashes: ",
                            g.playerArr[plrArrayIndex].playerName,
                            deckHash);
                }

                printf("[INFO]: Player %s loaded an invalid deck.\n",
                       g.playerArr[plrArrayIndex].playerName);

                for (int i = 0; i < pdi[pdiIndex].deckCount; i++) {
                    strncat(messageBuffer, pdi[pdiIndex].deckHash[i], DECK_HASH_LENGTH);
                    strncat(messageBuffer, space, spaceLen);
                }

                Command_GameSay gameSayCmd;
                gameSayCmd.set_message(messageBuffer);

                CommandContainer cont;
                GameCommand *gc = cont.add_game_command();
                gc->MutableExtension(Command_GameSay::ext)
                    ->CopyFrom(gameSayCmd);

                struct pendingCommand *cmd = prepCmd(b,
                                                     cont,
                                                     g.gameID,
                                                     b->magicRoomID);

                enq(cmd, &b->sendQueue);

                free(messageBuffer);
            }
        }
    }
}

void gameEndCallback(struct triceBot *b,
                     const Response *r,
                     void *param) {
    int id = *(int *) param;
    if (r->has_response_code()) {
        if (r->response_code() == Response::RespOk) {;
            pthread_mutex_lock(&b->gameList.mutex);

            struct game *g = NULL;
            struct gameListNode *current = b->gameList.gamesHead;
            while (current != NULL && g == NULL) {
                if (current->currentGame != NULL) {
                    if (current->currentGame->gameID == id) {
                        g = current->currentGame;
                    }
                }

                current = current->nextGame;
            }

            pthread_mutex_unlock(&b->gameList.mutex);

            if (g != NULL) {
                removeGame(&b->gameList, g);
            }
        }
    }

    free(param);
}

void onGameEnd(struct triceBot *b,
               struct game g) {
    pthread_mutex_lock(&b->mutex);
    int ID = b->magicRoomID;
    pthread_mutex_unlock(&b->mutex);

    Command_LeaveGame leaveGame;
    CommandContainer cont;
    GameCommand *gc = cont.add_game_command();
    gc->MutableExtension(Command_LeaveGame::ext)->CopyFrom(leaveGame);

    //IDs are set in prepCMD
    struct pendingCommand *cmd = prepCmd(b, cont, g.gameID, ID);
    cmd->callbackFunction = &gameEndCallback;
    cmd->param = malloc(sizeof(int));
    int *id = (int *) cmd->param;
    *id = g.gameID;

    enq(cmd, &b->sendQueue);
}

void botDisconnect(struct triceBot *b) {
    printf("[INFO]: The bot disconnected and will restart.\n");
    startBot(b);
}

void botConnect(struct triceBot *b) {
    printf("[INFO]: The bot has successfully connected to the server and will finish starting.\n");
}

int main(int argc, char * args[]) {
    printf("[INFO]: %s\n-> by djpiper28 see %s for git repo.\n",
           PROG_NAME,
           GITHUB_REPO);
    printf("-> Version %d.%d\n",
           VERSION_MAJOR,
           VERSION_MINOR);
    printf("-> Use the first argument for the mongoose debug level (0,1,2,3 or 4).\n");
    printf("[INFO]: Starting bot...\n");

    mg_log_set(argc > 1 ? args[1] : "0");

    struct tournamentBot bot;
    bot.running = 1;

    int status = readConf(&bot.config, "config.conf");
    int valid = 1;

    // Check conf is valid
    if (status) {
        if (bot.config.cockatriceUsername == NULL) {
            valid = 0;
            printf("[ERROR]: Cockatrice username is not defined in config.conf.\n");
        }

        if (bot.config.cockatricePassword == NULL) {
            valid = 0;
            printf("[ERROR]: Cockatrice password is not defined in config.conf.\n");
        }

        if (bot.config.roomName == NULL) {
            valid = 0;
            printf("[ERROR]: Cockatrice room name is not defined in config.conf.\n");
        }

        if (bot.config.cockatriceServer == NULL) {
            valid = 0;
            printf("[ERROR]: Cockatrice server address is not defined in config.conf.\n");
        }

        if (bot.config.clientID == NULL) {
            valid = 0;
            printf("[ERROR]: Cockatrice client ID is not defined in config.conf.\n");
        }

        if (bot.config.replayFolder == NULL) {
            valid = 0;
            printf("[ERROR]: Replay folder is not defined in config.conf.\n");
        }

        if (bot.config.authToken == NULL) {
            valid = 0;
            printf("[ERROR]: Authentication token is not defined in config.conf.\n");
        }

        if (bot.config.bindAddr == NULL) {
            valid = 0;
            printf("[ERROR]: API server bind address is not defined in config.conf.\n");
        }

        // Check certs exist and are readable.
        if (bot.config.cert == NULL) {
            valid = 0;
            printf("[ERROR]: SSL certificate file is not defined in config.conf.\n");
        } else {
            if (access(bot.config.cert, F_OK) != 0) {
                valid = 0;
                printf("[ERROR]: SSL certificate file defined in config.conf does not exist.\n");
            } else {
                if (access(bot.config.cert, R_OK) != 0) {
                    valid = 0;
                    printf("[ERROR]: SSL certificate file defined in config.conf cannot be read.\n");
                }
            }
        }

        if (bot.config.certkey == NULL) {
            valid = 0;
            printf("[ERROR]: SSL certificate key file is not defined in config.conf.\n");
        } else {
            if (access(bot.config.certkey, F_OK) != 0) {
                valid = 0;
                printf("[ERROR]: SSL certificate key file defined in config.conf does not exist.\n");
            } else {
                if (access(bot.config.certkey, R_OK) != 0) {
                    valid = 0;
                    printf("[ERROR]: SSL certificate key file defined in config.conf cannot be read.\n");
                }
            }
        }

        if (bot.config.maxMessagesPerSecond == -1) {
            valid = 0;
            printf("[ERROR]: Rate limit is not defined in config.conf.\n");
        } else if (bot.config.maxMessagesPerSecond <= 0) {
            valid = 0;
            printf("[ERROR]: Rate limit is defined in config.conf but is an invalid value. "
                   "Make sure it is a value above 0.\n");
        }

        if (valid) {
            printf("[INFO]: Config file read successfully.\n");
            initBot(&bot.b, bot.config);
            tb_initServer(&bot.server, &bot.b, bot.config);

#if DEBUG
            addDebugFunctions(&bot.b);
#endif

            set_onGameEnd(&onGameEnd, &bot.b);
            set_onBotConnect(&botConnect, &bot.b);
            set_onBotDisconnect(&botDisconnect, &bot.b);
            set_onGameEventJoin(&playerJoin, &bot.b);
            set_onGameEventLeave(&playerLeave, &bot.b);
            set_onGameEventPlayerPropertyChanged(&playerPropertyChange, &bot.b);

            printf("[INFO]: Starting bot...\n");

            initCommandList(&bot.b);
            startBot(&bot.b);
            tb_startServer(&bot.server);

            printf("[INFO]: Bot started.\n");

            for (;;) {
                sleep(5);
            }

            //freeCommandList();
        } else {
            printf("[ERROR]: Missing properties in config file, see README.md at "
                   "%s/blob/main/README.md.\n",
                   GITHUB_REPO);
        }
    } else if (status == 0) {
        printf("[ERROR]: There was an error reading the config file.\n");
    } else if (status == -1) {
        printf("[ERROR]: No config file exists, a new one was made. "
               "Please edit it then restart the bot.\n");
    }
}