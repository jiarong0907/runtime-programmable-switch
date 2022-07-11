/* -*- P4_16 -*- */
#include <core.p4>
#include <v1model.p4>

/* CONSTANTS */

const bit<16> TYPE_IPV4 = 0x800;
const bit<8>  TYPE_TCP  = 6;

#define DEFENCE_BLOOM_FILTER_ENTRIES 1024
#define DEFENCE_BLOOM_FILTER_BIT_WIDTH 16
#define DEFENCE_BLOOM_FILTER_BIT_WIDTH_PLUS_ONE 17
#define DEFENCE_BLOOM_FILTER_MAX_COUNTING_VALUE 65536

#define DEFENCE_BLOOM_FILTER_INCREASE_DECREASE_UNIT 1
#define DEFENCE_BLOOM_FILTER_THRESHOLD 50

#define DEFENCE_BLOOM_FILTER_RESET_PERIOD 10_000_000 // microseconds
#define DEFENCE_BLOOM_FILTER_RESET_STRENGTH 1

#define IP_SRC_COUNTING_BIT_WIDTH 16
#define IP_SRC_COUNTING_ENTRIES 2048

#define IP_SRC_RECORDING_MAX_ENTRIES 1024

#define IP_SRC_CHECKER_ENTRIES 4096

/*************************************************************************
*********************** H E A D E R S  ***********************************
*************************************************************************/

typedef bit<9>  egressSpec_t;
typedef bit<48> macAddr_t;
typedef bit<32> ip4Addr_t;

header ethernet_t {
    macAddr_t dstAddr;
    macAddr_t srcAddr;
    bit<16>   etherType;
}

header ipv4_t {
    bit<4>    version;
    bit<4>    ihl;
    bit<8>    diffserv;
    bit<16>   totalLen;
    bit<16>   identification;
    bit<3>    flags;
    bit<13>   fragOffset;
    bit<8>    ttl;
    bit<8>    protocol;
    bit<16>   hdrChecksum;
    ip4Addr_t srcAddr;
    ip4Addr_t dstAddr;
}

header tcp_t{
    bit<16> srcPort;
    bit<16> dstPort;
    bit<32> seqNo;
    bit<32> ackNo;
    bit<4>  dataOffset;
    bit<4>  res;
    bit<1>  cwr;
    bit<1>  ece;
    bit<1>  urg;
    bit<1>  ack;
    bit<1>  psh;
    bit<1>  rst;
    bit<1>  syn;
    bit<1>  fin;
    bit<16> window;
    bit<16> checksum;
    bit<16> urgentPtr;
}

struct metadata {
    /* empty */
}

struct headers {
    ethernet_t   ethernet;
    ipv4_t       ipv4;
    tcp_t        tcp;
}

register<bit<DEFENCE_BLOOM_FILTER_BIT_WIDTH> >(DEFENCE_BLOOM_FILTER_ENTRIES) defence_bloom_filter_for_ip_src;

register<bit<IP_SRC_COUNTING_BIT_WIDTH> >(IP_SRC_COUNTING_ENTRIES) ip_src_counting;
register<bit<32> >(IP_SRC_RECORDING_MAX_ENTRIES) ip_src_recording;
register<bit<32> >(1) ip_src_recording_last_pos;

register<bit<1> >(IP_SRC_CHECKER_ENTRIES) ip_src_checker;

register<bit<48> >(DEFENCE_BLOOM_FILTER_ENTRIES) last_update_time_for_defence_bloom_filter;

/*************************************************************************
*********************** P A R S E R  ***********************************
*************************************************************************/

parser MyParser(packet_in packet,
                out headers hdr,
                inout metadata meta,
                inout standard_metadata_t standard_metadata) {

    state start {
        transition parse_ethernet;
    }

    state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            TYPE_IPV4: parse_ipv4;
            default: accept;
        }
    }

    state parse_ipv4 {
        packet.extract(hdr.ipv4);
        transition select(hdr.ipv4.protocol){
            TYPE_TCP: tcp;
            default: accept;
        }
    }

    state tcp {
       packet.extract(hdr.tcp);
       transition accept;
    }
}

/*************************************************************************
************   C H E C K S U M    V E R I F I C A T I O N   *************
*************************************************************************/

control MyVerifyChecksum(inout headers hdr, inout metadata meta) {
    apply {  }
}


/*************************************************************************
**************  I N G R E S S   P R O C E S S I N G   *******************
*************************************************************************/

