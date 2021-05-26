#include "testGameStruct.h"
#include "../src/game_struct.h"

CPPUNIT_TEST_SUITE_REGISTRATION(TestGameStruct);

TestGameStruct::TestGameStruct () : CppUnit::TestCase("game_struct.h tests") {
    
}

void TestGameStruct::testInitAndFree() {
    struct gameList gl;
    initGameList(&gl);
    CPPUNIT_ASSERT(gl.gamesHead == NULL);
    freeGameList(&gl);
}

#define GAMES 256
#define PLAYERS 4
#define PLAYER_NAME snprintf(buff, 10, "player-%d", j);
void TestGameStruct::testAddAndRemove() {
    struct gameList gl;
    initGameList(&gl);
    char buff[10];
    struct gameData gameData = {NULL, NULL, NULL};
    
    // Test add
    for (int i = 0; i < GAMES; i++) {
        addGame(&gl, createGame(i, PLAYERS, gameData));
        CPPUNIT_ASSERT(getGameWithID(&gl, i) != NULL);
        for (int j = 0; j < PLAYERS; j++) {
            PLAYER_NAME
            addPlayer(&gl, getGameWithID(&gl, i), buff, j, 1);
        }
    }
    
    // Test remove    
    for (int i = 0; i < GAMES; i++) {
        removeGame(&gl, getGameWithID(&gl, i));
        CPPUNIT_ASSERT(getGameWithID(&gl, i) == NULL);
    }
    
    // Test that duplicate games dont break the list
    for (int i = 0; i < GAMES; i++) {
        addGame(&gl, createGame(i, PLAYERS, gameData));
        addGame(&gl, createGame(i, PLAYERS, gameData));
    }
    
    for (int i = 0; i < GAMES; i++) {        
        removeGame(&gl, getGameWithID(&gl, i));
        CPPUNIT_ASSERT(getGameWithID(&gl, i) == NULL);
    }
    
    freeGameList(&gl);
}

void TestGameStruct::testPlayerArray() {    
    struct gameList gl;
    initGameList(&gl);
    char buff[10];
    struct gameData gameData = {NULL, NULL, NULL};
    
    // Test add
    for (int i = 0; i < GAMES * 2; i++) {
        addGame(&gl, createGame(i, PLAYERS, gameData));        
        struct game *g = getGameWithID(&gl, i);
        CPPUNIT_ASSERT(g != NULL);
        
        for (int j = 0; j < PLAYERS; j++) {
            CPPUNIT_ASSERT(g->playerArr[j].playerName == NULL);
        }
        
        for (int j = 0; j < PLAYERS; j++) {
            PLAYER_NAME
            addPlayer(&gl, getGameWithID(&gl, i), buff, j, 1);
            CPPUNIT_ASSERT(getPlayerIDForGameIDAndName(&gl, i, buff) == j);
        }
        
        for (int j = 0; j < PLAYERS; j++) {
            PLAYER_NAME
            CPPUNIT_ASSERT(getPlayerIDForGameIDAndName(&gl, i, buff) == j);
        }
        
        for (int j = 0; j < PLAYERS; j++) {
            removePlayer(&gl, getGameWithID(&gl, i), j);
            PLAYER_NAME
            CPPUNIT_ASSERT(getPlayerIDForGameIDAndName(&gl, i, buff) == -1);
        }
    }
    
    freeGameList(&gl);
}

void TestGameStruct::testSearchList() {
    struct gameList gl;
    initGameList(&gl);
    char buff[10];
    struct gameData gameData = {NULL, NULL, NULL};
    
    // Test add
    for (int i = 0; i < GAMES * 2; i++) {
        addGame(&gl, createGame(i, PLAYERS, gameData));
        CPPUNIT_ASSERT(getGameWithID(&gl, i) != NULL);
        for (int j = 0; j < PLAYERS; j++) {
            PLAYER_NAME
            addPlayer(&gl, getGameWithID(&gl, i), buff, j, 1);
        }
    }
    
    // Test remove    
    for (int i = GAMES; i < GAMES * 2; i++) {
        removeGame(&gl, getGameWithID(&gl, i));
        CPPUNIT_ASSERT(getGameWithID(&gl, i) == NULL);
    }
    
    // Assert that non-removed games are not touched
    for (int i = 0; i < GAMES; i++) {
        CPPUNIT_ASSERT(getGameWithID(&gl, i) != NULL);
    }
    
    for (int i = 0; i < GAMES; i++) {
        for (int j = 0; j < PLAYERS; j++) {
            PLAYER_NAME
            CPPUNIT_ASSERT(getPlayerIDForGameIDAndName(&gl, i, buff) == j);
        }
    }
    
    freeGameList(&gl);
}

void TestGameStruct::testMisc() {
    removeGame(NULL, NULL);
    addGame(NULL, NULL);
    
    struct gameList gl;
    initGameList(&gl);

    struct gameListNode *gln = (struct gameListNode *) malloc(sizeof(struct gameListNode));
    gln->currentGame = NULL;
    gln->nextGame = NULL;
    freeGameListNode(&gl, gln);
    
    // Do again to assert that the mutex is not locked
    gln = (struct gameListNode *) malloc(sizeof(struct gameListNode));
    gln->currentGame = NULL;
    gln->nextGame = NULL;
    freeGameListNode(&gl, gln);
    
    freeGameList(&gl);
}

