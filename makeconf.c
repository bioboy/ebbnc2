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

#define CONF_HEADER "conf.h"

char* Input(const char* prompt, const char* defaultValue)
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
    if (len > sizeof(buffer) - 1) {
      len = sizeof(buffer) - 1;
    }
    strncpy(buffer, defaultValue, len);
    buffer[len] = '\0';
  }
  
  return buffer;
}

void AbortHandler(int signo)
{
  static const char* abortMessage = "\nConfig creation aborted!\n";
  ssize_t ret = write(fileno(stdout), abortMessage, strlen(abortMessage)); (void) ret;
  _exit(1);
  (void) signo;
}


char* Encrypt(const char* plain, const char* key)
{
  unsigned char ivec[XTEA_BLOCK_SIZE];
  if (XTeaGenerateIVec(ivec) < 0) { return NULL; }

  size_t plainLen = strlen(plain);
  size_t cipherSize = plainLen + XTEA_BLOCK_SIZE;
  char* cipher = malloc(cipherSize);
  if (!cipher) { return NULL; }
  
  ssize_t len = XTeaEncryptCBC((unsigned char*) plain, strlen(plain), 
                               (unsigned char*) cipher, cipherSize, 
                               ivec, (unsigned char*)key);
                               
  if (len < 0) {
    free(cipher);
    return NULL;
  }
  
  size_t encodedSize = len * 2 + 1;
  char* encoded = malloc(encodedSize);
  if (!encoded) {
    free(cipher);
    return NULL;
  }
  
  len = HexEncode(cipher, len, encoded, encodedSize);
  if (len < 0) {
    free(cipher);
    free(encoded);
    return NULL;
  }
  
  free(cipher);  
  return encoded;
}

bool SaveHeader(const char* buffer)
{
  FILE* fp = fopen(CONF_HEADER, "w");
  if (!fp) return false;
  
  fprintf(fp, "#define CONF_EMBEDDED \"%s\"\n", buffer);
  
  bool okay = !ferror(fp);
  fclose(fp);
  
  return okay;
}

