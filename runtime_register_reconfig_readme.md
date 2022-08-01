## Runtime register reconfiguration
---
### Content:


- Add new functions to bmv2:
    - `insert register_array <name> <size> <width>`
    - `delete register_array <name>`
    - `change register_array_size <size>`
    - `change register_array_bitwidth <width>`
    - `rehash register_array <target_register_array> 
        --according-to <recording_register_array> <recording_last_pos_register_array> <recording_counting_register_array> 
        --hash-function-for-counting <hash_function> 
        --hash-function-for-target <first_pos_hash_function> <second_pos_hash_function> ... <nth_pos_hash_function> 
        --reset <time_stamp_register_array>`


- New demo:
    - Defence SYN flooding attack [1]

- New unit tests:
    - `test_runtime_register_reconfig_commands`
    - `test_runtime_register_reconfig_p4objects`
    - `test_runtime_register_reconfig_rehash`

---
### How to run the demo:
1. Recompile the bmv2 from the updated source code.
2. Go to `runtime_examples/SYN_flooding_protection/app/`.
3. Run `./run.sh`. It will build up a network comprising of three hosts (`h1, h2, h3`) and one switch (`s1`). 

    (During initialization, `s1` will load `old_SYN_flooding_protection.p4`)
```
network architecture:

    h1 ---
          \
           \
             s1 --- h3
           /  
          /
    h2 ---

h1: 10.0.1.10 (00:04:00:00:00:01)
h2: 10.0.1.21 (00:04:00:00:00:02)
h3: 10.0.1.22 (00:04:00:00:00:03)

h1-eth0 <-> s1-eth1
h2-eth0 <-> s1-eth2
h3-eth0 <-> s1-eth3

h1 and h2 are clients, and h3 is the server. h1 will attack h3 using SYN flooding. h2 represents the normal users who want to connect to h3.
```

4. When mininet is running, enter the command

    `h1 python3 ./attacker_send.py 10.0.1.22 attack >> h1_log.log &` 
    
    (This will send SYN packets with pseudo IP src addresses `192.168.7.0-192.168.7.255` to `h3`.)
5. After a few seconds, enter the command `h2 python3 ./error_rate_checker.py 10.0.1.22`.
    
    (This will establish TCP connections between `192.168.1.0-192.168.1.255` and `h3`.)

You will see some outputs like 
```
    [2] round:sending on interface h2-eth0 to 10.0.1.22
    timeout
    [3] round:sending on interface h2-eth0 to 10.0.1.22
    timeout
    [4] round:sending on interface h2-eth0 to 10.0.1.22
    timeout
    ...
```
This shows that, when the bloom filter's capacity is low (128 in this case), it might be possible for switch to prevent some legal users (such as `h2`) from getting in touch with the server (in this case `h3`, at `10.0.1.22`).

6. Then, try to enlarge the bloom filter. Open a new terminal and go to `build` folder.
7. In this new terminal, run `simple_switch_CLI --thrift-port 9090` and, then, `runtime_reconfig new_SYN_flooding_protection.json reconfiguration_command.txt`

    (You should compile `new_SYN_flooding_protection.p4` in advance.)
8. Please wait the reconfiguration to complete.

Now, you can see some outputs like
```
    [22] round:sending on interface h2-eth0 to 10.0.1.22
    success
    [23] round:sending on interface h2-eth0 to 10.0.1.22
    success
    [24] round:sending on interface h2-eth0 to 10.0.1.22
    success
    ...
``` 

This is because, with a larger bloom filter (1024 entries in this case), it's easier to distinguish ordinary users from malicious ones.

---
### How to run the tests:
1. Compile the codes with `make -j4`
2. Compile the tests with `make check -j4 TESTS=''`
3. To run a certain test:

    Enter `sudo make check TESTS='<test_name>'` under the folder where tests are located.
    
    Or, enter `./<test_name>`

---
### Reference:
```
[1] Wei Chen and Dit-Yan Yeung, "Defending Against TCP SYN Flooding Attacks Under Different Types of IP Spoofing," International Conference on Networking, International Conference on Systems and International Conference on Mobile Communications and Learning Technologies (ICNICONSMCL'06), 2006, pp. 38-38, doi: 10.1109/ICNICONSMCL.2006.72.
```
