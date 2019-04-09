#!/bin/bash
# Updating stats-eth0 file with eth0 statistics do this from cron every 5m
./cban -i eth0 -r stats-eth0.rrd | ./rrdtool -
