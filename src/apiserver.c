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
#include "botcflags.h"

#include "room_commands.pb.h"
#include "commands.pb.h"
#include "game_commands.pb.h"
#include "get_pb_extension.h"
#include "response.pb.h"

//Internal connection struct
struct ServerConnection {
    struct apiServer *api;
    struct gameCreateCallbackWaitParam *param;
    int isGameCreate;
    int closing;
    long startTime;
};

void initServerConnection(struct ServerConnection *s, struct apiServer *api) {
    s->startTime = time(NULL);
    s->isGameCreate = 0;
    s->param = NULL;
    s->api = api;
    s->closing = 0;
}

void initServer(struct apiServer *server, 
                struct triceBot *triceBot, 
                struct Config config) {
    server->bottleneck = PTHREAD_MUTEX_INITIALIZER;
    server->config = config;
    server->triceBot = triceBot;
    server->running = 0;
}

//TODO: rewrite create game code

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

static void serverCreateGameCommand(struct ServerConnection *s, 
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
            char *tmp = (char *) malloc(sizeof(char) * BUFFER_LENGTH);
            
            for (int j = firstEquals + 1, ii = 0; 
                 j < lineEnd && ii < BUFFER_LENGTH; 
                 j++) {
                tmp[ii] = hm->body.ptr[j];        
                
                ii++;
            }
            
            //Read prop tag into prop
            int propLen = firstEquals - lineStart;
            
            //Error case - no prop len
            if (propLen == 0) {
                free(tmp);
                continue;
            }
            
            char *prop = (char *) malloc(sizeof(char) * propLen);
            
            
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
                if (strncmp(tmp, "TRUE", BUFFER_LENGTH)) {
                    isNum = 1;
                    number = 1;
                } else if (strncmp(tmp, "FALSE", BUFFER_LENGTH)) {
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
                    readNumberIfPropertiesMatch(number, 
                                                &spectatorsAllowed, 
                                                "spectatorsAllowed", 
                                                prop);
                    readNumberIfPropertiesMatch(number, 
                                                &spectatorsNeedPassword, 
                                                "spectatorsNeedPassword", 
                                                prop);
                    readNumberIfPropertiesMatch(number, 
                                                &spectatorsCanChat, 
                                                "spectatorsCanChat", 
                                                prop);
                    readNumberIfPropertiesMatch(number, 
                                                &spectatorsCanSeeHands, 
                                                "spectatorsCanSeeHands", 
                                                prop);
                    readNumberIfPropertiesMatch(number, 
                                                &onlyRegistered, 
                                                "onlyRegistered", 
                                                prop);
                }
                
                //Free tmp here as it is not assigned to a ptr
                free(tmp);    
            }
        }
    }
                
    //Check all fields have data
    int valid = authToken != NULL && s->api->config.authToken != NULL 
        && gameName != NULL && password != NULL 
        && playerCount != -1 && spectatorsAllowed != -1 
        && spectatorsNeedPassword != -1 && spectatorsCanChat != -1 
        && spectatorsCanSeeHands != -1 && onlyRegistered != -1;    
                    
    if (valid) { 
        //Check authtoken 
        if (strncmp(authToken, s->api->config.authToken, BUFFER_LENGTH) == 0) {
            s->param = sendCreateGameCommand(s->api->triceBot,
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
            s->isGameCreate = 1;
            
            printf("[INFO]: Creating game called '%s'\n", gameName);
        } else {
            sendInvalidAuthTokenResponse(c);
        }
    } 
    
    //Free the temp vars
    if (gameName != NULL)
        free(gameName);
    if (authToken != NULL)
        free(authToken);
    if (password != NULL)
        free(password);
    
    if (!valid) {
        printf("[ERROR]: Invalid game create command.\n");
        send404(c);
    } 
}

static void eventHandler(struct mg_connection *c,
                         int event,
                         void *ev_data,
                         void *fn_data) {
    struct apiServer *api = (struct apiServer *) c->fn_data;
    
    if (event == MG_EV_ACCEPT) {
        struct mg_tls_opts opts = {
            .cert = api->opts.cert,
            .certkey = api->opts.certkey,
        };
        
        mg_tls_init(c, &opts);
        
        //Init connection
        struct ServerConnection *s = (struct ServerConnection *) malloc(sizeof(struct ServerConnection));
        initServerConnection(s, api);
        
        c->fn_data = s;
    } else if (event == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;
        struct ServerConnection *s = (struct ServerConnection *) c->fn_data;
        api = s->api;
                
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
            serverCreateGameCommand(s, c, hm, fn_data);
        } else if (mg_http_match_uri(hm, "/api/")) {
            mg_http_reply(c, 200, HELP_STR);  
        } else {
            send404(c);
        }
    } else if (event == MG_EV_POLL && c->is_accepted) {         
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;
        struct ServerConnection *s = (struct ServerConnection *) c->fn_data;
        
        if (s->isGameCreate) {
            struct gameCreateCallbackWaitParam *paramdata = s->param;
            api = s->api;
            
            pthread_mutex_lock(&paramdata->mutex);
            int ID = paramdata->gameID;
            pthread_mutex_unlock(&paramdata->mutex);
                    
            if (ID != -1) { 
                char *data = (char *) malloc(sizeof(char) * BUFFER_LENGTH);
                snprintf(data, BUFFER_LENGTH, "gameid=%d", paramdata->gameID);  
                printf("[INFO]: Game created with ID %d.\n", paramdata->gameID);
                
                mg_http_reply(c, 200, data);  
                c->fn_data = NULL;
                
                free(data);
                freeGameCreateCallbackWaitParam(paramdata);
            } 
        } 
        
        //Timeout
        else if (time(NULL) - s->startTime > TIMEOUT && !s->closing) {
            mg_http_reply(c, 408, "timeout error");
            s->closing = 1;
        } else if (s->closing) {
            c->is_closing = 1;
        }
    } else if ((event == MG_EV_CLOSE || event == MG_EV_ERROR) && c->is_client) {
        struct ServerConnection *s = (struct ServerConnection *) c->fn_data;
        if (s != NULL) free(s);
    }
    
    (void) fn_data;
}

static void *pollingThread(void *apiIn) {
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
        mg_mgr_poll(&mgr, 50);  
        
        pthread_mutex_lock(&api->bottleneck); 
        cont = api->running;    
        pthread_mutex_unlock(&api->bottleneck);
    }
        
    mg_mgr_free(&mgr);    
    pthread_exit(NULL);
}

int startServer(struct apiServer *api) {  
    pthread_mutex_lock(&api->bottleneck);
    api->opts.cert = api->config.cert;
    api->opts.certkey = api->config.certkey;
    api->running = 1;
    pthread_mutex_unlock(&api->bottleneck);
    
    return pthread_create(&api->pollingThreadT, NULL, pollingThread, (void *) api);
}

void stopServer(struct apiServer *api) {    
    pthread_mutex_lock(&api->bottleneck);
    api->running = 0;
    pthread_mutex_unlock(&api->bottleneck);
    
    pthread_join(api->pollingThreadT, NULL);
}

#endif
