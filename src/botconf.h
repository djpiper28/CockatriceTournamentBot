#ifndef BOTCONF_H_
#define BOTCONF_H_

#include "botcflags.h"

#define BUFFER_LENGTH 4096 //aprox one page
#define CONF_FILE "config.conf"

struct Config {
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
         *certkey, 
         *authToken,
         *bindAddr;
};

/**
 * Reads the configuration from CONF_FILE to *config.
 * Returns 1 if the file was read successfully
 * Returns 0 if there was an error reading the file
 * Returns -1 if the file doesn't exist and was made
 */ 
int readConf(struct Config *config);

void freeConf(struct Config *config);

#endif
