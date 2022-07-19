#include <gtest/gtest.h>

#include <bm/bm_apps/packet_pipe.h>

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
            delete test_switch_old;
            delete test_switch_new;
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
const char RuntimeRegisterReconfigRehashTest::init_json_for_old[] = "old_SYN_flooding_protection_with_mark.p4.json";
const char RuntimeRegisterReconfigRehashTest::init_json_for_new[] = "new_SYN_flooding_protection_with_mark.p4.json";
