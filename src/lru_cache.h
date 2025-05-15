#ifndef LRU_CACHE_H
#define LRU_CACHE_H

void cache_init();
void cache_add(const char *email, const char *pubkey);
char *cache_get(const char *email);

#endif