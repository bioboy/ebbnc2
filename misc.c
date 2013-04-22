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
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include "misc.h"

bool IPPortToSockaddr(const char* ip, int port, struct sockaddr_storage* addr)
{
  memset(addr, 0, sizeof(addr));
  if (inet_pton(AF_INET, ip, &((struct sockaddr_in*)addr)->sin_addr) == 1) {
    addr->ss_family = AF_INET;
    ((struct sockaddr_in*)addr)->sin_port = htons(port);
    return true;
  }

  if (inet_pton(AF_INET6, ip, &((struct sockaddr_in6*)addr)->sin6_addr) == 1) {
    addr->ss_family = AF_INET6;
    ((struct sockaddr_in6*)addr)->sin6_port = htons(port);
    return true;
  }
  
  errno = EAFNOSUPPORT;
  return false;
}

int PortFromSockaddr(const struct sockaddr_storage* addr)
{
  switch (addr->ss_family) {
    case AF_INET:
      return ntohs(((struct sockaddr_in*)addr)->sin_port);
    case AF_INET6:
      return ntohs(((struct sockaddr_in6*)addr)->sin6_port);
    default:
      errno = EAFNOSUPPORT;
      return -1;
  }
}

bool IPFromSockaddr(const struct sockaddr_storage* addr, char* ip)
{
  switch (addr->ss_family) {
    case AF_INET:
      return inet_ntop(addr->ss_family, &((struct sockaddr_in*)addr)->sin_addr, ip, INET6_ADDRSTRLEN);
    case AF_INET6:
      return inet_ntop(addr->ss_family, &((struct sockaddr_in6*)addr)->sin6_addr, ip, INET6_ADDRSTRLEN);
    default:
      errno = EAFNOSUPPORT;
      return -1;
  }
}

void StripCRLF(char* buf)
{
  while (*buf && *buf != '\n' && *buf != '\r') ++buf;
  *buf = '\0';
}

char* Sprintf(const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  size_t len = vsnprintf(NULL, 0, fmt, args);
  va_end(args);
  
  char* buf = malloc(len + 1);
  if (!buf) {
    va_end(args);
    return NULL;
  }
  
  va_start(args, fmt);
  vsnprintf(buf, len + 1, fmt, args);
  va_end(args);
  
  return buf;
}

int AlreadyRunning(const char* pidFile)
{
  FILE* fp = fopen(pidFile, "r");
  if (!fp) {
    if (errno == ENOENT) return 0;
    return -1;
  }
  
  pid_t pid;
  if (fscanf(fp, "%i", &pid) != 1) {
    errno = EINVAL;
    return -1;
  }

  fclose(fp);
  
  if (kill(pid, 0) < 0) {
    if (errno == ESRCH) return 0;
    return -1;
  }
  
  return 1;
}

bool CreatePIDFile(const char* pidFile)
{
  FILE* fp = fopen(pidFile, "w");
  if (!fp) return false;
  
  int ret = fprintf(fp, "%i\n", getpid());
  fclose(fp);
  
  return ret > 0;
}

pid_t Daemonise()
{
  pid_t pid = fork();
  if (pid != 0) return pid;
  
  IGNORE_RESULT(freopen("/dev/null", "w", stdout));
  IGNORE_RESULT(freopen("/dev/null", "w", stdin));
  
  setsid();
  return 0;
}

void SetTimeout(int sock, time_t timeout, int opt)
{
  struct timeval tv;      
  tv.tv_sec = timeout;
  tv.tv_usec = 0;

  setsockopt(sock, SOL_SOCKET, opt, (char *)&tv, sizeof(tv));
}

void SetReadTimeout(int sock, time_t timeout)
{
  SetTimeout(sock, timeout, SO_RCVTIMEO);
}

void SetWriteTimeout(int sock, time_t timeout)
{
  SetTimeout(sock, timeout, SO_SNDTIMEO);
}
