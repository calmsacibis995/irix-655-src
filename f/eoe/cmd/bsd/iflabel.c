/**************************************************************************
 *                                                                        *
 *               Copyright (C) 1993, Silicon Graphics, Inc.               *
 *                                                                        *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *                                                                        *
 **************************************************************************/

#ifndef lint
static char sccsid[] = "@(#)iflabel.c\t1.00 (SGI) 7/27/91";
#endif /* not lint */

#ident "$Revision: 1.5 $"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/acl.h>
#include <sys/capability.h>
#include <sys/mac_label.h>
#include <sys/mac.h>
#include <pwd.h>
#include <grp.h>

#include <net/if.h>
#include <netinet/in.h>

#include <sys/t6attrs.h>
#include <sys/sesmgr_if.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>

static int
opt_msen_common (msen_t *msenp, const char *arg, const char *name)
{
	*msenp = msen_from_text (arg);
	if (*msenp == NULL)
	{
		fprintf (stderr, "invalid %s: %s\n", name, arg);
		return (1);
	}
	return (0);
}

static int
opt_min_msen (t6ifreq_t *t6ifr, const char *arg)
{

	return (opt_msen_common (&t6ifr->ifsec_min_msen, arg,
				 "minimum sensitivity label"));
}

static int
opt_max_msen (t6ifreq_t *t6ifr, const char *arg)
{

	return (opt_msen_common (&t6ifr->ifsec_max_msen, arg,
				 "maximum sensitivity label"));
}

static int
opt_dflt_msen (t6ifreq_t *t6ifr, const char *arg)
{
	int result;

	if (*arg == '\0')
	{
		t6ifr->ifsec_dflt.dflt_attrs &= ~T6M_SL;
		return (0);
	}
	t6ifr->ifsec_dflt.dflt_attrs |= T6M_SL;
	return (opt_msen_common (&t6ifr->ifsec_dflt.dflt_msen, arg,
				 "default sensitivity label"));
}

static int
opt_dflt_clearance (t6ifreq_t *t6ifr, const char *arg)
{
	int result;

	if (*arg == '\0')
	{
		t6ifr->ifsec_dflt.dflt_attrs &= ~T6M_CLEARANCE;
		return (0);
	}
	t6ifr->ifsec_dflt.dflt_attrs |= T6M_CLEARANCE;
	return (opt_msen_common (&t6ifr->ifsec_dflt.dflt_clearance, arg,
				 "default clearance label"));
}

static int
opt_mint_common (mint_t *mintp, const char *arg, const char *name)
{
	*mintp = mint_from_text (arg);
	if (*mintp == NULL)
	{
		fprintf (stderr, "invalid %s: %s\n", name, arg);
		return (1);
	}
	return (0);
}

static int
opt_min_mint (t6ifreq_t *t6ifr, const char *arg)
{
	return (opt_mint_common (&t6ifr->ifsec_min_mint, arg,
				 "minimum integrity label"));
}

static int
opt_max_mint (t6ifreq_t *t6ifr, const char *arg)
{
	return (opt_mint_common (&t6ifr->ifsec_max_mint, arg,
				 "maximum integrity label"));
}

static int
opt_dflt_mint (t6ifreq_t *t6ifr, const char *arg)
{
	if (*arg == '\0')
	{
		t6ifr->ifsec_dflt.dflt_attrs &= ~T6M_INTEG_LABEL;
		return (0);
	}
	t6ifr->ifsec_dflt.dflt_attrs |= T6M_INTEG_LABEL;
	return (opt_mint_common (&t6ifr->ifsec_dflt.dflt_mint, arg,
				 "default integrity label"));
}

static int
opt_dflt_acl (t6ifreq_t *t6ifr, const char *arg)
{
	if (*arg == '\0')
	{
		t6ifr->ifsec_dflt.dflt_attrs &= ~T6M_ACL;
		return (0);
	}
	t6ifr->ifsec_dflt.dflt_attrs |= T6M_ACL;
	t6ifr->ifsec_dflt.dflt_acl = acl_from_text (arg);
	if (t6ifr->ifsec_dflt.dflt_acl == NULL)
	{
		fprintf (stderr, "invalid default ACL: %s\n", arg);
		return (1);
	}
	return (0);
}

