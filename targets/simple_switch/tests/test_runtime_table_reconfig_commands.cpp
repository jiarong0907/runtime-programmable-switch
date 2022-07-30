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

class RuntimeTableReconfigCommandTest : public ::testing::Test {
    protected:
        SwitchTest sw{};
        size_t cxt_id{0};
        std::ifstream* new_json_file_stream;
        RuntimeTableReconfigCommandTest() 
            : new_json_file_stream(nullptr)
         { }

        virtual void SetUp() override {
            fs::path init_json_path = fs::path(testdata_dir) / fs::path(testdata_folder) / fs::path(init_json);
            _BM_ASSERT(sw.init_objects(init_json_path.string()) == 0);

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
};

const char* RuntimeTableReconfigCommandTest::testdata_dir = TESTDATADIR;
const char* RuntimeTableReconfigCommandTest::testdata_folder = "runtime_table_reconfig";

const char* RuntimeTableReconfigCommandTest::init_json = "runtime_table_reconfig_init.json";
const char* RuntimeTableReconfigCommandTest::new_json = "runtime_table_reconfig_new.json";

// test insert

TEST_F(RuntimeTableReconfigCommandTest, ValidInsertCommand) {
    std::istringstream reconfig_commands_ss("insert tabl ingress new_MyIngress.acl");

    ASSERT_EQ(RuntimeReconfigErrorCode::SUCCESS, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                new_json_file_stream,
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeTableReconfigCommandTest, InvalidInsertPrefix) {
    std::stringstream reconfig_commands_ss("insert tabl ingress xxx_MyIngress.acl");

    ASSERT_EQ(RuntimeReconfigErrorCode::PREFIX_ERROR, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                new_json_file_stream, 
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeTableReconfigCommandTest, DuplicateInsert) {
    std::stringstream reconfig_commands_ss("insert tabl ingress new_MyIngress.acl\n"
                                           "insert tabl ingress new_MyIngress.acl");

    ASSERT_EQ(RuntimeReconfigErrorCode::DUP_CHECK_ERROR, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                new_json_file_stream, 
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeTableReconfigCommandTest, UnfoundInsertTable) {
    std::stringstream reconfig_commands_ss("insert tabl ingress new_MyIngress.unfound");

    ASSERT_THROW(sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                    new_json_file_stream, 
                                                    &reconfig_commands_ss), std::out_of_range);
}

// test change

TEST_F(RuntimeTableReconfigCommandTest, ValidChangeCommand) {
    std::stringstream reconfig_commands_ss("change tabl ingress old_MyIngress.ipv4_lpm base_default_next null");

    ASSERT_EQ(RuntimeReconfigErrorCode::SUCCESS,
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                new_json_file_stream, 
                                                &reconfig_commands_ss)));
}


TEST_F(RuntimeTableReconfigCommandTest, ValidChangeBaseDefaultNextEdge) {
    std::stringstream reconfig_commands_ss("change tabl ingress old_MyIngress.ipv4_lpm base_default_next old_MyIngress.mark_tos");

    ASSERT_EQ(RuntimeReconfigErrorCode::SUCCESS, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                new_json_file_stream, 
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeTableReconfigCommandTest, ValidChangeHitEdge) {
    std::stringstream reconfig_commands_ss("change tabl ingress old_MyIngress.ipv4_lpm __HIT__ old_MyIngress.mark_tos");

    ASSERT_EQ(RuntimeReconfigErrorCode::SUCCESS, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                new_json_file_stream, 
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeTableReconfigCommandTest, ValidChangeMissEdge) {
    std::stringstream reconfig_commands_ss("change tabl ingress old_MyIngress.ipv4_lpm __MISS__ old_MyIngress.mark_tos");

    ASSERT_EQ(RuntimeReconfigErrorCode::SUCCESS, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                new_json_file_stream, 
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeTableReconfigCommandTest, ValidChangeActionEdge) {
    std::stringstream reconfig_commands_ss("change tabl ingress old_MyIngress.ipv4_lpm MyIngress.drop old_MyIngress.mark_tos");

    ASSERT_EQ(RuntimeReconfigErrorCode::SUCCESS, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                new_json_file_stream, 
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeTableReconfigCommandTest, InvalidChangePrefix0) {
    std::stringstream reconfig_commands_ss("change tabl ingress xxx_MyIngress.ipv4_lpm MyIngress.drop old_MyIngress.mark_tos");

    ASSERT_EQ(RuntimeReconfigErrorCode::PREFIX_ERROR, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                new_json_file_stream, 
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeTableReconfigCommandTest, InvalidChangePrefix1) {
    std::stringstream reconfig_commands_ss("change tabl ingress old_MyIngress.ipv4_lpm MyIngress.drop xxx_MyIngress.mark_tos");

    ASSERT_EQ(RuntimeReconfigErrorCode::PREFIX_ERROR, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                new_json_file_stream, 
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeTableReconfigCommandTest, UnfoundChangeTarget0) {
    std::stringstream reconfig_commands_ss("change tabl ingress new_MyIngress.unfound MyIngress.drop old_MyIngress.mark_tos");

    ASSERT_EQ(RuntimeReconfigErrorCode::UNFOUND_ID_ERROR, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                new_json_file_stream, 
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeTableReconfigCommandTest, UnfoundChangeTarget1) {
    std::stringstream reconfig_commands_ss("change tabl ingress old_MyIngress.ipv4_forward MyIngress.drop new_MyIngress.unfound");

    ASSERT_EQ(RuntimeReconfigErrorCode::UNFOUND_ID_ERROR, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                new_json_file_stream, 
                                                &reconfig_commands_ss)));
}

// test delete

TEST_F(RuntimeTableReconfigCommandTest, ValidDeleteCommand) {
    std::stringstream reconfig_commands_ss("delete tabl ingress old_MyIngress.ipv4_lpm");

    ASSERT_EQ(RuntimeReconfigErrorCode::SUCCESS, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                new_json_file_stream, 
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeTableReconfigCommandTest, InvalidDeletePrefix) {
    std::stringstream reconfig_commands_ss("delete tabl ingress xxx_MyIngress.ipv4_lpm");

    ASSERT_EQ(RuntimeReconfigErrorCode::PREFIX_ERROR, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                new_json_file_stream, 
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeTableReconfigCommandTest, UnfoundDeleteTarget) {
    std::stringstream reconfig_commands_ss("delete tabl ingress new_MyIngress.unfound");

    ASSERT_EQ(RuntimeReconfigErrorCode::UNFOUND_ID_ERROR, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                new_json_file_stream, 
                                                &reconfig_commands_ss)));
}