#ifndef QUEUE_
#define QUEUE_

#include <stdlib.h>
#include <stdio.h>

#include "server_message.pb.h"
#include "commands.pb.h"
#include "gamestruct.h"
#include "botconf.h"

//Type definitions
struct pendingCommand {
    char *message;
    void *param;
    int size = 0, cmdID = -1, timeSent = -1, isGame = 0;
    void (*callbackFunction)(const Response *, void *param);
};

struct pendingCommandQueue {
    struct pendingCommand      *payload;
    struct pendingCommandQueue *next;
};

//Queue methods
void freePendingCommand(struct pendingCommand *cmd) {
    free(cmd->message);
    free(cmd);
}

int hasNext(struct pendingCommandQueue *head) {
    return head != NULL;
}

struct pendingCommand *deq(struct pendingCommandQueue **head) {
    static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;  
    
    pthread_mutex_lock(&mutex);
    
    if (!hasNext(*head)) {
        pthread_mutex_unlock(&mutex);
        return NULL;
    } else {
        struct pendingCommandQueue *last = *head;
        *head = last->next;    
        
        struct pendingCommand *returnValue = last->payload;
        free(last);
        
        pthread_mutex_unlock(&mutex);
        return returnValue;
    }    
}

int isGameEq(const char *gameName, struct pendingCommand *node) {
    if (node->isGame) {
        struct gameCreateCallbackWaitParam *data = (struct gameCreateCallbackWaitParam *) node->param;
        
        if (data != NULL)       
            if (strncmp(data->gameName, gameName, BUFFER_LENGTH) == 0) 
                return 1;
    } 
    
    return 0;
}

struct pendingCommand *gameWithName(struct pendingCommandQueue **head,
                                    struct pendingCommandQueue **tail,
                                    const char *gameName) {    
    static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;  
    
    pthread_mutex_lock(&mutex);
    
    if (isGameEq(gameName, (*head)->payload)) {
        return deq(head);
    }
    
    struct pendingCommandQueue *current = *head, 
                               *last = NULL;
    struct pendingCommand *output = NULL;
    int cont = current != NULL;
    
    while (cont) {        
        if (current->payload != NULL) {
            if (isGameEq(gameName, current->payload)) {
                output = current->payload;
                last->next = current->next;
                
                if (last->next == NULL) {
                    *tail = last;
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
    
    pthread_mutex_unlock(&mutex);
    return output;
}

struct pendingCommand *cmdForCMDId(struct pendingCommandQueue **head,
                                   struct pendingCommandQueue **tail, int CMDId) {
    if ((*head)->payload->cmdID == CMDId) {
        return deq(head);
    }
    
    struct pendingCommandQueue *current = *head, *last;
    struct pendingCommand *output = NULL;
    int cont = 1;
        
    while (cont && current != NULL) {
        int next = 0;
        
        if (current->payload != NULL) {
            if (current->payload->cmdID == CMDId) {
                output = current->payload;
                last->next = current->next;
                    
                if (last->next == NULL) {
                    *tail = last;
                }
                    
                free(current);
                  
                cont = 0;                
            } else {
                next = 1;
            }
        } else {
            next = 1;
        }
        
        if (next) {
            if (current->next != NULL) {
                last = current;
                current = current->next;
            } else {
                cont = 0;
            }
        }        
    }
    
    return output;
}

void enq(struct pendingCommand *cmd, struct pendingCommandQueue **head, 
        struct pendingCommandQueue **tail) {    
    static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;  
    
    pthread_mutex_lock(&mutex);
    
    struct pendingCommandQueue *node = (struct pendingCommandQueue *)
                        malloc(sizeof(struct pendingCommandQueue));
    node->payload = cmd;
    node->next    = NULL;    
    
    if (!hasNext(*head)) {
        *head = node;
        *tail = node;
    } else {        
        (*tail)->next = node;        
        *tail = node;
    }    
    
    pthread_mutex_unlock(&mutex);
}

struct pendingCommand *peek(struct pendingCommandQueue *head) {
    return hasNext(head) ? head->payload : NULL;
}

#endif
