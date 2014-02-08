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
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include "config.h"
#include "misc.h"
#include "xtea.h"
#include "hex.h"
#include "info.h"

#define CONF_HEADER "conf.h"
#define MINIMUM_PASSLEN 8

void abortHandler(int signo)
{
    static const char* abortMessage = "\nConfig creation aborted!\n";
    IGNORE_RESULT(write(fileno(stdout), abortMessage, strlen(abortMessage)));
    _exit(1);
    (void)signo;
}


char* encrypt(const char* plain, const char* key)
{
    size_t plainLen = strlen(plain);
    size_t cipherSize = plainLen + XTEA_BLOCK_SIZE * 2;
    char* cipher = calloc(cipherSize, sizeof(char));
    if (!cipher) { return NULL; }

    if (XTeaGenerateIVec((unsigned char*)cipher) < 0) { return NULL; }

    ssize_t len = XTeaEncryptCBC((unsigned char*)plain, strlen(plain),
                                 (unsigned char*)cipher + XTEA_BLOCK_SIZE, cipherSize - XTEA_BLOCK_SIZE,
                                 (unsigned char*)cipher, (unsigned char*)key);
    if (len < 0) {
        free(cipher);
        return NULL;
    }

    len += XTEA_BLOCK_SIZE;

    size_t encodedSize = len * 2 + 1;
    char* encoded = calloc(encodedSize, sizeof(char));
    if (!encoded) {
        free(cipher);
        return NULL;
    }

    len = hexEncode(cipher, len, encoded, encodedSize);
    if (len < 0) {
        free(cipher);
        free(encoded);
        return NULL;
    }

    free(cipher);
    return encoded;
}

bool saveHeader(const char* buffer)
{
    FILE* fp = fopen(CONF_HEADER, "w");
    if (!fp) { return false; }

    fprintf(fp, "#define CONF_EMBEDDED \"%s\"\n", buffer);

    bool okay = !ferror(fp);
    fclose(fp);

    return okay;
}

