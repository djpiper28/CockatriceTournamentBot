#ifndef GAMESTRUCT_
#define GAMESTRUCT_
#include "game_struct.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

// Init the game list structure
void initGameList(struct gameList *gl) {
    gl->mutex = PTHREAD_MUTEX_INITIALIZER;
    gl->gamesHead = NULL;
}

#define MAX_LENGTH 255

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
              int ping) {
    pthread_mutex_lock(&gl->mutex);
    int done = 0;
    
    for (int i = 0; !done && i < g->playerCount; i++) {
        if (g->playerArr[i].playerName == NULL) {
            size_t nameLength = strlen(playerName) + 1; //NULL terminator
            char *playerNameCP = (char *) malloc(sizeof(char) * nameLength);
            strncpy(playerNameCP, playerName, nameLength);
            
            struct player p = {playerID, ping, playerNameCP};
            g->playerArr[i] = p;
            done = 1;
            
            printf("[INFO]: Player %s joined game %d.\n",
                   playerNameCP,
                   g->gameID);
        }
    }
    
    pthread_mutex_unlock(&gl->mutex);
    
    return done;
}

/**
 * Removes a player from a game
 * Returns 1 if addition was successful
 * Returns 0 if the array has no player with that ID or is NULL
 */
int removePlayer(struct gameList *gl,
                 struct game *g,
                 int playerID) {
    pthread_mutex_lock(&gl->mutex);
    int done = 0;
    
    for (int i = 0; i < g->playerCount && !done; i++) {
        struct player *p = &g->playerArr[i];
        
        if (p->playerName != NULL && p->playerID == playerID) {
            printf("[INFO]: Player %s left game %d.\n",
                   p->playerName,
                   g->gameID);
                   
            free(p->playerName);
            p->playerName = NULL;
        }
    }
    
    pthread_mutex_unlock(&gl->mutex);
    
    return done;
}

void initGameCreateCallbackWaitParam(struct gameCreateCallbackWaitParam *param,
                                     char *gameName,
                                     int gameNameLength,
                                     struct gameData gameData,
                                     void (*callbackFn)(struct gameCreateCallbackWaitParam *)) {
    param->gameName = gameName;
    param->gameNameLength = gameNameLength;
    param->gameID = -1;
    param->sendTime = time(NULL);
    param->callbackFn = callbackFn;
    param->gameData = gameData;
    param->mutex = PTHREAD_MUTEX_INITIALIZER;
}

// Resource free stuff
void freeGameCreateCallbackWaitParam(struct gameCreateCallbackWaitParam *gp) {
    if (gp != NULL) {
        if (gp->gameName != NULL) {
            free(gp->gameName);
        }
        
        if (gp->gameData.gameDataPtr != NULL
            && gp->gameData.freeGameData != NULL) {
            gp->gameData.freeGameData(gp->gameData.gameDataPtr);
        }
        
        pthread_mutex_destroy(&gp->mutex);
        free(gp);
    }
}

//Not thread safe version
static void freeGameListNodeNTS(struct gameListNode *gl) {
    if (gl != NULL) {
        if (gl->currentGame != NULL) {
            if (gl->currentGame->playerArr != NULL && gl->currentGame->playerCount > 0) {
                for (int i = 0; i < gl->currentGame->playerCount; i++) {
                    if (gl->currentGame->playerArr[i].playerName != NULL) {
                        free(gl->currentGame->playerArr[i].playerName);
                    }
                }
                
                free(gl->currentGame->playerArr);
            }
            
            if (gl->currentGame->gameData.gameDataPtr != NULL
                && gl->currentGame->gameData.freeGameData != NULL) {
                gl->currentGame->gameData.freeGameData(gl->currentGame->gameData.gameDataPtr);
            }
            
            free(gl->currentGame);
        }
        free(gl);
    }
}

void freeGameListNode(struct gameList *g, struct gameListNode *gl) {
    pthread_mutex_lock(&g->mutex);
    freeGameListNodeNTS(gl);
    pthread_mutex_unlock(&g->mutex);
}

void freeGameList(struct gameList *g) {
    pthread_mutex_lock(&g->mutex);
    
    struct gameListNode *current = g->gamesHead;
    
    while (current != NULL) {
        struct gameListNode *tmp = current;            
        current = current->nextGame;
            
        freeGameListNodeNTS(tmp);
    }
        
    pthread_mutex_unlock(&g->mutex);
    pthread_mutex_destroy(&g->mutex);
}

struct game *getGameWithIDNTS(struct gameList *g, int gameID) {
    struct gameListNode *current = g->gamesHead;
    struct game *out = NULL;
    
    if (current != NULL) {        
        while (current->currentGame->gameID != gameID) {
            current = current->nextGame;
            
            if (current == NULL) {
                break;
            }
        }
        
        if (current != NULL) { 
            out = current->currentGame;
        }
    }
    
    return out;
}

struct game *getGameWithID(struct gameList *g, int gameID) {
    pthread_mutex_lock(&g->mutex);    
    struct game *out = getGameWithIDNTS(g, gameID);
    pthread_mutex_unlock(&g->mutex);
    return out;
}

