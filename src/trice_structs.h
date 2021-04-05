#ifndef TRICE_STRUCTS_
#define TRICE_STRUCTS_

#include <pthread.h>
#include "botconf.h"
#include "gamestruct.h"
#include "response.pb.h"

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
};

#endif
