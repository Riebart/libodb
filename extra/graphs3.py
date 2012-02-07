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
import numpy

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

components = []
num=9
spe = []

for i in range(0,num):
    components.append([])


infile = open(sys.argv[1],"r")

lines = infile.readlines()

for line in lines:
    
    #line = infile.readline()
    elements = line.split()
    spe.append(0.0)
    
    #print(line)
    #print elements
    #syns.append(int(line))        
    if len(elements) > 1:
        
        for i in range(0,num):
            components[i].append(elements[i])

for i in range(num-3,num):
    for j in range(0,len(components[i])):
        spe[j] += float(components[i][j])**2
         
g.title('SPE' + ", " + sys.argv[2] + " second intervals")
g.plot(spe)
raw_input("")

for i in range (0, num):
    
    g.title('Principal component ' + str(i) + ", " + sys.argv[2] + " second intervals")
    g.plot(components[i])
    #g.plot(payload_len_ent)
    #g.hardcopy(filename=sys.argv[1]+'a.ps', eps=0, color=1, solid=1)
    raw_input("")
