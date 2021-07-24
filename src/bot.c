#ifndef BOT_
#define BOT_
#include "bot.h"

#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "bot_conf.h"
#include "game_struct.h"
#include "version_string.h"
#include "mongoose.h"
#include "get_pb_extension.h"
#include "cmd_queue.h"
#include "trice_structs.h"
#include "bot_c_flags.h"

//Pb imports
#include "game_replay.pb.h"
#include "room_event.pb.h"
#include "session_event.pb.h"
#include "server_message.pb.h"
#include "event_player_properties_changed.pb.h"
#include "event_add_to_list.pb.h"
#include "event_remove_from_list.pb.h"
#include "event_connection_closed.pb.h"
#include "event_join.pb.h"
#include "event_game_joined.pb.h"
#include "event_game_closed.pb.h"
#include "event_notify_user.pb.h"
#include "event_game_state_changed.pb.h"
#include "event_replay_added.pb.h"
#include "event_server_complete_list.pb.h"
#include "event_server_identification.pb.h"
#include "event_server_message.pb.h"
#include "event_server_shutdown.pb.h"
#include "event_list_rooms.pb.h"
#include "event_list_games.pb.h"
#include "event_room_say.pb.h"
#include "event_join_room.pb.h"
#include "event_leave_room.pb.h"
#include "event_user_joined.pb.h"
#include "event_user_left.pb.h"
#include "event_user_message.pb.h"
#include "response_login.pb.h"
#include "response_replay_download.pb.h"
#include "game_event_container.pb.h"
#include "game_event_context.pb.h"
#include "game_event.pb.h"
#include "session_commands.pb.h"
#include "room_commands.pb.h"
#include "game_commands.pb.h"
#include "commands.pb.h"
#include "command_leave_game.pb.h"
#include "command_replay_download.pb.h"
#include "command_deck_select.pb.h"
#include "command_ready_start.pb.h"
#include "command_concede.pb.h"
#include "serverinfo_replay_match.pb.h"

#define ID_LENGTH 16
#define RESPONSE 0
#define SESSION_EVENT 1
#define GAME_EVENT_CONTAINER 2
#define ROOM_EVENT 3
#define MAX_GAME_WAIT 1800

/**
 * This macro is to reduce code repetition in the session event method which
 * checks if the pointer is not null then copies it; unlocks the mutex and;
 * finally calls the pointer for the extension of the session event.
 * WARNING mutex must be unlocked before macro
 */
#define MACRO_CALL_FUNCTION_PTR_FOR_BOT_STATE_CHANGE(fn)\
pthread_mutex_lock(&b->mutex);\
void (*fn) (struct triceBot *b) = b->fn;\
pthread_mutex_unlock(&b->mutex);\
\
if (fn != NULL) {\
    fn(b);\
}


/**
 * This macro is to reduce code repetition in the session event method which
 * checks if the pointer is not null then copies it; unlocks the mutex and;
 * finally calls the pointer for the extension of the session event.
 * WARNING mutex must be unlocked before macro
 * WARNING the event variable must be called event which has the .GetExtension
 * method.
 */
#define MACRO_CALL_FUNCTION_PTR_FOR_EVENT(fn, type)\
pthread_mutex_lock(&b->mutex);\
void (*fn) (struct triceBot *b, type event) = b->fn;\
pthread_mutex_unlock(&b->mutex);\
\
if (fn != NULL) {\
    fn(b, event.GetExtension(type::ext));\
}

//This has an if statement unlike the rest as the only edge case is long
//The event must be called event and the player id an int called pid
#define MACRO_CALL_FUNCTION_PTR_FOR_GAME_EVENT(fn, type)\
if (event.HasExtension(type::ext)) {\
    pthread_mutex_lock(&b->mutex);\
    void (*fn) (struct triceBot *b, struct game, type, int) = b->fn;\
    pthread_mutex_unlock(&b->mutex);\
    \
    if (fn != NULL) {\
        fn(b, g, event.GetExtension(type::ext), pid);\
    }\
}

//Type defs moved to trice_structs.h due to circlular references

/**
 * struct triceBot *b -> pointer to the bot to init
 * struct Config config -> bot configuration
 */
void initBot(struct triceBot *b,
             struct Config config) {
    b->mutex = PTHREAD_MUTEX_INITIALIZER;
    b->config = config;
    
    b->roomName = NULL;

    b->magicRoomID = -1;
    b->lastPingTime = 0;
    b->cmdID = 0;
    b->lastSend = 0;
    b->lastGameWaitCheck = 0;
    b->running = 0;

    b->gameList.gamesHead = NULL;
    b->sendQueue.head = NULL;
    b->sendQueue.tail = NULL;
    b->callbackQueue.head = NULL;
    b->callbackQueue.tail = NULL;

    // Set all event functions to NULL
    b->onEventServerIdentifictaion = NULL;
    b->onEventServerCompleteList = NULL;
    b->onEventServerMessage = NULL;
    b->onEventServerShutdown = NULL;
    b->onEventConnectionClosed = NULL;
    b->onEventUserMessage = NULL;
    b->onEventListRooms = NULL;
    b->onEventAddToList = NULL;
    b->onEventRemoveFromList = NULL;
    b->onEventUserJoined = NULL;
    b->onEventUserLeft = NULL;
    b->onEventGameJoined = NULL;
    b->onEventNotifyUser = NULL;
    b->onEventReplayAdded = NULL;

    //Event Function Pointers for room events
    b->onEventJoinRoom = NULL;
    b->onEventLeaveRoom = NULL;
    b->onEventRoomSay = NULL;

    //Game events
    b->onGameEventJoin = NULL;
    b->onGameEventGameClosed = NULL;
    b->onGameEventHostChanged = NULL;
    b->onGameEventPlayerKicked = NULL;
    b->onGameEventPlayerPropertyChanged = NULL;
    b->onGameEventGameSay = NULL;
    b->onGameEventCreateArrow = NULL;
    b->onGameEventDeleteArrow = NULL;
    b->onGameEventCreateCounter = NULL;
    b->onGameEventSetCounter = NULL;
    b->onGameEventDelCounter = NULL;
    b->onGameEventDrawCards = NULL;
    b->onGameEventRevealCards = NULL;
    b->onGameEventShuffle = NULL;
    b->onGameEventRollDie = NULL;
    b->onGameEventMoveCard = NULL;
    b->onGameEventFlipCard = NULL;
    b->onGameEventDestroyCard = NULL;
    b->onGameEventCreateToken = NULL;
    b->onGameEventSetCardAttr = NULL;
    b->onGameEventSetCardCounter = NULL;
    b->onGameEventSetActivePlayer = NULL;
    b->onGameEventSetActivePhase = NULL;
    b->onGameEventDumpZone = NULL;
    b->onGameEventStopDumpZone = NULL;
    b->onGameEventChangeZoneProperties = NULL;
    b->onGameEventReverseTurn = NULL;

    //Game state changes
    b->onGameStart = NULL;
    b->onGameEnd = NULL;

    //Bot state changes
    b->onBotDisconnect = NULL;
    b->onBotConnect = NULL;
    b->onBotConnectionError = NULL;
    b->onBotLogin = NULL;
    b->onReplayDownload = NULL;
}

