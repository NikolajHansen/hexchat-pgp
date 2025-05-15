#ifndef DIRECTORY_CLIENT_H
#define DIRECTORY_CLIENT_H

#include <gpgme.h>

char *directory_lookup_key(const char *email, gpgme_ctx_t ctx);

#endif