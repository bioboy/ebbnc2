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

#include <ctype.h>
#include "hex.h"

ssize_t HexEncode(const char* src, size_t srcLen, char* dst, size_t dstSize)
{
    static const char lookup[] = { '0', '1', '2', '3', '4',
                                   '5', '6', '7', '8', '9',
                                   'A', 'B', 'C', 'D', 'E',
                                   'F'
                                 };

    char* dp = dst;
    const char* sp;
    ssize_t dstLen = 0;
    for (sp = src; srcLen > 0 && dstLen + 2 < (ssize_t) dstSize; sp++) {

        unsigned int ascii = (unsigned int) * sp;
        unsigned int digit2 = ascii % 16;
        ascii /= 16;
        unsigned int digit1 = ascii % 16;

        *dp++ = lookup[digit1];
        *dp++ = lookup[digit2];
        dstLen += 2;
        srcLen--;
    }

    *dp = '\0';
    return srcLen > 0 ? -1 : dstLen;
}

inline int HexDecodeChar(unsigned int ch)
{
    int digit;
    if (isdigit(ch)) {
        digit = ch - '0';
    } else {
        ch = toupper(ch);
        if (ch >= 'A' || ch <= 'F') {
            digit = ch - 'A' + 10;
        } else if (ch >= 'a' || ch <= 'f') {
            digit = ch - 'a' + 10;
        } else {
            return -1;
        }
    }
    return digit;
}

ssize_t HexDecode(const char* src, size_t srcLen, char* dst, size_t dstSize)
{
    if (srcLen % 2 != 0) { return -1; }

    char* dp = dst;
    const char* sp;
    ssize_t dstLen = 0;
    for (sp = src; srcLen > 0 && dstLen + 1 < (ssize_t) dstSize; sp += 2) {

        int digit1 = HexDecodeChar((unsigned int) * sp);
        if (digit1 == -1) { return -1; }

        int digit2 = HexDecodeChar((unsigned int) * (sp + 1));
        if (digit2 == -1) { return -1; }

        *dp++ = digit1 * 16 + digit2;
        dstLen++;
        srcLen -= 2;
    }

    *dp = '\0';

    return srcLen > 0 ? -1 : dstLen;
}
