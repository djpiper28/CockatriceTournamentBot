#ifndef QUEUE_
#define QUEUE_
#include "cmd_queue.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "commands.pb.h"
#include "gamestruct.h"
#include "botconf.h"
#include "trice_structs.h"

//Type defs moved to trice_structs.h due to circlular references

//Init method for the command queue
void initPendingCommandQueue(struct pendingCommandQueue *q) {
    q->mutex = PTHREAD_MUTEX_INITIALIZER;
    q->head = NULL;
    q->tail = NULL;
}

//Queue methods
void freePendingCommand(struct pendingCommand *cmd) {
    if (cmd != NULL) {
        if (cmd->message != NULL) {
            free(cmd->message);
        }
        
        if (cmd->isGame) {
            struct gameCreateCallbackWaitParam *p = (struct
                gameCreateCallbackWaitParam *) cmd->param;
            if (p != NULL) {
                if (fork() == 0) {
                    //Wait for timeout of the game callback in another thread
                    int waiting = 1;                    
                    while (waiting) {
                        usleep(25);
                        
                        pthread_mutex_lock(&p->mutex);
                        waiting = p->callbackFn == NULL;
                        pthread_mutex_unlock(&p->mutex);
                    }
                    
                    freeGameCreateCallbackWaitParam(p);                    
                    exit(0);
                }
            }
        }
        free(cmd);
    }
}

static int hasNextNTS(struct pendingCommandQueue *queue) {
    return queue->head != NULL;
}

int hasNext(struct pendingCommandQueue *queue) {
    pthread_mutex_lock(&queue->mutex);
    int res = hasNextNTS(queue);
    pthread_mutex_unlock(&queue->mutex);
    return res;
}

static struct pendingCommand *deqNTS(struct pendingCommandQueue *queue) {   
    if (!hasNextNTS(queue)) {
        return NULL;
    } else {
        struct pendingCommandQueueNode *last = queue->head;
        queue->head = last->next;    
        
        struct pendingCommand *returnValue = last->payload;
        free(last);
        
        if (queue->head == NULL) {
            queue->tail = NULL;
        }
        
        return returnValue;
    }    
}

struct pendingCommand *deq(struct pendingCommandQueue *queue) {  
    pthread_mutex_lock(&queue->mutex);    
    struct pendingCommand *returnValue = deqNTS(queue);
    pthread_mutex_unlock(&queue->mutex);
        
    return returnValue;
}

int isGameEq(const char *gameName, 
             struct pendingCommand *node) {
    if (node->isGame) {
        struct gameCreateCallbackWaitParam *data = (struct
                                    gameCreateCallbackWaitParam *) node->param;
        
        if (data != NULL)       
            return strncmp(data->gameName, gameName, BUFFER_LENGTH) == 0;
    } 
    
    return 0;
}

struct pendingCommand *gameWithName(struct pendingCommandQueue *queue,
                                    const char *gameName) {  
    pthread_mutex_lock(&queue->mutex);    
    if (isGameEq(gameName, queue->head->payload)) {
        pthread_mutex_unlock(&queue->mutex);    
        return deqNTS(queue);
    }
      
    struct pendingCommandQueueNode *current = queue->head, 
                               *last = NULL;
    struct pendingCommand *output = NULL;
    int cont = current != NULL;
    
    while (cont) {        
        if (current->payload != NULL) {
            if (isGameEq(gameName, current->payload)) {
                output = current->payload;
                last->next = current->next;
                
                if (last->next == NULL) {
                    queue->tail = last;
                }
                
                free(current);
                
                cont = 0;                
            }
        } 
        
        if (cont) {
            if (current->next == NULL) {
                cont = 0;
            } else {
                last = current;
                current = current->next;
            }
        }        
    } 
    
    pthread_mutex_unlock(&queue->mutex);
    
    return output;
}

struct pendingCommand *cmdForCMDId (int CMDId, 
                                    struct pendingCommandQueue *queue) {   
    pthread_mutex_lock(&queue->mutex);  
    if (queue->head->payload->cmdID == CMDId) {
        return deqNTS(queue);
    }
    
    struct pendingCommandQueueNode *current = queue->head, 
                                   *last = NULL;
    struct pendingCommand *output = NULL;
    int cont = 1;
        
    while (cont && current != NULL) {        
        if (current->payload != NULL) {
            if (current->payload->cmdID == CMDId) {
                output = current->payload;
                last->next = current->next;
                    
                if (last->next == NULL) {
                    queue->tail = last;
                }
                    
                free(current);
                  
                cont = 0;                
            }
        }
        
        if (cont) {
            if (current->next != NULL) {
                last = current;
                current = current->next;
            } else {
                cont = 0;
            }
        }        
    }
    
    
    pthread_mutex_unlock(&queue->mutex);
    
    return output;
}

void enq(struct pendingCommand *cmd, 
         struct pendingCommandQueue *queue) {        
    pthread_mutex_lock(&queue->mutex);
    
    struct pendingCommandQueueNode *node = (struct pendingCommandQueueNode *)
                                malloc(sizeof(struct pendingCommandQueueNode));
    node->payload = cmd;
    node->next    = NULL;    
    
    if (!hasNextNTS(queue)) {
        queue->head = node;
        queue->tail = node;
    } else {        
        queue->tail->next = node;        
        queue->tail = node;
    }    
    
    pthread_mutex_unlock(&queue->mutex);
}

struct pendingCommand *peek(struct pendingCommandQueue *queue) {
    pthread_mutex_lock(&queue->mutex);
    
    struct pendingCommand *out = hasNextNTS(queue) ? queue->head->payload : NULL;
    
    pthread_mutex_unlock(&queue->mutex);    
    return out;
}

void freePendingCommandQueue(struct pendingCommandQueue *queue) {
    pthread_mutex_lock(&queue->mutex);
    
    while (hasNextNTS(queue)) {
        freePendingCommand(deqNTS(queue));
    }
    
    pthread_mutex_unlock(&queue->mutex);   
    pthread_mutex_destroy(&queue->mutex);   
}

#endif
