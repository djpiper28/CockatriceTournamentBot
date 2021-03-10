#include <stdio.h> 
#include <stdlib.h> 
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include "botconf.h"
#include "ncursesinterface.h"
#include "apiserver.h"
#include "bot.h"
#include "running.h"

void stopAll() {
    printw("Stopping bot...\n"); 
    refresh();
    
    running = 0;
    
    stopServer();
    stopBot();
    
    exitCurses();
}

void *listenerThread (void *nothing) {
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
    stopAll();
    pthread_exit(NULL);
}

int startConsoleListener (pthread_t consoleListenerThread) {
    printw("Starting console listener.\nStarted cockatrice bot.\n");
    refresh();
        
    if(pthread_create(&consoleListenerThread, NULL, listenerThread, NULL)) {
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

void onIntt(int sig){ 
    stopAll();
}

int main (int argc, char * args[]) {
    mg_log_set("0");
    
    initCurses();        
    readConf();
        
    running = 1;    
    signal(SIGINT, onIntt); 
    
    startServer();
    startBot();
    
    pthread_t consoleListenerThread;
    if (!startConsoleListener(consoleListenerThread)) {
        stopBot();
    }
    
    startCoolInterface();
}
