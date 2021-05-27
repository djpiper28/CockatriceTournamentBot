#ifndef GAMESTRUCT_H_
#define GAMESTRUCT_H_

#include <pthread.h>

struct gameData {
    void *gameDataPtr;
    void (*freeGameData) (void *);
};

/**
 * Used for game create callback
 * Only the gameID is mutable, use the mutex when accessing or modifying it
 */
struct gameCreateCallbackWaitParam {
    char *gameName;
    int   gameID,
          sendTime,
          gameNameLength;
    struct gameData gameData;
    pthread_mutex_t mutex;
    void (*callbackFn)(struct gameCreateCallbackWaitParam *);
};

//Game list
struct gameList {
    struct gameListNode *gamesHead;
    pthread_mutex_t mutex;
};

struct gameListNode {
    struct game *currentGame;
    struct gameListNode *nextGame;
};

//playerArr has length playerCount. NULL players in playerArr haven't joined yet
struct game {
    int gameID,
        started,
        playerCount;
    long startTime,
         creationTime;
    struct gameData gameData;
    struct player *playerArr;
};

struct player {
    int playerID, ping;
    char *playerName;
};

// Add to non-circular linked-list, game data can be NULL
struct game *createGame(int gameID,
                        int playerCount,
                        struct gameData gameData);

/**
 * Adds a player to a game, playerName is copied
 * playerName input must NULL terminated
 * Returns 1 if addition was successful
 * Returns 0 if the array is full or NULL
 */
int addPlayer(struct gameList *gl, 
              struct game *g, 
              const char *playerName, 
              int playerID, 
              int ping);

/**
 * Removes a player from a game
 * Returns 1 if addition was successful
 * Returns 0 if the array has no player with that ID or is NULL
 */
int removePlayer(struct gameList *gl, struct game *g, int playerID);

// Init the game list structure
void initGameList(struct gameList *gl);

// Inits the param, you probably want to strcpy gameName first.
void initGameCreateCallbackWaitParam(struct gameCreateCallbackWaitParam *param,
                                     char *gameName,
                                     int gameNameLength,
                                     struct gameData gameData,
                                     void (*callbackFn)(struct gameCreateCallbackWaitParam *));

void freeGameCreateCallbackWaitParam(struct gameCreateCallbackWaitParam *gp);

void freeGameListNode(struct gameList *g, struct gameListNode *gl);

void freeGameList(struct gameList *g);

// Returns -1 if the player is not found
int getPlayerIDForGameIDAndName(struct gameList *g, int gameID, char *playerName);

// Returns NULL if not found
struct game *getGameWithIDNTS(struct gameList *g, int gameID);
struct game *getGameWithID(struct gameList *g, int gameID);

// gameID is -1 i the game is not found, returns a copy
struct game getGameWithIDNotRefNTS(struct gameList *g,
                                   int gameID);
struct game getGameWithIDNotRef(struct gameList *g,
                                int gameID);

// Frees a game copy from getGameWithIDNotRef
void freeGameCopy(struct game g);

void addGame(struct gameList *g, struct game *gamePointer);

void removeGame(struct gameList *g, struct game *gamePointer);

#endif
