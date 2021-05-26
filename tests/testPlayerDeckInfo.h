#ifndef TESTPLAYERDECKINFO_H_INCLUDED
#define TESTPLAYERDECKINFO_H_INCLUDED

#include <cppunit/extensions/HelperMacros.h>

class TestPlayerDeckInfo : public CppUnit::TestCase {
    CPPUNIT_TEST_SUITE(TestPlayerDeckInfo);
    CPPUNIT_TEST(testInitAndFree);
    CPPUNIT_TEST(testDeckFilter);
    CPPUNIT_TEST(testPlayerFilter);
    CPPUNIT_TEST_SUITE_END();
public:
    TestPlayerDeckInfo(void);
    void testInitAndFree();
    void testDeckFilter();
    void testPlayerFilter();
};

#endif
