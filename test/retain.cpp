// Copyright Takatoshi Kondo 2015
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "combi_test.hpp"

BOOST_AUTO_TEST_SUITE(test_retain)

BOOST_AUTO_TEST_CASE( simple ) {
    auto test = [](boost::asio::io_service& ios, auto& c, auto& s) {
        c->set_clean_session(true);

        std::uint16_t pid_sub;
        std::uint16_t pid_unsub;

        int order = 0;
        c->set_connack_handler(
            [&order, &c, &pid_sub]
            (bool sp, std::uint8_t connack_return_code) {
                BOOST_TEST(order++ == 0);
                BOOST_TEST(sp == false);
                BOOST_TEST(connack_return_code == mqtt::connect_return_code::accepted);

                c->publish_at_most_once("topic1", "retained_contents", true);

                pid_sub = c->subscribe("topic1", mqtt::qos::at_most_once);
                return true;
            });
        c->set_close_handler(
            [&order, &s]
            () {
                BOOST_TEST(order++ == 4);
                s.close();
            });
        c->set_error_handler(
            []
            (boost::system::error_code const&) {
                BOOST_CHECK(false);
            });
        c->set_puback_handler(
            []
            (std::uint16_t) {
                BOOST_CHECK(false);
                return true;
            });
        c->set_pubrec_handler(
            []
            (std::uint16_t) {
                BOOST_CHECK(false);
                return true;
            });
        c->set_pubcomp_handler(
            []
            (std::uint16_t) {
                BOOST_CHECK(false);
                return true;
            });
        c->set_suback_handler(
            [&order, &pid_sub]
            (std::uint16_t packet_id, std::vector<boost::optional<std::uint8_t>> results) {
                BOOST_TEST(order++ == 1);
                BOOST_TEST(packet_id == pid_sub);
                BOOST_TEST(results.size() == 1U);
                BOOST_TEST(*results[0] == mqtt::qos::at_most_once);
                return true;
            });
        c->set_unsuback_handler(
            [&order, &c, &pid_unsub]
            (std::uint16_t packet_id) {
                BOOST_TEST(order++ == 3);
                BOOST_TEST(packet_id == pid_unsub);
                c->disconnect();
                return true;
            });
        c->set_publish_handler(
            [&order, &c, &pid_unsub]
            (std::uint8_t header,
             boost::optional<std::uint16_t> packet_id,
             std::string topic,
             std::string contents) {
                BOOST_TEST(order++ == 2);
                BOOST_TEST(mqtt::publish::is_dup(header) == false);
                BOOST_TEST(mqtt::publish::get_qos(header) == mqtt::qos::at_most_once);
                BOOST_TEST(mqtt::publish::is_retain(header) == true);
                BOOST_CHECK(!packet_id);
                BOOST_TEST(topic == "topic1");
                BOOST_TEST(contents == "retained_contents");
                pid_unsub = c->unsubscribe("topic1");
                return true;
            });
        c->connect();
        ios.run();
        BOOST_TEST(order++ == 5);
    };
    do_combi_test(test);
}

BOOST_AUTO_TEST_CASE( overwrite ) {
    auto test = [](boost::asio::io_service& ios, auto& c, auto& s) {
        c->set_clean_session(true);

        std::uint16_t pid_sub;
        std::uint16_t pid_unsub;

        int order = 0;
        c->set_connack_handler(
            [&order, &c, &pid_sub]
            (bool sp, std::uint8_t connack_return_code) {
                BOOST_TEST(order++ == 0);
                BOOST_TEST(sp == false);
                BOOST_TEST(connack_return_code == mqtt::connect_return_code::accepted);

                c->publish_at_most_once("topic1", "retained_contents1", true);
                c->publish_at_most_once("topic1", "retained_contents2", true);
                c->publish_at_most_once("topic1", "retained_contents3", false);

                pid_sub = c->subscribe("topic1", mqtt::qos::at_most_once);
                return true;
            });
        c->set_close_handler(
            [&order, &s]
            () {
                BOOST_TEST(order++ == 4);
                s.close();
            });
        c->set_error_handler(
            []
            (boost::system::error_code const&) {
                BOOST_CHECK(false);
            });
        c->set_puback_handler(
            []
            (std::uint16_t) {
                BOOST_CHECK(false);
                return true;
            });
        c->set_pubrec_handler(
            []
            (std::uint16_t) {
                BOOST_CHECK(false);
                return true;
            });
        c->set_pubcomp_handler(
            []
            (std::uint16_t) {
                BOOST_CHECK(false);
                return true;
            });
        c->set_suback_handler(
            [&order, &pid_sub]
            (std::uint16_t packet_id, std::vector<boost::optional<std::uint8_t>> results) {
                BOOST_TEST(order++ == 1);
                BOOST_TEST(packet_id == pid_sub);
                BOOST_TEST(results.size() == 1U);
                BOOST_TEST(*results[0] == mqtt::qos::at_most_once);
                return true;
            });
        c->set_unsuback_handler(
            [&order, &c, &pid_unsub]
            (std::uint16_t packet_id) {
                BOOST_TEST(order++ == 3);
                BOOST_TEST(packet_id == pid_unsub);
                c->disconnect();
                return true;
            });
        c->set_publish_handler(
            [&order, &c, &pid_unsub]
            (std::uint8_t header,
             boost::optional<std::uint16_t> packet_id,
             std::string topic,
             std::string contents) {
                BOOST_TEST(order++ == 2);
                BOOST_TEST(mqtt::publish::is_dup(header) == false);
                BOOST_TEST(mqtt::publish::get_qos(header) == mqtt::qos::at_most_once);
                BOOST_TEST(mqtt::publish::is_retain(header) == true);
                BOOST_CHECK(!packet_id);
                BOOST_TEST(topic == "topic1");
                BOOST_TEST(contents == "retained_contents2");
                pid_unsub = c->unsubscribe("topic1");
                return true;
            });
        c->connect();
        ios.run();
        BOOST_TEST(order++ == 5);
    };
    do_combi_test(test);
}

