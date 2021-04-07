#ifndef APISERVER_H_
#define APISERVER_H_

#include "trice_structs.h"
#include "botconf.h"

void initServer(struct apiServer *server, 
                struct triceBot *triceBot, 
                struct Config config);

void startServer (struct apiServer *api);

void stopServer (struct apiServer *api);

#endif