int main(int argc, char** argv)
{
    enum Stage {
        STAGE_BOUNCER_NEXT,
        STAGE_BOUNCER_LISTEN,
        STAGE_BOUNCER_REMOTE,
        STAGE_BOUNCER_LOCAL,
        STAGE_BOUNCER_ANOTHER,
        STAGE_IDNT,
        STAGE_IDENT_TIMEOUT,
        STAGE_IDLE_TIMEOUT,
        STAGE_WRITE_TIMEOUT,
        STAGE_DNS_LOOKUP,
        STAGE_PID_FILE,
        STAGE_WELCOME_MSG,
        STAGE_PASSWORD

    } stage = STAGE_BOUNCER_LISTEN;

    enum Error {
        ERROR_NONE,
        ERROR_DEFAULT,
        ERROR_VALUE,
        ERROR_STRDUP,
        ERROR_PASSLEN
    };

    signal(SIGINT, abortHandler);

    hline();
    printf("%s conf editor %s by %s\n", EBBNC_PROGRAM, EBBNC_VERSION, EBBNC_AUTHOR);
    hline();
    atexit(hline);

    Config* config = Config_new();
    if (!config) {
        fprintf(stderr, "Unable to create config: %s\n", strerror(errno));
        return 1;
    }

    unsigned int bouncerCount = 1;
    printf("Bouncer #%u -\n", bouncerCount);
    Bouncer* bouncer = Bouncer_new();

    char key[17] = { 0 };
    bool done = false;
    enum Error error;
    while (!done) {

        error = ERROR_NONE;
        switch (stage) {
            case STAGE_BOUNCER_LISTEN : {
                char* value = promptInput("Listen ip:port", NULL);

                char* p = strtok(value, ":");
                if (p && isValidIP(p)) {
                    bouncer->listenIP = strdup(p);
                    if (!bouncer->listenIP) { error = ERROR_STRDUP; }
                }
                else {
                    error = ERROR_VALUE;
                    break;
                }

                p = strtok(NULL, " ");
                if (!p || strToLong(p, &bouncer->listenPort) != 1 || !isValidPort(bouncer->listenPort)) {
                    error = ERROR_VALUE;
                    free(bouncer->listenIP);
                    bouncer->listenIP = NULL;
                    error = ERROR_VALUE;
                }

                break;
            }
            case STAGE_BOUNCER_REMOTE : {
                char* value = promptInput("Remote host:port", NULL);

                char* p = strtok(value, ":");
                if (p && isValidHost(p)) {
                    bouncer->remoteHost = strdup(p);
                    if (!bouncer->remoteHost) { error = ERROR_STRDUP; }
                }
                else {
                    error = ERROR_VALUE;
                    break;
                }

                p = strtok(NULL, " ");
                if (!p || strToLong(p, &bouncer->remotePort) != 1 || !isValidPort(bouncer->remotePort)) {
                    error = ERROR_VALUE;
                    free(bouncer->remoteHost);
                    bouncer->remoteHost = NULL;
                    error = ERROR_VALUE;
                }

                break;
            }
            case STAGE_BOUNCER_LOCAL : {
                char* value = promptInput("Local ip", "None");
                if (value && strcasecmp(value, "None") != 0) {
                    if (isValidIP(value)) {
                        bouncer->localIP = strdup(value);
                        if (!bouncer->localIP) { error = ERROR_STRDUP; }
                    }
                    else {
                        error = ERROR_VALUE;
                    }
                }
                break;
            }
            case STAGE_BOUNCER_ANOTHER : {
                bouncer->next = config->bouncers;
                config->bouncers = bouncer;
                bouncer = NULL;

                char* value = promptInput("Another bouncer?", "yes");
                if (!strcasecmp(value, "yes")) {
                    stage = STAGE_BOUNCER_NEXT;
                    printf("Bouncer #%u -\n", ++bouncerCount);
                    bouncer = Bouncer_new();
                }
                else if (!strcasecmp(value, "no")) {
                    stage++;
                }
                else {
                    error = ERROR_VALUE;
                }
                break;
            }
            case STAGE_IDNT : {
                char* value = promptInput("Send idnt", "true");
                if (!strcasecmp(value, "true")) {
                    config->idnt = true;
                }
                else if (!strcasecmp(value, "false")) {
                    config->idnt = false;
                }
                else {
                    error = ERROR_VALUE;
                }
                break;
            }
            case STAGE_IDENT_TIMEOUT : {
                if (!config->idnt) { break; }

                char* value = promptInput("Ident timeout", "10");
                if (*value != '\0') {
                    if (strToInt(value, &config->identTimeout) != 1 || config->identTimeout < 0) {
                        error = ERROR_VALUE;
                    }
                }
                else {
                    error = ERROR_DEFAULT;
                }
                break;
            }
            case STAGE_IDLE_TIMEOUT : {
                char* value = promptInput("Idle timeout", "0");
                if (*value != '\0') {
                    if (strToInt(value, &config->idleTimeout) != 1 || config->idleTimeout < 0) {
                        error = ERROR_VALUE;
                    }
                }
                else {
                    error = ERROR_DEFAULT;
                }
                break;
            }
            case STAGE_WRITE_TIMEOUT : {
                char* value = promptInput("Write timeout", "30");
                if (*value != '\0') {
                    if (strToInt(value, &config->writeTimeout) != 1 || config->writeTimeout < 0) {
                        error = ERROR_VALUE;
                    }
                }
                else {
                    error = ERROR_DEFAULT;
                }
                break;
            }
            case STAGE_DNS_LOOKUP : {
                char* value = promptInput("Dns lookup", "true");
                if (!strcasecmp(value, "true")) {
                    config->dnsLookup = true;
                }
                else if (!strcasecmp(value, "false")) {
                    config->dnsLookup = false;
                }
                else {
                    error = ERROR_VALUE;
                }
                break;
            }
            case STAGE_PID_FILE : {
                char* value = promptInput("Pid file path", "none");
                if (strcmp(value, "none")) {
                    config->pidFile = strdup(value);
                    if (!config->pidFile) { error = ERROR_STRDUP; }
                }
                break;
            }
            case STAGE_WELCOME_MSG : {
                char* value = promptInput("Welcome message", "none");
                if (strcmp(value, "none")) {
                    config->welcomeMsg = strdup(value);
                    if (!config->welcomeMsg) { error = ERROR_STRDUP; }
                }
                break;
            }
            case STAGE_PASSWORD : {
                char* value = getpass("Password: ");
                if (strlen(value) >= MINIMUM_PASSLEN) {
                    strncpy(key, value, sizeof(key) - 1);
                }
                else {
                    error = ERROR_PASSLEN;
                }
                break;
            }
            default : {
                done = true;
                break;
            }
        }

        switch (error) {
            case ERROR_NONE : {
                stage++;
                break;
            }
            case ERROR_STRDUP : {
                perror("strdup");
                done = true;
                break;
            }
            case ERROR_VALUE : {
                fprintf(stderr, "Invalid value.\n");
                break;
            }
            case ERROR_DEFAULT : {
                fprintf(stderr, "No default value.\n");
                break;
            }
            case ERROR_PASSLEN : {
                fprintf(stderr, "Password must be %i or more characters.\n", MINIMUM_PASSLEN);
            }
        }
    }

    if (error != ERROR_NONE) {
        Config_free(&config);
        return 1;
    }

    hline();

    printf("Serializing config to buffer ..\n");

   char* plain = Config_saveBuffer(config);
    Config_free(&config);

    if (!plain) {
        fprintf(stderr, "Failed to serialize config to buffer: %s\n", strerror(errno));
        return 1;
    }

    printf("Encrypting buffer with xtea cipher ..\n");

    char* cipher = encrypt(plain, key);
    if (!cipher) {
        fprintf(stderr, "Error while encrypting buffer.\n");
        free(plain);
        return 1;
    }

    printf("Saving config ..\n");

    if (!saveHeader(cipher)) {
        fprintf(stderr, "Error while saving %s.\n", CONF_HEADER);
        free(cipher);
        free(plain);
        return 1;
    }

    free(cipher);
    free(plain);

    printf("Successfully saved encrypted config!\n");
    hline();
    printf("Run 'make' to create an %s binary with the config embedded.\n",
           EBBNC_PROGRAM);

    return 0;

    (void)argc;
    (void)argv;
}
