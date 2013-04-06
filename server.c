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
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include "misc.h"
#include "server.h"
#include "client.h"

struct Server* Server_New()
{
  struct Server* s = (struct Server*) malloc(sizeof(struct Server));
  if (!s) return NULL;
  
  s->sock = -1;
  memset(&s->addr, 0, sizeof(s->addr));
  s->cfg = NULL;
  
  return s;
}

void Server_Free(struct Server** sp)
{
  if (*sp) {
    struct Server* s = *sp;
    if (s->sock >= 0) close(s->sock);
    free(s);
    *sp = NULL;
  }
}

bool Server_Listen2(struct Server* s, const char* ip, int port)
{
  if (!IPPortToSockaddr(ip, port, &s->addr)) {
    fprintf(stderr, "Invalid listenip.\n");
    return false;
  }
  
  s->sock = socket(s->addr.ss_family, SOCK_STREAM, 0);
  if (s->sock < 0) {
    perror("socket");
    return false;
  }
  
  {
    int optval = 1;
    setsockopt(s->sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));    
  }
  
  if (bind(s->sock, (struct sockaddr*)&s->addr, sizeof(s->addr)) < 0) {
    perror("bind");
    return false;
  }
  
  if (listen(s->sock, SOMAXCONN) < 0) {
    perror("listen");
    return false;
  }
    
  return true;
}

struct Server* Server_Listen(struct Config* cfg)
{
  struct Server* s = Server_New();
  if (!s) {
    perror("Server_New");
    return NULL;
  }
  
  s->cfg = cfg;
  if (!Server_Listen2(s, cfg->listenIP, cfg->listenPort)) {
    Server_Free(&s);
    return NULL;
  }
  
  return s;
}

void Server_Accept(struct Server* s)
{
  struct sockaddr_storage addr;
  socklen_t len = sizeof(addr);
  int sock = accept(s->sock, (struct sockaddr*)&addr, &len);
  if (sock < 0) {
    perror("accept");
    return;
  }
  
  Client_Launch(s, sock, &addr);
}

void Server_Loop(struct Server* s)
{
  while (true) {
    Server_Accept(s);
  }
}
