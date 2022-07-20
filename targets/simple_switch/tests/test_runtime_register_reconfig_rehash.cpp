#include <gtest/gtest.h>

#include <bm/bm_apps/packet_pipe.h>
#include <bm/bm_sim/data.h>

#include <string>
#include <memory>
#include <vector>
#include <algorithm>  // for std::is_sorted

#include <boost/filesystem.hpp>

#include "simple_switch.h"

#include "utils.h"

namespace fs = boost::filesystem;

using bm::MatchErrorCode;
using bm::ActionData;
using bm::MatchKeyParam;
using bm::entry_handle_t;

namespace {
    void
    packet_handler(int port_num, const char *buffer, int len, void *cookie) {
        static_cast<SimpleSwitch *>(cookie)->receive(port_num, buffer, len);
    }
}

class RuntimeRegisterReconfigRehashTest : public ::testing::Test {
    protected:
        RuntimeRegisterReconfigRehashTest()
            : packet_inject_for_old(packet_in_addr_for_old), 
              packet_inject_for_new(packet_in_addr_for_new) 
        { }

        static void SetUpTestCase() {
            // create and start old switch
            test_switch_old = new SimpleSwitch();

            fs::path json_path_for_old = 
                fs::path(testdata_dir) / fs::path(testdata_folder) / fs::path(init_json_for_old);
            test_switch_old->init_objects(json_path_for_old.string());

            test_switch_old->set_dev_mgr_packet_in(0, packet_in_addr_for_old, nullptr);
            test_switch_old->Switch::start();
            test_switch_old->set_packet_handler(packet_handler,
                                                static_cast<void*>(test_switch_old));
            test_switch_old->start_and_return();

            // create and start new switch
            test_switch_new = new SimpleSwitch();

            fs::path json_path_for_new = 
                fs::path(testdata_dir) / fs::path(testdata_folder) / fs::path(init_json_for_new);
            test_switch_new->init_objects(json_path_for_new.string());

            test_switch_new->set_dev_mgr_packet_in(1, packet_in_addr_for_new, nullptr);
            test_switch_new->Switch::start();
            test_switch_new->set_packet_handler(packet_handler,
                                                static_cast<void*>(test_switch_new));
            test_switch_new->start_and_return();
        }

        static void TearDownTestCase() {
            // if delete swith, there will be an error
            // i don't know why

            // delete test_switch_old;
            // delete test_switch_new;
        }

        virtual void SetUp() override {
            // configure old switch
            packet_inject_for_old.start();
            auto receiver_func_for_old = std::bind(&PacketInReceiver::receive, &receiver_for_old,
                        std::placeholders::_1, std::placeholders::_2,
                        std::placeholders::_3, std::placeholders::_4);
            packet_inject_for_old.set_packet_receiver(receiver_func_for_old, nullptr);

            init_switch_content_populate(test_switch_old);

            // configure new switch
            packet_inject_for_new.start();
            auto receiver_func_for_new = std::bind(&PacketInReceiver::receive, &receiver_for_new,
                        std::placeholders::_1, std::placeholders::_2,
                        std::placeholders::_3, std::placeholders::_4);
            packet_inject_for_new.set_packet_receiver(receiver_func_for_new, nullptr);

            init_switch_content_populate(test_switch_new);
        }

        virtual void TearDown() override {
            test_switch_old->reset_state();
            test_switch_new->reset_state();
        }

