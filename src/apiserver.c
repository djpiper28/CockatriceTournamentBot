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

static void initServerConnection(struct ServerConnection *s, 
                                 struct apiServer *api) {
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
    
    if (mg_url_is_ssl(server->config.bindAddr)) {
        server->opts.cert = server->config.cert;
        server->opts.certkey = server->config.certkey;
        server->opts.ca = NULL;//server->config.cert;
    }
}

void freeServer(struct apiServer *api) {
    pthread_mutex_destroy(&api->bottleneck);
}

static void sendInvalidAuthTokenResponse(struct mg_connection *c) {
    mg_http_reply(c, 401, "", "invalid auth token");
}

static void send404(struct mg_connection *c) {
    mg_http_reply(c, 404, "", "error 404");  
}

static void readNumberIfPropertiesMatch(int number, 
                                        int *dest, 
                                        const char *property, 
                                        char *readProperty) {
    if (strncmp(property, readProperty, BUFFER_LENGTH) == 0) {
        *dest = number;
    }
}

struct str {
    const char *ptr;
    size_t len;
};

static struct str readNextLine(const char *buffer, 
                               size_t *ptr, 
                               size_t len) {    
    size_t i = 0;
    size_t tmp = *ptr;
    while (*ptr < len && buffer[*ptr] != '\n') { 
        i++;
        *ptr += 1;
    }
    *ptr += 1;
    
    struct str string = {buffer + tmp, i};
    return string;
}

