
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
#define MAX_DATA_POINTS 3600
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
typedef enum {INCOMING, OUTGOING } data_type_t;

FILE *f;
struct statistics {
    int processed;
    char ifname[255];
    unsigned long long  incoming_list[MAX_DATA_POINTS];
    unsigned long long  outgoing_list[MAX_DATA_POINTS];
    unsigned long long  data_counter;
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

unsigned long long *cban_get_data_by_type(struct statistics *stat, data_type_t type)
{
    if (type == INCOMING)
        return stat->incoming_list;
    
    if (type == OUTGOING)
	return stat->outgoing_list;

    return NULL;
}

void cban_add_current(struct statistics *stat, unsigned long long incoming, unsigned long long outgoing)
{
    int current_idx;

    stat->data_counter++;
    current_idx = stat->data_counter % MAX_DATA_POINTS;
    stat->incoming_list[current_idx] = incoming;
    stat->outgoing_list[current_idx] = outgoing;
    TRACE("%s: [%d] incoming: %llu outgoing: %llu\n", stat->ifname, current_idx, stat->incoming_list[current_idx], stat->outgoing_list[current_idx]);
}

unsigned long long * cban_get_last_entries(struct statistics *stat, int nr_entries, data_type_t type)
{
    unsigned long long *entries = NULL;
    unsigned long long *data = NULL;
    int entries_idx = 0, current_idx, i;

    if (nr_entries >= MAX_DATA_POINTS) return NULL;

    entries = (unsigned long long *) malloc(sizeof(unsigned long long) * nr_entries + 1);
    if (!entries) return NULL;

    current_idx = stat->data_counter % MAX_DATA_POINTS;

    data = cban_get_data_by_type(stat, type);

    for (i = current_idx - 1; i >= 0 && entries_idx < nr_entries; i--, entries_idx++) {
        entries[entries_idx] = data[i];
    }

    for (i = MAX_DATA_POINTS - 1 ; i > current_idx - 1 && entries_idx < nr_entries; i--,entries_idx++) {
        entries[entries_idx] = data[i];
    }

    return entries;
}

unsigned long long cban_get_current(struct statistics *stat, data_type_t type)
{
    int current_idx = stat->data_counter % MAX_DATA_POINTS;
    unsigned long long *data = cban_get_data_by_type(stat, type);
    TRACE("Type: %d [%d] Current: %llu\n", type, current_idx, data[current_idx]);
    return data[current_idx];
}

unsigned long long cban_get_last(struct statistics *stat, int seconds, data_type_t type)
{
    int current_idx = stat->data_counter % MAX_DATA_POINTS;
    int last_idx = MAX_DATA_POINTS - (seconds - current_idx);
    unsigned long long *data = cban_get_data_by_type(stat, type);
    TRACE("Type: %d [%d] Last: %llu\n", type, last_idx, data[last_idx]);
    return data[last_idx];
}

void cban_save_current(struct statistics *stat)
{
    stat->last_incoming = stat->incoming;
    stat->last_outgoing = stat->outgoing;
}

unsigned long long cban_compute_delta_2(unsigned long long last, unsigned long long current, int seconds)
{
    unsigned long long delta;

    if (current < last)
        delta = 0;
    else
        delta = (current - last) / seconds / update * 1000 / 1024;

    return delta;
}

unsigned long long cban_compute_delta_interval(struct statistics *stat, int seconds, data_type_t type)
{
    unsigned long long last, current;

    current = cban_get_current(stat, type);
    last = cban_get_last(stat, seconds, type);
    TRACE("Type: %d Last: %llu Current: %llu\n", type, last, current);

    return cban_compute_delta_2(last, current, seconds);
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

void cban_print(char *ifname, unsigned long long incoming, unsigned long long outgoing, int seconds)
{
    printf( "%s: incoming %s%s/ %d sec: %llu outgoing %s%s/ %d sec: %llu                \n",
            ifname,
            kilo, units, seconds, incoming*bits/divisor,
            kilo, units, seconds, outgoing*bits/divisor );
}

void cban_init_interfaces() 
{
    int i;
    for (i = 0; i < MAX_INTERFACES; i++) {
	struct statistics *stat = &interfaces[i];
	stat->data_counter = -1;
	stat->processed = 0;
	memset(stat->incoming_list, 0, MAX_DATA_POINTS);
	memset(stat->outgoing_list, 0, MAX_DATA_POINTS);
    }
}

int parse_proc_net_dev(void)
{
    char buffer[BUFFER], *current, *temporary;
    unsigned long long incoming, outgoing;
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
	incoming = outgoing = 0;
        struct statistics *stat = &interfaces[intf_idx];
        stat->processed = 0;

        ret = sscanf(current, "%[^:]: %llu %*u %*u %*u %*u %*u %*u %*u %llu %*u",
                     stat->ifname, &incoming, &outgoing);


        if (ret < 3)
            printf("Not all params converted %d for interface %s!\n", ret, stat->ifname);
        else
            stat->processed = 1;

	cban_add_current(stat, incoming, outgoing);

        TRACE("Interface: %s, incoming: %llu, outgoing: %llu\n", stat->ifname, incoming, outgoing);
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

/*
        for(i = 0; i < MAX_INTERFACES; i++) {
            struct statistics *stat = &interfaces[i];
            if (!stat->processed) continue;
            cban_save_current(stat);
        }
*/
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
                incoming = cban_compute_delta_interval(stat, 1, INCOMING);
		outgoing = cban_compute_delta_interval(stat, 1, OUTGOING);
                cban_print(stat->ifname, incoming, outgoing, 1);
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

    cban_init_interfaces();
    // so let's begin.
    monitor_interface();
}

