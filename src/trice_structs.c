#ifndef TRICE_STRUCTS_
#define TRICE_STRUCTS_
#include "trice_structs.h"

#include <pthread.h>
#include "bot_conf.h"
#include "game_struct.h"
#include "mongoose.h"

#include "response.pb.h"
#include "event_set_card_attr.pb.h"
#include "event_set_card_counter.pb.h"
#include "event_set_active_phase.pb.h"
#include "event_set_active_player.pb.h"
#include "event_dump_zone.pb.h"
#include "event_stop_dump_zone.pb.h"
#include "event_change_zone_properties.pb.h"
#include "event_reverse_turn.pb.h"
#include "event_move_card.pb.h"
#include "event_attach_card.pb.h"
#include "event_create_token.pb.h"
#include "event_roll_die.pb.h"
#include "event_flip_card.pb.h"
#include "event_destroy_card.pb.h"
#include "event_shuffle.pb.h"
#include "event_draw_cards.pb.h"
#include "event_reveal_cards.pb.h"
#include "event_create_counter.pb.h"
#include "event_del_counter.pb.h"
#include "event_set_counter.pb.h"
#include "event_game_say.pb.h"
#include "event_game_host_changed.pb.h"
#include "event_player_properties_changed.pb.h"
#include "event_kicked.pb.h"
#include "event_create_arrow.pb.h"
#include "event_delete_arrow.pb.h"
#include "event_add_to_list.pb.h"
#include "event_remove_from_list.pb.h"
#include "event_connection_closed.pb.h"
#include "event_join.pb.h"
#include "event_leave.pb.h"
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

#define MACRO_THREAD_SAFE_SETTER_FOR_GAME_FUNCTION_PTR(fn, type)\
void set_##fn (void (*event) (struct triceBot *, struct game, type),\
struct triceBot *b) {\
    pthread_mutex_lock(&b->mutex);\
    b->fn = event;\
    pthread_mutex_unlock(&b->mutex);\
}

#define MACRO_THREAD_SAFE_SETTER_FOR_GAME_FUNCTION_PTR_1(fn)\
void set_##fn (void (*event) (struct triceBot *, struct game),\
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
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR(onEventConnectionClosed,
                                          Event_ConnectionClosed)
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

//Game events
MACRO_THREAD_SAFE_SETTER_FOR_GAME_FUNCTION_PTR(onGameEventJoin,
                                               Event_Join)
MACRO_THREAD_SAFE_SETTER_FOR_GAME_FUNCTION_PTR(onGameEventLeave,
                                               Event_Leave)
MACRO_THREAD_SAFE_SETTER_FOR_GAME_FUNCTION_PTR(onGameEventGameClosed,
                                               Event_GameClosed)
MACRO_THREAD_SAFE_SETTER_FOR_GAME_FUNCTION_PTR(onGameEventHostChanged,
                                               Event_GameHostChanged)
MACRO_THREAD_SAFE_SETTER_FOR_GAME_FUNCTION_PTR(onGameEventPlayerKicked,
                                               Event_Kicked)
MACRO_THREAD_SAFE_SETTER_FOR_GAME_FUNCTION_PTR(onGameEventStateChanged,
                                               Event_GameStateChanged)
MACRO_THREAD_SAFE_SETTER_FOR_GAME_FUNCTION_PTR(onGameEventPlayerPropertyChanged,
                                               Event_PlayerPropertiesChanged)
MACRO_THREAD_SAFE_SETTER_FOR_GAME_FUNCTION_PTR(onGameEventGameSay,
                                               Event_GameSay)
MACRO_THREAD_SAFE_SETTER_FOR_GAME_FUNCTION_PTR(onGameEventCreateArrow,
                                               Event_CreateArrow)
MACRO_THREAD_SAFE_SETTER_FOR_GAME_FUNCTION_PTR(onGameEventDeleteArrow,
                                               Event_DeleteArrow)
