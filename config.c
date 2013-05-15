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
#include <limits.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include "config.h"
#include "misc.h"
#include "conf.h"
#include "hex.h"
#include "xtea.h"

struct Config* Config_New()
{
    struct Config* c = malloc(sizeof(struct Config));
    if (!c) { return NULL; }

    c->listenIP = NULL;
    c->listenPort = -1;
    c->remoteIP = NULL;
    c->remotePort = -1;
    c->idnt = true;
    c->identTimeout = 10;
    c->idleTimeout = 0;
    c->writeTimeout = 30;
    c->dnsLookup = true;
    c->pidFile = NULL;
    c->welcomeMsg = NULL;

    return c;
}

void Config_Free(struct Config** cp)
{
    if (*cp) {
        struct Config* c = *cp;
        free(c->listenIP);
        free(c->remoteIP);
        free(c->pidFile);
        free(c->welcomeMsg);
        free(c);
        *cp = NULL;
    }
}

bool Config_SanityCheck(struct Config* c)
{
    bool insane = false;
    if (c->listenPort == -1) {
        fprintf(stderr, "Config option is required: listenport\n");
        insane = true;
    }

    if (!c->remoteIP) {
        fprintf(stderr, "Config option is required: remoteip\n");
        insane = true;
    }

    if (c->remotePort == -1) {
        fprintf(stderr, "Config option is required: remoteport\n");
        insane = true;
    }

    return !insane;
}

struct Config* Config_LoadBuffer(const char* buffer)
{
    struct Config* c = Config_New();
    if (!c) {
        fprintf(stderr, "Unable to load config: %s\n", strerror(errno));
        return NULL;
    }

    bool error = false;
    int lineNo = 0;
    const char* p1 = buffer;
    char* line = NULL;
    while (!error && *p1) {
        ++lineNo;
        const char* p2 = p1;
        while (*p2 && *p2 != '\n' && *p2 != '\r') { p2++; }

        size_t len = p2 - p1;
        char* line = strndup(p1, len);
        if (!line) { goto strduperror; }

        while (*p2 == '\r' || *p2 == '\n') { p2++; }
        p1 = p2;

        if (*line == '\0' || *line == '#') {
            free(line);
            line = NULL;
            continue;
        }

        if (!strncasecmp(line, "listenip=", 9) && len > 9) {
            c->listenIP = strdup(line + 9);
            if (!c->listenIP) { goto strduperror; }
        }
        else if (!strncasecmp(line, "listenport=", 11) && len > 11) {
            if (StrToInt(line + 11, &c->listenPort) != 1 || c->listenPort < 0) {
                error = true;
            }
        }
        else if (!strncasecmp(line, "remoteip=", 9) && len > 9) {
            c->remoteIP = strdup(line + 9);
            if (!c->remoteIP) { goto strduperror; }
        }
        else if (!strncasecmp(line, "remoteport=", 11) && len > 11) {
            if (StrToInt(line + 11, &c->remotePort) != 1 || c->remotePort < 0) {
                error = true;
            }
        }
        else if (!strncasecmp(line, "idnt=", 5)) {
            char* value = line + 5;
            if (!strcasecmp(value, "true")) {
                c->idnt = true;
            }
            else if (!strcasecmp(value, "false")) {
                c->idnt = false;
            }
            else {
                error = true;
            }
        }
        else if (!strncasecmp(line, "identtimeout=", 13) && len > 13) {
            if (StrToInt(line + 13, &c->identTimeout) != 1 || c->identTimeout < 0) {
                error = true;
            }
        }
        else if (!strncasecmp(line, "idletimeout=", 12) && len > 12) {
            if (StrToInt(line + 12, &c->idleTimeout) != 1 || c->idleTimeout < 0) {
                error = true;
            }
        }
        else if (!strncasecmp(line, "writetimeout=", 13) && len > 13) {
            if (StrToInt(line + 13, &c->writeTimeout) != 1 || c->writeTimeout < 0) {
                error = true;
            }
        }
        else if (!strncasecmp(line, "dnslookup=", 10) && len > 10) {
            char* value = line + 10;
            if (!strcasecmp(value, "true")) {
                c->dnsLookup = true;
            }
            else if (!strcasecmp(value, "false")) {
                c->dnsLookup = false;
            }
            else {
                error = true;
            }
        }
        else if (!strncasecmp(line, "pidfile=", 8) && len > 8) {
            c->pidFile = strdup(line + 8);
            if (!c->pidFile) { goto strduperror; }
        }
        else if (!strncasecmp(line, "welcomemsg=", 11) && len > 11) {
            c->welcomeMsg = strdup(line + 11);
            if (!c->welcomeMsg) { goto strduperror; }
        }
        else {
            error = true;
        }

        free(line);
    }

