#include "directory_client.h"
#include <gpgme.h>
#include <stdlib.h>
#include <string.h>

char *directory_lookup_key(const char *email, gpgme_ctx_t ctx) {
    // Set keyserver
    gpgme_error_t err = gpgme_set_keylist_mode(ctx, GPGME_KEYLIST_MODE_EXTERN);
    if (err) {
        fprintf(stderr, "Failed to set keylist mode: %s\n", gpgme_strerror(err));
        return NULL;
    }
    gpgme_set_protocol(ctx, GPGME_PROTOCOL_OpenPGP);

    // Start key listing
    err = gpgme_op_keylist_start(ctx, email, 0);
    if (err) {
        fprintf(stderr, "Failed to start keylist: %s\n", gpgme_strerror(err));
        return NULL;
    }

    // Retrieve key
    gpgme_key_t key;
    err = gpgme_op_keylist_next(ctx, &key);
    gpgme_op_keylist_end(ctx);
    if (err || !key) {
        fprintf(stderr, "No key found for %s: %s\n", email, gpgme_strerror(err));
        return NULL;
    }

    // Export key
    gpgme_data_t key_data;
    err = gpgme_data_new(&key_data);
    if (err) {
        gpgme_key_release(key);
        fprintf(stderr, "Failed to create data buffer: %s\n", gpgme_strerror(err));
        return NULL;
    }
    err = gpgme_op_export(ctx, email, 0, key_data);
    gpgme_key_release(key);
    if (err) {
        gpgme_data_release(key_data);
        fprintf(stderr, "Failed to export key: %s\n", gpgme_strerror(err));
        return NULL;
    }

    // Read key data
    size_t len;
    char *pubkey = gpgme_data_release_and_get_mem(key_data, &len);
    if (!pubkey) {
        fprintf(stderr, "Failed to read key data\n");
        return NULL;
    }
    pubkey = (char *)realloc(pubkey, len + 1);
    pubkey[len] = '\0';
    return pubkey;
}