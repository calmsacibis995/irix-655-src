#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <setjmp.h>
#include <signal.h>
#include <sys/sbd.h>
#include <sys/PCI/bridge.h>

/* Program to dump the bridge registers on a HIPPI-Serial card. */

void
usage()
{
    printf("dumpbridge -m <module> -i <ioslot>\n");
    printf("\tdefault module = 1\n");
    printf("\t        ioslot = 3\n");
}

int module = 1;
int ioslot = 3;
char * devfmt = "/hw/module/%d/slot/io%d/hippi_serial/pci/controller";
char * devfmt_noNIC = "/hw/module/%d/slot/io%d/xwidget/pci/controller";
char devname[128];

main(int argc, char *argv[])
{
    int c;
    int noNIC = 0;
    int mmemfd;
    bridge_t * bregs;

    setbuf(stdout, 0);
    setbuf(stderr, 0);

    while ((c = getopt(argc, argv, "ni:m:")) != EOF) {
	switch (c) {
	    case 'i':
	    	ioslot = strtoul(optarg, NULL, 0);
		break;
	    case 'n':
		noNIC = 1;
		break;
	    case 'm':
	        module = strtoul(optarg, NULL, 0);
		break;
	    default:
		usage();
		exit(1);
	}
    }

    if (noNIC)
	sprintf (devname, devfmt_noNIC, module, ioslot);
    else
	sprintf (devname, devfmt, module, ioslot);

    if ((mmemfd = open(devname, O_RDWR)) < 0) {
	fprintf (stderr, "error opening %s: %s\n",
		 devname, strerror(errno));
	exit(1);
    }
    bregs = (void *) mmap (NULL, sizeof (bridge_t), PROT_READ | PROT_WRITE, 
			   MAP_SHARED, mmemfd, 0 );

    if (bregs == (bridge_t *)-1) {
	fprintf(stderr, "mmap() failed: %s\n", strerror(errno));
	close (mmemfd);
	exit(1);
    }
    printf ("Bridge registers for IO slot %d\n", ioslot);

    printf ("Widget registers:\n");
    printf ("\twidget ID = %x\n", bregs->b_wid_id);
    printf ("\twidget status = %x\n", bregs->b_wid_stat);
    printf ("\twidget err_upper = %x\n", bregs->b_wid_err_upper);
    printf ("\twidget err_lower = %x\n", bregs->b_wid_err_lower);
    printf ("\twidget control = %x\n", bregs->b_wid_control);
    printf ("\twidget req timeout= %x\n", bregs->b_wid_req_timeout);
    printf ("\twidget int upper= %x\n", bregs->b_wid_int_upper);
    printf ("\twidget int lower= %x\n", bregs->b_wid_int_lower);
    printf ("\twidget err cmdword= %x\n", bregs->b_wid_err_cmdword);
    printf ("\twidget llp= %x\n", bregs->b_wid_llp);
    printf ("\twidget tflush= %x\n", bregs->b_wid_tflush);
    printf ("\twidget aux err= %x\n", bregs->b_wid_aux_err);
    printf ("\twidget resp upper= %x\n", bregs->b_wid_resp_upper);
    printf ("\twidget resp lower= %x\n", bregs->b_wid_resp_lower);

    printf ("\nPMU & Map:\n");
    printf ("\tDirect Map Register = %x\n", bregs->b_dir_map);

    printf ("\nSSRAM:\n");
    printf ("\tSRAM parity err = %x\n", bregs->b_ram_perr);

    printf ("\nArbitration:\n");
    printf ("\tArb priority reg = %x\n", bregs->b_arb);

    printf ("\nNIC:\n");
    printf ("\tnic reg = %x\n", bregs->b_nic);

    printf ("\nPCI/GIO:\n");
    printf ("\tpci bus timeout = %x\n", bregs->b_pci_bus_timeout);
    printf ("\tpci type1 config = %x\n", bregs->b_pci_cfg);
    printf ("\tpci err upper = %x\n", bregs->b_pci_err_upper);
    printf ("\tpci err lower = %x\n", bregs->b_pci_err_lower);

    printf ("\nInterrupts:\n");
    printf ("\tinterrupt status = %x\n", bregs->b_int_status);
    printf ("\tinterrupt enable = %x\n", bregs->b_int_enable);
    printf ("\tinterrupt mode = %x\n", bregs->b_int_mode);
    printf ("\tinterrupt device = %x\n", bregs->b_int_device);
    printf ("\thost err field = %x\n", bregs->b_int_host_err);
    printf ("\thost0 host addr = %x\n", bregs->b_int_addr[0].addr);
    printf ("\thost1 host addr = %x\n", bregs->b_int_addr[1].addr);
    printf ("\thost2 host addr = %x\n", bregs->b_int_addr[2].addr);
    printf ("\thost3 host addr = %x\n", bregs->b_int_addr[3].addr);
    printf ("\thost4 host addr = %x\n", bregs->b_int_addr[4].addr);
    printf ("\thost5 host addr = %x\n", bregs->b_int_addr[5].addr);
    printf ("\thost6 host addr = %x\n", bregs->b_int_addr[6].addr);
    printf ("\thost7 host addr = %x\n", bregs->b_int_addr[7].addr);
 
    printf ("\nDevice Registers:\n");
    printf ("\tDevice0 register = %x\n", bregs->b_device[0].reg);
    printf ("\tDevice1 register = %x\n", bregs->b_device[1].reg);
    printf ("\tDevice2 register = %x\n", bregs->b_device[2].reg);
    printf ("\tDevice3 register = %x\n", bregs->b_device[3].reg);
    printf ("\tDevice4 register = %x\n", bregs->b_device[4].reg);
    printf ("\tDevice5 register = %x\n", bregs->b_device[5].reg);
    printf ("\tDevice6 register = %x\n", bregs->b_device[6].reg);
    printf ("\tDevice7 register = %x\n", bregs->b_device[7].reg);

    printf ("\nWrite Request Buffer Registers:\n");
    printf ("\tWRB 0 = %x\n", bregs->b_wr_req_buf[0].reg);
    printf ("\tWRB 1 = %x\n", bregs->b_wr_req_buf[1].reg);
    printf ("\tWRB 2 = %x\n", bregs->b_wr_req_buf[2].reg);
    printf ("\tWRB 3 = %x\n", bregs->b_wr_req_buf[3].reg);
    printf ("\tWRB 4 = %x\n", bregs->b_wr_req_buf[4].reg);
    printf ("\tWRB 5 = %x\n", bregs->b_wr_req_buf[5].reg);
    printf ("\tWRB 6 = %x\n", bregs->b_wr_req_buf[6].reg);
    printf ("\tWRB 7 = %x\n", bregs->b_wr_req_buf[7].reg);

    printf ("\teven RRB  = %x\n", bregs->b_even_resp);
    printf ("\todd  RRB  = %x\n", bregs->b_odd_resp);

    printf ("\tresp status = %x\n", bregs->b_resp_status);

    if (bregs != (bridge_t *) -1)
	    munmap ((void *)bregs, sizeof (bridge_t));

    close (mmemfd);
}
