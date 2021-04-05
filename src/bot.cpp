#ifndef BOT_
#define BOT_
#include <stdlib.h>
#include <ncurses.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include "botconf.h"
#include "ncursesinterface.h"
#include "gamestruct.h"
#include "version.h"
#include "version_string.h"
#include "mongoose.h"
#include "running.h"
#include "get_pb_extension.h"
#include "cmd_queue.h"
#include "trice_structs.h"

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
#include "commands.pb.h"
#include "command_leave_game.pb.h"
#include "command_replay_download.pb.h"
#include "command_deck_select.pb.h"
#include "command_ready_start.pb.h"
#include "command_concede.pb.h"

#include "serverinfo_replay_match.pb.h"

#define ID_LENGTH 16
#define PING_FREQUENCY 5
#define TIMEOUT PING_FREQUENCY
#define RESPONSE 0
#define SESSION_EVENT 1
#define GAME_EVENT_CONTAINER 2
#define ROOM_EVENT 3
#define REPLAY_DIR "replays"

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
    
    #if DEBUG
    printw("DEBUG: Login response received.\n");
    refresh();   
    #endif
    
    if (response != NULL) {    
        if (response->HasExtension(Response_Login::ext)) {    
            Response_Login resp = response->GetExtension(Response_Login::ext);
            
            if (response->response_code() == Response::RespOk) {
                b->loggedIn = 1;
                
                attron(GREEN_COLOUR_PAIR);
                printw("INFO: Logged in successfully (%d)\n",
                    response->response_code());
                refresh();  
                attroff(GREEN_COLOUR_PAIR);                       
                
                
                printw("INFO: Requesting room list\n");
                
                //Get room list
                CommandContainer cont;     
                SessionCommand *c = cont.add_session_command(); 
                c->MutableExtension(Command_ListRooms::ext);
                
                struct pendingCommand *cmd = prepCmdNTS(b, cont, -1, -1);  
                enq(cmd, &b->sendQueue);
                
                b->roomRequested = 1;
                
                attron(COLOR_GREEN);
                printw("INFO: Logged in and prepping to join room.\n");
                attroff(COLOR_GREEN);
            } else if (response->response_code() != Response::RespOk) {
                attron(RED_COLOUR_PAIR);
                printw("ERROR: Error logging in. Code: %d, Reason: %s at (unix) %d\n",
                       response->response_code(), 
                       resp.denied_reason_str().c_str(),
                       resp.denied_end_time());
                attroff(RED_COLOUR_PAIR);
                refresh();
                                                
                b->running = 0;
                b->loggedIn = 0;
            }   
        } else {
            attron(RED_COLOUR_PAIR);
            printw("ERROR: Invalid login response.\n");
            attroff(RED_COLOUR_PAIR);
            refresh();
        }
    }    
    
    pthread_mutex_unlock(&b->mutex);
}

//Login
static void sendLogin(struct triceBot *b) {
    pthread_mutex_lock(&b->mutex);
    
    //Login
    printw("INFO: Preparing login.\n");        
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
        
    printw("INFO: Login msg queued.\n");
    refresh();
    
    pthread_mutex_unlock(&b->mutex);
}

/**
 * Executes the callback of the command
 */ 
static void executeCallback(struct triceBot *b, 
                            struct pendingCommand *cmd, 
                            const Response *response) {
    if (cmd->callbackFunction != NULL) {
        #if DEBUG
        printw("DEBUG: Callback call for function with address %d.\n", cmd->callbackFunction); 
        refresh();   
        #endif
        cmd->callbackFunction(b, response, cmd->param);    
    } else {
        #if DEBUG           
        printw("DEBUG: NULL function pointer.\n");
        refresh();   
        #endif
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
        
        if (DEBUG || response.response_code() == -1) {
            printw("DEBUG: Callback received with cmdId: %d and response code:%d debug str: %s.\n",
                   response.cmd_id(), 
                   response.response_code(), 
                   response.ShortDebugString().c_str());
            refresh();
        }
        
        struct pendingCommand *cmd = NULL;
        if (response.cmd_id() != -1) 
            cmd = cmdForCMDId(response.cmd_id(), &b->callbackQueue);   
        
        if (response.HasExtension(Response_Login::ext)) 
            loginResponse(b, &response, NULL);            
        
        if (cmd != NULL) {       
            if (fork() == 0) {
                #if DEBUG   
                printw("DEBUG: CMD found with ID: %d.\n",
                       response.cmd_id());
                refresh();    
                #endif    
                executeCallback(b, cmd, &response);
                exit(0); //Leave the thread                
            }            
        } else {
            attron(RED_COLOUR_PAIR);
            printw("ERROR: Bad response - no CMD found with ID: %d.\n",
                   response.cmd_id());
            attroff(RED_COLOUR_PAIR); 
            refresh();                            
        }        
    } else {
        attron(RED_COLOUR_PAIR);
        printw("ERROR: Server response received with an empty response queue.\n");
        attroff(RED_COLOUR_PAIR);
        refresh();
    }   
}

