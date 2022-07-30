#include <gtest/gtest.h>

#include <bm/bm_sim/switch.h>
#include <bm/bm_sim/_assert.h>
#include <targets/simple_switch/simple_switch.h>

#include <fstream>
#include <sstream>
#include <vector>
#include <string>

#include <boost/filesystem.hpp>

using namespace bm;
typedef SimpleSwitch SwitchTest;

namespace fs = boost::filesystem;

class RuntimeTableReconfigP4ObjectsTest : public ::testing::Test {
    protected:
        SwitchTest sw{};
        size_t cxt_id{0};
        size_t ori_tables_num{0};
        size_t ori_actions_num{0};
        size_t added_tabl_ori_next_nodes_num{0};
        p4object_id_t drop_action_id{0};
        std::ifstream* new_json_file_stream;
        RuntimeTableReconfigP4ObjectsTest() 
            : new_json_file_stream(nullptr)
         { }

        virtual void SetUp() override {
            fs::path init_json_path = fs::path(testdata_dir) / fs::path(testdata_folder) / fs::path(init_json);
            _BM_ASSERT(sw.init_objects(init_json_path.string()) == 0);

            std::ifstream init_json_stream(init_json_path.string(), std::ios::in);
            _BM_ASSERT(init_json_stream);

            Json::Value cfg_root;
            init_json_stream >> cfg_root;
            Json::Value& cfg_pipelines = cfg_root["pipelines"];
            Json::Value& cfg_ingress_pipeline = cfg_pipelines[0];
            _BM_ASSERT(cfg_ingress_pipeline["name"].asString() == "ingress");

            Json::Value& cfg_tables_in_ingress_pipeline = cfg_ingress_pipeline["tables"];
            ori_tables_num = cfg_tables_in_ingress_pipeline.size();

            for (auto& cfg_tabl : cfg_tables_in_ingress_pipeline) {
                if (cfg_tabl["name"].asString() == added_tabl_name) {
                    _BM_ASSERT(cfg_tabl["next_tables"].isObject());
                    added_tabl_ori_next_nodes_num = cfg_tabl["next_tables"].getMemberNames().size();
                    break;
                }
            }

            Json::Value& cfg_actions = cfg_root["actions"];
            ori_actions_num = cfg_actions.size();
            for (const auto& cfg_action : cfg_actions) {
                // please note that this will only set drop_action_id to the id of MyIngress.drop's first occurance
                // this works for the example p4 program, 
                // but it might cause error when we change the example p4 program (e.g., use MyIngress.drop twice)
                // so, please check this after you modify the example p4 program
                if (cfg_action["name"].asString() == "MyIngress.drop") {
                    drop_action_id = cfg_action["id"].asInt();
                    break;
                }
            }

            fs::path new_json_file_path = fs::path(testdata_dir) / fs::path(testdata_folder) / fs::path(new_json);
            new_json_file_stream = new std::ifstream(new_json_file_path.string(), std::ios::in);
        }

        virtual void TearDown() override {
            delete new_json_file_stream;
         }

        static const char* testdata_dir;
        static const char* testdata_folder;

        static const char* init_json;
        static const char* new_json;

        static const std::string added_tabl_name;
};

const char* RuntimeTableReconfigP4ObjectsTest::testdata_dir = TESTDATADIR;
const char* RuntimeTableReconfigP4ObjectsTest::testdata_folder = "runtime_table_reconfig";

const char* RuntimeTableReconfigP4ObjectsTest::init_json = "runtime_table_reconfig_init.json";
const char* RuntimeTableReconfigP4ObjectsTest::new_json = "runtime_table_reconfig_new.json";

const std::string RuntimeTableReconfigP4ObjectsTest::added_tabl_name = "MyIngress.acl";

TEST_F(RuntimeTableReconfigP4ObjectsTest, InsertCheck) {
    std::istringstream reconfig_commands_ss("insert tabl ingress new_" + added_tabl_name);

    ASSERT_EQ(RuntimeReconfigErrorCode::SUCCESS, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                new_json_file_stream,
                                                &reconfig_commands_ss)));

    P4Objects* p4objects_rt = sw.get_p4objects_rt();

    std::string expected_added_tabl_name = added_tabl_name + "$" + std::to_string(ori_tables_num);

    ASSERT_NO_THROW(p4objects_rt->get_match_action_table(expected_added_tabl_name));
    MatchActionTable* table_ptr = p4objects_rt->get_match_action_table(expected_added_tabl_name);
    MatchTableAbstract* match_table = table_ptr->get_match_table();
    
    ASSERT_EQ(expected_added_tabl_name, match_table->get_name());
    ASSERT_EQ(ori_tables_num, match_table->get_id());

    ASSERT_NE(nullptr, p4objects_rt->get_json_value("ingress table", expected_added_tabl_name));
    const Json::Value* added_tabl_json_value_in_map = p4objects_rt->get_json_value("ingress table", expected_added_tabl_name);

    ASSERT_EQ(expected_added_tabl_name, (*added_tabl_json_value_in_map)["name"].asString());
    ASSERT_EQ(ori_tables_num, (*added_tabl_json_value_in_map)["id"].asInt());
    
    const Json::Value* added_tabl_json_value_in_cfg = nullptr;
    for (const auto& cfg_tabl : p4objects_rt->get_cfg()["pipelines"][0]["tables"]) {
        if (cfg_tabl["name"].asString() == expected_added_tabl_name) {
            added_tabl_json_value_in_cfg = &cfg_tabl;
            break;
        }
    }

    ASSERT_NE(nullptr, added_tabl_json_value_in_cfg);
    ASSERT_EQ(added_tabl_json_value_in_map, added_tabl_json_value_in_cfg);

    for (size_t i = ori_actions_num; i < ori_actions_num + added_tabl_ori_next_nodes_num; i++) {
        ASSERT_NO_THROW(p4objects_rt->get_action_by_id(i));
        ActionFn* action = p4objects_rt->get_action_by_id(i);
        std::string action_name = action->get_name();
        ASSERT_EQ(i, action->get_id());
        ASSERT_EQ(nullptr, match_table->get_next_nodes()[i]);
    }

    for (const auto& cfg_action_id : *added_tabl_json_value_in_cfg) {
        ASSERT_TRUE(cfg_action_id.asInt() >= ori_actions_num && 
            cfg_action_id.asInt() < ori_actions_num + added_tabl_ori_next_nodes_num);
    }

    if (match_table->get_has_next_node_hit()) {
        ASSERT_EQ(nullptr, match_table->get_next_node_hit());
    }

    if (match_table->get_has_next_node_miss()) {
        ASSERT_EQ(nullptr, match_table->get_next_node_miss());
    }
}

