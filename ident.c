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

#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <unistd.h>
#include "ident.h"
#include "misc.h"

#define IDENT_PORT      113

bool identLookup(int sock, time_t timeout, char* user)
{
    struct sockaddr_any peerAddr;
    socklen_t peerLen = sizeof(peerAddr);
    if (getpeername(sock, (struct sockaddr*) &peerAddr, &peerLen) < 0) {
        return false;
    }

    struct sockaddr_any localAddr;
    socklen_t localLen = sizeof(localAddr);
    if (getsockname(sock, (struct sockaddr*) &localAddr, &localLen) < 0) {
        return false;
    }

    struct sockaddr_any addr;
    memcpy(&addr, &peerAddr, sizeof(addr));
    switch (addr.san_family) {
        case AF_INET :
            addr.s4.sin_port = htons(IDENT_PORT);
            break;
        case AF_INET6 :
            addr.s6.sin6_port = htons(IDENT_PORT);
            break;
        default :
            return false;
    }

    int identSock = socket(addr.san_family, SOCK_STREAM, 0);
    if (identSock < 0) { return false; }

    setReadTimeout(identSock, timeout);
    setWriteTimeout(identSock, timeout);

    if (connect(identSock, &addr.sa, sockaddrLen(&addr)) < 0) {
      close(identSock);
      return false;
    }

    FILE* fp = fdopen(identSock, "r+");
    if (!fp) {
        close(identSock);
        return false;
    }

    int localPort = portFromSockaddr(&localAddr);
    int remotePort = portFromSockaddr(&peerAddr);

    if (fprintf(fp, "%i,%i\r\n", remotePort, localPort) < 0) {
        fclose(fp);
        return false;
    }

    int replyLocalPort;
    int replyRemotePort;
    int ret = fscanf(fp, "%i, %i : USERID :%*[^:]:%255s\r\n", &replyRemotePort,
                     &replyLocalPort, user);
    fclose(fp);
    return ret == 3 && replyLocalPort == localPort && replyRemotePort == remotePort;
}
