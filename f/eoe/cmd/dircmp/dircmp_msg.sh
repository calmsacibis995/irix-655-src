#!/usr/bin/sh
#     $Revision: 1.1 $
#***************************I18N File Header**********************************
#File                  : dircmp_msg.sh
#Module Name           : dircmp
#Purpose               : This file replace symbolic identifiers with 
#			 corresponding message in dircmp.src
#Author                : HCL
#Created               : 14 Jan 1998
#Compatibility Options : No support/Improper support/ EUC single byte/
#                        EUC multibyte/Sjis-Big5/Full multibyte/Unicode
#Present Compatibility : Full multibyte
#Type of Application (Normal/Important/Critical) : Normal
#Optimization Level (EUC & Single byte/Single byte/Not optimized)
#                      : Not optimized
#Change History        : 14 Jan 1998         HCL
#                        Creation
#************************End of I18N File Header**********************

catalog=uxsgicore
SOURCE_FILE=dircmp.src
DEST_FILE=dircmp.sh

get_msg ()
{
  parameter=$1
  x=`grep ${parameter}_CAT_NUM_STR $ROOT/usr/include/msgs/${catalog}.h | cut -d\" -f2 | head -1`
  def_msg=`grep ${parameter}_DEF_MSG $ROOT/usr/include/msgs/${catalog}.h | cut  -f3 `
  echo "$parameter=\`gettxt $x $def_msg\`" >> dircmp.msg
}
get_msg _MSG_DIRCMP_USAGE
get_msg _MSG_DIRCMP_NUMARGREQ
get_msg _MSG_DIRCMP_NOTADIR
get_msg _MSG_DIRCMP_ONLY
get_msg _MSG_DIRCMP_AND
get_msg _MSG_DIRCMP_DIRECTORY
get_msg _MSG_DIRCMP_SAME
get_msg _MSG_DIRCMP_DIFFERENT
get_msg _MSG_DIRCMP_EMPTYFILE
get_msg _MSG_DIRCMP_DIFFOF
get_msg _MSG_DIRCMP_IN
get_msg _MSG_DIRCMP_NOTATEXTFILE
get_msg _MSG_DIRCMP_DANGLINGLINK
get_msg _MSG_DIRCMP_COMPARISONOF
get_msg _MSG_DIRCMP_DANGLINGLINK1
get_msg _MSG_DIRCMP_SPECIAL
get_msg _MSG_DIRCMP_OPTION_TEXT
get_msg _MSG_DIRCMP_OPTION_EMPTY
get_msg _MSG_DIRCMP_OPTION_DANGLINGLINK

head -11  $SOURCE_FILE > $DEST_FILE
cat dircmp.msg >> $DEST_FILE
tail +13 $SOURCE_FILE >> $DEST_FILE

rm dircmp.msg

