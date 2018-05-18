/* Functions related to the Configuration */
#include <stdlib.h>
#include <string.h>

#include <gdk/gdk.h>
#include <glib.h>

#include "config.h"
#include "utils.h"


static GdkRGBA *parse_greeter_color_key(GKeyFile *keyfile, const char *key_name, const char *fallback);
static guint parse_greeter_hotkey_keyval(GKeyFile *keyfile, const char *key_name, const char *fallback);

/* Initialize the configuration, sourcing the greeter's configuration file */
Config *initialize_config(void)
{
    GError *err = NULL;

    Config *config = malloc(sizeof(Config));
    if (config == NULL) {
        g_error("Could not allocate memory for Config");
    }

    // Load the key-value file
    GKeyFile *keyfile = g_key_file_new();
    gboolean keyfile_loaded = g_key_file_load_from_file(
        keyfile, CONFIG_FILE, G_KEY_FILE_NONE, NULL);
    if (!keyfile_loaded) {
        g_error("Could not load configuration file.");
    }

    // Parse values from the keyfile into a Config.
    config->login_user = g_key_file_get_string(
        keyfile, "greeter", "user", &err);
    if (err != NULL) {
        config->login_user = (gchar *) "CHANGE_ME";
        g_clear_error(&err);
    }
    if (strcmp(config->login_user, "CHANGE_ME") == 0) {
        g_message("User configuration value is unchanged.");
    }
    config->show_password_label = g_key_file_get_boolean(
        keyfile, "greeter", "show-password-label", &err);
    if (err != NULL) {
        config->show_password_label = TRUE;
        g_clear_error(&err);
    }
    config->password_label_text = g_key_file_get_string(
        keyfile, "greeter", "password-label-text", &err);
    if (err != NULL) {
        config->password_label_text = (gchar *) "Password:";
        g_clear_error(&err);
    }
    config->show_input_cursor = g_key_file_get_boolean(
        keyfile, "greeter", "show-input-cursor", &err);
    if (err != NULL) {
        config->show_input_cursor = TRUE;
        g_clear_error(&err);
    }

    // Parse Hotkey Settings
    config->suspend_key = parse_greeter_hotkey_keyval(keyfile, "suspend-key", "u");
    config->hibernate_key = parse_greeter_hotkey_keyval(keyfile, "hibernate-key", "h");
    config->restart_key = parse_greeter_hotkey_keyval(keyfile, "restart-key", "r");
    config->shutdown_key = parse_greeter_hotkey_keyval(keyfile, "shutdown-key", "s");
    gchar *mod_key = g_key_file_get_string(
        keyfile, "greeter-hotkeys", "mod-key", &err);
    if (err != NULL) {
        mod_key = (gchar *) "meta";
        g_clear_error(&err);
    }
    if (strcmp(mod_key, "control") == 0) {
        config->mod_bit = GDK_CONTROL_MASK;
    } else if (strcmp(mod_key, "alt") == 0) {
        config->mod_bit = GDK_MOD1_MASK;
    } else if (strcmp(mod_key, "meta") == 0) {
        config->mod_bit = GDK_SUPER_MASK;
    } else {
        g_error("Invalid mod-key configuration value: '%s'\n", mod_key);
    }

    // Parse Theme Settings
    config->font = g_key_file_get_string(
        keyfile, "greeter-theme", "font", &err);
    if (err != NULL) {
        config->font = (gchar *) "Sans";
        g_clear_error(&err);
    }
    config->font_size = g_key_file_get_string(
        keyfile, "greeter-theme", "font-size", &err);
    if (err != NULL) {
        config->font_size = (gchar *) "1em";
        g_clear_error(&err);
    }
    config->text_color =
        parse_greeter_color_key(keyfile, "text-color", "#080800");
    config->error_color =
        parse_greeter_color_key(keyfile, "error-color", "#F8F8F0");
    config->background_image = g_key_file_get_string(
        keyfile, "greeter-theme", "background-image", &err);
    if (err != NULL || strcmp(config->background_image, "") == 0) {
        config->background_image = (gchar *) "\"\"";
        g_clear_error(&err);
    }
    config->background_color =
        parse_greeter_color_key(keyfile, "background-color", "#1B1D1E");
    config->window_color =
        parse_greeter_color_key(keyfile, "window-color", "#F92672");
    config->border_color =
        parse_greeter_color_key(keyfile, "border-color", "#080800");
    config->password_color =
        parse_greeter_color_key(keyfile, "password-color", "#F8F8F0");
    config->password_background_color =
        parse_greeter_color_key(keyfile, "password-background-color", "#1B1D1E");
    config->border_width = g_key_file_get_string(
        keyfile, "greeter-theme", "border-width", &err);
    if (err != NULL) {
        config->border_width = (gchar *) "2px";
        g_clear_error(&err);
    }

    gint layout_spacing = g_key_file_get_integer(
        keyfile, "greeter-theme", "layout-space", &err);
    if (err != NULL) {
        config->layout_spacing = (guint) 15;
        g_clear_error(&err);
    }
    if (layout_spacing < 0) {
        config->layout_spacing = (guint) (-1 * layout_spacing);
    } else {
        config->layout_spacing = (guint) layout_spacing;
    }


    g_key_file_free(keyfile);

    return config;
}


/* Cleanup any memory allocated for the Config */
void destroy_config(Config *config)
{
    free(config->login_user);
    free(config->font);
    free(config->font_size);
    free(config->text_color);
    free(config->error_color);
    free(config->background_image);
    free(config->background_color);
    free(config->window_color);
    free(config->border_color);
    free(config->border_width);
    free(config->password_label_text);
    free(config->password_color);
    free(config->password_background_color);
    free(config);
}


/* Parse a greeter-colors group key into a newly-allocated GdkRGBA value */
static GdkRGBA *parse_greeter_color_key(GKeyFile *keyfile, const char *key_name, const char *fallback)
{
    GError *err = NULL;
    gchar *color_string = g_key_file_get_string(
        keyfile, "greeter-theme", key_name, &err);
    if (err != NULL) {
        color_string = (gchar *) fallback;
        g_clear_error(&err);
    }
    if (strstr(color_string, "#") != NULL) {
        // Remove quotations from hex color strings
        remove_char(color_string, '"');
        remove_char(color_string, '\'');
    }

    GdkRGBA *color = malloc(sizeof(GdkRGBA));

    gboolean color_was_parsed = gdk_rgba_parse(color, color_string);
    if (!color_was_parsed) {
        g_critical("Could not parse the '%s' setting: %s",
                   key_name, color_string);
    }

    return color;
}

/* Parse a greeter-hotkeys key into the GDKkeyval of it's first character */
static guint parse_greeter_hotkey_keyval(GKeyFile *keyfile, const char *key_name, const char *fallback)
{
    GError *err = NULL;
    gchar *key = g_key_file_get_string(
        keyfile, "greeter-hotkeys", key_name, &err);
    if (err != NULL) {
        key = (gchar *) fallback;
        g_clear_error(&err);
    }

    if (strcmp(key, "") == 0) {
        g_error("Configuration contains empty key for '%s'\n", key_name);
    }

    return gdk_unicode_to_keyval((guint) key[0]);
}
