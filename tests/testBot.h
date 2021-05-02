#ifndef TEST_BOT_H
#define TEST_BOT_H

#include <cppunit/extensions/HelperMacros.h>

class TestBot : public CppUnit::TestCase {
    CPPUNIT_TEST_SUITE(TestBot);
    CPPUNIT_TEST(testTriceBotInitAndFree);
    CPPUNIT_TEST_SUITE_END();
public:
    TestBot (void);
    void testTriceBotInitAndFree();
};
    
#endif
