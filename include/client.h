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

#include <mutex>
#include <thread>
#include <string>
#include <netdb.h>

namespace log2 {
    namespace tcp {

        // 128 bit type
        typedef uint64_t* uint128_t;

        class client {
        public:

            client();
            client(const client& orig);

            virtual ~client() = 0;

            bool connect(const std::string host, const std::string port);
            bool connect(void);
            bool connected(void);

            bool tx_buff_size(const size_t &);
            bool rx_buff_size(const size_t &);

            size_t read(void *data, const size_t size, const size_t count);
            std::string readline(void);

            uint128_t read128(void);
            uint64_t read64(void);
            uint32_t read32(void);
            uint16_t read16(void);
            uint8_t read8(void);

            size_t write128(uint128_t bytes16);
            size_t write64(uint64_t bytes8);
            size_t write32(uint32_t bytes4);
            size_t write16(uint16_t byte2);

            size_t write32(uint8_t byte1, uint8_t byte2, uint8_t byte3, uint8_t byte4);
            size_t write24(uint8_t byte1, uint8_t byte2, uint8_t byte3);
            size_t write16(uint8_t byte1, uint8_t byte2);
            size_t write8(uint8_t byte);

            size_t write(const uint8_t *bytes, size_t length);
            size_t write(const void *data, size_t size, size_t count);
            size_t write(std::string str);

            int tx_flush(void);
            int rx_flush(void);

            void disconnect(void);
            void reset(void);

            void lock(void) {
                while (!this->write_mutex.try_lock()) {
                    std::this_thread::sleep_for(
                            std::chrono::nanoseconds(lock_interval)
                            );
                }
            }

            void unlock(void) {
                this->write_mutex.unlock();
            }

            void send(void) {
                this->tx_flush();
            }

        private:
            bool get_addr_info(const std::string host, const std::string port);

            friend class server;

            addrinfo hints;
            addrinfo *results, *rp;

            int socket_;
            int rx_buffer_size;
            int tx_buffer_size;

            FILE *tx;
            FILE *rx;

            int lock_interval;
            std::mutex write_mutex;
        };
    }
}

#endif	/* TCP_CLIENT_H */

