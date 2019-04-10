
/*  CurrentBANdwidth displays current traffic on a selected interfaces
 *  Copyright 2001 (C) Nicu Pavel <npavel@linuxconsulting.ro>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
*/

/*
TODO:
	print the value nicer (K,M,T).
	make a log file for graphs.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#define VERSION "0.1.9-0"
#define PROCFILE "/proc/net/dev"
#define UPDATE_INTERVAL 1
#define DEBUG(fmt, ...) do { if (debug >= 1) fprintf(stderr, fmt, __VA_ARGS__);} while(0)
#define TRACE(fmt, ...) do { if (debug >= 2) fprintf(stderr, fmt, __VA_ARGS__);} while(0)
#define MAX_INTERFACES 20
#define BUFFER 255
#define RX_POS 1
#define TX_POS 9

char *interface = NULL;
char *units="bytes";
char *kilo="";
char *rrd_filename;

static int update,debug=0,bits=1,divisor=1;
typedef enum {MRTG, RRDTOOL, CONSOLE_CLEAR, PLAIN} output_format_t;
static output_format_t output_format = PLAIN;

FILE *f;
struct statistics {
	int processed;
	char ifname[255];
	unsigned long long  last_incoming;
	unsigned long long  last_outgoing;
	unsigned long long  incoming;
	unsigned long long  outgoing;
	unsigned long long  delta_incoming;
	unsigned long long  delta_outgoing;
};

struct statistics interfaces[MAX_INTERFACES];
void monitor_interface();

void help ()
{
	printf ("\nCurrent BANdwidth by Nicu Pavel <npavel@linuxconsulting.ro>\n\n");
	printf ("Version: %s\n",VERSION);
	printf ("Usage:\n");
	printf ("\t-h : print available options.\n");
	printf ("\t-c : console auto clear output.\n");
	printf ("\t-m : output in a suitable form for MRTG.\n");
	printf ("\t-r <rrd_filename> : output in a suitable form for RRDTOOL.\n");
	printf ("\t-d : debug level 0 - none 1 - verbose 2 - nearly everything \n\t- default 0.\n");
	printf ("\t-i <interface> : output only this interface\n");
	printf ("\t-u <update interval>: specifies (in seconds) the delay between updates - default 1s.\n");
	printf ("\t-b : print statistics in bits rather then in bytes.\n");
	printf ("\t-k : print statistics in kilo.\n");
}

int open_proc_file ()
{
	f = fopen (PROCFILE,"r");
	if (f == NULL) {
		if (output_format == RRDTOOL) {
			printf ("U:U\n");
			return 0;
		}

		fprintf (stderr,"error:unable to open %s for reading.\n", PROCFILE);
		return 0;
	}
	DEBUG("opened %s at %p.\n",PROCFILE, f);
	return 1;
}

void cban_save_current(struct statistics *stat)
{
	stat->last_incoming = stat->incoming;
	stat->last_outgoing = stat->outgoing;
}

void cban_compute_delta(struct statistics *stat)
{
	if (stat->incoming < stat->last_incoming)
		stat->delta_incoming = 0;
	else
		stat->delta_incoming = (stat->incoming - stat->last_incoming) / update * 1000 / 1024;

	if (stat->outgoing < stat->last_outgoing)
		stat->delta_outgoing = 0;
	else
		stat->delta_outgoing = (stat->outgoing - stat->last_outgoing) / update * 1000 / 1024;
}

void cban_print(struct statistics *stat)
{
	printf( "%s: incoming %s%s/sec: %llu outgoing %s%s/sec: %llu                \n",
		stat->ifname,
		kilo, units, stat->delta_incoming*bits/divisor,
		kilo, units, stat->delta_outgoing*bits/divisor );

}


int parse_proc_net_dev(void)
{
	char buffer[BUFFER], *current, *temporary;
	int  ret, intf_idx = 0;

	if(!open_proc_file())
		return 0;

	// current pointer address is buffer address
	current = buffer;
	memset(buffer, 0, BUFFER );

	// skip 2 lines
	temporary = fgets(buffer, BUFFER, f);
	temporary = fgets(buffer, BUFFER, f);

	while(fgets(buffer, BUFFER, f)) {
		struct statistics *stat = &interfaces[intf_idx];
		stat->processed = 0;

		ret = sscanf(current, "%[^:]: %llu %*u %*u %*u %*u %*u %*u %*u %llu %*u",
			     stat->ifname, &stat->incoming, &stat->outgoing);

		if (ret < 3)
			printf("not all params converted %d!\n", ret);
		else
			stat->processed = 1;

		//printf("Interface: %s, incoming: %llu, outgoing: %llu\n", stat.ifname, stat.incoming, stat.outgoing);
		intf_idx++;
		if (intf_idx >= MAX_INTERFACES)
			break;
	}

	fclose(f);
	DEBUG("closed %s at %p.\n", PROCFILE, f);
	return 1;
}



void monitor_interface()
{
	int i;
	unsigned long long  incoming, outgoing;

	// if we are using format for rrdtool
	// we must do this before everithig starts
	// so in a case of error we fail ok.
	// (we are piped with rrdtool)
	if (output_format == RRDTOOL) {
		printf ("update %s N:", rrd_filename);
	}

	// initial state
	if(!parse_proc_net_dev()) {
		if (output_format == RRDTOOL) {
			printf ("U:U\n");
		} else {
			fprintf( stderr, "Error in parse_proc_net_dev() function.\n" );
		}
		exit(1);
	}


	if (output_format > RRDTOOL) {
		if (output_format == CONSOLE_CLEAR) {
			printf("%cc",27);              // reset terminal
			printf("%c[2J",27);            // clear screeen
		}

		for(i = 0; i < MAX_INTERFACES; i++) {
			struct statistics *stat = &interfaces[i];
			if (!stat->processed) continue;
			cban_save_current(stat);
		}

		while(1) {
			sleep(update);
			parse_proc_net_dev();

			if (output_format == CONSOLE_CLEAR)
				printf("%c[H",27); // use escape to put the cursor up

			for(i = 0; i < MAX_INTERFACES; i++) {
				struct statistics *stat = &interfaces[i];
				if (!stat->processed) {
					continue;
				}
				cban_compute_delta(stat);
				cban_save_current(stat);
				cban_print(stat);
			}
		}
	}
	// just output one set of values for mrtg or rrdtool.
	// rrdtool use total number of bytes in/out.
	else {
		if (output_format == RRDTOOL) {
//		printf( "%llu:%llu\n",
//		previous.incoming*bits/divisor,
//		previous.outgoing*bits/divisor );
		} else {
//		sleep(update);
//		parse_proc_net_dev();
//		incoming = (current.incoming - previous.incoming) / update * 1000 / 1024;
//		outgoing = (current.outgoing - previous.outgoing) / update * 1000 / 1024;
//		printf( "%llu\n%llu\n", incoming*bits/divisor, outgoing*bits/divisor );
		}
	}
}


int main ( int argc, char *argv[] )
{
	int option;

	while (( option = getopt ( argc, argv, "hbkcd:i:u:mr:")) != -1 ) {
		switch (option) {
		case 'h':
			help();
			return 0;
		case 'd':
			debug = atoi (optarg);
			DEBUG("debug level: %d\n",debug);
			break;
		case 'u':
			update = atoi (optarg);
			DEBUG("Updating every %d seconds.\n",update);
			break;
		case 'b':
			bits = 8;
			units = "bits";
			break;
		case 'k':
			divisor = 1024;
			kilo = "kilo";
			break;
		case 'i':
			interface = strdup (optarg);
			DEBUG("Using interface: %s\n", interface);
			break;
		case 'm':
			output_format = MRTG;
			debug = 0;
			break;
		case 'c':
			output_format = CONSOLE_CLEAR;
			break;
		case 'r':
			output_format = RRDTOOL;
			rrd_filename = strdup(optarg);
			debug = 0;
		}
	}
	if (output_format == RRDTOOL) {
		if (!rrd_filename) {
			fprintf(stderr, "error: you must specify what file to update when using rrdtool option.\n");
			return 2;
		}

		if (!interface) {
			fprintf(stderr, "error: you must specify an interface when using rrdtool output mode.\n");
			return 3;
		}

	}
	if ( !update ) {
		DEBUG("warning: using default update interval at %d seconds.\n",UPDATE_INTERVAL);
		update = UPDATE_INTERVAL;
	}

	// so let's begin.
	monitor_interface();
}

