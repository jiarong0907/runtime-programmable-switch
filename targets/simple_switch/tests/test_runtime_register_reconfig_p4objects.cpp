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

class RuntimeRegisterReconfigP4ObjectsTest : public ::testing::Test {
    protected:
        P4Objects p4objects{};
        uint32_t ori_register_arrays_num{0};
        std::vector<std::string> register_array_init_names{};
        RuntimeRegisterReconfigP4ObjectsTest() { }

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
            Json::Value& cfg_register_arrays = cfg_root["register_arrays"];
            ori_register_arrays_num = cfg_register_arrays.size();
            for (const auto& cfg_register_array : cfg_register_arrays) {
                register_array_init_names.push_back(cfg_register_array["name"].asString());
            }
        }

        virtual void TearDown() override { }

        static const char* testdata_dir;
        static const char* testdata_folder;

        static const char* init_json;
};

const char* RuntimeRegisterReconfigP4ObjectsTest::testdata_dir = TESTDATADIR;
const char* RuntimeRegisterReconfigP4ObjectsTest::testdata_folder = "runtime_register_reconfig";

const char* RuntimeRegisterReconfigP4ObjectsTest::init_json = "runtime_register_reconfig_init.json";

TEST_F(RuntimeRegisterReconfigP4ObjectsTest, InsertCheck) {
    const std::string added_register_array_name = "register_array_newly_added_0";
    const std::string added_register_array_size = "128";
    const std::string added_register_array_bitwidth = "32";

    p4objects.insert_register_array_rt(added_register_array_name, added_register_array_size, added_register_array_bitwidth);

    std::string expected_added_register_array_name = added_register_array_name + "$" + std::to_string(ori_register_arrays_num);
    ASSERT_NE(nullptr, p4objects.get_register_array_rt(expected_added_register_array_name));

    RegisterArray* added_register_array = p4objects.get_register_array_rt(expected_added_register_array_name);
    ASSERT_EQ(expected_added_register_array_name, added_register_array->get_name());
    ASSERT_EQ((size_t) std::stoi(added_register_array_size), added_register_array->size());
    ASSERT_EQ(std::stoi(added_register_array_bitwidth), added_register_array->get_bitwidth());

    ASSERT_NE(nullptr, p4objects.get_json_value("register_array", expected_added_register_array_name));
    const Json::Value* added_register_array_json_value_in_map = 
            p4objects.get_json_value("register_array", expected_added_register_array_name);

    ASSERT_EQ(expected_added_register_array_name, (*added_register_array_json_value_in_map)["name"].asString());
    ASSERT_EQ(ori_register_arrays_num, (*added_register_array_json_value_in_map)["id"].asInt());
    ASSERT_EQ((size_t) std::stoi(added_register_array_size), (size_t) (*added_register_array_json_value_in_map)["size"].asUInt());
    ASSERT_EQ(std::stoi(added_register_array_bitwidth), (*added_register_array_json_value_in_map)["bitwidth"].asInt());

    const Json::Value* added_register_array_json_value_in_cfg = nullptr;
    for (const auto& cfg_register_array : p4objects.get_cfg()["register_arrays"]) {
        if (cfg_register_array["name"].asString() == expected_added_register_array_name) {
            added_register_array_json_value_in_cfg = &cfg_register_array;
            break;
        }
    }

    ASSERT_NE(nullptr, added_register_array_json_value_in_cfg);
    ASSERT_EQ(added_register_array_json_value_in_map, added_register_array_json_value_in_cfg);
}

TEST_F(RuntimeRegisterReconfigP4ObjectsTest, ChangeSizeCheck) {
    const std::string changed_register_array_name = "defence_bloom_filter_for_ip_src";
    const std::string changed_register_array_size = "2048";

    p4objects.change_register_array_size_rt(changed_register_array_name, changed_register_array_size);

    ASSERT_NE(nullptr, p4objects.get_register_array_rt(changed_register_array_name));

    RegisterArray* changed_register_array = p4objects.get_register_array_rt(changed_register_array_name);
    ASSERT_EQ(changed_register_array_name, changed_register_array->get_name());
    ASSERT_EQ((size_t) std::stoi(changed_register_array_size), changed_register_array->size());

    ASSERT_NE(nullptr, p4objects.get_json_value("register_array", changed_register_array_name));
    const Json::Value* changed_register_array_json_value_in_map = 
            p4objects.get_json_value("register_array", changed_register_array_name);

    ASSERT_EQ(changed_register_array_name, (*changed_register_array_json_value_in_map)["name"].asString());
    ASSERT_EQ((size_t) std::stoi(changed_register_array_size), (size_t) (*changed_register_array_json_value_in_map)["size"].asUInt());

    const Json::Value* changed_register_array_json_value_in_cfg = nullptr;
    for (const auto& cfg_register_array : p4objects.get_cfg()["register_arrays"]) {
        if (cfg_register_array["name"].asString() == changed_register_array_name) {
            changed_register_array_json_value_in_cfg = &cfg_register_array;
            break;
        }
    }

    ASSERT_NE(nullptr, changed_register_array_json_value_in_cfg);
    ASSERT_EQ(changed_register_array_json_value_in_map, changed_register_array_json_value_in_cfg);
}

