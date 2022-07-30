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

class RuntimeConditionalReconfigCommandTest : public ::testing::Test {
    protected:
        SwitchTest sw{};
        size_t cxt_id{0};
        std::ifstream* new_json_file_stream;
        RuntimeConditionalReconfigCommandTest() 
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

const char* RuntimeConditionalReconfigCommandTest::testdata_dir = TESTDATADIR;
const char* RuntimeConditionalReconfigCommandTest::testdata_folder = "runtime_conditional_reconfig";

const char* RuntimeConditionalReconfigCommandTest::init_json = "runtime_conditional_reconfig_init.json";
const char* RuntimeConditionalReconfigCommandTest::new_json = "runtime_conditional_reconfig_new.json";

// test insert

TEST_F(RuntimeConditionalReconfigCommandTest, ValidInsertCommand) {
    std::istringstream reconfig_commands_ss("insert cond ingress new_node_6");

    ASSERT_EQ(RuntimeReconfigErrorCode::SUCCESS, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                new_json_file_stream,
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeConditionalReconfigCommandTest, InvalidInsertPrefix) {
    std::stringstream reconfig_commands_ss("insert cond ingress xxx_node_6");

    ASSERT_EQ(RuntimeReconfigErrorCode::PREFIX_ERROR, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                new_json_file_stream, 
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeConditionalReconfigCommandTest, DuplicateInsert) {
    std::stringstream reconfig_commands_ss("insert cond ingress new_node_6\n"
                                           "insert cond ingress new_node_6");

    ASSERT_EQ(RuntimeReconfigErrorCode::DUP_CHECK_ERROR, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                new_json_file_stream, 
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeConditionalReconfigCommandTest, UnfoundInsertConditional) {
    std::stringstream reconfig_commands_ss("insert cond ingress new_node_unfound");

    ASSERT_THROW(sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                    new_json_file_stream, 
                                                    &reconfig_commands_ss), std::out_of_range);
}

// test change

TEST_F(RuntimeConditionalReconfigCommandTest, ValidChangeTest0) {
    std::istringstream reconfig_commands_ss("change cond ingress old_node_4 true_next old_MyIngress.mark_tos");

    ASSERT_EQ(RuntimeReconfigErrorCode::SUCCESS, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                new_json_file_stream,
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeConditionalReconfigCommandTest, ValidChangeTest1) {
    std::istringstream reconfig_commands_ss("change cond ingress old_node_4 false_next old_MyIngress.mark_tos");

    ASSERT_EQ(RuntimeReconfigErrorCode::SUCCESS, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                new_json_file_stream,
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeConditionalReconfigCommandTest, InvalidChangePrefix0) {
    std::stringstream reconfig_commands_ss("change cond ingress old_node_4 true_next xxx_MyIngress.mark_tos");

    ASSERT_EQ(RuntimeReconfigErrorCode::PREFIX_ERROR, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                new_json_file_stream, 
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeConditionalReconfigCommandTest, InvalidChangePrefix1) {
    std::stringstream reconfig_commands_ss("change cond ingress old_node_4 false_next xxx_MyIngress.mark_tos");

    ASSERT_EQ(RuntimeReconfigErrorCode::PREFIX_ERROR, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                new_json_file_stream, 
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeConditionalReconfigCommandTest, UnfoundChangeTarget0) {
    std::stringstream reconfig_commands_ss("change cond ingress old_node_4 true_next new_MyIngress.unfound");

    ASSERT_EQ(RuntimeReconfigErrorCode::UNFOUND_ID_ERROR, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                new_json_file_stream, 
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeConditionalReconfigCommandTest, UnfoundChangeTarget1) {
    std::stringstream reconfig_commands_ss("change cond ingress old_node_4 false_next new_MyIngress.unfound");

    ASSERT_EQ(RuntimeReconfigErrorCode::UNFOUND_ID_ERROR, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                new_json_file_stream, 
                                                &reconfig_commands_ss)));
}

// test delete

TEST_F(RuntimeConditionalReconfigCommandTest, ValidDeleteTest) {
    std::istringstream reconfig_commands_ss("delete cond ingress old_node_4");

    ASSERT_EQ(RuntimeReconfigErrorCode::SUCCESS, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                new_json_file_stream,
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeConditionalReconfigCommandTest, InvalidDeletePrefix) {
    std::stringstream reconfig_commands_ss("delete cond ingress xxx_node_4");

    ASSERT_EQ(RuntimeReconfigErrorCode::PREFIX_ERROR, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                new_json_file_stream, 
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeConditionalReconfigCommandTest, UnfoundDeleteTarget) {
    std::stringstream reconfig_commands_ss("delete cond ingress new_node_unfound");

    ASSERT_EQ(RuntimeReconfigErrorCode::UNFOUND_ID_ERROR, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                new_json_file_stream, 
                                                &reconfig_commands_ss)));
}