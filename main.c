//
//  Copyright (C) 2013 ebftpd team
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <libgen.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include "config.h"
#include "server.h"
#include "misc.h"
#include "conf.h"
#include "info.h"

bool InitialiseSignals()
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_flags = 0;
    sa.sa_handler = SIG_IGN;

    if (sigaction(SIGPIPE, &sa, NULL) < 0) {
        perror("sigaction");
        return false;
    }

    return true;
}

int main(int argc, char** argv)
{
#ifndef CONF_EMBEDDED
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <config file>\n", argv[0]);
        return 1;
    }
#else
    if (argc != 1) {
        fprintf(stderr, "Usage: %s\n", argv[0]);
        fprintf(stderr, "Built with an embedded conf.\n");
        return 1;
    }

    char* key = getpass("Password: ");
    if (*key == '\0') {
        fprintf(stderr, "Password is required to load embedded conf.\n");
        return 1;
    }
#endif

    hline();
    printf("%s %s by %s\n", EBBNC_PROGRAM, EBBNC_VERSION, EBBNC_AUTHOR);
    hline();
    atexit(hline);

    printf("Initialising signals ..\n");
    InitialiseSignals();

    printf("Loading config file ..\n");

#ifndef CONF_EMBEDDED
    Config* cfg = Config_loadFile(argv[1]);
#else
    Config* cfg = Config_loadEmbedded(key);
#endif
    if (!cfg) { return 1; }

    if (cfg->pidFile) {
        printf("Checking if bouncer already running ..\n");
        int ret = isAlreadyRunning(cfg->pidFile);
        if (ret < 0) {
            fprintf(stderr, "Unable to check if already running: %s\n", strerror(errno));
            Config_free(&cfg);
            return 1;
        }

        if (ret != 0) {
            fprintf(stderr, "Bouncer already running!\n");
            Config_free(&cfg);
            return 1;
        }
    }

    printf("Initialising listening socket ..\n");
    Server* srv = Server_listen(cfg);
    if (!srv) {
        Server_free(&srv);
        Config_free(&cfg);
        return 1;
    }

    printf("Forking into background ..\n");
    pid_t pid = daemonise();
    if (pid < 0) {
        fprintf(stderr, "Failed to fork into background.\n");
        Server_free(&srv);
        Config_free(&cfg);
        return 1;
    }

    if (pid > 0) {
        printf("Bouncer running as PID #%i\n", pid);
        if (cfg->pidFile) {
            printf("Creating PID file at %s ..\n", cfg->pidFile);
            if (!createPIDFile(cfg->pidFile, pid)) {
                // this goes to /dev/null at the moment
                // this error is not fatal
                fprintf(stderr, "Failed to create PID file. This is only a warning.\n");
            }
        }

        hline();
        _exit(0);
    }

    printf("Waiting for connections ..\n");
    Server_loop(srv);

    Server_free(&srv);
    Config_free(&cfg);

    return 0;

    (void) argc;
}
