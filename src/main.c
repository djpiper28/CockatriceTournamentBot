#include <stdio.h> 
#include <stdlib.h> 
#include <pthread.h>
#include <string.h>
#include "botconf.h"
#include "apiserver.h"
#include "trice_structs.h"
#include "bot.h"
#include "version.h"
#include "cmd_queue.h"
#include "commands.pb.h"
#include "command_leave_game.pb.h"
#include "room_commands.pb.h"

struct tournamentBot {
    struct Config config;
    struct triceBot b;
    struct apiServer server;
    int running;
};

void stopAll(struct tournamentBot *bot) {
    printf("[INFO]: Stopping bot\n"); 
    
    bot->running = 0;    
    stopServer(&bot->server);
    stopBot(&bot->b);
    
    freeServer(&bot->server);
    freeBot(&bot->b);
}

#define LEN 1024
static void createGameCommandCallback (struct gameCreateCallbackWaitParam *g) {
    printf("[INFO]: Game with name '%s' and ID %d created by user command.\n", 
           g->gameName, 
           g->gameID);
}

int readNum(const char *msg) {
    char *buffer = (char *) malloc(sizeof(char) * LEN);
    
    fgets(buffer, LEN, stdin);
    int len;
    for (len = 0; len < LEN && buffer[len] != ' ' 
                            && buffer[len] != '\n'; len++);
    
    buffer[len] = 0; //null terminator at line end or space or buffer end
    
    int number = atoi(buffer);    
    free(buffer);
    
    return number;
}

void startConsoleListener (struct tournamentBot *bot) {
    int listening = 1;
    char *commandBuffer = (char *) malloc(sizeof(char) * LEN);
    
    while (listening) {
        fgets(commandBuffer, LEN, stdin);
        
        //Parse command
        int len;
        for (len = 0; len < LEN && commandBuffer[len] != ' ' 
                                && commandBuffer[len] != '\n'; len++);
        
        commandBuffer[len] = 0; //null terminator at line end or space or buffer end
        
        if (strncmp ("exit", commandBuffer, LEN) == 0) {
            listening = 0;
        } else if (strncmp ("creategame", commandBuffer, LEN) == 0) {
            printf("Create Game Command:\n");
            
            //Read username
            printf("> Game Name: ");            
            char *gameName = (char *) malloc(sizeof(char) * LEN);
            fgets(gameName, LEN, stdin);
            
            //Remove line feed
            for (len = 0; len < LEN && gameName[len] != '\n'; len++);
            
            gameName[len] = 0; //null terminator at line end or space or buffer end
            
            //Read password
            printf("> Game Password: ");  
            char *password = (char *) malloc(sizeof(char) * LEN);
            fgets(password, LEN, stdin);
                        
            //Remove line feed
            for (len = 0; len < LEN && password[len] != '\n'; len++);
            
            password[len] = 0; //null terminator at line end or space or buffer end
            
            
            sendCreateGameCommand(&bot->b, 
                                  gameName,
                                  password,
                                  readNum("> Player Count: "),    
                                  readNum("> Join as Spectator (1 yes or, 0 no): "), 
                                  readNum("> Spectators Allowed (1 yes or, 0 no): "),                                                         
                                  readNum("> Spectators Can Chat (1 yes or, 0 no): "),
                                  readNum("> Spectators Need Password (1 yes or, 0 no): "),
                                  readNum("> Spectators Can See Hands (1 yes or, 0 no): "),
                                  readNum("> Only Registered (1 yes or, 0 no): "),
                                  readNum("> Only Buddies (1 yes or, 0 no): "),                                                          
                                  &createGameCommandCallback);
            
            free(gameName);
            free(password);
        } else {
            printf("[Error]: No command '%s' found.\n", commandBuffer);
        }
    }
    
    free(commandBuffer);    
    stopAll(bot);
}

#ifndef DEBUG
#define DEBUG 0
#endif

#if DEBUG
#define MACRO_DEBUG_FOR_EVENT(fn, type)\
void DebugFor##fn (struct triceBot *b, type event) {\
    time_t rawtime;\
    struct tm *info;\
    char buffer[80];\
    time(&rawtime);\
    info = localtime( &rawtime );\
    strftime(buffer, 80, "%x - %H:%M:%S %Z", info);\
    \
    printf("[DEBUG] (%s) %s: %s\n",#fn , buffer, event.DebugString().c_str());\
}

#define MACRO_DEBUG_FOR_GAME_EVENT(fn,type)\
void DebugFor##fn (struct triceBot *b, struct game, type event) {\
    time_t rawtime;\
    struct tm *info;\
    char buffer[80];\
    time(&rawtime);\
    info = localtime( &rawtime );\
    strftime(buffer, 80, "%x - %H:%M:%S %Z", info);\
    \
    printf("[DEBUG] %s: %s\n", buffer, #fn);\
}

#define MACRO_DEBUG_FOR_STATE_CHANGE(fn)\
void DebugFor##fn (struct triceBot *b) {\
    time_t rawtime;\
    struct tm *info;\
    char buffer[80];\
    time(&rawtime);\
    info = localtime( &rawtime );\
    strftime(buffer, 80, "%x - %H:%M:%S %Z", info);\
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

