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

#ifndef TCP_SERVER_H
#define	TCP_SERVER_H
#include <thread>
#include <vector>
#include "tcp_client.h"

typedef std::vector<std::thread *> connection_threads;

/* function pointer to call to handle the
 * actual connection. either is API or CLI */
typedef void (*connection)(std::thread *, const int);

/* function pointer to stream reader.
 * the string that is returned from the reader
 * is written back to the stream. if empty string
 * nothing is sent.
 */
typedef std::string(*read_handler)(std::string);

class tcp_server : public tcp_client {
public:

    tcp_server();
    tcp_server(const tcp_server& orig);
    virtual ~tcp_server() = 0;

    // listens for incomming connections
    bool listen_for_connections(const std::string host,
            const std::string port);

    /* sets function to call when a new
     * connection is established */
    void set_conn_handler(connection conn) {
        this->my_connection = conn;
    }

    /* sets function to call when data is
     * read form the stream */
    void set_read_handler(read_handler reader) {
        tcp_server::my_reader = reader;
    }

    // container of connection threads
    static connection_threads server_connections;

    // kill all listeners and existing connections
    void kill_service(void) {
        tcp_server::kill = true;
    }

    void set_EOL_char(char c) {
        tcp_server::EOL_char = c;
    }

    // sets max number of connections we intend to buffer
    void set_max_conn_buffer(const int conns) {
        tcp_server::max_conn_buffered = conns;
    }

private:

    std::thread *server;
    static bool kill;
    connection my_connection;

    static read_handler my_reader;
    static int max_conn_buffered;
    static char EOL_char;

    void bind(void);

    /* listens for incomming connections and
     * calls the connections handler */
    static void listen_loop(const int &, addrinfo, connection con);

    // default connection handler
    static void connection_loop(std::thread *, const int);
};

#endif	/* TCP_SERVER_H */

