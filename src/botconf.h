#ifndef BOTCONF_
#define BOTCONF_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ncurses.h>
#include "ncursesinterface.h"

#define BUFFER_LENGTH 1024
#define TOKEN_LENGTH 32
#define SUCCESS 1
#define ERROR 0
#define HEX_BIT_MASK 0xF

struct config {
    char *cockatriceUsername, *cockatricePassword, *cockatriceServer, 
         *authToken, *cert, *certkey, *bindAddr, *clientID, *roomName;
    int success;
};

struct config config;

void readProperty(char *line) {    
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
    
    int ii = 0;
    //Ends with new line
    for (int i = equalsSignPtr + 1; ii < valueLen - 1; i++) {
        valueStr[ii] = line[i];
        ii++;
    }
        
    if (strncmp("username", propertyStr, BUFFER_LENGTH) == 0) {        
        config.cockatriceUsername = valueStr;      
    } else if (strncmp("password", propertyStr, BUFFER_LENGTH) == 0) {        
        config.cockatricePassword = valueStr;            
    } else if (strncmp("serveraddress", propertyStr, BUFFER_LENGTH) == 0) {        
        config.cockatriceServer = valueStr;
    } else if (strncmp("authtoken", propertyStr, BUFFER_LENGTH) == 0) {
        config.authToken = valueStr;               
    } else if (strncmp("certfile", propertyStr, BUFFER_LENGTH) == 0) {
        config.cert = valueStr;               
    } else if (strncmp("certkeyfile", propertyStr, BUFFER_LENGTH) == 0) {
        config.certkey = valueStr;   
    } else if (strncmp("bindAddr", propertyStr, BUFFER_LENGTH) == 0) {
        config.bindAddr = valueStr;   
    } else if (strncmp("clientID", propertyStr, BUFFER_LENGTH) == 0) {
        config.clientID = valueStr;   
    } else if (strncmp("roomName", propertyStr, BUFFER_LENGTH) == 0) {
        config.roomName = valueStr;   
    } else {      
        attron(RED_COLOUR_PAIR);
        printw("ERROR: No setting with property '%s' found on line with value %s\n",
               propertyStr, valueStr);    
        attroff(RED_COLOUR_PAIR);
        
        refresh();  
        free(valueStr);   
    }
    
    free(propertyStr);
}

//Grabbed from the mongoose source code because I am very lazy.
int toBase64 (int c) {               
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

char toHex(char c) {
    //Assumes char c is 4 bits of data and returns the hex character
    if (c < 10) {
        return '0' + c;
    } else { 
        return 'A' + (c - 10);
    }
} 

char* strToHex(const char *str, int len) {
    char *str2 = (char *) malloc(sizeof(char) * 2 * len + 1);
        
    for (int i = 0; i < len; i++) {
        str2[2 * i] = toHex(str[i] & HEX_BIT_MASK);
        str2[2 * i + 1] = toHex((str[i] >> 4) & HEX_BIT_MASK);  
    }
    str2[2 * len] = 0;
    
    return str2;
}

void makeNewFile() {
    // No file
    printw("Config file does not exist, creating a new one.\nThe values will need changing.\nPlease do not add spaces.\n");    
    refresh();
    
    FILE * configFile = fopen("config.conf", "w+");
    if (configFile != 0 && access("config.conf", W_OK) == 0) {
        // Create a new config file
        config.success = 0;
        
        char *generatedAuthToken = (char *) 
        malloc(sizeof(char) * (TOKEN_LENGTH + 1));
        
        //32 (TOKEN_LENGTH) base64 chars
        for (int i = 0; i < TOKEN_LENGTH; i++) {
            generatedAuthToken[i] = toBase64(rand() % 64);
        }
        generatedAuthToken[TOKEN_LENGTH] = 0;
        
        fprintf(configFile,
                "username=changeme\npassword=changeme\nserveraddress=ws://server.cockatrice.us:4748\nauthtoken=%s\ncertfile=server.pem\ncertkeyfile=server.pem\nbindAddr=https://0.0.0.0:8000\nclientID=changeme\nroomName=Magic\n",
                generatedAuthToken);       
        
        fclose (configFile);    //close file like a good boy
    } else {
        // Invalid permissions to make a new file
        config.success = -1;
        
        printw("Error creating new config file!\n");   
        refresh();
    }
    
    printw("Exiting until a valid, readable config files exists.\n");   
    refresh();
    exitCurses();
}

void readConf() {
    printw("Reading config files...\n");    
    refresh();
    
    if (access("config.conf", F_OK) == 0) {
        if (access("config.conf", R_OK) == 0) {
            // Read file
            config.success = SUCCESS;
            
            FILE * configFile = fopen("config.conf", "r");
            
            char * lineBuffer = (char *) malloc(sizeof(char) * BUFFER_LENGTH);
            while (fgets(lineBuffer, BUFFER_LENGTH, configFile) != NULL) {  
                // Process line         
                
                printw("Reading configuration line: %s", lineBuffer);    
                refresh();
                readProperty(lineBuffer);      
            }
            
            free(lineBuffer);       //free that bad boi
            fclose (configFile);    //close file like a good boy                    
        } else {
            // Unreadable file
            config.success = ERROR;
            
            attron(RED_COLOUR_PAIR);
            printw("ERROR: Error reading config file - exiting.\n");    
            attroff(RED_COLOUR_PAIR);
            
            refresh();
            exitCurses();
        }
    } else { 
        makeNewFile();        
    }
    
    attron(YELLOW_COLOUR_PAIR);
    printw("INFO: Read config file::\n===============\nusername=%s\npassword=%s\nserveraddress=%s\nauthtoken=%s\ncertfile=%s\ncertkeyfile=%s\nbindAddr=%s\nclientID=%s\nroomName=%s\n===============\n", 
           config.cockatriceUsername, config.cockatricePassword,
           config.cockatriceServer, config.authToken, config.cert, 
           config.certkey, config.bindAddr, config.clientID, config.roomName);
    attroff(YELLOW_COLOUR_PAIR);
    
    refresh();
}

#endif