/**
 * Check to see if the server should be pinged
 */
int needsPing(long lastPingTime) {
    return time(NULL) - lastPingTime >= PING_FREQUENCY;
}

static void pingCallback(struct triceBot *b,
                         const Response *resp,
                         void *param) {
    pthread_mutex_lock(&b->mutex);
    b->lastPingTime = time(NULL); // Ping every TIMEOUT with no msg received
    pthread_mutex_unlock(&b->mutex);
}

/**
 * Queues a ping command
 */
void sendPing(struct triceBot *b) {
    pthread_mutex_lock(&b->mutex);
    CommandContainer cont;
    SessionCommand *c = cont.add_session_command();
    c->MutableExtension(Command_Ping::ext);

    struct pendingCommand *cmd = prepCmdNTS(b, cont, -1, -1);
    cmd->callbackFunction = &pingCallback;
    enq(cmd, &b->sendQueue);
    pthread_mutex_unlock(&b->mutex);
}

/**
 * This part of the library will take a login response and it will check if it
 * is logged in. If it is not logged in it will call the onBotLogin state change
 * event in the same thread after requesting the room list
 */
void loginResponse(struct triceBot *b,
                   const Response *response,
                   void *param) {
    pthread_mutex_lock(&b->mutex);

    if (response != NULL) {
        if (response->HasExtension(Response_Login::ext)) {
            Response_Login resp = response->GetExtension(Response_Login::ext);

            if (response->response_code() == Response::RespOk) {
                b->loggedIn = 1;

                //Get room list
                CommandContainer cont;
                SessionCommand *c = cont.add_session_command();
                c->MutableExtension(Command_ListRooms::ext);

                struct pendingCommand *cmd = prepCmdNTS(b, cont, -1, -1);
                enq(cmd, &b->sendQueue);

                b->roomRequested = 1;

                pthread_mutex_unlock(&b->mutex);
                MACRO_CALL_FUNCTION_PTR_FOR_BOT_STATE_CHANGE(onBotLogin)
                pthread_mutex_lock(&b->mutex);
            } else if (response->response_code() != Response::RespOk) {
                b->running = 0;
                b->loggedIn = 0;
            }
        } else {
        }
    }

    pthread_mutex_unlock(&b->mutex);
}

//Login, login details are stored in b->config
void sendLogin(struct triceBot *b) {
    pthread_mutex_lock(&b->mutex);

    Command_Login cmdLogin;
    cmdLogin.set_user_name(b->config.cockatriceUsername);
    cmdLogin.set_password(b->config.cockatricePassword);
    cmdLogin.set_clientid(b->config.clientID);
    cmdLogin.set_clientver(VERSION_STRING);

    cmdLogin.add_clientfeatures("client_id");
    cmdLogin.add_clientfeatures("client_ver");
    cmdLogin.add_clientfeatures("feature_set");
    cmdLogin.add_clientfeatures("room_chat_history");
    cmdLogin.add_clientfeatures("client_warnings");
    cmdLogin.add_clientfeatures("forgot_password");
    cmdLogin.add_clientfeatures("idle_client");
    cmdLogin.add_clientfeatures("mod_log_lookup");
    cmdLogin.add_clientfeatures("user_ban_history");
    cmdLogin.add_clientfeatures("websocket");
    cmdLogin.add_clientfeatures("2.7.0_min_version");
    cmdLogin.add_clientfeatures("2.8.0_min_version");

    CommandContainer cont;
    SessionCommand *c = cont.add_session_command();
    c->MutableExtension(Command_Login::ext)->CopyFrom(cmdLogin);

    struct pendingCommand *cmd = prepCmdNTS(b, cont, -1, -1);
    enq(cmd, &b->sendQueue);

    pthread_mutex_unlock(&b->mutex);
}

/**
 * Executes the callback of the command
 */
void executeCallback(struct triceBot *b,
                     struct pendingCommand *cmd,
                     const Response *response) {
    if (cmd->callbackFunction != NULL) {
        cmd->callbackFunction(b, response, cmd->param);
    }

    //Free command - we don't like leaky memory
    if (cmd->message != NULL) {
        free(cmd->message);
    }

    if (cmd->isGame) {
        freeGameCreateCallbackWaitParam((struct gameCreateCallbackWaitParam *) cmd->param);
    }

    free(cmd);
}

/**
 * Length should include the null terminator!
 * Cleans out rogue null terminators and unprintable chars.
 *
 * Returns 1 if chars were cleaned
 */
static int cleanUpStringInput(char *str, int length) {
    int cleaned = 0;

    for (int i = 0; i < length - 1; i++) {
        if (str[i] < 32 || str[i] > 126) {
            str[i] = '?';
            cleaned = 1;
        }
    }

    //Sanity check - double check the null terminator at the end is there
    str[length - 1] = 0;

    return cleaned;
}

/**
 * Free the char* manually.
 * Names with slashes i.e: Test Tournament/Finals/Match 6 will be saved as as
 * REPLAY_FOLDER/Test Tournament/Finals/replay-Match 6-GAME_ID.cor
 * Names without slashes will be saved as replay-NAME-GAMEID.cor
 *
 * Set baseDIR to NULL if you do not want to make the folders yet
 */
