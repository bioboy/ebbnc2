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
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.    See the
//  GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.    If not, see <http://www.gnu.org/licenses/>.
//

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include "xtea.h"

#define XTEA_DELTA            0x9E3779B9
#define XTEA_NUM_ROUNDS     32

void XTeaEncrypt(const unsigned char src[XTEA_BLOCK_SIZE],
                 unsigned char dst[XTEA_BLOCK_SIZE],
                 const unsigned char ckey[XTEA_KEY_SIZE])
{
    const uint32_t* s = (const uint32_t*)src;
    uint32_t s0 = s[0];
    uint32_t s1 = s[1];
    uint32_t sum = 0;
    uint32_t* key = (uint32_t*)ckey;
    uint32_t* d = (uint32_t*)dst;

    unsigned int i;
    for (i = 0; i < XTEA_NUM_ROUNDS; i++) {
        s0 += (((s1 << 4) ^ (s1 >> 5)) + s1) ^ (sum + key[sum & 3]);
        sum += XTEA_DELTA;
        s1 += (((s0 << 4) ^ (s0 >> 5)) + s0) ^ (sum + key[(sum >> 11) & 3]);
    }

    d[0] = s0;
    d[1] = s1;
}

ssize_t XTeaEncryptECB(const unsigned char* src, size_t srcLen,
                       unsigned char* dst, size_t dstSize,
                       const unsigned char* key)
{
    ssize_t remaining = srcLen;
    ssize_t dstLen = srcLen;

    if (dstSize < srcLen) { return -1; }

    while (remaining >= XTEA_BLOCK_SIZE) {
        XTeaEncrypt(src, dst, key);
        src += XTEA_BLOCK_SIZE;
        remaining -= XTEA_BLOCK_SIZE;
        dst += XTEA_BLOCK_SIZE;
    }

    unsigned char last[XTEA_BLOCK_SIZE];
    unsigned int padding;
    if (remaining > 0) {
        padding = XTEA_BLOCK_SIZE - remaining;
        memcpy(last, src, remaining);
    }
    else if (dstSize >= srcLen + XTEA_BLOCK_SIZE) {
        padding = XTEA_BLOCK_SIZE;
    }
    else {
        return -1;
    }

    unsigned char pad = '0' + padding;
    dstLen += padding;
    memset(last + remaining, pad, padding);
    XTeaEncrypt(last, dst, key);

    return dstLen;
}

ssize_t XTeaEncryptCBC(const unsigned char* src, size_t srcLen,
                       unsigned char* dst, size_t dstSize,
                       const unsigned char ivec[XTEA_BLOCK_SIZE],
                       const unsigned char key[XTEA_KEY_SIZE])
{
    ssize_t remaining = srcLen;
    ssize_t dstLen = srcLen;

    if (dstSize < srcLen) { return -1; }

    unsigned char block[XTEA_BLOCK_SIZE];
    unsigned char iv[XTEA_BLOCK_SIZE];
    memcpy(iv, ivec, sizeof(iv));
    while (remaining >= XTEA_BLOCK_SIZE) {

        memcpy(block, src, XTEA_BLOCK_SIZE);
        unsigned int i;
        for (i = 0; i < XTEA_BLOCK_SIZE; ++i) {
            block[i] = (unsigned char)block[i] ^ iv[i];
        }

        XTeaEncrypt(block, dst, key);
        memcpy(iv, dst, XTEA_BLOCK_SIZE);
        src += XTEA_BLOCK_SIZE;
        remaining -= XTEA_BLOCK_SIZE;
        dst += XTEA_BLOCK_SIZE;
    }

    unsigned int padding;
    if (remaining > 0) {
        padding = XTEA_BLOCK_SIZE - remaining;
        memcpy(block, src, remaining);
    }
    else if (dstSize >= srcLen + XTEA_BLOCK_SIZE) {
        padding = XTEA_BLOCK_SIZE;
    }
    else {
        return -1;
    }

    unsigned char pad = '0' + padding;
    memset(block + remaining, pad, padding);
    dstLen += padding;
    unsigned int i;
    for (i = 0; i < XTEA_BLOCK_SIZE; ++i) {
        block[i] = (unsigned char)block[i] ^ iv[i];
    }
    XTeaEncrypt(block, dst, key);

    return dstLen;
}