    if (error) {
        fprintf(stderr, "Error at line %i in config file.\n", lineNo);
        Config_Free(&c);
        return NULL;
    }

    if (!c->listenIP) {
        c->listenIP = strdup("0.0.0.0");
        if (!c->listenIP) { goto strduperror; }
    }

    if (!Config_SanityCheck(c)) {
        Config_Free(&c);
        return NULL;
    }

    return c;

strduperror:
    fprintf(stderr, "Unable to load config: %s\n", strerror(errno));
    free(line);
    Config_Free(&c);
    return NULL;
}

struct Config* Config_LoadFile(const char* path)
{
    FILE* fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "Unable to open config file: %s\n", strerror(errno));
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char* buffer = malloc(size + 1);
    if (!buffer) {
        fprintf(stderr, "Unable to load config: %s\n", strerror(errno));
        fclose(fp);
        return NULL;
    }

    bool okay = fread(buffer, size, 1, fp) == 1 && !ferror(fp);
    fclose(fp);

    if (!okay) {
        fprintf(stderr, "Unknown error while reading config file.\n");
        free(buffer);
        return NULL;
    }

    *(buffer + size) = '\0';

    struct Config* c = Config_LoadBuffer(buffer);
    free(buffer);

    return c;
}

#ifdef CONF_EMBEDDED

char* Config_DecryptEmbedded(const char* key)
{
    size_t confLen = strlen(CONF_EMBEDDED);
    size_t cipherSize = confLen / 2 + 1;
    char* cipher = malloc(cipherSize);
    if (!cipher) { return NULL; }

    ssize_t cipherLen = HexDecode(CONF_EMBEDDED, confLen, cipher, cipherSize);
    if (cipherLen < XTEA_BLOCK_SIZE) {
        free(cipher);
        return NULL;
    }

    size_t plainSize = cipherLen;
    char* plain = malloc(plainSize);
    if (!plain) {
        free(cipher);
        return NULL;
    }

    unsigned char ivec[XTEA_BLOCK_SIZE];
    memcpy(ivec, cipher, XTEA_BLOCK_SIZE);

    ssize_t plainLen = XTeaDecryptCBC((unsigned char*) cipher + XTEA_BLOCK_SIZE,
                                      cipherLen - XTEA_BLOCK_SIZE,
                                      (unsigned char*) plain, plainSize,
                                      (unsigned char*) ivec, (unsigned char*) key);
    if (plainLen < 0) {
        free(cipher);
        free(plain);
        return NULL;
    }

    free(cipher);
    plain[plainLen] = '\0';

    // check if decryption was successful
    // should contain no control chars
    char* p;
    for (p = plain; *p; p++) {
        if (iscntrl(*p) && !isspace(*p)) {
            free(plain);
            return NULL;
        }
    }

    return plain;
}

struct Config* Config_LoadEmbedded(const char* key)
{
    char* buffer = Config_DecryptEmbedded(key);
    if (!buffer) {
        fprintf(stderr, "Error while decrypting embedded conf!\n");
        return NULL;
    }

    struct Config* cfg = Config_LoadBuffer(buffer);
    free(buffer);
    return cfg;
}

#endif

char* Config_SaveBuffer(struct Config* c)
{
    char* buffer = Scatprintf(NULL, "listenip=%s\n", c->listenIP);
    if (!buffer) { return NULL; }

    buffer = Scatprintf(buffer, "listenport=%i\n", c->listenPort);
    if (!buffer) { return NULL; }

    buffer = Scatprintf(buffer, "remoteip=%s\n", c->remoteIP);
    if (!buffer) { return NULL; }

    buffer = Scatprintf(buffer, "remoteport=%i\n", c->remotePort);
    if (!buffer) { return NULL; }

    buffer = Scatprintf(buffer, "idnt=%s\n", c->idnt ? "true" : "false");
    if (!buffer) { return NULL; }

    buffer = Scatprintf(buffer, "identtimeout=%i\n", c->identTimeout);
    if (!buffer) { return NULL; }

    buffer = Scatprintf(buffer, "idletimeout=%i\n", c->idleTimeout);
    if (!buffer) { return NULL; }

    buffer = Scatprintf(buffer, "writetimeout=%i\n", c->writeTimeout);
    if (!buffer) { return NULL; }

    buffer = Scatprintf(buffer, "dnslookup=%s\n", c->dnsLookup ? "true" : "false");
    if (!buffer) { return NULL; }

    if (c->pidFile) {
        buffer = Scatprintf(buffer, "pidfile=%s\n", c->pidFile);
        if (!buffer) { return NULL; }
    }

    if (c->welcomeMsg) {
        buffer = Scatprintf(buffer, "welcomemsg=%s\n", c->welcomeMsg);
        if (!buffer) { return NULL; }
    }

    return buffer;
}
