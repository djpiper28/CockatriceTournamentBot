#ifndef BOTCONF_H_
#define BOTCONF_H_

#include "botcflags.h"

#define SUCCESS 1
#define ERROR 0
#define BUFFER_LENGTH 1024

struct Config {
    int floodingCooldown;
    
    #if LOGIN_AUTOMATICALLY
    char *cockatriceUsername, 
         *cockatricePassword;
    #endif
    
    #if JOIN_ROOM_AUTOMATICALLY
    char *roomName;
    #endif    
    
    char *cockatriceServer, 
         *clientID,
         *replayFolder,
         
         //Tournament bot data TODO: move them elsewhere
         *cert, 
         *authToken,
         *certkey, 
         *bindAddr;
    int success;
};

int readConf(struct Config *config);

void freeConf(struct Config *config);

#endif
