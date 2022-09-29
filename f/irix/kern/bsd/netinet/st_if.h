#ifndef __ST_IF_H__
#define __ST_IF_H__

#include "st.h"
#include "sys/types.h"
#include "sys/st_ifnet.h"

#define max_stu max_STU

void if_st_attach(struct ifnet *ifp, st_ifnet_t *st_ifp);
#endif
