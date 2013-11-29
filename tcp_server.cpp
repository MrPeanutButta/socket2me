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

#include <vector>
#include <syslog.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/tcp.h>
#include <system_error>
#include "tcp_server.h"

connection_threads tcp_server::server_connections;
bool tcp_server::kill = false;
read_handler tcp_server::my_reader = nullptr;
char tcp_server::EOL_char = '\n';
int tcp_server::max_conn_buffered = 5;

tcp_server::tcp_server() : tcp_client() {
    my_connection = &tcp_server::connection_loop;
    server = nullptr;
}

tcp_server::tcp_server(const tcp_server& orig) {
}

tcp_server::~tcp_server() {
    if (server != nullptr) {
        delete server;
        server = nullptr;
    }

    kill_service();

    tcp_server::server_connections.clear();
}

/** Create TCP socket listener.
 *
 * creates socket and binds.
 */
bool tcp_server::listen_for_connections(const std::string host,
        const std::string port) {

    get_addr_info(host, port);
    this->bind();

    if (rp == NULL) return false;

    this->server = new std::thread(
            &tcp_server::listen_loop,
            tcp_socket,
            *rp,
            this->my_connection);

    this->server->detach();

    freeaddrinfo(results);

    return true;
}

/** Binds listener to interface.
 */
void tcp_server::bind(void) {

    for (rp = results; rp != NULL; rp = rp->ai_next) {
        tcp_socket = socket(rp->ai_family, rp->ai_socktype,
                rp->ai_protocol);
        if (tcp_socket == -1)
            continue;

        int option = 1;
        setsockopt(tcp_socket, SOL_SOCKET, SO_REUSEADDR,
                (char *) &option, sizeof (option));

        if (::bind(tcp_socket, rp->ai_addr, rp->ai_addrlen) == 0)
            break; // success

        close(tcp_socket);
    }

    if (rp == nullptr) {
        syslog(LOG_DEBUG, "unable to allocate interface to destination host");
        return;
    }
}

/** Listen for TCP connections.
 *
 * if successful bind, listen for new connections.
 * each new connection is a new thread.
 */
void tcp_server::listen_loop(const int &socket,
        addrinfo client_addrinfo, connection conn) {

    // client socket
    int client_socket = 0;

    // listen until parent BGP thread exits
    while (!tcp_server::kill) {
        // listen for connection
        if (::listen(socket, tcp_server::max_conn_buffered) == -1) {
            syslog(LOG_DEBUG, "unable to listen for connections");
            // failed, throw errno
            throw std::system_error(errno, std::system_category());
        }

        // accept the new connection
        client_socket = accept(socket, client_addrinfo.ai_addr,
                &client_addrinfo.ai_addrlen);

        // set options, no_delay, reuseaddr
        int option = 1;
        setsockopt(client_socket, SOL_SOCKET, TCP_NODELAY,
                (char *) &option, sizeof (option));
        setsockopt(client_socket, SOL_SOCKET, SO_REUSEADDR,
                (char *) &option, sizeof (option));

        if (client_socket == -1) {
            syslog(LOG_DEBUG, "unable to accept connection %d", errno);
            close(socket);
            close(client_socket);
            // failed, throw errno
            throw std::system_error(errno, std::system_category());
        } else {

            // success, create new thread to manage connection
            tcp_server::server_connections.push_back(nullptr);

            std::thread *__t = tcp_server::server_connections.back();

            __t = new std::thread(
                    conn,
                    __t, client_socket);

            // detach before delete
            __t->detach();
        }
    }

    close(socket);
    memset(&client_addrinfo, 0, sizeof (addrinfo));
}

/** Handle client connection.
 *
 * reads 'len' and 'command' string.
 * sends 'command' to parser.
 */
void tcp_server::connection_loop(std::thread *connection_thread, int client_socket) {

    // socket file streams
    FILE *c_tx, *c_rx = NULL;

    // open stream for write
    if (NULL == (c_tx = fdopen(client_socket, "w"))) {
        close(client_socket);
        syslog(LOG_DEBUG, "unable to open TX stream");
        throw std::system_error(errno, std::system_category());
    }

    // open stream for read
    if (NULL == (c_rx = fdopen(client_socket, "r"))) {
        close(client_socket);
        syslog(LOG_DEBUG, "unable to open RX stream");
        throw std::system_error(errno, std::system_category());
    }

    std::string _cmd_return;
    std::string stream;

    // while connected and BGP still running.
    // feof() should be safe for local socket.
    while (!feof(c_rx) && !tcp_server::kill) {

        // get chars from stream
        do {
            stream += fgetc(c_rx);
        } while (stream.back() != tcp_server::EOL_char);

        // EOF == disconnect
        if (stream.back() == EOF) break;

        if (stream.size() > 0) {

            // call read handler.
            if (my_reader != nullptr) {
                _cmd_return = tcp_server::my_reader(stream);
            } else {
                syslog(LOG_DEBUG,
                        "my_reader == nullptr, set_read_handler first");
            }

            stream.clear();

            /* write the length of ret_val first.
             * CLI is expected [len,data].
             * increased to 32bit value for larger outputs */
            if (_cmd_return.length() > 0) {
                // write data
                fwrite(_cmd_return.c_str(), 1, _cmd_return.length(), c_tx);
                // send
                fflush(c_tx);
                _cmd_return.clear();
            }
        }
    }

    fclose(c_tx);
    fclose(c_rx);
    close(client_socket);

    if (connection_thread != nullptr) {
        delete connection_thread;
        connection_thread = nullptr;
    }
}

