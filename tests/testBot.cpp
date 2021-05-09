#include <string>

#include "serverinfo_game.pb.h"
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
    CPPUNIT_ASSERT(b.running == 0);
    
    //CPPUNIT_ASSERT(b.mutex == PTHREAD_MUTEX_INITIALIZER);
    freeBot(&b);
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
    printf("Bot started with status %d\n", status);
    
    // Check the thread started
    CPPUNIT_ASSERT(status == 0);
    
    stopBot(&b);
    printf("Bot stopped\n");
    
    // Check the thread is ran fully
    CPPUNIT_ASSERT(started);
    CPPUNIT_ASSERT(!b.running);
    
    freeBot(&b);
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

#define TEST_GAME_EVENT_FN(fn,type)\
int fn##called = 0;\
static void TEST##fn (struct triceBot *b, struct game, type event) {\
    fn##called++;\
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

#define TEST_EVENT_FN_CALLED(fn, type) \
se->MutableExtension(type::ext);\
printf("Testing: %s\n", #fn);\
handleSessionEvent(&b, &serverMessage);\
TEST_EVENT_FN_SETTER(fn)\
handleSessionEvent(&b, &serverMessage);\
se->ClearExtension(type::ext);\
CPPUNIT_ASSERT(fn##called == 1);

void TestBot::testEventFunctionsAreCalled() {
    INIT_TRICE_BOT
    
    ServerMessage serverMessage;
    SessionEvent *se = new SessionEvent();
    serverMessage.set_allocated_session_event(se);
        
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

    serverMessage.release_session_event();
    
    freeBot(&b);
}

#define GAME_EVENTS_PER_CONTAINER 25
#define ID 69
#define COUNT 4
#define TEST_GAME_EVENT_FN_CALLED(fn, type) \
for (int i = 0; i < GAME_EVENTS_PER_CONTAINER; i++) {\
    ge = gc->add_event_list();\
    ge->MutableExtension(type::ext);\
}\
\
fn##called = 0;\
handleGameEvent(&b, &serverMessage);\
TEST_EVENT_FN_SETTER(fn)\
handleGameEvent(&b, &serverMessage);\
gc->clear_event_list();\
printf("Testing: %s (%d)\n", #fn, fn##called);\
if (fn##called != GAME_EVENTS_PER_CONTAINER) printf("FAILED\n");\
CPPUNIT_ASSERT(fn##called == GAME_EVENTS_PER_CONTAINER);

void TestBot::testGameEventFunctionsAreCalled() { 
    INIT_TRICE_BOT
    
    ServerMessage serverMessage;
    GameEventContainer *gc = new GameEventContainer();
    gc->set_game_id(ID);
    serverMessage.set_allocated_game_event_container(gc);
    GameEvent *ge;
    
    addGame(&b.gameList, createGame(ID, COUNT));
    
    TEST_GAME_EVENT_FN_CALLED(onGameEventJoin,
                              Event_Join)
    TEST_GAME_EVENT_FN_CALLED(onGameEventLeave,
                              Event_Leave)
    TEST_GAME_EVENT_FN_CALLED(onGameEventHostChanged,
                              Event_GameHostChanged)
    TEST_GAME_EVENT_FN_CALLED(onGameEventPlayerKicked,
                              Event_Kicked)
    TEST_GAME_EVENT_FN_CALLED(onGameEventStateChanged,
                              Event_GameStateChanged)
    TEST_GAME_EVENT_FN_CALLED(onGameEventPlayerPropertyChanged,
                              Event_PlayerPropertiesChanged)
    TEST_GAME_EVENT_FN_CALLED(onGameEventGameSay,
                              Event_GameSay)
    TEST_GAME_EVENT_FN_CALLED(onGameEventCreateArrow,
                              Event_CreateArrow)
    TEST_GAME_EVENT_FN_CALLED(onGameEventDeleteArrow,
                              Event_DeleteArrow)
    TEST_GAME_EVENT_FN_CALLED(onGameEventCreateCounter,
                              Event_CreateCounter)
    TEST_GAME_EVENT_FN_CALLED(onGameEventSetCounter,
                              Event_SetCounter)
    TEST_GAME_EVENT_FN_CALLED(onGameEventDelCounter,
                              Event_DelCounter)
    TEST_GAME_EVENT_FN_CALLED(onGameEventDrawCards,
                              Event_DrawCards)
    TEST_GAME_EVENT_FN_CALLED(onGameEventRevealCards,
                              Event_RevealCards)
    TEST_GAME_EVENT_FN_CALLED(onGameEventShuffle,
                              Event_Shuffle)
    TEST_GAME_EVENT_FN_CALLED(onGameEventRollDie,
                              Event_RollDie)
    TEST_GAME_EVENT_FN_CALLED(onGameEventMoveCard,
                              Event_MoveCard)
    TEST_GAME_EVENT_FN_CALLED(onGameEventFlipCard,
                              Event_FlipCard)
    TEST_GAME_EVENT_FN_CALLED(onGameEventDestroyCard,
                              Event_DestroyCard)
    TEST_GAME_EVENT_FN_CALLED(onGameEventAttachCard,
                              Event_AttachCard)
    TEST_GAME_EVENT_FN_CALLED(onGameEventCreateToken,
                              Event_CreateToken)
    TEST_GAME_EVENT_FN_CALLED(onGameEventSetCardAttr,
                              Event_SetCardAttr)
    TEST_GAME_EVENT_FN_CALLED(onGameEventSetCardCounter,
                              Event_SetCardCounter)
    TEST_GAME_EVENT_FN_CALLED(onGameEventSetActivePlayer,
                              Event_SetActivePlayer)
    TEST_GAME_EVENT_FN_CALLED(onGameEventSetActivePhase,
                              Event_SetActivePhase)
    TEST_GAME_EVENT_FN_CALLED(onGameEventDumpZone,
                              Event_DumpZone)
    TEST_GAME_EVENT_FN_CALLED(onGameEventStopDumpZone,
                              Event_StopDumpZone)
    TEST_GAME_EVENT_FN_CALLED(onGameEventChangeZoneProperties,
                              Event_ChangeZoneProperties)
    TEST_GAME_EVENT_FN_CALLED(onGameEventReverseTurn,
                              Event_ReverseTurn)
    //TEST_GAME_EVENT_FN_CALLED(onGameEventGameClosed,
    //                          Event_GameClosed)    
        
    for (int i = 0; i < GAME_EVENTS_PER_CONTAINER; i++) {
        ge = gc->add_event_list();
        ge->MutableExtension(Event_GameClosed::ext);
    }
    
    onGameEventGameClosedcalled = 0;
    handleGameEvent(&b, &serverMessage);
    TEST_EVENT_FN_SETTER(onGameEventGameClosed)
    handleGameEvent(&b, &serverMessage);
    gc->clear_event_list();
    printf("Testing: %s (%d)\n", "onGameEventGameClosed", onGameEventGameClosedcalled);\
    if (onGameEventGameClosedcalled != 0) printf("FAILED\n");
    CPPUNIT_ASSERT(onGameEventGameClosedcalled == 0);
    
    freeBot(&b);
}

int callbackSuccess = 0;
static void testCallback(struct triceBot *b, const Response *, void *param) {
    int *ptr = &callbackSuccess;
    callbackSuccess = (int *) param == ptr;
}

int callbackOneCalled = 0;
int callbackTwoCalled = 0;
struct gameCreateCallbackWaitParam *p;

static void testCallbackGameCreateParam(struct triceBot *b, const Response *, void *param) {
    callbackOneCalled = 1;
}

static void testGameCreateCallback(struct gameCreateCallbackWaitParam *g) {
    callbackTwoCalled = g == p;
}

void TestBot::testExecuteCallback() {
    INIT_TRICE_BOT
    CommandContainer cont;
    struct pendingCommand *cmd = prepCmd(&b, cont, -1, -1);
    
    Response response;
    executeCallback(&b, cmd, &response);
    
    cmd = prepCmd(&b, cont, -1, -1);
    cmd->callbackFunction = &testCallback;
    cmd->param = (void *) &callbackSuccess;
    
    executeCallback(&b, cmd, &response);    
    CPPUNIT_ASSERT(callbackSuccess);
    
    cmd = prepCmd(&b, cont, -1, -1);
    char *test = (char *) malloc(sizeof(char) * 5);
    strcpy(test, "test");
    
    p = (struct gameCreateCallbackWaitParam *)
        malloc(sizeof(struct gameCreateCallbackWaitParam));
    initGameCreateCallbackWaitParam(p, test, strlen(test), &testGameCreateCallback);
    cmd->callbackFunction = &testCallbackGameCreateParam;
    cmd->param = (void *) p;
    cmd->isGame = 1;
    
    executeCallback(&b, cmd, &response);    
    CPPUNIT_ASSERT(callbackOneCalled);
    CPPUNIT_ASSERT(!callbackTwoCalled);
        
    freeBot(&b);
}

#define PAGE_SIZE 4096
void TestBot::testReplayDownload() {
    INIT_TRICE_BOT
    
    FILE *f = fopen("../Match 4-84.cor", "r");
    int len = 0,
        bufLen = PAGE_SIZE;
        
    if (f == NULL) {
        printf("FAILED ../Match 4-84.cor does not exist.\n");
        CPPUNIT_ASSERT(f != NULL);
        return;
    }
        
    char *data = (char *) malloc(sizeof(char) * bufLen);
    int tmp;
    
    for (len = 0; (tmp = fgetc(f)) != -1 ; len++) {
        if (len < bufLen) {
            data[len] = tmp;
        } else {
            bufLen += PAGE_SIZE;
            char *temp = (char *) malloc(sizeof(char) * bufLen);
            for (int i = 0; i < len; i++) {
                temp[i] = data[i];
            }
            
            free(data);
            data = temp;
            data[len] = tmp;
        }
    }
    
    fclose(f);    
    rmdir("replay folder/");
    
    GameReplay gameReplay;
    gameReplay.ParseFromArray(data, len);
    saveReplay(&b, gameReplay);
    
    f = fopen("replay folder/e/Match 4-84.cor", "r");
    
    CPPUNIT_ASSERT(f != NULL);
    if (f != NULL) {
        for (int i = 0; i < len; i++) {
            int a = fgetc(f);
            if (a == -1) break;
            
            char b = a & 0xFF;
            if (b != data[i]) {
                printf("FAILED %x %x %d\n", b, data[i], i);
            }
            
            CPPUNIT_ASSERT(b == data[i]);
        }
        
        fclose(f);    
    }
    
    free(data);
    freeBot(&b);
}

pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
int success2 = 0;

#define ID 24
void testFn2(struct gameCreateCallbackWaitParam *p) {
    pthread_mutex_lock(&mutex2);
    success2 = p->gameID == ID;
    if (!success2) {
        printf("[ERROR]: game id was not set!\n");
    }
    pthread_mutex_unlock(&mutex2);
}

#define NAME "test-1"
#define LEN strlen(NAME) + 1
#define MAX_PLAYERS 4
void TestBot::testHandleGameCreate() {
    INIT_TRICE_BOT
    
    initPendingCommandQueue(&b.callbackQueue);    
    initPendingCommandQueue(&b.sendQueue);
    initGameList(&b.gameList);    
    
    b.loggedIn = 0;
    b.magicRoomID = -1;
    b.roomRequested = 0;
    b.cmdID = 0;
    
    CommandContainer cont;
    struct pendingCommand *node = (struct pendingCommand *)
        malloc(sizeof(struct pendingCommand));
    node = (struct pendingCommand *)
        malloc(sizeof(struct pendingCommand));
    
    node->message = (char *) malloc(sizeof(char) * 69);
    node->cmdID = 1;    
    node->isGame = 1;
    
    node->param = malloc(sizeof(struct gameCreateCallbackWaitParam));
    struct gameCreateCallbackWaitParam *param = 
        (struct gameCreateCallbackWaitParam *) node->param;    
    
    char *buff = (char *) malloc(sizeof(char) * LEN);
    snprintf(buff, LEN, "%s", NAME);
    initGameCreateCallbackWaitParam(param, buff, LEN, &testFn2);
    
    enq(node, &b.callbackQueue);
    
    Event_GameJoined game;
    ServerInfo_Game *s = game.mutable_game_info();
    s->set_description(NAME);
    s->set_game_id(ID);
    s->set_max_players(MAX_PLAYERS);
    game.set_judge(0);
    
    handleGameCreate(&b, game);
    
    long start = time(NULL);
    int success_ = 0;
    while (!success_ && (time(NULL) - start < 2)) {
        pthread_mutex_lock(&mutex2);
        success_ = success2;
        pthread_mutex_unlock(&mutex2);
    }
    
    if (!(success_ && success2)) {
        printf("timeout FAILURE\n");
    }
    CPPUNIT_ASSERT(success_ && success2);
    
    freePendingCommandQueue(&b.callbackQueue);    
    freePendingCommandQueue(&b.sendQueue);
    freeGameList(&b.gameList);
    
    freeBot(&b);
}

