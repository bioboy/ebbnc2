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
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include "ident.h"
#include "misc.h"

#define IDENT_PORT      113

bool IdentLookup(const struct sockaddr_storage* localAddr,
                 const struct sockaddr_storage* peerAddr,
                 time_t timeout, char* user)
{
    struct sockaddr_storage addr;
    memcpy(&addr, peerAddr, sizeof(addr));
    switch (addr.ss_family) {
        case AF_INET :
            ((struct sockaddr_in*)&addr)->sin_port = htons(IDENT_PORT);
            break;
        case AF_INET6 :
            ((struct sockaddr_in6*)&addr)->sin6_port = htons(IDENT_PORT);
            break;
        default :
            return false;
    }

    int sock = socket(addr.ss_family, SOCK_STREAM, 0);
    if (sock < 0) { return false; }

    SetReadTimeout(sock, timeout);
    SetWriteTimeout(sock, timeout);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) { return false; }

    FILE* fp = fdopen(sock, "r+");
    if (!fp) {
        close(sock);
        return false;
    }

    int localPort = PortFromSockaddr(localAddr);
    int remotePort = PortFromSockaddr(peerAddr);

    if (fprintf(fp, "%i, %i\r\n", remotePort, localPort) < 0) {
        fclose(fp);
        return false;
    }

    int replyLocalPort;
    int replyRemotePort;

    int ret = fscanf(fp, "%i , %i : USERID :%*[^:]:%255s\r\n", &replyRemotePort,
                     &replyLocalPort, user);
    fclose(fp);

    return ret == 3 && replyLocalPort == localPort && replyRemotePort == remotePort;
}