BOOST_AUTO_TEST_CASE( retain_and_publish ) {
    auto test = [](boost::asio::io_service& ios, auto& c, auto& s) {
        c->set_clean_session(true);

        std::uint16_t pid_sub;
        std::uint16_t pid_unsub;

        int order = 0;
        c->set_connack_handler(
            [&order, &c, &pid_sub]
            (bool sp, std::uint8_t connack_return_code) {
                BOOST_TEST(order++ == 0);
                BOOST_TEST(sp == false);
                BOOST_TEST(connack_return_code == mqtt::connect_return_code::accepted);
                pid_sub = c->subscribe("topic1", mqtt::qos::at_most_once);
                return true;
            });
        c->set_close_handler(
            [&order, &s]
            () {
                BOOST_TEST(order++ == 7);
                s.close();
            });
        c->set_error_handler(
            []
            (boost::system::error_code const&) {
                BOOST_CHECK(false);
            });
        c->set_puback_handler(
            []
            (std::uint16_t) {
                BOOST_CHECK(false);
                return true;
            });
        c->set_pubrec_handler(
            []
            (std::uint16_t) {
                BOOST_CHECK(false);
                return true;
            });
        c->set_pubcomp_handler(
            []
            (std::uint16_t) {
                BOOST_CHECK(false);
                return true;
            });
        c->set_suback_handler(
            [&order, &c, &pid_sub]
            (std::uint16_t packet_id, std::vector<boost::optional<std::uint8_t>> results) {
                BOOST_TEST(packet_id == pid_sub);
                BOOST_TEST(results.size() == 1U);
                BOOST_TEST(*results[0] == mqtt::qos::at_most_once);
                switch (order++) {
                case 1:
                    c->publish_at_most_once("topic1", "topic1_contents", true);
                    break;
                case 4:
                    break;
                default:
                    BOOST_CHECK(false);
                    break;
                }
                return true;
            });
        c->set_unsuback_handler(
            [&order, &c, &pid_sub, &pid_unsub]
            (std::uint16_t packet_id) {
                BOOST_TEST(packet_id == pid_unsub);
                switch (order++) {
                case 3:
                    pid_sub = c->subscribe("topic1", mqtt::qos::at_most_once);
                    break;
                case 6:
                    c->disconnect();
                    break;
                default:
                    BOOST_CHECK(false);
                    break;
                }
                return true;
            });
        c->set_publish_handler(
            [&order, &c, &pid_unsub]
            (std::uint8_t header,
             boost::optional<std::uint16_t> packet_id,
             std::string topic,
             std::string contents) {
                BOOST_TEST(mqtt::publish::is_dup(header) == false);
                BOOST_TEST(mqtt::publish::get_qos(header) == mqtt::qos::at_most_once);
                BOOST_CHECK(!packet_id);
                BOOST_TEST(topic == "topic1");
                BOOST_TEST(contents == "topic1_contents");
                pid_unsub = c->unsubscribe("topic1");
                switch (order++) {
                case 2:
                    BOOST_TEST(mqtt::publish::is_retain(header) == false);
                    break;
                case 5:
                    BOOST_TEST(mqtt::publish::is_retain(header) == true);
                    break;
                default:
                    BOOST_CHECK(false);
                    break;
                }
                return true;
            });
        c->connect();
        ios.run();
        BOOST_TEST(order++ == 8);
    };
    do_combi_test(test);
}


BOOST_AUTO_TEST_SUITE_END()