static int
opt_dflt_privs (t6ifreq_t *t6ifr, const char *arg)
{
	if (*arg == '\0')
	{
		t6ifr->ifsec_dflt.dflt_attrs &= ~T6M_PRIVILEGES;
		return (0);
	}
	t6ifr->ifsec_dflt.dflt_attrs |= T6M_PRIVILEGES;
	t6ifr->ifsec_dflt.dflt_privs = cap_from_text (arg);
	if (t6ifr->ifsec_dflt.dflt_privs == NULL)
	{
		fprintf (stderr, "invalid default privs: %s\n", arg);
		return (1);
	}
	return (0);
}

static int
opt_uid_common (uid_t *uidp, const char *arg, const char *name)
{
	if (isdigit (*arg))
	{
		char *end;
		uid_t uidtmp;

		uidtmp = (uid_t) strtoul (arg, &end, 10); 
		if ((uidtmp == 0 && end == arg) || *end != '\0')
		{
			fprintf (stderr, "invalid %s: %s\n", name, arg);
			return (1);
		}
		*uidp = uidtmp;
	}
	else
	{
		struct passwd *pwd;

		pwd = getpwnam (arg);
		if (pwd == NULL)
		{
			fprintf (stderr, "invalid %s: %s\n", name, arg);
			return (1);
		}
		*uidp = pwd->pw_uid;
	}
	return (0);
}

static int
opt_dflt_audit_id (t6ifreq_t *t6ifr, const char *arg)
{
	if (*arg == '\0')
	{
		t6ifr->ifsec_dflt.dflt_attrs &= ~T6M_AUDIT_ID;
		return (0);
	}
	t6ifr->ifsec_dflt.dflt_attrs |= T6M_AUDIT_ID;
	return (opt_uid_common (&t6ifr->ifsec_dflt.dflt_audit_id, arg,
				"default audit id"));
}

static int
opt_dflt_uid (t6ifreq_t *t6ifr, const char *arg)
{
	if (*arg == '\0')
	{
		t6ifr->ifsec_dflt.dflt_attrs &= ~T6M_UID;
		return (0);
	}
	t6ifr->ifsec_dflt.dflt_attrs |= T6M_UID;
	return (opt_uid_common (&t6ifr->ifsec_dflt.dflt_uid, arg,
				"default user id"));
}

static int
opt_gid_common (gid_t *gidp, const char *arg, const char *name)
{
	if (isdigit (*arg))
	{
		char *end;
		uid_t gidtmp;

		gidtmp = (gid_t) strtoul (arg, &end, 10); 
		if ((gidtmp == 0 && end == arg) || *end != '\0')
		{
			fprintf (stderr, "invalid %s: %s\n", name, arg);
			return (1);
		}
		*gidp = gidtmp;
	}
	else
	{
		struct group *grp;

		grp = getgrnam (arg);
		if (grp == NULL)
		{
			fprintf (stderr, "invalid %s: %s\n", name, arg);
			return (1);
		}
		*gidp = grp->gr_gid;
	}
	return (0);
}

static int
opt_dflt_gid (t6ifreq_t *t6ifr, const char *arg)
{
	if (*arg == '\0')
	{
		t6ifr->ifsec_dflt.dflt_attrs &= ~T6M_GID;
		return (0);
	}
	t6ifr->ifsec_dflt.dflt_attrs |= T6M_GID;
	return (opt_gid_common (&t6ifr->ifsec_dflt.dflt_gid, arg,
				"default group id"));
}

/*
 * Break up a line into words separated by one or more delimiter characters.
 * A character is a delimiter if isdelim returns TRUE. If isdelim is NULL,
 * no character is treated as a delimiter character. Adjacent delimiter
 * characters are treated as a single delimiter character.
 *
 * Words may be grouped into quoted strings. A character is a quote
 * character if isquote returns TRUE. If isquote is NULL, no character
 * is treated as a quote character. Adjacent quoted strings are combined.
 *
 * Returns a list of substrings of `line' terminated by a NULL pointer.
 * When finished with that list, parse_free() it.
 *
 * Returns a NULL pointer when an error is detected.
 */
