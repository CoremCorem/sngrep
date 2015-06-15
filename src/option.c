/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013,2014 Ivan Alonso (Kaian)
 ** Copyright (C) 2013,2014 Irontec SL. All rights reserved.
 **
 ** This program is free software: you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation, either version 3 of the License, or
 ** (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **
 ** You should have received a copy of the GNU General Public License
 ** along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **
 ****************************************************************************/
/**
 * @file option.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source code of functions defined in option.h
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "keybinding.h"
#include "option.h"
#include "setting.h"

/**
 * @brief Configuration options array
 *
 * Contains all availabe options that can be optionured
 * @todo make this dynamic
 */
option_opt_t options[1024];
int optscnt = 0;

int
init_options()
{
    // Custom user conf file
    char userconf[128];
    char *home = getenv("HOME");

    // Initialize settings
    setting_set_value(SETTING_SAVEPATH, home);
    setting_set_value(SETTING_FILTER_METHODS, "REGISTER,INVITE,SUBSCRIBE,NOTIFY,OPTIONS,PUBLISH,MESSAGE");

    // Add Call list column options
    set_option_value("cl.column0", "sipfrom");
    set_option_value("cl.column1", "sipto");
    set_option_value("cl.column2", "msgcnt");
    set_option_value("cl.column3", "src");
    set_option_value("cl.column4", "dst");
    set_option_value("cl.column5", "method");
    set_option_value("cl.column6", "state");

    // Initialize keybindings
    key_bindings_init();

    // Read options from configuration files
    read_options("/etc/sngreprc");
    read_options("/usr/local/etc/sngreprc");
    // Get user homedir configuration
    if (home) {
        sprintf(userconf, "%s/.sngreprc", home);
        read_options(userconf);
    }

    return 0;
}

void
deinit_options()
{
    int i;
    // Deallocate options memory
    for (i = 0; i < optscnt; i++) {
        free(options[i].opt);
        free(options[i].value);
    }
}

int
read_options(const char *fname)
{
    FILE *fh;
    char line[1024], type[20], option[50], value[50];
    int id;

    if (!(fh = fopen(fname, "rt")))
        return -1;

    while (fgets(line, 1024, fh) != NULL) {
        // Check if this line is a commentary or empty line
        if (!strlen(line) || *line == '#')
            continue;

        // Get configuration option from setting line
        if (sscanf(line, "%s %s %s", type, option, value) == 3) {
            if (!strcasecmp(type, "set")) {
                if ((id = setting_id(option)) >= 0) {
                    setting_set_value(id, value);
                } else {
                    set_option_value(option, value);
                }
            } else if (!strcasecmp(type, "ignore")) {
                set_ignore_value(option, value);
            } else if (!strcasecmp(type, "alias")) {
                set_alias_value(option, value);
            } else if (!strcasecmp(type, "bind")) {
                key_bind_action(key_action_id(option), key_from_str(value));
            } else if (!strcasecmp(type, "unbind")) {
                key_unbind_action(key_action_id(option), key_from_str(value));
            }
        }
    }
    fclose(fh);
    return 0;
}

const char*
get_option_value(const char *opt)
{
    int i;
    for (i = 0; i < optscnt; i++) {
        if (!strcasecmp(opt, options[i].opt)) {
            return options[i].value;
        }
    }
    return NULL;
}

int
get_option_int_value(const char *opt)
{
    const char *value;
    if ((value = get_option_value(opt))) {
        return atoi(value);
    }
    return -1;
}

void
set_option_value(const char *opt, const char *value)
{
    if (!opt || !value)
        return;

    int i;
    if (!get_option_value(opt)) {
        options[optscnt].type = COLUMN;
        options[optscnt].opt = strdup(opt);
        options[optscnt].value = strdup(value);
        optscnt++;
    } else {
        for (i = 0; i < optscnt; i++) {
            if (!strcasecmp(opt, options[i].opt)) {
                free(options[i].value);
                options[i].value = strdup(value);
            }
        }
    }
}

void
set_ignore_value(const char *opt, const char *value)
{
    options[optscnt].type = IGNORE;
    options[optscnt].opt = strdup(opt);
    options[optscnt].value = strdup(value);
    optscnt++;
}

int
is_ignored_value(const char *field, const char *fvalue)
{
    int i;
    for (i = 0; i < optscnt; i++) {
        if (!strcasecmp(options[i].opt, field) && !strcasecmp(options[i].value, fvalue)) {
            return 1;
        }
    }
    return 0;
}

void
set_alias_value(const char *address, const char *alias)
{
    options[optscnt].type = ALIAS;
    options[optscnt].opt = strdup(address);
    options[optscnt].value = strdup(alias);
    optscnt++;
}

const char *
get_alias_value(const char *address)
{
    int i;

    if (!address)
        return NULL;

    for (i = 0; i < optscnt; i++) {
        if (options[i].type != ALIAS)
            continue;
        if (!strcmp(options[i].opt, address))
            return options[i].value;
    }

    return address;
}

