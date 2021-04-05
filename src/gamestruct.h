#ifndef GAMESTRUCT_
#define GAMESTRUCT_

#include <stdlib.h>
#include <pthread.h>
#include <pthread.h>

//Used for game create callback
struct gameCreateCallbackWaitParam {   
    char *gameName;
    int gameID, sendTime;
};

//Game list
struct gameList {
    struct gameListNode *gamesHead;
    pthread_mutex_t mutex;
};

// Init the game list structure
void initGameList(struct gameList *gl) {
    gl->mutex = PTHREAD_MUTEX_INITIALIZER;
}

struct gameListNode {
    struct game *currentGame;
    struct gameListNode *nextGame;
};

struct game {
    int gameID, started;
    long startTime;
};

// Resource free stuff
void freeGameCreateCallbackWaitParam(struct gameCreateCallbackWaitParam *gp) {
    free(gp->gameName);
    free(gp);
}

//Not thread safe version
static void freeGameListNodeNTS(struct gameListNode *gl) {    
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
    
    while (current->nextGame != NULL) {     
        struct gameListNode *tmp = current;   
           
        tmp = current;
        current = current->nextGame;
        
        freeGameListNodeNTS (tmp);
    }
    
    pthread_mutex_unlock(&g->mutex);
    pthread_mutex_destroy(&g->mutex);
}

//
// Methods for game
//
// Add to non-circular linked-list
struct game *createGame(int gameID) {
    struct game *output = (struct game *) malloc(sizeof(struct game));
    output->gameID = gameID;
    output->started = 0;
    return output;
};

struct game *getGameWithID(struct gameList *g, int gameID) { 
    pthread_mutex_lock(&g->mutex);
    
    struct gameListNode *current = g->gamesHead;
    while (current != NULL && current->currentGame->gameID != gameID)
        current = current->nextGame;
    
    if (current == NULL)
        return NULL;
    
    struct game * out = current->currentGame;
    pthread_mutex_unlock(&g->mutex);
    
    return out;
}

void addGame (struct gameList *g, struct game *gamePointer) {    
    pthread_mutex_lock(&g->mutex); 
    
    if (g == NULL)
        return;
    
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
    
    if (g == NULL) {        
        return;
    } else {            
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