typedef int (*pl_quote_func) (int);
typedef int (*pl_delim_func) (int);

void
parse_free (char **words)
{
    char **tmp;

    if (words == (char **) NULL)
	return;

    tmp = words;
    while (*tmp != (char *) NULL)
	free (*tmp++);
    free (words);
}

static void
copy_down (char *a, int interval)
{
    char *b = a + interval;

    while (*a++ = *b++)
	/* empty loop */;
}

static char **
parse_line (const char *line, pl_quote_func isquote, pl_delim_func isdelim,
	    int *countp)
{
    unsigned int i, nwords;
    char quotechar, *begin, *end, *copy, **words = NULL;
    enum {S_EATDELIM, S_STRING, S_QUOTE, S_COPY, S_DONE, S_ERROR} state;

    /*
     * check for erroneous input
     */
    if (line == (char *) NULL)
	return (words);

    /*
     * make working copy of input
     */
    copy = malloc (strlen (line) + 1);
    if (copy == (char *) NULL)
	return (words);
    strcpy (copy, line);

    /*
     * allocate and initialize array of possible words
     */
    nwords = (strlen (line) + 1) / 2 + 1;
    words = (char **) malloc (nwords * sizeof (*words));
    if (words == (char **) NULL)
    {
	free (copy);
	return words;
    }
    for (i = 0; i < nwords; i++)
	words[i] = (char *) NULL;
    nwords = 0;

    /*
     * enter the machine
     */
    end = copy;
    state = S_EATDELIM;
    do
    {
	switch (state)
	{
	    case S_EATDELIM:
		while (isdelim && (*isdelim) (*end))
		    end++;
		begin = end;
		state = (*end == '\0' ? S_DONE : S_STRING);
		break;
	    case S_STRING:
		if (isquote && (*isquote) (*end))
		{
		    quotechar = *end;
		    copy_down (end, 1);
		    state = S_QUOTE;
		}
		else if (*end == '\0')
		{
		    state = S_COPY;
		    break;
		}
		else
		{
		    if (isdelim && (*isdelim) (*end))
		    {
			*end++ = '\0';
			state = S_COPY;
		    }
		    else
			end++;
		}
		break;
	    case S_QUOTE:
		while (*end != '\0' && *end != quotechar)
		    end++;
		if (*end == '\0')
		    state = S_ERROR;
		else
		{
		    copy_down (end, 1);
		    state = S_STRING;
		}
		break;
	    case S_COPY:
		words[nwords] = malloc (strlen (begin) + 1);
		if (words[nwords] == (char *) NULL)
		    state = S_ERROR;
		else
		{
		    strcpy(words[nwords++], begin);
		    state = S_EATDELIM;
		}
		break;
	    case S_ERROR:
		parse_free (words);
		words = (char **) NULL;
		state = S_DONE;
		break;
	}
    } while (state != S_DONE);

    if (countp)
	    *countp = nwords;
    free (copy);
    return (words);
}

static int
groupsep (int c)
{
	return (isspace (c) || c == ',');
}

static int
opt_dflt_groups (t6ifreq_t *t6ifr, const char *arg)
{
	t6groups_t *groups = (t6groups_t *) malloc (sizeof (*groups));
	char **grp_names;
	int ngroups;
	__uint32_t i;

	if (groups == NULL)
		return (1);

	if (*arg == '\0')
	{
		t6ifr->ifsec_dflt.dflt_attrs &= ~T6M_GROUPS;
		free (groups);
		return (0);
	}

	/* break into individual groups */
	grp_names = parse_line (arg, (pl_quote_func) NULL, groupsep, &ngroups);
	if (grp_names == NULL || (groups->ngroups = ngroups) > NGROUPS_MAX)
	{
		parse_free (grp_names);
		free (groups);
		return (1);
	}

	t6ifr->ifsec_dflt.dflt_attrs |= T6M_GROUPS;
	t6ifr->ifsec_dflt.dflt_groups = groups;
	for (i = 0; i < groups->ngroups; i++);
	{
		if (opt_gid_common (&groups->groups[i], grp_names[i],
				    "default group list"))
		{
			parse_free (grp_names);
			free (groups);
			return (1);
		}
	}

	parse_free (grp_names);
	return (0);
}

