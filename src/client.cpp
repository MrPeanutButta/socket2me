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
    socket(key, auth_), active_connection(0) {

    }

    bool client::authenticate(std::string host, std::string port) {
        if (auth_type == auth::OFF) return false;
        if (md5_key.empty()) return false;
        if (md5_hash.get() == nullptr) return false;

        std::hash<std::string> h_;
        std::size_t index = h_(host + port);

        connection_hashkey::iterator it_ = std::find
                (hashkey_conns.begin(),hashkey_conns.end(), index);

        if (it_ == hashkey_conns.end()) {
            add_connection(index);
        }

        this->set_IPendpoint(redundent_conns[index]);
        this->connect(host, port);
        if (this->connected()) {
            this->write(md5_hash.get(), MD5_HASH_SIZE);
            this->send();
        } else {
            return false;
        }

        switch (this->read8()) {
            case (int) auth_status::AUTH_OK:
                // reset to real active
                this->set_IPendpoint(redundent_conns[active_connection]);
                return true;
                break;
            case (int) auth_status::AUTH_FAILED:
                // reset to real active
                this->set_IPendpoint(redundent_conns[active_connection]);
                this->disconnect();
                return false;
                break;
            default: break;
        }

        return false;
    }

    void client::add_connection(std::size_t index) {
        bool first_conn = redundent_conns.empty();
        
        if (first_conn) {
            active_connection = index;
        }

        ip_endpoint *p_ipend = new ip_endpoint();
        hashkey_conns.push_back(index);
        redundent_conns[index] = std::make_shared<ip_endpoint>(*p_ipend);
         
        if(first_conn){
            this->set_IPendpoint(redundent_conns[active_connection]);
        }
    }

    bool client::failover(void) {
        for (auto &con : redundent_conns) {
            if (con.second->connected()) {
                active_connection = con.first;
                this->set_IPendpoint(redundent_conns[active_connection]);
                break;
            }
        }

        return connected();
    }
}