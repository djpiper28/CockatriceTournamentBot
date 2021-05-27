#include <string.h>
#include <stdlib.h>

#include "player_deck_info.h"

struct playerDeckInfo *initPlayerDeckInfoArr(int length) {    
    struct playerDeckInfo *pdi = (struct playerDeckInfo *) malloc(sizeof(struct playerDeckInfo) * length);
    return pdi;
}

struct playerDeckInfo initPlayerDeckInfo(char **deckHash,
                                         int deckCount,
                                         char *playerName,
                                         int isEmptySlot) {
    struct playerDeckInfo pdi;    
    pdi.deckCount = deckCount;
    pdi.isEmptySlot = isEmptySlot;
    pdi.playerUsingSlot = -1;
    
    for (int i = 0; i < deckCount; i++) {
        strncpy(pdi.deckHash[i], deckHash[i], DECK_HASH_LENGTH);
    }
    strncpy(pdi.playerName, playerName, PLAYER_NAME_LENGTH);
    
    return pdi;
}

void freePlayerDeckInfoArray(void *ptr) {
    free(ptr);
}

void *copyPlayerDeckInfo(void *ptr) {
    struct playerDeckInfo *pdiIn = (struct playerDeckInfo *) ptr;
    struct playerDeckInfo *pdi = (struct playerDeckInfo *) malloc(sizeof(struct playerDeckInfo));
    
    pdi->isEmptySlot = pdiIn->isEmptySlot;
    for (int i = 0; i < pdiIn->deckCount; i++) {
        strncpy(pdi->deckHash[i], pdiIn->deckHash[i], DECK_HASH_LENGTH);
    }
    strncpy(pdi->playerName, pdiIn->playerName, PLAYER_NAME_LENGTH);
    
    return (void *) pdi;
}

struct gameData gameDataForPlayerDeckInfo(struct playerDeckInfo *pdi) {
    struct gameData g = {
        (void *) pdi, 
        &freePlayerDeckInfoArray,
        &copyPlayerDeckInfo
        
    };
    return g;
}

int isPlayerAllowed(char *playerName,
                    int playerArrayIndex,
                    struct game g) {
    if (g.gameData.gameDataPtr == NULL 
        && playerArrayIndex >= 0 
        && playerArrayIndex < g.playerCount) {
        return 0;
    } else {
        struct playerDeckInfo *pdi = (struct playerDeckInfo *) g.gameData.gameDataPtr;
        // Default to not allowing
        int allowed = 0;
        
        // Iterate over the slots
        for (int i = 0; i < g.playerCount && !allowed; i++) {
            // Check if a player is not using the slot and it can joined by p
            if (pdi[i].playerUsingSlot == -1) {
                // Empty slots are slots with no expected player data
                if (pdi[i].isEmptySlot) {
                    allowed = 1;
                } else {                    
                    // If the playerName is * then they can join or;
                    // If the playername matches the expected name
                    allowed = strncmp(pdi[i].playerName, "*", PLAYER_NAME_LENGTH) == 0
                        || strncmp(pdi[i].playerName, playerName, PLAYER_NAME_LENGTH) == 0;
                }
                
                if (allowed) {
                    pdi[i].playerUsingSlot = playerArrayIndex;
                }
            }
        }     
        
        return allowed;
    }
}

int isPlayerDeckAllowed(char *deckHash,
                        int playerArrayIndex,
                        struct game g) {
    // Default to not allowing
    int allowed = 0;
    if (g.gameData.gameDataPtr != NULL 
        && playerArrayIndex >= 0 
        && playerArrayIndex < g.playerCount) {
        struct playerDeckInfo *pdi = (struct playerDeckInfo *) g.gameData.gameDataPtr;
        
        if (pdi[playerArrayIndex].deckCount == 1) {
            allowed = strncmp(pdi[playerArrayIndex].deckHash[0],
                              "*",
                              DECK_HASH_LENGTH) == 0;
        }
        
        for (int i = 0; i < pdi[playerArrayIndex].deckCount && i < MAX_DECKS && !allowed; i++) {
            // If the deckHash is * then they can join or;
            // If the deckHash matches the expected name
            allowed = strncmp(pdi[playerArrayIndex].deckHash[i],
                              deckHash,
                              DECK_HASH_LENGTH) == 0;
        }
    }
    return allowed;
}

void clearPlayerSlot(int playerIndex,
                     struct game g) {
    struct playerDeckInfo *pdi = (struct playerDeckInfo *) g.gameData.gameDataPtr;
    if (pdi != NULL) {
        int found = 0;
        for (int i = 0; i < g.playerCount && !found; i++) {
            if (pdi[i].playerUsingSlot == playerIndex) {
                pdi[i].playerUsingSlot = -1;
            }
        }
    }
}
