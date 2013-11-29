/*
 * TCPSockets
 * Copyright (C) Aaron Hebert 2013 - Present <aaron.hebert@gmail.com>
 *
 * TCPSockets is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * TCPSockets is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TCP_SOCKETS_H
#define	TCP_SOCKETS_H

#include "tcp_server.h"

class TCPSocket : public tcp_server {
public:
    
    TCPSocket() : tcp_server() {
    }
};

#endif	/* TCP_SOCKETS_H */

