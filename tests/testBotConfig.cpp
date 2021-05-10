#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "testBotConfig.h"
#include "../src/bot_conf.h"

CPPUNIT_TEST_SUITE_REGISTRATION(TestBotConfig);

TestBotConfig::TestBotConfig () : CppUnit::TestCase("bot_conf.h tests") {

}

/**
 * Test config files:
 * test1.conf
 * test2.conf
 */
#define TEST "test"
#define TEST_ONE_FILENAME "/tmp/test1.conf"
#define TEST_ONE_CONTENT \
"username=test\n"\
"password=test\n"\
"serveraddress=test\n"\
"roomName=test\n"\
"authtoken=test\n"\
"certfile=test\n"\
"certkeyfile=test\n"\
"bindAddr=test\n"\
"clientID=test\n"\
"ratelimit=69\n"\
"replayFolder=test"

#define TEST_TWO_CONTENT \
"username=test\n"\
"password=test\n"\
"serveraddress=test\n"\
"roomName=test\n"\
"authtoken=test\n"\
"certfile=test\n"\
"certkeyfile=test\n"\
"bindAddr=test\n"\
"clientID=test\n"\
"ratelimit=5\n"\
"replayFolder=test\n"

#define TEST_THREE_CONTENT \
"username=abc\n"\
"username=test\n"\
"password=\n"\
"password=test\n"\
"serveraddress=abc\n"\
"serveraddress=test\n"\
"roomName=abc\n"\
"roomName=test\n"\
"authtoken=abc\n"\
"authtoken=test\n"\
"certfile=abc\n"\
"certfile=test\n"\
"certkeyfile=abc\n"\
"certkeyfile=test\n"\
"bindAddr=abc\n"\
"bindAddr=test\n"\
"clientID=abc\n"\
"clientID=test\n"\
"replayFolder=abc\n"\
"ratelimit=5\n"\
"replayFolder=test"

#define TEST_FOUR_CONTENT \
"username=test\n"\
"password=test\n"\
"serveraddress=test\n"\
"roomName=test\n"\
"authtoken=test\n"\
"certfile=test\n"\
"certkeyfile=test\n"\
"#Test comment\n"\
"bindAddr=test\n"\
"clientID=test\n"\
"replayFolder=test\n"\
"ratelimit=5\n"\
"#Another test comment"

#define TEST_FIVE_CONTENT \
"username=test\n"\
"password=test\n"\
"=test\n"\
"test=\n"\
"serveraddress=test\n"\
"roomName=test\n"\
"authtoken=test\n"\
"certfile=test\n"\
"certkeyfile=test\n"\
"bindAddr=test\n"\
"clientID=test\n"\
"ratelimit=5\n"\
"replayFolder=test"

#define TEST_SIX_CONTENT \
"username=test\n"\
"password=test\n"\
"=test\n"\
"test=\n"\
"serveraddress=test\n"\
"roomName=test\n"\
"authtoken=test\n"\
"certfile=test\n"\
"certkeyfile=test\n"\
"bindAddr=test\n"\
"clientID=test\n"\
"ratelimit=5\n"\
"replayFolder=/tmp/test"

#define TEST_SEVEN_CONTENT \
"username=test\n"\
"password=test\n"\
"serveraddress=test\n"\
"roomName=test\n"\
"authtoken=test\n"\
"certfile=test\n"\
"certkeyfile=test\n"\
"bindAddr=test\n"\
"clientID=test\n"\
"ratelimit=5\n"\
"replayFolder=test///////"

static void writeConfigFile(char *filename, char *data) {
    FILE *f = fopen(filename, "w+");
    fprintf(f, "%s", data);
    fclose(f);
}

