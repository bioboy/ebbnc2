//
//  Copyright (C) 2012, 2013 ebftpd team
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
#include "config.h"
#include "server.h"
#include "misc.h"

const char* program = "ebBNC";
const char* version = "0.1b";

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
  
  if (sigaction(SIGINT, &sa, NULL) < 0) {
    perror("sigaction");
    return false;
  }
  
  return true;
}

void Hline()
{
  printf("---------------------------------------- --- -> >\n");
}

int main(int argc, char** argv)
{
  if (argc != 2) {
    fprintf(stderr, "usage: %s <config file>\n", argv[0]);
    return 1;
  }

  Hline();
  printf("%s %s by ebftpd team\n", program, version);
  Hline();
  atexit(Hline);
  
  printf("Loading config file ..\n");
  struct Config* cfg = Config_Load(argv[1]);
  if (!cfg) return 1;
  
  printf("Initialising signals ..\n");
  InitialiseSignals();
  
  if (cfg->pidFile)
  {
    printf("Checking if bouncer already running ..\n");
    int ret = AlreadyRunning(cfg->pidFile);
    if (ret < 0) {
      perror("AlreadyRunning");
      Config_Free(&cfg);
      return 1;
    }
    
    if (ret != 0) {
      fprintf(stderr, "Bouncer already running.\n");
      Config_Free(&cfg);
      return 1;
    }
  }
  
  printf("Initialising listening socket ..\n");
  struct Server* srv = Server_Listen(cfg);
  if (!srv) {
    Server_Free(&srv);
    Config_Free(&cfg);
    return 1;
  }

  printf("Forking into background ..\n");
  pid_t pid = Daemonise();
  if (pid < 0) {
    fprintf(stderr, "Failed to fork into background.\n");
    Server_Free(&srv);
    Config_Free(&cfg);
    return 1;
  }
  
  if (pid > 0) {
    printf("Bouncer running as PID #%i\n", pid);
    Hline();
    _exit(0);
  }
  
  if (cfg->pidFile) {
    printf("Creating PID file at %s ..\n", cfg->pidFile);
    if (!CreatePIDFile(cfg->pidFile)) {
      // this goes to /dev/null at the moment
      // this error is not fatal
      fprintf(stderr, "Failed to create PID file. This is only a warning.\n"); 
    }
  }

  printf("Waiting for connections ..\n");
  Server_Loop(srv);
  
  Server_Free(&srv);
  Config_Free(&cfg);
  
  return 0;
  
  (void) argc;
}
