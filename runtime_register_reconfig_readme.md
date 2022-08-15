## Runtime register reconfiguration
---
### Content:

- Add new functions to bmv2:
    - `insert register_array <name> <size> <width>`
    - `delete register_array <name>`
    - `change register_array_size <size>`
    - `change register_array_bitwidth <width>`

- New unit tests:
    - `test_runtime_register_reconfig_commands`
    - `test_runtime_register_reconfig_p4objects`
    - `test_runtime_table_reconfig_commands`
    - `test_runtime_table_reconfig_p4objects`
    - `test_runtime_conditional_reconfig_commands`
    - `test_runtime_conditional_reconfig_p4objects`
    - `test_runtime_flex_reconfig_commands`
    - `test_runtime_flex_reconfig_p4objects`
    - `test_runtime_flex_reconfig_trigger`

---
### How to run the tests:
1. Compile the codes with `make -j4`
2. Compile the tests with `make check -j4 TESTS=''`
3. To run a certain test:

    Enter `sudo make check TESTS='<test_name>'` under the folder where tests are located.
    
    Or, enter `./<test_name>`