static void serverCreateGameCommand(struct ServerConnection *s, 
                                    struct mg_connection *c, 
                                    struct mg_http_message *hm) {
    char *authToken = NULL, 
         *gameName = NULL, 
         *password = NULL;
    int playerCount = -1, 
        spectatorsAllowed = -1, 
        spectatorsNeedPassword = -1,
        spectatorsCanChat = -1, 
        spectatorsCanSeeHands = -1, 
        onlyRegistered = -1;
            
    //Read the buffer line by line
    size_t ptr = 0;
    while (ptr < hm->body.len - 1) {
        struct str line = readNextLine(hm->body.ptr, &ptr, hm->body.len);
        
        size_t eqPtr = 0;
        for (; eqPtr < line.len && line.ptr[eqPtr] != '='; eqPtr++);
        
        //Check is line has equals with non-null strings on each side
        if (eqPtr < line.len - 1 && eqPtr > 1) {
            size_t valueLen = line.len - eqPtr - 1;
            size_t propLen = eqPtr;
            
            //Error case - no prop len
            if (propLen != 0) {
                //Read value of the tag
                char *tmp = (char *) malloc(sizeof(char) * (valueLen + 1));
                size_t j = 0;
                for (size_t i = eqPtr + 1; j < valueLen; i++) {
                    tmp[j] = line.ptr[i];
                    j++;
                }
                tmp[valueLen] = 0;
                
                //Read prop tag into prop
                char *prop = (char *) malloc(sizeof(char) * (propLen + 1));
                for (size_t i = 0; i < eqPtr; i++)
                    prop[i] = line.ptr[i]; 
                prop[propLen] = 0;            
                
                //Process line
                //Prop len
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
                    number = atoi(tmp);
                    
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
            
            printf("[INFO]: Creating game called '%s' with %d players\n", 
                   gameName, 
                   playerCount);
        } else {
            sendInvalidAuthTokenResponse(c);
        }
    } else {        
        printf("[ERROR]: Invalid game create command.\n");
        send404(c);
    }
    
    //Free the temp vars
    if (gameName != NULL)
        free(gameName);
    if (authToken != NULL)
        free(authToken);
    if (password != NULL)
        free(password);
}

static void ErrorCallback(struct gameCreateCallbackWaitParam *param) {
    printf("[ERROR]: Game create callback error - api client disconnected.\n");
}

static void eventHandler(struct mg_connection *c,
                         int event,
                         void *ev_data,
                         void *fn_data) {
    struct apiServer *api = (struct apiServer *) c->fn_data;
    
    if (event == MG_EV_ACCEPT) {
        //Init connection struct
        struct ServerConnection *s = (struct ServerConnection *)
            malloc (sizeof(struct ServerConnection));
        initServerConnection(s, api);
        
        c->fn_data = (void *) s;        
        
        if (mg_url_is_ssl(api->config.bindAddr)) {
            mg_tls_init(c, &api->opts);
        }
    } else if (event == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;
        struct ServerConnection *s = (struct ServerConnection *) c->fn_data;
        api = s->api;
                
        if (mg_http_match_uri(hm, "/github/")) {
            mg_http_reply(c, 301, "", "<meta http-equiv=\"refresh\" content=\"0; URL=%s\" />", 
                          GITHUB_REPO);
        } else if (mg_http_match_uri(hm, "/api/version/")) {
            mg_http_reply(c, 200, "", "v%d.%d", 
                        VERSION_MAJOR, VERSION_MINOR);  
        } else if (mg_http_match_uri(hm, "/api/checkauthkey/")) {
            mg_http_reply(c, 200, "", "valid=%d", strncmp(hm->body.ptr,
                                                          api->config.authToken, 
                                                          BUFFER_LENGTH) == 0); 
        } else if (mg_http_match_uri(hm, "/api/creategame/")) { 
            serverCreateGameCommand(s, c, hm);            
        } else if (mg_http_match_uri(hm, "/api/")) {
            mg_http_reply(c, 200, "", HELP_STR);
        } else if (mg_http_match_uri(hm, "/replay*")) {
            #define DOWNLOAD_HEADER "Content-Disposition: attachment\r\n"
            
            struct mg_http_serve_opts opts = {
                .root_dir = api->config.replayFolder,
                .extra_headers = DOWNLOAD_HEADER
            };
            
            mg_http_serve_dir(c, hm, &opts);
        } else{
            send404(c);
        }      
    } else if (event == MG_EV_POLL && c->is_accepted) {         
        struct ServerConnection *s = (struct ServerConnection *) c->fn_data;
        
        if (s != NULL) {
            if (s->isGameCreate) {
                struct gameCreateCallbackWaitParam *paramdata = s->param;
                api = s->api;
                
                pthread_mutex_lock(&paramdata->mutex);
                int ID = paramdata->gameID;
                pthread_mutex_unlock(&paramdata->mutex);
                        
                if (ID != -1) {                     
                    char *data = (char *) malloc(sizeof(char) * BUFFER_LENGTH);
                    char *replayName = getReplayFileName(paramdata->gameID,
                                                         paramdata->gameName);
                    
                    snprintf(data, BUFFER_LENGTH, "gameid=%d\nreplayName=%s", 
                             paramdata->gameID,
                             replayName);
                    
                    printf("[INFO]: Game created with ID %d.\n",
                           paramdata->gameID);
                    
                    mg_http_reply(c, 200, "", data);  
                    
                    free(data);
                    free(replayName);
                    freeGameCreateCallbackWaitParam(paramdata);
                    
                    s->isGameCreate = 0;
                }
            } 
            
            //Timeout
            else if (time(NULL) - s->startTime > TIMEOUT && !s->closing) {
                mg_http_reply(c, 408, "", "timeout error");
                s->closing = 1;
            } else if (s->closing) {
                c->is_closing = 1;
            }
        }
    } else if ((event == MG_EV_CLOSE || event == MG_EV_ERROR) && c->is_accepted) {
        struct ServerConnection *s = (struct ServerConnection *) c->fn_data;
        
        if (s != NULL) { 
            api = s->api;
            if (s->isGameCreate) {
                pthread_mutex_lock(&s->param->mutex);
                
                int ID = s->param->gameID;
                if (ID != -1) {
                    freeGameCreateCallbackWaitParam(s->param);
                    free(s);
                } else {
                    s->param->callbackFn = &ErrorCallback;
                }
                
                pthread_mutex_unlock(&s->param->mutex);            
            }
            c->fn_data = NULL;
        }
    }
    
    (void) fn_data;
}

static void *pollingThread(void *apiIn) {
    struct mg_mgr mgr;
    struct mg_connection *c;
    struct apiServer *api = (struct apiServer *) apiIn;
    
    mg_mgr_init(&mgr);
    
    c = mg_http_listen(&mgr, 
                       api->config.bindAddr, 
                       eventHandler, 
                       apiIn);
    
    if (c == NULL) {
        //TODO: Handle error state
    }
    
    pthread_mutex_lock(&api->bottleneck);    
    int cont = api->running;
    pthread_mutex_unlock(&api->bottleneck);
    
    while (cont) {   
        mg_mgr_poll(&mgr, 100);  
        
        pthread_mutex_lock(&api->bottleneck); 
        cont = api->running;    
        pthread_mutex_unlock(&api->bottleneck);
    }
        
    mg_mgr_free(&mgr);    
    pthread_exit(NULL);
}

int startServer(struct apiServer *api) {     
    pthread_mutex_lock(&api->bottleneck);      
    if (api->running) {        
        pthread_mutex_unlock(&api->bottleneck);
        printf("[ERROR]: Server has already started.\n");
        return 0;
    } else {    
        printf("[INFO]: Starting api server, listening on %s\n", api->config.bindAddr);
        
        #if _SSL
        printf("-> SSL enabled. cert: %s & certkey: %s.\n",
               api->config.cert,
               api->config.certkey);
        #endif
        
        api->running = 1;
        pthread_mutex_unlock(&api->bottleneck);
        
        return pthread_create(&api->pollingThreadT, 
                            NULL, 
                            pollingThread, 
                            (void *) api);
    }
}

void stopServer(struct apiServer *api) {    
    pthread_mutex_lock(&api->bottleneck);
    api->running = 0;
    pthread_mutex_unlock(&api->bottleneck);
    
    pthread_join(api->pollingThreadT, NULL);
}

#endif
