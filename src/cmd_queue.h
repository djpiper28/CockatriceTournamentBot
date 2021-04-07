#ifndef CMD_QUEUE_H_
#define CMD_QUEUE_H_

#include "trice_structs.h"

//type defs in trice_structs.h due to circular references

//Init method for the command queue
void initPendingCommandQueue(struct pendingCommandQueue *q);

//Queue methods
void freePendingCommand(struct pendingCommand *cmd);

int hasNext(struct pendingCommandQueue *queue) ;

struct pendingCommand *deq(struct pendingCommandQueue *queue);

int isGameEq(const char *gameName, 
             struct pendingCommand *node);

struct pendingCommand *gameWithName(struct pendingCommandQueue *queue,
                                    const char *gameName);

struct pendingCommand *cmdForCMDId (int CMDId, 
                                    struct pendingCommandQueue *queue);
                                    
void enq(struct pendingCommand *cmd, 
         struct pendingCommandQueue *queue);
         
struct pendingCommand *peek(struct pendingCommandQueue *queue);

void freePendingCommandQueue(struct pendingCommandQueue *queue);

#endif
