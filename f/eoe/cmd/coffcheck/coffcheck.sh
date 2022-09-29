#!/bin/ksh -p
#       The flag is:
#           -p privileged (skips . $ENV)

# coffcheck
# Use: "coffcheck -help" for help.
#
# Finds COFF executable files on local file systems.
# Useful when upgrading to IRIX 6.5, which no longer supports COFF.
#
# Best to run as root, to avoid errors due to unreadable files or directories.
#
# Does a "find" command over all local file systems -- could take a while
# on systems with large local disk storage.
#
# Input:
#   Your local file systems and inst history.
#
# Output:
#   /usr/tmp/Coff.files.list:
#	    List of executable COFF files found on local file systems.
#	    (Files known to be installed using inst are intentionally
#	    excluded from this list -- see the next list.)
#   /usr/tmp/Coff.inst_subsystems.list:
#	    List of inst subsystems that contain one more currently
#	    installed executable COFF files.  These subsystems should
#	    be removed or upgraded when IRIX 6.5 is installed.
#   /usr/tmp/Coff.files+libs.list:
#		Same as Coff.files.list, but includes the shared libs each uses.
#		inst'ed subsystems are excluded.
#   /usr/tmp/Coff.libs.list:
#		Sorted list of all COFF shared libraries referenced by files
#		in Coff.files.list. inst'ed subsystems are excluded.
#   /usr/tmp/Coff.errs:
#	    Error output produced by this script's internals.
#
#   The above files are not produced if their output would be empty.


#########
# Step 1:
#   Establish a safe environment and temporary workarea in /usr/tmp/coffcheck.$$

export TMPDIR=${TMPDIR:=/usr/tmp}
export PATH=/bin:/sbin:/usr/bin:/usr/sbin:/usr/bsd:/etc:/usr/etc

# Where output list and errors go, if not empty.

coff_file_list=$TMPDIR/Coff.files.list
coff_subsys_list=$TMPDIR/Coff.inst_subsystems.list
errlist=$TMPDIR/Coff.errs

