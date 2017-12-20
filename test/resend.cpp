// Copyright Takatoshi Kondo 2015
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "combi_test.hpp"

BOOST_AUTO_TEST_SUITE(test_resend)

BOOST_AUTO_TEST_CASE( publish_qos1 ) {
    auto test = [](boost::asio::io_service& ios, auto& c, auto& s) {
        c->set_client_id("cid1");
        c->set_clean_session(true);

        std::uint16_t pid_pub;

        int order = 0;
        c->set_connack_handler(
            [&order, &c, &pid_pub]
            (bool sp, std::uint8_t connack_return_code) {
                BOOST_TEST(connack_return_code == mqtt::connect_return_code::accepted);
                switch (order++) {
                case 0: // clean session
                    BOOST_TEST(sp == false);
                    c->disconnect();
                    break;
                case 2:
                    BOOST_TEST(sp == false);
                    pid_pub = c->publish_at_least_once("topic1", "topic1_contents");
                    c->force_disconnect();
                    break;
                case 4:
                    BOOST_TEST(sp == true);
                    break;
                default:
                    BOOST_CHECK(false);
                    break;
                }
                return true;
            });
        c->set_close_handler(
            [&order, &c, &s]
            () {
                switch (order++) {
                case 1:
                    c->set_clean_session(false);
                    c->connect();
                    break;
                case 6:
                    s.close();
                    break;
                default:
                    BOOST_CHECK(false);
                    break;
                }
            });
        c->set_error_handler(
            [&order, &c]
            (boost::system::error_code const&) {
                BOOST_TEST(order++ == 3);
                c->connect();
            });
        c->set_puback_handler(
            [&order, &c, &pid_pub]
            (std::uint16_t packet_id) {
                BOOST_TEST(order++ == 5);
                BOOST_TEST(packet_id == pid_pub);
                c->disconnect();
                return true;
            });
        c->connect();
        ios.run();
        BOOST_TEST(order++ == 7);
    };
    do_combi_test(test);
}

BOOST_AUTO_TEST_CASE( publish_qos2 ) {
    auto test = [](boost::asio::io_service& ios, auto& c, auto& s) {
        c->set_client_id("cid1");
        c->set_clean_session(true);

        std::uint16_t pid_pub;

        int order = 0;
        c->set_connack_handler(
            [&order, &c, &pid_pub]
            (bool sp, std::uint8_t connack_return_code) {
                BOOST_TEST(connack_return_code == mqtt::connect_return_code::accepted);
                switch (order++) {
                case 0: // clean session
                    BOOST_TEST(sp == false);
                    c->disconnect();
                    break;
                case 2:
                    BOOST_TEST(sp == false);
                    pid_pub = c->publish_exactly_once("topic1", "topic1_contents");
                    c->force_disconnect();
                    break;
                case 4:
                    BOOST_TEST(sp == true);
                    break;
                default:
                    BOOST_CHECK(false);
                    break;
                }
                return true;
            });
        c->set_close_handler(
            [&order, &c, &s]
            () {
                switch (order++) {
                case 1:
                    c->set_clean_session(false);
                    c->connect();
                    break;
                case 7:
                    s.close();
                    break;
                default:
                    BOOST_CHECK(false);
                    break;
                }
            });
        c->set_error_handler(
            [&order, &c]
            (boost::system::error_code const&) {
                BOOST_TEST(order++ == 3);
                c->connect();
            });
        c->set_pubrec_handler(
            [&order, &pid_pub]
            (std::uint16_t packet_id) {
                BOOST_TEST(order++ == 5);
                BOOST_TEST(packet_id == pid_pub);
                return true;
            });
        c->set_pubcomp_handler(
            [&order, &c, &pid_pub]
            (std::uint16_t packet_id) {
                BOOST_TEST(order++ == 6);
                BOOST_TEST(packet_id == pid_pub);
                c->disconnect();
                return true;
            });
        c->connect();
        ios.run();
        BOOST_TEST(order++ == 8);
    };
    do_combi_test(test);
}

BOOST_AUTO_TEST_CASE( pubrel_qos2 ) {
    auto test = [](boost::asio::io_service& ios, auto& c, auto& s) {
        c->set_client_id("cid1");
        c->set_clean_session(true);

        std::uint16_t pid_pub;

        int order = 0;
        c->set_connack_handler(
            [&order, &c, &pid_pub]
            (bool sp, std::uint8_t connack_return_code) {
                BOOST_TEST(connack_return_code == mqtt::connect_return_code::accepted);
                switch (order++) {
                case 0: // clean session
                    BOOST_TEST(sp == false);
                    c->disconnect();
                    break;
                case 2:
                    BOOST_TEST(sp == false);
                    pid_pub = c->publish_exactly_once("topic1", "topic1_contents");
                    break;
                case 5:
                    BOOST_TEST(sp == true);
                    break;
                default:
                    BOOST_CHECK(false);
                    break;
                }
                return true;
            });
        c->set_close_handler(
            [&order, &c, &s]
            () {
                switch (order++) {
                case 1:
                    c->set_clean_session(false);
                    c->connect();
                    break;
                case 7:
                    s.close();
                    break;
                default:
                    BOOST_CHECK(false);
                    break;
                }
            });
        c->set_error_handler(
            [&order, &c]
            (boost::system::error_code const&) {
                BOOST_TEST(order++ == 4);
                c->connect();
            });
        c->set_pubrec_handler(
            [&order, &c, &pid_pub]
            (std::uint16_t packet_id) {
                switch (order++) {
                case 3:
                    BOOST_TEST(packet_id == pid_pub);
                    c->force_disconnect();
                    break;
                default:
                    BOOST_CHECK(false);
                    break;
                }
                return true;
            });
        c->set_pubcomp_handler(
            [&order, &c]
            (std::uint16_t packet_id) {
                BOOST_TEST(order++ == 6);
                BOOST_TEST(packet_id == 1);
                c->disconnect();
                return true;
            });
        c->connect();
        ios.run();
        BOOST_TEST(order++ == 8);
    };
    do_combi_test(test);
}

