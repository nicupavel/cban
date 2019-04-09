
#SYNOPSIS

    Displays the current traffic on the selected interface. 
    This can be shown in bits or bytes.
    
    To install just type make in the src directory. 
    After that you can do a make install to strip
debug information and to move binary to ../bin directory.

#USAGE
        -h : print available options.
	-m : output in a suitable form for MRTG (read below).
	-r <rrd_filename> : output for rrd tool (read below). 
	-d : debug level 0 - none 1 - verbose 2 - near everything.
	-i <interface> : select interface to monitor.
	-u <update interval>: specifies (in seconds) the delay between updates.
	-b : print statistics in bits rather then in bytes.
	-k : print statistics in kilo.
						        
#USING cban WITH MRTG

    In etc directory from the package you will find a file named
    sample-mrtg.cfg.
    You can integrate (or refer) this file with your mrtg.cfg.
    [sample-mrtg.cfg]
    
    ###########################
    # eth0
    
    Title[eth0]: Traffic eth0
    MaxBytes[eth0]: 125000
    AbsMax[eth0]: 125000
    Options[eth0]: gauge
    Target[eth0]: `/usr/sbin/cban -i eth0 -m`
    PageTop[eth0]: <H1>eth0 statistics</H1>
    YLegend[eth0]: Bytes/s
    ShortLegend[eth0]: B/s
    Legend1[eth0]: Incoming Traffic
    Legend2[eth0]: Outgoing Traffic
    Legend3[eth0]: Maximum Incoming Traffic
    Legend4[eth0]: Maximum Outgoing Traffic
    LegendI[eth0]: &nbsp;In:
    LegendO[eth0]: &nbsp;Out:
    WithPeak[eth0]: ymwd
    
    ############################
    
#USING cban WITH RRDTOOL

    First you have to create the rrd database - a sample script is in 
./etc/rrdtool/.This database should contain 2 data sources incoming and
outgoing.
    After you created this database issue:
    ./cban -i eth0 -u 5 -r stats-eth0.rrd | rrdtool -
    this will update your database with new records.
