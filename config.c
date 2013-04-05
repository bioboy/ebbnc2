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
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <errno.h>
#include "config.h"
#include "misc.h"

struct Config* Config_New()
{
  struct Config* c = (struct Config*) malloc(sizeof(struct Config));
  if (!c) return NULL;
  
  c->listenIP = NULL;
  c->listenPort = -1;
  c->remoteIP = NULL;
  c->remotePort = -1;
  c->idnt = true;
  c->identTimeout = 10;
  c->relayTimeout = 30;
  c->dnsLookup = true;
  c->pidFile = NULL;
  
  return c;
}

void Config_Free(struct Config** cp)
{
  if (*cp) {
    struct Config* c = *cp;
    free(c->listenIP);
    free(c->remoteIP);
    free(c->pidFile);
    free(c->welcomeMsg);
    free(c);
    *cp = NULL;
  }
}

bool Config_SanityCheck(struct Config* c)
{
  bool insane = false;
  if (c->listenPort == -1) {
    fprintf(stderr, "Config option is required: listenport\n");
    insane = true;
  }
  
  if (!c->remoteIP) {
    fprintf(stderr, "Config option is required: remoteip\n");
    insane = true;
  }
  
  if (c->remotePort == -1) {
    fprintf(stderr, "Config option is required: remoteport\n");
    insane = true;
  }
  
  return !insane;
}

struct Config* Config_Load(const char* path)
{  
  FILE* fp = fopen(path, "r");
  if (!fp) {
    fprintf(stderr, "Unable to open config file: %s\n", strerror(errno));
    return NULL;
  }
  
  struct Config* c = Config_New();
  if (!c) {
    fprintf(stderr, "Unable to load config: %s\n", strerror(errno));
    fclose(fp);
    return NULL;
  }
  
  bool error = false;
  int lineNo = 0;
  char buf[BUFSIZ];
  while (!error && fgets(buf, sizeof(buf), fp)) {
    ++lineNo;
    StripCRLF(buf);
    if (*buf == '\0' || *buf == '#') continue;

    size_t len = strlen(buf);
    if (!strncasecmp(buf, "listenip=", 9) && len > 9) {
      c->listenIP = strdup(buf + 9);
      if (!c->listenIP) goto strduperror;
    }
    else if (!strncasecmp(buf, "listenport=", 11) && len > 11) {
      if (sscanf(buf + 11, "%i", &c->listenPort) != 1 || c->listenPort < 0) error = true;
    }
    else if (!strncasecmp(buf, "remoteip=", 9) && len > 9) {
      c->remoteIP = strdup(buf + 9);
      if (!c->remoteIP) goto strduperror;
    }
    else if (!strncasecmp(buf, "remoteport=", 11) && len > 11) {
      if (sscanf(buf + 11, "%i", &c->remotePort) != 1 || c->remotePort < 0) error = true;
    }
    else if (!strncasecmp(buf, "idnt=", 5)) {
      char* value = buf + 5;
      if (!strcasecmp(value, "true")) c->idnt = true;
      else if (!strcasecmp(value, "false")) c->idnt = false;
      else error = true;
    }
    else if (!strncasecmp(buf, "identtimeout=", 13) && len > 13) {
      if (sscanf(buf + 13, "%i", &c->identTimeout) != 1 || c->identTimeout < 0) {
        error = true;
      }
    }
    else if (!strncasecmp(buf, "relaytimeout=", 13) && len > 13) {
      if (sscanf(buf + 13, "%i", &c->relayTimeout) != 1 || c->relayTimeout < 0) {
        error = true;
      }
    }
    else if (!strncasecmp(buf, "dnslookup=", 10) && len > 10) {
      char* value = buf + 10;
      if (!strcasecmp(value, "true")) c->dnsLookup = true;
      else if (!strcasecmp(value, "false")) c->dnsLookup = false;
      else error = true;      
    }
    else if (!strncasecmp(buf, "pidfile=", 8) && len > 8) {
      c->pidFile = strdup(buf + 8);
      if (!c->pidFile) goto strduperror;
    }
    else if (!strncasecmp(buf, "welcomemsg=", 11) && len > 11) {
      c->welcomeMsg = strdup(buf + 11);
      if (!c->welcomeMsg) goto strduperror;
    }
    else {
      error = true;
    }
  }
  
  fclose(fp);

  if (error) {
    fprintf(stderr, "Error at line %i in config file.\n", lineNo);
    Config_Free(&c);
    return NULL;
  }
  
  if (!c->listenIP) {
    c->listenIP = strdup("::");
    if (!c->listenIP) goto strduperror;
  }
  
  if (!Config_SanityCheck(c)) {
    Config_Free(&c);
    return NULL;
  }
  
  return c;
  
strduperror:
  perror("strdup");
  Config_Free(&c);
  return NULL;
}
