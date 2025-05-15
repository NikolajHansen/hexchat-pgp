#ifndef RECIPIENT_LIST_H
#define RECIPIENT_LIST_H

#include <hexchat-plugin.h>

typedef struct Recipient {
    char *nick;
    char *email;
    struct Recipient *next;
} Recipient;

void recipient_add(const char *nick, const char *email);
void recipient_remove(const char *nick);
void recipient_list(hexchat_plugin *ph);
Recipient *recipient_get_list();

#endif