char *getReplayFileName(int gameID,
                        const char *gameNameUnfiltered,
                        int length,
                        char *baseDIR) {
    //Allow non-null terminated data to work
    char *gameNameCP = (char *) malloc(sizeof(char) * (length + 1));
    strncpy(gameNameCP, gameNameUnfiltered, length + 1);

    if (cleanUpStringInput(gameNameCP, length + 1)) {
        printf("[WARNING]: A tournament with unprintable chars in its name was detected! All unprintable chars were changed to underscores.\n");
    }

    int makeDIR = baseDIR != NULL;

    /**
     * Security check remove all ../
     */
    for (int i = 0; i < length; i++) {
        if (gameNameCP[i] == '.' && i + 2 < length) {
            if (gameNameCP[i + 1] == '.' && gameNameCP[i + 2] == '/') {
                gameNameCP[i] = '_';
                gameNameCP[i + 1] = '_';
                gameNameCP[i + 2] = '_';

                if (makeDIR) {
                    printf("[WARNING]: A tournament tried to create a replay in ../ in the path but was stopped. ");
                    printf("The ../ (slash) was changed to ___.\n");
                }
            }
        } else if (gameNameCP[i] == '/' && i + 1 < length) {
            gameNameCP[i + 1] = (gameNameCP[i + 1] == '/') * '_'
                              + (gameNameCP[i + 1] != '/') * gameNameCP[i + 1];
        }
    }

    /**
     * Security check - do not use abs path!!!
     * Stop writing to an absolute path
     * */
    if (gameNameCP[0] == '/') {
        gameNameCP[0] = '_';

        if (makeDIR) {
            printf("[WARNING]: A tournament tried to create a replay in an absolute path but was stopped. ");
            printf("The Leading / was changed to an _.\n");
        }
    }

    // Get DIR struct
    int tempFolderBaseLength = 1;

    if (makeDIR) {
        tempFolderBaseLength += strnlen(baseDIR, BUFFER_LENGTH);
    }

    char *tempFolderName = (char *) malloc(sizeof(char)
                         * (tempFolderBaseLength + length + 2));

    int lastSlash = -1;

    // Make dir if it is not already made
    if (makeDIR && baseDIR != NULL) {
        DIR* dir = opendir(baseDIR);
        if (dir) {
            // Directory exists so close it
            closedir(dir);
        } else if (ENOENT == errno) {
            // Directory does not exist so make directory
            int code = mkdir(baseDIR, 0700);

            if (code == -1) {
                printf("[ERROR]: Failed to created the folder %s while getting replay name ready.\n",
                    baseDIR);

                switch (errno) {
                    case EACCES :
                        printf("-> The parent directory does not allow write.\n");
                        break;

                    case EEXIST:
                        printf("-> Pathname already exists.\n");
                        break;

                    case ENAMETOOLONG:
                        printf("-> Pathname is too long.\n");
                        break;

                    default:
                        perror("-> Mkdir failed.\n");
                        break;
                }
            } else {
                printf("[INFO]: Made dir %s for replays.\n",
                       baseDIR);
            }
        } else if (makeDIR) {
            printf("[ERROR]: Failed to create replay folder.\n");
        }
    } else {
        printf("[ERROR]: baseDIR is NULL, unable to create replay folder.\n");
    }

    // Create dirs
    // For a structure of a/b/c/d/e, this will create a then a/b, a/b/c... then a/b/c/d/e
    for (int i = 0; i < length; i++) {
        if (gameNameCP[i] == '/') {
            lastSlash = i;

            if (baseDIR != NULL) {
                if (tempFolderName != NULL) {
                    free(tempFolderName);
                } else {
                    printf("[ERROR]: Unable to free temp folder name.\n");
                }

                tempFolderName = (char *) malloc(sizeof(char) * (tempFolderBaseLength + i + 2));
                snprintf(tempFolderName,
                         tempFolderBaseLength + i + 1, /* reads up to the current char in gameNameCP */
                         "%s/%s",
                         baseDIR,
                         gameNameCP);

                if (makeDIR) {
                    DIR *dir = opendir(tempFolderName);

                    if (dir) {
                        // Directory exists so close it
                        closedir(dir);
                    } else if (ENOENT == errno) {
                        // Directory does not exist so make directory
                        int code = mkdir(tempFolderName, 0700);

                        if (code == -1) {
                            printf("[ERROR]: Failed to created the folder %s while "
                            "getting replay name ready.\n",
                            tempFolderName);

                            switch (errno) {
                                case EACCES :
                                    printf("-> The parent directory does not allow write.\n");
                                    break;

                                case EEXIST:
                                    printf("-> Pathname already exists.\n");
                                    break;

                                case ENAMETOOLONG:
                                    printf("-> Pathname is too long.\n");
                                    break;

                                default:
                                    perror("-> Mkdir failed.\n");
                                    break;
                            }
                        } else {
                            printf("[INFO]: Made dir %s for replays.\n",
                                tempFolderName);
                        }
                    } else {
                        // Error message
                        printf("[ERROR]: Failed to created the folder %s while "
                        "getting replay name ready.\n",
                        tempFolderName);
                    }
                }
            }
        }
    }

    if (tempFolderName != NULL) {
        free(tempFolderName);
    } else {
        printf("[ERROR]: Unable to free temp folder name.\n");
    }

    /**
     * All replays are stored in the replay folder and the names of the replay
     * are appended to the replay folder after a slash.
     * i.e: REPLAYFOLDER/test/replay-%s-%d.cor
     * */
    char *replayName = (char *) malloc(sizeof(char) * BUFFER_LENGTH);

    if (lastSlash == -1) {
        snprintf(replayName, BUFFER_LENGTH, "replay-%s-%d.cor",
                 gameNameCP,
                 gameID);
    } else {
        snprintf(replayName, BUFFER_LENGTH, "%s-%d.cor",
                 gameNameCP,
                 gameID);
    }

    if (gameNameCP != NULL) {
        free(gameNameCP);
    } else {
        printf("[ERROR]: Unable to free game name cp.\n");
    }

    return replayName;
}

/**
 * Downloads a replay to ./replays/ use getReplayFileName to get the URI
 * -> Fails silently
 * Is subject to cockaspagheti
 */
void replayResponseDownload(struct triceBot *b,
                            const Response_ReplayDownload replay) {
    int len = replay.replay_data().length();
    const char *replayData = replay.replay_data().c_str();

    GameReplay gameReplay;
    if (gameReplay.ParseFromArray(replayData, len)) {
        // Save the replay in another process
        if (fork() == 0) {
            saveReplay(b, gameReplay);
            _exit(0);
        }
    }
}

/**
 * Downloads a replay to ./replays/ use getReplayFileName to get the URI
 * -> Fails silently
 * Is subject to cockaspagheti
 */
void saveReplay(struct triceBot *b,
                const GameReplay gameReplay) {
    long len = gameReplay.ByteSizeLong();
    char *replayData = (char*) malloc(sizeof(char) * len);
    char *fileName = (char *) malloc(sizeof(char) * BUFFER_LENGTH * 2);
    gameReplay.SerializeToArray(replayData, len);

    char *replayName = getReplayFileName(gameReplay.game_info().game_id(),
                                         gameReplay.game_info().description().c_str(),
                                         gameReplay.game_info().description().length(),
                                         b->config.replayFolder);

    snprintf(fileName, BUFFER_LENGTH, "%s/%s", b->config.replayFolder, replayName);
    free(replayName);

    DIR* dir = opendir(b->config.replayFolder);

    if (dir) {
        closedir(dir);
    } else if (ENOENT == errno) {
        // Directory does not exist.
        mkdir(b->config.replayFolder, 0700);
    }

    if (access(fileName, F_OK) == 0) {
        printf("[INFO]: Replay %s exists already, it was not overwritten.\n", fileName);
    } else {
        FILE *replayFile = fopen(fileName, "wb+");

        if (replayFile != NULL) {
            for (int i = 0; i < len; i++) {
                fputc(replayData[i], replayFile);
            }

            fclose(replayFile); //close file like a good boy

            printf("[INFO]: Replay %s saved.\n", fileName);
        } else {
            printf("[ERROR]: An error occurred saving the replay as %s.\n", fileName);
        }
    }

    if (fileName != NULL) {
        free(fileName);
    }
}

/**
 * Handles a server message
 * make it call a user defined function of OnServerMSG
 */
