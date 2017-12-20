// Copyright Takatoshi Kondo 2015
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "combi_test.hpp"

BOOST_AUTO_TEST_SUITE(test_pubsub)

BOOST_AUTO_TEST_CASE( pub_qos0_sub_qos0 ) {
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
        c->set_pub_res_sent_handler(
            []
            (std::uint16_t) {
                BOOST_CHECK(false);
            });
        c->set_suback_handler(
            [&order, &c, &pid_sub]
            (std::uint16_t packet_id, std::vector<boost::optional<std::uint8_t>> results) {
                BOOST_TEST(order++ == 1);
                BOOST_TEST(packet_id == pid_sub);
                BOOST_TEST(results.size() == 1U);
                BOOST_TEST(*results[0] == mqtt::qos::at_most_once);
                c->publish_at_most_once("topic1", "topic1_contents");
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
                BOOST_TEST(mqtt::publish::is_retain(header) == false);
                BOOST_CHECK(!packet_id);
                BOOST_TEST(topic == "topic1");
                BOOST_TEST(contents == "topic1_contents");
                pid_unsub = c->unsubscribe("topic1");
                return true;
            });
        c->connect();
        ios.run();
        BOOST_TEST(order++ == 5);
    };
    do_combi_test(test);
}

BOOST_AUTO_TEST_CASE( pub_qos1_sub_qos0 ) {
    auto test = [](boost::asio::io_service& ios, auto& c, auto& s) {
        c->set_clean_session(true);
        bool pub_seq_finished = false;

        std::uint16_t pid_pub;
        std::uint16_t pid_sub;
        std::uint16_t pid_unsub;

        int order = 0;
        c->set_connack_handler(
            [&order, &c, &pid_sub]
            (bool sp, std::uint8_t connack_return_code) {
                BOOST_TEST(order++ == 0);
                BOOST_TEST(sp == false);
                BOOST_TEST(connack_return_code == mqtt::connect_return_code::accepted);
                pid_sub = c->subscribe(
                    std::vector<std::tuple<std::string, std::uint8_t>> {
                        std::make_tuple("topic1", mqtt::qos::at_most_once)
                            }
                );
                return true;
            });
        c->set_close_handler(
            [&order, &s]
            () {
                BOOST_TEST(order++ == 5);
                s.close();
            });
        c->set_error_handler(
            []
            (boost::system::error_code const&) {
                BOOST_CHECK(false);
            });
        c->set_puback_handler(
            [&order, &c, &pub_seq_finished, &pid_pub, &pid_unsub]
            (std::uint16_t packet_id) {
                BOOST_CHECK(order == 2 || order == 3);
                ++order;
                BOOST_TEST(packet_id == pid_pub);
                if (order == 4) {
                    pub_seq_finished = true;
                    pid_unsub = c->unsubscribe(
                        std::vector<std::string> {"topic1"});
                }
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
        c->set_pub_res_sent_handler(
            []
            (std::uint16_t) {
                BOOST_CHECK(false);
            });
        c->set_suback_handler(
            [&order, &c, &pid_pub, &pid_sub]
            (std::uint16_t packet_id, std::vector<boost::optional<std::uint8_t>> results) {
                BOOST_TEST(order++ == 1);
                BOOST_TEST(packet_id == pid_sub);
                BOOST_TEST(results.size() == 1U);
                BOOST_TEST(*results[0] == mqtt::qos::at_most_once);
                pid_pub = c->publish_at_least_once("topic1", "topic1_contents");
                return true;
            });
        c->set_unsuback_handler(
            [&order, &c, &pid_unsub]
            (std::uint16_t packet_id) {
                BOOST_TEST(order++ == 4);
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
                BOOST_CHECK(order == 2 || order == 3);
                ++order;
                BOOST_TEST(mqtt::publish::is_dup(header) == false);
                BOOST_TEST(mqtt::publish::get_qos(header) == mqtt::qos::at_most_once);
                BOOST_TEST(mqtt::publish::is_retain(header) == false);
                BOOST_TEST(!packet_id);
                BOOST_TEST(topic == "topic1");
                BOOST_TEST(contents == "topic1_contents");
                if (order == 4) pid_unsub = c->unsubscribe("topic1");
                return true;
            });
        c->connect();
        ios.run();
        BOOST_TEST(order++ == 6);
    };
    do_combi_test(test);
}

BOOST_AUTO_TEST_CASE( pub_qos2_sub_qos0 ) {
    auto test = [](boost::asio::io_service& ios, auto& c, auto& s) {
        c->set_clean_session(true);

        std::uint16_t pid_pub;
        std::uint16_t pid_sub;
        std::uint16_t pid_unsub;

        bool pub_seq_finished = false;
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
                BOOST_TEST(order++ == 6);
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
            [&order, &pid_pub]
            (std::uint16_t packet_id) {
                BOOST_CHECK(order == 2 || order == 3);
                ++order;
                BOOST_TEST(packet_id == pid_pub);
                return true;
            });
        c->set_pubcomp_handler(
            [&order, &c, &pub_seq_finished, &pid_unsub, &pid_pub]
            (std::uint16_t packet_id) {
                BOOST_CHECK(order == 3 || order == 4);
                ++order;
                BOOST_TEST(packet_id == pid_pub);
                if (order == 5) {
                    pub_seq_finished = true;
                    pid_unsub = c->unsubscribe("topic1");
                }
                return true;
            });
        c->set_pub_res_sent_handler(
            []
            (std::uint16_t) {
                BOOST_CHECK(false);
            });
        c->set_suback_handler(
            [&order, &c, &pid_pub]
            (std::uint16_t packet_id, std::vector<boost::optional<std::uint8_t>> results) {
                BOOST_TEST(order++ == 1);
                BOOST_TEST(packet_id == 1);
                BOOST_TEST(results.size() == 1U);
                BOOST_TEST(*results[0] == mqtt::qos::at_most_once);
                pid_pub = c->publish_exactly_once("topic1", "topic1_contents");
                return true;
            });
        c->set_unsuback_handler(
            [&order, &c, &pid_unsub]
            (std::uint16_t packet_id) {
                BOOST_TEST(order++ == 5);
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
                BOOST_CHECK(order == 2 || order == 3 || order == 4);
                ++order;
                BOOST_TEST(mqtt::publish::is_dup(header) == false);
                BOOST_TEST(mqtt::publish::get_qos(header) == mqtt::qos::at_most_once);
                BOOST_TEST(mqtt::publish::is_retain(header) == false);
                BOOST_CHECK(!packet_id);
                BOOST_TEST(topic == "topic1");
                BOOST_TEST(contents == "topic1_contents");
                if (order == 5) pid_unsub = c->unsubscribe("topic1");
                return true;
            });
        c->connect();
        ios.run();
        BOOST_TEST(order++ == 7);
    };
    do_combi_test(test);
}

BOOST_AUTO_TEST_CASE( pub_qos0_sub_qos1 ) {
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
                pid_sub = c->subscribe("topic1", mqtt::qos::at_least_once);
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
        c->set_pub_res_sent_handler(
            []
            (std::uint16_t) {
                BOOST_CHECK(false);
            });
        c->set_suback_handler(
            [&order, &c, &pid_sub]
            (std::uint16_t packet_id, std::vector<boost::optional<std::uint8_t>> results) {
                BOOST_TEST(order++ == 1);
                BOOST_TEST(packet_id == pid_sub);
                BOOST_TEST(results.size() == 1U);
                BOOST_TEST(*results[0] == mqtt::qos::at_least_once);
                c->publish_at_most_once("topic1", "topic1_contents");
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
                BOOST_TEST(mqtt::publish::is_retain(header) == false);
                BOOST_CHECK(!packet_id);
                BOOST_TEST(topic == "topic1");
                BOOST_TEST(contents == "topic1_contents");
                pid_unsub = c->unsubscribe("topic1");
                return true;
            });
        c->connect();
        ios.run();
        BOOST_TEST(order++ == 5);
    };
    do_combi_test(test);
}

