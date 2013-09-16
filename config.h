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

typedef struct {
    char*   listenIP;
    int     listenPort;
    char*   remoteHost;
    int     remotePort;
    bool    idnt;
    int     identTimeout;
    int     idleTimeout;
    int     writeTimeout;
    bool    dnsLookup;
    char*   pidFile;
    char*   welcomeMsg;
} Config;

Config* Config_new();
Config* Config_loadBuffer(const char* buffer);
Config* Config_loadFile(const char* path);
Config* Config_loadEmbedded(const char* key);
char* Config_saveBuffer(Config* c);
void Config_free(Config** cp);

#endif
