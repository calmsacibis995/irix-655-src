# Awk script to take the output of gasobjdump, strip off all the cruft
# and leave a stream of words which can be downloaded by rrdbg
#
# $Revision: 1.1 $   $Date: 1996/05/22 02:08:21 $
#
BEGIN {
	start = 0
}
{if (start > 0) {
	$0 = substr ($0, 1, 41)
	if (NF > 1) { print $2 }
	if (NF > 2) { print $3 }
	if (NF > 3) { print $4 }
	if (NF > 4) { print $5 }
}}

/Contents of section .text:/ {
	start=1;
}
