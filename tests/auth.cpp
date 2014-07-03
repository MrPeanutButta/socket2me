/* 
 * File:   auth.cpp
 * Author: aahebert
 *
 * Created on Jul 2, 2014, 2:11:35 PM
 */

#include <stdlib.h>
#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>
#include "server.h"
#include "client.h"

/*
 * Simple C++ Test Suite
 */

#ifdef AUTH_TEST

std::string server_read(std::string str) {
    std::cout << "server_read: " << str << std::endl;
    return "AUTH_OK\n";
}

void test_auth_pass(void);
void test_auth_fail(void);

void test_auth() {
    std::cout << "test_auth" << std::endl;

    tcp::server s("this is my md5 key", tcp::auth::MD5);

    s.set_read_callback(server_read);
    s.listen("127.0.0.1", "666");

    sleep(1);
    test_auth_pass();
    test_auth_fail();

    s.kill();
}

void test_auth_fail() {
    std::cout << "test_auth_fail" << std::endl;

    tcp::client c("th my bugger md5 key", tcp::auth::MD5);

    c.authenticate("127.0.0.1", "666");

    if (c.connected()) {

        c.write("test_auth_fail: authenticated\n");
        c.send();
        std::cout << c.readline() << std::endl;
        c.disconnect();
    } else {
        std::cerr << "test_auth_fail: authentication FAILED!\n";
    }
}

void test_auth_pass() {
    std::cout << "test_auth_pass" << std::endl;

    tcp::client c("this is my md5 key", tcp::auth::MD5);

    c.authenticate("127.0.0.1", "666");

    if (c.connected()) {

        c.write("test_auth_pass: authenticated\n");
        c.send();
        std::cout << c.readline() << std::endl;
        c.disconnect();
    } else {
        std::cerr << "test_auth_pass: authentication FAILED!\n";
    }
}

#endif

#ifdef SRV1_TEST

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

#endif

#ifdef SRV2_TEST

std::string srv2_read(std::string str) {
    std::cout << "srv2_read: " << str << std::endl;
    return "srv2_read: OK\n";
}

tcp::server * init_srv2(void) {
    std::cout << "init_srv2" << std::endl;

    tcp::server *s = new tcp::server("test_auth_pass", tcp::auth::MD5);

    s->set_read_callback(srv2_read);
    s->listen("127.0.0.1", "667");

    return s;
}

#endif

#ifdef SRV3_TEST

std::string srv3_read(std::string str) {
    std::cout << "srv3_read: " << str << std::endl;
    return "srv3_read: OK\n";
}

tcp::server * init_srv3(void) {
    std::cout << "init_srv3" << std::endl;

    tcp::server *s = new tcp::server("test_auth_pass", tcp::auth::MD5);

    s->set_read_callback(srv3_read);
    s->listen("127.0.0.1", "668");

    return s;
}

#endif

#ifdef CLIENT_TEST

void init_failover_client(void) {
    std::cout << "init_failover_client" << std::endl;

    tcp::client c("test_auth_pass", tcp::auth::MD5);

    c.authenticate("127.0.0.1", "666");
    c.authenticate("127.0.0.1", "667");
    c.authenticate("127.0.0.1", "668");

    while (c.connected()) {
        c.write("init_failover_client: data_write\n");
        c.send();
        std::cout << c.readline() << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }

    std::cout << "init_failover_client: no longer connected\n";
}

void test_failover(void) {

    std::this_thread::sleep_for(std::chrono::seconds(5));

    init_failover_client();
    //    //std::thread *t = new std::thread(init_failover_client);
    //    //t->detach();
    //
    //    //sleep(60);
    //    // start failover from s1
    //    /*std::this_thread::sleep_for(std::chrono::seconds(10));
    //    s3->kill();
    //    std::this_thread::sleep_for(std::chrono::seconds(60));
    //    s2->kill();
    //    std::this_thread::sleep_for(std::chrono::seconds(20));
    //    s1->kill();*/
    //    //*/

}

#endif

int main(int argc, char** argv) {

#ifdef AUTH_TEST
    std::cout << "%TEST_STARTED% test_auth (md5 authentication)" << std::endl;
    test_auth();
    std::cout << "%TEST_FINISHED% test_auth (md5 authentication)" << std::endl;
#endif

#ifdef CLIENT_TEST
    std::cout << "%TEST_STARTED% test_failover (redundant server, client failover)" << std::endl;
    test_failover();
    std::cout << "%TEST_FINISHED% test_failover (redundant server, client failover)" << std::endl;
#endif

#ifdef SRV1_TEST
    while (1) {
        tcp::server *s1 = init_srv1();
        std::this_thread::sleep_for(std::chrono::seconds(50));
        s1->kill();
        std::this_thread::sleep_for(std::chrono::seconds(5));
        delete s1;
    }
#endif

#ifdef SRV2_TEST
    while (1) {
        tcp::server *s2 = init_srv2();
        std::this_thread::sleep_for(std::chrono::seconds(70));
        s2->kill();
        delete s2;
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
#endif

#ifdef SRV3_TEST
    while (1) {
        tcp::server *s3 = init_srv3();
        std::this_thread::sleep_for(std::chrono::seconds(90));
        s3->kill();
        delete s3;
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
#endif

    return (EXIT_SUCCESS);
}

