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

#ifndef EBBNC_XTEA_H
#define EBBNC_XTEA_H

#include <stdio.h>

#define XTEA_BLOCK_SIZE     8
#define XTEA_KEY_SIZE       16

ssize_t XTeaEncryptECB(const unsigned char* src, size_t srcLen,
                       unsigned char* dst, size_t dstSize,
                       const unsigned char* key);
ssize_t XTeaDecryptECB(const unsigned char* src, size_t srcLen,
                       unsigned char* dst, size_t dstSize,
                       const unsigned char* key);

int XTeaGenerateIVec(unsigned char ivec[XTEA_BLOCK_SIZE]);
ssize_t XTeaEncryptCBC(const unsigned char* src, size_t srcLen,
                       unsigned char* dst, size_t dstSize,
                       const unsigned char ivec[XTEA_BLOCK_SIZE],
                       const unsigned char key[XTEA_KEY_SIZE]);
ssize_t XTeaDecryptCBC(const unsigned char* src, size_t srcLen,
                       unsigned char* dst, size_t dstSize,
                       const unsigned char ivec[XTEA_BLOCK_SIZE],
                       const unsigned char key[XTEA_KEY_SIZE]);

#endif
