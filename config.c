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

Bouncer* Bouncer_new()
{
    Bouncer* bouncer = calloc(1, sizeof(Bouncer));
    if (!bouncer) { return NULL; }

    bouncer->listenPort = -1;
    bouncer->remotePort = -1;

    return bouncer;
}

void Bouncer_free(Bouncer** bouncerp)
{
    if (*bouncerp) {
        Bouncer* bouncer = *bouncerp;
        free(bouncer->listenIP);
        free(bouncer->remoteHost);
        free(bouncer);
        *bouncerp = NULL;
    }
}

void Bouncer_freeList(Bouncer** bouncerp)
{
    if (*bouncerp) {
        Bouncer* bouncer = *bouncerp;
        while (bouncer->next) {
            Bouncer* temp = bouncer->next->next;
            Bouncer_free(&bouncer->next);
            bouncer->next = temp;
        }
        Bouncer_free(&bouncer);
        *bouncerp = NULL;
    }
}

Bouncer* Bouncer_parse(const char* s)
{
    Bouncer* bouncer = Bouncer_new();

    char* temp = strdup(s);
    if (!temp) { goto strduperror; }

    char* p = strtok(temp, ":");
    if (!p || *p == '\0') { goto parseerror; }

    bouncer->listenIP = strdup(p);
    if (!bouncer->listenIP) { goto strduperror; }

    p = strtok(NULL, " ");
    if (!p || *p == '\0') { goto parseerror; }

    if (sscanf(p, "%li", &bouncer->listenPort) != 1) { goto parseerror; }

    p = strtok(NULL, ":");
    if (!p || *p == '\0') { goto parseerror; }

    bouncer->remoteHost = strdup(p);
    if (!bouncer->remoteHost) { goto strduperror; }

    p = strtok(NULL, " ");
    if (!p || *p == '\0') { goto parseerror; }

    if (sscanf(p, "%li", &bouncer->remotePort) != 1) { goto parseerror; }

    p = strtok(NULL, " ");
    if (p && *p != '\0') {
        bouncer->localIP = strdup(p);
        if (!bouncer->localIP) { goto strduperror; }

        p = strtok(NULL, " ");
        if (p && p != '\0') { goto parseerror; }
    }

    free(temp);
    return bouncer;

strduperror:
    free(temp);
    Bouncer_free(&bouncer);
    errno = ENOMEM;
    return NULL;

parseerror:
    free(temp);
    Bouncer_free(&bouncer);
    errno = 0;
    return NULL;
}

Config* Config_new()
{
    Config* config = calloc(1, sizeof(Config));
    if (!config) { return NULL; }

    config->idnt = true;
    config->identTimeout = 10;
    config->idleTimeout = 0;
    config->writeTimeout = 30;
    config->dnsLookup = true;

    return config;
}

void Config_free(Config** configp)
{
    if (*configp) {
        Config* config = *configp;
        free(config->pidFile);
        free(config->welcomeMsg);
        free(config);
        *configp = NULL;
    }
}

void invalidValueError(const char* option)
{
  fprintf(stderr, "Config option has invalid value: %s\n", option);
}

void requiredOptionError(const char* option)
{
  fprintf(stderr, "Config option is required: %s\n", option);
}

bool Config_sanityCheck(Config* config)
{
    bool insane = false;

    if (!config->bouncers) {
        requiredOptionError("bouncer");
        insane = true;
    }
    else {
        Bouncer* bouncer = config->bouncers;
        while (bouncer && !insane) {
            if (!isValidIP(bouncer->listenIP)) {
              invalidValueError("listenip");
              insane = true;
            }

            if (!isValidPort(bouncer->listenPort)) {
              invalidValueError("listenport");
              insane = true;
            }

            if (!bouncer->remoteHost) {
                requiredOptionError("remotehost");
                insane = true;
            }

            if (!isValidHost(bouncer->remoteHost)) {
              invalidValueError("remotehost");
              insane = true;
            }

            if (!isValidPort(bouncer->remotePort)) {
              invalidValueError("remoteport");
              insane = true;
            }

            if (bouncer->localIP && !isValidIP(bouncer->localIP)) {
              invalidValueError("localip");
              insane = true;
            }

            bouncer = bouncer->next;
        }
    }


    return !insane;
}

