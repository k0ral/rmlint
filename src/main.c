/**
*  This file is part of rmlint.
*
*  rmlint is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation, either version 3 of the License, or
*  (at your option) any later version.
*
*  rmlint is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with rmlint.  If not, see <http://www.gnu.org/licenses/>.
*
** Authors:
 *
 *  - Christopher <sahib> Pahl 2010-2014 (https://github.com/sahib)
 *  - Daniel <SeeSpotRun> T.   2014-2014 (https://github.com/SeeSpotRun)
 *
** Hosted on http://github.com/sahib/rmlint
*
**/

#include <stdlib.h>
#include <string.h>
#include <locale.h>

#include "cmdline.h"

static char *remove_color_escapes(char *message) {
    char *dst = message;
    for(char *src = message; src && *src; src++) {
        if(*src == '\x1b') {
            src = strchr(src, 'm');
        } else {
            *dst++ = *src;
        }
    }

    if(dst) *dst = 0;
    return message;
}

static void logging_callback(
    G_GNUC_UNUSED const gchar *log_domain,
    GLogLevelFlags log_level,
    const gchar *message,
    gpointer user_data) {

    RmSession *session = user_data;
    if(session->settings->verbosity >= log_level) {
        if(!session->settings->color) {
            message = remove_color_escapes((char *)message);
        }
        g_printerr("%s", message);
    }
}

static volatile int CTRLC_COUNTER = 0;
static volatile RmSession *SESSION_POINTER = NULL;

static void signal_handler(int signum) {
    switch(signum) {
    case SIGINT:
        if(CTRLC_COUNTER++ == 0) {
            SESSION_POINTER->aborted = TRUE;
            warning(GRE"\nINFO: "NCO"Received Interrupt, stopping...\n");
        } else {
            warning(GRE"\nINFO: "NCO"Received second Interrupt, stopping hard.\n");
            die((RmSession *)SESSION_POINTER, EXIT_FAILURE);
            exit(EXIT_FAILURE);
        }
        break;
    case SIGFPE:
    case SIGABRT:
    case SIGSEGV:
        rm_error(RED"FATAL: "NCO"Aborting due to a fatal error. (signal received: %s)\n", g_strsignal(signum));
    default:
        rm_error(RED"FATAL: "NCO"Please file a bug report (See rmlint -h)\n");
        exit(EXIT_FAILURE);
        break;
    }
}

int main(int argc, char **argv) {
    int exit_state = EXIT_FAILURE;

    RmSettings settings;
    rm_set_default_settings(&settings);

    RmSession session;
    rm_session_init(&session, &settings);

    /* call logging_callback on every message */
    g_log_set_default_handler(logging_callback, &session);

    /* Make printing umlauts work */
    setlocale(LC_ALL, "");

    /* Register signals */
    SESSION_POINTER = &session;

    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = signal_handler;

    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGFPE,  &sa, NULL);
    sigaction(SIGABRT, &sa, NULL);

    /* Parse commandline */
    if(rm_parse_arguments(argc, argv, &session) != 0) {
        /* Check settings */
        if (rm_echo_settings(session.settings)) {
            /* Do all the real work */
            exit_state = rm_main(&session);
        } else {
            rm_error(RED"Aborting.\n"NCO);
        }
    }

    return exit_state;
}
