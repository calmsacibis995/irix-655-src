/*
 * miser_cpuset.c
 * 	This file implements the user command to perform create, destroy,
 * list, query, attach process, move processes, etc. to miser cpusets.
 */   

/**************************************************************************
 *                                                                        *
 *               Copyright (C) 1992-1996 Silicon Graphics, Inc.           *
 *                                                                        *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *                                                                        *
 **************************************************************************/


#include <sys/sysmp.h>
#include <errno.h>
#include "libmiser.h"


char* pname;    /* Hold the program name for usage from argv[0] */

miser_data_t		req;	
miser_queue_cpuset_t*	cs = (miser_queue_cpuset_t*) req.md_data;


/*
 * usage
 *	Print an usage message with a brief description of each possible
 * option and quit.
 */
void
usage()
{
        fprintf(stderr,
                "\nUsage: %s [-q cpuset_name [-A cmd]|[-c -f fname]|[-d]|"
			"[-l][-m]|[-Q]]\n\t\t| -C | -Q\n\n"
                "Valid Arguments:\n"
                "\t-q cpuset_name\n"
		"\t\t-A cmd\tRuns the command on the cpuset.\n"
		"\t\t-c -f fname\n"
			"\t\t\tCreates a cpuset according to config file.\n"
		"\t\t-d\tDestroys the cpuset (no process attached).\n"
		"\t\t-l\tLists all the processes in the cpuset.\n"
		"\t\t-m\tMoves all the attached processes out of the cpuset.\n"
		"\t\t-Q\tPrints a list of the cpus belong to the cpuset.\n"
		"\t-C\tQuery cpuset, to which process is currently attached.\n"
		"\t-Q\tLists names of all the cpusets currently defined.\n\n",
			pname);
        exit(1);

} /* usage */


/*
 * cpuset_create
 *	Parse cpuset configuration file and send cpuset queue create request
 * to the kernel.
 */
int 
cpuset_create (char* qname, char* fname)
{
	FILE*	fp;		/* Hold config file pointer */
	char	buffer[1024];	/* Hold a line in the config file */
	char*	bufptr;		/* Point to the line buffer */
	int	count = 0;	/* Index counter for mqc_cpuid */
	int	line = 0;	/* Line number in config file */
	int	cpu0_exist = 0; /* CPU0 flag - initialized to false */

	/* Get total configured cpus in the system */
	int maxcpus = sysmp(MP_NPROCS);

	/* Convert qname string to qid */
	cs->mqc_queue = miser_qid(qname);

	/* Initialize mqc_flags flag to 0 */
	cs->mqc_flags = 0;
        
	/* Open cpuset config file readonly */
	if (!(fp = fopen(fname, "r"))) {
		merror ("miser_cpuset: Failed to open file %s: %s",
			fname, strerror(errno)); 
		exit(1);
        }

	/* Parse config file and load values to request buffer */
	while(fgets(buffer, sizeof(buffer), fp) != NULL) {

		int	value;		/* Hold CPU numbers in config file */
		char	key[1024];	/* String buffer to hold token */

		bufptr = &buffer[0];	/* Point to begining of the buffer */
		line++;			/* Increment config file line number */

		if (sscanf(buffer, " %s ", key) == 1) {

			if (!strcmp(key, "#")) {
				continue;	/* Ignore comment line */	
			} 

			/* Check and set flag for EXCLUSIVE cpuset */ 
			else if (!strcmp(key, "EXCLUSIVE")) { 
				cs->mqc_flags |= MISER_CPUSET_CPU_EXCLUSIVE;
			} else if (!strcmp(key, "MEMORY_LOCAL")) {
				cs->mqc_flags |= MISER_CPUSET_MEMORY_LOCAL;
			} else if (!strcmp(key, "MEMORY_EXCLUSIVE")) {
				cs->mqc_flags |= MISER_CPUSET_MEMORY_EXCLUSIVE;
			} else if (!strcmp(key, "CPU"))  {

				/* Check and set value for a CPU entry */
				bufptr += strlen("CPU");

				/* No CPU ID specified */
				if (sscanf(bufptr, " %d ", &value) != 1)  {
					merror("miser_cpuset: File %s, "
						"Line[%d]: No CPU ID.",
						fname, line);
					exit(1);
				}

				/* CPU ID specified does not exist */
				if (value >= maxcpus) {
					merror("miser_cpuset: File %s, "
						"Line[%d]: CPU ID > maxcpus.",
						fname, line);
					exit(1);
				}

				/* CPU 0 is specified - set flag */
				if (value == 0)
					cpu0_exist = 1;

				/* Assign CPU ID in the array */ 
				cs->mqc_cpuid[count] = value;
				count++;
			} 

			/* Invalid token */
			else {
				merror("miser_cpuset: File %s, Line[%d]: '%s'"
					" invalid token.", fname, line, key);
				exit(1);
			}	
		}

	} /* while */

	/* CPU 0 is requested to be part of an EXCLUSIVE cpuset - exit */
	if (cs->mqc_flags & MISER_CPUSET_CPU_EXCLUSIVE && cpu0_exist) {
		merror("miser_cpuset: File %s, Line[%d]: CPU 0 cannot be "
			"a part of an EXCLUSIVE cpuset.", fname, line);
		exit(1);
	}

	/* Assign number of CPUs to the request buffer */
	cs->mqc_count = count;

	/* Set cpuset create flag to the request */ 
        req.md_request_type = MISER_CPUSET_CREATE;

	/* Send miser cpuset create request to the kernel */
        if (sysmp(MP_MISER_SENDREQUEST, &req, fileno(fp)) == -1) {
		merror("miser_cpuset: Failed to create cpuset %s: %s",
			qname, strerror(errno));
		exit(1);
        }

	return 0;	/* Operation successful */

} /* cpuset_create */


