#ifndef BOT_H_
#define BOT_H_

#include "trice_structs.h"
#include "commands.pb.h"
#include "response.pb.h"

//Type defs moved to trice_structs.h due to circlular references
#define PING_FREQUENCY 5
#define TIMEOUT PING_FREQUENCY

/**
 * All bot operations are thread safe allowing for it to be controlled by
 * another thread. All callbacks and event functions are ran on the bot thread. 
 * you may wish to consider starting a new thread for long operations.
 */ 

/**
 * struct triceBot *b -> pointer to the bot to init
 * struct Config config -> bot configuration
 */ 
void initBot(struct triceBot *b,
             struct Config config);
/**
 * struct triceBot *b -> pointer to the bot to free
 */ 
void freeBot(struct triceBot *b);

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

/**
 * Queues a ping command
 */ 
void sendPing(struct triceBot *b);

/**
 * Downloads a replay to ./replays/
 * -> fails silently
 */ 
void replayResponseDownload(struct triceBot *b,
                            const Response *response, 
                            void *param);

/**
 * This method sends a create game command
 * WARNING If the callback is NULL then the param struct is not freed as it 
 * assumes that a thread is polling it see apiServer for example.
 * Otherwise the callback is called and then the data is freed.
 * 
 * TL;DR.
 * Callback should not be NULL.
 * If callback is NULL then the param is leaked.
 */ 
//These are long type names
struct gameCreateCallbackWaitParam * 
sendCreateGameCommand(struct triceBot *b, 
                      char *gameName,
                      char *password,
                      int playerCount,    
                      int joinAsSpectator, 
                      int spectatorsAllowed,                                                         
                      int spectatorsCanChat,
                      int spectatorsNeedPassword,
                      int spectatorsCanSeeHands,
                      int onlyRegistered,
                      int onlyBuddies,                                                          
                      void (*callbackFn) (struct gameCreateCallbackWaitParam *));

/**
 * Stops the bot
 */
void stopBot(struct triceBot *b);

/**
 * Starts the bot in a pthread
 * returns 1 if it succeeded
 * returns 0 if it had an error
 */ 
int startBot(struct triceBot *b);

/**
 * should be called by the library host to shutdown the library
 */ 
void killProtoufLib();

#endif
