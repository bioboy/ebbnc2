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
#include <string.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/select.h>
#include "misc.h"
#include "server.h"
#include "client.h"

Server* Server_new()
{
    Server* server = calloc(1, sizeof(Server));
    if (!server) { return NULL; }

    server->sock = -1;

    return server;
}

void Server_free(Server** serverp)
{
    if (*serverp) {
        Server* server = *serverp;
        if (server->sock >= 0) { close(server->sock); }
        free(server);
        *serverp = NULL;
    }
}

void Server_freeList(Server** serverp)
{
    if (*serverp) {
        Server* server = *serverp;
        while (server->next) {
            Server* temp = server->next->next;
            Server_free(&server->next);
            server->next = temp;
        }
        Server_free(&server);
        *serverp = NULL;
    }
}

bool Server_listen2(Server* server, const char* ip, int port)
{
    if (!ipPortToSockaddr(ip, port, &server->addr)) {
        fprintf(stderr, "Invalid listenip.\n");
        return false;
    }

    server->sock = socket(server->addr.san_family, SOCK_STREAM, 0);
    if (server->sock < 0) {
        perror("socket");
        return false;
    }

    {
        int optval = 1;
        setsockopt(server->sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    }

    if (bind(server->sock, &server->addr.sa, sockaddrLen(&server->addr)) < 0) {
        perror("bind");
        return false;
    }

    if (listen(server->sock, SOMAXCONN) < 0) {
        perror("listen");
        return false;
    }

    return true;
}

Server* Server_listen(Config* config, Bouncer* bouncer)
{
    printf("Bouncing from %s:%li to %s:%li!\n", bouncer->listenIP, bouncer->listenPort,
                                                bouncer->remoteHost, bouncer->remotePort);

    Server* server = Server_new();
    if (!server) {
        perror("Server_new");
        return NULL;
    }

    server->config = config;
    server->bouncer = bouncer;
    if (!Server_listen2(server, bouncer->listenIP, bouncer->listenPort)) {
        Server_free(&server);
        return NULL;
    }

    return server;
}

Server* Server_listenAll(Config* config)
{
    Server* servers = NULL;
    Bouncer* bouncer = config->bouncers;
    while (bouncer) {
        Server* server = Server_listen(config, bouncer);
        if (!server) {
            Server_freeList(&servers);
            return NULL;
        }

        server->next = servers;
        servers = server;

        bouncer = bouncer->next;
    }
    return servers;
}

void Server_accept(Server* servers)
{
    int max = 0;
    fd_set set;
    FD_ZERO(&set);
    Server* server = servers;
    while (server) {
        FD_SET(server->sock, &set);
        max = server->sock > max ? server->sock : max;
        server = server->next;
    }

    int n = select(max + 1, &set, NULL, NULL, NULL);
    if (n < 0) {
        perror("select");
        usleep(10000); // prevent busy looping
        return;
    }

    server = servers;
    while (server) {
        if (FD_ISSET(server->sock, &set)) {
            struct sockaddr_any addr;
            socklen_t len = sizeof(addr);
            int sock = accept(server->sock, &addr.sa, &len);
            if (sock < 0) {
                perror("accept");
                return;
            }

            Client_launch(server, sock, &addr);
        }
        server = server->next;
    }
}

void Server_loop(Server* servers)
{
    while (true) {
      Server_accept(servers);
    }
}
