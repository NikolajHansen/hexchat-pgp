#include <hexchat-plugin.h>
#include <gpgme.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <locale.h>
#include "lru_cache.h"
#include "recipient_list.h"
#include "config_gui.h"
#include "directory_client.h"

// Global plugin handle for callbacks that cannot pass ph
static hexchat_plugin *ph_global = NULL;

// Configuration
char *config_email = NULL;
char *config_pubkey = NULL;
char *config_privkey_path = NULL;
const char *config_file = "~/.config/hexchat/hexchat-gpg.conf";
static bool encryption_enabled = false;
static gpgme_ctx_t gpg_context;

// Load configuration
void config_load() {
    char path[256];
    snprintf(path, sizeof(path), "%s/.config/hexchat/hexchat-gpg.conf", getenv("HOME"));
    FILE *f = fopen(path, "r");
    if (!f) return;
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        char *key = strtok(line, "=");
        char *value = strtok(NULL, "\n");
        if (key && value) {
            if (strcmp(key, "email") == 0) {
                free(config_email);
                config_email = strdup(value);
            } else if (strcmp(key, "pubkey") == 0) {
                free(config_pubkey);
                config_pubkey = strdup(value);
            } else if (strcmp(key, "privkey_path") == 0) {
                free(config_privkey_path);
                config_privkey_path = strdup(value);
            }
        }
    }
    fclose(f);
}

// Save configuration
void config_save() {
    char path[256];
    snprintf(path, sizeof(path), "%s/.config/hexchat", getenv("HOME"));
    mkdir(path, 0755);
    snprintf(path, sizeof(path), "%s/.config/hexchat/hexchat-gpg.conf", getenv("HOME"));
    FILE *f = fopen(path, "w");
    if (!f) return;
    if (config_email) fprintf(f, "email=%s\n", config_email);
    if (config_pubkey) fprintf(f, "pubkey=%s\n", config_pubkey);
    if (config_privkey_path) fprintf(f, "privkey_path=%s\n", config_privkey_path);
    fclose(f);
}

// GPG Operations
void gpg_init() {
    gpgme_check_version(NULL);
    gpgme_set_locale(NULL, LC_CTYPE, setlocale(LC_CTYPE, NULL));
    gpgme_error_t err = gpgme_new(&gpg_context);
    if (err) {
        fprintf(stderr, "GPGME context creation failed: %s\n", gpgme_strerror(err));
        exit(1);
    }
    gpgme_set_armor(gpg_context, 1); // Use ASCII armor for output
}

char *gpg_encrypt(const char *text, const char *email, hexchat_plugin *ph) {
    if (!encryption_enabled) return NULL;

    char *pubkey = cache_get(email);
    if (!pubkey) {
        pubkey = directory_lookup_key(email, gpg_context);
        if (pubkey) cache_add(email, pubkey);
    }
    if (!pubkey) {
        hexchat_printf(ph, "No public key for %s\n", email);
        return NULL;
    }

    // Import public key
    gpgme_data_t key_data;
    gpgme_error_t err = gpgme_data_new_from_mem(&key_data, pubkey, strlen(pubkey), 0);
    if (err) {
        hexchat_printf(ph, "Failed to load public key: %s\n", gpgme_strerror(err));
        return NULL;
    }
    err = gpgme_op_import(gpg_context, key_data);
    gpgme_data_release(key_data);
    if (err) {
        hexchat_printf(ph, "Failed to import public key: %s\n", gpgme_strerror(err));
        return NULL;
    }

    // Prepare plaintext
    gpgme_data_t plain, cipher;
    gpgme_data_new_from_mem(&plain, text, strlen(text), 0);
    gpgme_data_new(&cipher);

    // Set recipient
    gpgme_key_t recipient_key[2] = { NULL, NULL };
    err = gpgme_op_keylist_start(gpg_context, email, 0);
    if (!err) {
        err = gpgme_op_keylist_next(gpg_context, &recipient_key[0]);
        gpgme_op_keylist_end(gpg_context);
    }
    if (err || !recipient_key[0]) {
        hexchat_printf(ph, "No key found for %s\n", email);
        gpgme_data_release(plain);
        gpgme_data_release(cipher);
        return NULL;
    }

    // Encrypt
    err = gpgme_op_encrypt(gpg_context, recipient_key, GPGME_ENCRYPT_ALWAYS_TRUST, plain, cipher);
    gpgme_key_release(recipient_key[0]);
    gpgme_data_release(plain);
    if (err) {
        hexchat_printf(ph, "Encryption failed: %s\n", gpgme_strerror(err));
        gpgme_data_release(cipher);
        return NULL;
    }

    // Read encrypted output
    size_t len;
    char *encrypted = gpgme_data_release_and_get_mem(cipher, &len);
    if (!encrypted) {
        hexchat_printf(ph, "Failed to read encrypted data\n");
        return NULL;
    }
    encrypted = (char *)realloc(encrypted, len + 1);
    encrypted[len] = '\0';
    char *result = (char *)malloc(len + 20);
    snprintf(result, len + 20, "HEXCHAT_GPG:%s", encrypted);
    gpgme_free(encrypted);
    return result;
}

