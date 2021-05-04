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

void TestBot::testPrepCMD() {
    // Test struct triceBot as b
    INIT_TRICE_BOT
    
    int id = b.cmdID;
    
    CommandContainer cont;
    
    // Assert that the CMD has the correct initial data
    struct pendingCommand *c = prepCmd(&b, cont, -1, -1);
    
    // Assert message is set
    long msgLength = cont.ByteSizeLong();
    char *data = (char*) malloc(sizeof(char) * msgLength);
    cont.SerializeToArray(data, msgLength);
    
    CPPUNIT_ASSERT(strncmp(data, c->message, msgLength) == 0);
    
    free(data);
    
    // Assert length is set
    CPPUNIT_ASSERT(c->size == msgLength);    
    
    // Assert that the CMD ID has been increased
    CPPUNIT_ASSERT(id + 1 == b.cmdID);
    CPPUNIT_ASSERT(c->cmdID == id);
    
    // Assert that the time was set
    CPPUNIT_ASSERT(time(NULL) - c->timeSent < 1);
    
    // Assert that the rest are set to the default
    CPPUNIT_ASSERT(c->isGame == 0);
    CPPUNIT_ASSERT(c->callbackFunction == NULL);
    
    freePendingCommand(c);
    
    // Assert that roomID and gameID are set when they matter
    for (int i = -1; i < 1; i++) {
        for (int j = -1; j < 1; j++) {
            c = prepCmd(&b, cont, i, j);
            cont.ParseFromArray(c->message, c->size);
            
            int gameID = cont.game_id();
            int roomID = cont.room_id();
            
            if (i != -1) {
                CPPUNIT_ASSERT(i == gameID);
            }
            
            if (j != -1) {
                CPPUNIT_ASSERT(j == roomID);
            }
            
            freePendingCommand(c);
        }
    }
    
    // Assert that the CMD ID resets after 0x7FFFFFFF
    b.cmdID = 0x7FFFFFFE;
    c = prepCmd(&b, cont, -1, -1);
    
    CPPUNIT_ASSERT(b.cmdID == 0);
    CPPUNIT_ASSERT(c->cmdID == 0x7FFFFFFE);
    
    freePendingCommand(c);
}

void TestBot::testPrepEmptyCMD() {
    // Check it makes a cmd that is empty and incs the cmd id
    INIT_TRICE_BOT
        
    int id = b.cmdID;
    struct pendingCommand *c = prepEmptyCmd(&b);    
    
    CommandContainer cont;
    
    // Assert that the CMD ID has been increased
    CPPUNIT_ASSERT(id + 1 == b.cmdID);
    CPPUNIT_ASSERT(c->cmdID == id);
    
    // Assert that the time was set
    CPPUNIT_ASSERT(time(NULL) - c->timeSent < 1);
    
    // Assert message is set
    long msgLength = cont.ByteSizeLong();
    char *data = (char*) malloc(sizeof(char) * msgLength);
    cont.SerializeToArray(data, msgLength);
    
    CPPUNIT_ASSERT(strncmp(data, c->message, msgLength) == 0);
    
    free(data);
    
    // Assert length is set
    CPPUNIT_ASSERT(c->size == msgLength);   
        
    freePendingCommand(c);
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