// since some methods are not public, this test can't be conducted now
// TEST_F(RuntimeTableReconfigP4ObjectsTest, ChangeBaseDefaultNextEdgeCheck) {
//     std::stringstream reconfig_commands_ss("change tabl ingress old_MyIngress.ipv4_lpm base_default_next old_MyIngress.mark_tos");

//     ASSERT_EQ(RuntimeReconfigErrorCode::SUCCESS, 
//         static_cast<RuntimeReconfigErrorCode>(
//             sw.mt_runtime_reconfig_with_stream(cxt_id, 
//                                                 new_json_file_stream, 
//                                                 &reconfig_commands_ss)));
// }

TEST_F(RuntimeTableReconfigP4ObjectsTest, ChangeHitEdgeCheck) {
    std::stringstream reconfig_commands_ss("change tabl ingress old_MyIngress.ipv4_lpm __HIT__ old_MyIngress.mark_tos");

    ASSERT_EQ(RuntimeReconfigErrorCode::SUCCESS, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                new_json_file_stream, 
                                                &reconfig_commands_ss)));

    P4Objects* p4objects_rt = sw.get_p4objects_rt();
    ASSERT_NO_THROW(p4objects_rt->get_match_action_table("MyIngress.ipv4_lpm"));
    MatchActionTable* table_ptr = p4objects_rt->get_match_action_table("MyIngress.ipv4_lpm");
    MatchTableAbstract* match_table = table_ptr->get_match_table();

    ASSERT_EQ(true, match_table->get_has_next_node_hit());
    ASSERT_EQ("MyIngress.mark_tos", match_table->get_next_node_hit()->get_name());
}

TEST_F(RuntimeTableReconfigP4ObjectsTest, ChangeMissEdgeCheck) {
    std::stringstream reconfig_commands_ss("change tabl ingress old_MyIngress.ipv4_lpm __MISS__ old_MyIngress.mark_tos");

    ASSERT_EQ(RuntimeReconfigErrorCode::SUCCESS, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                new_json_file_stream, 
                                                &reconfig_commands_ss)));
    
    P4Objects* p4objects_rt = sw.get_p4objects_rt();
    ASSERT_NO_THROW(p4objects_rt->get_match_action_table("MyIngress.ipv4_lpm"));
    MatchActionTable* table_ptr = p4objects_rt->get_match_action_table("MyIngress.ipv4_lpm");
    MatchTableAbstract* match_table = table_ptr->get_match_table();

    ASSERT_EQ(true, match_table->get_has_next_node_miss());
    ASSERT_EQ("MyIngress.mark_tos", match_table->get_next_node_miss()->get_name());
}

TEST_F(RuntimeTableReconfigP4ObjectsTest, ChangeActionEdgeCheck) {
    std::stringstream reconfig_commands_ss("change tabl ingress old_MyIngress.ipv4_lpm MyIngress.drop old_MyIngress.mark_tos");

    ASSERT_EQ(RuntimeReconfigErrorCode::SUCCESS, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                new_json_file_stream, 
                                                &reconfig_commands_ss)));

    P4Objects* p4objects_rt = sw.get_p4objects_rt();
    ASSERT_NO_THROW(p4objects_rt->get_match_action_table("MyIngress.ipv4_lpm"));
    MatchActionTable* table_ptr = p4objects_rt->get_match_action_table("MyIngress.ipv4_lpm");
    MatchTableAbstract* match_table = table_ptr->get_match_table();

    ASSERT_EQ("MyIngress.mark_tos", match_table->get_next_nodes()[drop_action_id]->get_name());
}

TEST_F(RuntimeTableReconfigP4ObjectsTest, DeleteCheck) {
    std::stringstream reconfig_commands_ss("delete tabl ingress old_MyIngress.ipv4_lpm");

    ASSERT_EQ(RuntimeReconfigErrorCode::SUCCESS, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                new_json_file_stream, 
                                                &reconfig_commands_ss)));

    P4Objects* p4objects_rt = sw.get_p4objects_rt();
    ASSERT_THROW(p4objects_rt->get_match_action_table("MyIngress.ipv4_lpm"), std::out_of_range);
    ASSERT_EQ(nullptr, p4objects_rt->get_json_value("ingress table", "MyIngress.ipv4_lpm"));
}
