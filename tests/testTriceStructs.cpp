#include "testTriceStructs.h"
#include "../src/trice_structs.h"
#include "../src/bot.h"
#include "../src/botconf.h"

CPPUNIT_TEST_SUITE_REGISTRATION(TestTriceStructs);

TestTriceStructs::TestTriceStructs () : CppUnit::TestCase("trice_structs.h tests") {
    
}

#define INIT_TRICE_BOT \
struct Config testConfig = {\
    "username",\
    "password",\
    "room name",\
    "server.com",\
    "client id",\
    "replay folder",\
    NULL,\
    NULL,\
    NULL,\
    NULL\
};\
struct triceBot b;\
initBot(&b, testConfig);

// Function generator macros
#define TEST_EVENT_FN(fn, type)\
static void TEST##fn (struct triceBot *b, type event) {}

#define TEST_GAME_EVENT_FN(fn,type)\
static void TEST##fn (struct triceBot *b, struct game, type event) {}

#define TEST_STATE_CHANGE_FN(fn)\
static void TEST##fn (struct triceBot *b) {}

#define TEST_GAME_STATE_CHANGE_FN(fn)\
static void TEST##fn (struct triceBot *, struct game) {}

//Server events
TEST_EVENT_FN(onEventServerIdentifictaion,
              Event_ServerIdentification)
TEST_EVENT_FN(onEventServerCompleteList,
              Event_ServerCompleteList)
TEST_EVENT_FN(onEventServerMessage,
              Event_ServerMessage)
TEST_EVENT_FN(onEventServerShutdown,
              Event_ServerShutdown)
TEST_EVENT_FN(onEventConnectionClosed,
              Event_ConnectionClosed)
TEST_EVENT_FN(onEventUserMessage,
              Event_UserMessage)
TEST_EVENT_FN(onEventListRooms,
              Event_ListRooms)
TEST_EVENT_FN(onEventAddToList,
              Event_AddToList)
TEST_EVENT_FN(onEventRemoveFromList,
              Event_RemoveFromList)
TEST_EVENT_FN(onEventUserJoined,
              Event_UserJoined)
TEST_EVENT_FN(onEventUserLeft,
              Event_UserLeft)
TEST_EVENT_FN(onEventGameJoined,
              Event_GameJoined)
TEST_EVENT_FN(onEventNotifyUser,
              Event_NotifyUser)
TEST_EVENT_FN(onEventReplayAdded,
              Event_ReplayAdded)

//Game events
TEST_GAME_EVENT_FN(onGameEventJoin,
                   Event_Join)
TEST_GAME_EVENT_FN(onGameEventLeave,
                   Event_Leave)
TEST_GAME_EVENT_FN(onGameEventGameClosed,
                   Event_GameClosed)
TEST_GAME_EVENT_FN(onGameEventHostChanged,
                   Event_GameHostChanged)
TEST_GAME_EVENT_FN(onGameEventPlayerKicked,
                   Event_Kicked)
TEST_GAME_EVENT_FN(onGameEventStateChanged,
                   Event_GameStateChanged)
TEST_GAME_EVENT_FN(onGameEventPlayerPropertyChanged,
                   Event_PlayerPropertiesChanged)
TEST_GAME_EVENT_FN(onGameEventGameSay,
                   Event_GameSay)
TEST_GAME_EVENT_FN(onGameEventCreateArrow,
                   Event_CreateArrow)
TEST_GAME_EVENT_FN(onGameEventDeleteArrow,
                   Event_DeleteArrow)
TEST_GAME_EVENT_FN(onGameEventCreateCounter,
                   Event_CreateCounter)
TEST_GAME_EVENT_FN(onGameEventSetCounter,
                   Event_SetCounter)
TEST_GAME_EVENT_FN(onGameEventDelCounter,
                   Event_DelCounter)
TEST_GAME_EVENT_FN(onGameEventDrawCards,
                   Event_DrawCards)
TEST_GAME_EVENT_FN(onGameEventRevealCards,
                   Event_RevealCards)
TEST_GAME_EVENT_FN(onGameEventShuffle,
                   Event_Shuffle)
TEST_GAME_EVENT_FN(onGameEventRollDie,
                   Event_RollDie)
TEST_GAME_EVENT_FN(onGameEventMoveCard,
                   Event_MoveCard)
TEST_GAME_EVENT_FN(onGameEventFlipCard,
                   Event_FlipCard)
TEST_GAME_EVENT_FN(onGameEventDestroyCard,
                   Event_DestroyCard)
TEST_GAME_EVENT_FN(onGameEventAttachCard,
                   Event_AttachCard)
TEST_GAME_EVENT_FN(onGameEventCreateToken,
                   Event_CreateToken)
TEST_GAME_EVENT_FN(onGameEventSetCardAttr,
                   Event_SetCardAttr)
TEST_GAME_EVENT_FN(onGameEventSetCardCounter,
                   Event_SetCardCounter)
TEST_GAME_EVENT_FN(onGameEventSetActivePlayer,
                   Event_SetActivePlayer)
TEST_GAME_EVENT_FN(onGameEventSetActivePhase,
                   Event_SetActivePhase)
TEST_GAME_EVENT_FN(onGameEventDumpZone,
                   Event_DumpZone)
TEST_GAME_EVENT_FN(onGameEventStopDumpZone,
                   Event_StopDumpZone)
TEST_GAME_EVENT_FN(onGameEventChangeZoneProperties,
                   Event_ChangeZoneProperties)
