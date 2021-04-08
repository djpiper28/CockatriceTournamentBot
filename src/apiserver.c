#ifndef APISERVER_
#define APISERVER_
#include "apiserver.h"

#include <stdlib.h>
#include <pthread.h>
#include "botconf.h"
#include "gamestruct.h"
#include "version.h"
#include "helppage.h"
#include "trice_structs.h"
#include "bot.h"
#include "mongoose.h"

#include "room_commands.pb.h"
#include "commands.pb.h"
#include "game_commands.pb.h"
#include "get_pb_extension.h"
#include "response.pb.h"

void initServer(struct apiServer *server, 
                struct triceBot *triceBot, 
                struct Config config) {
    server->bottleneck = PTHREAD_MUTEX_INITIALIZER;
    server->config = config;
    server->triceBot = triceBot;
    server->running = 0;
}

void freeServer(struct apiServer *api) {
    pthread_mutex_destroy(&api->bottleneck);
}

static void sendInvalidAuthTokenResponse(struct mg_connection *c) {
    mg_http_reply(c, 401, "invalid auth token");
}

static void send404(struct mg_connection *c) {
    mg_http_reply(c, 404, "Error 404");  
}

static void readNumberIfPropertiesMatch(int number, 
                                        int *dest, 
                                        const char *property, 
                                        char *readProperty) {
    if (strncmp(property, readProperty, BUFFER_LENGTH) == 0) {
        *dest = number;
    }
}

static void serverCreateGameCommand(struct apiServer *api, 
                       struct mg_connection *c, 
                       struct mg_http_message *hm, 
                       void *fn_data) {
    char *authToken = NULL, 
         *gameName = NULL, 
         *password = NULL;
    int playerCount = -1, 
        spectatorsAllowed = -1, 
        spectatorsNeedPassword = -1,
        spectatorsCanChat = -1, 
        spectatorsCanSeeHands = -1, 
        onlyRegistered = -1;
    
    //Get the data from the mg_http_message
    int lineStart = 0,
        lineEnd = 0, 
        firstEquals = 0;   
    for (int i = 0; i < hm->body.len; i++) {
        //TODO: Make these into functions
        
        if (hm->body.ptr[i] == '=') {
            firstEquals = i;
        } else if (hm->body.ptr[i] == '\n' || i == hm->body.len - 1) {
            lineEnd = i == hm->body.len - 1 ? i + 1 : i;
            
            if (i == hm->body.len)
                lineEnd++;
            
            //Read value into temp
            int valueLen = lineEnd - firstEquals /*+1 for null terminator -1 to remove \n*/;
            char *tmp = (char *) malloc(sizeof(char) * valueLen);
            tmp[valueLen - 1] = 0;
            
            for (int j = firstEquals + 1, ii = 0; j < lineEnd; j++) {
                tmp[ii] = hm->body.ptr[j];        
                
                ii++;
            }
            
            //Read prop tag into prop
            int propLen = firstEquals - lineStart;
            char *prop = (char *) malloc(sizeof(char) * propLen);
            
            //Error case
            if (propLen == 0)
                continue;
            
            prop[propLen - 1] = 0;
            
            for (int j = lineStart, ii = 0; j < firstEquals; j++) {
                prop[ii] = hm->body.ptr[j];
                
                ii++;
            }
                
            // Inc line start
            lineStart = lineEnd + 1;
            
            // Check 
            #define MAX_PROP_LEN 22
            if (MAX_PROP_LEN < propLen)
                propLen = MAX_PROP_LEN;
                
            if (strncmp(prop, "authtoken", propLen) == 0) {                
                authToken = tmp;
            } else if (strncmp(prop, "gamename", propLen) == 0) {                
                    gameName = tmp;
            } else if (strncmp(prop, "password", propLen) == 0) {                
                    password = tmp;
            } else {
                //Check is number
                int isNum = valueLen < 3, 
                number = -1;
                for (int j = firstEquals + 1; j < lineEnd; j++) 
                    isNum &= hm->body.ptr[j] >= '0' && hm->body.ptr[j] <= '9';
                
                //Read number
                if (strncmp(tmp, "TRUE", valueLen)) {
                    isNum = 1;
                    number = 1;
                } else if (strncmp(tmp, "FALSE", valueLen)) {
                    isNum = 1;
                    number = 0;
                } else if (isNum) {
                    isNum = 1;
                    number = atoi(tmp);
                }
                
                if (isNum) {
                    readNumberIfPropertiesMatch(number, 
                                                &playerCount, 
                                                "playerCount", 
                                                prop);
                    readNumberIfPropertiesMatch(spectatorsAllowed, 
                                                &playerCount, 
                                                "spectatorsAllowed", 
                                                prop);
                    readNumberIfPropertiesMatch(spectatorsNeedPassword, 
                                                &playerCount, 
                                                "spectatorsNeedPassword", 
                                                prop);
                    readNumberIfPropertiesMatch(spectatorsCanChat, 
                                                &playerCount, 
                                                "spectatorsCanChat", 
                                                prop);
                    readNumberIfPropertiesMatch(spectatorsCanSeeHands, 
                                                &playerCount, 
                                                "spectatorsCanSeeHands", 
                                                prop);
                    readNumberIfPropertiesMatch(onlyRegistered, 
                                                &playerCount, 
                                                "onlyRegistered", 
                                                prop);
                } 
                
                free(tmp);    
            }
        }
    }
        
    //Check authtoken
    if (authToken != NULL) {
        if (strncmp(authToken, api->config.authToken, BUFFER_LENGTH) != 0) {
            sendInvalidAuthTokenResponse(c);
            return;
        }
    }
        
    //Check all fields have data
    int valid = authToken != NULL && gameName != NULL && password != NULL 
        && playerCount != -1 && spectatorsAllowed != -1 
        && spectatorsNeedPassword != -1 && spectatorsCanChat != -1 
        && spectatorsCanSeeHands != -1 && onlyRegistered != -1;    
                    
    if (valid)       
        c->fn_data = (void *) sendCreateGameCommand(api->triceBot,
                                                    gameName,
                                                    password,
                                                    playerCount,
                                                    1,
                                                    spectatorsAllowed,
                                                    spectatorsCanChat,
                                                    spectatorsNeedPassword,
                                                    spectatorsCanSeeHands,
                                                    onlyRegistered,
                                                    0,
                                                    NULL);
    
    //Free the temp vars
    if (gameName != NULL) {
    free(gameName);    }  
        
    if (authToken != NULL)
        free(authToken);
    if (password != NULL)
        free(password);
    
    if (!valid) {
        send404(c);
        return;
    } 
}

