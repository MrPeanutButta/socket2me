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

#ifndef TCP_SERVER_H
#define	TCP_SERVER_H

#include <thread>
#include <vector>
#include <string>
#include <memory>
#include "tcp.h"

namespace tcp {

    typedef std::vector<std::shared_ptr<std::thread>> connection_threads;

    /* function pointer to call to handle the
     * actual connection. either is API or CLI */
    typedef void (*connection)(std::thread *, const int);

    /* function pointer to stream reader.
     * the string that is returned from the reader
     * is written back to the stream. if empty string
     * nothing is sent.
     */
    typedef std::string(*read_handler)(std::string);

    class server : public socket {
    public:

        server(std::string key = "", auth auth_ = tcp::auth::OFF);
        virtual ~server();

        // listens for incomming connections
        bool listen(const std::string host,
                const std::string port);

        /* sets function to call when a new
         * connection is established */
        void set_conn_handler(connection conn) {
            this->my_connection = conn;
        }

        /* sets function to call when data is
         * read form the stream */
        void set_read_callback(read_handler reader) {
            server::my_reader = reader;
        }

        unsigned char *md5_auth_hash(void) {
            return md5_auth_hash_;
        }

        // sets max number of connections we intend to buffer

        void set_max_conn_buffer(const int conns) {
            server::max_conn_buffered = conns;
        }

        void kill(void) {
            server::kill_ = true;
        }

    private:

        std::unique_ptr<std::thread> server_;
        static bool kill_;
        connection my_connection;

        static read_handler my_reader;
        static int max_conn_buffered;
        static connection_threads connections;

        static unsigned char md5_auth_hash_[MD5_HASH_SIZE];
        static auth srv_auth_type_;

        void bind(void);

        /* listens for incomming connections and
         * calls the connections handler */
        static void listen_loop(const int &, addrinfo, connection con);

        // default connection handler
        static void connection_loop(std::thread *, const int);

        static bool authorized(ip_endpoint &);
    };
}

#endif	/* TCP_SERVER_H */