void XTeaDecrypt(const unsigned char src[XTEA_BLOCK_SIZE],
                 unsigned char dst[XTEA_BLOCK_SIZE],
                 const unsigned char ckey[XTEA_KEY_SIZE])
{
    const uint32_t* s = (const uint32_t*)src;
    uint32_t s0 = s[0];
    uint32_t s1 = s[1];
    uint32_t sum = XTEA_DELTA * XTEA_NUM_ROUNDS;
    uint32_t* key = (uint32_t*)ckey;
    uint32_t* d = (uint32_t*)dst;

    unsigned int i;
    for (i = 0; i < XTEA_NUM_ROUNDS; i++) {
        s1 -= (((s0 << 4) ^ (s0 >> 5)) + s0) ^ (sum + key[(sum >> 11) & 3]);
        sum -= XTEA_DELTA;
        s0 -= (((s1 << 4) ^ (s1 >> 5)) + s1) ^ (sum + key[sum & 3]);
    }

    d[0] = s0;
    d[1] = s1;
}

ssize_t XTeaDecryptECB(const unsigned char* src, size_t srcLen,
                       unsigned char* dst, size_t dstSize,
                       const unsigned char* key)
{
    ssize_t remaining = srcLen;
    ssize_t dstLen = srcLen;

    if (dstSize < srcLen || srcLen % XTEA_BLOCK_SIZE != 0) { return -1; }

    while (remaining > 0) {
        XTeaDecrypt(src, dst, key);
        src += XTEA_BLOCK_SIZE;
        remaining -= XTEA_BLOCK_SIZE;
        dst += XTEA_BLOCK_SIZE;
    }

    int padding = *(dst - 1) - '0';
    if (padding < 1 || (unsigned int)padding > XTEA_BLOCK_SIZE) { return -1; }

    return dstLen - padding;
}

ssize_t XTeaDecryptCBC(const unsigned char* src, size_t srcLen,
                       unsigned char* dst, size_t dstSize,
                       const unsigned char ivec[XTEA_BLOCK_SIZE],
                       const unsigned char key[XTEA_KEY_SIZE])
{
    ssize_t remaining = srcLen;
    ssize_t dstLen = srcLen;

    if (dstSize < srcLen || srcLen % XTEA_BLOCK_SIZE != 0) { return -1; }

    unsigned char temp[XTEA_BLOCK_SIZE];
    unsigned char iv[XTEA_BLOCK_SIZE];
    memcpy(iv, ivec, sizeof(ivec));
    while (remaining > 0) {

        memcpy(temp, src, XTEA_BLOCK_SIZE);
        XTeaDecrypt(src, dst, key);

        unsigned int i;
        for (i = 0; i < XTEA_BLOCK_SIZE; ++i) {
            dst[i] = (unsigned char)dst[i] ^ iv[i];
        }

        src += XTEA_BLOCK_SIZE;
        remaining -= XTEA_BLOCK_SIZE;
        dst += XTEA_BLOCK_SIZE;
        memcpy(iv, temp, XTEA_BLOCK_SIZE);
    }

    int padding = *(dst - 1) - '0';
    if (padding < 1 || (unsigned int)padding > XTEA_BLOCK_SIZE) { return -1; }

    return dstLen - padding;
}

int XTeaGenerateIVec(unsigned char ivec[XTEA_BLOCK_SIZE])
{
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) { return -1; }

    ssize_t ret = read(fd, ivec, sizeof(ivec));
    close(fd);

    return ret == sizeof(ivec) ? 0 : -1;
}
