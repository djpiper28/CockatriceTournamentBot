#include <stdio.h> 
#include <stdlib.h> 
#include <pthread.h>
#include <string.h>
#include "botconf.h"
#include "apiserver.h"
#include "trice_structs.h"
#include "bot.h"
#include "version.h"

struct tournamentBot {
    struct Config config;
    struct triceBot b;
    struct apiServer server;
    int running;
};

void stopAll(struct tournamentBot *bot) {
    printf("Stopping bot\n"); 
    
    bot->running = 0;    
    stopServer(&bot->server);
    stopBot(&bot->b);
}

#define LEN 1024
void startConsoleListener (struct tournamentBot *bot) {
    int listening = 1;
    char *commandBuffer = (char *) malloc(sizeof(char) * LEN);
    
    while (listening) {
        fgets(commandBuffer, LEN, stdin);
        
        //Parse command
        if (strncmp ("exit", commandBuffer, LEN) == 0) {
            listening = 0;
        }
    }
    
    free(commandBuffer);    
    stopAll(bot);
}

#ifndef DEBUG
#define DEBUG 0
#endif


#if DEBUG
#define MACRO_DEBUG_FOR_EVENT(fn, type)
void DebugFor##fn (struct triceBot *b, type event) {    
    time_t rawtime;    
    struct tm *info; 
    char buffer[80];   
    time(&rawtime);    
    info = localtime( &rawtime );    
    strftime(buffer, 80, "%x - %H:%M:%S %Z", info);
    
    printf("[DEBUG] / %s : %s\n", buffer, event.DebugString().c_str());
}

int main (int argc, char * args[]) {
    printf("INFO: %s\n-> by djpiper28 see https://www.github.com/djpiper28/%s for git repo.\n-> Version %d.%d\n",
           PROG_NAME, GITHUB_REPO, VERSION_MAJOR, VERSION_MINOR);
    printf("Starting bot\n");
    
    mg_log_set("0");
        
    struct tournamentBot bot;
    bot.running = 1;
    readConf(&bot.config);    
    initBot(&bot.b, bot.config);
    initServer(&bot.server, &bot.b, bot.config);
    
    startServer(&bot.server);
    startBot(&bot.b);
    
    #endif
    
    startConsoleListener(&bot);
}
