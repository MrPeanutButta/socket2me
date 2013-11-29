### TCPSockets : A generic TCP/IP class for Linux
A simple to use TCP/IP library for communication between your applications and other TCP/IP speakers.

### Example TCP Client Usage

``` cpp
#include "tcp_sockets.h"

/* also see tcp_client::write(void *data, const size_t &size, const size_t &count)
 * for writing binary data to the stream. */

int main(){
    TCPSocket tcpClient;

    // connect to loopback port 666
    tcpClient.connect("127.0.0.1", "666");

    // if connected we can write data
    if (tcpClient.connected()) {
        tcpClient.write("*************write from client test*************\n");
        tcpClient.send();
        tcpClient.disconnect();
    }

    return 0;
}
```

### Example TCP Server Usage

``` cpp
#include <string>
#include "tcp_sockets.h"

std::string test2(std::string text) {
    std::cout << "server test 2" << std::endl;
    std::cout << "test2 read " << text << std::endl;
    std::cout << "returning string";

    return "*************this is a test return*************";
}

int main(){

    TCPSocket tcpServer;
    // set callback for when data is read from the stream
    tcpServer.set_read_handler(test2);
    // listen on loopback for port 666 connections
    tcpServer.listen_for_connections("127.0.0.1", "666");
    // default EOL char is '\n' but can be set
    tcpServer.set_EOL_char('\n');

    return 0;
}
```