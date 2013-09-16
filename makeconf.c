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
        STAGE_LISTEN_IP,
        STAGE_LISTEN_PORT,
        STAGE_REMOTE_HOST,
        STAGE_REMOTE_PORT,
        STAGE_IDNT,
        STAGE_IDENT_TIMEOUT,
        STAGE_IDLE_TIMEOUT,
        STAGE_WRITE_TIMEOUT,
        STAGE_DNS_LOOKUP,
        STAGE_PID_FILE,
        STAGE_WELCOME_MSG,
        STAGE_PASSWORD

    } stage = STAGE_LISTEN_IP;

    enum Error {
        ERROR_NONE,
        ERROR_DEFAULT,
        ERROR_VALUE,
        ERROR_STRDUP
    };

    signal(SIGINT, abortHandler);

    hline();
    printf("%s conf editor %s by %s\n", EBBNC_PROGRAM, EBBNC_VERSION, EBBNC_AUTHOR);
    hline();
    atexit(hline);

    Config* c = Config_new();
    if (!c) {
        fprintf(stderr, "Unable to create config: %s\n", strerror(errno));
        return 1;
    }

    char key[17] = { 0 };
    bool done = false;
    enum Error error;
    while (!done) {

        error = ERROR_NONE;
        switch (stage) {
            case STAGE_LISTEN_IP : {
                char* value = promptInput("Listen ip", "0.0.0.0");
                if (isValidIP(value)) {
                    c->listenIP = strdup(value);
                    if (!c->listenIP) { error = ERROR_STRDUP; }
                }
                else {
                    error = ERROR_VALUE;
                }
                break;
            }
            case STAGE_LISTEN_PORT : {
                char* value = promptInput("Listen port", NULL);
                if (*value != '\0') {
                    if (strToInt(value, &c->listenPort) != 1 || !isValidPort(c->listenPort)) {
                        error = ERROR_VALUE;
                    }
                }
                else {
                    error = ERROR_DEFAULT;
                }
                break;
            }
            case STAGE_REMOTE_HOST : {
                char* value = promptInput("Remote host/ip", NULL);
                if (*value != '\0') {
                    if (isValidHost(value)) {
                        c->remoteHost = strdup(value);
                        if (!c->remoteHost) { error = ERROR_STRDUP; }
                    }
                    else {
                      error = ERROR_VALUE;
                    }
                }
                else {
                    error = ERROR_DEFAULT;
                }
                break;
            }
            case STAGE_REMOTE_PORT : {
                char* value = promptInput("Remote port", NULL);
                if (*value != '\0') {
                    if (strToInt(value, &c->remotePort) != 1 || !isValidPort(c->remotePort)) {
                        error = ERROR_VALUE;
                    }
                }
                else {
                    error = ERROR_DEFAULT;
                }
                break;
            }
            case STAGE_IDNT : {
                char* value = promptInput("Send idnt", "true");
                if (!strcasecmp(value, "true")) {
                    c->idnt = true;
                }
                else if (!strcasecmp(value, "false")) {
                    c->idnt = false;
                }
                else {
                    error = ERROR_VALUE;
                }
                break;
            }
            case STAGE_IDENT_TIMEOUT : {
                if (!c->idnt) { break; }

                char* value = promptInput("Ident timeout", "10");
                if (*value != '\0') {
                    if (strToInt(value, &c->identTimeout) != 1 || c->identTimeout < 0) {
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
                    if (strToInt(value, &c->idleTimeout) != 1 || c->idleTimeout < 0) {
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
                    if (strToInt(value, &c->writeTimeout) != 1 || c->writeTimeout < 0) {
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
                    c->dnsLookup = true;
                }
                else if (!strcasecmp(value, "false")) {
                    c->dnsLookup = false;
                }
                else {
                    error = ERROR_VALUE;
                }
                break;
            }
            case STAGE_PID_FILE : {
                char* value = promptInput("Pid file path", "none");
                if (strcmp(value, "none")) {
                    c->pidFile = strdup(value);
                    if (!c->pidFile) { error = ERROR_STRDUP; }
                }
                break;
            }
            case STAGE_WELCOME_MSG : {
                char* value = promptInput("Welcome message", "none");
                if (strcmp(value, "none")) {
                    c->welcomeMsg = strdup(value);
                    if (!c->welcomeMsg) { error = ERROR_STRDUP; }
                }
                break;
            }
            case STAGE_PASSWORD : {
                char* value = getpass("Password: ");
                if (*value != '\0') {
                    strncpy(key, value, sizeof(key) - 1);
                }
                else {
                    error = ERROR_DEFAULT;
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
        }
    }

    if (error != ERROR_NONE) {
        Config_free(&c);
        return 1;
    }

    hline();

    printf("Serializing config to buffer ..\n");

    char* plain = Config_saveBuffer(c);
    Config_free(&c);

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
