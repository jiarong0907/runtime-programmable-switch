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

using bm::RuntimeReconfigErrorCode;

namespace {
    void
    packet_handler(int port_num, const char *buffer, int len, void *cookie) {
        static_cast<SimpleSwitch *>(cookie)->receive(port_num, buffer, len);
    }
}

class RuntimeFlexReconfigCommandTest : public ::testing::Test {
    protected:
        size_t cxt_id{0};
        std::ifstream* new_json_file_stream;
        RuntimeFlexReconfigCommandTest() { }

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
            new_json_file_stream = new std::ifstream(init_json_path.string(), std::ios::in);
        }

        virtual void TearDown() override {
            delete new_json_file_stream;
        }

        static void TearDownTestCase() {

        }

        static const char* testdata_dir;
        static const char* testdata_folder;

        static const char* init_json;

        static SimpleSwitch* sw;
};

const char* RuntimeFlexReconfigCommandTest::testdata_dir = TESTDATADIR;
const char* RuntimeFlexReconfigCommandTest::testdata_folder = "runtime_flex_reconfig";

const char* RuntimeFlexReconfigCommandTest::init_json = "runtime_flex_reconfig_init.json";

SimpleSwitch* RuntimeFlexReconfigCommandTest::sw = nullptr;

// test insert