/**
 * Called when a room listed event is called
 * TODO: change to call a user defined function
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
            printw("INFO: joining a room...\n");
            
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

/**
 * TODO: make call user functions
 */ 
static void handleRoomEvent(struct triceBot *b,
                     ServerMessage *newServerMessage) {
    const RoomEvent roomEvent = newServerMessage->room_event();
    
    if (roomEvent.HasExtension(Event_JoinRoom::ext)) {
        Event_JoinRoom join = roomEvent.GetExtension(Event_JoinRoom::ext);
    } else if (roomEvent.HasExtension(Event_LeaveRoom::ext)) {
        Event_LeaveRoom leave = roomEvent.GetExtension(Event_LeaveRoom::ext);        
    } else if (roomEvent.HasExtension(Event_RoomSay::ext)) {
        Event_RoomSay roomSay = roomEvent.GetExtension(Event_RoomSay::ext);
        #if DEBUG
        printw("INFO: Room message: (%s) %s\n", 
               roomSay.name().c_str(),
               roomSay.message().c_str());
        refresh();
        #endif
    }
}

/**
 * TODO: make call user function
 */ 
static void handleServerMessageEvent(struct triceBot *b,
                               const SessionEvent *sessionEvent) {
    #if DEBUG
    Event_ServerMessage serverMessage = sessionEvent->GetExtension(
                                                    Event_ServerMessage::ext);
    printw("INFO: Server message: %s\n", 
           serverMessage.message().c_str());
    refresh();
    #endif
}

/**
 * Downloads a replay to ./replays/
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
        
        snprintf(fileName, BUFFER_LENGTH,"%s/replay%d.cod", REPLAY_DIR, *replayData);        
        
        //Check is replay directory is made
        DIR* dir = opendir(REPLAY_DIR);
        if (dir != NULL) {
            closedir(dir);
            made = 1;
        } else if (ENOENT == errno) {
            //Make folder
            if (mkdir(REPLAY_DIR, S_IRWXU) != -1) {
                made = 1;
            } else {
                attron(COLOR_RED);
                printw("ERROR: Cannot create replay folder (called replays).\n");
                attroff(COLOR_RED);
                refresh();
                
                made = 0;                
            }
        }
        
        if (made) {            
            if (access(fileName, F_OK) != 0) {
                FILE *replayFile = fopen(fileName, "wb+");
                if (replayFile != NULL == 0 && access(fileName, W_OK) == 0) {
                    for (int i = 0; i < len; i++)
                        fputc(replayData[i], replayFile);
                    
                    fclose (replayFile);    //close file like a good boy
                    attron(YELLOW_COLOUR_PAIR);
                    printw("INFO: Created replay file %s\n", fileName);
                    refresh;
                    attroff(YELLOW_COLOUR_PAIR);
                } else {                    
                    attron(COLOR_RED);
                    printw("ERROR: Cannot create replay file.\n");
                    attroff(COLOR_RED);
                    refresh();
                }
            } else {                
                attron(COLOR_RED);
                printw("ERROR: Replay file exists, not overwritten.\n");
                attroff(COLOR_RED);
                refresh();
            }
        }
        
        if (fileName != NULL)
            free(fileName);        
    } else {
        attron(RED_COLOUR_PAIR);
        printw("ERROR: Invalid replay download response.\n");
        attroff(RED_COLOUR_PAIR);
        refresh();
    }
    
    if (param != NULL)
        free(param);
}

/**
 * Called when a replay is ready
 * TODO: make it call a user function
 */ 
static void replayReady(struct triceBot *b,
                        const SessionEvent *sessionEvent) {
    Event_ReplayAdded replayAdded = sessionEvent
    ->GetExtension(Event_ReplayAdded::ext);
    
    ServerInfo_ReplayMatch replays = replayAdded.match_info();
    int gameID = replays.game_id();
    
    printw("INFO: Replays (\"%s\") in room %s added", replays.game_name().c_str(), 
           replays.room_name().c_str());
    
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
 * TODO: make call a user function
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
    } else {
        attron(RED_COLOUR_PAIR);
        printw("ERROR: No game with id %d found.\n", id);
        attroff(RED_COLOUR_PAIR);
    }
    
    refresh();
}

/**
 * Called when a game is created
 * TODO: make it call a user function
 */ 