///* conversions between wide and narrow char.
// * this came about because ruby uses wide char
// * for ALL THE THINGS, and doesn't give you an
// * easy option to use utf8 only.
// *
// * Uses 2 byte char EVEN FOR TCPSOCKETS.
// * DRIVES ME NUTS, I MEAN C'MON!!!
// */
//std::wstring to_wide_str(const std::string &s_src) {
//    return (std::wstring(s_src.begin(), s_src.end()));
//}
//
//std::string to_narrow_str(const std::wstring &ws_src) {
//    return (std::string(ws_src.begin(), ws_src.end()));
//}
//
///** Check if command is a legal API call
// * @param buffer : narrow string of the command
// * @return true if 'api-get'; false if not
// */
//bool is_api_call(const std::string &buffer) {
//
//    std::istringstream iss(buffer);
//
//    // clear flags
//    iss.clear();
//    std::vector<std::string> tokens;
//
//    // spit string by whitespace and back_insert into vector
//    std::copy(std::istream_iterator<std::string > (iss),
//            std::istream_iterator<std::string > (),
//            std::back_inserter<std::vector<std::string> >(tokens));
//
//    /* if they match, compare returns 0.
//     * we want to return !0, which == true. */
//    return !(tokens[0].compare("api-get"));
//}
//
///** Handle connections from sinatra API.
// *
// * reads 'len' and 'command' string.
// * sends 'command' to parser.
// */
//void tcp_server::sinatra_api(std::thread *connection_thread, int client_socket) {
//
//    // socket file streams
//    FILE *c_tx, *c_rx = NULL;
//
//    // open stream for write
//    if (NULL == (c_tx = fdopen(client_socket, "w"))) {
//        close(client_socket);
//        syslog(LOG_DEBUG, "unable to open TX stream");
//        throw std::system_error(errno, std::system_category());
//    }
//
//    // open stream for read
//    if (NULL == (c_rx = fdopen(client_socket, "r"))) {
//        close(client_socket);
//        syslog(LOG_DEBUG, "unable to open RX stream");
//        throw std::system_error(errno, std::system_category());
//    }
//
//    // wide char buffer
//    wchar_t wbuffer[255];
//
//    // set wide streams
//    fwide(c_rx, 1);
//    fwide(c_tx, 1);
//
//    wint_t len = 0;
//    std::string _cmd_return;
//
//    // if connected and BGP still running.
//    // feof() should be safe for local socket.
//    if (!feof(c_rx) && !parent_bgp_instance->stop_and_exit()) {
//
//        // read command
//        len = fgetwc(c_rx);
//
//        // EOF == disconnect
//        if (len == EOF) return;
//
//        // read command
//        wchar_t *dummy = fgetws(wbuffer, len + 1, c_rx);
//
//        /* need to makes this an api only command
//         * incase we are reading from a direct socket and
//         * not from sinatra */
//        if ((len > 0) && is_api_call(to_narrow_str(wbuffer))) {
//
//            /* tell BGP to parse the command, _cmd_return
//             * is used as console output */
//
//            _cmd_return = parent_bgp_instance->tcp_execute_line(to_narrow_str(wbuffer).c_str());
//
//            len = _cmd_return.length();
//
//            /* write the length of ret_val first.
//             * CLI is expected [len,data].
//             * increased to 32bit value for larger outputs */
//
//            // write data
//            fputws(to_wide_str(_cmd_return).c_str(), c_tx);
//
//            fflush(c_tx);
//            _cmd_return.clear();
//        } else {
//            /* illegal API call
//             * tell user to suck it! */
//            fputws(L"bgpd: illegal API call", c_tx);
//            // JK, we want to be nice.
//            fflush(c_tx);
//        }
//    }
//
//
//    fclose(c_tx);
//    fclose(c_rx);
//    close(client_socket);
//
//    if (connection_thread != nullptr) {
//        delete connection_thread;
//        connection_thread = nullptr;
//    }
//}
//
///** Create TCP socket listener for API.
// *
// * creates socket and binds.
// */
//bool tcp_server::listen_for_api(const char *ip, const char *port) {
//    get_addr_info(ip, port);
//    this->bind();
//
//    if (rp == NULL) return false;
//
//    this->server = new std::thread(
//            &tcp_server::listen_loop,
//            tcp_socket,
//            *rp,
//            sinatra_api);
//
//    this->server->detach();
//
//    freeaddrinfo(results);
//
//    return true;
//}