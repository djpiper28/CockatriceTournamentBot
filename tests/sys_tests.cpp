// C headers
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include <pthread.h>
#include "../src/bot_conf.h"

// C++ headers
#include <iostream>
#include <ios>
#include <sstream>
#include <vector>
#include <fstream>
#include <string>
#include <list>

#define NET_ERROR 1
#define MAIN_FUNC src_main
#include "../src/main.c"

struct CurlResponse {
    char *ptr;
    size_t len;
};

size_t write_callback(char *ptr_in,
                      size_t size,
                      size_t nmemb,
                      void *userdata)
{
    struct CurlResponse *response = (struct CurlResponse *) userdata;
    if (response->ptr == NULL) {
        response->ptr = (char *) malloc((size * nmemb) + 1);
        if (response->ptr == NULL) {
            std::cerr << "Error write_callback() : malloc failed."
                      << std::endl;
            return 0;
        }

        response->ptr[size * nmemb] = '\0';
        response->len = size * nmemb;
        memcpy(response->ptr, ptr_in, size * nmemb);
    } else {
        // We have to sellotape the chunks together
        response->ptr = (char *) realloc(response->ptr,
                                         response->len + (size * nmemb) + 1);
        if (response->ptr == NULL) {
            std::cerr << "Error write_callback() : realloc failed."
                      << std::endl;
            return 0;
        }

        response->ptr[response->len + (size * nmemb)] = '\0';
        memcpy(response->ptr + response->len, ptr_in, size * nmemb);
        response->len += size * nmemb;
    }

#ifdef DEBUG
    std::cerr << "Debug write_callback() : " << response->ptr << std::endl;
#endif

    return size * nmemb;
}

void freeCurlResponse(struct CurlResponse *resp)
{
    if (resp->ptr) {
        free(resp->ptr);
        resp->ptr = NULL;
    }
}

std::string sendRequest(std::string url,
                        std::string data)
{
#ifdef DEBUG
    std::cerr << "Debug sendRequest() : The URL is " << url
              << std::endl;
#endif

    CURL *curl = curl_easy_init();
    if(curl) {
        CURLcode res;

        struct CurlResponse response = {NULL, 0};

        // Set timeouts
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 2L);

        // Set url, user-agent and, headers
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_USE_SSL, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "cockatrice-tournament-bot");
        curl_easy_setopt(curl, CURLOPT_HTTPPOST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data.size());

        // Set response write
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &response);

        res = curl_easy_perform(curl);

        bool getSuccess = res == CURLE_OK && response.ptr != NULL;

        std::string resp;
        if (getSuccess) {
            resp = std::string(response.ptr);
        }

        curl_easy_cleanup(curl);
        freeCurlResponse(&response);

        if (!getSuccess) {
            if (res != CURLE_OK) {
                std::cerr << "Error sendRequest() : curl perform "
                          "failed after send."
                          << std::endl;
                fprintf(stderr,
                        "curl_easy_perform() failed: %s\n",
                        curl_easy_strerror(res));
            } else {
                std::cerr << "Error sendRequest() : no response was read."
                          << std::endl;
            }
            throw NET_ERROR;
        }

        return resp;
    } else {
        std::cerr << "Error sendRequest() : curl init failed."
                  << std::endl;
        throw NET_ERROR;
    }
}

std::string getRequest(std::string url)
{
#ifdef DEBUG
    std::cerr << "Debug getRequest() : The URL is " << url<< std::endl;
#endif

    CURL *curl = curl_easy_init();
    if(curl) {
        CURLcode res;

        struct CurlResponse response = {NULL, 0};

        // Set timeouts
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 2L);

        // Set url, user-agent and, headers
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_USE_SSL, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "cockatrice-tournament-bot");
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);

        // Set response write
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &response);

        res = curl_easy_perform(curl);

        bool getSuccess = res == CURLE_OK && response.ptr != NULL;

        std::string resp;
        if (getSuccess) {
            resp = std::string(response.ptr);
        }

        curl_easy_cleanup(curl);
        freeCurlResponse(&response);

        if (!getSuccess) {
            if (res != CURLE_OK) {
                std::cerr << "Error getRequest() : curl perform failed."
                          << std::endl;
                fprintf(stderr, "curl_easy_perform() failed: %s\n",
                        curl_easy_strerror(res));
            } else {
                std::cerr << "Error getRequest() : no response was read."
                          << std::endl;
            }
            throw NET_ERROR;
        }

        return resp;
    } else {
        std::cerr << "Error getRequest() : curl init failed."
                  << std::endl;
        throw NET_ERROR;
    }
}

void fail(char *reason)
{
    fprintf(stderr, "Fail: %s\n", reason);
    exit(1);
}

struct args_struct {
    int argc;
    char **argv;
};

