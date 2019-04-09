#! /opt/rrdtool/rrdcgi 

<HTML>
<HEAD>
<TITLE>ETH0 Traffic</TITLE>
</HEAD>
<BODY>
<P><RRD::GRAPH incoming.gif 
	    --height 170
	    --width 640
	    --vertical-label bits
	    --title "Incoming bits/s. <RRD::TIME::NOW %c>"
            DEF:value=/opt/rrdtool/stats-eth0.rrd:incoming:AVERAGE
	    CDEF:bitsvalue=value,8,*
            AREA:bitsvalue#00cc00:"Incoming">
</P>
<P><RRD::GRAPH outgoing.gif 
	    --height 170
	    --width 640
	    --vertical-label bits
	    --title "Outgoing bits/s. <RRD::TIME::NOW %c>"
            DEF:value=/opt/rrdtool/stats-eth0.rrd:outgoing:AVERAGE
	    CDEF:bitsvalue=value,8,*
            AREA:bitsvalue#0000FF:"Outgoing">
</P>

<P><RRD::GRAPH both.gif 
            --height 170
	    --width 640
	    --vertical-label bits
	    --title "Incoming&Outgoing bits/s. <RRD::TIME::NOW %c>"
	    --alt-autoscale
            DEF:incoming=/opt/rrdtool/stats-eth0.rrd:incoming:AVERAGE
	    DEF:outgoing=/opt/rrdtool/stats-eth0.rrd:outgoing:AVERAGE
	    CDEF:bitsincoming=incoming,8,*
	    CDEF:bitsoutgoing=outgoing,8,*
	    AREA:bitsincoming#00cc00:"Incoming"
	    LINE1:bitsoutgoing#0000FF:"Outgoing">
</P>

<P><RRD::GRAPH wboth.gif 
            --height 170
	    --width 640
	    --start -604800
	    --vertical-label bits
	    --title "Weekly Incoming&Outgoing bits/s. <RRD::TIME::NOW %c>"
	    --alt-autoscale
            DEF:incoming=/opt/rrdtool/stats-eth0.rrd:incoming:AVERAGE
	    DEF:outgoing=/opt/rrdtool/stats-eth0.rrd:outgoing:AVERAGE
	    CDEF:bitsincoming=incoming,8,*
	    CDEF:bitsoutgoing=outgoing,8,*
	    AREA:bitsincoming#00cc00:"Incoming"
	    LINE1:bitsoutgoing#0000FF:"Outgoing">
</P>

</BODY>
</HTML>