if [ $# -gt 0 ] # help
then echo '
This command accepts no options.  It checks all files on your
local filesystems for COFF executable programs.  This is the
object file format from IRIX 4.0.5 and earlier releases.
These programs will no longer execute under IRIX 6.2 and later
IRIX releases.

This program will create two output files, containing a list of
subsystems installed with "inst" that contain COFF executables
(so you know which subsystems must be upgraded to be fully
functional under IRIX 6.5 ), and also a list of COFF executables
on your system that do not appear to have been installed by
"inst".  Such programs are either locally built, copied from
other systems, or part of third party packages.  You will need to
build or otherwise get replacements for these programs, if you
will need the functions that they provide, once you have upgraded
to IRIX 6.5.  The default names of these output files are
'$coff_subsys_list and $coff_file_list',
respectively.

If you are currently running irix 4.0.x, almost all substems
installed by "inst" will be listed.  A few subsystems shipped
with IRIX 5 and IRIX 6.0.1 and 6.1 still contain COFF executables
also.
'
exit 0
fi

if test ! -w /etc/passwd
then
    echo You do not appear to be running this as super user.
    echo Some errors may occur due to unreadable files or directories as a result.\\n
fi

tmpdir=$TMPDIR/coffcheck.$$
exitval=1
trap 'cd /; exec 2>&1; fuser -kq $tmpdir $tmpdir/coffcheck.errs; rm -fr $tmpdir; trap 0; exit $exitval' 0 1 2 3 15
mkdir $tmpdir

cd $tmpdir	# Create tmp files in this temporary directory

exec 2>coffcheck.errs

#########
# Step 2:
#   If someone under a different uid has already run this script,
#   we won't be able to write the output files -- add a unique
#   numeric suffix so we can write them:

mkunique()
{
    (rm -f $1; ls -ld $1) 1>&- 2>&- || {
	eval $2=$1
	return
    }

    n=1
    while (rm -f $1.$n; ls -ld $1.$n) 1>&- 2>&-
    do
	let n=n+1
    done
    eval $2=$1.$n
}

mkunique $coff_file_list coff_file_list
mkunique $coff_subsys_list coff_subsys_list
mkunique $errlist errlist

#########
# Step 3:
#   Find all COFF files on local file systems.

echo Searching local filesystems for COFF executable files ... '\c'

# anything in the /stand and /usr/stand dirs are ommitted, as some of these
# must remain COFF on many systems in order for the PROM to be able to execute
# them.  Similarly with /unix*
# the first sed is in case filenames have embedded spaces or tabs
find / -local -type f \( -perm -100 -o -perm -10 -o -perm -1 \) -print |
	sed 's/[ 	]/\\&/g' |
	xargs file |
	egrep 'MIPSEB.*COFF|	mipseb ' |
	sed -e 's/:	.*//' -e '/^\/stand\//d' -e '/^\/usr\/stand/d' -e '/^\/unix/d' |
	sort -u > find.list

# The printer GUI files often come in both COFF and ELF forms,
# so IRIX 4 systems can use them also (clients copy them from the
# server).
if grep /var/spool/lp/gui_model find.list > lpgui.list
then # found some lp gui files, don't complain about coff equivalents
	# if ELF versions installed
	sed 's,gui_model/,&ELF/,' lpgui.list | ( while read elf rest; do
		if [ -x "$elf" ]; then echo $elf >> lpgui.elf;
		fi
		done )
	sed s,/ELF/,/, lpgui.elf > lpgui.rem
	if [ -s lpgui.rem ]; then
		fgrep -f lpgui.rem -v find.list > find.nlist && \
			mv find.nlist find.list
	fi
fi

echo '\n'Preparing results ... '\c'

#########
# Step 4:
#   Compile realpath - used to canonicalize 'versions long' output pathnames.
#   If unable to compile, realpath defaults to a no-op command.
#   A second field is expected on each line, and passed through unchanged.

export TOOLROOT=/
[ -d /usr/sysgen/root/usr/bin ] && TOOLROOT=/usr/sysgen/root
[ -d /usr/cpu/sysgen/root/usr/bin ] && TOOLROOT=/usr/cpu/sysgen/root

PATH=$TOOLROOT/usr/bin:$PATH

cat <<!! > realpath.c
    char buf1[1024], buf2[1024], buf3[1024], buf4[1024];

    /*
     * input: two fields per line (1) pathname, (2) string (inst subsys)
     * output: two fields per line (1) realpath of pathname, (2) same string
     */

    main()
    {
	while (gets (buf1) != 0) {
	    sscanf (buf1, "%s %s", buf2, buf3);
	    realpath (buf2, buf4);
	    printf ("%s %s\n", buf4, buf3);
	}
	return 0;
    }
!!

cc -o realpath realpath.c 2>/dev/null || echo cat > realpath
chmod +x realpath

#########
# Step 5:
#   Obtain list of inst'd files from "versions long".
#   Canonicalize these pathnames using realpath.
#   Feed that stream to two command pipes (via two named pipes).
#   The two command pipes calculate (1) the COFF files that
#   weren't installed via inst, and (2) the inst subsystems
#   which have one or more COFF files installed.

mknod pipe1 p	    # named pipe to feed first command pipe
mknod pipe2 p	    # named pipe to feed second command pipe

#############
# SubStep 5a:
#   First command pipe.
#   Calculate list of COFF files that were not inst'd.

< pipe1 awk '{print $1}' |
    sort -u |
    comm -23 find.list - > coff.file.list &
    
#############
# SubStep 5b:
#   Second command pipe.
#   Calculate list of inst subsystems having one or more COFF files.

< pipe2 sort -u |
    join -j 1 -o 1.2 - find.list |
    sort -u > coff.subsys.list &

#############
# SubStep 5c:
#   Common source for the two command pipes.
#   Obtain list of inst'd files from "versions long",
#   and feed to the above two command pipes.

versions long |
    awk '$1 == "f" {print "/" $NF, $4}' |
    ./realpath |
    tee pipe1 > pipe2

wait

#########
# Step 6:
#   Present results to user.

echo '\n'

if [ -s coff.file.list ]
then
    mv coff.file.list $coff_file_list
    echo List of files that won\'t run under IRIX 6.5 is in:
    echo '\t'$coff_file_list
else
    echo There are apparently no files on this system that won\'t run under IRIX 6.5
fi

if [ -s coff.subsys.list ]
then
    mv coff.subsys.list $coff_subsys_list
    echo List of inst subsystems containing files that won\'t run under IRIX 6.5 is in:
    echo '\t'$coff_subsys_list
else
    echo There are no inst subsystems containing files that won\'t run under IRIX 6.5
fi

if [ -s coffcheck.errs ]
then
    mv coffcheck.errs $errlist
    echo Some errors occurred during check, they are in:
    echo '\t'$errlist
fi

exitval=0
