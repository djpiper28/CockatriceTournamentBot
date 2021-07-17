#ifndef TESTTRICESTRUCTS_H
#define TESTTRICESTRUCTS_H
#include <cppunit/extensions/HelperMacros.h>

class TestTriceStructs : public CppUnit::TestCase {
        CPPUNIT_TEST_SUITE(TestTriceStructs);
        CPPUNIT_TEST(testTriceBotEventFunctionSetters);
        CPPUNIT_TEST_SUITE_END();
    public:
        TestTriceStructs(void);
        void testTriceBotEventFunctionSetters();
};

#endif