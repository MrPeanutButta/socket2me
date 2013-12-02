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

#include <netdb.h>
#include <syslog.h>
#include <cstring>
#include <unistd.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include "tcp_client.h"

// defaults buffer sizes.

enum peer_buffer {
    DEFAULT_TCP_BUFFER_SIZE = 4096
};

tcp_client::tcp_client() : tcp_socket(0),
tcp_rx_socket_buffer_size(DEFAULT_TCP_BUFFER_SIZE),
tcp_tx_socket_buffer_size(DEFAULT_TCP_BUFFER_SIZE){

    rp = nullptr;
    results = nullptr;
    tx = nullptr;
    rx = nullptr;
    tcp_client_reset();
}

tcp_client::tcp_client(const tcp_client& orig) {
}

tcp_client::~tcp_client() {
    tcp_client_reset();
}

void tcp_client::tcp_client_reset(void) {
    if (tcp_socket)close(tcp_socket);
    memset(&hints, 0, sizeof (addrinfo));
}
/** Sets server info.
 *
 * sets server info for the server we are connecting to.
 * this must be called before anything.
 */
bool tcp_client::get_addr_info(const std::string host, const std::string port) {

    hints.ai_family = AF_UNSPEC; // ipv4 or ipv6
    hints.ai_socktype = SOCK_STREAM; // tcp
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
    results = 0;

    // man getaddrinfo(3)
    int s = getaddrinfo(host.c_str(), port.c_str(), &hints, &results);

    if (s != 0) {
        syslog(LOG_DEBUG, "get_addr_info: %s", gai_strerror(s));
        return false;
    }

    return true;
}
/** Creates socket and connects.
 *
 * creates socket and initiates tcp connection.
 */
bool tcp_client::connect(const std::string host, const std::string port) {

    if (!get_addr_info(host, port)) return false;
    if (!results) return false;

    bool rc(false);

    // find a suitable interface
    for (rp = results; rp != NULL; rp = rp->ai_next) {
        tcp_socket = socket(rp->ai_family, rp->ai_socktype,
                rp->ai_protocol);
        if (tcp_socket == -1)
            continue;

        // attempt to connect
        if (::connect(tcp_socket, rp->ai_addr, rp->ai_addrlen) != -1) {

            // set options
            int option = 1;
            setsockopt(tcp_socket, SOL_SOCKET, SO_REUSEADDR,
                    (char *) &option, sizeof (option));

            rc = true;

            syslog(LOG_DEBUG, "connection to %s OK", host.c_str());

            // success
            break;
        }

        close(tcp_socket);
    }

    if (rp == NULL) {
        syslog(LOG_DEBUG, "unable to allocate interface to destination host");
        return false;
    }

    // free results
    freeaddrinfo(results);

    // open stream for write
    if (NULL == (this->tx = fdopen(tcp_socket, "w"))) {
        close(tcp_socket);
        syslog(LOG_DEBUG, "unable to open TX stream");
    } else rc = true;

    // open stream for read
    if (NULL == (this->rx = fdopen(tcp_socket, "r"))) {
        close(tcp_socket);
        syslog(LOG_DEBUG, "unable to open RX stream");
    } else rc = true;

    return rc;
}

void tcp_client::disconnect(void) {
    close(tcp_socket);
    if (tx)fclose(tx);
    if (rx)fclose(rx);

    rx = nullptr;
    tx = nullptr;
}

/** Resize tx socket buffer size.
 */
bool tcp_client::set_tcp_tx_buffer_size(const size_t &size) {

    tcp_tx_socket_buffer_size = size;
    bool ret_val(false);
    int option(size);

    ret_val = !setsockopt(tcp_socket, SOL_SOCKET, SO_SNDBUF,
            (char *) &option, sizeof (option));

    return ret_val;
}

/** Resize rx socket buffer size.
 */
bool tcp_client::set_tcp_rx_buffer_size(const size_t &size) {

    tcp_rx_socket_buffer_size = size;
    bool ret_val = false;
    int option = size;

    ret_val = !setsockopt(tcp_socket, SOL_SOCKET, SO_RCVBUF,
            (char *) &option, sizeof (option));

    return ret_val;
}

