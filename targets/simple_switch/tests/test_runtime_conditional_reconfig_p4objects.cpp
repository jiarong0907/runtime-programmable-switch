#include <gtest/gtest.h>

#include <bm/bm_apps/packet_pipe.h>
#include <bm/bm_sim/data.h>
#include <bm/bm_sim/match_error_codes.h>
#include <bm/bm_sim/logger.h>

#include <string>
#include <memory>
#include <vector>
#include <algorithm>  // for std::is_sorted

#include <boost/filesystem.hpp>

#include "simple_switch.h"

#include "utils.h"

namespace fs = boost::filesystem;

using bm::Conditional;
using bm::RuntimeReconfigErrorCode;

namespace {
    void
    packet_handler(int port_num, const char *buffer, int len, void *cookie) {
        static_cast<SimpleSwitch *>(cookie)->receive(port_num, buffer, len);
    }
}

class RuntimeConditionalReconfigP4ObjectsTest : public ::testing::Test {
    protected:
        size_t cxt_id{0};
        size_t conditional_name_max{0};
        size_t ori_conditionals_num{0};
        std::ifstream* new_json_file_stream;
        RuntimeConditionalReconfigP4ObjectsTest() 
            : new_json_file_stream(nullptr)
        { }

        static void SetUpTestCase() {
            sw = new SimpleSwitch();

            fs::path init_json_path = fs::path(testdata_dir) / fs::path(testdata_folder) / fs::path(init_json);
            _BM_ASSERT(sw->init_objects(init_json_path.string()) == 0);

            sw->set_dev_mgr_packet_in(0, "inproc://packets", nullptr);
            sw->Switch::start();
            sw->set_packet_handler(packet_handler, static_cast<void*>(sw));
            sw->start_and_return();
        }

        virtual void SetUp() override {
            fs::path init_json_path = fs::path(testdata_dir) / fs::path(testdata_folder) / fs::path(init_json);
            std::ifstream init_json_stream(init_json_path.string(), std::ios::in);
            _BM_ASSERT(init_json_stream);

            Json::Value cfg_root;
            init_json_stream >> cfg_root;
            Json::Value& cfg_pipelines = cfg_root["pipelines"];
            Json::Value& cfg_ingress_pipeline = cfg_pipelines[0];
            _BM_ASSERT(cfg_ingress_pipeline["name"].asString() == "ingress");

            Json::Value& cfg_conditionals = cfg_ingress_pipeline["conditionals"];
            ori_conditionals_num = cfg_conditionals.size();

            for (const auto& cfg_conditional : cfg_conditionals) {
                size_t conditional_name_index = (size_t) std::stoi(cfg_conditional["name"].asString().substr(5));
                if (conditional_name_index > conditional_name_max) {
                    conditional_name_max = conditional_name_index;
                }
            }

            fs::path new_json_file_path = fs::path(testdata_dir) / fs::path(testdata_folder) / fs::path(new_json);
            new_json_file_stream = new std::ifstream(new_json_file_path.string(), std::ios::in);
        }

        virtual void TearDown() override {
            delete new_json_file_stream;
        }

        static void TearDownTestCase() {

        }

        static SimpleSwitch* sw;

        static const char* testdata_dir;
        static const char* testdata_folder;

        static const char* init_json;
        static const char* new_json;
};

const char* RuntimeConditionalReconfigP4ObjectsTest::testdata_dir = TESTDATADIR;
const char* RuntimeConditionalReconfigP4ObjectsTest::testdata_folder = "runtime_conditional_reconfig";

const char* RuntimeConditionalReconfigP4ObjectsTest::init_json = "runtime_conditional_reconfig_init.json";
const char* RuntimeConditionalReconfigP4ObjectsTest::new_json = "runtime_conditional_reconfig_new.json";

SimpleSwitch* RuntimeConditionalReconfigP4ObjectsTest::sw = nullptr;

