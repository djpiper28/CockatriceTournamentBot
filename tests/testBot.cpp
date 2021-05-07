#include "testBot.h"
#include "../src/trice_structs.h"
#include "../src/bot.h"
#include "../src/cmd_queue.h"
#include "../src/bot_conf.h"

CPPUNIT_TEST_SUITE_REGISTRATION(TestBot);

TestBot::TestBot () : CppUnit::TestCase("bot.h tests") {
    
}

#define INIT_TRICE_BOT \
struct Config testConfig = {\
    "username",\
    "password",\
    "room name",\
    "sasdsad",\
    "client id",\
    "replay folder",\
    NULL,\
    NULL,\
    NULL,\
    NULL\
};\
struct triceBot b;\
initBot(&b, testConfig);

void TestBot::testTriceBotInitAndFree() {
    // Test struct triceBot as b
    INIT_TRICE_BOT
    
    // Assert init did all it was intended to do
    
    // Assert that the config is set to be that of the input
    CPPUNIT_ASSERT(b.config.cockatriceUsername == testConfig.cockatriceUsername);
    CPPUNIT_ASSERT(b.config.cockatricePassword == testConfig.cockatricePassword);
    CPPUNIT_ASSERT(b.config.roomName == testConfig.roomName);
    CPPUNIT_ASSERT(b.config.cockatriceServer == testConfig.cockatriceServer);
    CPPUNIT_ASSERT(b.config.clientID == testConfig.clientID);
    CPPUNIT_ASSERT(b.config.replayFolder == testConfig.replayFolder);
    
    // Assert that all internal state vars are set to the correct values
    CPPUNIT_ASSERT(b.magicRoomID == -1);
    CPPUNIT_ASSERT(b.lastPingTime == 0);
    CPPUNIT_ASSERT(b.cmdID == 0);
    CPPUNIT_ASSERT(b.lastSend == 0);
    CPPUNIT_ASSERT(b.lastGameWaitCheck == 0);
    
    //CPPUNIT_ASSERT(b.mutex == PTHREAD_MUTEX_INITIALIZER);
}

void TestBot::testGetReplayFileName() {
    // Base test
    char *filename = getReplayFileName(23, "test", 4, NULL);
    printf(" output >> %s\n", filename);
    CPPUNIT_ASSERT(strcmp(filename, "replay-test-23.cor") == 0);
    free(filename);
    
    // Check ../ is changed to __ so that the directory cannot be escaped
    filename = getReplayFileName(24, "test2../", 8, NULL);
    printf(" output >> %s\n", filename);
    CPPUNIT_ASSERT(strcmp(filename, "replay-test2___-24.cor") == 0);
    free(filename);   
    
    // Check ../ is changed to __ so that the directory cannot be escaped
    filename = getReplayFileName(25, "test2..//", 9, NULL);
    printf(" output >> %s\n", filename);
    CPPUNIT_ASSERT(strcmp(filename, "test2___/-25.cor") == 0);
    free(filename);
    
    // Check double slash is changed
    filename = getReplayFileName(26, "test2//", 7, NULL);
    printf(" output >> %s\n", filename);
    CPPUNIT_ASSERT(strcmp(filename, "test2/_-26.cor") == 0);
    free(filename);
    
    // Test no printf like functions are being used poorly
    filename = getReplayFileName(26, "%stest2//", 9, NULL);
    printf(" output >> %s\n", filename);
    CPPUNIT_ASSERT(strcmp(filename, "%stest2/_-26.cor") == 0);
    free(filename);
}

int started = 0;

static void testStartStopOnEnd(struct triceBot *b) {
    started = 1;
    printf("Bot disconnect\n");
    CPPUNIT_ASSERT(!b->running);
}

void TestBot::testStartStop() {
    INIT_TRICE_BOT
    started = 0;
    set_onBotDisconnect(&testStartStopOnEnd, &b);
    
    printf("Test starting\n");
    
    int status = startBot(&b);
    printf("Bot started\n");
    
    // Check the thread started
    CPPUNIT_ASSERT(status == 0);
    
    stopBot(&b);
    printf("Bot stopped\n");
    
    // Check the thread is ran fully
    CPPUNIT_ASSERT(started);
    CPPUNIT_ASSERT(!b.running);
}

/**
 *  =========================================================================
 *   Tests for all of the event functions being called at the correct time.
 *  =========================================================================
 */

// Function generator macros
#define TEST_EVENT_FN(fn, type)\
int fn##called = 0;\
static void TEST##fn (struct triceBot *b, type event) {\
    fn##called = 1;\
}

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

// Test setter macros
#define TEST_EVENT_FN_SETTER(fn)\
set_##fn (TEST##fn, &b);\
CPPUNIT_ASSERT(b.fn == TEST##fn);

#define TEST_EVENT_FN_CALLED(fn, type) \
ServerMessage serverMessage##fn;\
SessionEvent r##fn;\
r##fn.MutableExtension(type::ext);\
\
serverMessage##fn.set_allocated_session_event(&r##fn);\
handleSessionEvent(&b, &serverMessage##fn);\
CPPUNIT_ASSERT(fn##called == 1);

void TestBot::testEventFunctionsAreCalled() {
    INIT_TRICE_BOT
    
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
    
    TEST_EVENT_FN_CALLED(onEventServerIdentifictaion,
                         Event_ServerIdentification)
    TEST_EVENT_FN_CALLED(onEventServerCompleteList,
                         Event_ServerCompleteList)
    TEST_EVENT_FN_CALLED(onEventServerMessage,
                         Event_ServerMessage)
    TEST_EVENT_FN_CALLED(onEventServerShutdown,
                         Event_ServerShutdown)
    TEST_EVENT_FN_CALLED(onEventConnectionClosed,
                         Event_ConnectionClosed)
    TEST_EVENT_FN_CALLED(onEventUserMessage,
                         Event_UserMessage)
    TEST_EVENT_FN_CALLED(onEventListRooms,
                         Event_ListRooms)
    TEST_EVENT_FN_CALLED(onEventAddToList,
                         Event_AddToList)
    TEST_EVENT_FN_CALLED(onEventRemoveFromList,
                         Event_RemoveFromList)
    TEST_EVENT_FN_CALLED(onEventUserJoined,
                         Event_UserJoined)
    TEST_EVENT_FN_CALLED(onEventUserLeft,
                         Event_UserLeft)
    TEST_EVENT_FN_CALLED(onEventGameJoined,
                         Event_GameJoined)
    TEST_EVENT_FN_CALLED(onEventNotifyUser,
                         Event_NotifyUser)
    TEST_EVENT_FN_CALLED(onEventReplayAdded,
                         Event_ReplayAdded)
}

#define TEST_GAME_EVENT_FN(fn,type)\
int fn##called = 0;\
static void TEST##fn (struct triceBot *b, struct game, type event) {\
    fn##called = 1;\
}

#define TEST_STATE_CHANGE_FN(fn)\
int fn##called = 0;\
static void TEST##fn (struct triceBot *b) {\
    fn##called = 1;\
}

#define TEST_GAME_STATE_CHANGE_FN(fn)\
int fn##called = 0;\
static void TEST##fn (struct triceBot *, struct game) {\
    fn##called = 1;\
}

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
