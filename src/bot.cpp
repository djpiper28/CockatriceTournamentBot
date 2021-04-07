#ifndef BOT_
#define BOT_
#include "bot.h"

#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include "botconf.h"
#include "gamestruct.h"
#include "version_string.h"
#include "mongoose.h"
#include "get_pb_extension.h"
#include "cmd_queue.h"
#include "trice_structs.h"
#include "botcflags.h"

//Pb imports
#include "room_event.pb.h"
#include "session_event.pb.h"
#include "server_message.pb.h"
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
#define REPLAY_DIR "replays"

/**
 * This macro is to reduce code repetition in the session event method which
 * checks if the pointer is not null then copies it; unlocks the mutex and;
 * finally calls the pointer for the extension of the session event.
 * WARNING mutex must be unlocked before macro
 */ 
#define MACRO_CALL_FUNCTION_PTR_FOR_BOY_STATE_CHANGE(fn)\
if (b->fn != NULL) {\
    pthread_mutex_lock(&b->mutex);\
    void (*fn) (struct triceBot *b)\
    = b->fn;\
    pthread_mutex_unlock(&b->mutex);\
    \
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
if (b->fn != NULL) {\
    pthread_mutex_lock(&b->mutex);\
    void (*fn) (struct triceBot *b,\
    type event)\
    = b->fn;\
    pthread_mutex_unlock(&b->mutex);\
    \
    fn(b,\
       event.GetExtension(type::ext));\
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
    
    b->magicRoomID = -1;
    b->lastPingTime = 0;
    b->id = 0;
}

static struct pendingCommand *prepCmdNTS(struct triceBot *b, 
                                         CommandContainer cont, 
                                         int gameID,
                                         int roomID){        
    if (gameID != -1)
        cont.set_game_id(gameID);
    
    if (roomID != -1)
        cont.set_room_id(roomID);
    
    long msgLength = cont.ByteSizeLong();
    
    char *data = (char*) malloc(sizeof(char) * msgLength);
    cont.SerializeToArray(data, msgLength);
    
    struct pendingCommand *pending = (struct pendingCommand *) 
    malloc(sizeof (struct pendingCommand));
    
    //init pending command
    //returned command is edited by the user before queue addition
    pending->message  = data;
    pending->size     = msgLength; 
    pending->cmdID    = b->id;
    pending->timeSent = time(NULL);
    pending->isGame   = 0;
    pending->callbackFunction = NULL;
    
    #if DEBUG
    char *msg = strToHex(data, msgLength);
    printw("DEBUG: About to send a message: >>\"%s\"<<\n",
           msg);
    printw("With cmdId: %d and debug str: %s\n", 
           id, cont.DebugString().c_str());
    refresh();
    free(msg);        
    #endif
            
    b->cmdID ++;
    //id is in range 0 to 01111...111
    b->cmdID %= 0x7FFFFFFF;
    
    return pending;
}

/**
 * prepCmd takes the following arguments:
 * CommandContainer cont -> the command container for the generated command
 * int gameID -> the ID of the game if the command container should refer to a 
 * game, set to -1 leave the value already in cont the same
 * int roomID -> the ID of the room if the command container should refer to a 
 * game, set to -1 to leave the value already in cont the same
 */ 
struct pendingCommand *prepCmd(struct triceBot *b, 
                               CommandContainer cont, 
                               int gameID, 
                               int roomID) {
    pthread_mutex_lock(&b->mutex);  
    struct pendingCommand *a = prepCmdNTS(b, cont, gameID, roomID);
    pthread_mutex_unlock(&b->mutex);
    
    return a;
}

/**
 * Prepares and empty command container
 * is not used in this code but added for backwards compatibility with old 
 * servatrice server see remotclient.cpp in the cockatrice for the reason
 */ 
struct pendingCommand *prepEmptyCmd(struct triceBot *b) {    
    CommandContainer cont; 
    return prepCmd(b, cont, -1, -1);
}

/**
 * Check to see if the server should be pinged
 */ 
static int needsPing(long lastPingTime) {    
    return time(NULL) - lastPingTime >= PING_FREQUENCY;
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
    enq(cmd, &b->sendQueue);
    
    pthread_mutex_unlock(&b->mutex);
}

/**
 * This part of the library will take a login response and it will check if it 
 * is logged in. If it is not logged in it will TODO: make it do something
 */ 
static void loginResponse(struct triceBot *b, 
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
            } else if (response->response_code() != Response::RespOk) {                     
                b->running = 0;
                b->loggedIn = 0;
            }   
        } else {
        }
    }    
    
    pthread_mutex_unlock(&b->mutex);
}