static void handleGameCreate(struct triceBot *b,
                             const SessionEvent sessionEvent) {
    if (sessionEvent.HasExtension(Event_GameJoined::ext)) {
        #if DEBUG
        printw("DEBUG: Finding game that matches the id.\n");
        refresh();
        #endif
        
        Event_GameJoined listGames = sessionEvent.GetExtension(Event_GameJoined::ext);
        
        struct pendingCommand *cmd = gameWithName(&b->callbackQueue,
                                                  listGames.game_info().description().c_str());
        
        if (cmd != NULL) {
            //freed in api server. TODO: make API server not spaghetti
            struct gameCreateCallbackWaitParam *game = 
                                (struct gameCreateCallbackWaitParam *) cmd->param;
            
            if (game != NULL) {
                game->gameID = listGames.game_info().game_id();            
            } else {
                printw("ERROR: No callback found for game.\n");
            }
            
            free(game);
        } 
        
        addGame(&b->gameList, createGame(listGames.game_info().game_id()));          
    } else {
        attron(RED_COLOUR_PAIR);
        printw("ERROR: Invalid list games event.\n");
        refresh;
        attroff(RED_COLOUR_PAIR);
    }
}

/**
 * Calls the handler for each session event
 */ 
static void handleSessionEvent(struct triceBot *b,
                               ServerMessage *newServerMessage) {
    //Session event
    const SessionEvent sessionEvent = newServerMessage->session_event();
    
    attron(YELLOW_COLOUR_PAIR);
    if (sessionEvent.HasExtension(Event_ServerIdentification::ext)) {
        printw("INFO: Server identification event received.\n");
        sendLogin(b);   
        
    } else if (sessionEvent.HasExtension(Event_ServerCompleteList::ext)) {
        printw("INFO: Server complete list event received.\n");
        //TODO: handle this properly
        
    } else if (sessionEvent.HasExtension(Event_ServerMessage::ext)) { 
        printw("INFO: Server message event received.\n");    
        handleServerMessageEvent(b, &sessionEvent);
        
    } else if (sessionEvent.HasExtension(Event_ServerShutdown::ext)) {                
        printw("INFO: Server shutdown event received.\n");
        //TODO: handle this properly
        
    } else if (sessionEvent.HasExtension(Event_ConnectionClosed::ext)) {  
        printw("INFO: Connection closed event received.\n"); 
        //TODO: handle this properly
        
    } else if (sessionEvent.HasExtension(Event_UserMessage::ext)) {                        
        printw("INFO: User message event received.\n");
        //TODO: handle this properly
        
    } else if (sessionEvent.HasExtension(Event_ListRooms::ext)) {                
        printw("INFO: List rooms event received.\n");
        roomsListed(b, sessionEvent.GetExtension(Event_ListRooms::ext));       
        
    } else if (sessionEvent.HasExtension(Event_AddToList::ext)) {                
        printw("INFO: Add to list event received.\n");
        //TODO: handle this properly
        
    } else if (sessionEvent.HasExtension(Event_RemoveFromList::ext)) {                
        printw("INFO: Remove from list event received.\n");
        //TODO: handle this properly
        
    } else if (sessionEvent.HasExtension(Event_UserJoined::ext)) {                
        printw("INFO: User join event received.\n");
        //TODO: handle this properly
        
    } else if (sessionEvent.HasExtension(Event_UserLeft::ext)) {                
        printw("INFO: User leave event received.\n");
        //TODO: handle this properly
        
    } else if (sessionEvent.HasExtension(Event_GameJoined::ext)) {                
        printw("INFO: Game joined event received.\n");
        //Due to cockatrice spaghetti this is going to have to trigger the callback 
        //for the create game command        
        handleGameCreate(b, sessionEvent);
        
    } else if (sessionEvent.HasExtension(Event_NotifyUser::ext)) {                
        printw("INFO: Notify user event received.\n");
        //TODO: handle this properly
        
    } else if (sessionEvent.HasExtension(Event_ReplayAdded::ext)) {                
        printw("INFO: Replay ready event received.\n");
        replayReady(b, &sessionEvent);
        
    } else {
        attron(RED_COLOUR_PAIR);
        printw("ERROR: Unknown session event received.\n");
        attroff(RED_COLOUR_PAIR);
    }
    attroff(YELLOW_COLOUR_PAIR);
    
    refresh();
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
        attron(COLOR_PAIR(RED_COLOUR_PAIR));
        printw("ERROR: Websocket error in bot thread.\n");        
        attroff(COLOR_PAIR(RED_COLOUR_PAIR));        
    } else if (ev == MG_EV_WS_OPEN) {
        printw("INFO: Connection started.\n");
        refresh();
    } else if (ev == MG_EV_WS_MSG) { 
        struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;
        
        ServerMessage newServerMessage;
        newServerMessage.ParseFromArray(wm->data.ptr, wm->data.len);
        int messageType = newServerMessage.message_type();
        
        #if DEBUG
        attron(GREEN_COLOUR_PAIR);
        printw("DEBUG: Message received with op code :%d and debug str: %s\n", 
               wm->flags, newServerMessage->ShortDebugString().c_str());
        attroff(GREEN_COLOUR_PAIR);
        refresh();
        #endif
        
        if (messageType == RESPONSE) {
            handleResponse(b, &newServerMessage);            
        } else if (messageType == SESSION_EVENT) {   
            handleSessionEvent(b, &newServerMessage);  
        } else if (messageType == GAME_EVENT_CONTAINER) {   
            handleGameEvent(b, &newServerMessage);     
        } else if (messageType == ROOM_EVENT) {   
            handleRoomEvent(b, &newServerMessage);   
        } else {
            attron(RED_COLOUR_PAIR);
            printw("ERROR: Unknown message received.\n");
            attroff(RED_COLOUR_PAIR);
            refresh();  
        }
        
        b->lastPingTime = time(NULL); // Ping every TIMEOUT with no msg received             
    } if (ev == MG_EV_POLL) {
        if (!b->config.authRequired && !b->roomRequested) {
            printw("INFO: Requesting room list\n");
            
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
            #if DEBUG
            printw("DEBUG: Waiting for callback for cmd with ID: %d.\n",
                   cmd->cmdID);
            refresh();
            #endif
        }
        
        //Check for callback that has timed out
        if (hasNext(&b->callbackQueue)) {
            struct pendingCommand *cmd = peek(&b->callbackQueue);
            
            if (cmd != NULL) {
                if (time(NULL) - cmd->timeSent >= TIMEOUT) {
                    struct pendingCommand *cmdd = deq(&b->callbackQueue);
                    
                    attron(YELLOW_COLOUR_PAIR);
                    printw("INFO: Timeout for cmd with id %d\n",
                           cmdd->cmdID);
                    refresh();
                    attroff(YELLOW_COLOUR_PAIR);
                    
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
            attron(RED_COLOUR_PAIR);
            printw("ERROR: Bot disconnected due to socket close.\n");      
            attroff(RED_COLOUR_PAIR);
            
            refresh();
            b->running = 0;
            // Signal that we're done
    }  
        
    if (! b->running) {
        c->is_closing = 1;        
    }
        
    refresh();                                
}

/**
 * Run on a thread created by startBot
 * TODO: make it call a function on disconnect, the result of which should 
 * decide if it reconnects
 */ 
static void *botThread(void *in) {
    struct triceBot *b = (struct triceBot *) in;
    
    //Reconnect while running
    pthread_mutex_lock(&b->mutex);
    b->running = 1;
    pthread_mutex_unlock(&b->mutex);
    
    while (b->running) {
        struct mg_mgr mgr;
        
        pthread_mutex_lock(&b->mutex);
        b->loggedIn = 0;
        b->magicRoomID = -1;
        b->roomRequested = 0;
        b->id = 0;
        pthread_mutex_unlock(&b->mutex);
        // Event handler flips it to true
        
        printw("INFO: Connecting to the server...\n");
        refresh();
        
        struct mg_connection *c;
        mg_mgr_init(&mgr);
        c = mg_ws_connect(&mgr, b->config.cockatriceServer, botEventHandler, b, NULL);  
        
        // Create client
        while (b->running && c != NULL) {
            mg_mgr_poll(&mgr, 100);
        }    
        
        // Free all
        freeGameList(&b->gameList);
        freePendingCommandQueue(&b->sendQueue);
        freePendingCommandQueue(&b->callbackQueue);
        
        mg_mgr_free(&mgr); 
        
        printw("INFO: Bot thread stopped.\n");
        refresh();   
    }
    
    attron(RED_COLOUR_PAIR);
    printw("INFO: Bot not restarting.\n");
    attroff(RED_COLOUR_PAIR);
    
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
    
    printw("Bot stopped.\n");
    refresh();
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
    printw("Starting bot...\n");
    
    attron(COLOR_PAIR(YELLOW_COLOUR_PAIR));
    printw("INFO: Target cockatrice version: \"%s\" (%s - %s)\n", 
           VERSION_STRING, VERSION_COMMIT, VERSION_DATE);
    attroff(COLOR_PAIR(YELLOW_COLOUR_PAIR));
    
    refresh();    
    
    if (pthread_create(&b->pollingThreadBOT, NULL, botThread, (void *) b)) {
        attron(COLOR_PAIR(RED_COLOUR_PAIR));
        printw("ERROR: Error creating bot thread\n");
        attroff(COLOR_PAIR(RED_COLOUR_PAIR));
        
        refresh(); 
        
        exitCurses();
        return 0;
    }
    
    printw("Bot threads created.\nBot started.\n");
    refresh();
    
    return 1;
}

/**
 * should be called by the library host to shutdown the library
 */ 
void killProtoufLib() {    
    google::protobuf::ShutdownProtobufLibrary();    
}

#endif