/*
 * cpuset_attach
 *	Make cpuset attach request to the kernel and execute the command.
 */
int 
cpuset_attach(char* qname, char* cmdname, char** cmdargs)
{
	/* Convert qname string to qid */
	cs->mqc_queue = miser_qid(qname);

	/* Set cpuset attach flag to the request */
	req.md_request_type = MISER_CPUSET_ATTACH;

	/* Send miser cpuset attach request to the kernel */
	if (sysmp(MP_MISER_SENDREQUEST, &req) == -1)  {
		merror("miser_cpuset: Failed to attach to cpuset [%s]: %s", 
			qname, strerror(errno));
		exit(1);
	}

	/* Attach successful - execute the command */
	execvp(cmdname, cmdargs);

	/* Command execution failed - print error and exit */
	merror("miser_cpuset: Unable to exec program [%s]: %s", 
		cmdname, strerror(errno));
	exit(1);

	/* Operation unsuccessful */
	return 1;

} /* cpuset_attach */


/* 
 * cpuset_destroy
 * 	Make cpuset destroy request to the kernel.	
 */
int
cpuset_destroy(char* qname)
{
	/* Convert qname string to qid */
	cs->mqc_queue = miser_qid(qname);

	/* Set cpuset queue destroy flag to the request */
	req.md_request_type = MISER_CPUSET_DESTROY;

	/* Send miser cpuset queue destroy request to the kernel */
	if (sysmp(MP_MISER_SENDREQUEST, &req) == -1)  {
		merror("miser_cpuset: Failed to destroy cpuset [%s]: %s", 
			qname, strerror(errno));
		exit(1);
	}

	/* Operation successful */
	return 0;

} /* cpuset_destroy */


/*
 * cpuset_list_procs
 *	Query and print list of attached processes to a cpuset queue.
 */