BOOST_AUTO_TEST_CASE( pub_qos1_sub_qos1 ) {
    auto test = [](boost::asio::io_service& ios, auto& c, auto& s) {
        c->set_clean_session(true);

        std::uint16_t pid_pub;
        std::uint16_t pid_sub;
        std::uint16_t pid_unsub;

        bool pub_seq_finished = false;
        int order = 0;
        c->set_connack_handler(
            [&order, &c, &pid_sub]
            (bool sp, std::uint8_t connack_return_code) {
                BOOST_TEST(order++ == 0);
                BOOST_TEST(sp == false);
                BOOST_TEST(connack_return_code == mqtt::connect_return_code::accepted);
                pid_sub = c->subscribe("topic1", mqtt::qos::at_least_once);
                return true;
            });
        c->set_close_handler(
            [&order, &s]
            () {
                BOOST_TEST(order++ == 6);
                s.close();
            });
        c->set_error_handler(
            []
            (boost::system::error_code const&) {
                BOOST_CHECK(false);
            });
        c->set_puback_handler(
            [&order, &c, &pub_seq_finished, &pid_pub, &pid_unsub]
            (std::uint16_t packet_id) {
                BOOST_CHECK(order == 2 || order == 3 || order == 4);
                ++order;
                BOOST_TEST(packet_id == pid_pub);
                if (order == 5) {
                    pub_seq_finished = true;
                    pid_unsub = c->unsubscribe("topic1");
                }
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
        boost::optional<std::uint16_t> recv_packet_id;
        c->set_pub_res_sent_handler(
            [&order, &c, &pid_unsub, &recv_packet_id]
            (std::uint16_t packet_id) {
                BOOST_CHECK(order == 3 || order == 4);
                ++order;
                BOOST_TEST(*recv_packet_id == packet_id);
                if (order == 5) pid_unsub = c->unsubscribe("topic1");
            });
        c->set_suback_handler(
            [&order, &c, &pid_sub, &pid_pub]
            (std::uint16_t packet_id, std::vector<boost::optional<std::uint8_t>> results) {
                BOOST_TEST(order++ == 1);
                BOOST_TEST(packet_id == pid_sub);
                BOOST_TEST(results.size() == 1U);
                BOOST_TEST(*results[0] == mqtt::qos::at_least_once);
                pid_pub = c->publish_at_least_once("topic1", "topic1_contents");
                return true;
            });
        c->set_unsuback_handler(
            [&order, &c, &pid_unsub]
            (std::uint16_t packet_id) {
                BOOST_CHECK(order == 5);
                ++order;
                BOOST_TEST(packet_id == pid_unsub);
                c->disconnect();
                return true;
            });
        c->set_publish_handler(
            [&order, &recv_packet_id]
            (std::uint8_t header,
             boost::optional<std::uint16_t> packet_id,
             std::string topic,
             std::string contents) {
                BOOST_CHECK(order == 2 || order == 3);
                ++order;
                BOOST_TEST(mqtt::publish::is_dup(header) == false);
                BOOST_TEST(mqtt::publish::get_qos(header) == mqtt::qos::at_least_once);
                BOOST_TEST(mqtt::publish::is_retain(header) == false);
                BOOST_TEST(*packet_id != 0);
                recv_packet_id = packet_id;
                BOOST_TEST(topic == "topic1");
                BOOST_TEST(contents == "topic1_contents");
                return true;
            });
        c->connect();
        ios.run();
        BOOST_TEST(order++ == 7);
    };
    do_combi_test(test);
}

BOOST_AUTO_TEST_CASE( pub_qos2_sub_qos1 ) {
    auto test = [](boost::asio::io_service& ios, auto& c, auto& s) {
        c->set_clean_session(true);

        std::uint16_t pid_pub;
        std::uint16_t pid_sub;
        std::uint16_t pid_unsub;

        bool pub_seq_finished = false;
        int order = 0;
        c->set_connack_handler(
            [&order, &c, &pid_sub]
            (bool sp, std::uint8_t connack_return_code) {
                BOOST_TEST(order++ == 0);
                BOOST_TEST(sp == false);
                BOOST_TEST(connack_return_code == mqtt::connect_return_code::accepted);
                pid_sub = c->subscribe("topic1", mqtt::qos::at_least_once);
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
            [&order, &pid_pub]
            (std::uint16_t packet_id) {
                BOOST_CHECK(order == 2 || order == 3 || order == 4);
                ++order;
                BOOST_TEST(packet_id == pid_pub);
                return true;
            });
        c->set_pubcomp_handler(
            [&order, &c, &pub_seq_finished, &pid_pub, &pid_unsub]
            (std::uint16_t packet_id) {
                BOOST_CHECK(order == 3 || order == 4 || order == 5);
                ++order;
                BOOST_TEST(packet_id == pid_pub);
                if (order == 6) {
                    pub_seq_finished = true;
                    pid_unsub = c->unsubscribe("topic1");
                }
                return true;
            });
        boost::optional<std::uint16_t> recv_packet_id;
        c->set_pub_res_sent_handler(
            [&order, &c, &pid_unsub, &recv_packet_id]
            (std::uint16_t packet_id) {
                BOOST_CHECK(order == 3 || order == 4 || order == 5);
                ++order;
                BOOST_TEST(*recv_packet_id == packet_id);
                if (order == 6) pid_unsub = c->unsubscribe("topic1");
            });
        c->set_suback_handler(
            [&order, &c, &pid_sub, &pid_pub]
            (std::uint16_t packet_id, std::vector<boost::optional<std::uint8_t>> results) {
                BOOST_TEST(order++ == 1);
                BOOST_TEST(packet_id == pid_sub);
                BOOST_TEST(results.size() == 1U);
                BOOST_TEST(*results[0] == mqtt::qos::at_least_once);
                pid_pub = c->publish_exactly_once("topic1", "topic1_contents");
                return true;
            });
        c->set_unsuback_handler(
            [&order, &c, &pid_unsub]
            (std::uint16_t packet_id) {
                BOOST_CHECK(order == 6);
                ++order;
                BOOST_TEST(packet_id == pid_unsub);
                c->disconnect();
                return true;
            });
        c->set_publish_handler(
            [&order, &recv_packet_id]
            (std::uint8_t header,
             boost::optional<std::uint16_t> packet_id,
             std::string topic,
             std::string contents) {
                BOOST_CHECK(order == 2 || order == 3 || order == 4);
                ++order;
                BOOST_TEST(mqtt::publish::is_dup(header) == false);
                BOOST_TEST(mqtt::publish::get_qos(header) == mqtt::qos::at_least_once);
                BOOST_TEST(mqtt::publish::is_retain(header) == false);
                BOOST_TEST(*packet_id != 0);
                recv_packet_id = packet_id;
                BOOST_TEST(topic == "topic1");
                BOOST_TEST(contents == "topic1_contents");
                return true;
            });
        c->connect();
        ios.run();
        BOOST_TEST(order++ == 8);
    };
    do_combi_test(test);
}

BOOST_AUTO_TEST_CASE( pub_qos0_sub_qos2 ) {
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
                pid_sub = c->subscribe("topic1", mqtt::qos::exactly_once);
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
        c->set_pub_res_sent_handler(
            []
            (std::uint16_t) {
                BOOST_CHECK(false);
            });
        c->set_suback_handler(
            [&order, &c, &pid_sub]
            (std::uint16_t packet_id, std::vector<boost::optional<std::uint8_t>> results) {
                BOOST_TEST(order++ == 1);
                BOOST_TEST(packet_id == pid_sub);
                BOOST_TEST(results.size() == 1U);
                BOOST_TEST(*results[0] == mqtt::qos::exactly_once);
                c->publish_at_most_once("topic1", "topic1_contents");
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
                BOOST_TEST(mqtt::publish::is_retain(header) == false);
                BOOST_CHECK(!packet_id);
                BOOST_TEST(topic == "topic1");
                BOOST_TEST(contents == "topic1_contents");
                pid_unsub = c->unsubscribe("topic1");
                return true;
            });
        c->connect();
        ios.run();
        BOOST_TEST(order++ == 5);
    };
    do_combi_test(test);
}

BOOST_AUTO_TEST_CASE( pub_qos1_sub_qos2 ) {
    auto test = [](boost::asio::io_service& ios, auto& c, auto& s) {
        c->set_clean_session(true);

        std::uint16_t pid_pub;
        std::uint16_t pid_sub;
        std::uint16_t pid_unsub;

        bool pub_seq_finished = false;
        int order = 0;
        c->set_connack_handler(
            [&order, &c, &pid_sub]
            (bool sp, std::uint8_t connack_return_code) {
                BOOST_TEST(order++ == 0);
                BOOST_TEST(sp == false);
                BOOST_TEST(connack_return_code == mqtt::connect_return_code::accepted);
                pid_sub = c->subscribe("topic1", mqtt::qos::exactly_once);
                return true;
            });
        c->set_close_handler(
            [&order, &s]
            () {
                BOOST_TEST(order++ == 6);
                s.close();
            });
        c->set_error_handler(
            []
            (boost::system::error_code const&) {
                BOOST_CHECK(false);
            });
        c->set_puback_handler(
            [&order, &c, &pub_seq_finished, &pid_pub, &pid_unsub]
            (std::uint16_t packet_id) {
                BOOST_CHECK(order == 2 || order == 3 || order == 4);
                ++order;
                BOOST_TEST(packet_id == pid_pub);
                if (order == 5) {
                    pub_seq_finished = true;
                    pid_unsub = c->unsubscribe("topic1");
                }
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
        boost::optional<std::uint16_t> recv_packet_id;
        c->set_pub_res_sent_handler(
            [&order, &c, &pid_unsub, &recv_packet_id]
            (std::uint16_t packet_id) {
                BOOST_CHECK(order == 3 || order == 4);
                ++order;
                BOOST_TEST(*recv_packet_id == packet_id);
                if (order == 5) pid_unsub = c->unsubscribe("topic1");
            });
        c->set_suback_handler(
            [&order, &c, &pid_sub, &pid_pub]
            (std::uint16_t packet_id, std::vector<boost::optional<std::uint8_t>> results) {
                BOOST_TEST(order++ == 1);
                BOOST_TEST(packet_id == pid_sub);
                BOOST_TEST(results.size() == 1U);
                BOOST_TEST(*results[0] == mqtt::qos::exactly_once);
                pid_pub = c->publish_at_least_once("topic1", "topic1_contents");
                return true;
            });
        c->set_unsuback_handler(
            [&order, &c, &pid_unsub]
            (std::uint16_t packet_id) {
                BOOST_CHECK(order == 5);
                ++order;
                BOOST_TEST(packet_id == pid_unsub);
                c->disconnect();
                return true;
            });
        c->set_publish_handler(
            [&order, &recv_packet_id]
            (std::uint8_t header,
             boost::optional<std::uint16_t> packet_id,
             std::string topic,
             std::string contents) {
                BOOST_CHECK(order == 2 || order == 3);
                ++order;
                BOOST_TEST(mqtt::publish::is_dup(header) == false);
                BOOST_TEST(mqtt::publish::get_qos(header) == mqtt::qos::at_least_once);
                BOOST_TEST(mqtt::publish::is_retain(header) == false);
                BOOST_TEST(*packet_id != 0);
                recv_packet_id = packet_id;
                BOOST_TEST(topic == "topic1");
                BOOST_TEST(contents == "topic1_contents");
                return true;
            });
        c->connect();
        ios.run();
        BOOST_TEST(order++ == 7);
    };
    do_combi_test(test);
}

BOOST_AUTO_TEST_CASE( pub_qos2_sub_qos2 ) {
    auto test = [](boost::asio::io_service& ios, auto& c, auto& s) {
        c->set_clean_session(true);

        std::uint16_t pid_pub;
        std::uint16_t pid_sub;
        std::uint16_t pid_unsub;

        bool pub_seq_finished = false;
        int order = 0;
        c->set_connack_handler(
            [&order, &c, &pid_sub]
            (bool sp, std::uint8_t connack_return_code) {
                BOOST_TEST(order++ == 0);
                BOOST_TEST(sp == false);
                BOOST_TEST(connack_return_code == mqtt::connect_return_code::accepted);
                pid_sub = c->subscribe("topic1", mqtt::qos::exactly_once);
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
            [&order, &pid_pub]
            (std::uint16_t packet_id) {
                BOOST_CHECK(order == 2 || order == 3);
                ++order;
                BOOST_TEST(packet_id == pid_pub);
                return true;
            });
        c->set_pubcomp_handler(
            [&order, &c, &pub_seq_finished, &pid_pub, &pid_unsub]
            (std::uint16_t packet_id) {
                BOOST_CHECK(order == 3 || order == 4 || order == 5);
                ++order;
                BOOST_TEST(packet_id == pid_pub);
                if (order == 6) {
                    pub_seq_finished = true;
                    pid_unsub = c->unsubscribe("topic1");
                }
                return true;
            });
        boost::optional<std::uint16_t> recv_packet_id;
        c->set_pub_res_sent_handler(
            [&order, &c, &pid_unsub, &recv_packet_id]
            (std::uint16_t packet_id) {
                BOOST_CHECK(order == 3 || order == 4 || order == 5);
                ++order;
                BOOST_TEST(*recv_packet_id == packet_id);
                if (order == 6) pid_unsub = c->unsubscribe("topic1");
            });
        c->set_suback_handler(
            [&order, &c, &pid_sub, &pid_pub]
            (std::uint16_t packet_id, std::vector<boost::optional<std::uint8_t>> results) {
                BOOST_TEST(order++ == 1);
                BOOST_TEST(packet_id == pid_sub);
                BOOST_TEST(results.size() == 1U);
                BOOST_TEST(*results[0] == mqtt::qos::exactly_once);
                pid_pub = c->publish_exactly_once("topic1", "topic1_contents");
                return true;
            });
        c->set_unsuback_handler(
            [&order, &c, &pid_unsub]
            (std::uint16_t packet_id) {
                BOOST_CHECK(order == 6);
                ++order;
                BOOST_TEST(packet_id == pid_unsub);
                c->disconnect();
                return true;
            });
        c->set_publish_handler(
            [&order, &recv_packet_id]
            (std::uint8_t header,
             boost::optional<std::uint16_t> packet_id,
             std::string topic,
             std::string contents) {
                BOOST_CHECK(order == 2 || order == 3 || order == 4);
                ++order;
                BOOST_TEST(mqtt::publish::is_dup(header) == false);
                BOOST_TEST(mqtt::publish::get_qos(header) == mqtt::qos::exactly_once);
                BOOST_TEST(mqtt::publish::is_retain(header) == false);
                BOOST_TEST(*packet_id != 0);
                recv_packet_id = packet_id;
                BOOST_TEST(topic == "topic1");
                BOOST_TEST(contents == "topic1_contents");
                return true;
            });
        c->connect();
        ios.run();
        BOOST_TEST(order++ == 8);
    };
    do_combi_test(test);
}

BOOST_AUTO_TEST_CASE( publish_function ) {
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
            [&order, &c, &pid_sub]
            (std::uint16_t packet_id, std::vector<boost::optional<std::uint8_t>> results) {
                BOOST_TEST(order++ == 1);
                BOOST_TEST(packet_id == pid_sub);
                BOOST_TEST(results.size() == 1U);
                BOOST_TEST(*results[0] == mqtt::qos::at_most_once);
                c->publish("topic1", "topic1_contents", mqtt::qos::at_most_once);
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
                BOOST_TEST(mqtt::publish::is_retain(header) == false);
                BOOST_CHECK(!packet_id);
                BOOST_TEST(topic == "topic1");
                BOOST_TEST(contents == "topic1_contents");
                pid_unsub = c->unsubscribe("topic1");
                return true;
            });
        c->connect();
        ios.run();
        BOOST_TEST(order++ == 5);
    };
    do_combi_test(test);
}


BOOST_AUTO_TEST_SUITE_END()