//Game events
MACRO_DEBUG_FOR_GAME_EVENT(onGameEventJoin,
                           Event_Join)
MACRO_DEBUG_FOR_GAME_EVENT(onGameEventLeave,
                           Event_Leave)
MACRO_DEBUG_FOR_GAME_EVENT(onGameEventGameClosed,
                           Event_GameClosed)
MACRO_DEBUG_FOR_GAME_EVENT(onGameEventHostChanged,
                           Event_GameHostChanged)
MACRO_DEBUG_FOR_GAME_EVENT(onGameEventPlayerKicked,
                           Event_Kicked)
MACRO_DEBUG_FOR_GAME_EVENT(onGameEventStateChanged,
                           Event_GameStateChanged)
MACRO_DEBUG_FOR_GAME_EVENT(onGameEventPlayerPropertyChanged,
                           Event_PlayerPropertiesChanged)
MACRO_DEBUG_FOR_GAME_EVENT(onGameEventGameSay,
                           Event_GameSay)
MACRO_DEBUG_FOR_GAME_EVENT(onGameEventCreateArrow,
                           Event_CreateArrow)
MACRO_DEBUG_FOR_GAME_EVENT(onGameEventDeleteArrow,
                           Event_DeleteArrow)
MACRO_DEBUG_FOR_GAME_EVENT(onGameEventCreateCounter,
                           Event_CreateCounter)
MACRO_DEBUG_FOR_GAME_EVENT(onGameEventSetCounter,
                           Event_SetCounter)
MACRO_DEBUG_FOR_GAME_EVENT(onGameEventDelCounter,
                           Event_DelCounter)
MACRO_DEBUG_FOR_GAME_EVENT(onGameEventDrawCards,
                           Event_DrawCards)
MACRO_DEBUG_FOR_GAME_EVENT(onGameEventRevealCards,
                           Event_RevealCards)
MACRO_DEBUG_FOR_GAME_EVENT(onGameEventShuffle,
                           Event_Shuffle)
MACRO_DEBUG_FOR_GAME_EVENT(onGameEventRollDie,
                           Event_RollDie)
MACRO_DEBUG_FOR_GAME_EVENT(onGameEventMoveCard,
                           Event_MoveCard)
MACRO_DEBUG_FOR_GAME_EVENT(onGameEventFlipCard,
                           Event_FlipCard)
MACRO_DEBUG_FOR_GAME_EVENT(onGameEventDestroyCard,
                           Event_DestroyCard)
MACRO_DEBUG_FOR_GAME_EVENT(onGameEventAttachCard,
                           Event_AttachCard)
MACRO_DEBUG_FOR_GAME_EVENT(onGameEventCreateToken,
                           Event_CreateToken)
MACRO_DEBUG_FOR_GAME_EVENT(onGameEventSetCardAttr,
                           Event_SetCardAttr)
MACRO_DEBUG_FOR_GAME_EVENT(onGameEventSetCardCounter,
                           Event_SetCardCounter)
MACRO_DEBUG_FOR_GAME_EVENT(onGameEventSetActivePlayer,
                           Event_SetActivePlayer)
MACRO_DEBUG_FOR_GAME_EVENT(onGameEventSetActivePhase,
                           Event_SetActivePhase)
MACRO_DEBUG_FOR_GAME_EVENT(onGameEventDumpZone,
                           Event_DumpZone)
MACRO_DEBUG_FOR_GAME_EVENT(onGameEventStopDumpZone,
                           Event_StopDumpZone)
MACRO_DEBUG_FOR_GAME_EVENT(onGameEventChangeZoneProperties,
                           Event_ChangeZoneProperties)
MACRO_DEBUG_FOR_GAME_EVENT(onGameEventReverseTurn,
                           Event_ReverseTurn)

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
    
    enq(cmd, &b->sendQueue);
}

void onBotDisconnect(struct triceBot *b) {
    startBot(b);
}

int main (int argc, char * args[]) {
    printf("[INFO]: %s\n-> by djpiper28 see %s for git repo.\n",           
           PROG_NAME, GITHUB_REPO);
    printf("-> Version %d.%d\n", VERSION_MAJOR, VERSION_MINOR);
    printf("-> Use the first argument for the mongoose debug level (0,1,2,3 or 4).\n");
    printf("[INFO]: Starting bot\n");
    
    mg_log_set(argc > 1 ? args[1] : "0");
        
    struct tournamentBot bot;
    bot.running = 1;
    readConf(&bot.config); 
    initBot(&bot.b, bot.config);
    initServer(&bot.server, &bot.b, bot.config);
        
    #if DEBUG
    addDebugFunctions(&bot.b);
    #endif
    
    set_onGameEnd(&onGameEnd, &bot.b);
    set_onBotDisconnect(&onBotDisconnect, &bot.b);
    
    startServer(&bot.server);
    startBot(&bot.b);
    
    startConsoleListener(&bot);
}