//Login
static void sendLogin(struct triceBot *b) {
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
static void executeCallback(struct triceBot *b, 
                            struct pendingCommand *cmd, 
                            const Response *response) {
    if (cmd->callbackFunction != NULL) {
        cmd->callbackFunction(b, response, cmd->param);    
    }      
    
    //Free command - we don't like leaky memory
    if (cmd->message != NULL)
        free(cmd->message);   
    
    free(cmd);  
}

/**
 * Handles a server message
 * make it call a user defined function of OnServerMSG
 */
static void handleResponse (struct triceBot *b, 
                            ServerMessage *newServerMessage) {
    //Response
    if (hasNext(&b->callbackQueue)) {                   
        const Response response = newServerMessage->response(); 
        
        struct pendingCommand *cmd = NULL;
        if (response.cmd_id() != -1) 
            cmd = cmdForCMDId(response.cmd_id(), &b->callbackQueue);   
        
        if (response.HasExtension(Response_Login::ext)) 
            loginResponse(b, &response, NULL);            
        
        if (cmd != NULL) { 
            //Fork and execute callback
            if (fork() == 0) {
                #if DEBUG   
                printw("DEBUG: CMD found with ID: %d.\n",
                       response.cmd_id());
                refresh();    
                #endif    
                executeCallback(b, cmd, &response);
                exit(0); //Leave the thread                
            }            
        }       
    }
}

/**
 * Called when a room listed event is called
 */ 
static void roomsListed(struct triceBot *b,
                        Event_ListRooms listRooms) {
    pthread_mutex_lock(&b->mutex);
    
    int size = listRooms.room_list_size();
    int found = 0;
    for (int i = 0; i < size && !found; i++) {
        ServerInfo_Room room = listRooms.room_list().Get(i);
        
        if (strncmp(b->config.roomName, room.name().c_str(), BUFFER_LENGTH) == 0) {
            //Send room join cmd
            Command_JoinRoom roomJoin;
            roomJoin.set_room_id(room.room_id());
            b->magicRoomID = room.room_id();
            
            CommandContainer cont;
            SessionCommand *c = cont.add_session_command(); 
            c->MutableExtension(Command_JoinRoom::ext)->CopyFrom(roomJoin);
            
            struct pendingCommand *cmd = prepCmdNTS(b, cont, -1, -1);  
            enq(cmd, &b->sendQueue);            
            
            found = 1;
        }
    }
    
    pthread_mutex_unlock(&b->mutex);
}

//Join the room in config
static void handleRoomEvent(struct triceBot *b,
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

/**
 * Downloads a replay to ./replays/
 * -> Fails silently
 */ 
void replayResponseDownload(struct triceBot *b,
                            const Response *response, 
                            void *param) {
    if (response == NULL)
        return;
    
    if (response->HasExtension(Response_ReplayDownload::ext)) {
        Response_ReplayDownload replay = response->GetExtension(Response_ReplayDownload::ext);
        
        const char *replayData = replay.replay_data().c_str();
        int len = replay.replay_data().length();
        int made = 0;
        
        int *replayID = (int *) param;
        char *fileName = (char *) malloc(sizeof(char) * BUFFER_LENGTH);
        
        snprintf(fileName, BUFFER_LENGTH,"%s/replay%d.cod", REPLAY_DIR, *replayID);        
        
        //Check is replay directory is made
        DIR* dir = opendir(REPLAY_DIR);
        if (dir != NULL) {
            closedir(dir);
            made = 1;
        } else if (ENOENT == errno) {
            //Make folder
            made = mkdir(REPLAY_DIR, S_IRWXU) != -1;
        }
        
        if (made) {            
            if (access(fileName, F_OK) != 0) {
                FILE *replayFile = fopen(fileName, "wb+");
                if (replayFile != NULL == 0 && access(fileName, W_OK) == 0) {
                    for (int i = 0; i < len; i++)
                        fputc(replayData[i], replayFile);
                    
                    fclose (replayFile);    //close file like a good boy
                }
            }
        }
        
        if (fileName != NULL)
            free(fileName);        
    } 
    
    if (param != NULL)
        free(param);
}

/**
 * Called when a replay is ready
 * Set DOWNLOAD_REPLAYS to 1 with -DDOWNLOAD_REPLAYS=1 to make this be 
 * called on any replay.
 * If you want to have smart replay downloading then feel free to modify this
 * code. :)
 */ 
static void replayReady(struct triceBot *b,
                        const Event_ReplayAdded replayAdded) {    
    ServerInfo_ReplayMatch replays = replayAdded.match_info();
    int size = replays.replay_list_size();    
    
    //Download all replays
    for (int i = 0; i < size; i++) {
        ServerInfo_Replay replay = replays.replay_list().Get(i);
        
        Command_ReplayDownload replayDownload;
        replayDownload.set_replay_id(replay.replay_id());
        
        CommandContainer cont;
        SessionCommand *c = cont.add_session_command(); 
        c->MutableExtension(Command_ReplayDownload::ext)->CopyFrom(replayDownload);
        
        struct pendingCommand *cmd = prepCmd(b, cont, -1, -1);    
        cmd->callbackFunction = &replayResponseDownload;
        
        int *id = (int *) malloc(sizeof(int));
        *id = replay.replay_id();
        cmd->param = (void *) id;
        enq(cmd, &b->sendQueue);
    }    
}

/**
 * Called when a game event occurs
 * TODO: game event functions
 */ 
static void handleGameEvent(struct triceBot *b,
                            ServerMessage *newServerMessage) {    
    GameEventContainer gameEventContainer = newServerMessage->game_event_container();
    
    int id = gameEventContainer.game_id();
    struct game *currentGame = getGameWithID(&b->gameList, id);
    
    if (currentGame != NULL) {
        int size = gameEventContainer.event_list_size();
        for (int i = 0; i < size; i++) {
            GameEvent gameEvent = gameEventContainer.event_list().Get(i);
            
            if (gameEvent.HasExtension(Event_GameStateChanged::ext)) {
                Event_GameStateChanged stateChange = gameEvent.GetExtension(
                    Event_GameStateChanged::ext);
                
                // Game has ended - leave
                if (currentGame->started && !stateChange.game_started()) {
                    // Leave game message
                    Command_LeaveGame leaveGame;                    
                    CommandContainer cont;  
                    cont.set_game_id(id);
                    GameCommand *gc = cont.add_game_command();
                    gc->MutableExtension(Command_LeaveGame::ext)->CopyFrom(leaveGame);
                    
                    struct pendingCommand *cmd = prepCmd(b, cont, -1, b->magicRoomID);
                    
                    enq(cmd, &b->sendQueue);
                } else if (stateChange.game_started()) {
                    currentGame->started = 1;
                }
            }
        }
    }
}

/**
 * Called when a game is created
 */ 
static void handleGameCreate(struct triceBot *b,
                             const Event_GameJoined listGames) {
    #if DEBUG
    printw("DEBUG: Finding game that matches the id.\n");
    refresh();
    #endif
    
    struct pendingCommand *cmd = gameWithName(&b->callbackQueue,
                                              listGames.game_info().description().c_str());
    
    if (cmd != NULL) {
        struct gameCreateCallbackWaitParam *game = 
        (struct gameCreateCallbackWaitParam *) cmd->param;
        
        if (game != NULL) {
            game->gameID = listGames.game_info().game_id();    
            
            if (game->callbackFn != NULL) {
                /**
                 * WARNING THE USER IS ASSUMED TO HAVE A POLLING THREAD 
                 * OTHERWISE AND WILL LEAK MEMORY IF THE USER DOES NOT FREE
                 * IT.
                 */
                if (fork() == 0) {                    
                    game->callbackFn(game);
                    free(game->gameName);
                    free(game);
                    exit(0);
                }   
            }
            
        }
        
        free(game);
    } 
    
    addGame(&b->gameList, createGame(listGames.game_info().game_id()));   
}

/**
 * This method sends a create game command
 * WARNING If the callback is NULL then the param struct is not freed as it 
 * assumes that a thread is polling it see apiServer for example.
 * Otherwise the callback is called and then the data is freed.
 * 
 * TL;DR.
 * Callback should not be NULL.
 * If callback is NULL then the param is leaked.
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
                       void (*callbackFn) (struct gameCreateCallbackWaitParam *)) {
    Command_CreateGame createGame;
    int length;
    for (length = 0; gameName[length] != 0; length++);
    
    char *gameNameCopy = (char *) malloc(sizeof(char) * length + 1);
    for (int i = 0; i < length; i++)
        gameNameCopy[i] = gameName[i];
        
    createGame.set_description(gameNameCopy);
    createGame.set_password(password);
    createGame.set_max_players(playerCount);
    createGame.set_join_as_spectator(joinAsSpectator);
    
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
    
    param->gameName = gameNameCopy;
    param->gameID = -1;
    param->sendTime = time(NULL);
    param->callbackFn = callbackFn;
    
    cmd->param = (void *) &param;        
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
static void handleSessionEvent(struct triceBot *b,
                               ServerMessage *newServerMessage) {
    //Call session event function in new fork
    const SessionEvent event = newServerMessage->session_event();
    
    if (event.HasExtension(Event_ServerIdentification::ext)) {
        #if LOGIN_AUTOMATICALLY
        //Login when the server asks
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
        pthread_mutex_unlock(&b->mutex);
        handleGameCreate(b, 
                         event.GetExtension(Event_GameJoined::ext));
        pthread_mutex_lock(&b->mutex);
        
        MACRO_CALL_FUNCTION_PTR_FOR_EVENT(onEventGameJoined,
                                          Event_GameJoined)
    } else if (event.HasExtension(Event_NotifyUser::ext)) { 
        MACRO_CALL_FUNCTION_PTR_FOR_EVENT(onEventNotifyUser,
                                          Event_NotifyUser)
    } else if (event.HasExtension(Event_ReplayAdded::ext)) {   
        #if DOWNLOAD_REPLAYS
        replayReady(b, 
                    event.GetExtension(Event_ReplayAdded::ext));
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
        MACRO_CALL_FUNCTION_PTR_FOR_BOY_STATE_CHANGE(onBotConnectionError) 
    } else if (ev == MG_EV_WS_OPEN) {        
        MACRO_CALL_FUNCTION_PTR_FOR_BOY_STATE_CHANGE(onBotConnect)        
    } else if (ev == MG_EV_WS_MSG) { 
        struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;
        
        ServerMessage newServerMessage;
        newServerMessage.ParseFromArray(wm->data.ptr, wm->data.len);
        int messageType = newServerMessage.message_type();
        
        /**
         * Call handler in a fork such that the main thread is not blocked by user
         * code
         */
        if (fork() == 0) { 
            if (messageType == RESPONSE) {
                handleResponse(b, &newServerMessage);            
            } else if (messageType == SESSION_EVENT) {   
                handleSessionEvent(b, &newServerMessage);  
            } else if (messageType == GAME_EVENT_CONTAINER) {   
                handleGameEvent(b, &newServerMessage);     
            } else if (messageType == ROOM_EVENT) {   
                handleRoomEvent(b, &newServerMessage);   
            } 
            
            exit(0); //exit fork
        }
        
        b->lastPingTime = time(NULL); // Ping every TIMEOUT with no msg received             
    } if (ev == MG_EV_POLL) {
        if (!b->config.authRequired && !b->roomRequested) {            
            //Get room list
            CommandContainer cont;     
            SessionCommand *c = cont.add_session_command(); 
            c->MutableExtension(Command_ListRooms::ext);
            
            struct pendingCommand *cmd = prepCmd(b, cont, -1, -1);  
            enq(cmd, &b->sendQueue);  
            
            b->roomRequested = 1;
        }
        
        //Send commands
        if (hasNext(&b->sendQueue)) {           
            struct pendingCommand *cmd = deq(&b->sendQueue);   
            
            mg_ws_send(c, cmd->message, cmd->size, WEBSOCKET_OP_BINARY);            
            
            enq(cmd, &b->callbackQueue);
        }
        
        //Check for callback that has timed out
        if (hasNext(&b->callbackQueue)) {
            struct pendingCommand *cmd = peek(&b->callbackQueue);
            
            if (cmd != NULL) {
                if (time(NULL) - cmd->timeSent >= TIMEOUT) {
                    struct pendingCommand *cmdd = deq(&b->callbackQueue);
                    executeCallback(b, cmdd, NULL);                        
                }
            }
        }            
        
        //Send ping if needed
        if (needsPing(b->lastPingTime)) {
            sendPing(b);
            b->lastPingTime = time(NULL);
        }    
    } 
    
    if (ev == MG_EV_ERROR || ev == MG_EV_CLOSE) {
            b->running = 0;
            // Signal that we're done
    }  
        
    //Close connection if the bot has been told to halt
    if (! b->running) {
        c->is_closing = 1;        
    }                            
}

