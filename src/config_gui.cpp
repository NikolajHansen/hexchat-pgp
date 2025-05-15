#include "config_gui.h"
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <hexchat-plugin.h>

// External configuration variables defined in hexchat-gpg.cpp
extern char *config_email;
extern char *config_pubkey;
extern char *config_privkey_path;

static void config_apply(GtkWidget *widget, gpointer data) {
    GtkWidget *email_entry = (GtkWidget *)g_object_get_data(G_OBJECT(widget), "email_entry");
    GtkWidget *pubkey_entry = (GtkWidget *)g_object_get_data(G_OBJECT(widget), "pubkey_entry");
    GtkWidget *privkey_entry = (GtkWidget *)g_object_get_data(G_OBJECT(widget), "privkey_entry");

    free(config_email);
    free(config_pubkey);
    free(config_privkey_path);
    config_email = strdup(gtk_entry_get_text(GTK_ENTRY(email_entry)));
    config_pubkey = strdup(gtk_entry_get_text(GTK_ENTRY(pubkey_entry)));
    config_privkey_path = strdup(gtk_entry_get_text(GTK_ENTRY(privkey_entry)));

    // Save to file
    char path[256];
    snprintf(path, sizeof(path), "%s/.config/hexchat", getenv("HOME"));
    mkdir(path, 0755);
    snprintf(path, sizeof(path), "%s/.config/hexchat/hexchat-gpg.conf", getenv("HOME"));
    FILE *f = fopen(path, "w");
    if (f) {
        if (config_email) fprintf(f, "email=%s\n", config_email);
        if (config_pubkey) fprintf(f, "pubkey=%s\n", config_pubkey);
        if (config_privkey_path) fprintf(f, "privkey_path=%s\n", config_privkey_path);
        fclose(f);
    }

    gtk_widget_destroy(gtk_widget_get_toplevel(widget));
}

void config_gui_show(hexchat_plugin *ph) {
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "HexChat GPG Config");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 200);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    GtkWidget *email_entry = gtk_entry_new();
    if (config_email) gtk_entry_set_text(GTK_ENTRY(email_entry), config_email);
    gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new("Email:"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), email_entry, FALSE, FALSE, 0);

    GtkWidget *pubkey_entry = gtk_entry_new();
    if (config_pubkey) gtk_entry_set_text(GTK_ENTRY(pubkey_entry), config_pubkey);
    gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new("Public Key:"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), pubkey_entry, FALSE, FALSE, 0);

    GtkWidget *privkey_entry = gtk_entry_new();
    if (config_privkey_path) gtk_entry_set_text(GTK_ENTRY(privkey_entry), config_privkey_path);
    gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new("Private Key Path:"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), privkey_entry, FALSE, FALSE, 0);

    GtkWidget *apply_button = gtk_button_new_with_label("Apply");
    g_object_set_data(G_OBJECT(apply_button), "email_entry", email_entry);
    g_object_set_data(G_OBJECT(apply_button), "pubkey_entry", pubkey_entry);
    g_object_set_data(G_OBJECT(apply_button), "privkey_entry", privkey_entry);
    g_signal_connect(apply_button, "clicked", G_CALLBACK(config_apply), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), apply_button, FALSE, FALSE, 0);

    gtk_widget_show_all(window);
}