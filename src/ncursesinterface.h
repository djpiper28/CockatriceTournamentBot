#ifndef CURSESINTERFACE_
#define CURSESINTERFACE_

#include <ncurses.h>
#include <stdlib.h>
#include "gamestruct.h"
#include "version.h"
#include "running.h"

#define WHITE_COLOUR_PAIR COLOR_PAIR(0)
#define RED_COLOUR_PAIR COLOR_PAIR(1)
#define YELLOW_COLOUR_PAIR COLOR_PAIR(2)
#define GREEN_COLOUR_PAIR COLOR_PAIR(3)

struct displayData {
    int ping, threads, gamesCount, authTokens, coloursSupported;
    struct game * games;
};

struct terminalSize {
    int width, height;
};

void printBanner() {
    attron(YELLOW_COLOUR_PAIR);
    printw("INFO: %s\n-> by djpiper28 see https://www.github.com/djpiper28/%s for git repo.\n-> Version %d.%d\n",
           PROG_NAME, GITHUB_REPO, VERSION_MAJOR, VERSION_MINOR);
    attroff(YELLOW_COLOUR_PAIR);
    
    printw("\nStarting cockatrice bot...\n");
    
    refresh();
}

void initCurses() {    
    WINDOW *a = initscr();
    // Colours and that jazz
    if (has_colors()) {
        start_color();
        init_pair(0, COLOR_WHITE, COLOR_BLACK);
        init_pair(1, COLOR_RED, COLOR_BLACK);
        init_pair(2, COLOR_YELLOW, COLOR_BLACK);
        init_pair(3, COLOR_GREEN, COLOR_BLACK);
    } else {
        printw("ERROR: No colour support\n");
        exit(1);
    }
        
    scrollok(a, 1);
        
    printBanner();
}

void exitCurses() {
    #if DEBUG
    attron(RED_COLOUR_PAIR);
    printw("Press any key to exit.\n");
    attroff(RED_COLOUR_PAIR);
    refresh();
    
    getch();
    #endif
    
    endwin();
    exit(0);
}

void startCoolInterface() {    
    attron(GREEN_COLOUR_PAIR);
    printw("INFO: Started successfully.\n");
    refresh();
    attroff(GREEN_COLOUR_PAIR);
    
    while (running);
    exitCurses();
}

struct terminalSize getScreensize() {   
    struct terminalSize sizeOfWindow = {LINES, COLS};
    return sizeOfWindow;
}

#endif
