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

/* Not thread safe, to ensure thread safety run this in the same mutex as the
 * queue being changed. Call deq for thread safe version
 */
struct pendingCommand *deq_nts(struct pendingCommandQueue **head, 
                               struct pendingCommandQueue **tail) {   
    if (!hasNext(*head)) {
        return NULL;
    } else {
        struct pendingCommandQueue *last = *head;
        *head = last->next;    
        
        struct pendingCommand *returnValue = last->payload;
        free(last);
        
        if (*head == NULL) {
            *tail = NULL;
        }
        
        return returnValue;
    }    
}

struct pendingCommand *deq(struct pendingCommandQueue **head,
                           struct pendingCommandQueue **tail,
                           pthread_mutex_t mutex_queue) {  
    pthread_mutex_lock(&mutex_queue);    
    struct pendingCommand *returnValue = deq_nts(head, tail);
    pthread_mutex_unlock(&mutex_queue);
        
    return returnValue;
}

int isGameEq(const char *gameName, struct pendingCommand *node) {
    if (node->isGame) {
        struct gameCreateCallbackWaitParam *data = (struct
                                    gameCreateCallbackWaitParam *) node->param;
        
        if (data != NULL)       
            return strncmp(data->gameName, gameName, BUFFER_LENGTH) == 0;
    } 
    
    return 0;
}

struct pendingCommand *gameWithName(struct pendingCommandQueue **head,
                                    struct pendingCommandQueue **tail,
                                    const char *gameName, 
                                    pthread_mutex_t mutex_queue) {  
    pthread_mutex_lock(&mutex_queue);    
    if (isGameEq(gameName, (*head)->payload)) {
        return deq_nts(head, tail);
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
    
    pthread_mutex_unlock(&mutex_queue);
    
    return output;
}

struct pendingCommand *cmdForCMDId(struct pendingCommandQueue **head,
                                   struct pendingCommandQueue **tail, int CMDId, 
                                   pthread_mutex_t mutex_queue) {   
    pthread_mutex_lock(&mutex_queue);  
    if ((*head)->payload->cmdID == CMDId) {
        return deq_nts(head, tail);
    }
    
    struct pendingCommandQueue *current = *head, *last;
    struct pendingCommand *output = NULL;
    int cont = 1;
        
    while (cont && current != NULL) {        
        if (current->payload != NULL) {
            if (current->payload->cmdID == CMDId) {
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
            if (current->next != NULL) {
                last = current;
                current = current->next;
            } else {
                cont = 0;
            }
        }        
    }
    
    
    pthread_mutex_unlock(&mutex_queue);
    
    return output;
}

void enq(struct pendingCommand *cmd, struct pendingCommandQueue **head, 
         struct pendingCommandQueue **tail, 
         pthread_mutex_t mutex_queue) {        
    pthread_mutex_lock(&mutex_queue);
    
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
    
    pthread_mutex_unlock(&mutex_queue);
}

struct pendingCommand *peek(struct pendingCommandQueue *head) {
    return hasNext(head) ? head->payload : NULL;
}

#endif
