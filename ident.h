//
//  Copyright (C) 2012, 2013 ebftpd team
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

#ifndef __EBBNC_IDENT_H
#define __EBBNC_IDENT_H

#include <arpa/inet.h>
#include <time.h>
#include <stdbool.h>

#define IDENT_LEN 256

bool IdentLookup(const struct sockaddr_storage* localAddr, 
                 const struct sockaddr_storage* peerAddr, 
                 time_t timeout, char* user);

#endif
