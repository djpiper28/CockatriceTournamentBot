#ifndef TEST_GAME_STRUCT_H
#define TEST_GAME_STRUCT_H

#include <cppunit/extensions/HelperMacros.h>

#define GAMES 256
#define PLAYERS 4
#define PLAYER_NAME snprintf(buff, 10, "player-%d", j);

class TestGameStruct : public CppUnit::TestCase {
        CPPUNIT_TEST_SUITE(TestGameStruct);
        CPPUNIT_TEST(testInitAndFree);
        CPPUNIT_TEST(testAddAndRemove);
        CPPUNIT_TEST(testPlayerArray);
        CPPUNIT_TEST(testSearchList);
        CPPUNIT_TEST(testMisc);
        CPPUNIT_TEST(testGameCopy);
        CPPUNIT_TEST(testGameInfo);
        CPPUNIT_TEST_SUITE_END();
    public:
        TestGameStruct(void);
        void testInitAndFree();
        void testAddAndRemove();
        void testPlayerArray();
        void testSearchList();
        void testMisc();
        void testGameCopy();
        void testGameInfo();
};

#endif
