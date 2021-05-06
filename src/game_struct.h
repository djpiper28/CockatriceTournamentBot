#ifndef GAMESTRUCT_H_
#define GAMESTRUCT_H_

#include <pthread.h>

/**
 * Used for game create callback
 * Only the gameID is mutable, use the mutex when accessing or modifying it
 */
struct gameCreateCallbackWaitParam {
    char *gameName;
    int   gameID,
          sendTime,
          gameNameLength;
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
    struct player *playerArr;
};

struct player {
    int playerID;
    char *playerName;
};

// Add to non-circular linked-list
struct game *createGame(int gameID, int playerCount);

/**
 * Adds a player to a game, playerName is copied
 * playerName input must NULL terminated
 * Returns 1 if addition was successful
 * Returns 0 if the array is full or NULL
 */
int addPlayer(struct gameList *gl, struct game *g, const char *playerName, int playerID);

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
                                     void (*callbackFn)(struct gameCreateCallbackWaitParam *));

void freeGameCreateCallbackWaitParam(struct gameCreateCallbackWaitParam *gp);

void freeGameListNode(struct gameList *g, struct gameListNode *gl);

void freeGameList(struct gameList *g);

// Returns -1 if the player is not found
int getPlayerIDForGameIDAndName(struct gameList *g, int gameID, char *playerName);

struct game *getGameWithID(struct gameList *g, int gameID);

void addGame(struct gameList *g, struct game *gamePointer);

void removeGame(struct gameList *g, struct game *gamePointer);

#endif
