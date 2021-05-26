#ifndef BOT_H_
#define BOT_H_

#include "game_replay.pb.h"
#include "trice_structs.h"
#include "commands.pb.h"
#include "response.pb.h"
#include "server_message.pb.h"
#include "response_replay_download.pb.h"

//Type defs moved to trice_structs.h due to circlular references
#define PING_FREQUENCY 4
#define TIMEOUT 10

/**
 * All bot operations are thread safe allowing for it to be controlled by
 * another thread. All callbacks and event functions are ran on the bot thread.
 * you may wish to consider starting a new thread for long operations.
 */


/**
 * Called when a game is created
 */
void handleGameCreate(struct triceBot *b,
                      const Event_GameJoined gameCreate);

/**
 * Free the char* manually.
 * Names with slashes i.e: Test Tournament/Finals/Match 6 will be saved as as
 * REPLAY_FOLDER/Test Tournament/Finals/replay-Match 6-GAME_ID.cor
 * Names without slashes will be saved as replay-NAME-GAMEID.cor
 *
 * Set baseDIR to NULL if you do not want to make the folders yet
 */
char *getReplayFileName(int gameID,
                        const char *gameNameUnfiltered,
                        int length,
                        char *baseDIR);

/**
 * Downloads a replay to ./replays/ use getReplayFileName to get the URI
 * -> Fails silently
 * Is subject to cockaspagheti
 */
void replayResponseDownload(struct triceBot *b,
                            const Response_ReplayDownload replay);

/**
 * Downloads a replay to ./replays/ use getReplayFileName to get the URI
 * -> Fails silently
 * Is subject to cockaspagheti
 */
void saveReplay(struct triceBot *b,
                const GameReplay gameReplay);

/**
 * struct triceBot *b -> pointer to the bot to init
 * struct Config config -> bot configuration
 */
void initBot(struct triceBot *b,
             struct Config config);

/**
 * Check to see if the server should be pinged
 */
int needsPing(long lastPingTime);

/**
 * struct triceBot *b -> pointer to the bot to free
 */
void freeBot(struct triceBot *b);

/**
 * Queues a ping command
 */
void sendPing(struct triceBot *b);

/**
 * This part of the library will take a login response and it will check if it
 * is logged in. If it is not logged in it will call the onBotLogin state change
 * event in the same thread after requesting the room list
 */
void loginResponse(struct triceBot *b,
                   const Response *response,
                   void *param);

//Login, login details are stored in b->config
void sendLogin(struct triceBot *b);

/**
 * Executes the callback of the command
 */
void executeCallback(struct triceBot *b,
                     struct pendingCommand *cmd,
                     const Response *response);

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
                      struct gameData gameData,
                      void (*callbackFn)(struct gameCreateCallbackWaitParam *));


/**
 * Calls the handler for each session event
 * CFLAGS for this method
 * Set DOWNLOAD_REPLAYS to 0 if you do not want replays to be automatically downloaded
 * Set LOGIN_AUTOMATICALLY to 0 if you do not  want to automatically login
 * Set JOIN_ROOM_AUTOMATICALLY to 0 if you do not want to automatically join a room
 * All of these CFLAGS default to 1
 * The macros that are specific to this function are above ^^
 */
void handleSessionEvent(struct triceBot *b,
                        ServerMessage *newServerMessage);

/**
 * Handles a server message
 * make it call a user defined function of OnServerMSG
 */
void handleResponse(struct triceBot *b,
                    ServerMessage *newServerMessage);

//Join the room in config
void handleRoomEvent(struct triceBot *b,
                     ServerMessage *newServerMessage);

/**
 * Called when a game event occurs
 */
void handleGameEvent(struct triceBot *b,
                     ServerMessage *newServerMessage);

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
