#ifndef TESTCOMMANDS_H_INCLUDED
#define TESTCOMMANDS_H_INCLUDED

#include <cppunit/extensions/HelperMacros.h>

class TestCommands : public CppUnit::TestCase {
    CPPUNIT_TEST_SUITE(TestCommands);
    CPPUNIT_TEST(testInitAndFree);
    CPPUNIT_TEST_SUITE_END();
public:
    TestCommands(void);
    void testInitAndFree();
};

#endif
