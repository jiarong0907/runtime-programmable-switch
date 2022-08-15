#include <gtest/gtest.h>

#include <bm/bm_sim/P4Objects.h>

#include <ctype.h>

#include <algorithm>  // std::all_of
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <set>
#include <vector>
#include <unordered_map>

#include <boost/filesystem.hpp>

#include "jsoncpp/json.h"

using namespace bm;

namespace fs = boost::filesystem;

class RuntimeFlexReconfigP4ObjectsTest : public ::testing::Test {
    protected:
        P4Objects p4objects{};
        size_t ori_conditionals_num{0};
        size_t conditional_name_max{0};
        RuntimeFlexReconfigP4ObjectsTest() { }

        virtual void SetUp() override {
            fs::path init_json_path = fs::path(testdata_dir) / fs::path(testdata_folder) / fs::path(init_json);
            std::ifstream init_json_stream(init_json_path.string(), std::ios::in);
            _BM_ASSERT(init_json_stream);
            LookupStructureFactory factory;

            _BM_ASSERT(p4objects.init_objects(&init_json_stream, &factory) == 0);

            init_json_stream.clear();
            init_json_stream.seekg(0);

            Json::Value cfg_root;
            init_json_stream >> cfg_root;
            Json::Value& cfg_conditionals = cfg_root["pipelines"][0]["conditionals"];

            ori_conditionals_num = cfg_conditionals.size();

            for (const auto& cfg_conditional : cfg_conditionals) {
                size_t conditional_name_index = (size_t) std::stoi(cfg_conditional["name"].asString().substr(5));
                if (conditional_name_index > conditional_name_max) {
                    conditional_name_max = conditional_name_index;
                }
            }
        }

        virtual void TearDown() override { }

        static const char* testdata_dir;
        static const char* testdata_folder;

        static const char* init_json;
};

const char* RuntimeFlexReconfigP4ObjectsTest::testdata_dir = TESTDATADIR;
const char* RuntimeFlexReconfigP4ObjectsTest::testdata_folder = "runtime_flex_reconfig";

const char* RuntimeFlexReconfigP4ObjectsTest::init_json = "runtime_flex_reconfig_init.json";

TEST_F(RuntimeFlexReconfigP4ObjectsTest, InsertCheck) {
    const std::string added_flex_old_next_name = "MyIngress.mark_tos";
    const std::string added_flex_new_next_name = "MyIngress.ipv4_lpm";

    p4objects.insert_flex_rt("ingress", added_flex_old_next_name, added_flex_new_next_name);

    std::string expected_added_flex_name = "$TE_" + std::to_string(conditional_name_max + 1);

    ASSERT_NO_THROW(p4objects.get_conditional(expected_added_flex_name));

    Conditional* added_flex = p4objects.get_conditional(expected_added_flex_name);

    ASSERT_EQ(expected_added_flex_name, added_flex->get_name());
    ASSERT_EQ(ori_conditionals_num, added_flex->get_id());
    ASSERT_EQ(added_flex_old_next_name, added_flex->get_next_node_if_false()->get_name());
    ASSERT_EQ(added_flex_new_next_name, added_flex->get_next_node_if_true()->get_name());

    ASSERT_NE(nullptr, p4objects.get_json_value("ingress conditional", expected_added_flex_name));
    const Json::Value* added_flex_json_value_in_map = p4objects.get_json_value("ingress conditional", expected_added_flex_name);

    ASSERT_EQ(expected_added_flex_name, (*added_flex_json_value_in_map)["name"].asString());
    ASSERT_EQ(ori_conditionals_num, (*added_flex_json_value_in_map)["id"].asInt());
    ASSERT_EQ(added_flex_old_next_name, (*added_flex_json_value_in_map)["false_next"].asString());
    ASSERT_EQ(added_flex_new_next_name, (*added_flex_json_value_in_map)["true_next"].asString());

    const Json::Value* added_flex_json_value_in_cfg = nullptr;
    for (const auto& cfg_cond : p4objects.get_cfg()["pipelines"][0]["conditionals"]) {
        if (cfg_cond["name"].asString() == expected_added_flex_name) {
            added_flex_json_value_in_cfg = &cfg_cond;
            break;
        }
    }

    ASSERT_NE(nullptr, added_flex_json_value_in_cfg);
    ASSERT_EQ(added_flex_json_value_in_map, added_flex_json_value_in_cfg);
}

// test change
// we will not test change flex, because change flex use change_conditional_next_node_rt(), which has already been
// tested in test_runtime_conditional_reconfig_p4objects.cpp

// test delete

TEST_F(RuntimeFlexReconfigP4ObjectsTest, DeleteCheck) {
    p4objects.insert_flex_rt("ingress", "", "");

    std::string expected_added_flex_name = "$TE_" + std::to_string(conditional_name_max + 1);

    ASSERT_NO_THROW(p4objects.get_conditional(expected_added_flex_name));

    p4objects.delete_flex_rt("ingress", expected_added_flex_name);

    ASSERT_THROW(p4objects.get_conditional(expected_added_flex_name), std::out_of_range);
    ASSERT_EQ(nullptr, p4objects.get_json_value("ingress conditional", expected_added_flex_name));
}