void handleResponse(struct triceBot *b,
                    ServerMessage *newServerMessage) {
    //Response
    if (hasNext(&b->callbackQueue)) {
        const Response response = newServerMessage->response();
        struct pendingCommand *cmd = NULL;

        if (response.cmd_id() != (long unsigned int) -1) {
            cmd = cmdForCMDId(response.cmd_id(), &b->callbackQueue);

            // IF IT IS A GAME CREATE THEN IT FREES ITSELF ON THE SESSION EVENT
            // OR ON TIMEOUT
            // THIS IS IS ALL CAPS AS IT TOOK TOO LONG TO DEBUG BECAUSE I AM STUPID
            if (cmd != NULL) {
                if (cmd->isGame) {
                    enq(cmd, &b->callbackQueue);
                } else {
                    executeCallback(b, cmd, &response);
                }
            }
        }

        if (response.HasExtension(Response_Login::ext)) {
            loginResponse(b, &response, NULL);
        }
    }
}

/**
 * Called when a room listed event is called
 */
static void roomsListed(struct triceBot *b,
                        Event_ListRooms listRooms) {
    pthread_mutex_lock(&b->mutex);

    if (b->magicRoomID == -1) {
        int size = listRooms.room_list_size();
        int found = 0;

        for (int i = 0; i < size && !found; i++) {
            ServerInfo_Room room = listRooms.room_list().Get(i);
            const char *roomName = room.name().c_str();

            if (strncmp(b->config.roomName, roomName, BUFFER_LENGTH) == 0) {
                //Send room join cmd
                Command_JoinRoom roomJoin;
                roomJoin.set_room_id(room.room_id());
                b->magicRoomID = room.room_id();
                
                if (b->roomName != NULL) {
                    free(b->roomName);
                }
                
                int len = strnlen(roomName, BUFFER_LENGTH) + 1;
                b->roomName = (char *) malloc(sizeof(char) * len);
                strncpy(b->roomName, roomName, len);

                CommandContainer cont;
                SessionCommand *c = cont.add_session_command();
                c->MutableExtension(Command_JoinRoom::ext)->CopyFrom(roomJoin);

                struct pendingCommand *cmd = prepCmdNTS(b, cont, -1, -1);
                enq(cmd, &b->sendQueue);

                found = 1;
                printf("[INFO]: Automatic room join being sent for %s.\n",
                       roomName);
            }
        }

        if ((!found) && size > 0) {
            ServerInfo_Room room = listRooms.room_list().Get(0);
            const char *roomName = room.name().c_str();

            //Send room join cmd
            Command_JoinRoom roomJoin;
            roomJoin.set_room_id(room.room_id());
            b->magicRoomID = room.room_id();
            
            if (b->roomName != NULL) {
                free(b->roomName);
            }
            
            int len = strnlen(roomName, BUFFER_LENGTH) + 1;
            b->roomName = (char *) malloc(sizeof(char) * len);
            strncpy(b->roomName, roomName, len);

            CommandContainer cont;
            SessionCommand *c = cont.add_session_command();
            c->MutableExtension(Command_JoinRoom::ext)->CopyFrom(roomJoin);

            struct pendingCommand *cmd = prepCmdNTS(b, cont, -1, -1);
            enq(cmd, &b->sendQueue);

            found = 1;
            printf("[INFO]: Automatic room join being sent for %s, as %s was not found.\n",
                   roomName,
                   b->config.roomName);
        }
    }

    pthread_mutex_unlock(&b->mutex);
}

//Join the room in config
void handleRoomEvent(struct triceBot *b,
                     ServerMessage *newServerMessage) {
    const RoomEvent event = newServerMessage->room_event();

    if (event.HasExtension(Event_JoinRoom::ext)) {
        MACRO_CALL_FUNCTION_PTR_FOR_EVENT(onEventJoinRoom,
                                          Event_JoinRoom)
    } else if (event.HasExtension(Event_LeaveRoom::ext)) {
        MACRO_CALL_FUNCTION_PTR_FOR_EVENT(onEventLeaveRoom,
                                          Event_LeaveRoom)
    } else if (event.HasExtension(Event_RoomSay::ext)) {
        MACRO_CALL_FUNCTION_PTR_FOR_EVENT(onEventRoomSay,
                                          Event_RoomSay)
    }
}

void onReplayDownloadResponse(struct triceBot *b,
                              const Response *resp,
                              void *param) {
    // If resp is NULL the request timed out.
    if (resp == NULL || !resp->HasExtension(Response_ReplayDownload::ext)) {
        int replayID = *(int *) param;
        printf("[ERROR]: Replay %d failed to download.\n",
               replayID);

        Command_ReplayDownload replayDownload;
        replayDownload.set_replay_id(replayID);
        printf("[INFO]: Requested replay %d.\n",
               replayID);

        CommandContainer cont;
        SessionCommand *c = cont.add_session_command();
        c->MutableExtension(Command_ReplayDownload::ext)->CopyFrom(replayDownload);

        struct pendingCommand *cmd = prepCmd(b, cont, -1, -1);
        cmd->param = malloc(sizeof(int));
        *(int *) cmd->param = replayID;
        cmd->callbackFunction = &onReplayDownloadResponse;
        enq(cmd, &b->sendQueue);
    } else {
        replayResponseDownload(b, resp->GetExtension(Response_ReplayDownload::ext));
    }

    if (param != NULL) free(param); // Free the replay id
}

/**
 * Called when a replay is ready
 * Set DOWNLOAD_REPLAYS to 1 with -DDOWNLOAD_REPLAYS=1 to make this be
 * called on any replay.
 * If you want to have smart replay downloading then feel free to modify this
 * code. :)
 * This is subject to cockspaghet tm
 */
static void replayReady(struct triceBot *b,
                        const Event_ReplayAdded replayAdded) {
    ServerInfo_ReplayMatch replays = replayAdded.match_info();
    int size = replays.replay_list_size();

    //Download all replays
    for (int i = 0; i < size; i++) {
        ServerInfo_Replay replay = replays.replay_list().Get(i);
        int replayID = replay.replay_id();

        Command_ReplayDownload replayDownload;
        replayDownload.set_replay_id(replayID);
        printf("[INFO]: Requested replay %d.\n",
               replayID);

        CommandContainer cont;
        SessionCommand *c = cont.add_session_command();
        c->MutableExtension(Command_ReplayDownload::ext)->CopyFrom(replayDownload);

        struct pendingCommand *cmd = prepCmd(b, cont, -1, -1);
        cmd->param = malloc(sizeof(int));
        *(int *) cmd->param = replayID;
        cmd->callbackFunction = &onReplayDownloadResponse;
        enq(cmd, &b->sendQueue);
    }
}

/**
 * Called when a game event occurs
 */
