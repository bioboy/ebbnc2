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

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <poll.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <errno.h>
#include "client.h"
#include "ident.h"
#include "misc.h"

Client* Client_new()
{
    Client* client = calloc(1, sizeof(Client));
    if (!client) { return NULL; }

    client->cSock = -1;
    client->rSock = -1;

    return client;
}

void Client_free(Client** clientp)
{
    if (*clientp) {
        Client* client = *clientp;
        if (client->cSock >= 0) { close(client->cSock); }
        if (client->rSock >= 0) { close(client->rSock); }
        free(client);
        *clientp = NULL;
    }
}

void Client_errorReply(Client* client, const char* msg)
{
    char* buf = strPrintf("421 %s\r\n", msg);
    if (!buf) {
        perror("strPrintf");
        return;
    }

    IGNORE_RESULT(write(client->cSock, buf, strlen(buf)));
    free(buf);
}

void Client_errnoReply(Client* client, const char* funclient, int errno_)
{
    char errnoMsg[256];
    if (strerror_r(errno_, errnoMsg, sizeof(errnoMsg)) < 0) {
        strncpy(errnoMsg, "Unknown error", sizeof(errnoMsg));
    }

    char* msg = strPrintf("%s: %s", funclient, errnoMsg);
    if (!msg) {
        perror("sprintf");
        return;
    }

    Client_errorReply(client, msg);
    free(msg);
}

bool Client_sendIdnt(Client* client)
{
    if (!client->config->idnt) { return true; }

    char user[IDENT_LEN];

    {
        struct sockaddr_any localAddr;
        socklen_t slen = sizeof(localAddr);
        if (getsockname(client->cSock, &localAddr.sa, &slen) < 0) {
            perror("getsockname");
            return false;
        }

        if (!identLookup(&client->server->addr, &client->cAddr, client->config->identTimeout, user)) {
            strcpy(user, "*");
        }
    }

    char ip[INET6_ADDRSTRLEN];
    if (!ipFromSockaddr(&client->cAddr, ip)) { return false; }

    char hostname[NI_MAXHOST];
    if (!client->config->dnsLookup ||
        getnameinfo(&client->cAddr.sa, sizeof(client->cAddr),
                    hostname, sizeof(hostname), NULL, 0, 0) != 0) {

        strncpy(hostname, ip, sizeof(ip));
    }

    char* buf = strPrintf("IDNT %s@%s:%s\n", user, ip, hostname);
    if (!buf) {
        perror("strPrintf");
        return false;
    }

    ssize_t len = strlen(buf);
    bool ret = write(client->rSock, buf, len) == len;
    free(buf);
    return ret;
}

bool Client_connect(Client* client)
{
    const char* errmsg = NULL;
    if (!hostPortToSockaddr(client->bouncer->remoteHost, client->bouncer->remotePort, &client->rAddr, &errmsg)) {
        if (!errmsg) {
          Client_errnoReply(client, "hostPortToSockaddr", errno);
          return false;
        }

        char* msg = strPrintf("hostPortToSockaddr: %s", errmsg);
        if (!msg) {
            perror("sprintf");
            return false;
        }

        Client_errorReply(client, msg);
        free(msg);
        return false;
    }

    client->rSock = socket(client->rAddr.san_family, SOCK_STREAM, 0);
    if (client->rSock < 0) {
        Client_errnoReply(client, "socket", errno);
        return false;
    }

    if (client->config->writeTimeout > 0) {
      setWriteTimeout(client->rSock, client->config->writeTimeout);
    }

    {
        int optval = 1;
        setsockopt(client->rSock, IPPROTO_TCP, TCP_NODELAY, (char*)&optval, sizeof(optval));
    }

    if (client->bouncer->localIP) {
        struct sockaddr_any lAddr;
        if (!ipPortToSockaddr(client->bouncer->localIP, INADDR_ANY, &lAddr)) {
            Client_errorReply(client, "invalid localip");
            return false;
        }

        if (bind(client->rSock, &lAddr.sa, sockaddrLen(&lAddr)) < 0) {
            Client_errnoReply(client, "bind", errno);
            return false;
        }
    }

    if (connect(client->rSock, &client->rAddr.sa, sockaddrLen(&client->rAddr)) < 0) {
        Client_errnoReply(client, "connect", errno);
        return false;
    }

    return true;
}

void Client_relay(Client* client)
{
    int timeout = client->config->idleTimeout == 0 ? -1 : client->config->idleTimeout * 1000;
    char buf[BUFSIZ];
    struct pollfd fds[2];

    fds[0].fd = client->cSock;
    fds[0].events = POLLIN;
    fds[0].revents = 0;

    fds[1].fd = client->rSock;
    fds[1].events = POLLIN;
    fds[1].revents = 0;

    while (true) {
        int ret = poll(fds, 2, timeout);
        if (ret == 0) {
            Client_errorReply(client, "Idle timeout");
            break;
        }

        if (ret < 0) {
            Client_errnoReply(client, "poll", errno);
            break;
        }

        if (fds[0].revents & POLLIN) {
            ssize_t len = read(client->cSock, buf, sizeof(buf));
            if (len <= 0) { break; }

            ssize_t ret = write(client->rSock, buf, len);
            if (ret < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    Client_errorReply(client, "Server write timeout");
                }
                else {
                    Client_errnoReply(client, "write", errno);
                }
                break;
            }

            if (ret != len) {
                Client_errorReply(client, "Short write");
                break;
            }
        }

        if (fds[1].revents & POLLIN) {
            ssize_t len = read(client->rSock, buf, sizeof(buf));
            if (len == 0) {
                Client_errorReply(client, "Connection closed");
                break;
            }

            if (len < 0) {
                Client_errnoReply(client, "read", errno);
                break;
            }

            ssize_t ret = write(client->cSock, buf, len);
            if (ret < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    Client_errorReply(client, "Client write timeout");
                }
                else {
                    Client_errnoReply(client, "write", errno);
                }
            }
        }
    }
}

bool Client_welcome(Client* client)
{
    if (!client->config->welcomeMsg) { return true; }

    char* buf = strPrintf("220-%s\r\n", client->config->welcomeMsg);
    if (!buf) {
        perror("strPrintf");
        return false;
    }

    ssize_t len = strlen(buf);
    ssize_t ret = write(client->cSock, buf, len);
    free(buf);

    return ret == len;
}

void* Client_threadMain(void* clientv)
{
    Client* client = clientv;

    if (client->config->writeTimeout > 0) {
      setWriteTimeout(client->cSock, client->config->writeTimeout);
    }

    if (Client_connect(client) &&
        Client_sendIdnt(client) &&
        Client_welcome(client)) {

        Client_relay(client);
    }

    Client_free(&client);
    return NULL;
}

void Client_launch(Server* server, int sock, const struct sockaddr_any* addr)
{
    Client* client = Client_new();
    if (!client) {
        perror("Client_new");
        return;
    }

    client->config = server->config;
    client->bouncer = server->bouncer;
    client->cSock = sock;
    client->server = server;
    memcpy(&client->cAddr, addr, sizeof(client->cAddr));

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, CLIENT_STACKSIZE);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&client->threadId, &attr, Client_threadMain, client);
}
