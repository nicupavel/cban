#!/bin/bash
./rrdtool create stats-eth0.rrd --step 300 \
DS:incoming:DERIVE:600:0:U  \
DS:outgoing:DERIVE:600:0:U  \
RRA:AVERAGE:0.5:1:288 \
RRA:AVERAGE:0.5:12:168 \
RRA:AVERAGE:0.5:288:30 \
RRA:AVERAGE:0.5:288:365 \
RRA:MAX:0.5:12:168 \
RRA:MAX:0.5:288:30 \
RRA:MAX:0.5:288:365 \