TEST_F(RuntimeConditionalReconfigP4ObjectsTest, InsertCheck) {
    std::istringstream reconfig_commands_ss("insert cond ingress new_node_6");

    ASSERT_EQ(RuntimeReconfigErrorCode::SUCCESS, 
        static_cast<RuntimeReconfigErrorCode>(
            sw->mt_runtime_reconfig_with_stream(cxt_id, 
                                                new_json_file_stream,
                                                &reconfig_commands_ss)));

    P4Objects* p4objects_rt = sw->get_p4objects_rt();

    std::string expected_added_cond_name = "node_" + std::to_string(conditional_name_max + 1);

    ASSERT_NO_THROW(p4objects_rt->get_conditional(expected_added_cond_name));
    Conditional* added_cond = p4objects_rt->get_conditional(expected_added_cond_name);

    ASSERT_EQ(expected_added_cond_name, added_cond->get_name());
    ASSERT_EQ(ori_conditionals_num, added_cond->get_id());

    ASSERT_NE(nullptr, p4objects_rt->get_json_value("ingress conditional", expected_added_cond_name));
    const Json::Value* added_cond_json_value_in_map = p4objects_rt->get_json_value("ingress conditional", expected_added_cond_name);

    ASSERT_EQ(expected_added_cond_name, (*added_cond_json_value_in_map)["name"].asString());
    ASSERT_EQ(ori_conditionals_num, (*added_cond_json_value_in_map)["id"].asInt());

    const Json::Value* added_cond_json_value_in_cfg = nullptr;
    for (const auto& cfg_cond : p4objects_rt->get_cfg()["pipelines"][0]["conditionals"]) {
        if (cfg_cond["name"].asString() == expected_added_cond_name) {
            added_cond_json_value_in_cfg = &cfg_cond;
            break;
        }
    }

    ASSERT_NE(nullptr, added_cond_json_value_in_cfg);
    ASSERT_EQ(added_cond_json_value_in_map, added_cond_json_value_in_cfg);

    ASSERT_EQ(nullptr, added_cond->get_next_node_if_true());
    ASSERT_EQ(nullptr, added_cond->get_next_node_if_false());
}

TEST_F(RuntimeConditionalReconfigP4ObjectsTest, ChangeTrueNextCheck) {
    std::istringstream reconfig_commands_ss("change cond ingress old_node_4 true_next old_MyIngress.mark_tos");

    ASSERT_EQ(RuntimeReconfigErrorCode::SUCCESS, 
        static_cast<RuntimeReconfigErrorCode>(
            sw->mt_runtime_reconfig_with_stream(cxt_id, 
                                                new_json_file_stream,
                                                &reconfig_commands_ss)));

    P4Objects* p4objects_rt = sw->get_p4objects_rt();
    ASSERT_NO_THROW(p4objects_rt->get_conditional("node_4"));

    Conditional* changed_conditional = p4objects_rt->get_conditional("node_4");
    ASSERT_EQ("MyIngress.mark_tos", changed_conditional->get_next_node_if_true()->get_name());

    ASSERT_NE(nullptr, p4objects_rt->get_json_value("ingress conditional", "node_4"));
    const Json::Value* changed_conditional_json_value_in_map = p4objects_rt->get_json_value("ingress conditional", "node_4");

    ASSERT_EQ("MyIngress.mark_tos", (*changed_conditional_json_value_in_map)["true_next"].asString());
}

TEST_F(RuntimeConditionalReconfigP4ObjectsTest, ChangeFalseNextCheck) {
    std::istringstream reconfig_commands_ss("change cond ingress old_node_4 false_next old_MyIngress.mark_tos");

    ASSERT_EQ(RuntimeReconfigErrorCode::SUCCESS, 
        static_cast<RuntimeReconfigErrorCode>(
            sw->mt_runtime_reconfig_with_stream(cxt_id, 
                                                new_json_file_stream,
                                                &reconfig_commands_ss)));

    P4Objects* p4objects_rt = sw->get_p4objects_rt();
    ASSERT_NO_THROW(p4objects_rt->get_conditional("node_4"));

    Conditional* changed_conditional = p4objects_rt->get_conditional("node_4");
    ASSERT_EQ("MyIngress.mark_tos", changed_conditional->get_next_node_if_false()->get_name());

    ASSERT_NE(nullptr, p4objects_rt->get_json_value("ingress conditional", "node_4"));
    const Json::Value* changed_conditional_json_value_in_map = p4objects_rt->get_json_value("ingress conditional", "node_4");

    ASSERT_EQ("MyIngress.mark_tos", (*changed_conditional_json_value_in_map)["false_next"].asString());
}

TEST_F(RuntimeConditionalReconfigP4ObjectsTest, DeleteTest) {
    std::istringstream reconfig_commands_ss("delete cond ingress old_node_4");

    ASSERT_EQ(RuntimeReconfigErrorCode::SUCCESS, 
        static_cast<RuntimeReconfigErrorCode>(
            sw->mt_runtime_reconfig_with_stream(cxt_id, 
                                                new_json_file_stream,
                                                &reconfig_commands_ss)));

    P4Objects* p4objects_rt = sw->get_p4objects_rt();
    ASSERT_THROW(p4objects_rt->get_conditional("node_4"), std::out_of_range);
    ASSERT_EQ(nullptr, p4objects_rt->get_json_value("ingress conditional", "node_4"));
}