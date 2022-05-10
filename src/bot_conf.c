#ifndef BOTCONF_
#define BOTCONF_
#include "logger.h"
#include "bot_conf.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define TOKEN_LENGTH 32
#define HEX_BIT_MASK 0xF

static void readProperty(char *line, int length, struct Config *config)
{
    // Don't read comments which are lines starting with #
    if (length > 1) {
        if (line[0] == '#') {
            return;
        }
    }

    int equalsSignPtr = -1, valueLen = 1;

    for (int i = 0; i < length && line[i] != 0 && line[i] != '\n'; i++) {
        if (line[i] == '=' && equalsSignPtr == -1) {
            equalsSignPtr = i;
        } else if (equalsSignPtr != -1) {
            valueLen ++;
        }
    }

    int propertyLen = equalsSignPtr + 1;

    //Guard statement to stop empty property tags being processed
    if (propertyLen == 0 || valueLen == 0 || equalsSignPtr == -1) {
        return;
    }

    char *propertyStr = (char *) malloc(sizeof(char) * propertyLen),
          *valueStr   = (char *) malloc(sizeof(char) * valueLen);

    for (int i = 0; i < propertyLen - 1; i++) { //null terminator
        propertyStr[i] = line[i];
    }

    propertyStr[propertyLen - 1] = 0;

    int ii = 0;

    //Ends with new line
    for (int i = equalsSignPtr + 1; ii < valueLen - 1; i++) {
        valueStr[ii] = line[i];
        ii++;
    }

    valueStr[valueLen - 1] = 0;

    //Read the config data
#if LOGIN_AUTOMATICALLY
    if (strncmp("username", propertyStr, length) == 0) {
        if (config->cockatriceUsername != NULL) free(config->cockatriceUsername);
        config->cockatriceUsername = valueStr;
    } else if (strncmp("password", propertyStr, length) == 0) {
        if (config->cockatricePassword != NULL) free(config->cockatricePassword);
        config->cockatricePassword = valueStr;
    } else
#endif

#if JOIN_ROOM_AUTOMATICALLY
        if (strncmp("roomName", propertyStr, length) == 0) {
            if (config->roomName != NULL) free(config->roomName);
            config->roomName = valueStr;
        } else
#endif

            if (strncmp("serveraddress", propertyStr, length) == 0) {
                if (config->cockatriceServer != NULL) free(config->cockatriceServer);
                config->cockatriceServer = valueStr;
            } else if (strncmp("authtoken", propertyStr, length) == 0) {
                if (config->authToken != NULL) free(config->authToken);
                config->authToken = valueStr;
            } else if (strncmp("ratelimit", propertyStr, length) == 0) {
                int temp = atoi(valueStr);
                if (config->maxMessagesPerSecond == 0) {
                    lprintf(LOG_ERROR, "Rate limit is not set a valid value\n");
                } else {
                    config->maxMessagesPerSecond = temp;
                }
                free(valueStr);
            } else if (strncmp("replayFolder", propertyStr, length) == 0) {
                if (config->replayFolder != NULL) free(config->replayFolder);
                config->replayFolder = valueStr;

                //Remove trailing slashes i,e:
                //    test///// to test
                //    test/test/ to test/test
                for (int i = valueLen - 2, trailingSlashes = 1;
                        i >= 0 && trailingSlashes;
                        i--) {
                    if (config->replayFolder[i] == '/') {
                        config->replayFolder[i] = 0;
                    } else {
                        trailingSlashes = 0;
                    }
                }

                // Warn the user that the location is absolute
                // and they might be trying to write to /replays/
                if (valueLen >= 1) {
                    if (config->replayFolder[0] == '/') {
                        lprintf(LOG_WARNING, "Replay folder is an absolute location (%s)\n",
                               config->replayFolder);
                    }
                }
            } else if (strncmp("clientID", propertyStr, length) == 0) {
                if (config->clientID != NULL) free(config->clientID);
                config->clientID = valueStr;
            } else

                //API Server config
                if (strncmp("certfile", propertyStr, length) == 0) {
                    if (config->cert != NULL) free(config->cert);
                    config->cert = valueStr;
                } else if (strncmp("certkeyfile", propertyStr, length) == 0) {
                    if (config->certkey != NULL) free(config->certkey);
                    config->certkey = valueStr;
                } else if (strncmp("bindAddr", propertyStr, length) == 0) {
                    if (config->bindAddr != NULL) free(config->bindAddr);
                    config->bindAddr = valueStr;
                } else if (strncmp("externURL", propertyStr, length) == 0) {
                    config->externURL = valueStr;
                }

    //Free string if not used
                else {
                    free(valueStr);
                }

    free(propertyStr);
}

//Grabbed from the mongoose source code because I am very lazy.
static int toBase64(int c)
{
    if (c < 0) {
        return toBase64(-c);
    } else {
        if (c < 26) {
            return c + 'A';
        } else if (c < 52) {
            return c - 26 + 'a';
        } else if (c < 62) {
            return c - 52 + '0';
        } else {
            return c == 62 ? '+' : '/';
        }
    }
}