void *start_src_main(void *args_in)
{
    struct args_struct *args = (struct args_struct *) args_in;
    src_main(args->argc, args->argv);
    pthread_exit(NULL);
}

std::list<std::string> get_lines(std::string text)
{
    std::list<std::string> ret;
    std::string line;
    std::stringstream streamData(text);
    while (std::getline(streamData, line, '\n')) {
        ret.push_back(line);
    }

    return ret;
}

std::string get_tag(std::string in)
{
    std::string ret = in;
    size_t index = ret.find("=");
    if (index == std::string::npos) return ""; // Failed to find

    ret = ret.substr(0L, index);
    printf("INFO-tag: %s\n", ret.c_str());

    return ret;
}

std::string get_prop(std::string in)
{
    std::string ret = in;
    size_t index = ret.find("=");
    if (index == std::string::npos) return ""; // Failed to find

    ret = ret.substr(index + 1L);
    printf("INFO-prop: %s\n", ret.c_str());

    return ret;
}

#define STR(x) std::string(x)

#define GAME_NAME STR("game/name")
#define PASSWORD STR("password123")
#define P1_NAME STR("dave")
#define P1_NAME_2 STR("eric")
#define P1_DECK_1 STR("12345678")
#define P2_NAME STR("hans_gruber_from_die_hard")
#define P2_DECK_1 STR("23456789")
#define P2_DECK_2 STR("34567890")

#define ERROR "error 404"
#define WAIT 5
#define endl STR("\n")

struct game_info {
    int gameid;
    std::string replayname;
};

struct game_info test_create_game(struct Config config, int pdi)
{
    std::string api_request = STR("authtoken=") + config.authToken;
    api_request += endl;
    api_request += STR("gamename=") + GAME_NAME;
    api_request += endl;
    api_request += STR("password=") + PASSWORD;
    api_request += endl;
    api_request += STR("playerCount=2");
    api_request += endl;
    api_request += STR("spectatorsAllowed=1");
    api_request += endl;
    api_request += STR("spectatorsNeedPassword=1");
    api_request += endl;
    api_request += STR("spectatorsCanChat=1");
    api_request += endl;
    api_request += STR("spectatorsCanSeeHands=1");
    api_request += endl;
    api_request += STR("onlyRegistered=1");
    api_request += endl;

    // Player deck verification
    if (pdi) {
        api_request += STR("playerDeckVerification=1");
        api_request += endl;
        api_request += STR("playerName=") + P1_NAME;
        api_request += endl;
        api_request += STR("deckHash=") + P1_DECK_1;
        api_request += endl;

        api_request += STR("playerName=") + P2_NAME;
        api_request += endl;
        api_request += STR("deckHash=") + P2_DECK_1;
        api_request += endl;
        api_request += STR("deckHash=") + P2_DECK_2;
        api_request += endl;
    }

    // Make request
    int gameid = -1;
    std::string replayname = "";
    std::string base_url = STR(config.bindAddr);
    std::string ret = sendRequest(base_url + STR("/api/creategame"), api_request);
    if (ret == "timeout error") fail("timeout error for game create");
    if (ret == "invalid player info") fail("invalid player info for game create");

    // Get data from api call
    std::string line;
    std::list<std::string> lines = get_lines(ret);
    while (lines.size() > 0) {
        line = lines.back();
        lines.pop_back();
        std::string tag = get_tag(line);
        std::string prop = get_prop(line);

        // Guard against invalid or empty lines
        if (tag == "" || prop == "") continue;

        if (tag == "gameid") {
            gameid = atoi(prop.c_str());
        }
        if (tag == "replayName") {
            replayname = prop;
        }
    }

    // Check if valid
    if (gameid < 1) fail("no gameid for game create"); // also checks for atoi fails
    if (replayname == "") fail("no replay name for game create");

    struct game_info info = {gameid, replayname};
    return info;
}

void test_end_game(struct Config config, struct game_info info, int expect_error)
{
    std::string request = STR("authtoken=") + config.authToken + endl;
    request += STR("gameid=") + std::to_string(info.gameid) + endl;

    std::string ret = sendRequest(config.bindAddr + STR("/api/endgame"), request);
    if (expect_error) {
        if (ret == "success") fail("end game failed, error expected");
    } else {
        if (ret != "success") fail("end game failed");
    }
}