/** Return reference to current TCP socket rx buffer size.
 */
int tcp_client::get_tcp_rx_buffer_size(void) {

    int sockbufsize = 0;
    socklen_t size = sizeof (int);

    int ret_val = getsockopt(this->tcp_socket, SOL_SOCKET, SO_RCVBUF,
            (char *) &sockbufsize, &size);

    if (ret_val < 0)
        perror("getsockopt");

    return sockbufsize;
}

/** Return reference to current TCP socket tx buffer size.
 */
int tcp_client::get_tcp_tx_buffer_size(void) {

    int sockbufsize = 0;
    socklen_t size = sizeof (int);

    int ret_val = getsockopt(this->tcp_socket, SOL_SOCKET, SO_SNDBUF,
            (char *) &sockbufsize, &size);

    if (ret_val < 0)
        perror("getsockopt");

    return sockbufsize;
}

/** Check for stream == 'nullptr' and socket fd == 0.
 *
 * if either FILE stream == nullptr return false.
 * if socket == 0 return false.
 *
 * testing feof() will cause deadlock if
 * socket is no longer valid causing the thread to hang.
 * using feof_unlocked() to remedy locking; this may require
 * mutex but i believe that this is called from a single thread.
 */
bool tcp_client::connected(void) {

    if (this->rx == nullptr) return false;
    if (this->tx == nullptr) return false;
    if (this->tcp_socket <= 0) return false;

    if (feof_unlocked(this->rx)) return false;
    if (feof_unlocked(this->tx)) return false;

    return true;
}

/** Read from rx stream.
 *
 * calls fread() on rx stream.
 */
size_t tcp_client::read(void *data, const size_t &size,
        const size_t &count) {

    if (!connected()) {
        syslog(LOG_DEBUG, "can not read, no stream available");
        return 0;
    }

    return fread(data, size, count, this->rx);
}

std::string tcp_client::readline(void) {
    std::string read_string;

    do {
        read_string += this->read_char();
    } while (read_string.back() != '\n');

    return read_string;
}

/** Read single char from rx stream.
 */
char tcp_client::read_char(void) {

    if (!connected()) {
        syslog(LOG_DEBUG, "can not read, no stream available");
        return 0;
    }

    return fgetc(this->rx);
}

/** Write tx stream.
 *
 * calls fwrite() on tx stream.
 */
size_t tcp_client::write(void *data, const size_t &size,
        const size_t &count) {

    if (!connected()) {
        syslog(LOG_DEBUG, "can not write, no stream available");
        return 0;
    }

    this->lock_write();
    size_t write_ = fwrite(data, size, count, this->tx);
    this->unlock_write();

    return write_;
}

/** Write tx stream.
 *
 * calls fwrite() on tx stream.
 */
size_t tcp_client::write(std::string data) {

    if (!connected()) {
        syslog(LOG_DEBUG, "can not write, no stream available");
        return 0;
    }

    this->lock_write();
    size_t write_ = fwrite(data.c_str(), sizeof (char), data.length(), this->tx);
    this->unlock_write();

    return write_;
}


/** Format data to tx stream.
 *
 * calls fprintf() on tx stream.
 */
size_t tcp_client::write_formated(const char *format, const int &a,
        const int &b) {

    if (!connected()) {
        syslog(LOG_DEBUG, "can not write_formated, no stream available");
        return 0;
    }

    this->lock_write();
    int write_ = fprintf(this->tx, format, a, b);
    this->unlock_write();

    return write_;
}

/** Flush (send data and clear buffer) tx stream.
 *
 * calls fflush() on tx stream to send data
 * and empty buffer.
 */
int tcp_client::tx_flush(void) {
    this->lock_write();
    int rc = fflush(this->tx);
    this->unlock_write();

    return rc;
}
/** Flush (clear buffer) rx stream.
 *
 * calls fflush() on rx stream to send data
 * and empty buffer.
 */
int tcp_client::rx_flush(void) {
    return fflush(this->rx);
}