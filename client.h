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

#ifndef __EBBNC_CLIENT_H
#define __EBBNC_CLIENT_H

#include <arpa/inet.h>
#include <pthread.h>
#include "config.h"

struct Client
{
  pthread_t threadId;
  int cSock;
  struct sockaddr_storage cAddr;
  int rSock;
  struct sockaddr_storage rAddr;
  struct Config* cfg;
};

void Client_Launch(struct Config* cfg, int sock, const struct sockaddr_storage* addr);

#endif
