/*
 * eoe/cmd/miser/libmiser/parse.c
 *      Library functions that parse miser data.
 */

/**************************************************************************
 *									  *
 * 		 Copyright (C) 1997 Silicon Graphics, Inc.		  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/


#include "libmiser.h"

#define MISER_BUFFER_SIZE 18	/* size of buffer to print ctime */ 


static miser_data_t*	parse_req;
static miser_qhdr_t 	parse_qhdr;

miser_qseg_t*	(*qdef_elems)(void *, uint64_t);

void*	(*qdef_alloc)(miser_qhdr_t *);
int	(*qdef_callb)(void *);


/*
 * parse
 *	Parse contents of the specified file and return the data through 
 * miser_data structure.
 */ 
miser_data_t*
parse(int16_t type, const char *fname)
{
	extern int	yyparse();
	miser_data_t *	req;
	int		error;

	if (!parse_start(type, fname))
		return (miser_data_t *) 0;

	error = yyparse();
	req = parse_stop();

	if (error && req) {
		free(req);
		return (miser_data_t *) 0;
	}

	return req;

} /* parse */


/*
 * parse_start
 *	Start parse operation based on type. 
 */
int
parse_start(int16_t type, const char *fname)
{
	parse_select(type);
	switch (type) {
	case PARSE_JSUB:
	case PARSE_QMOV:
		parse_req = (miser_data_t *) malloc(sizeof(miser_data_t));
		if (!parse_req) {
			merror("ran out of memory");
			return 0;
		}
		memset(parse_req, 0, sizeof(miser_data_t));
		break;
	case PARSE_QDEF:
		parse_req = 0;
		break;
	default:
		merror("illegal parse command (%d)", type);
		return 0;
	}

	if (fname && !parse_open(fname))
		return 0;

	return 1;
}


/*
 * parse_stop
 *      Stop parse operation. 
 */
miser_data_t*
parse_stop()
{
	parse_close();

	return parse_req;

} /* parse_stop */


/*
 * parse_jseg_start
 *      Start parsing miser job segment.
 */
miser_seg_t*
parse_jseg_start()
{
	miser_job_t* job = (miser_job_t *) parse_req->md_data;
	miser_seg_t* seg = &job->mj_segments[job->mj_count];

	if (seg > (miser_seg_t *)(((char *)parse_req) +
			sizeof(miser_data_t) - sizeof(miser_seg_t))) {
		parse_error("too many segments");
		return (miser_seg_t *) 0;
	}

	seg->ms_multiple = 1;
	seg->ms_flags = MISER_SEG_DESCR_FLAG_NORMAL;

	return seg;

} /* parse_jseg_start */


/*
 * parse_jseg_stop
 *      Stop parsing miser job segment.
 */
int
parse_jseg_stop()
{
	miser_job_t* job = (miser_job_t *) parse_req->md_data;
	miser_seg_t* seg = &job->mj_segments[job->mj_count];
	char*	     err = 0;

	if ((seg->ms_flags & MISER_SEG_DESCR_FLAG_STATIC) &&
	    seg->ms_multiple == 1)
		seg->ms_multiple = seg->ms_resources.mr_cpus;

	if ((seg->ms_flags & MISER_EXCEPTION_WEIGHTLESS) == 0)
		seg->ms_flags |= MISER_EXCEPTION_TERMINATE;

	if (seg->ms_rtime == 0)
		err = "time not specified";

	else if (seg->ms_resources.mr_cpus == 0)
		err = "mr_cpus not specified";

	else if (seg->ms_resources.mr_memory == 0)
		err = "memory not specified";

	else if ((seg->ms_flags & MISER_SEG_DESCR_FLAG_STATIC) &&
		 seg->ms_multiple != seg->ms_resources.mr_cpus)
		err = "ms_multiple can't be used with static";

	else if ((seg->ms_flags & MISER_EXCEPTION_TERMINATE) &&
		 (seg->ms_flags & MISER_EXCEPTION_WEIGHTLESS))
		err = "kill and weightless are mutually exclusive";

	if (err) {
		parse_error("invalid segment (#%d): %s", job->mj_count, err);
		return 0;
	}

	job->mj_count++;

	return 1;	/* Success */

} /* parse_jseg_stop */


/*
 * parse_qseg_start
 *      Start parsing miser queue segment.
 */
miser_qseg_t *
parse_qseg_start()
{
	miser_move_t* move;
	miser_qseg_t* qseg;

	switch(parse_type()) {

	case PARSE_QMOV:
		move = (miser_move_t *) parse_req->md_data;
		qseg = &move->mm_qsegs[move->mm_count];

		if (qseg > (miser_qseg_t *)(((char *)parse_req) +
				sizeof(miser_data_t) - sizeof(miser_qseg_t))) {
			parse_error("too many segments");
			return (miser_qseg_t *) 0;
		}

		break;

	case PARSE_QDEF:
		if (parse_qhdr.cseg >= parse_qhdr.nseg) {
			parse_error("too many segments");
			return (miser_qseg_t *) 0;
		}

		qseg = qdef_elems((void *)parse_req, parse_qhdr.cseg);
		break;

	default:
		merror("internal error");
		return (miser_qseg_t *) 0;
	}

	return qseg;

} /* parse_qseg_start */


/*
 * parse_qseg_stop
 *      Stop parsing miser queue segment.
 */
int
parse_qseg_stop()
{
	miser_move_t* move;
	miser_qseg_t* qseg;

	switch (parse_type()) {

	case PARSE_QMOV:
		move = (miser_move_t *) parse_req->md_data;
		qseg = &move->mm_qsegs[move->mm_count];
		move->mm_count++;
		break;

	case PARSE_QDEF:
		qseg = qdef_elems((void *)parse_req, parse_qhdr.cseg);
		/* XXX local quantum adjust??? */
		parse_qhdr.cseg++;
		break;

	default:
		merror("internal error");
		return 0;
	}

	return 1;	/* Success */

} /* parse_qseg_stop */


/*
 * parse_qdef_start
 *      Start parsing miser queue definition.
 */
miser_qhdr_t*
parse_qdef_start()
{
	memset(&parse_qhdr, 0, sizeof(miser_qhdr_t));

	return &parse_qhdr;

} /* parse_qdef_start */


/*
 * parse_qdef_hdr
 *      Start parsing miser queue definition header.
 */
int
parse_qdef_hdr()
{
	extern FILE* yyin;
	char* err = 0;

	parse_qhdr.file = yyin;

	if (qdef_alloc == 0 || qdef_elems == 0)
		err = "no queue allocation routine";

	else if (parse_qhdr.nseg == 0)
		err = "nseg must be positive";

	else if (!(parse_req = (miser_data_t *) qdef_alloc(&parse_qhdr)))
		err = "failed to allocate queue";

	if (err) {
		parse_error("%s", err);
		return 0;
	}

	return 1;	/* Success */

} /* parse_qdef_hdr */


/*
 * parse_qdef_stop
 *      Stop parsing miser queue definition.
 */
int
parse_qdef_stop()
{
	char* err;

	if (qdef_callb == 0)
		err = "no queue insertion routine";

	else if (parse_req == 0)
		err = "empty queue definition";

	else if (parse_qhdr.cseg != parse_qhdr.nseg)
		err = "segment number doesn't match";

	else if (qdef_callb(parse_req) == 0)
		err = "failed to insert queue";

	if (err) {
		parse_error("%s", err);
		return 0;
	}

	return 1;	/* Success */

} /* parse_qdef_stop */
