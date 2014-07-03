/* 
 * File:   auth.cpp
 * Author: aahebert
 *
 * Created on Jul 2, 2014, 2:11:35 PM
 */

#include <stdlib.h>
#include <iostream>
#include <string>
#include <unistd.h>
#include "server.h"
#include "client.h"

/*
 * Simple C++ Test Suite
 */

std::string server_read(std::string str) {
    std::cout << "my server read: " << str << std::endl;
    return "OK\n";
}

void test2(void);

void test1() {
    std::cout << "auth test 1" << std::endl;

    tcp::server s("this is my md5 key", tcp::auth::MD5);

    s.set_read_callback(server_read);
    s.listen("127.0.0.1", "666");

    sleep(1);
    test2();

    s.kill();
}

void test2() {
    std::cout << "auth test 2" << std::endl;

    tcp::client c("this is my md5 key", tcp::auth::MD5);
    //c.connect("127.0.0.1", "666");
    c.authenticate("127.0.0.1", "666");

    if (c.connected()) {

        c.write("my md5 hash key is \"this is my md5 key\"\n");
        c.send();
        std::cout << c.readline() << std::endl;

        c.disconnect();
    }
}

int main(int argc, char** argv) {
    std::cout << "%SUITE_STARTING% auth" << std::endl;
    std::cout << "%SUITE_STARTED%" << std::endl;

    std::cout << "%TEST_STARTED% test1 (auth)" << std::endl;
    test1();
    std::cout << "%TEST_FINISHED% time=0 test1 (auth)" << std::endl;

    return (EXIT_SUCCESS);
}

