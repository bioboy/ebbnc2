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

#include "hex.h"

ssize_t HexEncode(const char* src, char* dst, size_t dstSize)
{
  ssize_t dstLen = 0;
  while (*src && dstLen + 3 <= (ssize_t) dstSize) {
    sprintf(dst, "%0X", *src);
    src++;
    dst += 2;
    dstLen += 2;
  }

  return *src ? -1 : dstLen;
}

ssize_t HexDecode(const char* src, char* dst, size_t dstSize)
{
  unsigned int ch;
  ssize_t dstLen = 0;
  while (*src && dstLen + 2 <= (ssize_t) dstSize) {
    if (sscanf(src, "%2X", &ch) != 1) {
      break;
    }
    src += 2;
    *dst++ = ch;
    dstLen++;
  }

  return *src ? -1 : dstLen;
}