TEST_GAME_EVENT_FN(onGameEventReverseTurn,
                   Event_ReverseTurn)

//Game state changes
TEST_GAME_STATE_CHANGE_FN(onGameStart)
TEST_GAME_STATE_CHANGE_FN(onGameEnd)

//Room events
TEST_EVENT_FN(onEventJoinRoom,
              Event_JoinRoom)
TEST_EVENT_FN(onEventLeaveRoom,
              Event_LeaveRoom)
TEST_EVENT_FN(onEventRoomSay,
              Event_RoomSay)

//State changes
TEST_STATE_CHANGE_FN(onBotDisconnect)
TEST_STATE_CHANGE_FN(onBotConnect)
TEST_STATE_CHANGE_FN(onBotConnectionError)
TEST_STATE_CHANGE_FN(onBotLogin)
TEST_STATE_CHANGE_FN(onReplayDownload)

// Test setter macros
#define TEST_EVENT_FN_SETTER(fn)\
set_##fn (TEST##fn, &b);\
CPPUNIT_ASSERT(b.fn == TEST##fn);

void TestTriceStructs::testTriceBotEventFunctionSetters() {
    // Init trice bot struct
    INIT_TRICE_BOT
    
    // Test each setter
    ///Server events
    TEST_EVENT_FN_SETTER(onEventServerIdentifictaion)
    TEST_EVENT_FN_SETTER(onEventServerCompleteList)
    TEST_EVENT_FN_SETTER(onEventServerMessage)
    TEST_EVENT_FN_SETTER(onEventServerShutdown)
    TEST_EVENT_FN_SETTER(onEventConnectionClosed)
    TEST_EVENT_FN_SETTER(onEventUserMessage)
    TEST_EVENT_FN_SETTER(onEventListRooms)
    TEST_EVENT_FN_SETTER(onEventAddToList)
    TEST_EVENT_FN_SETTER(onEventRemoveFromList)
    TEST_EVENT_FN_SETTER(onEventUserJoined)
    TEST_EVENT_FN_SETTER(onEventUserLeft)
    TEST_EVENT_FN_SETTER(onEventGameJoined)
    TEST_EVENT_FN_SETTER(onEventNotifyUser)
    TEST_EVENT_FN_SETTER(onEventReplayAdded)
    
    //Room events
    TEST_EVENT_FN_SETTER(onEventJoinRoom)
    TEST_EVENT_FN_SETTER(onEventLeaveRoom)
    TEST_EVENT_FN_SETTER(onEventRoomSay)
    
    //Game events
    TEST_EVENT_FN_SETTER(onGameEventJoin)
    TEST_EVENT_FN_SETTER(onGameEventLeave)
    TEST_EVENT_FN_SETTER(onGameEventGameClosed)
    TEST_EVENT_FN_SETTER(onGameEventHostChanged)
    TEST_EVENT_FN_SETTER(onGameEventPlayerKicked)
    TEST_EVENT_FN_SETTER(onGameEventStateChanged)
    TEST_EVENT_FN_SETTER(onGameEventPlayerPropertyChanged)
    TEST_EVENT_FN_SETTER(onGameEventGameSay)
    TEST_EVENT_FN_SETTER(onGameEventCreateArrow)
    TEST_EVENT_FN_SETTER(onGameEventDeleteArrow)
    TEST_EVENT_FN_SETTER(onGameEventCreateCounter)
    TEST_EVENT_FN_SETTER(onGameEventSetCounter)
    TEST_EVENT_FN_SETTER(onGameEventDelCounter)
    TEST_EVENT_FN_SETTER(onGameEventDrawCards)
    TEST_EVENT_FN_SETTER(onGameEventRevealCards)
    TEST_EVENT_FN_SETTER(onGameEventShuffle)
    TEST_EVENT_FN_SETTER(onGameEventRollDie)
    TEST_EVENT_FN_SETTER(onGameEventMoveCard)
    TEST_EVENT_FN_SETTER(onGameEventFlipCard)
    TEST_EVENT_FN_SETTER(onGameEventDestroyCard)
    TEST_EVENT_FN_SETTER(onGameEventAttachCard)
    TEST_EVENT_FN_SETTER(onGameEventCreateToken)
    TEST_EVENT_FN_SETTER(onGameEventSetCardAttr)
    TEST_EVENT_FN_SETTER(onGameEventSetCardCounter)
    TEST_EVENT_FN_SETTER(onGameEventSetActivePlayer)
    TEST_EVENT_FN_SETTER(onGameEventSetActivePhase)
    TEST_EVENT_FN_SETTER(onGameEventDumpZone)
    TEST_EVENT_FN_SETTER(onGameEventStopDumpZone)
    TEST_EVENT_FN_SETTER(onGameEventChangeZoneProperties)
    TEST_EVENT_FN_SETTER(onGameEventReverseTurn)
    
    //Game state changes
    TEST_EVENT_FN_SETTER(onGameStart)
    TEST_EVENT_FN_SETTER(onGameEnd)
    
    //Setters for bot state updates
    TEST_EVENT_FN_SETTER(onBotDisconnect)
    TEST_EVENT_FN_SETTER(onBotConnect)
    TEST_EVENT_FN_SETTER(onBotConnectionError)
    TEST_EVENT_FN_SETTER(onBotLogin)
    TEST_EVENT_FN_SETTER(onReplayDownload)
}