int main(int argc, char** argv)
{
  enum Stage { 
    StageListenIP,
    StageListenPort,
    StageRemoteIP,
    StageRemotePort,
    StageIdnt,
    StageIdentTimeout,
    StageIdleTimeout,
    StageWriteTimeout,
    StageDnsLookup,
    StagePidFile,
    StageWelcomeMsg,
    StagePassword
    
  } stage = StageListenIP;
  
  enum Error {
    ErrorNone,
    ErrorDefault,
    ErrorValue,
    ErrorStrdup
  };

  signal(SIGINT, AbortHandler);
  
  struct Config* c = Config_New();
  if (!c) {
    fprintf(stderr, "Unable to create config: %s\n", strerror(errno));
    return 1;
  }

  char key[17] = { 0 };
  bool done = false;
  enum Error error;
  while (!done) {
    
    error = ErrorNone;
    switch (stage) {
      case StageListenIP :
      {
        char* value = Input("Listen ip", "0.0.0.0");
        if (ValidIP(value)) {
          c->listenIP = strdup(value);
          if (!c->listenIP) { error = ErrorStrdup; }
        }
        else {
          error = ErrorValue;
        }
        break;
      }
      case StageListenPort :
      {
        char* value = Input("Listen port", NULL);
        if (*value != '\0') {
          if (StrToInt(value, &c->listenPort) != 1 || !ValidPort(c->listenPort)) {
            error = ErrorValue;
          }
        }
        else {
          error = ErrorDefault;
        }
        break;
      }
      case StageRemoteIP :
      {
        char* value = Input("Remote ip", NULL);
        if (*value != '\0') {
          if (ValidIP(value)) {
            c->remoteIP = strdup(value);
            if (!c->remoteIP) {
              error = ErrorStrdup;
            }
          }
          else {
            error = ErrorValue;
          }
        }
        else {
          error = ErrorDefault;
        }
        break;
      }
      case StageRemotePort :
      {
        char* value = Input("Remote port", NULL);
        if (*value != '\0') {
          if (StrToInt(value, &c->remotePort) != 1 || !ValidPort(c->remotePort)) {
            error = ErrorValue;
          }
        }
        else {
          error = ErrorDefault;
        }
        break;
      }
      case StageIdnt :
      {
        char* value = Input("Send idnt", "true");
        if (!strcasecmp(value, "true")) {
          c->idnt = true;
        }
        else if (!strcasecmp(value, "false")) {
          c->idnt = false;
        }
        else {
          error = ErrorValue;
        }
        break;
      }
      case StageIdentTimeout :
      {
        if (!c->idnt) { break; }
        
        char* value = Input("Ident timeout", "10");
        if (*value != '\0') {
          if (StrToInt(value, &c->identTimeout) != 1 || c->identTimeout < 0) {
            error = ErrorValue;
          }
        }
        else {
          error = ErrorDefault;
        }
        break;
      }
      case StageIdleTimeout :
      {
        if (!c->idnt) { break; }
        
        char* value = Input("Idle timeout", "0");
        if (*value != '\0') {
          if (StrToInt(value, &c->idleTimeout) != 1 || c->idleTimeout < 0) {
            error = ErrorValue;
          }
        }
        else {
          error = ErrorDefault;
        }
        break;
      }
      case StageWriteTimeout :
      {
        char* value = Input("Write timeout", "30");
        if (*value != '\0') {
          if (StrToInt(value, &c->writeTimeout) != 1 || c->writeTimeout < 0) {
            error = ErrorValue;
          }
        }
        else {
          error = ErrorDefault;
        }
        break;
      }
      case StageDnsLookup :
      {
        char* value = Input("Dns lookup", "true");
        if (!strcasecmp(value, "true")) {
          c->dnsLookup = true;
        }
        else if (!strcasecmp(value, "false")) {
          c->dnsLookup = false;
        }
        else {
          error = ErrorValue;
        }
        break;
      }
      case StagePidFile :
      {
        char* value = Input("Pid file path", "none");
        if (strcmp(value, "none")) {
          c->pidFile = strdup(value);
          if (!c->pidFile) { error = ErrorStrdup; }
        }
        break;
      }
      case StageWelcomeMsg :
      {
        char* value = Input("Welcome message", "none");
        if (strcmp(value, "none")) {
          c->welcomeMsg = strdup(value);
          if (!c->welcomeMsg) { error = ErrorStrdup; }
        }
        break;
      }
      case StagePassword :
      {
        char* value = Input("Password", NULL);
        if (*value != '\0') {
          strncpy(key, value, sizeof(key) - 1);
        }
        else {
          error = ErrorDefault;
        }
        break;
      }
      default :
      {
          done = true;
          break;
      }
    }
    
    
    switch (error) {
      case ErrorNone :
      {
        stage++;
        break;
      }
      case ErrorStrdup :
      {
        perror("strdup");
        done = true;
        break;
      }
      case ErrorValue :
      {
        fprintf(stderr, "Invalid value.\n");
        break;
      }
      case ErrorDefault :
      {
        fprintf(stderr, "No default value.\n");
        break;
      }
    }
  }

  if (error != ErrorNone) { 
    Config_Free(&c);
    return 1;
  }
  
  printf("Serializing config to buffer ..\n");
  
  char* plain = Config_SaveBuffer(c);
  Config_Free(&c);
  
  if (!plain) {
    fprintf(stderr, "Failed to serialize config to buffer: %s\n", strerror(errno));
    return 1;
  }
  
  printf("Encrypting buffer with xtea cipher ..\n");
  
  char* cipher = Encrypt(plain, key);
  if (!cipher) {
    fprintf(stderr, "Error while encrypting buffer.\n");
    free(plain);
    return 1;
  }
  
  printf("Saving config ..\n");

  if (!SaveHeader(cipher)) {
    fprintf(stderr, "Error while saving %s.\n", CONF_HEADER);
    free(cipher);
    free(plain);
    return 1;
  }
  
  free(cipher);
  free(plain);

  printf("Successfully saved encrypted config!\n");
    
  return 0;
  
  (void) argc;
  (void) argv;
}