static void eventHandler (struct mg_connection *c,
                          int event,
                          void *ev_data,
                          void *fn_data) {
    struct apiServer *api = (struct apiServer *) fn_data;
    
    if (event == MG_EV_ACCEPT) {
        c->fn_data = NULL;
        mg_tls_init(c, &api->opts);        
    } else if (event == MG_EV_HTTP_MSG) {         
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;
                
        if (mg_http_match_uri(hm, "/github/")) {
            //TODO: http redirect or something
            mg_http_reply(c, 303, GITHUB_REPO);
        } else if (mg_http_match_uri(hm, "/api/version/")) {
            mg_http_reply(c, 200, "v%d.%d", 
                        VERSION_MAJOR, VERSION_MINOR);  
        } else if (mg_http_match_uri(hm, "/api/checkauthkey/")) {
            mg_http_reply(c, 200, "%d", strncmp(hm->body.ptr,
                                                api->config.authToken, 
                                                BUFFER_LENGTH) == 0); 
        } else if (mg_http_match_uri(hm, "/api/creategame/")) { 
            serverCreateGameCommand(api, c, hm, fn_data);
        } else if (mg_http_match_uri(hm, "/api/")) {
            mg_http_reply(c, 200, HELP_STR);  
        } else {
            send404(c);
        }
    } else if (event == MG_EV_POLL && c->fn_data != NULL && c->is_accepted) {        
        struct gameCreateCallbackWaitParam *paramdata = 
                (struct gameCreateCallbackWaitParam*) c->fn_data;
                    
        if (paramdata->gameID != -1) { 
            char *data = (char *) malloc(sizeof(char) * BUFFER_LENGTH);
            snprintf(data, BUFFER_LENGTH, "gameid=%d", paramdata->gameID);                    
            mg_http_reply(c, 200, data);  
            c->fn_data = NULL;
            
            free(data);
            freeGameCreateCallbackWaitParam(paramdata);
        } else if (time(NULL) - paramdata->sendTime > TIMEOUT) {
            mg_http_reply(c, 408, "error");  
        }        
    } else if (event == MG_EV_CLOSE || event == MG_EV_ERROR) {
        //TODO: Handle error state
    }
}

static void *pollingThread (void *apiIn) {
    struct mg_mgr mgr;
    struct mg_connection *c;
    struct apiServer *api = (struct apiServer *) apiIn;
    
    mg_mgr_init(&mgr);
    
    c = mg_http_listen(&mgr, api->config.bindAddr, eventHandler, apiIn);
    
    if (c == NULL) {
        //TODO: Handle error state
    }
    
    pthread_mutex_lock(&api->bottleneck);    
    int cont = api->running;
    pthread_mutex_unlock(&api->bottleneck);
    
    while (cont) {   
        mg_mgr_poll(&mgr, 250);  
        
        pthread_mutex_lock(&api->bottleneck); 
        cont = api->running;    
        pthread_mutex_unlock(&api->bottleneck);
    }
        
    mg_mgr_free(&mgr);    
    pthread_exit(NULL);
}

int startServer (struct apiServer *api) {  
    pthread_mutex_lock(&api->bottleneck);
    api->opts.cert = api->config.cert;
    api->opts.certkey = api->config.certkey;
    api->running = 1;
    pthread_mutex_unlock(&api->bottleneck);
    
    return pthread_create(&api->pollingThreadT, NULL, pollingThread, (void *) api);
}

void stopServer (struct apiServer *api) {    
    pthread_mutex_lock(&api->bottleneck);
    api->running = 0;
    pthread_mutex_unlock(&api->bottleneck);
    
    pthread_join(api->pollingThreadT, NULL);
}

#endif
