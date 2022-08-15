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

class RuntimeFlexReconfigTriggerTest : public ::testing::Test {
    protected:
        P4Objects p4objects{};
        RuntimeFlexReconfigTriggerTest() { }

        virtual void SetUp() override {
            fs::path init_json_path = fs::path(testdata_dir) / fs::path(testdata_folder) / fs::path(init_json);
            std::ifstream init_json_stream(init_json_path.string(), std::ios::in);
            _BM_ASSERT(init_json_stream);
            LookupStructureFactory factory;

            _BM_ASSERT(p4objects.init_objects(&init_json_stream, &factory) == 0);
        }

        virtual void TearDown() override { }

        static const char* testdata_dir;
        static const char* testdata_folder;

        static const char* init_json;
};

const char* RuntimeFlexReconfigTriggerTest::testdata_dir = TESTDATADIR;
const char* RuntimeFlexReconfigTriggerTest::testdata_folder = "runtime_flex_reconfig";

const char* RuntimeFlexReconfigTriggerTest::init_json = "runtime_flex_reconfig_init.json";

TEST_F(RuntimeFlexReconfigTriggerTest, FlexHeaderAndParserCheck) {
    ASSERT_NE(-1, p4objects.get_phv_factory().get_header_type(0).get_field_offset("flexMetadata.version"));
    int flex_header_offset = p4objects.get_phv_factory().get_header_type(0).get_field_offset("flexMetadata.version");
    ASSERT_EQ(8, p4objects.get_phv_factory().get_header_type(0).get_finfo(flex_header_offset).bitwidth);

    ASSERT_NE(nullptr, p4objects.get_parse_state_rt("parser", "flex_start"));
}