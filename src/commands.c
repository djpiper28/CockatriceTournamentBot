#include <string.h>
#include "room_commands.pb.h"
#include "game_commands.pb.h"
#include "session_commands.pb.h"
#include "version.h"
#include "commands.h"
#include "bot.h"
#include "bot_c_flags.h"
#include "cmd_queue.h"
#include "game_struct.h"

static void userMsgDiscordLink(struct triceBot *b, Event_UserMessage event) {
    pthread_mutex_lock(&b->mutex);

    char msg[BUFFER_LENGTH];
    snprintf(msg, BUFFER_LENGTH,
             "You can invite squirebot with this link: https://discord.com/oauth2/authorize?client_id=%s&scope=bot&permissions=8",
             b->config.clientID);

    Command_Message cmdMsg;
    cmdMsg.set_user_name(event.sender_name());
    cmdMsg.set_message(msg);

    CommandContainer cont;
    SessionCommand *c = cont.add_session_command();
    c->MutableExtension(Command_Message::ext)->CopyFrom(cmdMsg);

    struct pendingCommand *cmd = prepCmdNTS(b, cont, -1, -1);
    enq(cmd, &b->sendQueue);

    pthread_mutex_unlock(&b->mutex);
}

static void userMsgGithubLink(struct triceBot *b, Event_UserMessage event) {
    pthread_mutex_lock(&b->mutex);

    char msg[BUFFER_LENGTH];
    snprintf(msg, BUFFER_LENGTH,
             "You can find the sourcecode for tricebot at: %s",
             GITHUB_REPO);

    Command_Message cmdMsg;
    cmdMsg.set_user_name(event.sender_name());
    cmdMsg.set_message(msg);

    CommandContainer cont;
    SessionCommand *c = cont.add_session_command();
    c->MutableExtension(Command_Message::ext)->CopyFrom(cmdMsg);

    struct pendingCommand *cmd = prepCmdNTS(b, cont, -1, -1);
    enq(cmd, &b->sendQueue);

    pthread_mutex_unlock(&b->mutex);
}

// WARNING: the commands are hard coded and the COMMAND_COUNT needs to be set by hand.
#define COMMAND_COUNT 3
#define COMMAND_DISCORD {"discord", "Shows the squirebot invite link (PMs only).",\
NULL, NULL, &userMsgDiscordLink}
#define COMMAND_GITHUB {"github", "Shows the link to the github repo (PMs only).",\
NULL, NULL, &userMsgGithubLink}
#define COMMAND_HELP {"help", "Shows this help message."}

struct bot_command {
    char *commandName,
    *commandDescription;
    void (*fnRoomSay) (struct triceBot *, Event_RoomSay);
    void (*fnGameSay) (struct triceBot *, struct game, Event_GameSay, int);
    void (*fnUserMsg) (struct triceBot *, Event_UserMessage);
};

struct bot_command_list {
    char commandPrefix;
    char *helpMessage;
    struct bot_command *commands;
    size_t commandsLength;
};

static struct bot_command commands[COMMAND_COUNT];
static struct bot_command_list list = {'!',
    NULL,
    commands,
    COMMAND_COUNT
};

static void execGameSayCommand(struct triceBot *b, struct game g, Event_GameSay event, int gameid) {
    const char *msg = event.message().c_str();
    if (msg[0] == list.commandPrefix) {
        for (size_t i = 0; i < list.commandsLength; i++) {
            if (strncmp(list.commands[i].commandName, 1 + msg, event.message().size() - 1) == 0) {
                if (list.commands[i].fnGameSay)
                    list.commands[i].fnGameSay(b, g, event, gameid);
                break;
            }
        }
    }
}

static void execRoomSayCommand(struct triceBot *b, Event_RoomSay event) {
    const char *msg = event.message().c_str();
    if (msg[0] == list.commandPrefix) {
        for (size_t i = 0; i < list.commandsLength; i++) {
            if (strncmp(list.commands[i].commandName, 1 + msg, event.message().size() - 1) == 0) {
                if (list.commands[i].fnRoomSay)
                    list.commands[i].fnRoomSay(b, event);
                break;
            }
        }
    }
}

static void execUserMsgCommand(struct triceBot *b, Event_UserMessage event) {
    const char *msg = event.message().c_str();
    if (msg[0] == list.commandPrefix) {
        for (size_t i = 0; i < list.commandsLength; i++) {
            if (strncmp(list.commands[i].commandName, 1 + msg, event.message().size() - 1) == 0) {
                if (list.commands[i].fnUserMsg)
                    list.commands[i].fnUserMsg(b, event);
                break;
            }
        }
    }
}

#define TMP_BUFFER_LENGTH 1024
void initCommandList(struct triceBot* b) {
    // Init commands
    commands[0] = COMMAND_DISCORD;
    commands[1] = COMMAND_GITHUB;
    commands[2] = COMMAND_HELP;
    
    // Init help message
    char *helpMessage = (char *) malloc(sizeof(char) * BUFFER_LENGTH);
    helpMessage[BUFFER_LENGTH - 1] = 0;
    snprintf(helpMessage, BUFFER_LENGTH, "Help for %s:\n", PROG_NAME);

    // Add commands to help message
    size_t len = strnlen(helpMessage, BUFFER_LENGTH) + 1;
    for (size_t i = 0; i < list.commandsLength; i++) {
        struct bot_command cmd = list.commands[i];
        char tmpBuffer[TMP_BUFFER_LENGTH];
        snprintf(tmpBuffer,
                 TMP_BUFFER_LENGTH,
                 "\t- %s : %s\n",
                 cmd.commandName,
                 cmd.commandDescription);

        len += strnlen(tmpBuffer, TMP_BUFFER_LENGTH);
        strncat(helpMessage, tmpBuffer, BUFFER_LENGTH - len);
    }

    set_onGameEventGameSay(&execGameSayCommand, b);
    set_onEventRoomSay(&execRoomSayCommand, b);
    set_onEventUserMessage(&execUserMsgCommand, b);
}

void freeCommandList() {
    if (list.helpMessage != NULL) {
        free(list.helpMessage);
    }
}