static int
opt_attributes (t6ifreq_t *t6ifr, const char *arg)
{
	static struct attrs {
		char *name;
		t6mask_t mask;
	} attrs[] = {
		{"msen", T6M_SL},
		{"mint", T6M_INTEG_LABEL},
		{"clearance", T6M_CLEARANCE},
		{"acl", T6M_ACL},
		{"privs", T6M_PRIVILEGES},
		{"audit_id", T6M_AUDIT_ID},
		{"uid", T6M_UID},
		{"gid", T6M_GID},
		{"groups", T6M_GROUPS},
		{(char *) NULL, 0},
	};
	char **attr_names;
	int i, count;

	attr_names = parse_line (arg, (pl_quote_func) NULL, groupsep, &count);
	if (attr_names == NULL)
		return (1);

	t6ifr->ifsec_mand_attrs = 0;
	for (i = 0; i < count; i++)
	{
		int j;

		for (j = 0; attrs[j].name != NULL; j++)
			if (strcmp (attrs[j].name, attr_names[i]) == 0)
				break;
		if (attrs[j].name == NULL)
		{
			fprintf (stderr, "unknown attribute %s\n",
				 attr_names[i]);
			parse_free (attr_names);
			return (1);
		}
		t6ifr->ifsec_mand_attrs |= attrs[j].mask;
	}

	parse_free (attr_names);
	return (0);
}

