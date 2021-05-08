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
void TestGameStruct::testAddAndRemove() {
    struct gameList gl;
    initGameList(&gl);
    
    // Test add
    for (int i = 0; i < GAMES; i++) {
        addGame(&gl, createGame(i, 4));
        CPPUNIT_ASSERT(getGameWithID(&gl, i) != NULL);
    }
    
    // Test remove    
    for (int i = 0; i < GAMES; i++) {
        removeGame(&gl, getGameWithID(&gl, i));
        CPPUNIT_ASSERT(getGameWithID(&gl, i) == NULL);
    }
    
    // Test that duplicate games dont break the list
    for (int i = 0; i < GAMES; i++) {
        addGame(&gl, createGame(i, 4));
        addGame(&gl, createGame(i, 4));
    }
    
    for (int i = 0; i < GAMES; i++) {        
        removeGame(&gl, getGameWithID(&gl, i));
        CPPUNIT_ASSERT(getGameWithID(&gl, i) == NULL);
    }
    
    freeGameList(&gl);
}
