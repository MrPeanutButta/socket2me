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

#include <netdb.h>
#include <syslog.h>
#include <cstring>
#include <unistd.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include "client.h"

namespace log2 {
    namespace tcp {
        char EOL = '\n';

        // default buffer sizes 4096
        // default lock interval 10

        client::client() : socket_(0),
        rx_buffer_size(4096),
        tx_buffer_size(4096), lock_interval(10) {

            rp = nullptr;
            results = nullptr;
            tx = nullptr;
            rx = nullptr;
            reset();
        }

        client::client(const client& orig) {
        }

        client::~client() {
            reset();
        }

        void client::reset(void) {
            if (socket_ > 0)close(socket_);
            memset(&hints, 0, sizeof (addrinfo));
        }

        /** Sets server info.
         *
         * sets server info for the server we are connecting to.
         * this must be called before anything.
         */
        bool client::get_addr_info(const std::string host, const std::string port) {

            hints.ai_family = AF_UNSPEC; // ipv4 or ipv6
            hints.ai_socktype = SOCK_STREAM; // tcp
            hints.ai_flags = AI_PASSIVE;
            hints.ai_protocol = 0;
            hints.ai_canonname = nullptr;
            hints.ai_addr = nullptr;
            hints.ai_next = nullptr;
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
        bool client::connect(const std::string host, const std::string port) {

            if (!get_addr_info(host, port)) return false;
            if (!results) return false;

            bool rc(false);

            // find a suitable interface
            for (rp = results; rp != nullptr; rp = rp->ai_next) {
                socket_ = socket(rp->ai_family, rp->ai_socktype,
                        rp->ai_protocol);
                if (socket_ == -1)
                    continue;

                // attempt to connect
                if (::connect(socket_, rp->ai_addr, rp->ai_addrlen) != -1) {

                    // set options
                    int option = 1;
                    setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR,
                            (char *) &option, sizeof (option));

                    rc = true;

                    syslog(LOG_DEBUG, "connection to %s OK", host.c_str());

                    // success
                    break;
                }

                close(socket_);
            }

            if (rp == nullptr) {
                syslog(LOG_DEBUG, "unable to allocate interface to destination host");
                return false;
            }

            // free results
            freeaddrinfo(results);

            // open stream for write
            if (nullptr == (this->tx = fdopen(socket_, "w"))) {
                close(socket_);
                syslog(LOG_DEBUG, "unable to open TX stream");
            } else rc = true;

            // open stream for read
            if (nullptr == (this->rx = fdopen(socket_, "r"))) {
                close(socket_);
                syslog(LOG_DEBUG, "unable to open RX stream");
            } else rc = true;

            return rc;
        }

        void client::disconnect(void) {
            close(socket_);
            if (tx)fclose(tx);
            if (rx)fclose(rx);

            rx = nullptr;
            tx = nullptr;
        }

        /** Resize tx socket buffer size.
         */
        bool client::tx_buff_size(const size_t &size) {

            tx_buffer_size = size;
            bool ret_val(false);
            int option(size);

            ret_val = !setsockopt(socket_, SOL_SOCKET, SO_SNDBUF,
                    (char *) &option, sizeof (option));

            return ret_val;
        }