static void
print_status (t6ifreq_t *t6ifr)
{
	char *cp = t6ifr->ifsec_name, if_name[30];
	char buf[256], *bufptr = buf;
	int i;

	/* print interface name */
	if (cp != NULL && *cp != '\0')
	{
		memset ((void *) if_name, '\0', sizeof (if_name));
		strncpy (if_name, cp, sizeof (t6ifr->ifsec_name));
		printf ("Interface:\t%s\n", if_name);
	}

	/* print list of mandatory attributes */
	for (i = T6_SL; i <= T6_GROUPS; i++)
	{
		if (!(t6ifr->ifsec_mand_attrs & t6_mask (i)))
			continue;

		switch (i)
		{
			case T6_SL:
				strcpy (bufptr, "msen,");
				bufptr += strlen (bufptr);
				break;
			case T6_INTEG_LABEL:
				strcpy (bufptr, "mint,");
				bufptr += strlen (bufptr);
				break;
			case T6_CLEARANCE:
				strcpy (bufptr, "clearance,");
				bufptr += strlen (bufptr);
				break;
			case T6_ACL:
				strcpy (bufptr, "acl,");
				bufptr += strlen (bufptr);
				break;
			case T6_PRIVILEGES:
				strcpy (bufptr, "privs,");
				bufptr += strlen (bufptr);
				break;
			case T6_AUDIT_ID:
				strcpy (bufptr, "audit_id,");
				bufptr += strlen (bufptr);
				break;
			case T6_UID:
				strcpy (bufptr, "uid,");
				bufptr += strlen (bufptr);
				break;
			case T6_GID:
				strcpy (bufptr, "gid,");
				bufptr += strlen (bufptr);
				break;
			case T6_GROUPS:
				strcpy (bufptr, "groups,");
				bufptr += strlen (bufptr);
				break;
			default:
				break;
		}
	}

	bufptr = strrchr (buf, ',');
	if (bufptr != NULL)
	{
		*bufptr = '\0';
		printf ("Mandatory Attrs: %s\n", buf);
	}

	/* minimum sensitivity */
	if (t6ifr->ifsec_min_msen != NULL)
	{
		bufptr = msen_to_text (t6ifr->ifsec_min_msen, (size_t *) NULL);
		if (bufptr != NULL)
		{
			printf ("Minimum Sensitivity: %s\n", bufptr);
			msen_free ((msen_t) bufptr);
		}
	}

	/* maximum sensitivity */
	if (t6ifr->ifsec_max_msen != NULL)
	{
		bufptr = msen_to_text (t6ifr->ifsec_max_msen, (size_t *) NULL);
		if (bufptr != NULL)
		{
			printf ("Maximum Sensitivity: %s\n", bufptr);
			msen_free ((msen_t) bufptr);
		}
	}

	/* minimum integrity */
	if (t6ifr->ifsec_min_mint != NULL)
	{
		bufptr = mint_to_text (t6ifr->ifsec_min_mint, (size_t *) NULL);
		if (bufptr != NULL)
		{
			printf ("Minimum Integrity: %s\n", bufptr);
			mint_free ((mint_t) bufptr);
		}
	}

	/* maximum integrity */
	if (t6ifr->ifsec_max_mint != NULL)
	{
		bufptr = mint_to_text (t6ifr->ifsec_max_mint, (size_t *) NULL);
		if (bufptr != NULL)
		{
			printf ("Maximum Integrity: %s\n", bufptr);
			mint_free ((mint_t) bufptr);
		}
	}

	for (i = T6_SL; i <= T6_GROUPS; i++)
	{
		if (!(t6ifr->ifsec_dflt.dflt_attrs & t6_mask (i)))
			continue;

		switch (i)
		{
			case T6_SL:
				bufptr = msen_to_text (t6ifr->ifsec_dflt.dflt_msen,
						       (size_t *) NULL);
				if (bufptr != NULL)
				{
					printf ("Default Sensitivity: %s\n",
						bufptr);
					msen_free ((msen_t) bufptr);
				}
				break;
			case T6_INTEG_LABEL:
				bufptr = mint_to_text (t6ifr->ifsec_dflt.dflt_mint,
						       (size_t *) NULL);
				if (bufptr != NULL)
				{
					printf ("Default Integrity: %s\n",
						bufptr);
					mint_free ((mint_t) bufptr);
				}
				break;
			case T6_CLEARANCE:
				bufptr = msen_to_text (t6ifr->ifsec_dflt.dflt_clearance,
						       (size_t *) NULL);
				if (bufptr != NULL)
				{
					printf ("Default Clearance: %s\n",
						bufptr);
					msen_free ((msen_t) bufptr);
				}
				break;
			case T6_ACL:
				bufptr = acl_to_text (t6ifr->ifsec_dflt.dflt_acl,
						      (ssize_t *) NULL);
				if (bufptr != NULL)
				{
					printf ("Default ACL: %s\n", bufptr);
					acl_free (bufptr);
				}
				break;
			case T6_PRIVILEGES:
				bufptr = cap_to_text (t6ifr->ifsec_dflt.dflt_privs,
						      (size_t *) NULL);
				if (bufptr != NULL)
				{
					printf ("Default Privs: %s\n", bufptr);
					cap_free (bufptr);
				}
				break;
			case T6_AUDIT_ID:
			{
				struct passwd *pwd;

				pwd = getpwuid (t6ifr->ifsec_dflt.dflt_audit_id);
				if (pwd != NULL)
					printf ("Default Audit ID: %s\n",
						pwd->pw_name);
				break;
			}
			case T6_UID:
			{
				struct passwd *pwd;

				pwd = getpwuid (t6ifr->ifsec_dflt.dflt_uid);
				if (pwd != NULL)
					printf ("Default User ID: %s\n",
						pwd->pw_name);
				break;
			}
			case T6_GID:
			{
				struct group *grp;

				grp = getgrgid (t6ifr->ifsec_dflt.dflt_gid);
				if (grp != NULL)
					printf ("Default Group ID: %s\n",
						grp->gr_name);
				break;
			}
			case T6_GROUPS:
			{
				int i;
				t6groups_t *glst = t6ifr->ifsec_dflt.dflt_groups;

				printf ("Default Group List:\n");
				for (i = 0; i < glst->ngroups; i++)
				{
					struct group *grp;

					grp = getgrgid (glst->groups[i]);
					if (grp != NULL)
						printf ("\tGroup %2d: %s\n",
							i + 1, grp->gr_name);
				}
				break;
			}
		}
	}
}

