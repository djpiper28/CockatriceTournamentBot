#include <string.h>

#include "testCommands.h"

#include "../src/trice_structs.h"
#include "../src/bot.h"
#include "../src/bot_conf.h"
#include "../src/commands.h"

CPPUNIT_TEST_SUITE_REGISTRATION(TestCommands);

TestCommands::TestCommands () : CppUnit::TestCase("commands.h tests") {}

void TestCommands::testInitAndFree()
{
    struct Config testConfig = {
        "username",
        "password",
        "room name",
        5,
        "sasdsad",
        "client id",
        "replay folder",
        NULL,
        NULL,
        NULL,
        NULL
    };
    struct triceBot b;
    initBot(&b, testConfig);

    initCommandList(&b);
    freeCommandList();
}
