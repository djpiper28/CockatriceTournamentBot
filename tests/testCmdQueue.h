#ifndef TEST_CMD_QUEUE_H
#define TEST_CMD_QUEUE_H

#include <cppunit/extensions/HelperMacros.h>

class TestCmdQueue : public CppUnit::TestCase {
    CPPUNIT_TEST_SUITE(TestCmdQueue);
    CPPUNIT_TEST(testInitAndFree);
    CPPUNIT_TEST(testQueueOps);
    CPPUNIT_TEST(testSearchOps);
    CPPUNIT_TEST(testPrepCMD);
    CPPUNIT_TEST(testPrepEmptyCMD);
    CPPUNIT_TEST(testGameCreateCallbackFree);
    CPPUNIT_TEST_SUITE_END();
public:
    TestCmdQueue(void);
    void testInitAndFree();
    void testQueueOps();
    void testSearchOps();
    void testPrepCMD();
    void testPrepEmptyCMD();
    void testGameCreateCallbackFree();
};

#endif