// Creates a new file with the arbitary settings, is not path safe
void makeNewFile(char *filename)
{
    // No file
    FILE * configFile = fopen(filename, "w+");

    if (configFile != NULL && access(filename, W_OK) == 0) {
        // Create a new config file;
        char *generatedAuthToken = (char *)
                                   malloc(sizeof(char) * (TOKEN_LENGTH + 1));

        //32 (TOKEN_LENGTH) base64 chars
        for (int i = 0; i < TOKEN_LENGTH; i++) {
            generatedAuthToken[i] = toBase64(rand() % 64);
        }

        generatedAuthToken[TOKEN_LENGTH] = 0;

        fprintf(configFile, "username=changeme\n"); // vv
        fprintf(configFile, "password=changeme\n"); // For auto login
        fprintf(configFile, "serveraddress=ws://server.cockatrice.us:4748\n");
        fprintf(configFile, "roomName=Magic\n"); //For auto room join
        fprintf(configFile, "replayFolder=changeme\n");
        fprintf(configFile, "ratelimit=5\n");

        //Tournament bot data TODO: move them elsewhere
        fprintf(configFile, "authtoken=%s\n", generatedAuthToken);
        fprintf(configFile, "certfile=server.pem\n");
        fprintf(configFile, "certkeyfile=server.pem\n");

        fprintf(configFile, "bindAddr=https://0.0.0.0:8000\n");
        fprintf(configFile, "clientID=changeme\n");
        fprintf(configFile, "externURL=https://0.0.0.0:8000\n");

        free(generatedAuthToken);
        fclose(configFile); //close file like a good boy
    } else {
        // Invalid permissions to make a new file
        lprintf(LOG_ERROR, "Unable to create config.conf.\n");
    }
}

#define INIT_CONFIG \
config->cockatriceUsername = NULL;\
config->cockatricePassword = NULL;\
config->roomName = NULL;\
config->cockatriceServer = NULL;\
config->clientID = NULL;\
config->replayFolder = NULL;\
config->cert = NULL;\
config->certkey = NULL;\
config->authToken = NULL;\
config->bindAddr = NULL;\
config->maxMessagesPerSecond = -1;\
config->externURL = NULL;

// Reads the configuration from a buffer: char *data, of length: int length
void readConfFromBuffer(struct Config *config, char *data, int length)
{
    INIT_CONFIG

    int lineStartPtr = 0, lineEndPtr = -1;
    for (int i = 0; i < length; i++) {
        if (i == length - 1 || data[i] == '\n') {
            lineEndPtr = i - 1;
            if (i == length -1) lineEndPtr++; // Edge case
            readProperty(data + lineStartPtr,
                         1 + lineEndPtr - lineStartPtr,
                         config);
            lineStartPtr = i + 1;
        }
    }
}

/**
 * Reads the configuration from CONF_FILE to *config.
 * Returns 1 if the file was read successfully
 * Returns 0 if there was an error reading the file
 * Returns -1 if the file doesn't exist and was made
 */
int readConf(struct Config *config, char *filename)
{
    INIT_CONFIG

    if (access(filename, F_OK) == 0) {
        if (access(filename, R_OK) == 0) {
            // Read file
            FILE * configFile = fopen(filename, "r");

            // Guard statement - null file pointer due to unreadable file
            if (configFile == NULL) {
                return 0;
            }

            char *lineBuffer = (char *) malloc(sizeof(char) * BUFFER_LENGTH);

            while (fgets(lineBuffer, BUFFER_LENGTH, configFile) != NULL) {
                // Process line
                readProperty(lineBuffer,
                             BUFFER_LENGTH,
                             config);
            }

            free(lineBuffer); //free that bad boi
            fclose(configFile); //close file like a good boy
            return 1;
        } else {
            // Unreadable file
            return 0;
        }
    } else {
        makeNewFile(filename);
        return -1;
    }
}

void freeConf(struct Config *config)
{
#if LOGIN_AUTOMATICALLY

    if (config->cockatriceUsername != NULL) {
        free(config->cockatriceUsername);
    }

    if (config->cockatricePassword != NULL) {
        free(config->cockatricePassword);
    }

#endif

#if JOIN_ROOM_AUTOMATICALLY

    if (config->roomName != NULL) {
        free(config->roomName);
    }

#endif

    if (config->cockatriceServer != NULL) {
        free(config->cockatriceServer);
    }

    if (config->clientID != NULL) {
        free(config->clientID);
    }

    //Tournament bot data TODO: move them elsewhere
    if (config->cert != NULL) {
        free(config->cert);
    }

    if (config->certkey != NULL) {
        free(config->certkey);
    }

    if (config->authToken != NULL) {
        free(config->authToken);
    }

    if (config->bindAddr != NULL) {
        free(config->bindAddr);
    }
}

#endif