char *gpg_decrypt(const char *text, hexchat_plugin *ph) {
    if (!encryption_enabled || !config_privkey_path) {
        hexchat_printf(ph, "Private key path not configured or encryption disabled\n");
        return NULL;
    }

    // Import private key
    FILE *key_file = fopen(config_privkey_path, "r");
    if (!key_file) {
        hexchat_printf(ph, "Failed to open private key file\n");
        return NULL;
    }
    char key_buf[4096];
    size_t key_len = fread(key_buf, 1, sizeof(key_buf) - 1, key_file);
    fclose(key_file);
    key_buf[key_len] = '\0';

    gpgme_data_t key_data;
    gpgme_error_t err = gpgme_data_new_from_mem(&key_data, key_buf, key_len, 0);
    if (err) {
        hexchat_printf(ph, "Failed to load private key: %s\n", gpgme_strerror(err));
        return NULL;
    }
    err = gpgme_op_import(gpg_context, key_data);
    gpgme_data_release(key_data);
    if (err) {
        hexchat_printf(ph, "Failed to import private key: %s\n", gpgme_strerror(err));
        return NULL;
    }

    // Prepare ciphertext (skip "HEXCHAT_GPG:" prefix)
    const char *ciphertext = text + 11;
    gpgme_data_t cipher, plain;
    gpgme_data_new_from_mem(&cipher, ciphertext, strlen(ciphertext), 0);
    gpgme_data_new(&plain);

    // Decrypt
    err = gpgme_op_decrypt(gpg_context, cipher, plain);
    gpgme_data_release(cipher);
    if (err) {
        hexchat_printf(ph, "Decryption failed: %s\n", gpgme_strerror(err));
        gpgme_data_release(plain);
        return NULL;
    }

    // Read plaintext
    size_t len;
    char *decrypted = gpgme_data_release_and_get_mem(plain, &len);
    if (!decrypted) {
        hexchat_printf(ph, "Failed to read decrypted data\n");
        return NULL;
    }
    decrypted = (char *)realloc(decrypted, len + 1);
    decrypted[len] = '\0';
    return decrypted;
}

// HexChat Command Handler
static int command_gpg(char *word[], char *word_eol[], void *userdata) {
    if (!ph_global) {
        fprintf(stderr, "Plugin handle not initialized\n");
        return HEXCHAT_EAT_ALL;
    }
    if (strcmp(word[1], "config") == 0) {
        config_gui_show(ph_global);
    } else if (strcmp(word[1], "addrecipient") == 0 && word[2] && word[3]) {
        recipient_add(word[2], word[3]);
        hexchat_printf(ph_global, "Added recipient: %s (%s)\n", word[2], word[3]);
    } else if (strcmp(word[1], "deluser") == 0 && word[2]) {
        recipient_remove(word[2]);
        hexchat_printf(ph_global, "Removed recipient: %s\n", word[2]);
    } else if (strcmp(word[1], "listusers") == 0) {
        recipient_list(ph_global);
    } else if (strcmp(word[1], "on") == 0) {
        encryption_enabled = true;
        hexchat_printf(ph_global, "GPG encryption enabled\n");
    } else if (strcmp(word[1], "off") == 0) {
        encryption_enabled = false;
        hexchat_printf(ph_global, "GPG encryption disabled\n");
    } else {
        hexchat_printf(ph_global, "Usage: /GPG <config|addrecipient <nick> <email>|deluser <nick>|listusers|on|off>\n");
    }
    return HEXCHAT_EAT_ALL;
}

// Message Handler
static int message_handler(char *word[], void *userdata) {
    if (!ph_global) {
        fprintf(stderr, "Plugin handle not initialized\n");
        return HEXCHAT_EAT_NONE;
    }
    if (!encryption_enabled) return HEXCHAT_EAT_NONE;
    const char *nick = word[1];
    const char *message = word[2];
    if (strncmp(message, "HEXCHAT_GPG", 11) == 0) {
        char *decrypted = gpg_decrypt(message, ph_global);
        if (decrypted) {
            hexchat_printf(ph_global, "[%s] %s\n", nick, decrypted);
            free(decrypted);
            return HEXCHAT_EAT_ALL;
        }
    }
    return HEXCHAT_EAT_NONE;
}

// Channel Message Handler
static int channel_message_handler(char *word[], void *userdata) {
    if (!ph_global) {
        fprintf(stderr, "Plugin handle not initialized\n");
        return HEXCHAT_EAT_NONE;
    }
    if (!encryption_enabled) return HEXCHAT_EAT_NONE;
    const char *message = word[2];
    Recipient *current = recipient_get_list();
    while (current) {
        char *encrypted = gpg_encrypt(message, current->email, ph_global);
        if (encrypted) {
            hexchat_commandf(ph_global, "PRIVMSG %s :%s", current->nick, encrypted);
            free(encrypted);
        }
        current = current->next;
    }
    return HEXCHAT_EAT_ALL; // Prevent sending unencrypted message
}

// Plugin Interface
extern "C" void hexchat_plugin_init(hexchat_plugin *ph) {
    ph_global = ph; // Store plugin handle
    cache_init();
    config_load();
    gpg_init();
    hexchat_hook_command(ph, "GPG", HEXCHAT_PRI_NORM, command_gpg, "GPG commands", NULL);
    hexchat_hook_print(ph, "Channel Message", HEXCHAT_PRI_NORM, channel_message_handler, NULL);
    hexchat_hook_print(ph, "Private Message", HEXCHAT_PRI_NORM, message_handler, NULL);
    hexchat_printf(ph, "hexchat-gpg loaded\n");
}

extern "C" void hexchat_plugin_deinit(hexchat_plugin *ph) {
    if (gpg_context) gpgme_release(gpg_context);
    free(config_email);
    free(config_pubkey);
    free(config_privkey_path);
}

extern "C" int hexchat_plugin_get_info(const char **name, const char **desc, const char **version, void **reserved) {
    *name = "hexchat-gpg";
    *desc = "GPG encryption for HexChat";
    *version = "0.1";
    return 1;
}