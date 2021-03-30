#ifndef GAMESTRUCT_
#define GAMESTRUCT_

#include <stdlib.h>
#include <pthread.h>
#include "version_string.h"

struct gameList {
    struct game *currentGame;
    struct gameList *nextGame;
};

struct gameCreateCallbackWaitParam {   
    char *gameName;
    int gameID, sendTime;
};

struct game {
    int gameID, started;
    long startTime;
};

// Resource free stuff
void freeGameCreateCallbackWaitParam(struct gameCreateCallbackWaitParam *g) {
    free(g->gameName);
    free(g);
}

void freeGameListNode(struct gameList *gl) {
    free (gl->currentGame);   
    free (gl);
}

void freeGameList(struct gameList *g) {
    struct gameList *current = g;
    
    while (current->nextGame != NULL) {     
        struct gameList *tmp = current;   
           
        tmp = current;
        current = current->nextGame;
        
        freeGameListNode (tmp);
    }
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

struct game *getGameWithID(struct gameList *list, int gameID) { 
    struct gameList *current = list;
    while (current != NULL && current->currentGame->gameID != gameID)
        current = current->nextGame;
    
    if (current == NULL)
        return NULL;
    
    return current->currentGame;
}

void addGame (struct gameList **list, struct game *gamePointer) {       
    
    if (list == NULL)
        return;
    
    if (*list == NULL) {        
        struct gameList *next = (struct gameList *) malloc(sizeof(struct gameList));
        next->currentGame = gamePointer;
        next->nextGame = NULL;
        
        *list = next;
    } else {    
        struct gameList *current = *list;
        
        while (current->nextGame != NULL)
            current = current->nextGame;
        
        struct gameList *next = (struct gameList *) malloc(sizeof(struct gameList));
        next->currentGame = gamePointer;
        next->nextGame = NULL;
            
        current->nextGame = next;
    }    
}

void removeGame (struct gameList **list, struct game *gamePointer) {        
    if (list == NULL) {        
        return;
    } else {            
        struct gameList *current = *list;                
        if (current->currentGame == gamePointer) {
            struct gameList *next = current->nextGame;
            freeGameListNode(current);
            
            *list = next;
        }        
        
        int found = 0;
        while (current->nextGame != NULL && !found) {
            if (current->nextGame->currentGame == gamePointer) {
                //Remove  
                struct gameList *next = current->nextGame;              
                
                current->nextGame = next->nextGame;
                freeGameListNode(next);
                found = 1;
            } else {            
                current = current->nextGame;            
            }
        }
    }
}

#endif
