#include "recipient_list.h"
#include <stdlib.h>
#include <string.h>

static Recipient *recipients = NULL;

void recipient_add(const char *nick, const char *email) {
    Recipient *r = (Recipient *)malloc(sizeof(Recipient));
    r->nick = strdup(nick);
    r->email = strdup(email);
    r->next = recipients;
    recipients = r;
}

void recipient_remove(const char *nick) {
    Recipient *current = recipients, *prev = NULL;
    while (current) {
        if (strcmp(current->nick, nick) == 0) {
            if (prev) prev->next = current->next;
            else recipients = current->next;
            free(current->nick);
            free(current->email);
            free(current);
            return;
        }
        prev = current;
        current = current->next;
    }
}

void recipient_list(hexchat_plugin *ph) {
    Recipient *current = recipients;
    while (current) {
        hexchat_printf(ph, "Nick: %s, Email: %s\n", current->nick, current->email);
        current = current->next;
    }
}

Recipient *recipient_get_list() {
    return recipients;
}