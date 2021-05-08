#ifndef TEST_GAME_STRUCT_H
#define TEST_GAME_STRUCT_H

#include <cppunit/extensions/HelperMacros.h>

class TestGameStruct : public CppUnit::TestCase {
        CPPUNIT_TEST_SUITE(TestGameStruct);
        CPPUNIT_TEST(testInitAndFree);
        CPPUNIT_TEST(testAddAndRemove);
        CPPUNIT_TEST(testPlayerArray);
        CPPUNIT_TEST(testSearchList);
        CPPUNIT_TEST_SUITE_END();
    public:
        TestGameStruct(void);
        void testInitAndFree();
        void testAddAndRemove();
        void testPlayerArray();
        void testSearchList();
};

#endif
