#ifndef QUEUE_
#define QUEUE_
#include "cmd_queue.h"
#include "logger.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "commands.pb.h"
#include "game_struct.h"
#include "bot_conf.h"
#include "trice_structs.h"

//Type defs moved to trice_structs.h due to circlular references

//Init method for the command queue
void initPendingCommandQueue(struct pendingCommandQueue *q)
{
    q->mutex = PTHREAD_MUTEX_INITIALIZER;
    q->head = NULL;
    q->tail = NULL;
}

static void *freeCmdForGameCreate(void *param)
{
    if (param != NULL) {
        struct gameCreateCallbackWaitParam *p = (struct gameCreateCallbackWaitParam *)
                                                param;

        //Wait for timeout of the game callback in another thread
        int waiting = 1;
        while (waiting) {
            pthread_mutex_lock(&p->mutex);
            waiting = p->callbackFn == NULL;
            pthread_mutex_unlock(&p->mutex);

            if (waiting) {
                usleep(25);
            }
        }

        freeGameCreateCallbackWaitParam(p);
    }

    pthread_exit(NULL);
}

//Queue methods
void freePendingCommand(struct pendingCommand *cmd)
{
    if (cmd != NULL) {
        if (cmd->message != NULL) {
            free(cmd->message);
        }

        if (cmd->isGame) {
            struct gameCreateCallbackWaitParam *p = (struct gameCreateCallbackWaitParam *)
                                                    cmd->param;

            if (p != NULL) {
                pthread_t t;
                if (pthread_create(&t, NULL, freeCmdForGameCreate, (void *) p) != 0) {
                    lprintf(LOG_INFO, "Error creating poll thread for free cmd game "
                           "create, running polling thread on current thread.\n");

                    //Wait for timeout of the game callback in another thread
                    int waiting = 1;
                    while (waiting) {
                        pthread_mutex_lock(&p->mutex);
                        waiting = p->callbackFn == NULL;
                        pthread_mutex_unlock(&p->mutex);

                        if (waiting) {
                            usleep(25);
                        }
                    }

                    freeGameCreateCallbackWaitParam(p);
                }
            }
        }

        free(cmd);
    }
}

static int hasNextNTS(struct pendingCommandQueue *queue)
{
    return queue->head != NULL;
}

int hasNext(struct pendingCommandQueue *queue)
{
    pthread_mutex_lock(&queue->mutex);
    int res = hasNextNTS(queue);
    pthread_mutex_unlock(&queue->mutex);
    return res;
}

static struct pendingCommand *deqNTS(struct pendingCommandQueue *queue)
{
    if (queue->head != NULL) {
        struct pendingCommandQueueNode *last = queue->head;
        queue->head = last->next;

        struct pendingCommand *returnValue = last->payload;
        free(last);

        if (queue->head == NULL) {
            queue->tail = NULL;
        }

        return returnValue;
    } else {
        return NULL;
    }
}

struct pendingCommand *deq(struct pendingCommandQueue *queue)
{
    pthread_mutex_lock(&queue->mutex);
    struct pendingCommand *returnValue = deqNTS(queue);
    pthread_mutex_unlock(&queue->mutex);

    return returnValue;
}

int isGameEq(const char *gameName,
             struct pendingCommand *node)
{
    if (node->isGame) {
        struct gameCreateCallbackWaitParam *data = (struct
                gameCreateCallbackWaitParam *) node->param;

        if (data != NULL) {
            return strncmp(data->gameName, gameName, BUFFER_LENGTH) == 0;
        }
    }

    return 0;
}

struct pendingCommand *gameWithName(struct pendingCommandQueue *queue,
                                    const char *gameName)
{
    pthread_mutex_lock(&queue->mutex);

    struct pendingCommand *output = NULL;
    if (queue->head != NULL) {
        if (isGameEq(gameName, queue->head->payload)) {
            pthread_mutex_unlock(&queue->mutex);
            return deqNTS(queue);
        }

        struct pendingCommandQueueNode *current = queue->head,
                                            *last = NULL;


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
    }

    pthread_mutex_unlock(&queue->mutex);

    return output;
}

struct pendingCommand *cmdForCMDId(int CMDId,
                                   struct pendingCommandQueue *queue)
{
    pthread_mutex_lock(&queue->mutex);

    if (queue->head->payload->cmdID == CMDId) {
        struct pendingCommand *out = deqNTS(queue);
        pthread_mutex_unlock(&queue->mutex);
        return out;
    }

    struct pendingCommandQueueNode *current = queue->head,
                                        *last = NULL;

    struct pendingCommand *output = NULL;

    int cont = 1;

    while (cont && current != NULL) {
        if (current->payload != NULL) {
            if (current->payload->cmdID == CMDId) {
                output     = current->payload;
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

struct pendingCommand *prepCmdNTS(struct triceBot *b,
                                  CommandContainer cont,
                                  int gameID,
                                  int roomID)
{
    struct pendingCommand *pending = (struct pendingCommand *)
                                     malloc(sizeof(struct pendingCommand));
    pending->cmdID = b->cmdID;
    cont.set_cmd_id(pending->cmdID);

    if (gameID != -1) {
        cont.set_game_id(gameID);
    }

    if (roomID != -1) {
        cont.set_room_id(roomID);
    }

    long msgLength = cont.ByteSizeLong();

    char *data = (char*) malloc(sizeof(char) * msgLength);
    cont.SerializeToArray(data, msgLength);


    //init pending command
    //returned command is edited by the user before queue addition
    pending->message  = data;
    pending->size     = msgLength;
    pending->timeSent = time(NULL);
    pending->isGame   = 0;
    pending->callbackFunction = NULL;

    b->cmdID ++;
    //id is in range 0 to 01111...111
    b->cmdID %= 0x7FFFFFFF;

    return pending;
}

/**
 * prepCmd takes the following arguments:
 * CommandContainer cont -> the command container for the generated command
 * int gameID -> the ID of the game if the command container should refer to a
 * game, set to -1 leave the value already in cont the same
 * int roomID -> the ID of the room if the command container should refer to a
 * game, set to -1 to leave the value already in cont the same
 */
struct pendingCommand *prepCmd(struct triceBot *b,
                               CommandContainer cont,
                               int gameID,
                               int roomID)
{
    pthread_mutex_lock(&b->mutex);
    struct pendingCommand *a = prepCmdNTS(b, cont, gameID, roomID);
    pthread_mutex_unlock(&b->mutex);

    return a;
}

/**
* Prepares and empty command container
* is not used in this code but added for backwards compatibility with old
* servatrice server see remotclient.cpp in the cockatrice for the reason
*/
struct pendingCommand *prepEmptyCmd(struct triceBot *b)
{
    CommandContainer cont; // Ignore the cppcheck warning
    return prepCmd(b, cont, -1, -1);
}

void enq(struct pendingCommand *cmd,
         struct pendingCommandQueue *queue)
{
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

struct pendingCommand *peek(struct pendingCommandQueue *queue)
{
    pthread_mutex_lock(&queue->mutex);

    struct pendingCommand *out = hasNextNTS(queue) ? queue->head->payload : NULL;

    pthread_mutex_unlock(&queue->mutex);
    return out;
}

void freePendingCommandQueue(struct pendingCommandQueue *queue)
{
    pthread_mutex_lock(&queue->mutex);

    while (hasNextNTS(queue)) {
        freePendingCommand(deqNTS(queue));
    }

    queue->head = NULL;
    queue->tail = NULL;

    pthread_mutex_unlock(&queue->mutex);
    pthread_mutex_destroy(&queue->mutex);
}

#endif
