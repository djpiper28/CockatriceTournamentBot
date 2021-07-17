#ifndef CMD_QUEUE_H_
#define CMD_QUEUE_H_

#include "trice_structs.h"
#include "commands.pb.h"

//type defs in trice_structs.h due to circular references

//Init method for the command queue
void initPendingCommandQueue(struct pendingCommandQueue *q);

//Queue methods
void freePendingCommand(struct pendingCommand *cmd);

int hasNext(struct pendingCommandQueue *queue);

struct pendingCommand *deq(struct pendingCommandQueue *queue);

int isGameEq(const char *gameName,
             struct pendingCommand *node);

struct pendingCommand *gameWithName(struct pendingCommandQueue *queue,
                                    const char *gameName);

struct pendingCommand *cmdForCMDId(int CMDId,
                                   struct pendingCommandQueue *queue);

/**
 * Same as prepCMD but not thread safe, only use when the mutex for the queue
 * is locked.
 * WARNING: Not thread safe!
 */
struct pendingCommand *prepCmdNTS(struct triceBot *b,
                                  CommandContainer cont,
                                  int gameID,
                                  int roomID);

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
                               int roomID);

/**
 * Prepares and empty command container
 * is not used in this code but added for backwards compatibility with old
 * servatrice server see remotclient.cpp in the cockatrice for the reason
 */
struct pendingCommand *prepEmptyCmd(struct triceBot *b);

void enq(struct pendingCommand *cmd,
         struct pendingCommandQueue *queue);

struct pendingCommand *peek(struct pendingCommandQueue *queue);

void freePendingCommandQueue(struct pendingCommandQueue *queue);

#endif