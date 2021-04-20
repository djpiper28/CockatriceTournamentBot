#ifndef BOTCONF_
#define BOTCONF_

#include "botconf.h"
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
    
    for (int i = 0; i < propertyLen - 1; i++)  //null terminator
        propertyStr[i] = line[i];
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
    } else if (strncmp("clientID", propertyStr, BUFFER_LENGTH) == 0) {
        config->clientID = valueStr;   
    } else 
        
    //Read numerical data
    if (strncmp("floodingCooldown", propertyStr, BUFFER_LENGTH) == 0) {
        config->floodingCooldown = atoi(valueStr);  
        free(valueStr);    
    } else 
            
    //API Server config
    #if _SSL    
    if (strncmp("certfile", propertyStr, BUFFER_LENGTH) == 0) {
        config->cert = valueStr;               
    } else if (strncmp("certkeyfile", propertyStr, BUFFER_LENGTH) == 0) {
        config->certkey = valueStr;   
    } else
    #endif
    
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
static int toBase64 (int c) {               
    if (c < 0) {
        return toBase64 (-c); 
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
    FILE * configFile = fopen("config.conf", "w+");
    if (configFile != 0 && access("config.conf", W_OK) == 0) {
        // Create a new config file
        config->success = SUCCESS;
        
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
        #if _SSL
        fprintf(configFile, "authtoken=%s\n", generatedAuthToken);
        fprintf(configFile, "certfile=server.pem\n");
        fprintf(configFile, "certkeyfile=server.pem\n");
        #endif
        
        fprintf(configFile, "bindAddr=https://0.0.0.0:8000\n");
        fprintf(configFile, "clientID=changeme\n");
        
        fclose (configFile);    //close file like a good boy
    } else {
        // Invalid permissions to make a new file
        config->success = ERROR;
    }
}

int readConf(struct Config *config) {
    if (access("config.conf", F_OK) == 0) {
        if (access("config.conf", R_OK) == 0) {
            // Read file
            config->success = SUCCESS;
            
            FILE * configFile = fopen("config.conf", "r");
            
            char * lineBuffer = (char *) malloc(sizeof(char) * BUFFER_LENGTH);
            while (fgets(lineBuffer, BUFFER_LENGTH, configFile) != NULL) {  
                // Process line         
                readProperty(lineBuffer, config);      
            }
            
            free(lineBuffer);       //free that bad boi
            fclose (configFile);    //close file like a good boy                
            return 1;
        } else {
            // Unreadable file
            config->success = ERROR;
            return 0;
        }
    } else { 
        makeNewFile(config);   
        return 0;
    }
}

void freeConf(struct Config *config) {
    #if LOGIN_AUTOMATICALLY
    if (config->cockatriceUsername != NULL) free (config->cockatriceUsername);
    if (config->cockatricePassword != NULL) free (config->cockatricePassword);
    #endif
    
    #if JOIN_ROOM_AUTOMATICALLY
    if (config->roomName != NULL) free (config->roomName);
    #endif
    
    if (config->cockatriceServer != NULL) free (config->cockatriceServer);
    if (config->clientID != NULL) free (config->clientID);
    
    //Tournament bot data TODO: move them elsewhere
    if (config->cert != NULL) free (config->cert);
    if (config->certkey != NULL) free (config->certkey);
    if (config->authToken != NULL) free (config->authToken);
    if (config->bindAddr != NULL) free (config->bindAddr);
}
    
#endif
