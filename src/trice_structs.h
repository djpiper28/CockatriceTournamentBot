#ifndef TRICE_STRUCTS_
#define TRICE_STRUCTS_

#include <pthread.h>
#include "botconf.h"
#include "gamestruct.h"
#include "response.pb.h"

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

struct pendingCommand {
    char *message;
    void *param;
    int size = 0, 
        cmdID = -1, 
        timeSent = -1, 
        isGame = 0;
    void (*callbackFunction)(struct triceBot *b, const Response *resp, void *param);
};

struct pendingCommandQueueNode {
    struct pendingCommand      *payload;
    struct pendingCommandQueueNode *next;
};

struct pendingCommandQueue {
    struct pendingCommandQueueNode *head, *tail;
    pthread_mutex_t mutex;
};

/**
 * This macro is to ensure that all of the function pointers for a session event
 * are consistent and to reduce the amount of code that I have to type.
 * A good interactive tester for macro expansions is https://godbolt.org/ with
 * gcc arguments of -E
 */ 
#define MACRO_CREATE_SERVER_EVENT_FUNCTION_PTR(fn, type)\
void (*fn) (struct triceBot *, type);

#define MACRO_CREATE_SERVER_EVENT_FUNCTION_PTR_1(fn)\
void (*fn) (struct triceBot *);

enum disconnectReason {
    SERVER_KICK,
    SERVER_DISCONNECT,
    WEBSOCKET_CLOSE
};

struct triceBot {
    struct Config config;
    
    pthread_t pollingThreadBOT;
    pthread_mutex_t mutex;
    struct pendingCommandQueue sendQueue, callbackQueue;
    struct gameList gameList;
    int loggedIn, 
        magicRoomID, 
        roomRequested,
        cmdID,
        running,
        id;
    long lastPingTime; 
    
    //Event Function Pointers for session events    
    MACRO_CREATE_SERVER_EVENT_FUNCTION_PTR(onEventServerIdentifictaion, Event_ServerIdentification)
    MACRO_CREATE_SERVER_EVENT_FUNCTION_PTR(onEventServerCompleteList, Event_ServerCompleteList)
    MACRO_CREATE_SERVER_EVENT_FUNCTION_PTR(onEventServerMessage, Event_ServerMessage)
    MACRO_CREATE_SERVER_EVENT_FUNCTION_PTR(onEventServerShutdown, Event_ServerShutdown)
    MACRO_CREATE_SERVER_EVENT_FUNCTION_PTR(onEventConnectionClosed, Event_ConnectionClosed)
    MACRO_CREATE_SERVER_EVENT_FUNCTION_PTR(onEventUserMessage, Event_UserMessage)
    MACRO_CREATE_SERVER_EVENT_FUNCTION_PTR(onEventListRooms, Event_ListRooms)
    MACRO_CREATE_SERVER_EVENT_FUNCTION_PTR(onEventAddToList, Event_AddToList)
    MACRO_CREATE_SERVER_EVENT_FUNCTION_PTR(onEventRemoveFromList, Event_RemoveFromList)
    MACRO_CREATE_SERVER_EVENT_FUNCTION_PTR(onEventUserJoined, Event_UserJoined)
    MACRO_CREATE_SERVER_EVENT_FUNCTION_PTR(onEventUserLeft, Event_UserLeft)
    MACRO_CREATE_SERVER_EVENT_FUNCTION_PTR(onEventGameJoined, Event_GameJoined)
    MACRO_CREATE_SERVER_EVENT_FUNCTION_PTR(onEventNotifyUser, Event_NotifyUser)
    MACRO_CREATE_SERVER_EVENT_FUNCTION_PTR(onEventReplayAdded, Event_ReplayAdded)
    
    //Bot state changes
    MACRO_CREATE_SERVER_EVENT_FUNCTION_PTR(onBotDisconnect, disconnectReason)
    MACRO_CREATE_SERVER_EVENT_FUNCTION_PTR_1(onBotReconnect)
    MACRO_CREATE_SERVER_EVENT_FUNCTION_PTR_1(onBotConnect)
};

//macros to gen functions
#define MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR(fn, type)\
void set_##fn (\
               void (*event) (struct triceBot *, type),\
               struct triceBot *b) {\
    pthread_mutex_lock(&b->mutex);\
    b->fn = event;\
    pthread_mutex_unlock(&b->mutex);\
}

#define MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR_1(fn)\
void set_##fn (\
               void (*event) (struct triceBot *),\
               struct triceBot *b) {\
    pthread_mutex_lock(&b->mutex);\
    b->fn = event;\
    pthread_mutex_unlock(&b->mutex);\
}

/**
 * For each of the onEvent.* (regex) function pointers in the triceBot structure
 * there is a set_onEvent.* function that is thread safe. They are made with 
 * macros to ensure that they are consistent
 */ 
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR(onEventServerIdentifictaion, Event_ServerIdentification)
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR(onEventServerCompleteList, Event_ServerCompleteList)
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR(onEventServerMessage, Event_ServerMessage)
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR(onEventServerShutdown, Event_ServerShutdown)
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR(onEventUserMessage, Event_UserMessage)
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR(onEventListRooms, Event_ListRooms)
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR(onEventAddToList, Event_AddToList)
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR(onEventRemoveFromList, Event_RemoveFromList)
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR(onEventUserJoined, Event_UserJoined)
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR(onEventUserLeft, Event_UserLeft)
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR(onEventGameJoined, Event_GameJoined)
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR(onEventNotifyUser, Event_NotifyUser)
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR(onEventReplayAdded, Event_ReplayAdded)

//Setters for bot state updates
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR(onBotDisconnect, disconnectReason)
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR_1(onBotReconnect)
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR_1(onBotConnect)

#endif
