# Runtime programmable switch

This is a runtime programmable software switch implemented based on the BMv2 model. It is the software backend of the FlexCore ecosystem. It automically reconfigures the switch using the commands generated by the FlexCore ecosystem. More details can be found in the FlexCore [paper](https://jxing.me/pdf/flexcore-nsdi22.pdf) and [repo](https://github.com/jiarong0907/FlexCore).


## Supported primitives

### Reconfig tables
- `insert tabl <pipeline_name> <table_name>`: Insert a table into a pipeline. `insert tabl ingress new_acl`: Insert a new table named `acl` to a pipeline named `ingress` (without setting the next table pointer.).
- `change tabl <pipeline_name> <table_name> <edge_name> <table_name_next>`: Change the next table of an edge of a table in a specific pipeline. `change tabl ingress new_acl base_default_next old_routing`: Change the next table of base_default_next of new_acl in ingress to the old_routing.
- `delete tabl <pipeline_name> <table_name>`: Delete a table from a pipeline. `delete tabl ingress old_routing`: Delete old_routing from ingress.

### control flow branch
- `insert cond <pipeline_name> <branch_name>`: Insert a branch into a pipeline. `insert cond new_node_4`: Insert a new conditional node named `node_4` to a pipeline named `ingress` (without setting the next table pointers).
- `change cond <pipeline_name> <branch_name> <true_next|false_next> <node_name>`: Set the next table of the true/false branch of a conditional node in a pipeline. `change cond ingress old_node_4 false_next old_routing`: Change the next table of the false branch of old_node_4 to old_routing in ingress.
- `delete cond <pipeline_name> <branch_name>`: Delete a branch from a pipeline. `delete cond ingress old_node_4`: Delete old_node_4 from ingress.

### FlexEdge
- `insert flex <pipeline_name> <node_name> null null`: Insert a FlexEdge into a pipeline (without setting its next table pointers). `insert flex ingress flx_TE0 null null`: Insert TE0 into ingress.
- `change flex <pipeline_name> <flx_name> <true_next|false_next> <node_next>`: Change the next table of branch true/false of FlexEdge in a pipeline. `change flex ingress flx_TE0 false_next old_routing`: Change the next table of the false branch of TE0 in ingress to old_routing.
- `delete flex <pipeline_name> <flx_name>`: Delete a FlexEdge from a pipeline. `delete flex ingress flx_TE0`: Delete TE0 from ingress.
- `trigger on`: Set the global version to 1, which means all FlexEdges will switch to the true branch (new program).
- `trigger off`: Set the global version to 0, which means all FlexEdges will switch to the old branch (old program).


### Special
- `change init <pipeline_name> <table_name_next>`: Set the first table of a pipeline.`change init ingress new_acl`: Set the first table of ingress to new_acl.




## Setup

**Recommended:** You can skip the complicated setup by using our pre-configured VM image ([download](https://drive.google.com/file/d/1umMzK2CWWf4EiwDaInYPz7kDs7vmWdf9/view?usp=share_link)). 

The following is the steps to setup the environment from scratch:
1. Download VM image of mininet-2.3.0-210211-ubuntu-18.04.5-server-amd64-ovf.zip from https://github.com/mininet/mininet/releases/
2. Unzip the downloaded zip file and import the ovf to VirtualBox
3. Add a network interface for the VM: Settings -> Network -> Adapter 2 -> check "Enable Network Adapter" -> Select "Host-only Adapter" in "Attached to" -> OK
4. Configure more memory and CPU for the VM: we used 8GB memory and 4 cores.
5. Extend the disk space to 32GB (The imported vmdk file will probably at `~/VirtualBox VMs/<VM_name_you_used_when_import>/*.vmdk`)
```
# Outside VM
cd ~/VirtualBox VMs/<VM_name_you_used_when_import>/
VBoxManage clonemedium "mininet-vm-x86_64.vmdk" "flexcore.vdi" --format vdi
VBoxManage modifymedium "flexcore.vdi" --resize 32768
# Inside VM (username: mininet, password: mininet),
# follow this link https://askubuntu.com/questions/116351/increase-partition-size-on-which-ubuntu-is-installed to configure the partition
sudo fdisk /dev/sda # inside this tool, use p, d, n, a, w in order. See the link for detail
sudo reboot
sudo resize2fs /dev/sda1
```
6. In the VM, add `sudo dhclient eth1` at the end of the `~/.bashrc`, and restart the terminal
7. You should be able to have network access from now on. Follow the following steps to install all dependencies
```
sudo apt-get install -y automake cmake libjudy-dev libgmp-dev libpcap-dev libboost-dev libboost-test-dev libboost-program-options-dev libboost-filesystem-dev libboost-thread-dev libevent-dev libtool flex bison pkg-config g++ libssl-dev
sudo apt-get install -y cmake g++ git automake libtool libgc-dev bison flex libfl-dev libgmp-dev libboost-dev libboost-iostreams-dev libboost-graph-dev llvm pkg-config python python-scapy python-ipaddr python-ply tcpdump
sudo apt-get install -y doxygen graphviz texlive-full
sudo apt-get install -y python-pip
sudo apt-get install -y curl
sudo pip install cffi==1.5.2
sudo ln -s /usr/bin/mcs /usr/bin/gmcs

git clone https://github.com/protocolbuffers/protobuf.git
cd protobuf
git submodule update --init --recursive
git fetch --all --tags
git checkout -b v3.2.0 v3.2.0
./autogen.sh
./configure
make -j4
sudo make install
sudo ldconfig  # refresh shared library cache.
cd ..

git clone https://github.com/jiarong0907/runtime-programmable-switch.git
cd runtime-programmable-switch
bash travis/install-thrift.sh
bash travis/install-nanomsg.sh
bash travis/install-nnpy.sh
./autogen.sh
./configure --disable-logging-macros --disable-elogger 'CFLAGS=-O3' 'CXXFLAGS=-O3'
make -j4
sudo make install
sudo ldconfig
cd ..

git clone --recursive https://github.com/p4lang/p4c.git
cd p4c
git checkout --recurse-submodules 8b718711469c6d5f6227bb2a2fe79406faec2334
mkdir build
cd build
# cmake .. -DENABLE_EBPF=OFF -DENABLE_P4C_GRAPHS=OFF -DENABLE_P4TEST=OFF -DENABLE_DOCS=OFF
cmake .. -DENABLE_EBPF=OFF -DENABLE_P4C_GRAPHS=OFF -DENABLE_DOCS=OFF
make -j4  # There might be some errors for making test, but it will not affect running the experiments
sudo make install
sudo ldconfig
cd ../../
```

Then, you should be able to run examples under the folder `runtime-programmable-switch/runtime_examples/`. See more details in the following text.


## Running example
We put examples in `runtime_examples/` to illustrate the usage of runtime reconfiguration. To reconfigure a running p4 program from the old version to the new version at runtime, the user needs to get three files ready: 1) the old p4 program, 2) the new p4 program, and 3) the reconfiguration plan.

