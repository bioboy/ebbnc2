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
#include <fcntl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "misc.h"

bool isValidIP(const char* ip)
{
    struct sockaddr_any addr;
    return ipPortToSockaddr(ip, 0, &addr);
}

bool isValidHost(const char* host)
{
    struct sockaddr_any addr;
    return hostPortToSockaddr(host, 0, &addr, NULL);
}

bool isValidPort(int port)
{
    return port >= 0 && port <= 65535;
}

bool ipPortToSockaddr(const char* ip, int port, struct sockaddr_any* addr)
{
    memset(addr, 0, sizeof(*addr));
    if (inet_pton(AF_INET, ip, &addr->s4.sin_addr) == 1) {
        addr->san_family = AF_INET;
        addr->s4.sin_port = htons(port);
        return true;
    }

    if (inet_pton(AF_INET6, ip, &addr->s6.sin6_addr) == 1) {
        addr->san_family = AF_INET6;
        addr->s6.sin6_port = htons(port);
        return true;
    }

    errno = EAFNOSUPPORT;
    return false;
}

bool hostPortToSockaddr(const char* host, int port, struct sockaddr_any* addr, const char** errmsg)
{
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_socktype = SOCK_STREAM;

    char service[6];
    snprintf(service, sizeof(service), "%i", port);

    struct addrinfo* res;
    int error = getaddrinfo(host, service, &hints, &res);
    if (error != 0) {
      if (errmsg) {
        if (error == EAI_SYSTEM) {
          *errmsg = NULL;
        }
        else {
          *errmsg = gai_strerror(error);
        }
      }
      return false;
    }

    memcpy(addr, res->ai_addr, res->ai_addrlen);
    return true;
}

int portFromSockaddr(const struct sockaddr_any* addr)
{
    switch (addr->san_family) {
        case AF_INET :
            return ntohs(addr->s4.sin_port);
        case AF_INET6 :
            return ntohs(addr->s6.sin6_port);
        default :
            errno = EAFNOSUPPORT;
            return -1;
    }
}

bool ipFromSockaddr(const struct sockaddr_any* addr, char* ip)
{
    switch (addr->san_family) {
        case AF_INET :
            return inet_ntop(addr->san_family, &addr->s4.sin_addr, ip,
                             INET6_ADDRSTRLEN);
        case AF_INET6 :
            return inet_ntop(addr->san_family, &addr->s6.sin6_addr, ip,
                             INET6_ADDRSTRLEN);
        default :
            errno = EAFNOSUPPORT;
            return -1;
    }
}

void stripCRLF(char* buf)
{
    while (*buf && *buf != '\n' && *buf != '\r') { ++buf; }
    *buf = '\0';
}

char* strPrintf(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    size_t len = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    char* buf = calloc(len + 1, sizeof(char));
    if (!buf) {
        va_end(args);
        return NULL;
    }

    va_start(args, fmt);
    vsnprintf(buf, len + 1, fmt, args);
    va_end(args);

    return buf;
}

char* strCatPrintf(char* s, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    char* buf1 = calloc(len + 1, sizeof(char));
    if (!buf1) {
        va_end(args);
        free(s);
        return NULL;
    }

    va_start(args, fmt);
    len = vsnprintf(buf1, len + 1, fmt, args);
    va_end(args);

    char* buf2 = buf1;
    if (s != NULL) {
        size_t size = strlen(s) + len + 1;
        buf2 = calloc(size, sizeof(char));
        if (!buf2) {
            free(buf1);
            free(s);
            return NULL;
        }

        *buf2 = '\0';

        strncat(buf2, s, size);
        strncat(buf2, buf1, size);
        free(buf1);
    }

    free(s);
    return buf2;
}

int isAlreadyRunning(const char* pidFile)
{
    FILE* fp = fopen(pidFile, "r");
    if (!fp) {
        if (errno == ENOENT) { return 0; }
        return -1;
    }

    pid_t pid;
    if (fscanf(fp, "%i", &pid) != 1) {
        errno = EINVAL;
        return -1;
    }

    fclose(fp);

    if (kill(pid, 0) < 0) {
        if (errno == ESRCH) { return 0; }
        return -1;
    }

    return 1;
}

bool createPIDFile(const char* pidFile, pid_t pid)
{
    FILE* fp = fopen(pidFile, "w");
    if (!fp) { return false; }

    int ret = fprintf(fp, "%i\n", pid);
    fclose(fp);

    return ret > 0;
}

pid_t daemonise()
{
    pid_t pid = fork();
    if (pid != 0) { return pid; }

    IGNORE_RESULT(freopen("/dev/null", "w", stdout));
    IGNORE_RESULT(freopen("/dev/null", "w", stdin));

    setsid();
    return 0;
}

void setTimeout(int sock, time_t timeout, int opt)
{
    struct timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    setsockopt(sock, SOL_SOCKET, opt, (char*)&tv, sizeof(tv));
}

void setReadTimeout(int sock, time_t timeout)
{
    setTimeout(sock, timeout, SO_RCVTIMEO);
}

void setWriteTimeout(int sock, time_t timeout)
{
    setTimeout(sock, timeout, SO_SNDTIMEO);
}

bool strToInt(const char* s, int* i)
{
    char* p;
    long value = strtol(s, &p, 10);
    *i = (int) value;
    return *p == '\0';
}

char* promptInput(const char* prompt, const char* defaultValue)
{
    static char buffer[BUFSIZ];

    if (defaultValue != NULL) {
        printf("%s [%s]: ", prompt, defaultValue);
    }
    else {
        printf("%s: ", prompt);
    }

    if (!fgets(buffer, sizeof(buffer), stdin)) {
        *buffer = '\0';
        return buffer;
    }

    char* p = buffer + strlen(buffer) - 1;
    while (*p == '\n' || *p == '\r') { *p-- = '\0'; }

    if (*buffer == '\0' && defaultValue != NULL) {
        size_t len = strlen(defaultValue);
        if (len > sizeof(buffer) - 1) { len = sizeof(buffer) - 1; }
        strncpy(buffer, defaultValue, len);
        buffer[len] = '\0';
    }

    return buffer;
}

void hline()
{
    printf("------------------------------------------------------- --- -> >\n");
}

socklen_t sockaddrLen(const struct sockaddr_any* addr)
{
    return addr->san_family == AF_INET ?
           sizeof(struct sockaddr_in) :
           sizeof(struct sockaddr_in6);
}
