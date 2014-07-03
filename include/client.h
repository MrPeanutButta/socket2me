/*
 * log2sockets
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

    class client : public socket {
    public:

        //client();
        //client(const client& orig);

        client(std::string key = "", auth auth_ = tcp::auth::OFF) :
        socket(key, auth_) {
        }
        // virtual ~client() = 0;


    };
}

#endif	/* TCP_CLIENT_H */

