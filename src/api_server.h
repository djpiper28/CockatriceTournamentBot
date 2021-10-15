#ifndef APISERVER_H_
#define APISERVER_H_

#include "trice_structs.h"
#include "bot_conf.h"

#define FAVICON_NAME "favicon.png"

struct tb_apiServerStr {
    const char *ptr;
    size_t len;
};

struct tb_apiServerPropety {
    size_t eqPtr, valueLen, propLen;
    char *property;
    char *value;
};

struct tb_apiServer {
    pthread_t pollingThreadT;
    pthread_mutex_t bottleneck;
    struct mg_tls_opts opts;
    struct triceBot *triceBot;
    struct Config config;
    int running;
    char * replayFolerWildcard;
};

struct tb_apiServerPropety tb_readProperty(struct tb_apiServerStr line);

struct tb_apiServerStr tb_readNextLine(const char *buffer,
                                       size_t *ptr,
                                       size_t len);

void tb_readNumberIfPropertiesMatch(int number,
                                    int *dest,
                                    const char *property,
                                    char *readProperty);

void tb_initServer(struct tb_apiServer *server,
                   struct triceBot *triceBot,
                   struct Config config);

void tb_freeServer(struct tb_apiServer *api);

int tb_startServer(struct tb_apiServer *api);

void tb_stopServer(struct tb_apiServer *api);

#endif