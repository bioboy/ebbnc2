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
#include <arpa/inet.h>
#include <time.h>

bool IPPortToSockaddr(const char* ip, int port, struct sockaddr_storage* addr);
int PortFromSockaddr(const struct sockaddr_storage* addr);
bool IPFromSockaddr(const struct sockaddr_storage* addr, char* ip);
void StripCRLF(char* buf);
char* Sprintf(const char* fmt, ...);
int AlreadyRunning(const char* pidFile);
bool CreatePIDFile(const char* pidFile);
int Daemonise();
void SetReadTimeout(int sock, time_t timeout);
void SetWriteTimeout(int sock, time_t timeout);

#define IGNORE_RESULT(x) ({ typeof(x) z = x; (void)sizeof z; })

#endif
