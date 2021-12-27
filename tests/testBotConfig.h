#ifndef TEST_BOT_CONFIG_H
#define TEST_BOT_CONFIG_H

#include <cppunit/extensions/HelperMacros.h>

class TestBotConfig : public CppUnit::TestCase
{
    CPPUNIT_TEST_SUITE(TestBotConfig);
    CPPUNIT_TEST(testReadConf);
    CPPUNIT_TEST(testMakeNewConfFile);
    CPPUNIT_TEST_SUITE_END();
public:
    TestBotConfig(void);
    void testReadConf();
    void testMakeNewConfFile();
};

#endif