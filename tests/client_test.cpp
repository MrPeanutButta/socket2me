/*
 * File:   client_test.cpp
 * Author: aaron
 *
 * Created on Nov 29, 2013, 2:19:09 PM
 */

#include <stdlib.h>
#include <iostream>
#include <string>
#include <unistd.h>
#include "tcp_sockets.h"

/*
 * Simple C++ Test Suite
 */

std::string test2(std::string text) {
    std::cout << "client test 2" << std::endl;

    std::cout << "test2 read " << text << std::endl;

    std::cout << "returning string";

    return "this is a test return";
}

void test1() {
    std::cout << "client test 1" << std::endl;
    TCPSocket tcpServer;
    TCPSocket tcpClient;

    tcpServer.set_read_callback(test2);
    tcpServer.listen_for_connections("127.0.0.1", "666");
    tcpServer.set_EOL_char('\n');

    sleep(5);

    tcpClient.connect("127.0.0.1", "666");
    if (tcpClient.connected()) {
        tcpClient.write("*************write from client test*************\n");
        tcpClient.send();
        tcpClient.disconnect();
    }

    tcpServer.kill_service();

}

int main(int argc, char** argv) {
    std::cout << "%SUITE_STARTING% client_test" << std::endl;
    std::cout << "%SUITE_STARTED%" << std::endl;

    std::cout << "%TEST_STARTED% test1 (client_test)" << std::endl;
    test1();

    return (EXIT_SUCCESS);
}



