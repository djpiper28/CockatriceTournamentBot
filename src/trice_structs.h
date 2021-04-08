#ifndef TRICE_STRUCTS_H_
#define TRICE_STRUCTS_H_

#include <pthread.h>
#include "botconf.h"
#include "gamestruct.h"
#include "response.pb.h"
#include "mongoose.h"

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
    void (*callbackFunction) (struct triceBot *b, const Response *resp, void *param);
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
#define MACRO_CREATE_EVENT_FUNCTION_PTR(fn, type)\
void (*fn) (struct triceBot *, type) = NULL;

#define MACRO_CREATE_EVENT_FUNCTION_PTR_1(fn)\
void (*fn) (struct triceBot *) = NULL;

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
    MACRO_CREATE_EVENT_FUNCTION_PTR(onEventServerIdentifictaion,
                                    Event_ServerIdentification)
    MACRO_CREATE_EVENT_FUNCTION_PTR(onEventServerCompleteList,
                                    Event_ServerCompleteList)
    MACRO_CREATE_EVENT_FUNCTION_PTR(onEventServerMessage,
                                    Event_ServerMessage)
    MACRO_CREATE_EVENT_FUNCTION_PTR(onEventServerShutdown,
                                    Event_ServerShutdown)
    MACRO_CREATE_EVENT_FUNCTION_PTR(onEventConnectionClosed,
                                    Event_ConnectionClosed)
    MACRO_CREATE_EVENT_FUNCTION_PTR(onEventUserMessage,
                                    Event_UserMessage)
    MACRO_CREATE_EVENT_FUNCTION_PTR(onEventListRooms,
                                    Event_ListRooms)
    MACRO_CREATE_EVENT_FUNCTION_PTR(onEventAddToList,
                                    Event_AddToList)
    MACRO_CREATE_EVENT_FUNCTION_PTR(onEventRemoveFromList,
                                    Event_RemoveFromList)
    MACRO_CREATE_EVENT_FUNCTION_PTR(onEventUserJoined,
                                    Event_UserJoined)
    MACRO_CREATE_EVENT_FUNCTION_PTR(onEventUserLeft,
                                    Event_UserLeft)
    MACRO_CREATE_EVENT_FUNCTION_PTR(onEventGameJoined,
                                    Event_GameJoined)
    MACRO_CREATE_EVENT_FUNCTION_PTR(onEventNotifyUser,
                                    Event_NotifyUser)
    MACRO_CREATE_EVENT_FUNCTION_PTR(onEventReplayAdded,
                                    Event_ReplayAdded)
    
    //Event Function Pointers for room events
    MACRO_CREATE_EVENT_FUNCTION_PTR(onEventJoinRoom,
                                    Event_JoinRoom)
    MACRO_CREATE_EVENT_FUNCTION_PTR(onEventLeaveRoom,
                                    Event_LeaveRoom)
    MACRO_CREATE_EVENT_FUNCTION_PTR(onEventRoomSay,
                                    Event_RoomSay)
    
    //Game events
    /*
    JOIN = 1000;
    LEAVE = 1001;
    GAME_CLOSED = 1002;
    GAME_HOST_CHANGED = 1003;
    KICKED = 1004;
    GAME_STATE_CHANGED = 1005;
    PLAYER_PROPERTIES_CHANGED = 1007;
    GAME_SAY = 1009;
    CREATE_ARROW = 2000;
    DELETE_ARROW = 2001;
    CREATE_COUNTER = 2002;
    SET_COUNTER = 2003;
    DEL_COUNTER = 2004;
    DRAW_CARDS = 2005;
    REVEAL_CARDS = 2006;
    SHUFFLE = 2007;
    ROLL_DIE = 2008;
    MOVE_CARD = 2009;
    FLIP_CARD = 2010;
    DESTROY_CARD = 2011;
    ATTACH_CARD = 2012;
    CREATE_TOKEN = 2013;
    SET_CARD_ATTR = 2014;
    SET_CARD_COUNTER = 2015;
    SET_ACTIVE_PLAYER = 2016;
    SET_ACTIVE_PHASE = 2017;
    DUMP_ZONE = 2018;
    STOP_DUMP_ZONE = 2019;
    CHANGE_ZONE_PROPERTIES = 2020;
    REVERSE_TURN = 2021;
    */
    
    //Bot state changes
    MACRO_CREATE_EVENT_FUNCTION_PTR_1(onBotDisconnect)
    MACRO_CREATE_EVENT_FUNCTION_PTR_1(onBotConnect)
    MACRO_CREATE_EVENT_FUNCTION_PTR_1(onBotConnectionError)
};

//macros to gen functions
#define MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR_DEF(fn, type)\
void set_##fn (void (*event) (struct triceBot *, type), struct triceBot *b);

#define MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR_DEF_1(fn)\
void set_##fn (void (*event) (struct triceBot *), struct triceBot *b);
/**
 * For each of the onEvent.* (regex) function pointers in the triceBot structure
 * there is a set_onEvent.* function that is thread safe. They are made with 
 * macros to ensure that they are consistent
 */ 

//Server events
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR_DEF(onEventServerIdentifictaion,
                                              Event_ServerIdentification)
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR_DEF(onEventServerCompleteList,
                                              Event_ServerCompleteList)
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR_DEF(onEventServerMessage, 
                                              Event_ServerMessage)
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR_DEF(onEventServerShutdown, 
                                              Event_ServerShutdown)
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR_DEF(onEventUserMessage, 
                                              Event_UserMessage)
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR_DEF(onEventListRooms, 
                                              Event_ListRooms)
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR_DEF(onEventAddToList, 
                                              Event_AddToList)
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR_DEF(onEventRemoveFromList, 
                                              Event_RemoveFromList)
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR_DEF(onEventUserJoined, 
                                              Event_UserJoined)
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR_DEF(onEventUserLeft, 
                                              Event_UserLeft)
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR_DEF(onEventGameJoined, 
                                              Event_GameJoined)
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR_DEF(onEventNotifyUser, 
                                              Event_NotifyUser)
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR_DEF(onEventReplayAdded, 
                                              Event_ReplayAdded)

//Room events
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR_DEF(onEventJoinRoom,
                                              Event_JoinRoom)
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR_DEF(onEventLeaveRoom,
                                              Event_LeaveRoom)
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR_DEF(onEventRoomSay,
                                              Event_RoomSay)
//Setters for bot state updates
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR_DEF_1(onBotDisconnect)
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR_DEF_1(onBotConnect)
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR_DEF_1(onBotConnectionError)

#endif
