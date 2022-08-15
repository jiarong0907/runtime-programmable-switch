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

namespace fs = boost::filesystem;

namespace {
    class SwitchTest : public Switch {
        public:
        int receive_(port_t port_num, const char *buffer, int len) override {
            (void) port_num; (void) buffer; (void) len;
            return 0;
        }

        void start_and_return_() override { }
    };
}

class RuntimeRegisterReconfigCommandTest : public ::testing::Test {
    protected:
        SwitchTest sw{};
        size_t cxt_id{0};
        RuntimeRegisterReconfigCommandTest() { }

        virtual void SetUp() override {
            fs::path init_json_path = fs::path(testdata_dir) / fs::path(testdata_folder) / fs::path(init_json);
            _BM_ASSERT(sw.init_objects(init_json_path.string()) == 0);
        }

        virtual void TearDown() override { }

        static const char* testdata_dir;
        static const char* testdata_folder;

        static const char* init_json;
};

const char* RuntimeRegisterReconfigCommandTest::testdata_dir = TESTDATADIR;
const char* RuntimeRegisterReconfigCommandTest::testdata_folder = "runtime_register_reconfig";

const char* RuntimeRegisterReconfigCommandTest::init_json = "runtime_register_reconfig_init.json";

TEST_F(RuntimeRegisterReconfigCommandTest, ValidInsertCommand) {
    std::istringstream new_json_file_ss("{}");
    std::istringstream reconfig_commands_ss("insert register_array new_register_array_0 128 32");

    ASSERT_EQ(RuntimeReconfigErrorCode::SUCCESS, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                &new_json_file_ss, 
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeRegisterReconfigCommandTest, ValidChangeSizeCommand) {
    std::istringstream new_json_file_ss("{}");
    std::istringstream reconfig_commands_ss("change register_array_size old_defence_bloom_filter_for_ip_src 2048");

    ASSERT_EQ(RuntimeReconfigErrorCode::SUCCESS, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                &new_json_file_ss, 
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeRegisterReconfigCommandTest, ValidChangeBitwidthCommand) {
    std::istringstream new_json_file_ss("{}");
    std::istringstream reconfig_commands_ss("change register_array_bitwidth old_defence_bloom_filter_for_ip_src 64");

    ASSERT_EQ(RuntimeReconfigErrorCode::SUCCESS, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                &new_json_file_ss, 
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeRegisterReconfigCommandTest, ValidDeleteCommand) {
    std::istringstream new_json_file_ss("{}");
    std::istringstream reconfig_commands_ss("delete register_array old_defence_bloom_filter_for_ip_src");

    ASSERT_EQ(RuntimeReconfigErrorCode::SUCCESS, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                &new_json_file_ss, 
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeRegisterReconfigCommandTest, NonexistentJsonFile) {
    fs::path nonexistent_json = fs::path(testdata_dir) / fs::path(testdata_folder) / fs::path("nonexistent.file");
    fs::path exist_plan = fs::path(testdata_dir) / fs::path(testdata_folder) / fs::path("exist_plan.txt");

    ASSERT_EQ(RuntimeReconfigErrorCode::OPEN_JSON_FILE_FAIL,
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig(cxt_id, 
                                    nonexistent_json.string(), 
                                    exist_plan.string())));
}

TEST_F(RuntimeRegisterReconfigCommandTest, NonexistentPlanFile) {
    fs::path nonexistent_json = fs::path(testdata_dir) / fs::path(testdata_folder) / fs::path("exist_json.json");
    fs::path exist_plan = fs::path(testdata_dir) / fs::path(testdata_folder) / fs::path("nonexistent.file");

    ASSERT_EQ(RuntimeReconfigErrorCode::OPEN_PLAN_FILE_FAIL,
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig(cxt_id, 
                                    nonexistent_json.string(), 
                                    exist_plan.string())));
}

TEST_F(RuntimeRegisterReconfigCommandTest, JsonFileWithError) {
    std::istringstream new_json_file_ss("{\"calculations\":[{\"name\":\"calc\",\"id\":0,\"input\":[],\"algo\":\"bad_hash_1\"}]}");
    std::istringstream reconfig_commands_ss("");

    ASSERT_EQ(RuntimeReconfigErrorCode::P4OBJECTS_INIT_FAIL, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                &new_json_file_ss, 
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeRegisterReconfigCommandTest, InvalidInsertPrefix) {
    std::istringstream new_json_file_ss("{}");
    std::istringstream reconfig_commands_ss("insert register_array xxx_register_array_00 1024 32");

    ASSERT_EQ(RuntimeReconfigErrorCode::PREFIX_ERROR, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                &new_json_file_ss, 
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeRegisterReconfigCommandTest, DuplicateInsert) {
    std::istringstream new_json_file_ss("{}");
    std::istringstream reconfig_commands_ss("insert register_array new_regsiter_array_00 1024 32\n"
                                            "insert register_array new_regsiter_array_00 1024 32");

    ASSERT_EQ(RuntimeReconfigErrorCode::DUP_CHECK_ERROR, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                &new_json_file_ss, 
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeRegisterReconfigCommandTest, UnfoundChangeSizeTarget) {
    std::istringstream new_json_file_ss("{}");
    std::istringstream reconfig_commands_ss("change register_array_size new_nonexistent_register_array 2048");

    ASSERT_EQ(RuntimeReconfigErrorCode::UNFOUND_ID_ERROR, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                &new_json_file_ss, 
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeRegisterReconfigCommandTest, UnfoundChangeBitwidthTarget) {
    std::istringstream new_json_file_ss("{}");
    std::istringstream reconfig_commands_ss("change register_array_bitwidth new_nonexistent_register_array 64");

    ASSERT_EQ(RuntimeReconfigErrorCode::UNFOUND_ID_ERROR, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                &new_json_file_ss, 
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeRegisterReconfigCommandTest, InvalidChangeSizePrefix) {
    std::istringstream new_json_file_ss("{}");
    std::istringstream reconfig_commands_ss("change register_array_size xxx_defence_bloom_filter_for_ip_src 2048");

    ASSERT_EQ(RuntimeReconfigErrorCode::PREFIX_ERROR, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                &new_json_file_ss, 
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeRegisterReconfigCommandTest, InvalidChaneBitWidthPrefix) {
    std::istringstream new_json_file_ss("{}");
    std::istringstream reconfig_commands_ss("change register_array_bitwidth xxx_defence_bloom_filter_for_ip_src 64");

    ASSERT_EQ(RuntimeReconfigErrorCode::PREFIX_ERROR, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                &new_json_file_ss, 
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeRegisterReconfigCommandTest, UnfoundDeleteTarget) {
    std::istringstream new_json_file_ss("{}");
    std::istringstream reconfig_commands_ss("delete register_array new_nonexistent_register_array");

    ASSERT_EQ(RuntimeReconfigErrorCode::UNFOUND_ID_ERROR, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                &new_json_file_ss, 
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeRegisterReconfigCommandTest, InvalidDeletePrefix) {
    std::istringstream new_json_file_ss("{}");
    std::istringstream reconfig_commands_ss("delete register_array xxx_defence_bloom_filter_for_ip_src");

    ASSERT_EQ(RuntimeReconfigErrorCode::PREFIX_ERROR, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                &new_json_file_ss, 
                                                &reconfig_commands_ss)));
}

