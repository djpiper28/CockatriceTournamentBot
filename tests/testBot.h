#ifndef TEST_BOT_H
#define TEST_BOT_H

#include <cppunit/extensions/HelperMacros.h>

class TestBot : public CppUnit::TestCase {
        CPPUNIT_TEST_SUITE(TestBot);
        CPPUNIT_TEST(testTriceBotInitAndFree);
        CPPUNIT_TEST(testStartStop);
        CPPUNIT_TEST(testEventFunctionsAreCalled);
        CPPUNIT_TEST(testGameEventFunctionsAreCalled);
        CPPUNIT_TEST(testGetReplayFileName);
        CPPUNIT_TEST(testReplayDownload);
        CPPUNIT_TEST(testExecuteCallback);
        CPPUNIT_TEST(testHandleGameCreate);
        CPPUNIT_TEST_SUITE_END();
    public:
        TestBot(void);
        void testTriceBotInitAndFree();
        void testStartStop();
        void testEventFunctionsAreCalled();
        void testGameEventFunctionsAreCalled();
        void testGetReplayFileName();
        void testReplayDownload();
        void testExecuteCallback();
        void testHandleGameCreate();
};

#endif
