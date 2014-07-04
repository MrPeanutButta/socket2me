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



#ifndef TCP_SOCKETS_H
#define	TCP_SOCKETS_H

#include <map>
#include <mutex>
#include <thread>
#include <string>
#include <netdb.h>
#include <functional>
#include <memory>
#include <vector>
#include <cstddef>
#include <cassert>
#include <cstring>
#include <syslog.h>
#include "md5.h"

namespace tcp {
    
    extern char EOL;
   
    // auth ON/OFF
    enum class auth : uint8_t {
        OFF, MD5
    };

    // simple custom ACK tokens
    enum class auth_status : uint8_t {
        AUTH_OK, AUTH_FAILED
    };
    
    
    // 128 bit type, can read MD5, INET6 for example
    typedef uint64_t* uint128_t;

    class ip_point {
    public:

        ip_point() : socket_(0),
        rx_buffer_size(4096),
        tx_buffer_size(4096) {
            rp = nullptr;
            results = nullptr;
            tx = nullptr;
            rx = nullptr;
        }
        
        std::string host;
        std::string port;
        
        addrinfo hints;
        addrinfo *results, *rp;

        int socket_;
        int rx_buffer_size;
        int tx_buffer_size;

        FILE *tx;
        FILE *rx;

        bool connected(void) {

            if (this->rx == nullptr) return false;
            if (this->tx == nullptr) return false;
            if (this->socket_ <= 0) return false;

            if (feof_unlocked(this->rx)) return false;
            if (feof_unlocked(this->tx)) return false;

            return true;
        }
    };

    class socket {
    private:
        friend class server;
        friend class client;

        // key and digested key
        std::string md5_key_;
        std::unique_ptr<unsigned char> md5_hash_;

        bool is_authed_;
        auth auth_type_;

        int lock_interval_;
        std::mutex write_mutex_;
        
        std::shared_ptr<ip_point> ip_endpoint_;

        void init_md5(const std::string &key);
        void add_connection(std::size_t index);

    public:

        virtual ~socket() = 0;

        socket();
        socket(const socket& orig);
        socket(std::string key = "", auth auth_ = tcp::auth::OFF);

        void ip_endpoint(std::shared_ptr<ip_point> &ep);
        bool authenticate(std::string host, std::string port);
        bool connect(const std::string host, const std::string port);
        bool connected(void);
        bool failover(void);
        bool tx_buff_size(const size_t &);
        bool rx_buff_size(const size_t &);

        size_t read(void *data, const size_t size, const size_t count);
        std::string readline(void);
        uint128_t read128(void);
        uint64_t read64(void);
        uint32_t read32(void);
        uint16_t read16(void);
        uint8_t read8(void);

        size_t write32(uint8_t byte1, uint8_t byte2, uint8_t byte3, uint8_t byte4);
        size_t write24(uint8_t byte1, uint8_t byte2, uint8_t byte3);
        size_t write16(uint8_t byte1, uint8_t byte2);
        size_t write8(uint8_t byte);
        size_t write128(uint128_t bytes16);
        size_t write64(uint64_t bytes8);
        size_t write32(uint32_t bytes4);
        size_t write16(uint16_t bytes2);
        size_t write(const uint8_t *bytes, size_t length);
        size_t write(const void *data, size_t size, size_t count);
        size_t write(std::string str);

        int tx_flush(void);
        int rx_flush(void);

        void disconnect(void);
        void reset(void);

        void lock(void) {
            while (!this->write_mutex_.try_lock()) {
                std::this_thread::sleep_for(
                        std::chrono::nanoseconds(lock_interval_)
                        );
            }
        }

        void unlock(void) {
            this->write_mutex_.unlock();
        }

        void send(void) {
            this->tx_flush();
        }

    private:
        bool get_addr_info(const std::string host, const std::string port);
    };
}
#endif	/* TCP_SOCKETS_H */