/**
 * Run on a thread created by startBot
 */ 
static void *botThread(void *in) {
    struct triceBot *b = (struct triceBot *) in;
    
    //Reconnect while running
    pthread_mutex_lock(&b->mutex);
    b->running = 1;
    pthread_mutex_unlock(&b->mutex);
    
    //Init data
    struct mg_mgr mgr;
    
    pthread_mutex_lock(&b->mutex);
    b->loggedIn = 0;
    b->magicRoomID = -1;
    b->roomRequested = 0;
    b->id = 0;
    pthread_mutex_unlock(&b->mutex);
    
    // Event handler flips it to true  
    struct mg_connection *c;
    mg_mgr_init(&mgr);
    c = mg_ws_connect(&mgr, b->config.cockatriceServer, botEventHandler, b, NULL);  
    
    if (c == NULL) {
        MACRO_CALL_FUNCTION_PTR_FOR_BOY_STATE_CHANGE(onBotConnectionError)
    } else {
        pthread_mutex_lock(&b->mutex);
        int cont = b->running;        
        pthread_mutex_unlock(&b->mutex);
        
        while (cont) {
            mg_mgr_poll(&mgr, 250);
            
            pthread_mutex_lock(&b->mutex);
            cont = b->running;
            pthread_mutex_unlock(&b->mutex);
        }
        
        MACRO_CALL_FUNCTION_PTR_FOR_BOY_STATE_CHANGE(onBotDisconnect)
    }
    
    //Free data
    freeGameList(&b->gameList);
    freePendingCommandQueue(&b->sendQueue);
    freePendingCommandQueue(&b->callbackQueue);
    
    mg_mgr_free(&mgr);     
    pthread_exit(NULL);
}

/**
 * Stops the bot
 */
void stopBot(struct triceBot *b) {    
    pthread_mutex_lock(&b->mutex);
    b->running = 0;    
    pthread_mutex_unlock(&b->mutex);
    
    pthread_join(b->pollingThreadBOT, NULL);
}

/**
 * struct triceBot *b -> pointer to the bot to free
 */ 
void freeBot(struct triceBot *b) {
    pthread_mutex_lock(&b->mutex);  
    
    if (b->running)
        stopBot(b);
    
    freeGameList(&b->gameList);
    freePendingCommandQueue(&b->sendQueue);    
    freePendingCommandQueue(&b->callbackQueue);
    
    pthread_mutex_unlock(&b->mutex);
    pthread_mutex_destroy(&b->mutex);
}

/**
 * Starts the bot in a pthread
 * returns 1 if it succeeded
 * returns 0 if it had an error
 */ 
int startBot(struct triceBot *b) {
    return pthread_create(&b->pollingThreadBOT, NULL, botThread, (void *) b);
}

/**
 * should be called by the library host to shutdown the library
 */ 
void killProtoufLib() {    
    google::protobuf::ShutdownProtobufLibrary();    
}

#endif