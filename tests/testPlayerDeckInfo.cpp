#include <string.h>
#include "testPlayerDeckInfo.h"
#include "../src/player_deck_info.h"

CPPUNIT_TEST_SUITE_REGISTRATION(TestPlayerDeckInfo);

TestPlayerDeckInfo::TestPlayerDeckInfo () : CppUnit::TestCase("player_deck_info.h tests") {}

void TestPlayerDeckInfo::testInitAndFree() {
    int count = 5;
    char **hashes = (char **) malloc(sizeof(char *) * count);
    for (int i = 0; i < count; i++) {
        hashes[i] = (char *) malloc(sizeof(char) * 9);
        snprintf(hashes[i], 9, "hash1234");
    }    
    struct playerDeckInfo pdi = initPlayerDeckInfo(hashes, count, "test-Player", 0);
    for (int i = 0; i < count; i++) {
        free(hashes[i]);
    }
    
    for (int i = 0; i < count; i++) {
        hashes[i] = (char *) malloc(sizeof(char) * 9);
        snprintf(hashes[i], 9, "");
    }    
    pdi = initPlayerDeckInfo(hashes, count, "test-Player", 0);
    for (int i = 0; i < count; i++) {
        free(hashes[i]);
    }
    
    for (int i = 0; i < count; i++) {
        hashes[i] = (char *) malloc(sizeof(char) * 9);
        snprintf(hashes[i], 9, "a");
    }    
    pdi = initPlayerDeckInfo(hashes, count, "test-Player", 0);   
    for (int i = 0; i < count; i++) {
        free(hashes[i]);
    }
    
    for (int i = 0; i < count; i++) {
        int len = strlen("hash123456789");
        hashes[i] = (char *) malloc(sizeof(char) * len);
        snprintf(hashes[i], len, "hash123456789");
    }    
    pdi = initPlayerDeckInfo(hashes, count, "test-Player", 0);    
    for (int i = 0; i < count; i++) {
        free(hashes[i]);
    }
    
    for (int i = 0; i < count; i++) {
        hashes[i] = (char *) malloc(sizeof(char) * 9);
        snprintf(hashes[i], 9, "hash1234");
    }
    int len = 10000;
    char *longStr = (char *) malloc(sizeof(char) * len);
    for(int i = 0; i < len - 1; i++) {
        longStr[i] =  'a';
    }
    longStr[len] = 0;
    
    pdi = initPlayerDeckInfo(hashes, count, longStr, 0);    
    for (int i = 0; i < count; i++) {
        free(hashes[i]);
    }
}

void TestPlayerDeckInfo::testDeckFilter() {
    // Init test
    int max_players = 5;
    int count = 5;
    char **hashes = (char **) malloc(sizeof(char *) * count);
    for (int i = 0; i < count; i++) {
        hashes[i] = (char *) malloc(sizeof(char) * 9);
        snprintf(hashes[i], 9, "hash1234");
    }
    
    struct playerDeckInfo *pdi = initPlayerDeckInfoArr(max_players);
    pdi[0] = initPlayerDeckInfo(hashes, count, "test-player", 0);
    for (int i = 1; i < max_players; i++) {        
        char *name = (char *) malloc(sizeof(char) * 256);
        snprintf(name, 256, "test-Player-%d", i);
        pdi[i] = initPlayerDeckInfo(hashes, count, name, 0);
        free(name);
    }
    
    struct game *g = createGame(1, max_players, gameDataForPlayerDeckInfo(pdi));
    int allowed;
    
    // Not in list or player deck info
    allowed = isPlayerAllowed("bad", 0, *g);
    CPPUNIT_ASSERT(!allowed);
    
    // Accepting state - in list and player deck info
    allowed = isPlayerAllowed("test-player", 0, *g);    
    CPPUNIT_ASSERT(allowed);
    
    // Slot used up
    allowed = isPlayerAllowed("test-player", 0, *g);    
    CPPUNIT_ASSERT(!allowed);
    
    // Fill the list and test that these players are allowed
    for (int i = 1; i < max_players; i++) {
        char *name = (char *) malloc(sizeof(char) * 256);
        snprintf(name, 256, "test-Player-%d", i);
        
        allowed = isPlayerAllowed(name, 0, *g);    
        CPPUNIT_ASSERT(allowed);
        free(name);
    }
    
    // Check that nobody is allowed to join when the slots are empty
    for (int i = max_players; i < max_players + 10; i++) {
        allowed = isPlayerAllowed("test-Player", 0, *g);
        CPPUNIT_ASSERT(!allowed);
    }
    
    // Test wildcard
    pdi[0] = initPlayerDeckInfo(hashes, count, "*", 0);
    allowed = isPlayerAllowed("any old player", 0, *g);
    CPPUNIT_ASSERT(allowed);
    
    allowed = isPlayerAllowed("any old player", 0, *g);
    CPPUNIT_ASSERT(!allowed);
        
    for (int i = 0; i < count; i++) {
        free(hashes[i]);
    }
    freePlayerDeckInfoArray(pdi);
}

void TestPlayerDeckInfo::testPlayerFilter() {
    int count = 5;
    char **hashes = (char **) malloc(sizeof(char *) * count);
    for (int i = 0; i < count; i++) {
        hashes[i] = (char *) malloc(sizeof(char) * 9);
        snprintf(hashes[i], 9, "hash1234");
    }    
    struct playerDeckInfo pdi = initPlayerDeckInfo(hashes, count, "test-Player", 0);
    for (int i = 0; i < count; i++) {
        free(hashes[i]);
    }
}
