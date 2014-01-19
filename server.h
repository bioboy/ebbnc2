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

#ifndef EBBNC_SERVER_H
#define EBBNC_SERVER_H

#include <sys/socket.h>
#include "config.h"
#include "misc.h"

typedef struct Server {
    int                 sock;
    struct sockaddr_any addr;
    Config*             config;
    Bouncer*            bouncer;
    struct Server*      next;
} Server;

Server* Server_listenAll(Config* config);
void Server_free(Server** serverp);
void Server_freeList(Server** serverp);
void Server_loop(Server* servers);

#endif
