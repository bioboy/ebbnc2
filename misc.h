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

#ifndef EBBNC_MISC_H
#define EBBNC_MISC_H

#include <stdbool.h>
#include <sys/socket.h>
#include <time.h>
#include <sys/types.h>
#include <netinet/in.h>

struct sockaddr_any
{
  union
  {
    sa_family_t san_family;
    struct sockaddr_storage ss;
    struct sockaddr_in s4;
    struct sockaddr_in6 s6;
    struct sockaddr sa;
  };
};

bool isValidIP(const char* ip);
bool isValidHost(const char* host);
bool isValidPort(int port);
bool ipPortToSockaddr(const char* ip, int port, struct sockaddr_any* addr);
bool hostPortToSockaddr(const char* host, int port, struct sockaddr_any* addr, const char** errmsg);
int portFromSockaddr(const struct sockaddr_any* addr);
bool ipFromSockaddr(const struct sockaddr_any* addr, char* ip);
void stripCRLF(char* buf);
char* strPrintf(const char* fmt, ...);
char* strCatPrintf(char* s, const char* fmt, ...);
int isAlreadyRunning(const char* pidFile);
bool createPIDFile(const char* pidFile, pid_t pid);
int daemonise();
void setReadTimeout(int sock, time_t timeout);
void setWriteTimeout(int sock, time_t timeout);
bool strToInt(const char* s, int* i);
char* promptInput(const char* prompt, const char* defaultValue);
void hline();
socklen_t sockaddrLen(const struct sockaddr_any* addr);

#define IGNORE_RESULT(x) ({ typeof(x) z = x; (void)sizeof(z); })

#endif
