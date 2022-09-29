# Awk script to take the output of gasobjdump and convert it into 
# declarations which can be included by other files.
#
# $Revision: 1.3 $  $Date: 1996/08/05 17:42:51 $

# First word in the f/w is the startPC, which should be stripped off
# and replaced with zero. The startPC should be saved by the downloader
# program and used to write into the RR's PC before un-halting it.

BEGIN {
	start = 0
	numwords = 0
	startPC = 0
}
{if (start > 0) {
	$0 = substr ($0, 1, 41)
	if (startPC == 0) {
	    startPC = $2
	    print "    0x00000000, 0x" $3 ", 0x" $4 ", 0x" $5 ","
	    numwords = numwords + 4
	}
	else if (NF == 5) {
	    print "    0x" $2 ", 0x" $3 ", 0x" $4 ", 0x" $5 ","
	    numwords = numwords + 4
	}
	else if (NF == 4) {
	    print "    0x" $2 ", 0x" $3 ", 0x" $4
	    numwords = numwords + 3
	}
	else if (NF == 3) {
	    print "    0x" $2 ", 0x" $3
	    numwords = numwords + 2
	}
	else if (NF == 2) {
	    print "    0x" $2
	    numwords = numwords + 1
	}
}}

# Look for beginning of firmware
/Contents of section .text:/ {
	start=1;
	print "u_int32_t " which "_fw[] = {"
}

END {
	print "};\n"
	print "u_int32_t " which "_fw_base = 0;\n"
	print "int " which "_fw_size = " numwords ";\n"
	print "u_int32_t " which "_startPC = 0x" startPC ";\n"
}
