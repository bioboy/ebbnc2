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

Client* Client_New()
{
    Client* c = calloc(1, sizeof(Client));
    if (!c) { return NULL; }

    c->cSock = -1;
    c->rSock = -1;

    return c;
}

void Client_Free(Client** cp)
{
    if (*cp) {
        Client* c = *cp;
        if (c->cSock >= 0) { close(c->cSock); }
        if (c->rSock >= 0) { close(c->rSock); }
        free(c);
        *cp = NULL;
    }
}

void Client_ErrorReply(Client* c, const char* msg)
{
    char* buf = Sprintf("421 %s\r\n", msg);
    if (!buf) {
        perror("Sprintf");
        return;
    }

    IGNORE_RESULT(write(c->cSock, buf, strlen(buf)));
    free(buf);
}

void Client_ErrnoReply(Client* c, const char* func, int errno_)
{
    char errnoMsg[256];
    if (strerror_r(errno_, errnoMsg, sizeof(errnoMsg)) < 0) {
        strncpy(errnoMsg, "Unknown error", sizeof(errnoMsg));
    }

    char* msg = Sprintf("%s: %s", func, errnoMsg);
    if (!msg) {
        perror("sprintf");
        return;
    }

    Client_ErrorReply(c, msg);
    free(msg);
}

bool Client_Idnt(Client* c)
{
    if (!c->cfg->idnt) { return true; }

    char user[IDENT_LEN];

    {
        struct sockaddr_any localAddr;
        socklen_t slen = sizeof(localAddr);
        if (getsockname(c->cSock, &localAddr.sa, &slen) < 0) {
            perror("getsockname");
            return false;
        }

        if (!IdentLookup(&c->srv->addr, &c->cAddr, c->cfg->identTimeout, user)) {
            strcpy(user, "*");
        }
    }

    char ip[INET6_ADDRSTRLEN];
    if (!IPFromSockaddr(&c->cAddr, ip)) { return false; }

    char hostname[NI_MAXHOST];
    if (!c->cfg->dnsLookup ||
        getnameinfo(&c->cAddr.sa, sizeof(c->cAddr),
                    hostname, sizeof(hostname), NULL, 0, 0) != 0) {

        strncpy(hostname, ip, sizeof(ip));
    }

    char* buf = Sprintf("IDNT %s@%s:%s\n", user, ip, hostname);
    if (!buf) {
        perror("Sprintf");
        return false;
    }

    ssize_t len = strlen(buf);
    bool ret = write(c->rSock, buf, len) == len;
    free(buf);
    return ret;
}

bool Client_Connect(Client* c)
{
    const char* errmsg = NULL;
    if (!HostPortToSockaddr(c->cfg->remoteHost, c->cfg->remotePort, &c->rAddr, &errmsg)) {
        if (!errmsg) {
          Client_ErrnoReply(c, "HostPortToSockaddr", errno);
          return false;
        }

        char* msg = Sprintf("HostPortToSockaddr: %s", errmsg);
        if (!msg) {
            perror("sprintf");
            return false;
        }

        Client_ErrorReply(c, msg);
        free(msg);
        return false;
    }

    c->rSock = socket(c->rAddr.san_family, SOCK_STREAM, 0);
    if (c->rSock < 0) {
        Client_ErrnoReply(c, "socket", errno);
        return false;
    }

    if (c->cfg->writeTimeout > 0) { SetWriteTimeout(c->rSock, c->cfg->writeTimeout); }

    {
        int optval = 1;
        setsockopt(c->rSock, IPPROTO_TCP, TCP_NODELAY, (char*)&optval, sizeof(optval));
    }

    if (connect(c->rSock, &c->rAddr.sa, SockaddrLen(&c->rAddr)) < 0) {
        Client_ErrnoReply(c, "connect", errno);
        return false;
    }

    return true;
}

void Client_Relay(Client* c)
{
    int timeout = c->cfg->idleTimeout == 0 ? -1 : c->cfg->idleTimeout * 1000;
    char buf[BUFSIZ];
    struct pollfd fds[2];

    fds[0].fd = c->cSock;
    fds[0].events = POLLIN;
    fds[0].revents = 0;

    fds[1].fd = c->rSock;
    fds[1].events = POLLIN;
    fds[1].revents = 0;

    while (true) {
        int ret = poll(fds, 2, timeout);
        if (ret == 0) {
            Client_ErrorReply(c, "Idle timeout");
            break;
        }

        if (ret < 0) {
            Client_ErrnoReply(c, "poll", errno);
            break;
        }

        if (fds[0].revents & POLLIN) {
            ssize_t len = read(c->cSock, buf, sizeof(buf));
            if (len <= 0) { break; }

            ssize_t ret = write(c->rSock, buf, len);
            if (ret < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    Client_ErrorReply(c, "Server write timeout");
                }
                else {
                    Client_ErrnoReply(c, "write", errno);
                }
                break;
            }

            if (ret != len) {
                Client_ErrorReply(c, "Short write");
                break;
            }
        }

        if (fds[1].revents & POLLIN) {
            ssize_t len = read(c->rSock, buf, sizeof(buf));
            if (len == 0) {
                Client_ErrorReply(c, "Connection closed");
                break;
            }

            if (len < 0) {
                Client_ErrnoReply(c, "read", errno);
                break;
            }

            ssize_t ret = write(c->cSock, buf, len);
            if (ret < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    Client_ErrorReply(c, "Client write timeout");
                }
                else {
                    Client_ErrnoReply(c, "write", errno);
                }
            }
        }
    }
}

bool Client_Welcome(Client* c)
{
    if (!c->cfg->welcomeMsg) { return true; }

    char* buf = Sprintf("220-%s\r\n", c->cfg->welcomeMsg);
    if (!buf) {
        perror("Sprintf");
        return false;
    }

    ssize_t len = strlen(buf);
    ssize_t ret = write(c->cSock, buf, len);
    free(buf);

    return ret == len;
}

void* Client_ThreadMain(void* cv)
{
    Client* c = (Client*) cv;

    if (c->cfg->writeTimeout > 0) { SetWriteTimeout(c->cSock, c->cfg->writeTimeout); }

    if (Client_Connect(c) &&
        Client_Idnt(c) &&
        Client_Welcome(c)) {

        Client_Relay(c);
    }

    Client_Free(&c);
    return NULL;
}

void Client_Launch(Server* srv, int sock,
                   const struct sockaddr_any* addr)
{
    Client* c = Client_New();
    if (!c) {
        perror("Client_New");
        return;
    }

    c->cfg = srv->cfg;
    c->cSock = sock;
    c->srv = srv;
    memcpy(&c->cAddr, addr, sizeof(c->cAddr));

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, CLIENT_STACKSIZE);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&c->threadId, &attr, Client_ThreadMain, c);
}