void handleGameEvent(struct triceBot *b,
                     ServerMessage *newServerMessage) {
    GameEventContainer gameEventContainer = newServerMessage->game_event_container();

    int id = gameEventContainer.game_id();

    // These must be done in the same lock otherwise a race condition could occur
    pthread_mutex_lock(&b->gameList.mutex);
    struct game g = getGameWithIDNotRefNTS(&b->gameList, id);
    struct game *currentGame = getGameWithIDNTS(&b->gameList, id);
    pthread_mutex_unlock(&b->gameList.mutex);

    if (g.gameID != -1 && currentGame != NULL) {
        //Track state
        int size = gameEventContainer.event_list_size();

        for (int i = 0; i < size; i++) {
            GameEvent event = gameEventContainer.event_list().Get(i);
            int pid = event.has_player_id() ? event.player_id() : -1;

            if (event.HasExtension(Event_GameStateChanged::ext)) {
                Event_GameStateChanged stateChange = event.GetExtension(
                        Event_GameStateChanged::ext);

                if (currentGame->started && !stateChange.game_started()) {
                    currentGame->started = 0;

                    // Game has ended
                    pthread_mutex_lock(&b->mutex);
                    void (*fn)(struct triceBot *, struct game) = b->onGameEnd;
                    pthread_mutex_unlock(&b->mutex);

                    if (fn != NULL) {
                        fn(b, g);
                    }
                } else if (stateChange.game_started() && !currentGame->started) {
                    currentGame->started = 1;

                    // Game has started
                    pthread_mutex_lock(&b->mutex);
                    void (*fn)(struct triceBot *, struct game) = b->onGameStart;
                    pthread_mutex_unlock(&b->mutex);

                    if (fn != NULL) {
                        fn(b, *currentGame);
                    }
                }

                MACRO_CALL_FUNCTION_PTR_FOR_GAME_EVENT(onGameEventStateChanged,
                                                       Event_GameStateChanged)
            }

            //Add player to list
            if (event.HasExtension(Event_Join::ext)) {
                Event_Join jEvent = event.GetExtension(Event_Join::ext);

                if (jEvent.has_player_properties()) {
                    ServerInfo_PlayerProperties pp = jEvent.player_properties();

                    //Track non-spectator, non-judge players.
                    if (!(pp.spectator() || pp.judge())) {
                        int found = 0;
                        pid = pp.player_id();

                        pthread_mutex_lock(&b->gameList.mutex);
                        for (int i = 0; !found && i < currentGame->playerCount; i++) {
                            if (currentGame->playerArr[i].playerID == pp.player_id()) {
                                found = 1;
                                if (pp.has_ping_seconds()) {
                                    const char *name = pp.user_info().name().c_str();
                                    int len = pp.user_info().name().length() + 1;

                                    char *nameCopy = (char *) malloc(sizeof(char) * len);
                                    strncpy(nameCopy, name, len);

                                    free(currentGame->playerArr[i].playerName);
                                    currentGame->playerArr[i].playerName = nameCopy;
                                }
                            }
                        }
                        pthread_mutex_unlock(&b->gameList.mutex);

                        if (!found) {
                            addPlayer(&b->gameList,
                                      currentGame,
                                      pp.user_info().name().c_str(),
                                      pp.player_id(),
                                      pp.has_ping_seconds() ? pp.ping_seconds() : -2);
                        }
                    }
                }
            }

            //Remove player from list
            if (event.HasExtension(Event_Leave::ext)) {
                // Call this before the player leaves
                MACRO_CALL_FUNCTION_PTR_FOR_GAME_EVENT(onGameEventLeave,
                                                       Event_Leave)

                removePlayer(&b->gameList,
                             currentGame,
                             event.player_id());
            }

            //Change player ping
            if (event.HasExtension(Event_PlayerPropertiesChanged::ext)) {
                Event_PlayerPropertiesChanged ppcEvent = event.GetExtension(
                    Event_PlayerPropertiesChanged::ext);

                if (ppcEvent.has_player_properties()) {
                    ServerInfo_PlayerProperties pp = ppcEvent.player_properties();

                    //Track non-spectator, non-judge players.
                    if (pp.has_spectator() && pp.has_judge()) {
                        if (!(pp.spectator() || pp.judge())) {
                            int found = 0;

                            pthread_mutex_lock(&b->gameList.mutex);
                            for (int i = 0; !found && i < currentGame->playerCount; i++) {
                                if (currentGame->playerArr[i].playerID == pp.player_id()) {
                                    found = 1;
                                    if (pp.has_ping_seconds()) {
                                        currentGame->playerArr[i].ping = pp.ping_seconds();
                                    }
                                }
                            }
                            pthread_mutex_unlock(&b->gameList.mutex);

                            if (!found) {
                                addPlayer(&b->gameList,
                                        currentGame,
                                        pp.user_info().has_name() ?
                                            "Uknown player.\0" :
                                            pp.user_info().name().c_str(),
                                        pp.player_id(),
                                        pp.has_ping_seconds() ? pp.ping_seconds() : -2);
                            }
                        }
                    }
                }
            }

            //Game event call
            MACRO_CALL_FUNCTION_PTR_FOR_GAME_EVENT(onGameEventJoin,
                                                   Event_Join)
            else MACRO_CALL_FUNCTION_PTR_FOR_GAME_EVENT(onGameEventGameClosed,
                                                        Event_GameClosed)
            else MACRO_CALL_FUNCTION_PTR_FOR_GAME_EVENT(onGameEventHostChanged,
                                                        Event_GameHostChanged)
            else MACRO_CALL_FUNCTION_PTR_FOR_GAME_EVENT(onGameEventPlayerKicked,
                                                        Event_Kicked)
            else MACRO_CALL_FUNCTION_PTR_FOR_GAME_EVENT(onGameEventPlayerPropertyChanged,
                                                        Event_PlayerPropertiesChanged)
            else MACRO_CALL_FUNCTION_PTR_FOR_GAME_EVENT(onGameEventGameSay,
                                                        Event_GameSay)
            else MACRO_CALL_FUNCTION_PTR_FOR_GAME_EVENT(onGameEventCreateArrow,
                                                        Event_CreateArrow)
            else MACRO_CALL_FUNCTION_PTR_FOR_GAME_EVENT(onGameEventDeleteArrow,
                                                        Event_DeleteArrow)
            else MACRO_CALL_FUNCTION_PTR_FOR_GAME_EVENT(onGameEventCreateCounter,
                                                        Event_CreateCounter)
            else MACRO_CALL_FUNCTION_PTR_FOR_GAME_EVENT(onGameEventSetCounter,
                                                        Event_SetCounter)
            else MACRO_CALL_FUNCTION_PTR_FOR_GAME_EVENT(onGameEventDelCounter,
                                                        Event_DelCounter)
            else MACRO_CALL_FUNCTION_PTR_FOR_GAME_EVENT(onGameEventDrawCards,
                                                        Event_DrawCards)
            else MACRO_CALL_FUNCTION_PTR_FOR_GAME_EVENT(onGameEventRevealCards,
                                                        Event_RevealCards)
            else MACRO_CALL_FUNCTION_PTR_FOR_GAME_EVENT(onGameEventShuffle,
                                                        Event_Shuffle)
            else MACRO_CALL_FUNCTION_PTR_FOR_GAME_EVENT(onGameEventRollDie,
                                                        Event_RollDie)
            else MACRO_CALL_FUNCTION_PTR_FOR_GAME_EVENT(onGameEventMoveCard,
                                                        Event_MoveCard)
            else MACRO_CALL_FUNCTION_PTR_FOR_GAME_EVENT(onGameEventFlipCard,
                                                        Event_FlipCard)
            else MACRO_CALL_FUNCTION_PTR_FOR_GAME_EVENT(onGameEventDestroyCard,
                                                        Event_DestroyCard)
            else MACRO_CALL_FUNCTION_PTR_FOR_GAME_EVENT(onGameEventAttachCard,
                                                        Event_AttachCard)
            else MACRO_CALL_FUNCTION_PTR_FOR_GAME_EVENT(onGameEventCreateToken,
                                                        Event_CreateToken)
            else MACRO_CALL_FUNCTION_PTR_FOR_GAME_EVENT(onGameEventSetCardAttr,
                                                        Event_SetCardAttr)
            else MACRO_CALL_FUNCTION_PTR_FOR_GAME_EVENT(onGameEventSetCardCounter,
                                                        Event_SetCardCounter)
            else MACRO_CALL_FUNCTION_PTR_FOR_GAME_EVENT(onGameEventSetActivePlayer,
                                                        Event_SetActivePlayer)
            else MACRO_CALL_FUNCTION_PTR_FOR_GAME_EVENT(onGameEventSetActivePhase,
                                                        Event_SetActivePhase)
            else MACRO_CALL_FUNCTION_PTR_FOR_GAME_EVENT(onGameEventDumpZone,
                                                        Event_DumpZone)
            else MACRO_CALL_FUNCTION_PTR_FOR_GAME_EVENT(onGameEventStopDumpZone,
                                                        Event_StopDumpZone)
            else MACRO_CALL_FUNCTION_PTR_FOR_GAME_EVENT(onGameEventChangeZoneProperties,
                                                        Event_ChangeZoneProperties)
            else MACRO_CALL_FUNCTION_PTR_FOR_GAME_EVENT(onGameEventReverseTurn,
                                                        Event_ReverseTurn)

            //Free game. Call after event functions to stop seg fault
            if (event.HasExtension(Event_GameClosed::ext)) {
                //Free game
                removeGame(&b->gameList, currentGame);
            }
        }
    }

    freeGameCopy(g);
}

