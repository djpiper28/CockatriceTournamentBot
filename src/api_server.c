#ifndef APISERVER_
#define APISERVER_
#include "api_server.h"

#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include "bot_conf.h"
#include "game_struct.h"
#include "version.h"
#include "helppage.h"
#include "trice_structs.h"
#include "bot.h"
#include "mongoose.h"
#include "bot_c_flags.h"
#include "cmd_queue.h"

#include "command_kick_from_game.pb.h"
#include "commands.pb.h"
#include "game_commands.pb.h"
#include "get_pb_extension.h"
#include "response.pb.h"

#define DOWNLOAD_HEADER "Content-Disposition: attachment\r\n"

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
    
    server->replayFolerWildcard = (char *) malloc(sizeof(char) * BUFFER_LENGTH);
    snprintf(server->replayFolerWildcard,
             BUFFER_LENGTH,
             "/%s/**/*", server->config.replayFolder);
}

void freeServer(struct apiServer *api) {
    pthread_mutex_destroy(&api->bottleneck);
    free(api->replayFolerWildcard);
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

static void serverKickPlayerCommand(struct ServerConnection *s,
                                    struct mg_connection *c,
                                    struct mg_http_message *hm) {
    char *authToken = NULL,
          *playerName = NULL;
    int gameID = -1;
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
            
            //Error case - no prop len or, value len
            if (propLen != 0 || valueLen != 0) {
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
                
                for (size_t i = 0; i < eqPtr; i++) {
                    prop[i] = line.ptr[i];
                }
                
                prop[propLen] = 0;
                
                //Process line
                //Prop len
#define MAX_PROP_LEN 22
                
                if (MAX_PROP_LEN < propLen) {
                    propLen = MAX_PROP_LEN;
                }
                
                if (strncmp(prop, "authtoken", propLen) == 0) {
                    authToken = tmp;
                } else if (strncmp(prop, "target", propLen) == 0) {
                    playerName = tmp;
                } else {
                    //Check is number
                    int isNum = valueLen < 8,
                        number = atoi(tmp);
                        
                    if (isNum) {
                        readNumberIfPropertiesMatch(number,
                                                    &gameID,
                                                    "gameid",
                                                    prop);
                    }
                    
                    //Free tmp here as it is not assigned to a ptr
                    free(tmp);
                }
                
                free(prop);
            }
        }
    }
    
    //Check all fields have data
    int valid = authToken != NULL && playerName != NULL && gameID != -1;
    
    if (valid) {
        //Check authtoken
        if (strncmp(authToken, s->api->config.authToken, BUFFER_LENGTH) == 0) {
            int playerID = getPlayerIDForGameIDAndName(&s->api->triceBot->gameList,
                           gameID,
                           playerName);
                           
            if (playerID == -1) {
                mg_http_reply(c, 200, "", "error not found");
            } else {
                Command_KickFromGame kickCommand;
                kickCommand.set_player_id(playerID);
                
                CommandContainer cont;
                GameCommand *gc = cont.add_game_command();
                gc->MutableExtension(Command_KickFromGame::ext)->CopyFrom(kickCommand);
                
                struct pendingCommand *cmd = prepCmd(s->api->triceBot,
                                                     cont,
                                                     gameID,
                                                     s->api->triceBot->magicRoomID);
                                                     
                enq(cmd, &s->api->triceBot->sendQueue);
                
                mg_http_reply(c, 200, "", "success");
            }
        } else {
            sendInvalidAuthTokenResponse(c);
        }
    } else {
        printf("[ERROR]: Invalid kick player command.\n");
        send404(c);
    }
    
    //Free the temp vars
    if (playerName != NULL) {
        free(playerName);
    }
    
    if (authToken != NULL) {
        free(authToken);
    }
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
            
            //Error case - no prop len or, value len
            if (propLen != 0 || valueLen != 0) {
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
                
                for (size_t i = 0; i < eqPtr; i++) {
                    prop[i] = line.ptr[i];
                }
                
                prop[propLen] = 0;
                
                //Process line
                //Prop len
#define MAX_PROP_LEN 22
                
                if (MAX_PROP_LEN < propLen) {
                    propLen = MAX_PROP_LEN;
                }
                
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
                
                free(prop);
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
    if (gameName != NULL) {
        free(gameName);
    }
    
    if (authToken != NULL) {
        free(authToken);
    }
    
    if (password != NULL) {
        free(password);
    }
}

static void ErrorCallback(struct gameCreateCallbackWaitParam *param) {
    printf("[ERROR]: Game create callback error - api client disconnected.\n");
}

static int min(int a, int b) {
    return a > b ? b : a;
}

