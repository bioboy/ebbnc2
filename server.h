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

#include <arpa/inet.h>
#include "config.h"

struct Server {
    int sock;
    struct sockaddr_storage addr;
    struct Config* cfg;
};

struct Server* Server_Listen(struct Config* cfg);
void Server_Free(struct Server** sp);
void Server_Loop(struct Server* s);

#endif
