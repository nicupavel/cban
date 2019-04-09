
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

#define VERSION "0.1.8-0"
#define PROCFILE "/proc/net/dev"
#define UPDATE_INTERVAL 1
#define DEBUG(x) if ( debug >= 1 ) x;
#define DEBUG2(x) if ( debug >= 2 ) x;
#define BUFFER 255

char *interface,*units="bytes",*kilo="",*rrd_filename;
static int update,debug=0,bits=1,divisor=1;
static int use_format=0; // not for console
static int format_type=0; // 0 - mrtg , 1-rrdtool

FILE *fd;
struct statistics 
{
	unsigned long  incoming,outgoing;
};

void parse (char **input);
void monitor_interface();

void help ()
{
	printf ("\nCurrent BANdwidth by Nicu Pavel <npavel@linuxconsulting.ro>\n\n");
	printf ("Version: %s\n",VERSION);
	printf ("Usage:\n");
	printf ("\t-h : print available options.\n");
	printf ("\t-m : output in a suitable form for MRTG.\n");
	printf ("\t-r <rrd_filename> : output in a suitable form for RRDTOOL.\n");
	printf ("\t-d : debug level 0 - none 1 - verbose 2 - near everything \n\t- default 0.\n");
	printf ("\t-i <interface> : select interface to monitor.\n");
	printf ("\t-u <update interval>: specifies (in seconds) the delay between updates - default 1s.\n");
	printf ("\t-b : print statistics in bits rather then in bytes.\n");
	printf ("\t-k : print statistics in kilo.\n");
}

int process_data (struct statistics *stat )
{
	char buffer[ BUFFER ], *current, *temporary, *token;
	int location;
	
	// current pointer address is buffer address
	current = buffer;
	memset( buffer, 0, BUFFER );
	
	fgets( buffer, BUFFER, fd );
	fgets( buffer, BUFFER, fd );
	
	while( fgets( buffer, BUFFER, fd ) )
	{
		buffer[strlen(buffer)-1] = 0;
		DEBUG2(printf ("debug:%s\n",current));
		parse( &current );
		DEBUG2(printf ("debug:%s\n",current));
		// there is a space there so removeit.
		temporary = current + 1; 
		DEBUG2(printf ("debug:%s\n",temporary));
		if( !strncmp( temporary, interface, strlen(interface)))
		{
			DEBUG2(printf ("debug:interface not matching skipping.\n"));
			break;
		}
	} 
	// first delimiter after the interface
	current = strchr( temporary, ':' )+1;
	temporary = current;
	location = 0;
	while( temporary )
	{
		//extract space from the temporary buffer
		token = strsep( &temporary, " " );
		if( !*token )
			continue;
		// now the token points to a number not a space
		location++;
		if( location == 1 ) 
		{
			stat->incoming = strtoul(token,NULL,10);
			continue;
		}
		if( location == 9 )
		{
			stat->outgoing = strtoul(token,NULL,10);
			break;
		}
	}
	
	fclose(fd);
	DEBUG(printf ("debug:closed %s at %x.\n",PROCFILE,fd));
	return open_proc_file();
}