/* Known interface options */
static struct options
{
	char *opt_name;
	char *opt_arg;
	int (*opt_func) (t6ifreq_t *, const char *arg);
} options[] = {
	{"min_msen", "{msen_label}", opt_min_msen},
	{"max_msen", "{msen_label}", opt_max_msen},
	{"min_mint", "{mint_label}", opt_min_mint},
	{"max_mint", "{mint_label}", opt_max_mint},
	{"attributes", "{msen,mint,clearance,acl,privs,audit_id,uid,gid,groups}", opt_attributes},
	{"dflt_msen", "{msen_label}", opt_dflt_msen},
	{"dflt_mint", "{mint_label}", opt_dflt_mint},
	{"dflt_clearance", "{msen_label}", opt_dflt_clearance},
	{"dflt_acl", "{acl_text}", opt_dflt_acl},
	{"dflt_privs", "{cap_text}", opt_dflt_privs},
	{"dflt_audit_id", "{user}", opt_dflt_audit_id},
	{"dflt_uid", "{user}", opt_dflt_uid},
	{"dflt_gid", "{group}", opt_dflt_gid},
	{"dflt_groups", "{group_list}", opt_dflt_groups},
	{NULL, NULL, NULL},
};

static void
usage (void)
{
	int i;

	puts ("iflabel interface [args]");
	for (i = 0; options[i].opt_name != NULL; i++)
		printf ("\t%s=%s\n", options[i].opt_name, options[i].opt_arg);

	exit (EXIT_FAILURE);
}

