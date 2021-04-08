#ifndef APISERVER_H_
#define APISERVER_H_

#include "trice_structs.h"
#include "botconf.h"

struct response {
    char *data;
    int len;
};

struct apiServer {
    pthread_t pollingThreadT;
    pthread_mutex_t bottleneck;
    struct mg_tls_opts opts;
    struct triceBot *triceBot; 
    struct Config config;
    int running;
};


void initServer(struct apiServer *server, 
                struct triceBot *triceBot, 
                struct Config config);

void freeServer(struct apiServer *api);

int startServer (struct apiServer *api);

void stopServer (struct apiServer *api);

#endif
