#include <stdio.h> 
#include <stdlib.h> 
#include <pthread.h>
#include <string.h>
#include "botconf.h"
#include "ncursesinterface.h"
#include "apiserver.h"
#include "trice_structs.h"
#include "bot.h"

struct tournamentBot {
    struct Config config;
    struct triceBot b;
    struct apiServer server;
    int running;
};

void stopAll(struct tournamentBot *bot) {
    printw("Stopping bot...\n"); 
    refresh();
    
    bot->running = 0;    
    stopServer(&bot->server);
    stopBot(&bot->b);
    
    exitCurses();
}

void *listenerThread (void *botIn) {
    struct tournamentBot* bot = (struct tournamentBot *) botIn;
    
    int listening = 1;
    char *commandBuffer = (char *) malloc(sizeof(char) * 1024);
    
    while (listening) {
        getstr(commandBuffer);
        
        //Parse command
        if (strncmp ("exit", commandBuffer, 1024) == 0) {
            listening = 0;
        }
    }
    
    printw("Stopped console listener thread.\n");
    refresh();
    
    free(commandBuffer);    
    stopAll(bot);
    pthread_exit(NULL);
}

int startConsoleListener (struct tournamentBot *bot) {
    printw("Starting console listener.\n");
    printw("Started cockatrice bot.\n");
    refresh();
        
    pthread_t consoleListenerThread;
    if(pthread_create(&consoleListenerThread, NULL, listenerThread, (void *) bot)) {
        attron(RED_COLOUR_PAIR);
        printw("ERROR: Error creating thread\n");
        attroff(RED_COLOUR_PAIR);
        
        refresh();        
        return 0;
    }
    
    printw("Started console listener thread.\n");
    refresh();
      
    return 1;
}

int main (int argc, char * args[]) {
    mg_log_set("0");
    
    initCurses(); 
    
    struct tournamentBot bot;
    bot.running = 1;
    readConf(&bot.config);    
    initBot(&bot.b, bot.config);
    initServer(&bot.server, &bot.b, bot.config);
    
    startServer(&bot.server);
    startBot(&bot.b);
    
    if (!startConsoleListener(&bot)) {
        stopBot(&bot.b);
        stopServer(&bot.server);
    }
    
    startCoolInterface(&bot.running);
}
