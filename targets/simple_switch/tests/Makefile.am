if COND_THRIFT
MAYBE_CLI_TESTS = CLI_tests
endif

SUBDIRS = . $(MAYBE_CLI_TESTS)

AM_CPPFLAGS += \
-isystem $(top_srcdir)/third_party/gtest/include \
-I$(srcdir)/.. \
-I$(srcdir)/ \
-DTESTDATADIR=\"$(srcdir)/testdata\"
LDADD = $(builddir)/../libsimpleswitch.la \
$(top_builddir)/third_party/gtest/libgtest.la \
$(top_builddir)/src/bm_apps/libbmapps.la \
-lboost_filesystem

# Define unit tests
common_source = main.cpp utils.cpp utils.h
TESTS = test_packet_redirect \
test_truncate \
test_swap \
test_queueing \
test_recirc \
test_parser_error \
test_runtime_register_reconfig_rehash \
test_runtime_table_reconfig_commands \
test_runtime_table_reconfig_p4objects \
test_runtime_conditional_reconfig_commands \
test_runtime_conditional_reconfig_p4objects \
test_runtime_flex_reconfig_commands \
test_runtime_flex_reconfig_p4objects \
test_runtime_flex_reconfig_trigger

check_PROGRAMS = $(TESTS) test_all

# Sources for tests
test_packet_redirect_SOURCES = $(common_source) test_packet_redirect.cpp
test_truncate_SOURCES = $(common_source) test_truncate.cpp
test_swap_SOURCES = $(common_source) test_swap.cpp
test_queueing_SOURCES = $(common_source) test_queueing.cpp
test_recirc_SOURCES = $(common_source) test_recirc.cpp
test_parser_error_SOURCES = $(common_source) test_parser_error.cpp
test_runtime_register_reconfig_rehash_SOURCES = $(common_source) test_runtime_register_reconfig_rehash.cpp
test_runtime_table_reconfig_commands_SOURCES = $(common_source) test_runtime_table_reconfig_commands.cpp
test_runtime_table_reconfig_p4objects_SOURCES = $(common_source) test_runtime_table_reconfig_p4objects.cpp
test_runtime_conditional_reconfig_commands_SOURCES = $(common_source) test_runtime_conditional_reconfig_commands.cpp
test_runtime_conditional_reconfig_p4objects_SOURCES = $(common_source) test_runtime_conditional_reconfig_p4objects.cpp
test_runtime_flex_reconfig_commands_SOURCES = $(common_source) test_runtime_flex_reconfig_commands.cpp
test_runtime_flex_reconfig_p4objects_SOURCES = $(common_source) test_runtime_flex_reconfig_p4objects.cpp
test_runtime_flex_reconfig_trigger_SOURCES = $(common_source) test_runtime_flex_reconfig_trigger.cpp

test_all_SOURCES = $(common_source) \
test_packet_redirect.cpp \
test_truncate.cpp \
test_swap.cpp \
test_queueing.cpp \
test_recirc.cpp \
test_parser_error.cpp \
test_runtime_register_reconfig_rehash.cpp \
test_runtime_table_reconfig_commands.cpp \
test_runtime_table_reconfig_p4objects.cpp \
test_runtime_conditional_reconfig_commands.cpp \
test_runtime_conditional_reconfig_p4objects.cpp \
test_runtime_flex_reconfig_commands.cpp \
test_runtime_flex_reconfig_p4objects.cpp \
test_runtime_flex_reconfig_trigger.cpp

EXTRA_DIST = \
testdata/packet_redirect.json \
testdata/truncate.json \
testdata/swap_1.json \
testdata/swap_2.json \
testdata/queueing.json \
testdata/recirc.json \
testdata/parser_error.p4 \
testdata/parser_error.json \
testdata/runtime_register_reconfig/new_SYN_flooding_protection.json \
testdata/runtime_register_reconfig/old_SYN_flooding_protection.json \
testdata/runtime_register_reconfig/reconfiguration_command.txt \
testdata/runtime_register_reconfig/old_SYN_flooding_protection.p4 \
testdata/runtime_register_reconfig/new_SYN_flooding_protection.p4 \
testdata/runtime_table_reconfig/runtime_table_reconfig_init.p4 \
testdata/runtime_table_reconfig/runtime_table_reconfig_new.p4 \
testdata/runtime_table_reconfig/runtime_table_reconfig_init.json \
testdata/runtime_table_reconfig/runtime_table_reconfig_new.json \
testdata/runtime_conditional_reconfig/runtime_conditional_reconfig_init.p4 \
testdata/runtime_conditional_reconfig/runtime_conditional_reconfig_new.p4 \
testdata/runtime_conditional_reconfig/runtime_conditional_reconfig_init.json \
testdata/runtime_conditional_reconfig/runtime_conditional_reconfig_new.json \
testdata/runtime_flex_reconfig/runtime_flex_reconfig_init.json \
testdata/runtime_flex_reconfig/runtime_flex_reconfig_init.p4