TEST_F(RuntimeRegisterReconfigP4ObjectsTest, ChangeBitwidthCheck) {
    const std::string changed_register_array_name = "defence_bloom_filter_for_ip_src";
    const std::string changed_register_array_bitwidth = "64";

    p4objects.change_register_array_bitwidth_rt(changed_register_array_name, changed_register_array_bitwidth);

    ASSERT_NE(nullptr, p4objects.get_register_array_rt(changed_register_array_name));

    RegisterArray* changed_register_array = p4objects.get_register_array_rt(changed_register_array_name);
    ASSERT_EQ(changed_register_array_name, changed_register_array->get_name());
    ASSERT_EQ(std::stoi(changed_register_array_bitwidth), changed_register_array->get_bitwidth());

    ASSERT_NE(nullptr, p4objects.get_json_value("register_array", changed_register_array_name));
    const Json::Value* changed_register_array_json_value_in_map = 
            p4objects.get_json_value("register_array", changed_register_array_name);

    ASSERT_EQ(changed_register_array_name, (*changed_register_array_json_value_in_map)["name"].asString());
    ASSERT_EQ(std::stoi(changed_register_array_bitwidth), (*changed_register_array_json_value_in_map)["bitwidth"].asInt());

    const Json::Value* changed_register_array_json_value_in_cfg = nullptr;
    for (const auto& cfg_register_array : p4objects.get_cfg()["register_arrays"]) {
        if (cfg_register_array["name"].asString() == changed_register_array_name) {
            changed_register_array_json_value_in_cfg = &cfg_register_array;
            break;
        }
    }

    ASSERT_NE(nullptr, changed_register_array_json_value_in_cfg);
    ASSERT_EQ(changed_register_array_json_value_in_map, changed_register_array_json_value_in_cfg);
}

TEST_F(RuntimeRegisterReconfigP4ObjectsTest, DeleteCheck) {
    struct RegisterArrayInfo {
        std::string name;
        int id;
        size_t size;
        int bitwidth;

        RegisterArrayInfo(const std::string& name, int id, size_t size, int bitwidth) 
            : name(name), id(id), size(size), bitwidth(bitwidth)
        { }
    };

    const std::string deleted_register_array_name = "defence_bloom_filter_for_ip_src";

    std::unordered_map<std::string, RegisterArrayInfo> ori_register_array_infos;
    for (const std::string& register_array_name : register_array_init_names) {
        RegisterArray* ori_register_array = p4objects.get_register_array_rt(register_array_name);
        ASSERT_NE(nullptr, ori_register_array);
        ori_register_array_infos.insert({register_array_name, 
                                                {ori_register_array->get_name(), ori_register_array->get_id(), 
                                                ori_register_array->size(), ori_register_array->get_bitwidth()}
                                        });
    }

    std::unordered_map<std::string, Json::Value> ori_register_array_json_values_in_map;
    for (const std::string& register_array_name : register_array_init_names) {
        const Json::Value* ori_register_array_json_value = p4objects.get_json_value("register_array", register_array_name);
        ASSERT_NE(nullptr, ori_register_array_json_value);
        ori_register_array_json_values_in_map.insert({register_array_name, *ori_register_array_json_value});
    }

    p4objects.delete_register_array_rt(deleted_register_array_name);

    for (const std::string& register_array_name : register_array_init_names) {
        if (register_array_name == deleted_register_array_name) {
            ASSERT_TRUE(!p4objects.get_register_array_rt(register_array_name));
            continue;
        }

        ASSERT_TRUE(p4objects.get_register_array_rt(register_array_name));
        const RegisterArray* cur_register_array = p4objects.get_register_array_rt(register_array_name);
        ASSERT_TRUE(ori_register_array_infos.find(register_array_name) != ori_register_array_infos.end());
        ASSERT_EQ(cur_register_array->get_name(), ori_register_array_infos.find(register_array_name)->second.name);
        ASSERT_EQ(cur_register_array->get_id(), ori_register_array_infos.find(register_array_name)->second.id);
        ASSERT_EQ(cur_register_array->size(), ori_register_array_infos.find(register_array_name)->second.size);
        ASSERT_EQ(cur_register_array->get_bitwidth(), ori_register_array_infos.find(register_array_name)->second.bitwidth);
    }

    for (const std::string& register_array_name : register_array_init_names) {
        if (register_array_name == deleted_register_array_name) {
            // please note that this step will cause the program to log an error: "get_json_value: cannot find register_array name: {}""
            ASSERT_TRUE(!p4objects.get_json_value("register_array", register_array_name));
            continue;
        }

        ASSERT_TRUE(p4objects.get_json_value("register_array", register_array_name));
        const Json::Value* cur_register_array_json_value = p4objects.get_json_value("register_array", register_array_name);
        ASSERT_TRUE(ori_register_array_json_values_in_map.find(register_array_name) != ori_register_array_json_values_in_map.end());
        ASSERT_EQ((*cur_register_array_json_value)["name"].asString(), 
            ori_register_array_json_values_in_map.find(register_array_name)->second["name"].asString());
        ASSERT_EQ((*cur_register_array_json_value)["id"].asInt(), 
            ori_register_array_json_values_in_map.find(register_array_name)->second["id"].asInt());
        ASSERT_EQ((*cur_register_array_json_value)["size"].asUInt(), 
            ori_register_array_json_values_in_map.find(register_array_name)->second["size"].asUInt());
        ASSERT_EQ((*cur_register_array_json_value)["bitwidth"].asInt(), 
            ori_register_array_json_values_in_map.find(register_array_name)->second["bitwidth"].asInt());
    }

    const Json::Value& cfg_register_arrays = p4objects.get_cfg()["register_arrays"];
    for (const Json::Value& cfg_register_array : cfg_register_arrays) {
        ASSERT_TRUE(p4objects.get_json_value("register_array", cfg_register_array["name"].asString()));
        const Json::Value* cur_register_array_json_value = p4objects.get_json_value("register_array", cfg_register_array["name"].asString());
        ASSERT_EQ(&cfg_register_array, cur_register_array_json_value);
    }
} 