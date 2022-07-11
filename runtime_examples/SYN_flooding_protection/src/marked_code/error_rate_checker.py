#!/usr/bin/env python3
import os
import sys
import socket
import random
from tabnanny import verbose
from urllib import response

from scapy.all import (
    TCP,
    get_if_list, 
    IP, 
    Ether, 
    get_if_hwaddr,
    sendp, 
    RandIP,
    srp1
)

TOTAL_TEST_TIMES = 70

def get_if():
    ifs=get_if_list()
    iface=None
    for i in get_if_list():
        if "eth0" in i:
            iface=i
            break;
    if not iface:
        print("Cannot find eth0 interface")
        exit(1)
    return iface


def main():
    if len(sys.argv) < 2:
        print('pass 1 arguments: <destination>')
        exit(1)

    addr = socket.gethostbyname(sys.argv[1])

    iface = get_if()

    i = 0
    fail_pkt_num = 0

    while (i < TOTAL_TEST_TIMES):
        print("[{}] round: sending on interface {} to {}".format(i, iface, str(addr)))
        pkt =  Ether(src=get_if_hwaddr(iface), dst='ff:ff:ff:ff:ff:ff')
        dport = 4097 + i
        sport = 4097 + i
        src = RandIP("192.168.1.1/24")
        pkt = pkt / IP(src=src, dst=addr) / TCP(dport=dport, sport=sport, flags='S', seq=1000)
        # pkt.show2()
       
        ans = srp1(pkt, verbose=False, timeout=3, iface=iface)
        if ans is None:
            print("timeout")
            fail_pkt_num += 1
            i += 1
            continue
        else:
            print("success")
            response = Ether(src=get_if_hwaddr(iface), dst='ff:ff:ff:ff:ff:ff') / \
                        IP(src=src, dst=addr) / \
                            TCP(sport=sport, dport=dport, flags='A', seq=ans[TCP].ack + 1, ack=ans[TCP].seq + 1)
            sendp(response, iface=iface, verbose=False)
            i += 1

    print("=" * 100)
    print("total test times: {}".format(TOTAL_TEST_TIMES))
    print("fail times: {}".format(fail_pkt_num))
    print("error rate: {:.3f}".format(fail_pkt_num / TOTAL_TEST_TIMES))
    print("=" * 100)
   

if __name__ == '__main__':
    main()
