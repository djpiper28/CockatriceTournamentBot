#ifndef BOTCONF_H_
#define BOTCONF_H_

#include "bot_c_flags.h"

#define BUFFER_LENGTH 4096 //aprox one page

struct Config {
#if LOGIN_AUTOMATICALLY
    char *cockatriceUsername,
         *cockatricePassword;
#endif

#if JOIN_ROOM_AUTOMATICALLY
    char *roomName;
#endif
    int maxMessagesPerSecond;
    char *cockatriceServer,
         *clientID,
         *replayFolder,

         //Tournament bot data TODO: move them elsewhere
         *cert,
         *certkey,
         *authToken,
         *bindAddr;
};

// Creates a new file with the arbitary settings, is not path safe
void makeNewFile(char *filename);

// Reads the configuration from a buffer: char *data, of length: int length
void readConfFromBuffer(struct Config *config, char *data, int length);

/**
 * Reads the configuration from CONF_FILE to *config.
 * Returns 1 if the file was read successfully
 * Returns 0 if there was an error reading the file
 * Returns -1 if the file doesn't exist and was made
 */
int readConf(struct Config *config, char *filename);

/**
 * Frees the config file, requires all the structs to
 * be malloced, which is done in readConf
 */
void freeConf(struct Config *config);

#endif