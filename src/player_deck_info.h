#ifndef PLAYER_DECK_INFO_H_INCLUDED
#define PLAYER_DECK_INFO_H_INCLUDED

#include "game_struct.h"
#include "bot_c_flags.h"

#define PLAYER_NAME_LENGTH 256
#define MAX_DECKS 10

struct playerDeckInfo {
    char deckHash[DECK_HASH_LENGTH][MAX_DECKS];
    char playerName[PLAYER_NAME_LENGTH];
    int isEmptySlot, playerUsingSlot, deckCount;
};

struct playerDeckInfo *initPlayerDeckInfoArr(int length);

struct playerDeckInfo initPlayerDeckInfo(char **deckHash,
                                         int deckCount,
                                         char *playerName,
                                         int isEmptySlot);

void freePlayerDeckInfoArray(void* ptr);

struct gameData gameDataForPlayerDeckInfo(struct playerDeckInfo *pdi);

int isPlayerAllowed(char *playerName,
                    int playerArrayIndex,
                    struct game g);

int isPlayerDeckAllowed(char *deckHash,
                        int playerArrayIndex,
                        struct game g);

void clearPlayerSlot(int playerIndex,
                     struct game g);

#endif