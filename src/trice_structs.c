#ifndef TRICE_STRUCTS_
#define TRICE_STRUCTS_
#include "trice_structs.h"

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

//macros to gen functions
#define MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR(fn, type)\
void set_##fn (void (*event) (struct triceBot *, type),\
               struct triceBot *b) {\
    pthread_mutex_lock(&b->mutex);\
    b->fn = event;\
    pthread_mutex_unlock(&b->mutex);\
}

#define MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR_1(fn)\
void set_##fn (void (*event) (struct triceBot *),\
               struct triceBot *b) {\
    pthread_mutex_lock(&b->mutex);\
    b->fn = event;\
    pthread_mutex_unlock(&b->mutex);\
}
//Lock mutex then set b->fn to the event then unlock the mutex

/**
 * For each of the onEvent.* (regex) function pointers in the triceBot structure
 * there is a set_onEvent.* function that is thread safe. They are made with 
 * macros to ensure that they are consistent
 */ 

//Server events
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR(onEventServerIdentifictaion,
                                          Event_ServerIdentification)
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR(onEventServerCompleteList,
                                          Event_ServerCompleteList)
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR(onEventServerMessage, 
                                          Event_ServerMessage)
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR(onEventServerShutdown, 
                                          Event_ServerShutdown)
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR(onEventUserMessage, 
                                          Event_UserMessage)
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR(onEventListRooms, 
                                          Event_ListRooms)
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR(onEventAddToList, 
                                          Event_AddToList)
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR(onEventRemoveFromList, 
                                          Event_RemoveFromList)
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR(onEventUserJoined, 
                                          Event_UserJoined)
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR(onEventUserLeft, 
                                          Event_UserLeft)
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR(onEventGameJoined, 
                                          Event_GameJoined)
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR(onEventNotifyUser, 
                                          Event_NotifyUser)
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR(onEventReplayAdded, 
                                          Event_ReplayAdded)

//Room events
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR(onEventJoinRoom,
                                          Event_JoinRoom)
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR(onEventLeaveRoom,
                                          Event_LeaveRoom)
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR(onEventRoomSay,
                                          Event_RoomSay)

//Setters for bot state updates
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR_1(onBotDisconnect)
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR_1(onBotConnect)
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR_1(onBotConnectionError)

#endif
