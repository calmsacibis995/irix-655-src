#! smake
#
DIR      != pwd
ROOT     = ${DIR}/root
TOOLROOT = ${DIR}/troot
WORKAREA = ${DIR}
LST_FILE = file.lst
TMP_FILE = ._file.lst_

defalt: irix eoe

check:
	@if [[ ! (-d ${ROOT} && -d ${TOOLROOT}) ]];then		\
	  echo "Error: Build files were not loaded";  exit 1;	\
	fi

hippi_buildable eoe_buildable irix_buildable: check
	trg=$(@); dir=$${trg%_buildable};			\
	if [[ -d $$dir ]]; then cpio -icdu < $$dir-bld.cpio; fi

buildable: eoe_buildable irix_buildable hippi_buildable

irix eoe hippi! ${@}_buildable
	export ROOT=${ROOT} TOOLROOT=${TOOLROOT} WORKAREA=${WORKAREA};	\
	$(MAKE) -D SRC_PRD _$(@)

_irix _eoe _hippi:
	trg=$(@); dir=$${trg#_};				\
	if [[ -d $$dir ]]; then cd $$dir && $(MAKE) -k; fi

clobber clean: ${LST_FILE}
	find * -print | sort > ${TMP_FILE} && \
	comm -23 ${TMP_FILE} ${LST_FILE} | xargs rm -rf
