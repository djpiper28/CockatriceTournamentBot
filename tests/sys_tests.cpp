#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <curl/curl.h>
#include <pthread.h>
#include "../src/bot_conf.h"

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

void fail(char *reason) {
   fprintf(stderr, "Fail: %s\n", reason);
   exit(1);
}

struct args_struct {
	  int argc;
	  char **argv;
};

void *start_src_main(void *args_in) {
	  struct args_struct *args = (struct args_struct *) args_in;
	  src_main(args->argc, args->argv);
	  pthread_exit(NULL);
}

#define ERROR "error 404"
#define WAIT 5

int main(int argc, char **argv) {
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

    std::string base_url = config.bindAddr;
		
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

		// Do not free anything as I am lazy
	  return status;
}