int
main (int argc, char *argv[])
{
	struct options *optionsp;
	t6ifreq_t t6ifr;
	char *if_name;
	int s;
	struct mac_b_label msen_min, msen_max, msen_dflt, clearance_dflt;
	struct mac_b_label mint_min, mint_max, mint_dflt;
	struct acl acl_dflt;
	struct cap_set cap_dflt;
	t6groups_t groups_dflt;
	cap_t ocap;
	cap_value_t cap_network_mgt[] = {CAP_NETWORK_MGT};

	if (argc < 2)
		usage ();

	if_name = *++argv;
	++argv;

	s = socket (AF_INET, SOCK_DGRAM, 0);
	if (s == -1)
	{
		perror ("iflabel: socket");
		exit (EXIT_FAILURE);
	}

	memset ((void *) &t6ifr, '\0', sizeof (t6ifr));

	/* fill in interface names */
	strncpy (t6ifr.ifsec_name, if_name, sizeof (t6ifr.ifsec_name));

	/* fill in t6ifr fields */
	t6ifr.ifsec_min_msen = &msen_min;
	t6ifr.ifsec_max_msen = &msen_max;
	t6ifr.ifsec_min_mint = &mint_min;
	t6ifr.ifsec_max_mint = &mint_max;

	/* fill in default fields */
	t6ifr.ifsec_dflt.dflt_msen = &msen_dflt;
	t6ifr.ifsec_dflt.dflt_mint = &mint_dflt;
	t6ifr.ifsec_dflt.dflt_clearance = &clearance_dflt;
	t6ifr.ifsec_dflt.dflt_acl = &acl_dflt;
	t6ifr.ifsec_dflt.dflt_privs = &cap_dflt;
	t6ifr.ifsec_dflt.dflt_audit_id = 0;
	t6ifr.ifsec_dflt.dflt_uid = 0;
	t6ifr.ifsec_dflt.dflt_gid = 0;
	t6ifr.ifsec_dflt.dflt_groups = &groups_dflt;

	/* get interface properties */
	ocap = cap_acquire (1, cap_network_mgt);
	if (ioctl (s, SIOCGIFLABEL, (caddr_t) &t6ifr) == -1)
	{
		if (errno != ENOTCONN)
		{
			cap_surrender (ocap);
			perror ("iflabel: ioctl (SIOCGIFLABEL)");
			exit (EXIT_FAILURE);
		}
		if (*argv == NULL)
		{
			cap_surrender (ocap);
			fprintf (stderr,
			"iflabel: interface %s has not yet been labelled\n",
				 if_name);
			exit (EXIT_FAILURE);
		}
	}
	cap_surrender (ocap);

	if (*argv == NULL)
	{
		print_status (&t6ifr);
		exit (0);
	}

	while (*argv != NULL)
	{
		char *c, *p;
		int i = 0;

		/* allocate space for copy of arg */
		c = malloc (strlen (*argv) + 1);
		if (c == NULL)
		{
			perror ("iflabel: malloc");
			exit (EXIT_FAILURE);
		}
		strcpy (c, *argv);

		/* break the option argument at the '=' */
		p = strchr (c, '=');
		if (p == NULL)
		{
			fprintf (stderr, "invalid option: %s\n", c);
			free (c);
			exit (EXIT_FAILURE);
		}
		*p = '\0';

		/* find a matching option */
		for (optionsp = options; optionsp[i].opt_name != NULL;
		     ++optionsp)
		{
			if (strcmp (optionsp[i].opt_name, c) == 0)
				break;
		}

		/* if nothing matched, bomb out */
		if (optionsp->opt_name == NULL)
		{
			free (c);
			usage ();
		}

		/* execute option function */
		if ((*optionsp->opt_func) (&t6ifr, p + 1))
			exit (EXIT_FAILURE);

		argv++;
		free (c);
	}

	/* check sensitivity label range */
	if (t6ifr.ifsec_min_msen != NULL || t6ifr.ifsec_max_msen != NULL)
	{
		if (t6ifr.ifsec_min_msen == NULL)
			t6ifr.ifsec_min_msen = t6ifr.ifsec_max_msen;
		if (t6ifr.ifsec_max_msen == NULL)
			t6ifr.ifsec_max_msen = t6ifr.ifsec_min_msen;
		if (!msen_dom (t6ifr.ifsec_max_msen, t6ifr.ifsec_min_msen))
		{
			fprintf (stderr, "maximum sensitivity doesn't dominate minimum\n");
			exit (EXIT_FAILURE);
		}
	}

	/* ensure the default sensitivity value is in range */
	if (t6ifr.ifsec_dflt.dflt_attrs & T6M_SL)
	{
		if (!msen_dom (t6ifr.ifsec_max_msen, t6ifr.ifsec_dflt.dflt_msen) ||
		    !msen_dom (t6ifr.ifsec_dflt.dflt_msen, t6ifr.ifsec_min_msen))
		{
			fprintf (stderr, "default sensitivity not in sensitivity range\n");
			exit (EXIT_FAILURE);
		}
	}

	/* check integrity label range */
	if (t6ifr.ifsec_min_mint != NULL || t6ifr.ifsec_max_mint != NULL)
	{
		if (t6ifr.ifsec_min_mint == NULL)
			t6ifr.ifsec_min_mint = t6ifr.ifsec_max_mint;
		if (t6ifr.ifsec_max_mint == NULL)
			t6ifr.ifsec_max_mint = t6ifr.ifsec_min_mint;
		if (!mint_dom (t6ifr.ifsec_min_mint, t6ifr.ifsec_max_mint))
		{
			fprintf (stderr, "maximum integrity doesn't dominate minimum\n");
			exit (EXIT_FAILURE);
		}
	}

	/* ensure the default integrity value is in range */
	if (t6ifr.ifsec_dflt.dflt_attrs & T6M_INTEG_LABEL)
	{
		if (!mint_dom (t6ifr.ifsec_dflt.dflt_mint, t6ifr.ifsec_max_mint) ||
		    !mint_dom (t6ifr.ifsec_min_mint, t6ifr.ifsec_dflt.dflt_mint))
		{
			fprintf (stderr, "default integrity not in integrity range\n");
			exit (EXIT_FAILURE);
		}
	}

	/* set interface properties */
	ocap = cap_acquire (1, cap_network_mgt);
	if (ioctl (s, SIOCSIFLABEL, (caddr_t) &t6ifr) == -1)
	{
		cap_surrender (ocap);
		perror ("iflabel: ioctl (SIOCSIFLABEL)");
		exit (EXIT_FAILURE);
	}
	cap_surrender (ocap);

	exit (EXIT_SUCCESS);
}