Config* Config_loadBuffer(const char* buffer)
{
    Config* config = Config_new();
    if (!config) {
        fprintf(stderr, "Unable to load config: %s\n", strerror(errno));
        return NULL;
    }

    bool error = false;
    const char* p1 = buffer;
    char* line = NULL;
    while (!error && *p1) {
        const char* p2 = p1;
        while (*p2 && *p2 != '\n' && *p2 != '\r') { p2++; }

        size_t len = p2 - p1;
        line = strndup(p1, len);
        if (!line) { goto strduperror; }

        while (*p2 == '\r' || *p2 == '\n') { p2++; }
        p1 = p2;

        if (*line == '\0' || *line == '#') {
            free(line);
            line = NULL;
            continue;
        }

        if (!strncasecmp(line, "bouncer=", 8) && len > 8) {
            Bouncer* bouncer = Bouncer_parse(line + 8);
            if (!bouncer) {
                if (errno == ENOMEM) {
                    goto strduperror;
                }
                else {
                    error = true;
                }
            }
            else {
                bouncer->next = config->bouncers;
                config->bouncers = bouncer;
            }
        }
        else if (!strncasecmp(line, "idnt=", 5)) {
            char* value = line + 5;
            if (!strcasecmp(value, "true")) {
                config->idnt = true;
            }
            else if (!strcasecmp(value, "false")) {
                config->idnt = false;
            }
            else {
                error = true;
            }
        }
        else if (!strncasecmp(line, "identtimeout=", 13) && len > 13) {
            if (strToInt(line + 13, &config->identTimeout) != 1 || config->identTimeout < 0) {
                error = true;
            }
        }
        else if (!strncasecmp(line, "idletimeout=", 12) && len > 12) {
            if (strToInt(line + 12, &config->idleTimeout) != 1 || config->idleTimeout < 0) {
                error = true;
            }
        }
        else if (!strncasecmp(line, "writetimeout=", 13) && len > 13) {
            if (strToInt(line + 13, &config->writeTimeout) != 1 || config->writeTimeout < 0) {
                error = true;
            }
        }
        else if (!strncasecmp(line, "dnslookup=", 10) && len > 10) {
            char* value = line + 10;
            if (!strcasecmp(value, "true")) {
                config->dnsLookup = true;
            }
            else if (!strcasecmp(value, "false")) {
                config->dnsLookup = false;
            }
            else {
                error = true;
            }
        }
        else if (!strncasecmp(line, "pidfile=", 8) && len > 8) {
            config->pidFile = strdup(line + 8);
            if (!config->pidFile) { goto strduperror; }
        }
        else if (!strncasecmp(line, "welcomemsg=", 11) && len > 11) {
            config->welcomeMsg = strdup(line + 11);
            if (!config->welcomeMsg) { goto strduperror; }
        }
        else {
            error = true;
        }

        if (!error) {
            free(line);
            line = NULL;
        }
    }

    if (error) {
        fprintf(stderr, "Error on this line in config file: %s\n", line);
        free(line);
        Config_free(&config);
        return NULL;
    }

    if (!Config_sanityCheck(config)) {
        Config_free(&config);
        return NULL;
    }

    return config;

strduperror:
    fprintf(stderr, "Unable to load config: %s\n", strerror(errno));
    free(line);
    Config_free(&config);
    return NULL;
}

Config* Config_loadFile(const char* path)
{
    FILE* fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "Unable to open config file: %s\n", strerror(errno));
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char* buffer = calloc(size + 1, sizeof(char));
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

    Config* config = Config_loadBuffer(buffer);
    free(buffer);

    return config;
}

#ifdef CONF_EMBEDDED

char* Config_decryptEmbedded(const char* key)
{
    size_t confLen = strlen(CONF_EMBEDDED);
    size_t cipherSize = confLen / 2 + 1;
    char* cipher = calloc(cipherSize, sizeof(char));
    if (!cipher) { return NULL; }

    ssize_t cipherLen = hexDecode(CONF_EMBEDDED, confLen, cipher, cipherSize);
    if (cipherLen < XTEA_BLOCK_SIZE) {
        free(cipher);
        return NULL;
    }

    size_t plainSize = cipherLen;
    char* plain = calloc(plainSize, sizeof(char));
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

Config* Config_loadEmbedded(const char* key)
{
    char* buffer = Config_decryptEmbedded(key);
    if (!buffer) {
        fprintf(stderr, "Error while decrypting embedded conf!\n");
        return NULL;
    }

    Config* config = Config_loadBuffer(buffer);
    free(buffer);
    return config;
}

#endif

char* Config_saveBuffer(Config* config)
{
    char* buffer = strCatPrintf(NULL, "idnt=%s\n", config->idnt ? "true" : "false");
    if (!buffer) { return NULL; }

    buffer = strCatPrintf(buffer, "identtimeout=%i\n", config->identTimeout);
    if (!buffer) { return NULL; }

    buffer = strCatPrintf(buffer, "idletimeout=%i\n", config->idleTimeout);
    if (!buffer) { return NULL; }

    buffer = strCatPrintf(buffer, "writetimeout=%i\n", config->writeTimeout);
    if (!buffer) { return NULL; }

    buffer = strCatPrintf(buffer, "dnslookup=%s\n", config->dnsLookup ? "true" : "false");
    if (!buffer) { return NULL; }

    if (config->pidFile) {
        buffer = strCatPrintf(buffer, "pidfile=%s\n", config->pidFile);
        if (!buffer) { return NULL; }
    }

    if (config->welcomeMsg) {
        buffer = strCatPrintf(buffer, "welcomemsg=%s\n", config->welcomeMsg);
        if (!buffer) { return NULL; }
    }

    Bouncer* bouncer = config->bouncers;
    while (bouncer) {
        buffer = strCatPrintf(buffer, "bouncer=%s:%i %s:%i\n", bouncer->listenIP, bouncer->listenPort,
                                                               bouncer->remoteHost, bouncer->remotePort);
        if (!buffer) { return NULL; }
        bouncer = bouncer->next;
    }

    return buffer;
}