void TestBotConfig::testReadConf() {
    struct Config config;
    writeConfigFile(TEST_ONE_FILENAME, TEST_ONE_CONTENT);

    // Test one is a read from file test with a perfectly fomatted config
    readConf(&config, TEST_ONE_FILENAME);
    // Assert all fields are set to test
    CPPUNIT_ASSERT(strcmp(config.cockatriceUsername, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.cockatricePassword, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.roomName, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.cockatriceServer, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.clientID, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.replayFolder, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.cert, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.certkey, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.authToken, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.bindAddr, TEST) == 0);
    CPPUNIT_ASSERT(config.maxMessagesPerSecond == 69);
    freeConf(&config);

    // Test two is a file with an empty line at the end
    readConfFromBuffer(&config, TEST_TWO_CONTENT, strlen(TEST_TWO_CONTENT));
    // Assert all fields are set to test
    CPPUNIT_ASSERT(strcmp(config.cockatriceUsername, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.cockatricePassword, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.roomName, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.cockatriceServer, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.clientID, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.replayFolder, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.cert, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.certkey, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.authToken, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.bindAddr, TEST) == 0);
    CPPUNIT_ASSERT(config.maxMessagesPerSecond == 69);
    freeConf(&config);

    // Test three is a test with a duplicate username tag and an empty password
    // tag followed by a good one
    readConfFromBuffer(&config, TEST_THREE_CONTENT, strlen(TEST_THREE_CONTENT));
    // Assert all fields are set to test
    CPPUNIT_ASSERT(strcmp(config.cockatriceUsername, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.cockatricePassword, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.roomName, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.cockatriceServer, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.clientID, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.replayFolder, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.cert, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.certkey, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.authToken, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.bindAddr, TEST) == 0);
    CPPUNIT_ASSERT(config.maxMessagesPerSecond == 69);
    freeConf(&config);

    // Test that comments are not read and do not break the config reader
    readConfFromBuffer(&config, TEST_FOUR_CONTENT, strlen(TEST_FOUR_CONTENT));
    // Assert all fields are set to test
    CPPUNIT_ASSERT(strcmp(config.cockatriceUsername, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.cockatricePassword, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.roomName, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.cockatriceServer, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.clientID, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.replayFolder, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.cert, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.certkey, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.authToken, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.bindAddr, TEST) == 0);
    CPPUNIT_ASSERT(config.maxMessagesPerSecond == 69);
    freeConf(&config);


    // Test five is a file with an empty property tag
    readConfFromBuffer(&config, TEST_FIVE_CONTENT, strlen(TEST_FIVE_CONTENT));
    // Assert all fields are set to test
    CPPUNIT_ASSERT(strcmp(config.cockatriceUsername, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.cockatricePassword, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.roomName, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.cockatriceServer, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.clientID, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.replayFolder, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.cert, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.certkey, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.authToken, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.bindAddr, TEST) == 0);
    CPPUNIT_ASSERT(config.maxMessagesPerSecond == 69);
    freeConf(&config);

    // Test six is to make sure that absolute paths are accepted
    readConfFromBuffer(&config, TEST_SIX_CONTENT, strlen(TEST_SIX_CONTENT));
    // Assert all fields are set correctly
    CPPUNIT_ASSERT(strcmp(config.cockatriceUsername, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.cockatricePassword, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.roomName, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.cockatriceServer, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.clientID, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.replayFolder, "/tmp/test") == 0);
    CPPUNIT_ASSERT(strcmp(config.cert, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.certkey, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.authToken, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.bindAddr, TEST) == 0);
    CPPUNIT_ASSERT(config.maxMessagesPerSecond == 69);
    freeConf(&config);

    // Test seven is to make sure that trailing slashes in the replay folder name are removed
    readConfFromBuffer(&config, TEST_SEVEN_CONTENT, strlen(TEST_SEVEN_CONTENT));
    // Assert all fields are set to test
    CPPUNIT_ASSERT(strcmp(config.cockatriceUsername, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.cockatricePassword, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.roomName, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.cockatriceServer, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.clientID, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.replayFolder, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.cert, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.certkey, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.authToken, TEST) == 0);
    CPPUNIT_ASSERT(strcmp(config.bindAddr, TEST) == 0);
    CPPUNIT_ASSERT(config.maxMessagesPerSecond == 69);
    freeConf(&config);
}

#define TEST_NEW_FILE "/tmp/test_file.conf"
void TestBotConfig::testMakeNewConfFile() {
    makeNewFile(TEST_NEW_FILE);

    struct Config config;
    // Test one is a read from file test with a perfectly fomatted config
    readConf(&config, TEST_ONE_FILENAME);

    // Assert all fields are set
    CPPUNIT_ASSERT(config.cockatriceUsername != NULL);
    CPPUNIT_ASSERT(config.cockatricePassword != NULL);
    CPPUNIT_ASSERT(config.roomName != NULL);
    CPPUNIT_ASSERT(config.cockatriceServer != NULL);
    CPPUNIT_ASSERT(config.clientID != NULL);
    CPPUNIT_ASSERT(config.replayFolder != NULL);
    CPPUNIT_ASSERT(config.cert != NULL);
    CPPUNIT_ASSERT(config.certkey != NULL);
    CPPUNIT_ASSERT(config.authToken != NULL);
    CPPUNIT_ASSERT(config.bindAddr != NULL);
    CPPUNIT_ASSERT(config.maxMessagesPerSecond == 69);

    freeConf(&config);
}

