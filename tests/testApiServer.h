#ifndef TESTAPISERVER_H_INCLUDED
#define TESTAPISERVER_H_INCLUDED

#include <cppunit/extensions/HelperMacros.h>
#include "../src/api_server.h"
#include "../src/trice_structs.h"
#include "../src/bot.h"

class TestApiServer : public CppUnit::TestCase {
        CPPUNIT_TEST_SUITE(TestApiServer);
        CPPUNIT_TEST(testInitAndFree);
        CPPUNIT_TEST(testCreateGame);
        CPPUNIT_TEST(testDownloadReplay);
        CPPUNIT_TEST(testCheckApiKey);
        CPPUNIT_TEST(testHelpPageEndPoint);
        CPPUNIT_TEST(testVersionEndPoint);
        CPPUNIT_TEST(testUtilFunctions);
        CPPUNIT_TEST_SUITE_END();
    public:
        TestApiServer(void);
        ~TestApiServer();
        void testInitAndFree();
        void testCreateGame();
        void testDownloadReplay();
        void testCheckApiKey();
        void testHelpPageEndPoint();
        void testVersionEndPoint();
        void testUtilFunctions();
    private:
        struct Config config;
        struct triceBot b;
        struct tb_apiServer server;
};

#endif
