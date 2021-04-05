#ifndef APISERVER_
#define APISERVER_

#include <stdlib.h>
#include <ncurses.h>
#include <pthread.h>
#include "botconf.h"
#include "ncursesinterface.h"
#include "gamestruct.h"
#include "version.h"
#include "helppage.h"
#include "running.h"
#include "bot.h"
#include "trice_structs.h"
#include "mongoose.h"

#include "room_commands.pb.h"
#include "commands.pb.h"
#include "get_pb_extension.h"
#include "response.pb.h"

struct response {
    char *data;
    int len;
};

struct apiServer {
    pthread_t pollingThreadT;
    struct mg_tls_opts opts;
    struct triceBot *triceBot; 
    struct Config config;
};

void initServer(struct apiServer *server, 
                struct triceBot *triceBot, 
                struct Config config) {
    server->config = config;
    server->triceBot = triceBot;
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

static void createGame(struct apiServer *api, 
                       struct mg_connection *c, 
                       struct mg_http_message *hm, 
                       void *fn_data) {
    printw("INFO: Game create command\n");
    refresh();
    
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
                    
    if (valid) {
        // Create message
        Command_CreateGame createGame;
        createGame.set_description(gameName);
        createGame.set_password(password);
        createGame.set_max_players(playerCount);
        createGame.set_join_as_spectator(1);
        
        createGame.set_spectators_allowed(spectatorsAllowed);
        createGame.set_spectators_can_talk(spectatorsCanChat);
        createGame.set_spectators_need_password(spectatorsNeedPassword);
        createGame.set_spectators_see_everything(spectatorsCanSeeHands);
        
        createGame.set_only_registered(onlyRegistered);
        
        CommandContainer cont;  
        RoomCommand *rc = cont.add_room_command();
        rc->MutableExtension(Command_CreateGame::ext)->CopyFrom(createGame);
        
        struct pendingCommand *cmd = prepCmd(api->triceBot,
                                             cont, 
                                             -1, 
                                             api->triceBot->magicRoomID);
        
        struct gameCreateCallbackWaitParam *param = (struct 
                                gameCreateCallbackWaitParam *)
                                malloc(sizeof(struct gameCreateCallbackWaitParam));        
        
        param->gameName = gameName;
        param->gameID = -1;
        param->sendTime = time(NULL);
        
        c->fn_data = (void *) param; 
        
        cmd->param = (void *) param;        
        cmd->isGame = 1;
        enq(cmd, &api->triceBot->sendQueue);                        
    } else {
        if (gameName != NULL)
            free(gameName);
    }  
        
    //Free the temp vars
    if (authToken != NULL)
        free(authToken);
    if (password != NULL)
        free(password);
    
    if (!valid) {
        attron(RED_COLOUR_PAIR);
        printw("ERROR: Invalid create game message received (%s)\n", hm->message.ptr);
        attroff(RED_COLOUR_PAIR);
        refresh();
        
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
                                        api->config.authToken, BUFFER_LENGTH) == 0); 
        } else if (mg_http_match_uri(hm, "/api/creategame/")) { 
            createGame(api, c, hm, fn_data);
        } else if (mg_http_match_uri(hm, "/api/")) {
            mg_http_reply(c, 200, HELP_STR);  
        } else {
            send404(c);
        }
    } else if (event == MG_EV_POLL && c->fn_data != NULL) {        
        struct gameCreateCallbackWaitParam *paramdata = (struct gameCreateCallbackWaitParam*) c->fn_data;
                    
        if (paramdata->gameID != -1) { 
            #if DEBUG
            printw("DEBUG: Game made\n");
            refresh();
            #endif  
            
            printw("INFO: Sending 200 as game created\n"); 
            refresh();
            
            char *data = (char *) malloc(sizeof(char) * BUFFER_LENGTH);
            snprintf(data, BUFFER_LENGTH, "gameid=%d", paramdata->gameID);                    
            mg_http_reply(c, 200, data);  
            c->fn_data = NULL;
            
            free(data);
            freeGameCreateCallbackWaitParam(paramdata);
        } else if (time(NULL) - paramdata->sendTime > TIMEOUT) {
            #if DEBUG
            printw("DEBUG: Timeout on game creation\n");
            refresh();
            #endif  
            
            printw("INFO: Sending 408 as game timeout\n");
            mg_http_reply(c, 408, "error");  
        }
        
        refresh();
    } else if (event == MG_EV_CLOSE || event == MG_EV_ERROR) {
        if (fn_data != NULL) {        
            struct gameCreateCallbackWaitParam *paramdata = (struct gameCreateCallbackWaitParam*) fn_data;          
                free(paramdata->gameName);
                free(paramdata);
                c->fn_data = NULL;
        }
    }
}

void *pollingThread (void *apiIn) {
    struct mg_mgr mgr;
    struct mg_connection *c;
    struct apiServer *api = (struct apiServer *) apiIn;
    
    mg_mgr_init(&mgr);
    
    c = mg_http_listen(&mgr, api->config.bindAddr, eventHandler, apiIn);
    
    if (c == NULL) {
        attron(RED_COLOUR_PAIR);
        printw("ERROR: Error creating https server for api handler.\n");
        attroff(RED_COLOUR_PAIR);
        
        refresh();
        exitCurses();
    }
    
    while (running) {
        mg_mgr_poll(&mgr, 50);  
    }
        
    mg_mgr_free(&mgr);    
    
    printw("Stopped API server thread...");
    refresh();
    
    pthread_exit(NULL);
}

void startServer (struct apiServer *api) {
    printw("Starting API...\nStarting server API server thread...\n");
    refresh();    
    
    //opts.ca = config.ca;
    api->opts.cert = api->config.cert;
    api->opts.certkey = api->config.certkey;
    
    if (pthread_create(&api->pollingThreadT, NULL, pollingThread, (void *) api)) {
        attron(RED_COLOUR_PAIR);
        printw("ERROR: Error creating thread\n");        
        attroff(RED_COLOUR_PAIR);
        
        refresh(); 
        
        exitCurses();
    }
    
    printw("API server thread created.\nAPI server started.\n");
    refresh();
}

void stopServer() {    
    printw("API Server stopped.\n");
    refresh();
}

#endif