        /** Resize rx socket buffer size.
         */
        bool client::rx_buff_size(const size_t &size) {

            rx_buffer_size = size;
            bool ret_val = false;
            int option = size;

            ret_val = !setsockopt(socket_, SOL_SOCKET, SO_RCVBUF,
                    (char *) &option, sizeof (option));

            return ret_val;
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
        bool client::connected(void) {

            if (this->rx == nullptr) return false;
            if (this->tx == nullptr) return false;
            if (this->socket_ <= 0) return false;

            if (feof_unlocked(this->rx)) return false;
            if (feof_unlocked(this->tx)) return false;

            return true;
        }

        /** Read from rx stream.
         *
         * calls fread() on rx stream.
         */
        size_t client::read(void *data, const size_t size,
                const size_t count) {

            if (!connected()) {
                syslog(LOG_DEBUG, "can not read, no stream available");
                return 0;
            }

            return fread(data, size, count, this->rx);
        }

        std::string client::readline(void) {
            std::string read_string;

            do {
                read_string += this->read8();
            } while (read_string.back() != tcp::EOL);

            return read_string;
        }

        /** Read single unsigned char from rx stream.
         */
        uint8_t client::read8(void) {

            if (!connected()) {
                syslog(LOG_DEBUG, "can not read, no stream available");
                return 0;
            }

            this->lock();
            uint8_t c = fgetc(this->rx);
            this->unlock();

            return c;
        }

        uint16_t client::read16(void) {

            if (!connected()) {
                syslog(LOG_DEBUG, "can not read, no stream available");
                return 0;
            }

            uint16_t u16 = 0;

            this->lock();
            fread(&u16, sizeof (uint16_t), 1, this->rx);
            this->unlock();

            return u16;
        }

        uint32_t client::read32(void) {

            if (!connected()) {
                syslog(LOG_DEBUG, "can not read, no stream available");
                return 0;
            }

            uint32_t u32 = 0;

            this->lock();
            fread(&u32, sizeof (uint32_t), 1, this->rx);
            this->unlock();

            return u32;
        }

        uint64_t client::read64(void) {

            if (!connected()) {
                syslog(LOG_DEBUG, "can not read, no stream available");
                return 0;
            }

            uint64_t u64 = 0;
            
            this->lock();
            fread(&u64, sizeof (uint64_t), 1, this->rx);
            this->unlock();
            
            return u64;
        }

        /* must delete[] returned array
         */
        uint128_t client::read128(void) {

            if (!connected()) {
                syslog(LOG_DEBUG, "can not read, no stream available");
                return 0;
            }

            uint64_t *u128 = new uint64_t[2];
            
            this->lock();
            fread(&u128, sizeof (uint64_t) * 2, 1, this->rx);
            this->unlock();
            
            return u128;
        }

        size_t client::write(const uint8_t *bytes, size_t length) {

            if (!connected()) {
                syslog(LOG_DEBUG, "can not write, no stream available");
                return 0;
            }

            this->lock();
            size_t write_ = fwrite(bytes, sizeof (uint8_t), length, this->tx);
            this->unlock();

            return write_;
        }

        /** Write tx stream.
         *
         * calls fwrite() on tx stream.
         */
        size_t client::write(const void *data, size_t size, size_t count) {

            if (!connected()) {
                syslog(LOG_DEBUG, "can not write, no stream available");
                return 0;
            }

            this->lock();
            size_t write_ = fwrite(data, size, count, this->tx);
            this->unlock();

            return write_;
        }

        /** Write tx stream.
         *
         * calls fwrite() on tx stream.
         */
        size_t client::write(std::string str) {

            if (!connected()) {
                syslog(LOG_DEBUG, "can not write, no stream available");
                return 0;
            }

            this->lock();
            size_t write_ = fwrite(str.c_str(), sizeof (char), str.length(), this->tx);
            this->unlock();

            return write_;
        }

        size_t client::write32(uint8_t byte1, uint8_t byte2, uint8_t byte3, uint8_t byte4) {

            if (!connected()) {
                syslog(LOG_DEBUG, "can not write_formated, no stream available");
                return 0;
            }

            size_t write_ = fprintf(this->tx, "%c%c%c%c", byte1, byte2, byte3, byte4);           

            return write_;
        }

        size_t client::write128(uint128_t bytes16) {
            if (!connected()) {
                syslog(LOG_DEBUG, "can not write_formated, no stream available");
                return 0;
            }

            size_t write_ = this->write((uint8_t *) bytes16, sizeof (uint64_t) * 2);

            return write_;
        }

        size_t client::write64(uint64_t bytes8) {
            if (!connected()) {
                syslog(LOG_DEBUG, "can not write_formated, no stream available");
                return 0;
            }

            size_t write_ = this->write((uint8_t *) & bytes8, sizeof (uint64_t));

            return write_;
        }

        size_t client::write32(uint32_t bytes4) {
            if (!connected()) {
                syslog(LOG_DEBUG, "can not write_formated, no stream available");
                return 0;
            }

            size_t write_ = this->write((uint8_t *) & bytes4, sizeof (uint32_t));

            return write_;
        }

        size_t client::write16(uint16_t bytes2) {
            if (!connected()) {
                syslog(LOG_DEBUG, "can not write_formated, no stream available");
                return 0;
            }

            size_t write_ = this->write((uint8_t *) & bytes2, sizeof (uint16_t));

            return write_;
        }

        size_t client::write24(uint8_t byte1, uint8_t byte2, uint8_t byte3) {

            if (!connected()) {
                syslog(LOG_DEBUG, "can not write_formated, no stream available");
                return 0;
            }

            size_t write_ = fprintf(this->tx, "%c%c%c", byte1, byte2, byte3);

            return write_;
        }

        /** Format data to tx stream.
         *
         * calls fprintf() on tx stream.
         */
        size_t client::write16(uint8_t byte1, uint8_t byte2) {

            if (!connected()) {
                syslog(LOG_DEBUG, "can not write_formated, no stream available");
                return 0;
            }

            size_t write_ = fprintf(this->tx, "%c%c", byte1, byte2);

            return write_;
        }

        size_t client::write8(uint8_t byte) {

            if (!connected()) {
                syslog(LOG_DEBUG, "can not write_formated, no stream available");
                return 0;
            }

            size_t write_ = fprintf(this->tx, "%c", byte);

            return write_;
        }

        /** Flush (send data and clear buffer) tx stream.
         *
         * calls fflush() on tx stream to send data
         * and empty buffer.
         */
        int client::tx_flush(void) {
            this->lock();
            int rc = fflush(this->tx);
            this->unlock();

            return rc;
        }

        /** Flush (clear buffer) rx stream.
         *
         * calls fflush() on rx stream to send data
         * and empty buffer.
         */
        int client::rx_flush(void) {
            this->lock();
            int rc = fflush(this->rx);
            this->unlock();
            
            return rc;
        }
    }
}