        void init_switch_content_populate(SimpleSwitch* sw) {
            // config table MyIngress.check_ports
            {
                // table_add check_ports set_direction 1 3 => 0
                std::vector<MatchKeyParam> match_key_0;
                match_key_0.emplace_back(MatchKeyParam::Type::EXACT, std::string("\x00\x01", 2));
                match_key_0.emplace_back(MatchKeyParam::Type::EXACT, std::string("\x00\x03", 2));
                ActionData data_0;
                data_0.push_back_action_data(0);
                entry_handle_t handle_0;
                sw->mt_add_entry(0, "MyIngress.check_ports", 
                                    match_key_0, "MyIngress.set_direction", 
                                    std::move(data_0), &handle_0);
            }
           
            {
                // table_add check_ports set_direction 2 3 => 0
                std::vector<MatchKeyParam> match_key_1;
                match_key_1.emplace_back(MatchKeyParam::Type::EXACT, std::string("\x00\x02", 2));
                match_key_1.emplace_back(MatchKeyParam::Type::EXACT, std::string("\x00\x03", 2));
                ActionData data_1;
                data_1.push_back_action_data(0);
                entry_handle_t handle_1;
                sw->mt_add_entry(0, "MyIngress.check_ports", 
                                    match_key_1, "MyIngress.set_direction", 
                                    std::move(data_1), &handle_1);
            }
            
            {
                // table_add check_ports set_direction 3 1 => 1
                std::vector<MatchKeyParam> match_key_2;
                match_key_2.emplace_back(MatchKeyParam::Type::EXACT, std::string("\x00\x03", 2));
                match_key_2.emplace_back(MatchKeyParam::Type::EXACT, std::string("\x00\x01", 2));
                ActionData data_2;
                data_2.push_back_action_data(1);
                entry_handle_t handle_2;
                sw->mt_add_entry(0, "MyIngress.check_ports", 
                                    match_key_2, "MyIngress.set_direction", 
                                    std::move(data_2), &handle_2);
            }
            
            {
                // table_add check_ports set_direction 3 2 => 1
                std::vector<MatchKeyParam> match_key_3;
                match_key_3.emplace_back(MatchKeyParam::Type::EXACT, std::string("\x00\x03", 2));
                match_key_3.emplace_back(MatchKeyParam::Type::EXACT, std::string("\x00\x02", 2));
                ActionData data_3;
                data_3.push_back_action_data(1);
                entry_handle_t handle_3;
                sw->mt_add_entry(0, "MyIngress.check_ports", 
                                    match_key_3, "MyIngress.set_direction", 
                                    std::move(data_3), &handle_3);
            }
           

            // config table MyIngress.ipv4_lpm
            {
                // table_add ipv4_lpm ipv4_forward 10.0.1.10/32 => 00:04:00:00:00:01 1
                std::vector<MatchKeyParam> match_key_4;
                match_key_4.emplace_back(MatchKeyParam::Type::LPM,
                                            std::string("\x0a\x00\x01\x0a", 4), 32);
                ActionData data_4;
                data_4.push_back_action_data("\x00\x04\x00\x00\x00\x01", 6);
                data_4.push_back_action_data(1);
                entry_handle_t handle_4;
                sw->mt_add_entry(0, "MyIngress.ipv4_lpm", match_key_4, 
                                    "MyIngress.ipv4_forward", data_4, &handle_4);
            }
           
            {
                // table_add ipv4_lpm ipv4_forward 10.0.1.21/32 => 00:04:00:00:00:02 2
                std::vector<MatchKeyParam> match_key_5;
                match_key_5.emplace_back(MatchKeyParam::Type::LPM,
                                            std::string("\x0a\x00\x01\x15", 4), 32);
                ActionData data_5;
                data_5.push_back_action_data("\x00\x04\x00\x00\x00\x02", 6);
                data_5.push_back_action_data(2);
                entry_handle_t handle_5;
                sw->mt_add_entry(0, "MyIngress.ipv4_lpm", match_key_5, 
                                    "MyIngress.ipv4_forward", data_5, &handle_5);
            }
            
            {
                // table_add ipv4_lpm ipv4_forward 10.0.1.22/32 => 00:04:00:00:00:03 3
                std::vector<MatchKeyParam> match_key_6;
                match_key_6.emplace_back(MatchKeyParam::Type::LPM,
                                            std::string("\x0a\x00\x01\x16", 4), 32);
                ActionData data_6;
                data_6.push_back_action_data("\x00\x04\x00\x00\x00\x03", 6);
                data_6.push_back_action_data(3);
                entry_handle_t handle_6;
                sw->mt_add_entry(0, "MyIngress.ipv4_lpm", match_key_6, 
                                    "MyIngress.ipv4_forward", data_6, &handle_6);
            }
           
            {
                // table_add ipv4_lpm ipv4_forward 192.168.1.0/24 => 00:04:00:00:00:02 2
                std::vector<MatchKeyParam> match_key_7;
                match_key_7.emplace_back(MatchKeyParam::Type::LPM,
                                            std::string("\xc0\xa8\x01\x00", 4), 24);
                ActionData data_7;
                data_7.push_back_action_data("\x00\x04\x00\x00\x00\x02", 6);
                data_7.push_back_action_data(2);
                entry_handle_t handle_7;
                sw->mt_add_entry(0, "MyIngress.ipv4_lpm", match_key_7, 
                                    "MyIngress.ipv4_forward", data_7, &handle_7);
            }
           
        }

