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

bool ValidIP(const char* ip);
bool ValidHost(const char* host);
bool ValidPort(int port);
bool IPPortToSockaddr(const char* ip, int port, struct sockaddr_any* addr);
bool HostPortToSockaddr(const char* host, int port, struct sockaddr_any* addr, const char** errmsg);
int PortFromSockaddr(const struct sockaddr_any* addr);
bool IPFromSockaddr(const struct sockaddr_any* addr, char* ip);
void StripCRLF(char* buf);
char* Sprintf(const char* fmt, ...);
char* Scatprintf(char* s, const char* fmt, ...);
int AlreadyRunning(const char* pidFile);
bool CreatePIDFile(const char* pidFile, pid_t pid);
int Daemonise();
void SetReadTimeout(int sock, time_t timeout);
void SetWriteTimeout(int sock, time_t timeout);
bool StrToInt(const char* s, int* i);
char* PromptInput(const char* prompt, const char* defaultValue);
void Hline();
socklen_t SockaddrLen(const struct sockaddr_any* addr);

#define IGNORE_RESULT(x) ({ typeof(x) z = x; (void)sizeof(z); })

#endif