int 
cpuset_list_procs(char* qname)
{
	pid_t *first, *last;	/* Pointer to a process id */

	/* Map miser_cpuset_pid structure onto the miser_data buffer */
	miser_cpuset_pid_t *cpuset_pid = (miser_cpuset_pid_t *) req.md_data;

	/* Point to the last and first pid position in the miser_data */
	last	= (pid_t *)(&req+1);
	first	= (pid_t *) cpuset_pid->mcp_pids;
	last	= first + (last - first);

	/* Initialize request buffer */
	cpuset_pid->mcp_queue = miser_qid(qname);
	cpuset_pid->mcp_count = 0;
	cpuset_pid->mcp_max_count = last - cpuset_pid->mcp_pids;

	/* Set cpuset list process flag to the request */
	req.md_request_type = MISER_CPUSET_LIST_PROCS;

	/* Send miser cpuset list process request to the kernel */
	if (sysmp(MP_MISER_SENDREQUEST, &req) == -1)  {
		merror("miser_cpuset: Failed to list procs in cpuset [%s]: %s",
			qname, strerror(errno));
		exit(1);
	}

	/* Point to the last pid in the result buffer */
	last = first + cpuset_pid->mcp_count;

	printf("\nAttached Processes to CPUSET Queue [%s]:\n", qname);
	printf("-----------------------------------------\n\n");

	/* Print pid per line */
	while(first != last) 
		printf("   %d\n", *first++);

	printf("\n");

	return 0; /* Operation successful */

} /* cpuset_list_procs */


/*
 * cpuset_move_procs
 * 	Moves all the attached processes out of the cpuset queue.
 */
int 
cpuset_move_procs(char* qname)
{
	/* Convert qname string to qid */
	cs->mqc_queue = miser_qid(qname);

	/* Set cpuset queue move process flag to the request */
	req.md_request_type = MISER_CPUSET_MOVE_PROCS;

	/* Send miser cpuset queue move process request to the kernel */
	if (sysmp(MP_MISER_SENDREQUEST, &req) == -1)  {
		merror("miser_cpuset: Failed to move procs in cpuset [%s]: %s",
			qname, strerror(errno));
		exit(1);
	}

	return 0; /* Operation sucessful */

} /* cpuset_move_procs */


/*
 * cpuset_query_current
 *	Query and prints the cpuset queue name to which the process is 
 * currently attached.
 */
int 
cpuset_query_current()
{
	/* Get total configured cpus in the system */
	int maxcpus = sysmp(MP_NPROCS);

	/* Map miser_queue_names structure onto the miser_data buffer */
	miser_queue_names_t *names = (miser_queue_names_t*) req.md_data;

	/* Set process attached cpuset query flag to the request */
        req.md_request_type = MISER_CPUSET_QUERY_CURRENT;

	/* Send process attached cpuset query request to the kernel */
        if (sysmp(MP_MISER_SENDREQUEST, &req) == -1) {
                merror("miser_cpuset: Failed to query current cpuset: %s",
			strerror(errno));
                exit(1);
        }

	
	/* Process belongs to a cpu */
        if (names->mqn_queues[0] <= maxcpus ) {
                printf("\nCurrent Process MUSTRUN to cpu: [%d]\n\n", 
			strtol((char*) &cs->mqc_queue, 0, 0));
        } 

	/* Process belongs to a CPUSET */
	else  {
		char buf[10];

		memset(buf, 0, 10);
		strncpy(buf, (char *)&names->mqn_queues[0], sizeof(id_type_t));
                printf("\nCurrent Process Attached to CPUSET: [%s]\n\n", buf);
	}

        return 0;	/* Operation successful */

} /* cpuset_query_current */


/*
 * cpuset_query_names
 * 	Query and lists the names of all the cpusets currently defined.	
 */
int 
cpuset_query_names() 
{
	int  i;	
	char buf[10];	/* Hold cpuset queue name */

	/* Get total configured cpus in the system */
	int maxcpus = sysmp(MP_NPROCS);

	/* Map miser_queue_names structure onto the miser_data buffer */
	miser_queue_names_t *names = (miser_queue_names_t*) req.md_data;

	/* Set cpuset queue names query flag to the request */
	req.md_request_type = MISER_CPUSET_QUERY_NAMES;


	/* Send cpuset queue names query request to the kernel */
	if (sysmp(MP_MISER_SENDREQUEST, &req)) {
		merror("miser_cpuset: Failed to query cpuset names: %s",
			strerror(errno));
		exit(1);
	}

	if (names->mqn_count > 0)
		printf("\nMiser CPUSET Queues:\n");
	else
		printf("\nNo Miser CPUSET Queues defined.\n");

	/* Print list of cpusets configured */
	for (i = 0; i < names->mqn_count; i++) {

		if (names->mqn_queues[i] <= maxcpus)  {
			printf("   CPU %d is restricted.\n", 
			       (int)names->mqn_queues[i]);
			continue;
		}	

		/* Initialize buffer, copy string and print */
		memset(buf, 0, 10);
		strncpy(buf, (char *)&names->mqn_queues[i], sizeof(id_type_t));
                printf("   QUEUE[%s]\n", buf);
	}

	printf("\n");

	return 0; 	/* Operation successful */

} /* cpuset_query_names */


