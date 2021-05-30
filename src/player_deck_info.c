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

struct gameData gameDataForPlayerDeckInfo(struct playerDeckInfo *pdi) {
    struct gameData g = {
        (void *) pdi,
        &freePlayerDeckInfoArray
    };
    return g;
}

int isPlayerAllowed(char *playerName,
                    int pid,
                    struct game g) {
    // Default to not allowing
    int exactMatch = 0;
    int index = -1;
    
    if (g.gameData.gameDataPtr != NULL) {
        struct playerDeckInfo *pdi = (struct playerDeckInfo *) g.gameData.gameDataPtr;
        
        // Iterate over the slots
        for (int i = 0; i < g.playerCount && !exactMatch; i++) {
            // Check if a player is not using the slot and it can joined by p
            if (pdi[i].playerUsingSlot == -1) {
                int allowed;
                
                // Empty slots are slots with no expected player data
                if (pdi[i].isEmptySlot) {
                    allowed = 1;
                } else {                    
                    // If the playername matches the expected name exactly or;
                    exactMatch = strncmp(pdi[i].playerName,
                                         playerName,
                                         PLAYER_NAME_LENGTH) == 0;
                    
                    // If the playerName is * then they can join
                    allowed = exactMatch || strncmp(pdi[i].playerName,
                                                    "*",
                                                    PLAYER_NAME_LENGTH) == 0;
                }
                
                if (allowed) {
                    index = i;
                }
            }
        }
        
        if (index != -1) {
            pdi[index].playerUsingSlot = pid;
        }
    }
    
    return index != -1;
}

int isPlayerDeckAllowed(char *deckHash,
                        int pid,
                        struct game g) {
    // Default to not allowing
    int allowed = 0;
    if (g.gameData.gameDataPtr != NULL) {
        struct playerDeckInfo *pdi = (struct playerDeckInfo *) g.gameData.gameDataPtr;
        
        int index = -1;
        for (int i = 0; index == -1 && i < g.playerCount; i++) {
            if (pdi[i].playerUsingSlot == pid) {
                index = i;
            }
        }
        
        if (pdi[index].deckCount == 1) {
            allowed = strncmp(pdi[index].deckHash[0],
                              "*",
                              DECK_HASH_LENGTH) == 0;
        }
        
        for (int i = 0; i < pdi[index].deckCount && i < MAX_DECKS && !allowed; i++) {
            // If the deckHash is * then they can join or;
            // If the deckHash matches the expected name
            allowed = strncmp(pdi[index].deckHash[i],
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
                found = 1;
            }
        }
    }
}