Take the `runtime_examples/multi-tenant` for example. We assume the user wants to migrate the ACL table from switch `s2` to switch `s3` (see Section 11.2 in our paper for more detail). If we focus on switch `s3`, then the three files are:
* `multi_tenant_without_acl.p4`: The old p4 program for `s3` that simply forwards packets based on destination IP address (see `s*-commands.txt` for the forwarding rules, and `p4app.json` for the topology)
* `multi_tenant_with_acl.p4`: The new p4 program for `s3` that is the same as the old one except that it also checks the packet header and applies ACL table if it is tcp packet.
* `command_ExecWO2W.txt`: The reconfiguration plan for `s3` which could be written by the users, or generated by our planning algorithms according to the consistency requirement.

Note that for switch `s2`, the old and new p4 program is swapped and the plan is slightly different.

To run the old p4 program, we need to compile the p4 program with p4c
```
p4c-bm2-ss --p4v 16 "multi_tenant_without_acl.p4" -o "multi_tenant_without_acl.p4.json"
```
For the runtime reconfiguration feature to work, we also need the json file for the new p4 program
```
p4c-bm2-ss --p4v 16 "multi_tenant_with_acl.p4" -o "multi_tenant_with_acl.p4.json"
```
Run the mininet (we are using a mininet environment from an older version of `p4lang/tutorials`, so if you prefer to using a newer mininet environment, please change some necessary scripts and run accordingly)
```
./run.sh
```
The switches will be up and running and forwarding traffic accordingly (see more details in `s*-commands.txt`). You can use the script `send.py` and check the pcap dump in `build/*.pcap` to observe the traffic before and after the reconfiguration.

To reconfigure the program to include the ACL table, run the `simple_switch_CLI` and submit the `runtime_reconfig` command
```
cd build
simple_switch_CLI --thrift-port 9093  # please change the thrift port of the target switch
RuntimeCmd: runtime_reconfig multi_tenant_with_acl.p4.json command_ExecWO2W.txt
```
Note that the two input files `multi_tenant_with_acl.p4.json` and `command_ExecWO2W.txt` are the new program's json and the plan respectively. Also, the working folder for the `simple_switch_CLI` is in `build/`, so the two files should exist in that folder when the command is submitted.


## Citing
If you feel our paper and code is helpful, please consider citing our paper by:
```
@inproceedings {flexcore-xing,
    author = {Jiarong Xing and Kuo-Feng Hsu and Matty Kadosh and Alan Lo and Yonatan Piasetzky and Arvind Krishnamurthy},
    title = {Runtime Programmable Switches},
    booktitle = {19th USENIX Symposium on Networked Systems Design and Implementation (NSDI 22)},
    year = {2022}
}
```

## Contact
If you have any questions about of paper and the code, please contact us:
```
Jiarong Xing (jxing@rice.edu)
Kuo-Feng Hsu (alex1230608@gmail.com)
```