/*
 * cpuset_query_cpus
 *	Query and lists all the cpus that belong to the specified cpuset.
 */
int 
cpuset_query_cpus(char *qname) 
{
	int i;

	/* Convert qname string to qid */
	cs->mqc_queue = miser_qid(qname);

	/* Set cpuset query cpus flag to the request */
	req.md_request_type = MISER_CPUSET_QUERY_CPUS;

	/* Send cpuset query cpus request to the kernel */
	if (sysmp(MP_MISER_SENDREQUEST, &req)) {
		merror("miser_cpuset: Could not query cpus in cpuset %s: %s",
			qname, strerror(errno));
		exit(1);
	}

	printf("\nMiser CPUSET Queue[%s] contains %d CPUs:\n",
			qname, cs->mqc_count);

	/* Print list of CPUs belongs to the cpuset */
	for (i = 0; i < cs->mqc_count ; i++) 
		printf ("   CPU[%d]\n", cs->mqc_cpuid[i]);

	printf("\n");

	return 0;	/* Operation successful */

} /* cpuset_query_cpus */


int
main(int argc, char** argv)
{
	int	c = 0;		/* Hold command line option char */
	int	attach = 0;	/* '-A' flag */
	int	command = 0;	/* Command+1 index in argv */
	int	current = 0;	/* '-C' flag */
	int	create = 0;	/* '-c' flag */
	int	destroy = 0;	/* '-d' flag */
	char*	fname = 0;	/* '-f' flag */
	int	list_procs = 0;	/* '-l' flag */
	int	move_procs = 0; /* '-m' flag */
	int	query = 0;	/* '-Q' flag */
	char*	queue = 0;	/* '-q' flag */

	/* Get program name */
	pname = argv[0];

	while ((c = getopt(argc, argv, "A:Ccdf:lmQq:")) != -1) {

		switch(c) {

		case 'A':
			attach = 1;
			command = optind;
			break;

                case 'C':
                        current = 1;
                        break;
                        
                case 'c':
                        create = 1;
                        break;

                case 'd':
                        destroy = 1;
                        break;

                case 'f':
                        fname = optarg;
                        break;	

		case 'l':
			list_procs = 1;
			break;

		case 'm':
			move_procs = 1;
			break;

                case 'Q':
                        query = 1;
                        break;

                case 'q':
                        queue = optarg;
                        break;

		} /* switch */

		if (attach)
			break;

	} /* while */

	/* Following arguments cannot co-exist */
	if (attach + query + create + destroy + current + list_procs 
		+ move_procs != 1)
		usage();

	/* Create request must have a cpuset queue name */
	if (!queue && create && fname)
		usage();

	/* Call specified cpuset queue related functions */
        if (queue) {

		/* Attach a command to the cpuset queue */
	        if (attach && command)
			cpuset_attach(queue,argv[command-1], &argv[command-1]); 
		/* Destroy the cpuset queue specified - no process attached */
		else if (destroy)
			cpuset_destroy(queue);

		/* List processes attached to the specified cpuset queue */
		else if(list_procs)
			cpuset_list_procs(queue);
 
		/* Move all processes from the cpuset queue */
		else if(move_procs)
			cpuset_move_procs(queue);

		/* List cpus belong to the specified cpuset queue */
		else if (query)
			cpuset_query_cpus(queue);

		/* Create the cpuset queue according to the config file */
		else if (create && fname)
			cpuset_create(queue, fname);

		else
			usage();
	} 

	/* Get all the cpuset names */
	else if (query) 
		cpuset_query_names();

	/* Name of the cpuset, current process is attached to */
        else if (current) 
                cpuset_query_current();

	else
		usage();

	return 0;

} /* main */