static void *freeCmdForGameCreate(void *param) {
    if (param != NULL) {
        struct gameCreateCallbackWaitParam *p = (struct gameCreateCallbackWaitParam *)
        param;

        pthread_mutex_lock(&p->mutex);
        void (*callbackFn)(struct gameCreateCallbackWaitParam *) =
            p->callbackFn;
        pthread_mutex_unlock(&p->mutex);

        callbackFn(p);
        freeGameCreateCallbackWaitParam(p);
    }

    pthread_exit(NULL);
}

/**
 * Called when a game is created
 */
void handleGameCreate(struct triceBot *b,
                      const Event_GameJoined gameCreate) {
    const char *gameName = gameCreate.game_info().description().c_str();
    struct pendingCommand *cmd = gameWithName(&b->callbackQueue,
                                              gameName);

    if (cmd != NULL) {
        //Game create callbackQueue
        struct gameCreateCallbackWaitParam *game = (struct gameCreateCallbackWaitParam *)
            cmd->param;

        //Check for null pointer
        if (game != NULL) {
            //Create and add game item to the list
            addGame(&b->gameList, createGame(gameCreate.game_info().game_id(),
                                             gameCreate.game_info().max_players(),
                                             game->gameName,
                                             game->gameData));
            // The game data should not freed when the create param is.
            game->gameData = {NULL, NULL};

            //Set the game ID (used in callbackFn both cases)
            pthread_mutex_lock(&game->mutex);
            game->gameID = gameCreate.game_info().game_id();
            void (*callbackFn)(struct gameCreateCallbackWaitParam *) =
                game->callbackFn;
            pthread_mutex_unlock(&game->mutex);

            if (callbackFn != NULL) {
                /**
                 * WARNING THE USER IS ASSUMED TO HAVE A POLLING THREAD
                 * OTHERWISE AND WILL LEAK MEMORY IF THE USER DOES NOT FREE
                 * IT.
                 */
                pthread_t t;

                if (pthread_create(&t, NULL, freeCmdForGameCreate, (void *) game) != 0) {
                    printf("[INFO]: Error creating poll thread for game "
                           "create callback, running polling thread on current thread.\n");

                    //Wait for timeout of the game callback in another thread
                    pthread_mutex_lock(&game->mutex);
                    void (*callbackFn)(struct gameCreateCallbackWaitParam *) =
                        game->callbackFn;
                    pthread_mutex_unlock(&game->mutex);

                    callbackFn(game);
                    freeGameCreateCallbackWaitParam(game);
                }
            }
        }
    }
}

/**
 * This method sends a create game command
 * WARNING If the callback is NULL then the param struct is not freed as it
 * assumes that a thread is polling it see apiServer for example.
 * Otherwise the callback is called and then the data is freed.
 * If the bot disconnects while waiting for a callback then it will wait in
 * another thread for the callbackFn to stop being null then free the struct.
 * The game name is cleaned of unprintable chars but the password is not as it
 * is not seen by the user.
 *
 * TL;DR.
 * Callback should not be NULL.
 * If callback is NULL then the param is leaked.
 * If the bot disconnects while waiting for a callback then it will wait in
 * another thread for the callbackFn to stop being null then free the struct.
 */
//These are long type names
struct gameCreateCallbackWaitParam *
sendCreateGameCommand(struct triceBot *b,
                      char *gameName,
                      char *password,
                      int playerCount,
                      int joinAsSpectator,
                      int spectatorsAllowed,
                      int spectatorsCanChat,
                      int spectatorsNeedPassword,
                      int spectatorsCanSeeHands,
                      int onlyRegistered,
                      int onlyBuddies,
                      struct gameData gameData,
                      void (*callbackFn)(struct gameCreateCallbackWaitParam *)) {
    Command_CreateGame createGame;
    int gameNameLength = strnlen(gameName, BUFFER_LENGTH);

    char *gameNameCopy = (char *) malloc(sizeof(char) * gameNameLength + 1);
    strncpy(gameNameCopy, gameName, gameNameLength + 1);
    cleanUpStringInput(gameNameCopy, gameNameLength + 1);

    createGame.set_description(gameNameCopy);
    createGame.set_password(password);
    createGame.set_max_players(playerCount);
    createGame.set_join_as_spectator(joinAsSpectator);
    createGame.set_join_as_judge(joinAsSpectator);

    createGame.set_spectators_allowed(spectatorsAllowed);
    createGame.set_spectators_can_talk(spectatorsCanChat);
    createGame.set_spectators_need_password(spectatorsNeedPassword);
    createGame.set_spectators_see_everything(spectatorsCanSeeHands);

    createGame.set_only_registered(onlyRegistered);
    createGame.set_only_buddies(onlyBuddies);

    CommandContainer cont;
    RoomCommand *rc = cont.add_room_command();
    rc->MutableExtension(Command_CreateGame::ext)->CopyFrom(createGame);

    struct pendingCommand *cmd = prepCmd(b,
                                         cont,
                                         -1,
                                         b->magicRoomID);

    struct gameCreateCallbackWaitParam *param =
        (struct gameCreateCallbackWaitParam *)
        malloc(sizeof(struct gameCreateCallbackWaitParam));

    initGameCreateCallbackWaitParam(param,
                                    gameNameCopy,
                                    gameNameLength,
                                    gameData,
                                    callbackFn);

    cmd->param = (void *) param;
    cmd->isGame = 1;
    enq(cmd, &b->sendQueue);

    return param;
}

