#ifndef GAMESTRUCT_
#define GAMESTRUCT_
#include "game_struct.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
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
              int playerID) {
    pthread_mutex_lock(&gl->mutex);
    int done = 0;
    for (int i = 0; !done && i < g->playerCount; i++) {
        if (g->playerArr[i].playerName == NULL) {
            size_t nameLength = strlen(playerName) + 1; //NULL terminator
            char *playerNameCP = (char *) malloc(sizeof(char) * nameLength);
            strncpy(playerNameCP, playerName, nameLength);
            
            struct player p = {playerID, playerNameCP};
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
            
            free (p->playerName);
            p->playerName = NULL;
        }
    }    
    pthread_mutex_unlock(&gl->mutex);
    
    return done;
}

// Resource free stuff
void freeGameCreateCallbackWaitParam(struct gameCreateCallbackWaitParam *gp) {
    if (gp != NULL) {
        if (gp->gameName != NULL) 
            free(gp->gameName);
        pthread_mutex_destroy(&gp->mutex);
        free(gp);
    }
}

//Not thread safe version
static void freeGameListNodeNTS(struct gameListNode *gl) {
    free (gl->currentGame->playerArr->playerName);    
    free (gl->currentGame->playerArr);    
    free (gl->currentGame);   
    free (gl);
}

void freeGameListNode(struct gameList *g, struct gameListNode *gl) {
    pthread_mutex_lock(&g->mutex);    
    freeGameListNodeNTS(gl);    
    pthread_mutex_unlock(&g->mutex);
}

void freeGameList(struct gameList *g) {
    pthread_mutex_lock(&g->mutex);
    
    struct gameListNode *current = g->gamesHead;
    
    if (current != NULL)
    while (current->nextGame != NULL) {     
        struct gameListNode *tmp = current;   
           
        tmp = current;
        current = current->nextGame;
        
        freeGameListNodeNTS (tmp);
    }
    
    pthread_mutex_unlock(&g->mutex);
    pthread_mutex_destroy(&g->mutex);
}

// Returns -1 if the player is not found
int getPlayerIDForGameIDAndName(struct gameList *g, int gameID, char *playerName) {
    pthread_mutex_lock(&g->mutex);
    int playerID = -1;
    
    // Linear search over games list
    struct gameListNode *current = g->gamesHead;
    while (current != NULL && current->currentGame->gameID != gameID)
        current = current->nextGame;
    
    if (current != NULL) {        
        struct game *g = current->currentGame;
        
        /**
         * Iterate until a player with matching name is found or there are no 
         * palyers left to check.
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
    
    pthread_mutex_unlock(&g->mutex);
    
    return playerID;
}

struct game *createGame(int gameID, int playerCount) {
    struct game *g = (struct game *) malloc(sizeof(struct game));
    g->gameID = gameID;
    g->started = 0;
    g->startTime = -1;
    g->creationTime = time(NULL);
    g->playerCount = playerCount;
    g->playerArr = (struct player *) malloc(sizeof(struct player) * playerCount);
    
    return g;
}

struct game *getGameWithID(struct gameList *g, int gameID) { 
    pthread_mutex_lock(&g->mutex);
    
    struct gameListNode *current = g->gamesHead;
    struct game * out = NULL;
    while (current != NULL && current->currentGame->gameID != gameID)
        current = current->nextGame;
    
    if (current != NULL)
        out = current->currentGame;
    
    pthread_mutex_unlock(&g->mutex);
    
    return out;
}

void addGame (struct gameList *g, struct game *gamePointer) {   
    if (g == NULL)
        return;
    
    pthread_mutex_lock(&g->mutex); 
    
    if (g->gamesHead == NULL) {        
        struct gameListNode *next = (struct gameListNode *) malloc(sizeof(struct gameListNode));
        next->currentGame = gamePointer;
        next->nextGame = NULL;
        
        g->gamesHead = next;
    } else {    
        struct gameListNode *current = g->gamesHead;
        
        while (current->nextGame != NULL)
            current = current->nextGame;
        
        struct gameListNode *next = (struct gameListNode *) malloc(sizeof(struct gameListNode));
        next->currentGame = gamePointer;
        next->nextGame = NULL;
            
        current->nextGame = next;
    } 
    
    pthread_mutex_unlock(&g->mutex);
}

void removeGame (struct gameList *g, struct game *gamePointer) {       
    pthread_mutex_lock(&g->mutex);
    
    if (g != NULL) {
        struct gameListNode *current = g->gamesHead;
        if (current->currentGame == gamePointer) {
            struct gameListNode *next = current->nextGame;
            freeGameListNodeNTS(next);
            
            g->gamesHead = next;
        }        
        
        int found = 0;
        while (current->nextGame != NULL && !found) {
            if (current->nextGame->currentGame == gamePointer) {
                //Remove  
                struct gameListNode *next = current->nextGame;              
                
                current->nextGame = next->nextGame;
                freeGameListNodeNTS(next);
                found = 1;
            } else {            
                current = current->nextGame;            
            }
        }
    }
    
    pthread_mutex_unlock(&g->mutex);
}

#endif
