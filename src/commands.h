#ifndef COMMANDS_H_
#define COMMANDS_H_

#include "trice_structs.h"
#include "bot_conf.h"

// Call at the start of main
void initCommandList(struct triceBot *b);
// Call at the end of main
void freeCommandList();

#endif