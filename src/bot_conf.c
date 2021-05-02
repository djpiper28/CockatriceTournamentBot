#ifndef BOTCONF_
#define BOTCONF_

#include "bot_conf.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define TOKEN_LENGTH 32
#define HEX_BIT_MASK 0xF

static void readProperty(char *line, struct Config *config) {
    int equalsSignPtr = -1, valueLen = 1;
    
    for (int i = 0; i < BUFFER_LENGTH && line[i] != 0 && line[i] != '\n'; i++) {
        if (line[i] == '=' && equalsSignPtr == -1) {
            equalsSignPtr = i;
        } else if (equalsSignPtr != -1) {
            valueLen ++;
        }
    }
    
    int propertyLen = equalsSignPtr + 1;
    
    char *propertyStr = (char *) malloc(sizeof(char) * propertyLen),
          *valueStr    = (char *) malloc(sizeof(char) * valueLen);
          
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
    
    if (strncmp("username", propertyStr, BUFFER_LENGTH) == 0) {
        config->cockatriceUsername = valueStr;
    } else if (strncmp("password", propertyStr, BUFFER_LENGTH) == 0) {
        config->cockatricePassword = valueStr;
    } else
#endif
    
#if JOIN_ROOM_AUTOMATICALLY
        if (strncmp("roomName", propertyStr, BUFFER_LENGTH) == 0) {
            config->roomName = valueStr;
        } else
#endif
        
            if (strncmp("serveraddress", propertyStr, BUFFER_LENGTH) == 0) {
                config->cockatriceServer = valueStr;
            } else if (strncmp("authtoken", propertyStr, BUFFER_LENGTH) == 0) {
                config->authToken = valueStr;
            } else if (strncmp("replayFolder", propertyStr, BUFFER_LENGTH) == 0) {
                config->replayFolder = valueStr;
                
                //Remove trailing slashes i,e:
                //    test///// to test
                //    test/test/ to test/test
                for (int i = valueLen - 2, trailingSlashes = 1; i >= 0 && trailingSlashes; i--) {
                    if (config->replayFolder[i] == '/') {
                        config->replayFolder[i] = 0;
                    } else {
                        trailingSlashes = 0;
                    }
                }
                
                //Warn the user that the location is absolute and they might be trying to write to /replays/
                if (valueLen >= 1) {
                    if (config->replayFolder[0] == '/') {
                        printf("[WARNING]: Replay folder is an absolute location (%s)\n",
                               config->replayFolder);
                    }
                }
            } else if (strncmp("clientID", propertyStr, BUFFER_LENGTH) == 0) {
                config->clientID = valueStr;
            } else
            
                //API Server config
                if (strncmp("certfile", propertyStr, BUFFER_LENGTH) == 0) {
                    config->cert = valueStr;
                } else if (strncmp("certkeyfile", propertyStr, BUFFER_LENGTH) == 0) {
                    config->certkey = valueStr;
                } else
                
                    if (strncmp("bindAddr", propertyStr, BUFFER_LENGTH) == 0) {
                        config->bindAddr = valueStr;
                    }
                    
    //Free string if not used
                    else {
                        free(valueStr);
                    }
                    
    free(propertyStr);
}

//Grabbed from the mongoose source code because I am very lazy.
static int toBase64(int c) {
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

static void makeNewFile(struct Config *config) {
    // No file
    FILE * configFile = fopen(CONF_FILE, "w+");
    
    if (configFile != NULL && access(CONF_FILE, W_OK) == 0) {
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
        fprintf(configFile, "roomName=Magic\nauthRequired=0\n"); //For auto room join
        
        //Tournament bot data TODO: move them elsewhere
        fprintf(configFile, "authtoken=%s\n", generatedAuthToken);
        fprintf(configFile, "certfile=server.pem\n");
        fprintf(configFile, "certkeyfile=server.pem\n");
        
        fprintf(configFile, "bindAddr=https://0.0.0.0:8000\n");
        fprintf(configFile, "clientID=changeme\n");
        
        fclose(configFile);     //close file like a good boy
    } else {
        // Invalid permissions to make a new file
        printf("[ERROR]: Unable to create config.conf.\n");
    }
}

/**
 * Reads the configuration from CONF_FILE to *config.
 * Returns 1 if the file was read successfully
 * Returns 0 if there was an error reading the file
 * Returns -1 if the file doesn't exist and was made
 */
int readConf(struct Config *config) {
    if (access(CONF_FILE, F_OK) == 0) {
        if (access(CONF_FILE, R_OK) == 0) {
            // Read file
            FILE * configFile = fopen(CONF_FILE, "r");
            
            // Guard statement - null file pointer due to unreadable file
            if (configFile == NULL) {
                return 0;
            }
            
            char * lineBuffer = (char *) malloc(sizeof(char) * BUFFER_LENGTH);
            
            while (fgets(lineBuffer, BUFFER_LENGTH, configFile) != NULL) {
                // Process line
                readProperty(lineBuffer, config);
            }
            
            free(lineBuffer);       //free that bad boi
            fclose(configFile);     //close file like a good boy
            return 1;
        } else {
            // Unreadable file
            return 0;
        }
    } else {
        makeNewFile(config);
        return -1;
    }
}

void freeConf(struct Config *config) {
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