void freeGameCopy(struct game g) {    
    if (g.playerArr == NULL) {
        return;
    }
    
    for (int i = 0; i < g.playerCount; i++) {
        if (g.playerArr[i].playerName != NULL) {
            free(g.playerArr[i].playerName);
        }
    }
        
    free(g.playerArr);
}

struct game getGameWithIDNotRefNTS(struct gameList *g, int gameID) {
    struct game *ga = getGameWithIDNTS(g, gameID);
    struct game out;
    if (ga == NULL) {
        out.gameID = -1;
        out.playerArr = NULL;
    } else if (ga->gameID != -1) {
        out = *ga;
        
        // Copy player array
        struct player *playerArr = (struct player *) 
            malloc(sizeof(struct player) * out.playerCount);
        
        for (int i = 0; i < out.playerCount; i++) {
            playerArr[i].playerID = out.playerArr[i].playerID;
            if (out.playerArr[i].playerName != NULL) {
                int len = strnlen(out.playerArr[i].playerName, 256);
                playerArr[i].playerName = (char *) malloc(sizeof(char) * (len + 1));
                strncpy(playerArr[i].playerName, out.playerArr[i].playerName, len + 1);
            } else {
                playerArr[i].playerName = NULL;
            }
        }
        
        out.playerArr = playerArr;
    } else {
        out.gameID = -1; 
        out.playerArr = NULL;
    }
    return out;
}

struct game getGameWithIDNotRef(struct gameList *g, int gameID) {
    pthread_mutex_lock(&g->mutex);
    struct game ga = getGameWithIDNotRefNTS(g, gameID);   
    pthread_mutex_unlock(&g->mutex);
    return ga;
}

// Returns -1 if the player is not found
int getPlayerIDForGameIDAndName(struct gameList *gl, int gameID, char *playerName) {
    pthread_mutex_lock(&gl->mutex);
    int playerID = -1;
    
    // Linear search over games list
    struct game *g = getGameWithIDNTS(gl, gameID);
    
    if (g != NULL) {        
        /**
         * Iterate until a player with matching name is found or there are no
         * players left to check.
         */
        for (int i = 0; i < g->playerCount && playerID == -1; i++) {
            //Check if it a player
            if (g->playerArr[i].playerName != NULL) {
                if (strncmp(playerName, g->playerArr[i].playerName, MAX_LENGTH) == 0) {
                    playerID = g->playerArr[i].playerID;
                }
            }
        }
    }
    
    pthread_mutex_unlock(&gl->mutex);
    
    return playerID;
}

struct game *createGame(int gameID, int playerCount, struct gameData gameData) {
    struct game *g = (struct game *) malloc(sizeof(struct game));
    g->gameID = gameID;
    g->started = 0;
    g->startTime = -1;
    g->creationTime = time(NULL);
    g->gameData = gameData;
    g->playerCount = playerCount;
    g->playerArr = (struct player *) malloc(sizeof(struct player) * playerCount);
    
    for (int i = 0; i < playerCount; i++) {
        g->playerArr[i].playerName = NULL;
    }
    
    return g;
}
    
void addGame(struct gameList *g, struct game *gamePointer) {    
    if (g == NULL || gamePointer == NULL) {
        return;
    }
    
    pthread_mutex_lock(&g->mutex);
    if (getGameWithIDNTS(g, gamePointer->gameID) != NULL) {
        pthread_mutex_unlock(&g->mutex);
        return;
    }
    
    if (g->gamesHead == NULL) {
        struct gameListNode *next = (struct gameListNode *) malloc(sizeof(struct gameListNode));
        next->currentGame = gamePointer;
        next->nextGame = NULL;
        
        g->gamesHead = next;
    } else {
        struct gameListNode *current = g->gamesHead;
        
        while (current->nextGame != NULL) {
            current = current->nextGame;
        }
        
        struct gameListNode *next = (struct gameListNode *) malloc(sizeof(struct gameListNode));        
        
        next->currentGame = gamePointer;        
        next->nextGame = NULL;        
        current->nextGame = next;
        
        printf("[INFO]: Joined game %d.\n", gamePointer->gameID);
    }
    
    pthread_mutex_unlock(&g->mutex);
}

void removeGame(struct gameList *g, struct game *gamePointer) {
    if (g == NULL) {
        return;
    }
    
    pthread_mutex_lock(&g->mutex);    
    struct gameListNode *current = g->gamesHead;
    if (current != NULL) {
        // Edge case - game is the head of the list
        if (current->currentGame == gamePointer) {
            struct gameListNode *next = current->nextGame;
            freeGameListNodeNTS(current);
            
            g->gamesHead = next;
        } else {
            int found = 0;
            
            while (current->nextGame != NULL && !found) {
                if (current->nextGame->currentGame == gamePointer && gamePointer != NULL) {
                    // Remove the target. Set the current pointer to next's next.
                    struct gameListNode *target = current->nextGame;
                    
                    current->nextGame = current->nextGame->nextGame;
                    freeGameListNodeNTS(target);
                    found = 1;
                } else {
                    current = current->nextGame;
                }
            }
        }
    }
    
    pthread_mutex_unlock(&g->mutex);
}

#endif
