#define PLUGIN_EXPORT __attribute__((visibility("default")))

#include <stdlib.h>
#include <string.h>
#include <hexchat-plugin.h>
#include <gpgme.h>
#include <locale.h>

#define PNAME "HexChat-PGP"
#define PDESC "PGP Encryption/Decryption Plugin for HexChat"
#define PVERSION "1.0"

static hexchat_plugin *ph;

// Function prototypes
PLUGIN_EXPORT int hexchat_plugin_init(hexchat_plugin *plugin_handle, char **plugin_name, char **plugin_desc, char **plugin_version, char *arg);
PLUGIN_EXPORT int hexchat_plugin_deinit(void);

static int on_message_in(char *word[], void *userdata);
static int on_message_out(char *word[], char *word_eol[], void *userdata);

// GPGME Context for operations
static gpgme_ctx_t ctx;

// Function to initialize GPGME
static int gpgme_init() {
    gpgme_check_version(NULL);
    gpgme_set_locale(NULL, LC_CTYPE, setlocale(LC_CTYPE, NULL));
    return gpgme_new(&ctx);
}

// Function to lookup public key
static char* lookup_public_key(const char *email) {
    gpgme_error_t err;
    gpgme_key_t key;
    
    err = gpgme_op_keylist_start(ctx, email, 0);
    if (!err) {
        err = gpgme_op_keylist_next(ctx, &key);
        if (!err) {
            char *key_id = strdup(key->subkeys->keyid);
            gpgme_key_unref(key);
            return key_id;
        }
        gpgme_op_keylist_end(ctx);
    }
    return NULL;
}

// Function to encrypt message
static char* encrypt_message(const char *message, const char *recipient_email) {
    gpgme_error_t err;
    gpgme_data_t plain, cipher;
    char *encrypted = NULL;

    gpgme_data_new_from_mem(&plain, message, strlen(message), 0);
    gpgme_data_new(&cipher);
    
    char *key_id = lookup_public_key(recipient_email);
    if (key_id) {
        gpgme_encrypt_result_t result;
        gpgme_key_t key;
        err = gpgme_get_key(ctx, key_id, &key, 0);
        if (!err) {
            gpgme_key_t keys[2] = {key, NULL};
            err = gpgme_op_encrypt(ctx, keys, GPGME_ENCRYPT_ALWAYS_TRUST, plain, cipher);
            if (!err) {
                size_t size;
                result = gpgme_op_encrypt_result(ctx);
                gpgme_data_seek(cipher, 0, SEEK_SET);
                gpgme_data_read(cipher, NULL, 0); 
                size = gpgme_data_seek(cipher, 0, SEEK_END);
                gpgme_data_seek(cipher, 0, SEEK_SET);
                encrypted = malloc(size + 1);
                if (encrypted) {
                    gpgme_data_read(cipher, encrypted, size); 
                    encrypted[size] = '\0';
                }
            }
            gpgme_key_unref(key);
        }
        free(key_id);
    }

    gpgme_data_release(plain);
    gpgme_data_release(cipher);
    return encrypted;
}

// Function to decrypt message
static char* decrypt_message(const char *encrypted_message) {
    gpgme_data_t plain, cipher;
    gpgme_error_t err;
    char *decrypted = NULL;

    gpgme_data_new_from_mem(&cipher, encrypted_message, strlen(encrypted_message), 0);
    gpgme_data_new(&plain);

    err = gpgme_op_decrypt(ctx, cipher, plain);
    if (!err) {
        size_t size;
        gpgme_data_seek(plain, 0, SEEK_SET);
        gpgme_data_read(plain, NULL, 0); 
        size = gpgme_data_seek(plain, 0, SEEK_END);
        gpgme_data_seek(plain, 0, SEEK_SET);
        decrypted = malloc(size + 1);
        if (decrypted) {
            gpgme_data_read(plain, decrypted, size); 
            decrypted[size] = '\0';
        }
    }

    gpgme_data_release(plain);
    gpgme_data_release(cipher);
    return decrypted;
}

// Hook for incoming messages
static int on_message_in(char *word[], void *userdata) {
    if (strncmp(word[1], "-----BEGIN PGP MESSAGE-----", 28) == 0) {
        char *decrypted = decrypt_message(word[1]);
        if (decrypted) {
            hexchat_emit_print(ph, "Channel Message", word[0], decrypted, NULL);
            free(decrypted);
            return HEXCHAT_EAT_ALL;
        }
    }
    return HEXCHAT_EAT_NONE;
}

// Hook for outgoing messages
static int on_message_out(char *word[], char *word_eol[], void *userdata) {
    // Check if the message starts with "_pgp"
    if (strncmp(word_eol[1], "_pgp", 4) == 0) {
        // Get the recipient's email. Here we're using a placeholder; 
        // in a real scenario, you'd need to determine this from HexChat's user list or context.
        const char *recipient_email = "barnabasdk@gmail.com"; // TODO: Implement actual email lookup
        
        // Remove "_pgp" prefix from the message before encryption
        char *message_to_encrypt = word_eol[1] + 4; // Skip the first 4 characters

        char *encrypted = encrypt_message(message_to_encrypt, recipient_email);
        if (encrypted) {
            // Send the encrypted message
            hexchat_commandf(ph, "PRIVMSG %s :%s", word[1], encrypted);
            free(encrypted);
            return HEXCHAT_EAT_ALL; // Prevent sending the unencrypted message
        } else {
            hexchat_printf(ph, "%s: Encryption failed, sending unencrypted.\n", PNAME);
            // Fall through to send the message unencrypted if encryption fails
        }
    }
    return HEXCHAT_EAT_NONE; // If the message doesn't start with "_pgp" or if encryption fails, send as-is
}

PLUGIN_EXPORT int hexchat_plugin_init(hexchat_plugin *plugin_handle, char **plugin_name, char **plugin_desc, char **plugin_version, char *arg) {
    ph = plugin_handle;
    *plugin_name = PNAME;
    *plugin_desc = PDESC;
    *plugin_version = PVERSION;

    if (gpgme_init() != GPG_ERR_NO_ERROR) {
        hexchat_printf(ph, "%s: Failed to initialize GPGME\n", PNAME);
        return 0;
    }

    hexchat_hook_print(ph, "Channel Message", HEXCHAT_PRI_NORM, on_message_in, NULL);
    hexchat_hook_command(ph, "PRIVMSG", HEXCHAT_PRI_NORM, on_message_out, NULL, NULL);

    hexchat_printf(ph, "%s plugin loaded\n", PNAME);
    return 1;
}

PLUGIN_EXPORT int hexchat_plugin_deinit(void) {
    gpgme_release(ctx);
    hexchat_printf(ph, "%s plugin unloaded\n", PNAME);
    return 1;
}