MACRO_THREAD_SAFE_SETTER_FOR_GAME_FUNCTION_PTR(onGameEventCreateCounter,
                                               Event_CreateCounter)
MACRO_THREAD_SAFE_SETTER_FOR_GAME_FUNCTION_PTR(onGameEventSetCounter,
                                               Event_SetCounter)
MACRO_THREAD_SAFE_SETTER_FOR_GAME_FUNCTION_PTR(onGameEventDelCounter,
                                               Event_DelCounter)
MACRO_THREAD_SAFE_SETTER_FOR_GAME_FUNCTION_PTR(onGameEventDrawCards,
                                               Event_DrawCards)
MACRO_THREAD_SAFE_SETTER_FOR_GAME_FUNCTION_PTR(onGameEventRevealCards,
                                               Event_RevealCards)
MACRO_THREAD_SAFE_SETTER_FOR_GAME_FUNCTION_PTR(onGameEventShuffle,
                                               Event_Shuffle)
MACRO_THREAD_SAFE_SETTER_FOR_GAME_FUNCTION_PTR(onGameEventRollDie,
                                               Event_RollDie)
MACRO_THREAD_SAFE_SETTER_FOR_GAME_FUNCTION_PTR(onGameEventMoveCard,
                                               Event_MoveCard)
MACRO_THREAD_SAFE_SETTER_FOR_GAME_FUNCTION_PTR(onGameEventFlipCard,
                                               Event_FlipCard)
MACRO_THREAD_SAFE_SETTER_FOR_GAME_FUNCTION_PTR(onGameEventDestroyCard,
                                               Event_DestroyCard)
MACRO_THREAD_SAFE_SETTER_FOR_GAME_FUNCTION_PTR(onGameEventAttachCard,
                                               Event_AttachCard)
MACRO_THREAD_SAFE_SETTER_FOR_GAME_FUNCTION_PTR(onGameEventCreateToken,
                                               Event_CreateToken)
MACRO_THREAD_SAFE_SETTER_FOR_GAME_FUNCTION_PTR(onGameEventSetCardAttr,
                                               Event_SetCardAttr)
MACRO_THREAD_SAFE_SETTER_FOR_GAME_FUNCTION_PTR(onGameEventSetCardCounter,
                                               Event_SetCardCounter)
MACRO_THREAD_SAFE_SETTER_FOR_GAME_FUNCTION_PTR(onGameEventSetActivePlayer,
                                               Event_SetActivePlayer)
MACRO_THREAD_SAFE_SETTER_FOR_GAME_FUNCTION_PTR(onGameEventSetActivePhase,
                                               Event_SetActivePhase)
MACRO_THREAD_SAFE_SETTER_FOR_GAME_FUNCTION_PTR(onGameEventDumpZone,
                                               Event_DumpZone)
MACRO_THREAD_SAFE_SETTER_FOR_GAME_FUNCTION_PTR(onGameEventStopDumpZone,
                                               Event_StopDumpZone)
MACRO_THREAD_SAFE_SETTER_FOR_GAME_FUNCTION_PTR(onGameEventChangeZoneProperties,
                                               Event_ChangeZoneProperties)
MACRO_THREAD_SAFE_SETTER_FOR_GAME_FUNCTION_PTR(onGameEventReverseTurn,
                                               Event_ReverseTurn)

//Game state changes
MACRO_THREAD_SAFE_SETTER_FOR_GAME_FUNCTION_PTR_1(onGameStart)
MACRO_THREAD_SAFE_SETTER_FOR_GAME_FUNCTION_PTR_1(onGameEnd)

//Setters for bot state updates
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR_1(onBotDisconnect)
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR_1(onBotConnect)
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR_1(onBotConnectionError)
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR_1(onBotLogin)
MACRO_THREAD_SAFE_SETTER_FOR_FUNCTION_PTR_1(onReplayDownload)
#endif