control MyIngress(inout headers hdr,
                  inout metadata meta,
                  inout standard_metadata_t standard_metadata) {
    bit<1> direction = 0;
    bit<1> check_ports_table_hit = 0;

    bit<32> reg_pos_0 = 0;
    bit<32> reg_pos_1 = 0;
    bit<32> reg_pos_2 = 0;

    bit<DEFENCE_BLOOM_FILTER_BIT_WIDTH> reg_pos_0_val = 0;
    bit<DEFENCE_BLOOM_FILTER_BIT_WIDTH> reg_pos_1_val = 0;
    bit<DEFENCE_BLOOM_FILTER_BIT_WIDTH> reg_pos_2_val = 0;

    action drop() {
        mark_to_drop(standard_metadata);
    }

    action ipv4_forward(macAddr_t dstAddr, egressSpec_t port) {
        standard_metadata.egress_spec = port;
        hdr.ethernet.srcAddr = hdr.ethernet.dstAddr;
        hdr.ethernet.dstAddr = dstAddr;
        hdr.ipv4.ttl = hdr.ipv4.ttl - 1;
    }

    table ipv4_lpm {
        key = {
            hdr.ipv4.dstAddr: lpm;
        }
        actions = {
            ipv4_forward;
            drop;
            NoAction;
        }
        size = 1024;
        default_action = drop();
    }

    action set_direction(bit<1> dir) {
        direction = dir;
        check_ports_table_hit = 1;
    }

    table check_ports {
        key = {
            standard_metadata.ingress_port: exact;
            standard_metadata.egress_spec: exact;
        }
        actions = {
            set_direction;
            NoAction;
        }
        size = 1024;
        default_action = NoAction();
    }

    action generate_bloom_filter_pos_using_ip_src() {
        hash(reg_pos_0, HashAlgorithm.crc16, (bit<32>)0, { hdr.ipv4.srcAddr }, (bit<32>)DEFENCE_BLOOM_FILTER_ENTRIES);
        hash(reg_pos_1, HashAlgorithm.identity, (bit<32>)0, { hdr.ipv4.srcAddr }, (bit<32>)DEFENCE_BLOOM_FILTER_ENTRIES);
        hash(reg_pos_2, HashAlgorithm.csum16, (bit<32>)0, { hdr.ipv4.srcAddr }, (bit<32>)DEFENCE_BLOOM_FILTER_ENTRIES);
    }

    table node_before_find_bloom_filter_pos_using_ip_src_0 {
        actions = {
            NoAction;
        }
        default_action = NoAction();
    }

    table find_bloom_filter_pos_using_ip_src_0 {
        actions = {
            generate_bloom_filter_pos_using_ip_src;
        }
        default_action = generate_bloom_filter_pos_using_ip_src();
    }

     table node_after_find_bloom_filter_pos_using_ip_src_0 {
        actions = {
            NoAction;
        }
        default_action = NoAction();
    }

    table node_before_find_bloom_filter_pos_using_ip_src_1 {
        actions = {
            NoAction;
        }
        default_action = NoAction();
    }

    table find_bloom_filter_pos_using_ip_src_1 {
        actions = {
            generate_bloom_filter_pos_using_ip_src;
        }
        default_action = generate_bloom_filter_pos_using_ip_src();
    }

    table node_after_find_bloom_filter_pos_using_ip_src_1 {
        actions = {
            NoAction;
        }
        default_action = NoAction();
    }

    apply {
        if (hdr.ipv4.isValid()) {
            ipv4_lpm.apply();
            if (hdr.tcp.isValid()) {
                check_ports.apply(); 
                     // client to server
                    if (check_ports_table_hit == 1) {
                        if (direction == 0) {
                            if (hdr.tcp.syn == 1) {
                                node_before_find_bloom_filter_pos_using_ip_src_0.apply();
                                find_bloom_filter_pos_using_ip_src_0.apply();
                                node_after_find_bloom_filter_pos_using_ip_src_0.apply();

                                defence_bloom_filter_for_ip_src.read(reg_pos_0_val, reg_pos_0);
                                defence_bloom_filter_for_ip_src.read(reg_pos_1_val, reg_pos_1);
                                defence_bloom_filter_for_ip_src.read(reg_pos_2_val, reg_pos_2);

                                if (((bit<DEFENCE_BLOOM_FILTER_BIT_WIDTH_PLUS_ONE>)reg_pos_0_val + DEFENCE_BLOOM_FILTER_INCREASE_DECREASE_UNIT) < 
                                                (DEFENCE_BLOOM_FILTER_MAX_COUNTING_VALUE - 1)) {
                                    reg_pos_0_val = reg_pos_0_val + DEFENCE_BLOOM_FILTER_INCREASE_DECREASE_UNIT;
                                }
                                
                                if (((bit<DEFENCE_BLOOM_FILTER_BIT_WIDTH_PLUS_ONE>)reg_pos_1_val + DEFENCE_BLOOM_FILTER_INCREASE_DECREASE_UNIT) < 
                                                (DEFENCE_BLOOM_FILTER_MAX_COUNTING_VALUE - 1)) {
                                    reg_pos_1_val = reg_pos_1_val + DEFENCE_BLOOM_FILTER_INCREASE_DECREASE_UNIT;
                                }
                                
                                if (((bit<DEFENCE_BLOOM_FILTER_BIT_WIDTH_PLUS_ONE>)reg_pos_2_val + DEFENCE_BLOOM_FILTER_INCREASE_DECREASE_UNIT) < 
                                                (DEFENCE_BLOOM_FILTER_MAX_COUNTING_VALUE - 1)) {
                                    reg_pos_2_val = reg_pos_2_val + DEFENCE_BLOOM_FILTER_INCREASE_DECREASE_UNIT;
                                }
                                
                                defence_bloom_filter_for_ip_src.write(reg_pos_0, reg_pos_0_val);
                                defence_bloom_filter_for_ip_src.write(reg_pos_1, reg_pos_1_val);
                                defence_bloom_filter_for_ip_src.write(reg_pos_2, reg_pos_2_val);

                                bit<32> ip_src_checker_pos = 0;
                                bit<1> ip_src_checker_pos_val = 0;
                                hash(ip_src_checker_pos, HashAlgorithm.crc32, (bit<32>)0, { hdr.ipv4.srcAddr }, (bit<32>)IP_SRC_CHECKER_ENTRIES);
                                ip_src_checker.read(ip_src_checker_pos_val, ip_src_checker_pos);
                                // is new hdr.ipv4.srcAddr
                                if (ip_src_checker_pos_val == 0) {
                                    ip_src_checker.write(ip_src_checker_pos, 1);
                                    bit<32> ori_ip_src_recording_last_pos;
                                    ip_src_recording_last_pos.read(ori_ip_src_recording_last_pos, 0);
                                    ip_src_recording.write(ori_ip_src_recording_last_pos, hdr.ipv4.srcAddr);
                                    assert(ori_ip_src_recording_last_pos + 1 < IP_SRC_RECORDING_MAX_ENTRIES);
                                    ip_src_recording_last_pos.write(0, ori_ip_src_recording_last_pos + 1);
                                }

                                bit<32> ip_src_counting_pos = 0;
                                bit<IP_SRC_COUNTING_BIT_WIDTH> ip_src_counting_pos_val;
                                hash(ip_src_counting_pos, HashAlgorithm.crc32, (bit<32>)0, { hdr.ipv4.srcAddr }, (bit<32>)IP_SRC_COUNTING_ENTRIES);
                                ip_src_counting.read(ip_src_counting_pos_val, ip_src_counting_pos);
                                ip_src_counting.write(ip_src_counting_pos, ip_src_counting_pos_val + 1);
                            } else if (hdr.tcp.ack == 1) {
                                node_before_find_bloom_filter_pos_using_ip_src_1.apply();
                                find_bloom_filter_pos_using_ip_src_1.apply();
                                node_after_find_bloom_filter_pos_using_ip_src_1.apply();

                                defence_bloom_filter_for_ip_src.read(reg_pos_0_val, reg_pos_0);
                                defence_bloom_filter_for_ip_src.read(reg_pos_1_val, reg_pos_1);
                                defence_bloom_filter_for_ip_src.read(reg_pos_2_val, reg_pos_2);

                                if (reg_pos_0_val >= DEFENCE_BLOOM_FILTER_INCREASE_DECREASE_UNIT) {
                                    reg_pos_0_val = reg_pos_0_val - DEFENCE_BLOOM_FILTER_INCREASE_DECREASE_UNIT;
                                }
                                if (reg_pos_1_val >= DEFENCE_BLOOM_FILTER_INCREASE_DECREASE_UNIT) {
                                    reg_pos_1_val = reg_pos_1_val - DEFENCE_BLOOM_FILTER_INCREASE_DECREASE_UNIT;
                                }
                                if (reg_pos_2_val >= DEFENCE_BLOOM_FILTER_INCREASE_DECREASE_UNIT) {
                                    reg_pos_2_val = reg_pos_2_val - DEFENCE_BLOOM_FILTER_INCREASE_DECREASE_UNIT;
                                }
                                
                                defence_bloom_filter_for_ip_src.write(reg_pos_0, reg_pos_0_val);
                                defence_bloom_filter_for_ip_src.write(reg_pos_1, reg_pos_1_val);
                                defence_bloom_filter_for_ip_src.write(reg_pos_2, reg_pos_2_val);

                                bit<32> ip_src_counting_pos = 0;
                                bit<IP_SRC_COUNTING_BIT_WIDTH> ip_src_counting_pos_val;
                                hash(ip_src_counting_pos, HashAlgorithm.crc32, (bit<32>)0, { hdr.ipv4.srcAddr }, (bit<32>)IP_SRC_COUNTING_ENTRIES);
                                ip_src_counting.read(ip_src_counting_pos_val, ip_src_counting_pos);
                                if (ip_src_counting_pos_val >= 1) {
                                    ip_src_counting.write(ip_src_counting_pos, ip_src_counting_pos_val - 1);
                                }
                            } 

                            if (hdr.tcp.syn == 1 || hdr.tcp.ack == 1) {
                                if (reg_pos_0_val >= DEFENCE_BLOOM_FILTER_THRESHOLD &&
                                    reg_pos_1_val >= DEFENCE_BLOOM_FILTER_THRESHOLD &&
                                    reg_pos_2_val >= DEFENCE_BLOOM_FILTER_THRESHOLD) {
                                        drop();
                                }

                                bit<48> reg_pos_0_last_update_time = 0;
                                bit<48> reg_pos_1_last_update_time = 0;
                                bit<48> reg_pos_2_last_update_time = 0;

                                last_update_time_for_defence_bloom_filter.read(reg_pos_0_last_update_time, reg_pos_0);
                                if (reg_pos_0_last_update_time == 0) {
                                    last_update_time_for_defence_bloom_filter.write(reg_pos_0, standard_metadata.ingress_global_timestamp);
                                }
                                last_update_time_for_defence_bloom_filter.read(reg_pos_1_last_update_time, reg_pos_1);
                                if (reg_pos_1_last_update_time == 0) {
                                    last_update_time_for_defence_bloom_filter.write(reg_pos_1, standard_metadata.ingress_global_timestamp);
                                }
                                last_update_time_for_defence_bloom_filter.read(reg_pos_2_last_update_time, reg_pos_2);
                                if (reg_pos_2_last_update_time == 0) {
                                    last_update_time_for_defence_bloom_filter.write(reg_pos_2, standard_metadata.ingress_global_timestamp);
                                }

                                if (standard_metadata.ingress_global_timestamp - reg_pos_0_last_update_time > DEFENCE_BLOOM_FILTER_RESET_PERIOD) {
                                    defence_bloom_filter_for_ip_src.write(reg_pos_0, reg_pos_0_val >> DEFENCE_BLOOM_FILTER_RESET_STRENGTH);
                                    last_update_time_for_defence_bloom_filter.write(reg_pos_0, standard_metadata.ingress_global_timestamp);
                                }

                                if (standard_metadata.ingress_global_timestamp - reg_pos_1_last_update_time > DEFENCE_BLOOM_FILTER_RESET_PERIOD) {
                                    defence_bloom_filter_for_ip_src.write(reg_pos_1, reg_pos_1_val >> DEFENCE_BLOOM_FILTER_RESET_STRENGTH);
                                    last_update_time_for_defence_bloom_filter.write(reg_pos_1, standard_metadata.ingress_global_timestamp);
                                }

                                if (standard_metadata.ingress_global_timestamp - reg_pos_2_last_update_time > DEFENCE_BLOOM_FILTER_RESET_PERIOD) {
                                    defence_bloom_filter_for_ip_src.write(reg_pos_2, reg_pos_2_val >> DEFENCE_BLOOM_FILTER_RESET_STRENGTH);
                                    last_update_time_for_defence_bloom_filter.write(reg_pos_2, standard_metadata.ingress_global_timestamp);
                                }
                            }
                        } else if (direction == 1) {
                            // if from server to client, do nothing
                        }
                    }
                }
            }
    }
}