void test_update_pdi(struct Config config, struct game_info info, int expect_error)
{
    std::string request = STR("authtoken=") + config.authToken + endl;
    request += STR("gameid=") + std::to_string(info.gameid) + endl;
    request += STR("oldplayername=") + P1_NAME + endl;
    request += STR("newplayername=") + P1_NAME_2 + endl;

		std::string resp = sendRequest(config.bindAddr + STR("/api/updateplayerinfo"), request);
    if (expect_error) {
    	  if ("success" == resp) fail("update pdi failed, expected error did not happen");
    } else {
    	  if ("success" != resp) fail("update pdi failed as an error occurred");
				
				std::string new_status_pg = getRequest(config.bindAddr + STR("/") + STR(info.replayname));
        if (new_status_pg.find(P1_NAME) != std::string::npos) fail("(pdi) name was not changed");
        if (new_status_pg.find(P1_NAME_2) == std::string::npos) fail("(pdi) name was changed wrongly");
    }
}

int test_api(struct Config config)
{
    // Create test game with no pdi then close it
    struct game_info info_no_pdi = test_create_game(config, 0);

    printf("Sleeping 1.\n");
    sleep(1);
    std::string base_url = STR(config.bindAddr);
    std::string get_status_game = getRequest(base_url + STR("/") + STR(info_no_pdi.replayname));
    if (get_status_game.find(GAME_NAME) == std::string::npos)
        fail("Failed to get game status for game with no pdi");

    test_end_game(config, info_no_pdi, 0);

    // Test not found
    struct game_info info_test = info_no_pdi;
    info_test.gameid = 180;
    test_end_game(config, info_test, 1);
    test_update_pdi(config, info_test, 1);

		// Create test game with pdi then test the unique pdi endpoints
    struct game_info info_pdi = test_create_game(config, 1);

    printf("Sleeping 1.\n");
    get_status_game = getRequest(base_url + STR("/") + STR(info_pdi.replayname));
    if (get_status_game.find(GAME_NAME) == std::string::npos)
        fail("Failed to get game status for game with pdi");

    if (get_status_game.find(P1_NAME) == std::string::npos) fail("p1 name not found");
    if (get_status_game.find(P1_DECK_1) == std::string::npos) fail("p1 deck not found");

    if (get_status_game.find(P2_NAME) == std::string::npos) fail("p2 name not found");
    if (get_status_game.find(P2_DECK_1) == std::string::npos) fail("p2 hash 1 not found");
    if (get_status_game.find(P2_DECK_2) == std::string::npos) fail("p2 hash 2 not found");

    // Test update methods for the game with pdi
    test_update_pdi(config, info_pdi, 0);
    
    // Test disable pdi
		
		// Test end game
    test_end_game(config, info_pdi, 0);

    return 0;
}

int main(int argc, char **argv)
{
    struct args_struct args_in = {argc, argv};
    pthread_t main_thread;
    pthread_create(&main_thread, NULL, start_src_main, (void *) &args_in);

    // Wait for startup to complete
    fprintf(stderr, "Waiting %d to start system tests\n", WAIT);
    for (int i = 0; i < WAIT; i++) {
        fprintf(stderr, "%d...\n", WAIT - i);
        sleep(1);
    }

    fprintf(stderr, "Tests have started\n.");
    int status = 0;

    struct Config config;
    status |= !readConf(&config, "config.conf");
    if (status) fail("Cannot read config.conf");
    if (config.bindAddr == NULL) fail("NULL bind address");

    std::string base_url = STR(config.bindAddr);

    // Test api page
    status = ERROR == getRequest(base_url + "/api");
    if (status) fail("Cannot get /api");

    status = ERROR == getRequest(base_url + "/api/");
    if (status) fail("Cannot get /api/");

    // Test faq page
    status = ERROR == getRequest(base_url + "/faq");
    if (status) fail("Cannot get /faq");

    status = ERROR == getRequest(base_url + "/faq/");
    if (status) fail("Cannot get /faq/");

    // Test index page
    status = ERROR == getRequest(base_url + "/");
    if (status) fail("Cannot get /faq");

    // Test discord
    status = ERROR == getRequest(base_url + "/discord");
    if (status) fail("Cannot get /discord");

    status = ERROR == getRequest(base_url + "/discord/");
    if (status) fail("Cannot get /discord/");

    // Test robots
    status = ERROR == getRequest(base_url + "/robots.txt");
    if (status) fail("Cannot get /robots.txt");

    // Test status
    status = ERROR == getRequest(base_url + "/status");
    if (status) fail("Cannot get /status");

    status = ERROR == getRequest(base_url + "/status/");
    if (status) fail("Cannot get /status/");

    // Test version
    status = ERROR == getRequest(base_url + "/api/version");
    if (status) fail("Cannot get /api/version");

    status = ERROR == getRequest(base_url + "/api/version/");
    if (status) fail("Cannot get /api/version/");

    std::string token = std::string(config.authToken);

    // Check auth key
    status = "valid=1" != sendRequest(base_url + "/api/checkauthkey", token);
    if (status) fail("Cannot checkauthkey");

    status = test_api(config);

    // Do not free anything as I am lazy
    return status;
}

