#include "lru_cache.h"
#include <string.h>
#include <stdlib.h>

#define CACHE_SIZE 100

typedef struct {
    char *email;
    char *pubkey;
} CacheEntry;

static CacheEntry entries[CACHE_SIZE];
static int size = 0;

void cache_init() {
    for (int i = 0; i < CACHE_SIZE; i++) {
        entries[i].email = NULL;
        entries[i].pubkey = NULL;
    }
    size = 0;
}

void cache_add(const char *email, const char *pubkey) {
    if (size >= CACHE_SIZE) {
        free(entries[size - 1].email);
        free(entries[size - 1].pubkey);
        size--;
    }
    for (int i = size; i > 0; i--) {
        entries[i] = entries[i - 1];
    }
    entries[0].email = strdup(email);
    entries[0].pubkey = strdup(pubkey);
    size++;
}

char *cache_get(const char *email) {
    for (int i = 0; i < size; i++) {
        if (strcmp(entries[i].email, email) == 0) {
            CacheEntry temp = entries[i];
            for (int j = i; j > 0; j--) {
                entries[j] = entries[j - 1];
            }
            entries[0] = temp;
            return entries[0].pubkey;
        }
    }
    return NULL;
}