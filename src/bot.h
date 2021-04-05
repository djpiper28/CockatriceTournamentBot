#ifndef BOT_H_
#define BOT_H_

#include "bot.cpp"

//Type defs moved to trice_structs.h due to circlular references

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
 */ 
void replayResponseDownload(struct triceBot *b,
                            const Response *response, 
                            void *param);

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
