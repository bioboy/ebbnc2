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

#ifndef EBBNC_CONFIG_H
#define EBBNC_CONFIG_H

#include <stdbool.h>

struct Config
{
  char* listenIP;
  int listenPort;
  char* remoteIP;
  int remotePort;
  bool idnt;
  int identTimeout;
  int idleTimeout;
  int writeTimeout;
  bool dnsLookup;
  char* pidFile;
  char* welcomeMsg;
};

struct Config* Config_New();
struct Config* Config_LoadBuffer(const char* buffer);
struct Config* Config_LoadFile(const char* path);
char* Config_SaveBuffer(struct Config* c);
void Config_Free(struct Config** cp);

#endif