BOOST_AUTO_TEST_CASE( publish_pubrel_qos2 ) {
    auto test = [](boost::asio::io_service& ios, auto& c, auto& s) {
        c->set_client_id("cid1");
        c->set_clean_session(true);

        std::uint16_t pid_pub;

        int order = 0;
        c->set_connack_handler(
            [&order, &c, & pid_pub]
            (bool sp, std::uint8_t connack_return_code) {
                BOOST_TEST(connack_return_code == mqtt::connect_return_code::accepted);
                switch (order++) {
                case 0: // clean session
                    BOOST_TEST(sp == false);
                    c->disconnect();
                    break;
                case 2:
                    BOOST_TEST(sp == false);
                    pid_pub = c->publish_exactly_once("topic1", "topic1_contents");
                    c->force_disconnect();
                    break;
                case 4:
                    BOOST_TEST(sp == true);
                    break;
                case 7:
                    BOOST_TEST(sp == true);
                    break;
                default:
                    BOOST_CHECK(false);
                    break;
                }
                return true;
            });
        c->set_close_handler(
            [&order, &c, &s]
            () {
                switch (order++) {
                case 1:
                    c->set_clean_session(false);
                    c->connect();
                    break;
                case 9:
                    s.close();
                    break;
                default:
                    BOOST_CHECK(false);
                    break;
                }
            });
        c->set_error_handler(
            [&order, &c]
            (boost::system::error_code const&) {
                switch (order++) {
                case 3:
                    c->connect();
                    break;
                case 6:
                    c->connect();
                    break;
                default:
                    BOOST_CHECK(false);
                    break;
                }
            });
        c->set_pubrec_handler(
            [&order, &c, &pid_pub]
            (std::uint16_t packet_id) {
                switch (order++) {
                case 5:
                    BOOST_TEST(packet_id == pid_pub);
                    c->force_disconnect();
                    break;
                default:
                    BOOST_CHECK(false);
                    break;
                }
                return true;
            });
        c->set_pubcomp_handler(
            [&order, &c, &pid_pub]
            (std::uint16_t packet_id) {
                BOOST_TEST(order++ == 8);
                BOOST_TEST(packet_id == pid_pub);
                c->disconnect();
                return true;
            });
        c->connect();
        ios.run();
        BOOST_TEST(order++ == 10);
    };
    do_combi_test(test);
}

BOOST_AUTO_TEST_CASE( multi_publish_qos1 ) {
    auto test = [](boost::asio::io_service& ios, auto& c, auto& s) {
        c->set_client_id("cid1");
        c->set_clean_session(true);

        std::uint16_t pid_pub1;
        std::uint16_t pid_pub2;

        int order = 0;
        c->set_connack_handler(
            [&order, &c, &pid_pub1, &pid_pub2]
            (bool sp, std::uint8_t connack_return_code) {
                BOOST_TEST(connack_return_code == mqtt::connect_return_code::accepted);
                switch (order++) {
                case 0: // clean session
                    BOOST_TEST(sp == false);
                    c->disconnect();
                    break;
                case 2:
                    BOOST_TEST(sp == false);
                    pid_pub1 = c->publish_at_least_once("topic1", "topic1_contents1");
                    pid_pub2 = c->publish_at_least_once("topic1", "topic1_contents2");
                    c->force_disconnect();
                    break;
                case 4:
                    BOOST_TEST(sp == true);
                    break;
                default:
                    BOOST_CHECK(false);
                    break;
                }
                return true;
            });
        c->set_close_handler(
            [&order, &c, &s]
            () {
                switch (order++) {
                case 1:
                    c->set_clean_session(false);
                    c->connect();
                    break;
                case 7:
                    s.close();
                    break;
                default:
                    BOOST_CHECK(false);
                    break;
                }
            });
        c->set_error_handler(
            [&order, &c, &s]
            (boost::system::error_code const&) {
                switch (order++) {
                case 3:
                    c->connect();
                    break;
                case 7:
                    s.close();
                    break;
                default:
                    BOOST_CHECK(false);
                    break;
                }
            });
        c->set_puback_handler(
            [&order, &c, &pid_pub1, &pid_pub2]
            (std::uint16_t packet_id) {
                switch (order++) {
                case 5:
                    BOOST_TEST(packet_id == pid_pub1);
                    break;
                case 6:
                    BOOST_TEST(packet_id == pid_pub2);
                    break;
                }
                c->disconnect();
                return true;
            });
        c->connect();
        ios.run();
        BOOST_TEST(order++ == 8);
    };
    do_combi_test(test);
}

BOOST_AUTO_TEST_SUITE_END()