/**
 * Calls the handler for each session event
 * CFLAGS for this method
 * Set DOWNLOAD_REPLAYS to 0 if you do not want replays to be automatically downloaded
 * Set LOGIN_AUTOMATICALLY to 0 if you do not  want to automatically login
 * Set JOIN_ROOM_AUTOMATICALLY to 0 if you do not want to automatically join a room
 * All of these CFLAGS default to 1
 * The macros that are specific to this function are above ^^
 */
void handleSessionEvent(struct triceBot *b,
                        ServerMessage *newServerMessage) {
    const SessionEvent event = newServerMessage->session_event();

    if (event.HasExtension(Event_ServerIdentification::ext)) {
#if LOGIN_AUTOMATICALLY
        //Login when the server asks
        printf("[INFO]: Automatic login being sent.\n");
        sendLogin(b);
#endif
        MACRO_CALL_FUNCTION_PTR_FOR_EVENT(onEventServerIdentifictaion,
                                          Event_ServerIdentification)
    } else if (event.HasExtension(Event_ServerCompleteList::ext)) {
        MACRO_CALL_FUNCTION_PTR_FOR_EVENT(onEventServerCompleteList,
                                          Event_ServerCompleteList)
    } else if (event.HasExtension(Event_ServerMessage::ext)) {
        MACRO_CALL_FUNCTION_PTR_FOR_EVENT(onEventServerMessage,
                                          Event_ServerMessage)
    } else if (event.HasExtension(Event_ServerShutdown::ext)) {
        MACRO_CALL_FUNCTION_PTR_FOR_EVENT(onEventServerShutdown,
                                          Event_ServerShutdown)
    } else if (event.HasExtension(Event_ConnectionClosed::ext)) {
        MACRO_CALL_FUNCTION_PTR_FOR_EVENT(onEventConnectionClosed,
                                          Event_ConnectionClosed)
    } else if (event.HasExtension(Event_UserMessage::ext)) {
        MACRO_CALL_FUNCTION_PTR_FOR_EVENT(onEventUserMessage,
                                          Event_UserMessage)
    } else if (event.HasExtension(Event_ListRooms::ext)) {
#if JOIN_ROOM_AUTOMATICALLY
        roomsListed(b,
                    event.GetExtension(Event_ListRooms::ext));
#endif

        MACRO_CALL_FUNCTION_PTR_FOR_EVENT(onEventListRooms,
                                          Event_ListRooms)
    } else if (event.HasExtension(Event_AddToList::ext)) {
        MACRO_CALL_FUNCTION_PTR_FOR_EVENT(onEventAddToList,
                                          Event_AddToList)
    } else if (event.HasExtension(Event_RemoveFromList::ext)) {
        MACRO_CALL_FUNCTION_PTR_FOR_EVENT(onEventRemoveFromList,
                                          Event_RemoveFromList)
    } else if (event.HasExtension(Event_UserJoined::ext)) {
        MACRO_CALL_FUNCTION_PTR_FOR_EVENT(onEventUserJoined,
                                          Event_UserJoined)
    } else if (event.HasExtension(Event_UserLeft::ext)) {
        MACRO_CALL_FUNCTION_PTR_FOR_EVENT(onEventUserLeft,
                                          Event_UserLeft)
    } else if (event.HasExtension(Event_GameJoined::ext)) {
        /**
         * Due to cockatrice spaghetti this is going to have to trigger the
         * callback for the create game command
         */
        handleGameCreate(b,
                         event.GetExtension(Event_GameJoined::ext));

        MACRO_CALL_FUNCTION_PTR_FOR_EVENT(onEventGameJoined,
                                          Event_GameJoined)
    } else if (event.HasExtension(Event_NotifyUser::ext)) {
        MACRO_CALL_FUNCTION_PTR_FOR_EVENT(onEventNotifyUser,
                                          Event_NotifyUser)
    } else if (event.HasExtension(Event_ReplayAdded::ext)) {
#if DOWNLOAD_REPLAYS
        replayReady(b,
                    event.GetExtension(Event_ReplayAdded::ext));
        printf("[INFO]: Automatic replay download being sent.\n");
#endif

        MACRO_CALL_FUNCTION_PTR_FOR_EVENT(onEventReplayAdded,
                                          Event_ReplayAdded)
    }
}

/**
 * Called on mgr_poll
 */