void monitor_interface()
{
	struct statistics previous,current;
	unsigned long  incoming, outgoing;
	
	// if we are using format for rrdtool
	// we must do this before everithig starts
	// so in a case of error we fail ok.
	// (we are piped with rrdtool)
	if (use_format && format_type){
		printf ("update %s N:",rrd_filename);
	}
	 
	if( process_data(&previous))
	{
	    if (use_format && format_type){
		printf ("U:U\n");
		exit (1);
	    } else {
		fprintf( stderr, "problems in process_data function.\n" );
		exit(1);
	    }
	}

	if (! use_format) {
	    
	    printf("%cc",27);              // reset terminal
	    printf("%c[2J",27);            // clear screeen
	    
	    while(1)
	    {
		sleep(update);
		process_data(&current);
		incoming = (current.incoming - previous.incoming) / update * 1000 / 1024;
		outgoing = (current.outgoing - previous.outgoing) / update * 1000 / 1024;
		
		printf("%c[H",27); // use escape to put the cursor up
		
		printf( "incoming %s%s/sec: %lu outgoing %s%s/sec: %lu                \n", 
			kilo, units, incoming*bits/divisor, 
			kilo, units, outgoing*bits/divisor );
			
		DEBUG2 (printf( "total incoming %s%s: %lu total outgoing %s%s: %lu\n",
			kilo, units, previous.incoming*bits/divisor, 
			kilo, units, previous.outgoing*bits/divisor ));
			
		DEBUG (printf( "total incoming %s%s: %lu total outgoing %s%s: %lu\n", 
			kilo, units, current.incoming*bits/divisor, 
			kilo, units, current.outgoing*bits/divisor ));
		
		memcpy(&previous,&current,sizeof(struct statistics)); // let's save current data
	    }
	}
	// just output one set of values for mrtg or rrdtool.
	// rrdtool use total number of bytes in/out.
	else {
    	    if (format_type)
	    {	
		// rrdtool
		printf( "%lu:%lu\n", 
		previous.incoming*bits/divisor, 
		previous.outgoing*bits/divisor );
		} else {
		    //mrtg
	    	    sleep(update);
		    process_data(&current);
		    incoming = (current.incoming - previous.incoming) / update * 1000 / 1024;
		    outgoing = (current.outgoing - previous.outgoing) / update * 1000 / 1024;
		    printf( "%lu\n%lu\n", 
		    incoming*bits/divisor, 
		    outgoing*bits/divisor );
		}	
	}
}

void parse( char **in )
{  
	char *string;
	int remove, gotspace;
	
	string = *in;
	remove = 1;
	gotspace = 0;
	while( *string ) {
		switch( *string ) {
		case ' ':
		case '\t':
			if( remove ) {
				if( gotspace ) {
					strcpy( string, string+1 );
				} else {
					gotspace = 1;
					string++;
				} 
			} else {
				string++;
			}
			break;
		case '"':
			if( *(string-1) != '\\' )
				remove = ! remove;
			string++;
			break;
		default:
			gotspace = 0;
			string++;
		}
	}
}

int open_proc_file ()
{
    fd = fopen (PROCFILE,"r");
    if (fd == NULL)
    {
	if (use_format && format_type){
	    printf ("U:U\n");
	    return 1;
	} else {
	    fprintf (stderr,"error:unable to open %s for reading.\n", PROCFILE);
	    return 1;
	}
    }
    DEBUG (printf ("debug:opened %s at %x.\n",PROCFILE,fd));
    return 0;
}

int main ( int argc, char *argv[] )
{
	int option;
	
	while (( option = getopt ( argc, argv, "hbkd:i:u:mr:")) != -1 )
	{
		switch (option) 
		{
		case 'h':
			help();
			return 0;
		case 'd':
			debug = atoi (optarg);
			DEBUG(printf ("debug: debug level: %d\n",debug));
			break;
		case 'u':
			update = atoi (optarg);
			DEBUG (printf ("Updating every %d seconds.\n",update));
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
			DEBUG(printf ("Interface: %s\n",interface));
			break;
		case 'm':
			use_format = 1;
			format_type = 0;
			debug = 0;
			break;
		case 'r':
			use_format = 1;
			format_type = 1;
			rrd_filename = strdup (optarg);
			debug = 0;
		}
	}
	if ( !interface ) 
	{
		printf ("error: you must specify a interface.\nexample: %s -i eth0.\n",argv[0]);
		return 0;
	}
	if ( use_format && format_type && !rrd_filename )
	{
		printf ("error: you must specify what file to update when using rrdtool option.\n");
		return 0;
	}
	if ( !update ) 
	{
		DEBUG (printf ("warning: using default update interval at %d seconds.\n",UPDATE_INTERVAL));
		update = UPDATE_INTERVAL;
	}
	if ( open_proc_file () ) 
	{
		return 1;
	}
	// so let's begin.
	monitor_interface();
}

