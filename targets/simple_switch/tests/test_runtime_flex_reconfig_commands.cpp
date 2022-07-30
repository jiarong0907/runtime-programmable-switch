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

class RuntimeFlexReconfigCommandTest : public ::testing::Test {
    protected:
        SwitchTest sw{};
        size_t cxt_id{0};
        RuntimeFlexReconfigCommandTest() { }

        virtual void SetUp() override {
            fs::path init_json_path = fs::path(testdata_dir) / fs::path(testdata_folder) / fs::path(init_json);
            _BM_ASSERT(sw.init_objects(init_json_path.string()) == 0);
        }

        virtual void TearDown() override { }

        static const char* testdata_dir;
        static const char* testdata_folder;

        static const char* init_json;
};

const char* RuntimeFlexReconfigCommandTest::testdata_dir = TESTDATADIR;
const char* RuntimeFlexReconfigCommandTest::testdata_folder = "runtime_flex_reconfig";

const char* RuntimeFlexReconfigCommandTest::init_json = "runtime_flex_reconfig_init.json";

// test insert

TEST_F(RuntimeFlexReconfigCommandTest, ValidInsertCommandWithNullEdge) {
    std::istringstream new_json_file_ss("{}");
    std::istringstream reconfig_commands_ss("insert flex ingress flx_TE0 null null");

    ASSERT_EQ(RuntimeReconfigErrorCode::SUCCESS, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                &new_json_file_ss, 
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeFlexReconfigCommandTest, ValidInsertCommandWithTrueNextEdge) {
    std::istringstream new_json_file_ss("{}");
    std::istringstream reconfig_commands_ss("insert flex ingress flx_TE0 null old_MyIngress.mark_tos");

    ASSERT_EQ(RuntimeReconfigErrorCode::SUCCESS, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                &new_json_file_ss, 
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeFlexReconfigCommandTest, ValidInsertCommandWithFalseNextEdge) {
    std::istringstream new_json_file_ss("{}");
    std::istringstream reconfig_commands_ss("insert flex ingress flx_TE0 old_MyIngress.mark_tos null");

    ASSERT_EQ(RuntimeReconfigErrorCode::SUCCESS, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                &new_json_file_ss, 
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeFlexReconfigCommandTest, InvalidInsertPrefix) {
    std::istringstream new_json_file_ss("{}");
    std::stringstream reconfig_commands_ss("insert flex ingress xxx_TE0 null null");

    ASSERT_EQ(RuntimeReconfigErrorCode::PREFIX_ERROR, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                &new_json_file_ss, 
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeFlexReconfigCommandTest, DuplicateInsert) {
    std::istringstream new_json_file_ss("{}");
    std::stringstream reconfig_commands_ss("insert flex ingress flx_TE0 null null\n"
                                           "insert flex ingress flx_TE0 null null");

    ASSERT_EQ(RuntimeReconfigErrorCode::DUP_CHECK_ERROR, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                &new_json_file_ss, 
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeFlexReconfigCommandTest, InvalidInsertCommandWithUnfoundTrueNextEdge) {
    std::istringstream new_json_file_ss("{}");
    std::istringstream reconfig_commands_ss("insert flex ingress flx_TE0 null old_MyIngress.unfound");

    ASSERT_THROW(sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                &new_json_file_ss, 
                                                &reconfig_commands_ss), std::exception);
}

TEST_F(RuntimeFlexReconfigCommandTest, InvalidInsertCommandWithUnfoundFalseNextEdge) {
    std::istringstream new_json_file_ss("{}");
    std::istringstream reconfig_commands_ss("insert flex ingress flx_TE0 old_MyIngress.unfound null");

    ASSERT_THROW(sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                &new_json_file_ss, 
                                                &reconfig_commands_ss), std::exception);
}

// test change

TEST_F(RuntimeFlexReconfigCommandTest, ValidChangeTrueNextCommand) {
    std::istringstream new_json_file_ss("{}");
    std::istringstream reconfig_commands_ss("insert flex ingress flx_TE0 null null\n"
                                            "change flex ingress flx_TE0 true_next old_MyIngress.mark_tos");

     ASSERT_EQ(RuntimeReconfigErrorCode::SUCCESS, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                &new_json_file_ss, 
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeFlexReconfigCommandTest, ValidChangeFalseNextCommand) {
    std::istringstream new_json_file_ss("{}");
    std::istringstream reconfig_commands_ss("insert flex ingress flx_TE0 null null\n"
                                            "change flex ingress flx_TE0 false_next old_MyIngress.mark_tos");

     ASSERT_EQ(RuntimeReconfigErrorCode::SUCCESS, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                &new_json_file_ss, 
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeFlexReconfigCommandTest, InvalidChangePrefix0) {
    std::istringstream new_json_file_ss("{}");
    std::istringstream reconfig_commands_ss("insert flex ingress flx_TE0 null null\n"
                                            "change flex ingress flx_TE0 true_next xxx_MyIngress.mark_tos");

     ASSERT_EQ(RuntimeReconfigErrorCode::PREFIX_ERROR, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                &new_json_file_ss, 
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeFlexReconfigCommandTest, InvalidChangePrefix1) {
    std::istringstream new_json_file_ss("{}");
    std::istringstream reconfig_commands_ss("insert flex ingress flx_TE0 null null\n"
                                            "change flex ingress flx_TE0 true_next xxx_MyIngress.mark_tos");

     ASSERT_EQ(RuntimeReconfigErrorCode::PREFIX_ERROR, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                &new_json_file_ss, 
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeFlexReconfigCommandTest, ChangeUnfoundTrueNextEdge) {
    std::istringstream new_json_file_ss("{}");
    std::istringstream reconfig_commands_ss("insert flex ingress flx_TE0 null null\n"
                                            "change flex ingress flx_TE0 true_next new_MyIngress.unfound");

     ASSERT_EQ(RuntimeReconfigErrorCode::UNFOUND_ID_ERROR, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                &new_json_file_ss, 
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeFlexReconfigCommandTest, ChangeUnfoundFalseNextEdge) {
    std::istringstream new_json_file_ss("{}");
    std::istringstream reconfig_commands_ss("insert flex ingress flx_TE0 null null\n"
                                            "change flex ingress flx_TE0 false_next new_MyIngress.unfound");

     ASSERT_EQ(RuntimeReconfigErrorCode::UNFOUND_ID_ERROR, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                &new_json_file_ss, 
                                                &reconfig_commands_ss)));
}

// test delete

TEST_F(RuntimeFlexReconfigCommandTest, ValidDeleteTest) {
    std::istringstream new_json_file_ss("{}");
    std::istringstream reconfig_commands_ss("insert flex ingress flx_TE0 null null\n"
                                            "delete flex ingress flx_TE0");

    ASSERT_EQ(RuntimeReconfigErrorCode::SUCCESS, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                &new_json_file_ss,
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeFlexReconfigCommandTest, InvalidDeletePrefix) {
    std::istringstream new_json_file_ss("{}");
    std::istringstream reconfig_commands_ss("insert flex ingress flx_TE0 null null\n"
                                            "delete flex ingress xxx_TE0");

    ASSERT_EQ(RuntimeReconfigErrorCode::PREFIX_ERROR, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                &new_json_file_ss,
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeFlexReconfigCommandTest, UnfoundDeleteTarget) {
    std::istringstream new_json_file_ss("{}");
    std::istringstream reconfig_commands_ss("insert flex ingress flx_TE0 null null\n"
                                            "delete flex ingress flx_unfound");

    ASSERT_EQ(RuntimeReconfigErrorCode::UNFOUND_ID_ERROR, 
        static_cast<RuntimeReconfigErrorCode>(
            sw.mt_runtime_reconfig_with_stream(cxt_id, 
                                                &new_json_file_ss,
                                                &reconfig_commands_ss)));
}