TEST_F(RuntimeFlexReconfigCommandTest, ValidInsertCommandWithNullEdge) {
    std::istringstream reconfig_commands_ss("insert flex ingress flx_TE0 null null");

    ASSERT_EQ(RuntimeReconfigErrorCode::SUCCESS, 
        static_cast<RuntimeReconfigErrorCode>(
            sw->mt_runtime_reconfig_with_stream(cxt_id, 
                                                new_json_file_stream, 
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeFlexReconfigCommandTest, ValidInsertCommandWithTrueNextEdge) {
    std::istringstream reconfig_commands_ss("insert flex ingress flx_TE1 null old_MyIngress.mark_tos");

    ASSERT_EQ(RuntimeReconfigErrorCode::SUCCESS, 
        static_cast<RuntimeReconfigErrorCode>(
            sw->mt_runtime_reconfig_with_stream(cxt_id, 
                                                new_json_file_stream, 
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeFlexReconfigCommandTest, ValidInsertCommandWithFalseNextEdge) {
    std::istringstream reconfig_commands_ss("insert flex ingress flx_TE2 old_MyIngress.mark_tos null");

    ASSERT_EQ(RuntimeReconfigErrorCode::SUCCESS, 
        static_cast<RuntimeReconfigErrorCode>(
            sw->mt_runtime_reconfig_with_stream(cxt_id, 
                                                new_json_file_stream, 
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeFlexReconfigCommandTest, InvalidInsertPrefix) {
    std::stringstream reconfig_commands_ss("insert flex ingress xxx_TE0 null null");

    ASSERT_EQ(RuntimeReconfigErrorCode::PREFIX_ERROR, 
        static_cast<RuntimeReconfigErrorCode>(
            sw->mt_runtime_reconfig_with_stream(cxt_id, 
                                                new_json_file_stream, 
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeFlexReconfigCommandTest, DuplicateInsert) {
    std::stringstream reconfig_commands_ss("insert flex ingress flx_TE4 null null\n"
                                           "insert flex ingress flx_TE4 null null");

    ASSERT_EQ(RuntimeReconfigErrorCode::DUP_CHECK_ERROR, 
        static_cast<RuntimeReconfigErrorCode>(
            sw->mt_runtime_reconfig_with_stream(cxt_id, 
                                                new_json_file_stream, 
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeFlexReconfigCommandTest, InvalidInsertCommandWithUnfoundTrueNextEdge) {
    std::istringstream reconfig_commands_ss("insert flex ingress flx_TE5 null old_MyIngress.unfound");

    ASSERT_THROW(sw->mt_runtime_reconfig_with_stream(cxt_id, 
                                                new_json_file_stream, 
                                                &reconfig_commands_ss), std::exception);
}

TEST_F(RuntimeFlexReconfigCommandTest, InvalidInsertCommandWithUnfoundFalseNextEdge) {
    std::istringstream reconfig_commands_ss("insert flex ingress flx_TE5 old_MyIngress.unfound null");

    ASSERT_THROW(sw->mt_runtime_reconfig_with_stream(cxt_id, 
                                                new_json_file_stream, 
                                                &reconfig_commands_ss), std::exception);
}

// test change

TEST_F(RuntimeFlexReconfigCommandTest, ValidChangeTrueNextCommand) {
    std::istringstream reconfig_commands_ss("change flex ingress flx_TE0 true_next old_MyIngress.mark_tos");

     ASSERT_EQ(RuntimeReconfigErrorCode::SUCCESS, 
        static_cast<RuntimeReconfigErrorCode>(
            sw->mt_runtime_reconfig_with_stream(cxt_id, 
                                                new_json_file_stream, 
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeFlexReconfigCommandTest, ValidChangeFalseNextCommand) {
    std::istringstream reconfig_commands_ss("change flex ingress flx_TE0 false_next old_MyIngress.mark_tos");

     ASSERT_EQ(RuntimeReconfigErrorCode::SUCCESS, 
        static_cast<RuntimeReconfigErrorCode>(
            sw->mt_runtime_reconfig_with_stream(cxt_id, 
                                                new_json_file_stream, 
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeFlexReconfigCommandTest, InvalidChangePrefix0) {
    std::istringstream reconfig_commands_ss("change flex ingress flx_TE0 true_next xxx_MyIngress.mark_tos");

     ASSERT_EQ(RuntimeReconfigErrorCode::PREFIX_ERROR, 
        static_cast<RuntimeReconfigErrorCode>(
            sw->mt_runtime_reconfig_with_stream(cxt_id, 
                                                new_json_file_stream, 
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeFlexReconfigCommandTest, InvalidChangePrefix1) {
    std::istringstream reconfig_commands_ss("change flex ingress flx_TE0 true_next xxx_MyIngress.mark_tos");

     ASSERT_EQ(RuntimeReconfigErrorCode::PREFIX_ERROR, 
        static_cast<RuntimeReconfigErrorCode>(
            sw->mt_runtime_reconfig_with_stream(cxt_id, 
                                                new_json_file_stream, 
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeFlexReconfigCommandTest, ChangeUnfoundTrueNextEdge) {
    std::istringstream reconfig_commands_ss("change flex ingress flx_TE0 true_next new_MyIngress.unfound");

     ASSERT_EQ(RuntimeReconfigErrorCode::UNFOUND_ID_ERROR, 
        static_cast<RuntimeReconfigErrorCode>(
            sw->mt_runtime_reconfig_with_stream(cxt_id, 
                                                new_json_file_stream, 
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeFlexReconfigCommandTest, ChangeUnfoundFalseNextEdge) {
    std::istringstream reconfig_commands_ss("change flex ingress flx_TE0 false_next new_MyIngress.unfound");

     ASSERT_EQ(RuntimeReconfigErrorCode::UNFOUND_ID_ERROR, 
        static_cast<RuntimeReconfigErrorCode>(
            sw->mt_runtime_reconfig_with_stream(cxt_id, 
                                                new_json_file_stream, 
                                                &reconfig_commands_ss)));
}

// test delete

TEST_F(RuntimeFlexReconfigCommandTest, ValidDeleteTest) {
    std::istringstream reconfig_commands_ss("delete flex ingress flx_TE0");

    ASSERT_EQ(RuntimeReconfigErrorCode::SUCCESS, 
        static_cast<RuntimeReconfigErrorCode>(
            sw->mt_runtime_reconfig_with_stream(cxt_id, 
                                                new_json_file_stream,
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeFlexReconfigCommandTest, InvalidDeletePrefix) {
    std::istringstream reconfig_commands_ss("delete flex ingress xxx_TE0");

    ASSERT_EQ(RuntimeReconfigErrorCode::PREFIX_ERROR, 
        static_cast<RuntimeReconfigErrorCode>(
            sw->mt_runtime_reconfig_with_stream(cxt_id, 
                                                new_json_file_stream,
                                                &reconfig_commands_ss)));
}

TEST_F(RuntimeFlexReconfigCommandTest, UnfoundDeleteTarget) {
    std::istringstream reconfig_commands_ss("delete flex ingress flx_unfound");

    ASSERT_EQ(RuntimeReconfigErrorCode::UNFOUND_ID_ERROR, 
        static_cast<RuntimeReconfigErrorCode>(
            sw->mt_runtime_reconfig_with_stream(cxt_id, 
                                                new_json_file_stream,
                                                &reconfig_commands_ss)));
}