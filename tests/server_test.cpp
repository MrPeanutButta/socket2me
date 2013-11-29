/*
 * File:   server_test.cpp
 * Author: aaron
 *
 * Created on Nov 29, 2013, 12:48:45 PM
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
    std::cout << "server_test test 2" << std::endl;

    std::cout << "test2 read " << text << std::endl;

    std::cout << "returning string";

    return "this is a test return";
}

void test1() {
    std::cout << "server_test test 1" << std::endl;
    TCPSocket tcp;

    tcp.set_read_callback(test2);
    tcp.listen_for_connections("127.0.0.1", "666");
    tcp.set_EOL_char('\r');

    sleep(180);

}



int main(int argc, char** argv) {
    std::cout << "%SUITE_STARTING% server_test" << std::endl;
    std::cout << "%SUITE_STARTED%" << std::endl;

    std::cout << "%TEST_STARTED% test1 (server_test)" << std::endl;
    test1();

    return (EXIT_SUCCESS);
}

