#include "testApiServer.h"

CPPUNIT_TEST_SUITE_REGISTRATION(TestApiServer);

#define AUTH_TOKEN "auth_token"

TestApiServer::TestApiServer () : CppUnit::TestCase("api_server.h tests") {
    config = {
        "username",
        "password",
        "room name",
        5,
        "sasdsad",
        "client id",
        "replays",
        "cert",
        "certkey",
        AUTH_TOKEN,
        "https://0.0.0.0:8000"
    };
    initBot(&b, config);
}

TestApiServer::~TestApiServer() {
    freeBot(&b);
    tb_freeServer(&server);
}


void TestApiServer::testInitAndFree() {
    tb_initServer(&server, &b, config);
    CPPUNIT_ASSERT(server.running == 0);
    CPPUNIT_ASSERT(server.triceBot == &b);
    CPPUNIT_ASSERT(server.opts.ca == NULL);
    
    // Assert that the config URL is https
    CPPUNIT_ASSERT(mg_url_is_ssl(config.bindAddr)); 
    CPPUNIT_ASSERT(server.opts.cert == config.cert);
    CPPUNIT_ASSERT(server.opts.certkey == config.certkey);
    
    char replayFolerWildcard[BUFFER_LENGTH];
    snprintf(replayFolerWildcard,
             BUFFER_LENGTH,
             "/%s/**/*", config.replayFolder);
    CPPUNIT_ASSERT(strncmp(server.replayFolerWildcard, replayFolerWildcard, BUFFER_LENGTH) == 0);
    
    tb_freeServer(&server);
    
    // re-init server for the rest of the tests
    tb_initServer(&server, &b, config);
}

void TestApiServer::testCreateGame() {
    
}

void TestApiServer::testDownloadReplay() {
    
}

void TestApiServer::testCheckApiKey() {

}

void TestApiServer::testHelpPageEndPoint() {
    
}

void TestApiServer::testVersionEndPoint() {
    
}

void TestApiServer::testUtilFunctions() {
    // Test read is properties match
    int input = 5;
    int destination = -1;
    char *property = "test";
    char *readProperty = "abcdef";
    
    tb_readNumberIfPropertiesMatch(input,
                                   &destination,
                                   property,
                                   readProperty);
    CPPUNIT_ASSERT(input != destination);
    
    input = 5;
    destination = -1;
    property = "test";
    readProperty = "";
    
    tb_readNumberIfPropertiesMatch(input,
                                   &destination,
                                   property,
                                   readProperty);
    CPPUNIT_ASSERT(input != destination);
    
    input = 5;
    destination = -1;
    property = "test";
    readProperty = "test";
    
    tb_readNumberIfPropertiesMatch(input,
                                   &destination,
                                   property,
                                   readProperty);
    CPPUNIT_ASSERT(input == destination);
    
    char *data = "12\n45\n78";
    size_t len = strlen(data);
    size_t ptr = 0;
    struct tb_apiServerStr lineOne = tb_readNextLine(data, &ptr, len);
    CPPUNIT_ASSERT(lineOne.ptr == data);
    CPPUNIT_ASSERT(lineOne.len == 2);
    CPPUNIT_ASSERT(ptr == 3);
        
    struct tb_apiServerStr lineTwo = tb_readNextLine(data, &ptr, len);
    CPPUNIT_ASSERT(lineTwo.ptr == data + 3);
    CPPUNIT_ASSERT(lineTwo.len == 2);
    CPPUNIT_ASSERT(ptr == 6);
    
    struct tb_apiServerPropety prop;
    data = "abc=def\nghi=jkl\nestset";
    len = strlen(data);
    ptr = 0;
    lineOne = tb_readNextLine(data, &ptr, len);
    CPPUNIT_ASSERT(lineOne.ptr == data);
    
    prop = tb_readProperty(lineOne);
    CPPUNIT_ASSERT(prop.property != NULL);
    CPPUNIT_ASSERT(prop.value != NULL);
    CPPUNIT_ASSERT(strncmp(prop.property, "abc", prop.propLen) == 0);
    CPPUNIT_ASSERT(strncmp(prop.value, "def", prop.propLen) == 0);
    
    lineTwo = tb_readNextLine(data, &ptr, len);
    CPPUNIT_ASSERT(lineTwo.ptr == data + 8);
    
    prop = tb_readProperty(lineTwo);
    CPPUNIT_ASSERT(prop.property != NULL);
    CPPUNIT_ASSERT(prop.value != NULL);
    CPPUNIT_ASSERT(strncmp(prop.property, "ghi", prop.propLen) == 0);
    CPPUNIT_ASSERT(strncmp(prop.value, "jkl", prop.propLen) == 0);
    
    struct tb_apiServerStr lineThree = tb_readNextLine(data, &ptr, len);
    prop = tb_readProperty(lineThree);
    CPPUNIT_ASSERT(prop.propLen == 0);
    CPPUNIT_ASSERT(prop.valueLen== 0);
    CPPUNIT_ASSERT(prop.property == NULL);
    CPPUNIT_ASSERT(prop.value == NULL);
}

