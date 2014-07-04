### tcp::socket : A simple C++ class for TCP/IP on Linux
A simple to use TCP/IP library for communication between daemons and other TCP/IP speakers.

This project has been split from a larger project.

This isn't exactly standardized **TCP/IP**, most of this code is specific to inter-process communication **IPC**. 

The idea of this project is to provide redundancy and reliable data delivery.

* TCP/IP client/server interface
* Client fail-over to secondary hosts:ports
* MD5 token exchange
	* This library has a proprietary MD5 token exchange for simple pre-shared key.
* Multi-threaded TCP server

**see auth.cpp for more test examples**
### Example TCP Client Usage

``` cpp
#include "tcp.h"

void init_failover_client(void) {
    std::cout << "init_failover_client" << std::endl;

    // Passing MD5 key and auth type
    tcp::client c("test_auth_pass", tcp::auth::MD5);

    while(!c.authenticate("127.0.0.1", "666"));
    
    // Failover connections
    c.add_failover("127.0.0.1", "667");
    c.add_failover("127.0.0.1", "668");

    do {
        while (c.connected()) {
            c.write("init_failover_client: data_write\n");
            c.send();
            std::cout << c.readline();
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        std::cout << "attempting failover\n";
    } while (c.failover());

    std::cout << "init_failover_client: no longer connected\n";
}
```

### Example TCP Server Usage

``` cpp
#include <iostream>
#include <string>
#include "tcp.h"

std::string srv1_read(std::string str) {
    std::cout << "srv1_read: " << str << std::endl;
    return "srv1_read: OK\n";
}

tcp::server *init_srv1(void) {
    std::cout << "init_srv1" << std::endl;

    tcp::server *s = new tcp::server("test_auth_pass", tcp::auth::MD5);

    s->set_read_callback(srv1_read);
    s->listen("127.0.0.1", "666");

    return s;
}

int main(){
        tcp::server *s1 = init_srv1();
        std::this_thread::sleep_for(std::chrono::seconds(50));
        s1->disconnect();
        s1->kill();
        std::cout << "srv1 killed" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));
        delete s1;
	return 0;
}
```