#define MAX_MSG_LENGTH_BYTES 4096
static void eventHandler(struct mg_connection *c,
                         int event,
                         void *ev_data,
                         void *fn_data) {
    struct apiServer *api = (struct apiServer *) c->fn_data;
    
    if (event == MG_EV_ACCEPT) {
        //Init connection struct
        struct ServerConnection *s = (struct ServerConnection *)
                                     malloc(sizeof(struct ServerConnection));
        initServerConnection(s, api);
        
        c->fn_data = (void *) s;
        
        if (mg_url_is_ssl(api->config.bindAddr)) {
            mg_tls_init(c, &api->opts);
        }
    } else if (event == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;
        struct ServerConnection *s = (struct ServerConnection *) c->fn_data;
        api = s->api;
        
#if DISCORD
        
        if (mg_http_match_uri(hm, "/index")
                || mg_http_match_uri(hm, "/index/")
                || mg_http_match_uri(hm, "/")) {
            mg_http_reply(c, 301, "", "<meta http-equiv=\"refresh\" content=\"0; URL="
                          "https://discord.com/oauth2/authorize?client_id=%s&scope=bot&permissions=8\" />",
                          api->config.clientID);
        } else
#endif
        
            if (mg_http_match_uri(hm, "/github")) {
                mg_http_reply(c, 301, "",
                              "<meta http-equiv=\"refresh\" content=\"0; URL=%s\" />",
                              GITHUB_REPO);
            } else if (mg_http_match_uri(hm, "/api/version/")
                       || mg_http_match_uri(hm, "/api/version")) {
                mg_http_reply(c, 200, "", "v%d.%d",
                              VERSION_MAJOR, VERSION_MINOR);
            }
        //Check message is short enough for the api to care about
            else if (hm->body.len > MAX_MSG_LENGTH_BYTES) {
                mg_http_reply(c, 413, "",
                              "ERROR YOUR MESSAGE EXCEEDS THE MAX SIZE OF %d BYTES",
                              MAX_MSG_LENGTH_BYTES);
            }
            
            else if (mg_http_match_uri(hm, "/api/checkauthkey/")
                     || mg_http_match_uri(hm, "/api/checkauthkey")) {
                mg_http_reply(c, 200, "", "valid=%d", strncmp(hm->body.ptr,
                              api->config.authToken,
                              BUFFER_LENGTH) == 0);
            } else if (mg_http_match_uri(hm, "/api/creategame/")
                       || mg_http_match_uri(hm, "/api/creategame")) {
                serverCreateGameCommand(s, c, hm);
            } else if (mg_http_match_uri(hm, "/api/kickplayer/")
                       || mg_http_match_uri(hm, "/api/kickplayer")) {
                serverKickPlayerCommand(s, c, hm);
            } else if (mg_http_match_uri(hm, "/api/")
                       || mg_http_match_uri(hm, "/api")) {
                mg_http_reply(c, 200, "", "%s", HELP_STR);
            } else if (mg_http_match_uri(hm,
                                            api->replayFolerWildcard)) {
                int gameFinished = 0;
                int gameID = -1;
                // Parse gameId from URI in form ../(<FOLDER>/)*<NAME>-id.cor
                /** FST
                 * 0 -> 1 '-' else 0
                 * 1 -> 2 /[0-9]/ else 0
                 * state 2:
                 *   2 -> 2 [0-9]
                 *   2 -> 3 .
                 *   else 0
                 * 3 -> 4 c else 0
                 * 4 -> 5 o else 0
                 * 5 -> 6 r else 0 (accepting state)
                 */
                int state = 0, gameIdStartPtr, gameIdEndPtr;
                
                for (size_t i = 0; i < hm->uri.len; i++) {
                    switch (hm->uri.ptr[i]) {
                        case '-':
                            state = 1;
                            break;
                            
                        case '0':
                        case '1':
                        case '2':
                        case '3':
                        case '4':
                        case '5':
                        case '6':
                        case '7':
                        case '8':
                        case '9':
                            if (state == 1) {
                                state = 2;
                                gameIdStartPtr = i;
                            } else if (state != 2) {
                                state = 0;
                            }
                            
                            break;
                            
                        case '.':
                            if (state == 2) {
                                gameIdEndPtr = i - 1;
                                // To not be misleading it will point to the last byte
                                state = 3;
                            } else {
                                state = 0;
                            }
                            
                            break;
                            
                        case 'c':
                            if (state == 3) {
                                state = 4;
                            } else {
                                state = 0;
                            }
                            
                            break;
                            
                        case 'o':
                            if (state == 4) {
                                state = 5;
                            } else {
                                state = 0;
                            }
                            
                            break;
                            
                        case 'r':
                            if (state == 5) {
                                state = 6; //accepting
                            } else {
                                state = 0;
                            }
                            
                            break;
                            
                        default:
                            state = 0;
                            break;
                    }
                }
                
                // Accepting state
                if (state == 6) {
                    // +1 for off by one error, +1 for null terminator
                    int len = gameIdEndPtr - gameIdStartPtr + 2;
                    char *gameIDCp = (char *) malloc(sizeof(char) * len);
                    strncpy(gameIDCp, hm->uri.ptr + gameIdStartPtr, len);
                    
                    gameID = atoi(gameIDCp);
                    free(gameIDCp); // Free the tmp var
                    
                    struct game g = getGameWithIDNotRef(&api->triceBot->gameList, gameID);
                    gameFinished = g.gameID == -1 || g.playerArr == NULL;
                    
                    if (!gameFinished) {
                        //<li></li>
                        #define BASE_LEN 9
                        int buffLen = 0;
                        int players = 0;
                        for (int i = 0; i < g.playerCount; i++) {
                            buffLen += BASE_LEN 
                                    + strnlen(g.playerArr[i].playerName,
                                              256);
                            players++;
                        }
                        
                        char *buff = (char *) malloc(sizeof(char) * (buffLen + 1));
                        int ptr = 0;                        
                        for (int i = 0; i < g.playerCount; i++) {
                            #define BUFF_LEN 266
                            char buffTmp[BUFF_LEN];
                            snprintf(buffTmp,
                                     BUFF_LEN,
                                     "<li>%s</li>",
                                     g.playerArr[i].playerName);
                            snprintf(buff + ptr,
                                     min(buffLen - ptr, BUFF_LEN),
                                     "%s",
                                     buffTmp);
                        }
                        
                        mg_http_reply(c,
                                      200,
                                      "",
                                      "<title>Game %d (%d/%d)</title>\n"
                                      "<h1>%s</h1>"
                                      "<h3>Game %d is in progress on server '%s'.</h3>\n"
                                      "<h4>Current players are:</h4>\n"
                                      "<ol>\n%s\n</ol>\n"
                                      "<a href=\"%s\">Github Repo</a> | Version v%d.%d",
                                      gameID,
                                      players,
                                      g.playerCount,
                                      PROG_NAME,
                                      gameID,
                                      api->config.cockatriceServer,
                                      buff,
                                      GITHUB_REPO,
                                      VERSION_MAJOR,
                                      VERSION_MINOR);
                        
                        free(buff);
                    }
                    
                    freeGameCopy(g);
                } else {
                    gameFinished = 1;
                }
                
                if (gameFinished) {
                    struct mg_http_serve_opts opts = {
                        .root_dir = ".",
                        .extra_headers = DOWNLOAD_HEADER
                    };
                    
                    mg_http_serve_dir(c, hm, &opts);
                }
            } else {
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
                                                         paramdata->gameName,
                                                         paramdata->gameNameLength,
                                                         NULL);
                                                         
                    snprintf(data,
                             BUFFER_LENGTH,
                             "gameid=%d\nreplayName=%s/%s",
                             paramdata->gameID,
                             api->config.replayFolder,
                             replayName);
                             
                    printf("[INFO]: Game created with ID %d.\n",
                           paramdata->gameID);
                           
                    mg_http_reply(c,
                                  201,
                                  "",
                                  "%s",
                                  data);
                                  
                    free(data);
                    free(replayName);
                    freeGameCreateCallbackWaitParam(paramdata);
                    
                    s->isGameCreate = 0;
                }
            }
            
            //Timeout
            else if (time(NULL) - s->startTime > TIMEOUT && !s->closing) {
                mg_http_reply(c,
                              408,
                              "",
                              "timeout error");
                s->closing = 1;
            } else if (s->closing) {
                c->is_closing = 1;
            }
        }
    } else if ((event == MG_EV_CLOSE || event == MG_EV_ERROR) && c->is_accepted) {
        struct ServerConnection *s = (struct ServerConnection *) c->fn_data;
        
        if (s != NULL) {
            //api = s->api;
            
            if (s->isGameCreate) {
                pthread_mutex_lock(&s->param->mutex);
                
                int ID = s->param->gameID;
                
                if (ID != -1) {
                    freeGameCreateCallbackWaitParam(s->param);
                    free(s);
                } else {
                    s->param->callbackFn = &ErrorCallback;
                    pthread_mutex_unlock(&s->param->mutex);
                }
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
        printf("[ERROR]: Unable to init mongoose.\n");
        exit(13);
        //TODO: Handle error state!
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
    //WARNING: -probably bad
    signal(SIGPIPE, SIG_IGN);
    
    
    pthread_mutex_lock(&api->bottleneck);
    
    if (api->running) {
        pthread_mutex_unlock(&api->bottleneck);
        printf("[ERROR]: Server has already started.\n");
        return 0;
    } else {
        printf("[INFO]: Starting api server, listening on %s\n",
               api->config.bindAddr);
        printf("-> SSL enabled. cert: %s & certkey: %s.\n",
               api->config.cert,
               api->config.certkey);
        printf("-> Serving replays on /replay* and %s\n",
               api->replayFolerWildcard);
               
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
