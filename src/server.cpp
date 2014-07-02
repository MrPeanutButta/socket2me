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

#include <vector>
#include <syslog.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/tcp.h>
#include <system_error>
#include "server.h"
#include "tcp.h"

namespace log2 {
    namespace tcp {
        extern char EOL;
        connection_threads server::connections;
        bool server::kill_ = false;
        read_handler server::my_reader = nullptr;
        int server::max_conn_buffered = 5;
        unsigned char server::md5_auth_hash_[MD5_HASH_SIZE];
        auth server::srv_auth_type_;

        server::server() : client() {
            my_connection = &server::connection_loop;
            server_ = nullptr;
        }

        server::server(const server& orig) {
        }

        server::~server() {
            server::kill_ = true;
            server::connections.clear();
        }

        /** Create TCP socket listener.
         *
         * creates socket and binds.
         */
        bool server::listen(const std::string host,
                const std::string port) {

            get_addr_info(host, port);
            this->bind();

            if (rp == nullptr) return false;

            this->server_.reset(new std::thread(
                    &server::listen_loop,
                    socket_,
                    *rp,
                    this->my_connection));

            this->server_->detach();

            freeaddrinfo(results);

            return true;
        }

        /** Binds listener to interface.
         */
        void server::bind(void) {

            for (rp = results; rp != nullptr; rp = rp->ai_next) {
                socket_ = ::socket(rp->ai_family, rp->ai_socktype,
                        rp->ai_protocol);
                if (socket_ == -1)
                    continue;

                int option = 1;
                setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR,
                        (char *) &option, sizeof (option));

                if (::bind(socket_, rp->ai_addr, rp->ai_addrlen) == 0)
                    break; // success

                close(socket_);
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
        void server::listen_loop(const int &socket,
                addrinfo client_addrinfo, connection conn) {

            // client socket
            int client_socket = 0;

            // listen until parent BGP thread exits
            while (!server::kill_) {
                // listen for connection
                if (::listen(socket, server::max_conn_buffered) == -1) {
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
                    server::connections.push_back(
                            std::shared_ptr<std::thread>(new std::thread(
                            conn,
                            nullptr, client_socket)));

                    server::connections.back()->detach();

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
        void server::connection_loop(std::thread *connection_thread, int client_socket) {

            // socket file streams
            full_duplex duplex;

            // open stream for write
            if (nullptr == (duplex.tx = fdopen(client_socket, "w"))) {
                close(client_socket);
                syslog(LOG_DEBUG, "unable to open TX stream");
                throw std::system_error(errno, std::system_category());
            }

            // open stream for read
            if (nullptr == (duplex.rx = fdopen(client_socket, "r"))) {
                close(client_socket);
                syslog(LOG_DEBUG, "unable to open RX stream");
                throw std::system_error(errno, std::system_category());
            }

            if (server::authorized(&duplex)) {

                std::string _cmd_return;
                std::string stream;

                // while connected and BGP still running.
                // feof() should be safe for local socket.
                while (!feof(duplex.rx) && !server::kill_) {

                    // get chars from stream
                    do {
                        stream += fgetc(duplex.rx);
                    } while (stream.back() != tcp::EOL);

                    // EOF == disconnect
                    if (stream.back() == EOF) break;

                    if (stream.size() > 0) {

                        // call read handler.
                        if (my_reader != nullptr) {
                            _cmd_return = server::my_reader(stream);
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
                            fwrite(_cmd_return.c_str(), 1, _cmd_return.length(), duplex.tx);
                            // send
                            fflush(duplex.tx);
                            _cmd_return.clear();
                        }
                    }
                }
            }

            fclose(duplex.tx);
            fclose(duplex.rx);
            close(client_socket);

            if (connection_thread != nullptr) {
                delete connection_thread;
                connection_thread = nullptr;
            }
        }

        bool server::authorized(full_duplex *f_dup) {
            if (server::srv_auth_type_ == auth::OFF) return true;

            unsigned char token[MD5_HASH_SIZE];
            fread(&token, sizeof (token), 1, f_dup->rx);
            return (!memcmp(&server::md5_auth_hash_, &token, MD5_HASH_SIZE));
        }
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
//    FILE *c_tx, *c_rx = nullptr;
//
//    // open stream for write
//    if (nullptr == (c_tx = fdopen(client_socket, "w"))) {
//        close(client_socket);
//        syslog(LOG_DEBUG, "unable to open TX stream");
//        throw std::system_error(errno, std::system_category());
//    }
//
//    // open stream for read
//    if (nullptr == (c_rx = fdopen(client_socket, "r"))) {
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
//    if (rp == nullptr) return false;
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