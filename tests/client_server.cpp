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
#include "tcp.h"

/*
 * Simple C++ Test Suite
 */

std::string test2(std::string text) {
    std::cout << "test2: server read" << std::endl;

    std::cout << "test2: read " << text << std::endl;

    std::cout << "test2: returning string\n";

    return "this is a test2 server return\t";
}

void test1() {
    std::cout << "client test 1" << std::endl;
    log2::tcp::socket tcpServer;
    log2::tcp::socket tcpClient;

    tcpServer.set_read_callback(test2);
    tcpServer.listen("127.0.0.1", "666");
    log2::tcp::EOL = '\t';

    sleep(5);

    tcpClient.connect("127.0.0.1", "666");
    if (tcpClient.connected()) {
        tcpClient.write(": from test1: *************write from client test*************\t");
        tcpClient.send();
        std::cout << "test1: " << tcpClient.readline();
        tcpClient.disconnect();
    }

    tcpServer.kill();
}

int main(int argc, char** argv) {
    std::cout << "%SUITE_STARTING% client_test" << std::endl;
    std::cout << "%SUITE_STARTED%" << std::endl;

    std::cout << "%TEST_STARTED% test1 (client_test)" << std::endl;
    test1();

    return (EXIT_SUCCESS);
}



