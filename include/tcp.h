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

#ifndef TCP_SOCKETS_H
#define	TCP_SOCKETS_H

#include <functional>
#include <memory>
#include <vector>
#include <cstddef>
#include <cassert>
#include <cstring>
#include "md5.h"
#include "server.h"

namespace log2 {

    /** md5 - returns message-digest
     * 
     * @param key : string to digest
     * @return string md5 of key param
     */
    unsigned char *md5(std::string key) {
        assert(!key.empty());

        MD5_CTX md5_ctx;
        MD5_Init(&md5_ctx);
        MD5_Update(&md5_ctx, (void *) key.c_str(), key.size());

        unsigned char *res = new unsigned char[MD5_HASH_SIZE];

        MD5_Final(res, &md5_ctx);

        assert(res != nullptr);

        return res;
    }

    /** hash<T> - quick template hash
     * 
     * @param key : object to hash
     * @return string for of hash
     */
    template <class T>
    std::string hash(T key) {
        std::hash<T> h_;
        return std::string(h_(key));
    }

    namespace tcp {

        extern char EOL;

        class socket : public server {
        private:
            // key and digested key
            std::string md5_key;
            std::unique_ptr<unsigned char> md5_hash;

            bool is_authed;
            auth auth_type;

            void init_md5(const std::string &key) {
                md5_key = key;
                md5_hash.reset(log2::md5(key));

                memcpy(&server::md5_auth_hash_, md5_hash.get(), MD5_HASH_SIZE);
            }

        public:

            socket(std::string key = "", auth auth_ = auth::OFF) :
            server() {

                server::srv_auth_type_ = auth_;
                auth_type = auth_;

                switch (auth_) {
                    case auth::MD5:
                        init_md5(key);
                        break;
                    default: break;
                }
            }

            bool authenticate(std::string host, std::string port) {
                if (auth_type == auth::OFF) return false;
                if (md5_key.empty()) return false;
                if (md5_hash.get() == nullptr) return false;

                this->connect(host, port);
                if (this->connected()) {
                    this->write128((uint128_t) md5_hash.get());
                    this->send();
                } else {
                    return false;
                }

                switch (this->read8()) {
                    case (int) auth_status::AUTH_OK:
                        return true;
                        break;
                    case (int) auth_status::AUTH_FAILED:
                        this->disconnect();
                        return false;
                        break;
                    default: break;
                }

                return false;
            }
        };
    }
}
#endif	/* TCP_SOCKETS_H */

