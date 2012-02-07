#!/usr/bin/python

# MPL2.0 HEADER START
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# MPL2.0 HEADER END
#
# Copyright 2010-2012 Michael Himbeault and Travis Friesen
#

import Gnuplot
import sys

if len(sys.argv) < 3:
    print "Error, more args\n"
    sys.exit()

g = Gnuplot.Gnuplot()
g('set parametric')
g('set style data points')
g('set pointsize 0.5')
g('set hidden')
g.xlabel('Pseudo-time')
#g.ylabel('RTT (s)')

ip_src_ent = []
ip_src_gini = []
ip_dst_ent = []
ip_dst_gini = []
src_port_ent = []
src_port_gini = []
dst_port_ent = []
dst_port_gini = []
payload_len_ent = []
payload_len_gini = []

syns = []

infile = open(sys.argv[1],"r")

lines = infile.readlines()

for line in lines:
    
    #line = infile.readline()
    elements = line.split(",")
    
    #print(line)
    
    #syns.append(int(line))        
    
    if (len(elements) > 1):
        ip_src_ent.append(elements[0])
        ip_src_gini.append(elements[1])
        ip_dst_ent.append(elements[2])
        ip_dst_gini.append(elements[3])
        src_port_ent.append(elements[4])
        src_port_gini.append(elements[5])
        dst_port_ent.append(elements[6])
        dst_port_gini.append(elements[7])
        
        if (len(elements) > 8):
            payload_len_ent.append(elements[8])
            payload_len_gini.append(elements[9])
            #payload_len_ent.append(elements[10])
            

g.title('Entropy, ' + sys.argv[2] + " second intervals")
#g.plot(syns)
g.plot(ip_src_ent, ip_dst_ent, src_port_ent, dst_port_ent, payload_len_ent)
#g.plot(payload_len_ent)
g.hardcopy(filename=sys.argv[1]+'a.ps', eps=0, color=1, solid=1)
raw_input("")

g.title('Gini coefficient, ' + sys.argv[2] + " second intervals")
g.plot(ip_src_gini, ip_dst_gini, src_port_gini, dst_port_gini, payload_len_gini)

#g.plot(Gnuplot.File("rttproxy.txt", title=None))
g.hardcopy(filename=sys.argv[1]+'b.ps', eps=0, color=1, solid=1)
raw_input("")
