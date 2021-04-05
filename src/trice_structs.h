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
    void (*onEventServerIdentifictaion) (struct triceBot *b,
                                         Event_ServerIdentification event);
    void (*onEventServerCompleteList)   (struct triceBot *b,
                                         Event_ServerCompleteList event);
    void (*onEventServerMessage)        (struct triceBot *b,
                                         Event_ServerMessage event);
    void (*onEventServerShutdown)       (struct triceBot *b,
                                         Event_ServerShutdown event);
    void (*onEventConnectionClosed)     (struct triceBot *b,
                                         Event_ConnectionClosed event);
    void (*onEventUserMessage)          (struct triceBot *b,
                                         Event_UserMessage event);
    void (*onEventListRooms)            (struct triceBot *b,
                                         Event_ListRooms event);
    void (*onEventAddToList)            (struct triceBot *b,
                                         Event_AddToList event);
    void (*onEventRemoveFromList)       (struct triceBot *b,
                                         Event_RemoveFromList event);
    void (*onEventUserJoined)           (struct triceBot *b,
                                         Event_UserJoined event);
    void (*onEventUserLeft)             (struct triceBot *b,
                                         Event_UserLeft event);
    void (*onEventGameJoined)           (struct triceBot *b,
                                         Event_GameJoined event);
    void (*onEventNotifyUser)           (struct triceBot *b,
                                         Event_NotifyUser event);
    void (*onEventReplayAdded)          (struct triceBot *b,
                                         Event_ReplayAdded event);
};

#endif
