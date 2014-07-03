### log2::tcp::socket : A simple C++ class for TCP/IP on Linux
A simple to use TCP/IP library for communication between your applications and other TCP/IP speakers.
This project has been split from a larger project. I thought this was a bit useful for using in other projects. Hopefully others may find it useful.

### Example TCP Client Usage

``` cpp
#include "tcp.h"

int main(){
    tcp::client tcpClient;

    // connect to loopback port 666
    tcpClient.connect("127.0.0.1", "666");

    // if connected we can write data
    if (tcpClient.connected()) {
        // writes will buffer until send() is called
        tcpClient.write("*************write from client test*************\n");
        tcpClient.send();

        // read a response
        std::cout << tcpClient.readline() << std::endl;

        tcpClient.disconnect();
    }

    return 0;
}
```

### Example TCP Server Usage

``` cpp
#include <iostream>
#include <string>
#include "tcp.h"

std::string test2(std::string text) {
    std::cout << "server test 2" << std::endl;
    std::cout << "test2 read " << text << std::endl;
    std::cout << "returning string";

    return "*************this is a test return*************\n";
}

int main(){

    tcp::server tcpServer;
	// default EOL char is '\n' but can be set
    log2::tcp::EOL = '\n';
    // set callback for when data is read from the stream
    tcpServer.set_read_callback(test2);
    // listen on loopback for port 666 connections
    tcpServer.listen("127.0.0.1", "666");

    return 0;
}
```