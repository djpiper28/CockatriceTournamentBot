#ifndef TEST_BOT_H
#define TEST_BOT_H

#include <cppunit/extensions/HelperMacros.h>

class TestBot : public CppUnit::TestCase {
        CPPUNIT_TEST_SUITE(TestBot);
        CPPUNIT_TEST(testTriceBotInitAndFree);
        CPPUNIT_TEST(testGetReplayFileName);
        CPPUNIT_TEST(testStartStop);
        CPPUNIT_TEST(testEventFunctionsAreCalled);
        CPPUNIT_TEST(testGameEventFunctionsAreCalled);
        CPPUNIT_TEST_SUITE_END();
    public:
        TestBot(void);
        void testTriceBotInitAndFree();
        void testGetReplayFileName();
        void testStartStop();
        void testEventFunctionsAreCalled();
        void testGameEventFunctionsAreCalled();
};

#endif