    protected:
        static const char attack_pkt_bin[];

        static const char packet_in_addr_for_old[];
        static const char packet_in_addr_for_new[];

        static SimpleSwitch* test_switch_old;
        static SimpleSwitch* test_switch_new;
        
        bm_apps::PacketInject packet_inject_for_old;
        bm_apps::PacketInject packet_inject_for_new;

        PacketInReceiver receiver_for_old{};
        PacketInReceiver receiver_for_new{};
        
        static const char testdata_dir[];
        static const char testdata_folder[];
        static const char init_json_for_old[];
        static const char init_json_for_new[];
        static const char plan_file[];
};

// pkt = Ether(src="00:00:00:00:00:00", dst="ff:ff:ff:ff:ff:ff") / 
//          IP(src="192.168.7.2", dst="10.0.1.22") / 
//              TCP(dport=1234, sport=49153, flags="S")
const char RuntimeRegisterReconfigRehashTest::attack_pkt_bin[] = 
    "\xff\xff\xff\xff\xff\xff\x00\x00\x00\x00\x00\x00\x08\x00E\x00\x00"
    "(\x00\x01\x00\x00@\x06\xa8\x0f\xc0\xa8\x07\x02\n\x00\x01\x16\xc0"
    "\x01\x04\xd2\x00\x00\x00\x00\x00\x00\x00\x00P\x02 \x00\xf8N\x00\x00";

const char RuntimeRegisterReconfigRehashTest::packet_in_addr_for_old[] = "inproc://packets_for_old";
const char RuntimeRegisterReconfigRehashTest::packet_in_addr_for_new[] = "inproc://packets_for_new";

SimpleSwitch* RuntimeRegisterReconfigRehashTest::test_switch_old = nullptr;
SimpleSwitch* RuntimeRegisterReconfigRehashTest::test_switch_new = nullptr;

const char RuntimeRegisterReconfigRehashTest::testdata_dir[] = TESTDATADIR;
const char RuntimeRegisterReconfigRehashTest::testdata_folder[] = "runtime_register_reconfig";
const char RuntimeRegisterReconfigRehashTest::init_json_for_old[] = "old_SYN_flooding_protection.json";
const char RuntimeRegisterReconfigRehashTest::init_json_for_new[] = "new_SYN_flooding_protection.json";
const char RuntimeRegisterReconfigRehashTest::plan_file[] = "reconfiguration_command.txt";

TEST_F(RuntimeRegisterReconfigRehashTest, RehashCheck) {
    constexpr int port_in = 1;
    constexpr int send_recv_times = 3;

    for (int i = 0; i < send_recv_times; i++) {
        packet_inject_for_old.send(port_in, attack_pkt_bin, sizeof(attack_pkt_bin));

        int recv_port = -1;
        char recv_buffer[1024];
        receiver_for_old.read(recv_buffer, sizeof(recv_buffer), &recv_port);
    }

    fs::path json_path_for_new = 
                fs::path(testdata_dir) / fs::path(testdata_folder) / fs::path(init_json_for_new);

    fs::path plan_file_path = 
                fs::path(testdata_dir) / fs::path(testdata_folder) / fs::path(plan_file);

    test_switch_old->mt_runtime_reconfig(0, json_path_for_new.string(), plan_file_path.string());

    std::vector<bm::Data> bloom_filter_after_reconfig = 
        test_switch_old->register_read_all(0, "defence_bloom_filter_for_ip_src");
   
    for (int i = 0; i < send_recv_times; i++) {
        packet_inject_for_new.send(port_in, attack_pkt_bin, sizeof(attack_pkt_bin));

        int recv_port = -1;
        char recv_buffer[1024];
        receiver_for_new.read(recv_buffer, sizeof(recv_buffer), &recv_port);
    }

    std::vector<bm::Data> bloom_filter_for_new_switch = 
        test_switch_new->register_read_all(0, "defence_bloom_filter_for_ip_src");

    ASSERT_EQ(bloom_filter_after_reconfig.size(), bloom_filter_for_new_switch.size());
    for (size_t i = 0; i < bloom_filter_for_new_switch.size(); i++) {
        ASSERT_TRUE(bloom_filter_after_reconfig[i].get<uint32_t>() == 
                        bloom_filter_for_new_switch[i].get<uint32_t>());
    }
}