/*************************************************************************
****************  E G R E S S   P R O C E S S I N G   *******************
*************************************************************************/

control MyEgress(inout headers hdr,
                 inout metadata meta,
                 inout standard_metadata_t standard_metadata) {
    apply {  }
}

/*************************************************************************
*************   C H E C K S U M    C O M P U T A T I O N   **************
*************************************************************************/

control MyComputeChecksum(inout headers  hdr, inout metadata meta) {
     apply {
        update_checksum(
            hdr.ipv4.isValid(),
            { hdr.ipv4.version,
              hdr.ipv4.ihl,
              hdr.ipv4.diffserv,
              hdr.ipv4.totalLen,
              hdr.ipv4.identification,
              hdr.ipv4.flags,
              hdr.ipv4.fragOffset,
              hdr.ipv4.ttl,
              hdr.ipv4.protocol,
              hdr.ipv4.srcAddr,
              hdr.ipv4.dstAddr },
            hdr.ipv4.hdrChecksum,
            HashAlgorithm.csum16);
    }
}

/*************************************************************************
***********************  D E P A R S E R  *******************************
*************************************************************************/

control MyDeparser(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.ipv4);
        packet.emit(hdr.tcp);
    }
}

/*************************************************************************
***********************  S W I T C H  *******************************
*************************************************************************/

V1Switch(
MyParser(),
MyVerifyChecksum(),
MyIngress(),
MyEgress(),
MyComputeChecksum(),
MyDeparser()
) main;
