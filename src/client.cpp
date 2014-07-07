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

#include <algorithm>
#include "client.h"

namespace tcp {
    //extern class ip_endpoint;

    client::client(std::string key, auth auth_) :
    socket(key, auth_) {

    }

    bool client::authenticate(std::string host, std::string port) {
        if (auth_type_ == auth::OFF) return false;
        if (md5_key_.empty()) return false;
        if (md5_hash_.get() == nullptr) return false;

        if (redundent_conns.empty()) {
            add_failover(host, port);
            this->ip_endpoint(redundent_conns.back());
        }

        this->connect(host, port);
        if (this->connected()) {
            this->write(md5_hash_.get(), MD5_HASH_SIZE);
            this->send();
        } else {
            return false;
        }

        switch (this->read8()) {
            case (int) auth_status::AUTH_OK:
                // reset to real active
                return true;
                break;
            case (int) auth_status::AUTH_FAILED:
                // reset to real active
                this->disconnect();
                return false;
                break;
            default: break;
        }

        return false;
    }

    void client::add_failover(std::string host, std::string port) {

        ip_point *p_ipend = new ip_point();
        p_ipend->host = host;
        p_ipend->port = port;

        redundent_conns.push_back(std::make_shared<ip_point>(*p_ipend));

    }

    /**
     * Trigger a failover scenario
     * @return true if connection recovers flase
     */
    bool client::failover(void) {
        this->disconnect();
        
        do {
            for (auto &con : redundent_conns) {
                this->ip_endpoint(con);
                if (authenticate(con->host, con->port)) {
                    return connected();
                }
            }
            
            /* This may cause an infinite loop if,
             * if left disconnected.
             * The benefits of this would be that 
             * the daemon will come back up once
             * a good connection is established.
             * There needs to be a more controlled 
             * break here */
        } while (!connected());

        // not reached
        return connected();
    }
}