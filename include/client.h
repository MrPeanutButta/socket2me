/*
 * socket
 * Copyright (C) log2 2013 - Present <aaron.hebert@log2.co>
 *
 * log2sockets is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * log2sockets is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TCP_CLIENT_H
#define	TCP_CLIENT_H

#include <cstdint>
#include "tcp.h"

namespace tcp {

    // hash indexed redundant connection map
    typedef std::map<std::size_t, std::shared_ptr<ip_endpoint>> connections;
    typedef std::vector<std::size_t> connection_hashkey;

    class client : public socket {
    private:
        // hash of active connection
        std::size_t active_connection;
        connections redundent_conns;
        connection_hashkey hashkey_conns;

    public:

        client(std::string key = "", auth auth_ = tcp::auth::OFF);

        bool authenticate(std::string host, std::string port);
        void add_connection(std::size_t index);
        bool failover(void);
    };
}

#endif	/* TCP_CLIENT_H */