static void botEventHandler(struct mg_connection *c,
                            int ev,
                            void *ev_data,
                            void *fn_data) {
    struct triceBot *b = (struct triceBot *) fn_data;

    if (ev == MG_EV_ERROR) {
        MACRO_CALL_FUNCTION_PTR_FOR_BOT_STATE_CHANGE(onBotConnectionError)
    } else if (ev == MG_EV_WS_OPEN) {
    } else if (ev == MG_EV_WS_MSG) {
        struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;

        ServerMessage newServerMessage;
        if (newServerMessage.ParseFromArray(wm->data.ptr, wm->data.len)) {
            int messageType = newServerMessage.message_type();

#if MEGA_DEBUG
            printf("[MEGA_DEBUG]: %s\n", newServerMessage.DebugString().c_str());
#endif

            /**
             * Call handler is blocked by user code
             */
            if (messageType == RESPONSE) {
                handleResponse(b, &newServerMessage);
            } else if (messageType == SESSION_EVENT) {
                handleSessionEvent(b, &newServerMessage);
            } else if (messageType == GAME_EVENT_CONTAINER) {
                handleGameEvent(b, &newServerMessage);
            } else if (messageType == ROOM_EVENT) {
                handleRoomEvent(b, &newServerMessage);
            }
        } else {
            printf("[ERROR]: Unreadable message received.\n");
        }
    }

    if (ev == MG_EV_POLL) {
        pthread_mutex_lock(&b->mutex);

        if (!b->roomRequested) {
            //Get room list
            CommandContainer cont;
            SessionCommand *c = cont.add_session_command();
            c->MutableExtension(Command_ListRooms::ext);

            struct pendingCommand *cmd = prepCmdNTS(b, cont, -1, -1);
            pthread_mutex_unlock(&b->mutex);
            enq(cmd, &b->sendQueue);
            pthread_mutex_lock(&b->mutex);

            b->roomRequested = 1;
        }

        pthread_mutex_unlock(&b->mutex);

        //new game game timeouts
        long currentTime = time(NULL);

        if (b->lastGameWaitCheck != currentTime) {
            //Iterate over games
            pthread_mutex_lock(&b->mutex);
            pthread_mutex_lock(&b->gameList.mutex);
            struct gameListNode *current = b->gameList.gamesHead,
                                *old = NULL;
            pthread_mutex_unlock(&b->mutex);

            while (current != NULL) {
                int removeGameFlag = 0;

                //Check for game start timeout
                if ((!current->currentGame->started)
                        && (currentTime - current->currentGame->creationTime) > MAX_GAME_WAIT) {
                    pthread_mutex_lock(&b->mutex);

                    Command_LeaveGame leaveGame;
                    CommandContainer cont;
                    GameCommand *gc = cont.add_game_command();
                    gc->MutableExtension(Command_LeaveGame::ext)->CopyFrom(leaveGame);

                    //IDs are set in prepCMD
                    struct pendingCommand *cmd = prepCmdNTS(b,
                                                            cont,
                                                            current->currentGame->gameID,
                                                            b->magicRoomID);

                    printf("[INFO]: Leaving game %d after %d seconds of inactivity.\n",
                           current->currentGame->gameID,
                           MAX_GAME_WAIT);

                    pthread_mutex_unlock(&b->mutex);
                    enq(cmd, &b->sendQueue);
                    removeGameFlag = 1;
                }

                old = current;
                current = current->nextGame;
                if (removeGameFlag) {
                    pthread_mutex_unlock(&b->gameList.mutex);
                    removeGame(&b->gameList, old->currentGame);
                    break;
                }
            }
        }

        pthread_mutex_unlock(&b->gameList.mutex);

        b->lastGameWaitCheck = currentTime;

        //Send commands
        pthread_mutex_lock(&b->mutex);
        int rateLimited;
        if (currentTime == b->lastSend) {
            rateLimited = b->messagesSentThisSecond >= b->config.maxMessagesPerSecond;
        } else {
            rateLimited = 0;
            b->lastSend = currentTime;
            b->config.maxMessagesPerSecond = 0;
        }

        if (!rateLimited) {
            struct pendingCommand *cmd = deq(&b->sendQueue);

            if (cmd != NULL) {
                mg_ws_send(c, cmd->message, cmd->size, WEBSOCKET_OP_BINARY);

                enq(cmd, &b->callbackQueue);

                b->config.maxMessagesPerSecond++;

#if MEGA_DEBUG
                printf("[MEGA_DEBUG]: MSG of length %d and id %d sent\n", cmd->size, cmd->cmdID);
#endif
            }
        }
        pthread_mutex_unlock(&b->mutex);

        //Check for callback that has timed out
        struct pendingCommand *cmd = deq(&b->callbackQueue);

        if (cmd != NULL) {
            if (time(NULL) - cmd->timeSent >= TIMEOUT) {
                executeCallback(b, cmd, NULL);
            } else {
                enq(cmd, &b->callbackQueue);
            }
        }

        //Send ping if needed
        pthread_mutex_lock(&b->mutex);

        if (needsPing(b->lastPingTime)) {
            pthread_mutex_unlock(&b->mutex);
            sendPing(b);
            pthread_mutex_lock(&b->mutex);
            b->lastPingTime = time(NULL);
        }

        pthread_mutex_unlock(&b->mutex);
    }

    pthread_mutex_lock(&b->mutex);

    if (ev == MG_EV_ERROR || ev == MG_EV_CLOSE) {
        b->running = 0;
        // Signal that we're done
    }

    // Close connection if the bot has been told to halt
    if (! b->running) {
        c->is_closing = 1;
    }

    pthread_mutex_unlock(&b->mutex);
}

/**
 * Run on a thread created by startBot
 */
static void *botThread(void *in) {
    struct triceBot *b = (struct triceBot *) in;

    //Init data
    struct mg_mgr mgr;

    pthread_mutex_lock(&b->mutex);

    // Assert that the memory was freed and that the user is not being a fool.
    if (b->gameList.gamesHead != NULL) {
        freeGameList(&b->gameList);
    }
    if (b->sendQueue.head != NULL || b->sendQueue.tail != NULL) {
        freePendingCommandQueue(&b->sendQueue);
    }
    if (b->callbackQueue.head != NULL || b->callbackQueue.tail != NULL) {
        freePendingCommandQueue(&b->callbackQueue);
    }

    initPendingCommandQueue(&b->callbackQueue);
    initPendingCommandQueue(&b->sendQueue);
    initGameList(&b->gameList);

    b->loggedIn = 0;
    b->magicRoomID = -1;
    b->roomRequested = 0;
    b->cmdID = 0;
    pthread_mutex_unlock(&b->mutex);

    // Event handler flips it to true
    struct mg_connection *c;
    mg_mgr_init(&mgr);
    c = mg_ws_connect(&mgr, b->config.cockatriceServer, botEventHandler, b, NULL);

    if (c == NULL) {
        MACRO_CALL_FUNCTION_PTR_FOR_BOT_STATE_CHANGE(onBotConnectionError)
    } else {
        pthread_mutex_lock(&b->mutex);
        int cont = b->running;
        pthread_mutex_unlock(&b->mutex);

        MACRO_CALL_FUNCTION_PTR_FOR_BOT_STATE_CHANGE(onBotConnect)

        while (cont) {
            mg_mgr_poll(&mgr, 100);

            pthread_mutex_lock(&b->mutex);
            cont = b->running;
            pthread_mutex_unlock(&b->mutex);
        }
    }

    MACRO_CALL_FUNCTION_PTR_FOR_BOT_STATE_CHANGE(onBotDisconnect)

    //Free data
    pthread_mutex_lock(&b->mutex);
    freeGameList(&b->gameList);
    freePendingCommandQueue(&b->sendQueue);
    freePendingCommandQueue(&b->callbackQueue);        
    if (b->roomName != NULL) {
        free(b->roomName);
        b->roomName = NULL;
    }
    
    pthread_mutex_unlock(&b->mutex);

    mg_mgr_free(&mgr);

    pthread_exit(NULL);
}

/**
 * Stops the bot
 */
void stopBot(struct triceBot *b) {
    int flag = 0;
    pthread_mutex_lock(&b->mutex);

    if (b->running) {
        b->running = 0;
        flag = 1;
    }

    pthread_mutex_unlock(&b->mutex);

    if (flag) {
        pthread_join(b->pollingThreadBOT, NULL);
    }
}

/**
 * struct triceBot *b -> pointer to the bot to free
 */
void freeBot(struct triceBot *b) {
    pthread_mutex_lock(&b->mutex);

    if (b->running) {
        pthread_mutex_unlock(&b->mutex);
        stopBot(b);
        pthread_mutex_lock(&b->mutex);
    }
    // The data always is freed on stopBot() by the bot's thread see line 1149

    pthread_mutex_unlock(&b->mutex);
    pthread_mutex_destroy(&b->mutex);
}

/**
 * Starts the bot in a pthread
 * returns 1 if it succeeded
 * returns 0 if it had an error
 */
int startBot(struct triceBot *b) {
    pthread_mutex_lock(&b->mutex);
    b->running = 1;
    b->roomName = NULL;
    pthread_mutex_unlock(&b->mutex);

    return pthread_create(&b->pollingThreadBOT, NULL, botThread, (void *) b);
}

/**
 * should be called by the library host to shutdown the library
 */
void killProtoufLib() {
    google::protobuf::ShutdownProtobufLibrary();
}

#endif
