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

#ifndef __EBBNC_CONFIG_H
#define __EBBNC_CONFIG_H

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

struct Config* Config_Load(const char* path);
void Config_Free(struct Config** cp);

#endif
