#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "testCmdQueue.h"
#include "../src/cmd_queue.h"
#include "../src/bot.h"
#include "../src/game_struct.h"

CPPUNIT_TEST_SUITE_REGISTRATION(TestCmdQueue);

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

TestCmdQueue::TestCmdQueue () : CppUnit::TestCase("cm_queue.h tests") {
    
}

#define LEN 25
// Tests for seg faults and that the struct has the correct starting values
void TestCmdQueue::testInitAndFree() {
    // Test on empty queue
    struct pendingCommandQueue queue;
    initPendingCommandQueue(&queue);
    CPPUNIT_ASSERT(queue.head == NULL);
    CPPUNIT_ASSERT(queue.tail == NULL);
    
    freePendingCommandQueue(&queue);
    
    // Test on a queue with elements
    initPendingCommandQueue(&queue);
    CPPUNIT_ASSERT(queue.head == NULL);
    CPPUNIT_ASSERT(queue.tail == NULL);
    
    for (int i = 0; i < LEN; i++) {
        struct pendingCommand *node = (struct pendingCommand *)
                                      malloc(sizeof(struct pendingCommand));
        node->message = (char *) malloc(sizeof(char) * 69);
        node->isGame = 0;
        enq(node, &queue);
    }
    
    freePendingCommandQueue(&queue);
}

void TestCmdQueue::testQueueOps() {   
    // Test that all the nodes that enqueued are dequeued in the same order
    struct pendingCommandQueue queue; 
    initPendingCommandQueue(&queue);
    
    struct pendingCommand *nodes[LEN];
    for (int i = 0; i < LEN; i++) {
        nodes[i] = (struct pendingCommand *)
                   malloc(sizeof(struct pendingCommand));
        nodes[i]->message = (char *) malloc(sizeof(char) * 69);
        nodes[i]->isGame = 0;
        enq(nodes[i], &queue);
    }    
    
    for (int i = 0; i < LEN; i++) {
        CPPUNIT_ASSERT(nodes[i] == peek(&queue));
        CPPUNIT_ASSERT(nodes[i] == deq(&queue));
    }
    
    freePendingCommandQueue(&queue);
}

#define GAME_NAME_MACRO snprintf(buff, LEN, "game-%d", i);
void TestCmdQueue::testSearchOps() {
    struct pendingCommandQueue queue;
    initPendingCommandQueue(&queue);
    
    // Populate the queue
    struct pendingCommand *nodes[LEN];
    for (int i = 0; i < LEN; i++) {
        nodes[i] = (struct pendingCommand *)
        malloc(sizeof(struct pendingCommand));
        nodes[i]->message = (char *) malloc(sizeof(char) * 69);
        nodes[i]->cmdID = i;
        
        nodes[i]->isGame = 1;
        nodes[i]->param = malloc(sizeof(struct gameCreateCallbackWaitParam));
        struct gameCreateCallbackWaitParam *param = 
            (struct gameCreateCallbackWaitParam *) nodes[i]->param;
        
        char *buff = (char *) malloc(sizeof(char) * LEN);
        GAME_NAME_MACRO
        initGameCreateCallbackWaitParam(param, buff, LEN, NULL);
        
        CPPUNIT_ASSERT(isGameEq(buff, nodes[i]));        
        
        enq(nodes[i], &queue);
    }
    
    // Assert the queue search works for all items in the queue
    for (int i = 0; i < LEN; i++) {
        char buff[LEN];
        GAME_NAME_MACRO
        
        struct pendingCommand *node = cmdForCMDId(i, &queue);
        CPPUNIT_ASSERT(node == nodes[i]);
        enq(node, &queue);
        
        CPPUNIT_ASSERT(gameWithName(&queue, buff) == nodes[i]);
    }
    
    freePendingCommandQueue(&queue);
}

void TestCmdQueue::testPrepCMD() {
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

void TestCmdQueue::testPrepEmptyCMD() {
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
