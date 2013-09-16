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

#ifndef EBBNC_IDENT_H
#define EBBNC_IDENT_H

#include <arpa/inet.h>
#include <time.h>
#include <stdbool.h>
#include "misc.h"

#define IDENT_LEN 256

bool identLookup(const struct sockaddr_any* localAddr,
                 const struct sockaddr_any* peerAddr,
                 time_t timeout, char* user);

#endif
