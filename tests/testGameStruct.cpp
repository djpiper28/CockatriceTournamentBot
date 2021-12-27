#include <string.h>

#include "testGameStruct.h"
#include "../src/game_struct.h"

CPPUNIT_TEST_SUITE_REGISTRATION(TestGameStruct);

TestGameStruct::TestGameStruct () : CppUnit::TestCase("game_struct.h tests")
{

}

int freeCalled[GAMES];

void TestGameStruct::testInitAndFree()
{
    struct gameList gl;
    initGameList(&gl);
    CPPUNIT_ASSERT(gl.gamesHead == NULL);
    freeGameList(&gl);
}

void freeGameData(void *data)
{
    freeCalled[*(int*) data] = 1;
    free(data);
}

void TestGameStruct::testGameInfo()
{
    struct gameList gl;
    initGameList(&gl);

    char buff[10];

    // Init free called array
    for (int i = 0; i < GAMES; i++) {
        freeCalled[i] = 0;
    }

    // Test add
    for (int i = 0; i < GAMES; i++) {
        int *ptr = (int *) malloc(sizeof(int));
        *ptr = i;
        struct gameData gameData = {ptr, &freeGameData};

        addGame(&gl, createGame(i, PLAYERS, "game", gameData));
        CPPUNIT_ASSERT(getGameWithID(&gl, i) != NULL);
        struct game game = getGameWithIDNotRef(&gl, i);

        CPPUNIT_ASSERT(game.gameID == i);
        CPPUNIT_ASSERT(*(int *) game.gameData.gameDataPtr == i);
        freeGameCopy(game);

        CPPUNIT_ASSERT(freeCalled[i] == 0);
        for (int j = 0; j < PLAYERS; j++) {
            PLAYER_NAME
            addPlayer(&gl, getGameWithID(&gl, i), buff, j, 1);
        }
    }

    // Test remove
    for (int i = 0; i < GAMES; i++) {
        removeGame(&gl, getGameWithID(&gl, i));
        CPPUNIT_ASSERT(getGameWithID(&gl, i) == NULL);

        // Assert the free function was called
        CPPUNIT_ASSERT(freeCalled[i] == 1);
    }

    freeGameList(&gl);
}

void TestGameStruct::testGameCopy()
{
    struct gameList gl;
    initGameList(&gl);

    char buff[10];
    struct gameData gameData = {NULL, NULL};

    // Test add
    for (int i = 0; i < GAMES; i++) {
        addGame(&gl, createGame(i, PLAYERS, "game", gameData));
        CPPUNIT_ASSERT(getGameWithID(&gl, i) != NULL);
        struct game game = getGameWithIDNotRef(&gl, i);

        CPPUNIT_ASSERT(game.gameID == i);
        freeGameCopy(game);
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

    freeGameList(&gl);
}

void TestGameStruct::testAddAndRemove()
{
    struct gameList gl;
    initGameList(&gl);
    char buff[10];
    struct gameData gameData = {NULL, NULL};

    // Test add
    for (int i = 0; i < GAMES; i++) {
        addGame(&gl, createGame(i, PLAYERS, "game", gameData));
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
        addGame(&gl, createGame(i, PLAYERS, "game", gameData));
        addGame(&gl, createGame(i, PLAYERS, "game", gameData));
    }

    for (int i = 0; i < GAMES; i++) {
        removeGame(&gl, getGameWithID(&gl, i));
        CPPUNIT_ASSERT(getGameWithID(&gl, i) == NULL);
    }

    freeGameList(&gl);
}

void TestGameStruct::testPlayerArray()
{
    struct gameList gl;
    initGameList(&gl);
    char buff[10];
    struct gameData gameData = {NULL, NULL};

    // Test add
    for (int i = 0; i < GAMES * 2; i++) {
        addGame(&gl, createGame(i, PLAYERS, "game", gameData));
        struct game *g = getGameWithID(&gl, i);
        CPPUNIT_ASSERT(g != NULL);
        CPPUNIT_ASSERT(strcmp(g->gameName, "game") == 0);

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

void TestGameStruct::testSearchList()
{
    struct gameList gl;
    initGameList(&gl);
    char buff[10];
    struct gameData gameData = {NULL, NULL};

    // Test add
    for (int i = 0; i < GAMES * 2; i++) {
        addGame(&gl, createGame(i, PLAYERS, "game", gameData));
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

void TestGameStruct::testMisc()
{
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

