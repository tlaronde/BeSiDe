/*	$NetBSD: subr_userconf.c,v 1.27 2018/09/16 23:18:55 mrg Exp $	*/

/*
 * Copyright (c) 2023 Thierry Laronde <tlaronde@kergis.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Copyright (c) 1996 Mats O Jansson <moj@stacken.kth.se>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	OpenBSD: subr_userconf.c,v 1.19 2000/01/08 23:23:37 d Exp
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: subr_userconf.c,v 1.27 2018/09/16 23:18:55 mrg Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/time.h>
#include <sys/userconf.h>

#include <dev/cons.h>

#define UC_BUFSIZE		40
#define UC_MEMSIZE		1024 	/* unit is UC_MEMUNIT */
#define UC_MEMUNIT		(sizeof(void *))
#define UC_MAX_MEM_OBJECTS	128
#define UC_MAX_INDIRECT		16

extern struct cfdata cfdata[];			/* in <kern>/ioconf.c */
extern const char *const userconf_kconf[];	/* in <kern>/uckconf.c */

/*
 * Aliases and memory allocation are tightly related. Explanations are
 * at the end of the file where all the memory stuff is gathered. [TL]
 */
struct alias {
	char *s;	/* allocated or free chunk */
	struct alias *next;
	int size;	/* strlen(.s) or real size of free chunk */
	int flags;	/* possible flags are defined below */
};
#define UC_ALIAS_ALLOCATED	0x01
#define UC_ALIAS_NAME		0x02	/* name of alias */
#define UC_ALIAS_IS_NAME(a)	((a)->flags & UC_ALIAS_NAME)
#define UC_ALIAS_DEF		0x04	/* def of an alias */
#define UC_ALIAS_IS_DEF(a)	((a)->flags & UC_ALIAS_DEF)
#define UC_ALIAS_MACRO		0x08
#define UC_ALIAS_IS_MACRO(a)	((a)->flags & UC_ALIAS_MACRO)
#define UC_ALIAS_PARSING	0x10

static void *userconf_mem[UC_MEMSIZE];
static struct alias userconf_mem_obj[UC_MAX_MEM_OBJECTS];	/* fixed pool */
static struct alias *userconf_mem_free;	/* structs holding mem free info */
static struct alias *userconf_aliases;	/* structs holding aliases info */
#define UC_ALIASES 	userconf_aliases
#define UC_MEM		userconf_mem
#define UC_MEMFREE	userconf_mem_free
#define UC_MEMOBJ	userconf_mem_obj

struct cmd {
	const char *name;
	unsigned char namelen;
	unsigned char min_args;	/* >= 0 */
	signed char max_args;	/* -1 is unlimited */
	char key;
};

/*
 * Commands, keys and args are case insensitive.
 */
static struct cmd userconf_cmds[] = {
	{ "alias",	5, 2,  2, 'A' },
	{ "base",	4, 1,  1, 'B' },
	{ "change",	6, 1, -1, 'C' },	
	{ "disable",	7, 1, -1, 'D' },
	{ "enable",	6, 1, -1, 'E' },
	{ "exit",	4, 0,  0, 'Q' },
	{ "find",	4, 1, -1, 'F' },
	{ "help",	4, 0,  0, 'H' },
	{ "help",	4, 0,  0, '?' },
	{ "kmap",	4, 0, -1, 'K' },
	{ "lines",	5, 1,  1, 'W' },	/* Wrap */
	{ "list",	4, 0, -1, 'L' },
	{ "print",	5, 0, -1, 'P' },
	{ "quit",	4, 0,  0, 'Q' },
	{ "unalias",	7, 1,  1, 'U' },
	{ "vis",	3, 1,  1, 'V' },
	{ "debug0",	6, 0,  0, '0' },
	{ "debug1",	6, 0,  0, '1' },
	{ "debug2",	6, 0,  0, '2' },
	{ NULL,		0, 0,  0, '\0' }
};

struct args {
	const char *list;
	const struct alias *a;	/* NULL if not an alias */
	int len;	/* total length of list */
	int barg;	/* index in list of start of current arg */
	int earg;	/* index in list after current arg */
	int cursor;	/* next not space to parse */
	int flags;	/* see below */
	int val;	/* UC_ARG_NUMBER or unit (pattern) parsed value */
};
#define UC_ARG		(args[nargs-1].list+args[nargs-1].barg)
#define UC_ARGP		(args[*nargs-1].list+args[*nargs-1].barg)
#define UC_ARGLEN	(args[nargs-1].earg-args[nargs-1].barg)
#define UC_ARGLENP	(args[*nargs-1].earg-args[*nargs-1].barg)
#define UC_REMAINS	(args[nargs-1].list+args[nargs-1].cursor)
#define UC_REMAINSP	(args[*nargs-1].list+args[*nargs-1].cursor)
#define UC_REMAINSLEN	(args[nargs-1].len-args[nargs-1].cursor)
#define UC_REMAINSLENP	(args[*nargs-1].len-args[*nargs-1].cursor)
/* ARG types */
#define UC_ARG_NOARG	0
#define UC_ARG_FLAG	0x01
#define UC_ARG_NUMBER	0x02
#define UC_ARG_PATTERN	0x04
#define UC_ARG_STRING	0x08
#define UC_ARG_TYPE	(args[nargs-1].flags & 0x0F)
/* ARG properties */
#define UC_ARG_PATTERN_UNIT			0x10
#define UC_ARG_PATTERN_STARRED			0x20
#define UC_ARG_PATTERN_ANCHORED_BEGIN		0x40
#define UC_ARG_PATTERN_ANCHORED_END		0x80

static int userconf_base = 16;			/* Base for "large" numbers */
static int userconf_maxdev = -1;		/* # of used device slots   */
static int userconf_totdev = -1;		/* # of device slots        */
#if 0
static int userconf_maxlocnames = -1;		/* # of locnames            */
#endif
static int userconf_cnt = -1;			/* Line counter for ...     */
static int userconf_lines = 12;			/* ... # of lines per page  */
static int userconf_histlen = 0;
static int userconf_histcur = 0;
static char userconf_history[1024];
static int userconf_histsz = sizeof(userconf_history);
static char userconf_argbuf[UC_BUFSIZE];		/* Additional input         */
static char userconf_cmdbuf[UC_BUFSIZE];		/* Command line             */
static char userconf_histbuf[UC_BUFSIZE];


static void		userconf_alias(const char *name, int namelen,
				int is_macro, const char *def, int deflen);
static void		userconf_debug0(void);
static void		userconf_debug1(void);
static void		userconf_debug2(void);
static struct alias *	userconf_alias_find(const char *name, int namelen);
static int		userconf_args_init(struct args args[], int *nargs,
				const char *cmdline, bool *noargs);
static int		userconf_args_next(struct args args[], int *nargs);
static void		userconf_args_next_token(struct args args[], int *nargs);
static int		userconf_args_parse_number(const char *c, int len, int *val);
static void		userconf_args_push_alias(struct args args[], int *nargs);
static int		userconf_args_set_pattern(struct args args[], int *nargs);
static void		userconf_free(struct alias *a);
static struct alias *	userconf_malloc(int str_len);
static void		userconf_mem_init(void);
static void		userconf_unalias(const char *name, int namelen);

void
userconf_init(void)
{
	int i;
	struct cfdata *cf;

	i = 0;
	for (cf = cfdata; cf->cf_name; cf++)
		i++;

	userconf_maxdev = i - 1;
	userconf_totdev = i - 1;

	userconf_mem_init();

	/*
	 * Process the config(1) generated userconf instructions first
	 * until quit command or end of array.
	 */
	for (i = 0; userconf_kconf[i] != NULL; i++) {
		if (userconf_parse(userconf_kconf[i]) == -1)
			break;
	}

	userconf_bootinfo();
}

static int
userconf_more(void)
{
	int quit = 0;
	char c = '\0';

	if (userconf_cnt != -1) {
		if (userconf_cnt == userconf_lines) {
			printf("-- more --");
			cnpollc(1);
			c = cngetc();
			cnpollc(0);
			userconf_cnt = 0;
			printf("\r            \r");
		}
		userconf_cnt++;
		if (c == 'q' || c == 'Q')
			quit = 1;
	}
	return quit;
}

static void
userconf_hist_cmd(char cmd)
{
	userconf_histcur = userconf_histlen;
	if (userconf_histcur < userconf_histsz) {
		userconf_history[userconf_histcur] = cmd;
		userconf_histcur++;
	}
}

static void
userconf_hist_int(int val)
{
	snprintf(userconf_histbuf, sizeof(userconf_histbuf), " %d", val);
	if ((userconf_histcur + strlen(userconf_histbuf)) < userconf_histsz) {
		memcpy(&userconf_history[userconf_histcur],
		      userconf_histbuf,
		      strlen(userconf_histbuf));
		userconf_histcur = userconf_histcur + strlen(userconf_histbuf);
	}
}

static void
userconf_hist_eoc(void)
{
	if (userconf_histcur < userconf_histsz) {
		userconf_history[userconf_histcur] = '\n';
		userconf_histcur++;
		userconf_histlen = userconf_histcur;
	}
}

static void
userconf_pnum(int val)
{
	if (val > -2 && val < 16) {
		printf("%d",val);
	} else {
		switch (userconf_base) {
		case 8:
			printf("0%o",val);
			break;
		case 10:
			printf("%d",val);
			break;
		case 16:
		default:
			printf("0x%x",val);
			break;
		}
	}
}

static void
userconf_pdevnam(short dev)
{
	struct cfdata *cd;

	cd = &cfdata[dev];
	printf("%s", cd->cf_name);
	switch (cd->cf_fstate) {
	case FSTATE_NOTFOUND:
	case FSTATE_DNOTFOUND:
		printf("%d", cd->cf_unit);
		break;
	case FSTATE_FOUND:
		printf("*FOUND*");
		break;
	case FSTATE_STAR:
	case FSTATE_DSTAR:
		printf("*");
		break;
	default:
		printf("*UNKNOWN*");
		break;
	}
}

static void
userconf_pdev(short devno)
{
	struct cfdata *cd;
	const struct cfparent *cfp;
	int   *l;
	const struct cfiattrdata *ia;
	const struct cflocdesc *ld;
	int nld, i;

	if (devno > userconf_maxdev) {
		printf("Unknown devno %d (max is %d)\n", (int) devno, userconf_maxdev);
		return;
	}

	cd = &cfdata[devno];

	printf("[%3d] ", devno);
	userconf_pdevnam(devno);
	printf(" at");
	cfp = cd->cf_pspec;
	if (cfp == NULL)
		printf(" root");
	else if (cfp->cfp_parent != NULL && cfp->cfp_unit != -1)
		printf(" %s%d", cfp->cfp_parent, cfp->cfp_unit);
	else
		printf(" %s?", cfp->cfp_parent != NULL ? cfp->cfp_parent
						       : cfp->cfp_iattr);
	switch (cd->cf_fstate) {
	case FSTATE_NOTFOUND:
	case FSTATE_FOUND:
	case FSTATE_STAR:
		break;
	case FSTATE_DNOTFOUND:
	case FSTATE_DSTAR:
		printf(" disable");
		break;
	default:
		printf(" ???");
		break;
	}
	if (cfp) {
		l = cd->cf_loc;
		ia = cfiattr_lookup(cfp->cfp_iattr, 0);
		KASSERT(ia);
		ld = ia->ci_locdesc;
		nld = ia->ci_loclen;
		for (i = 0; i < nld; i++) {
			printf(" %s ", ld[i].cld_name);
			if (!ld[i].cld_defaultstr
			    || (l[i] != ld[i].cld_default))
				userconf_pnum(l[i]);
			else
				printf("?");
		}
	}
	printf("\n");
}

static int
userconf_parse_number(const char *c, int *val)
{
	u_int num = 0;
	int neg = 0;
	int base = 10;

	if (*c == '-') {
		neg = 1;
		c++;
	}

	if (*c == '\0')
		return -1;

	if (*c == '0') {
		base = 8;
		c++;
		if (*c == 'x' || *c == 'X') {
			base = 16;
			c++;
		}
	}
	while (*c != '\n' && *c != '\t' && *c != ' ' && *c != '\0') {
		u_char cc = *c;

		if (cc >= '0' && cc <= '9')
			cc = cc - '0';
		else if (cc >= 'a' && cc <= 'f')
			cc = cc - 'a' + 10;
		else if (cc >= 'A' && cc <= 'F')
			cc = cc - 'A' + 10;
		else
			return -1;

		if (cc >= base)	/* not a valid digit */
			return -1;

		num = num * base + cc;
		c++;
	}

	if (neg && num > INT_MAX)	/* overflow */
		return 1;
	*val = neg ? - num : num;
	return 0;
}

static void
userconf_modify(const struct cflocdesc *item, int *val)
{
	int ok = 0;
	int a;
	char *c;

	while (!ok) {
		printf("%s [", item->cld_name);
		if (item->cld_defaultstr && (*val == item->cld_default))
			printf("?");
		else
			userconf_pnum(*val);
		printf("] ? ");

		cngetsn(userconf_argbuf, sizeof(userconf_argbuf));

		c = userconf_argbuf;
		while (*c == ' ' || *c == '\t' || *c == '\n') c++;

		if (*c != '\0') {
			if (*c == '?') {
				if (item->cld_defaultstr) {
					*val = item->cld_default;
					ok = 1;
				} else
					printf("No default\n");
			} else if (userconf_parse_number(c, &a) == 0) {
				*val = a;
				ok = 1;
			} else {
				printf("Unknown argument\n");
			}
		} else {
			ok = 1;
		}
	}
}

static void
userconf_change(int devno)
{
	struct cfdata *cd;
	char c = '\0';
	int   *l;
	int   ln;
	const struct cfiattrdata *ia;
	const struct cflocdesc *ld;
	int nld;

	if (devno >= 0 && devno <=  userconf_maxdev) {

		userconf_pdev(devno);

		while (c != 'y' && c != 'Y' && c != 'n' && c != 'N') {
			printf("change (y/n) ?");
			cnpollc(1);
			c = cngetc();
			cnpollc(0);
			printf("\n");
		}

		if (c == 'y' || c == 'Y') {

			userconf_hist_cmd('C');
			userconf_hist_int(devno);

			cd = &cfdata[devno];
			l = cd->cf_loc;
			ia = cfiattr_lookup(cd->cf_pspec->cfp_iattr, 0);
			KASSERT(ia);
			ld = ia->ci_locdesc;
			nld = ia->ci_loclen;

			for (ln = 0; ln < nld; ln++)
			{
				userconf_modify(&ld[ln], l);

				userconf_hist_int(*l);

				l++;
			}

			printf("[%3d] ", devno);
			userconf_pdevnam(devno);
			printf(" changed\n");
			userconf_pdev(devno);

			userconf_hist_eoc();

		}
	} else {
		printf("Incorrect devno (valid: [0,%d])\n", userconf_maxdev);
	}
}

static void
userconf_disable(int devno)
{
	int done = 0;

	if (devno >= 0 && devno <= userconf_maxdev) {
		switch (cfdata[devno].cf_fstate) {
		case FSTATE_NOTFOUND:
			cfdata[devno].cf_fstate = FSTATE_DNOTFOUND;
			break;
		case FSTATE_STAR:
			cfdata[devno].cf_fstate = FSTATE_DSTAR;
			break;
		case FSTATE_DNOTFOUND:
		case FSTATE_DSTAR:
			done = 1;
			break;
		default:
			printf("Error unknown state\n");
			break;
		}

		printf("[%3d] ", devno);
		userconf_pdevnam(devno);
		if (done) {
			printf(" already");
		} else {
			userconf_hist_cmd('D');
			userconf_hist_int(devno);
			userconf_hist_eoc();
		}
		printf(" disabled\n");
	} else {
		printf("Incorrect devno (valid: [0,%d])\n", userconf_maxdev);
	}
}

static void
userconf_enable(int devno)
{
	int done = 0;

	if (devno >= 0 && devno <= userconf_maxdev) {
		switch (cfdata[devno].cf_fstate) {
		case FSTATE_DNOTFOUND:
			cfdata[devno].cf_fstate = FSTATE_NOTFOUND;
			break;
		case FSTATE_DSTAR:
			cfdata[devno].cf_fstate = FSTATE_STAR;
			break;
		case FSTATE_NOTFOUND:
		case FSTATE_STAR:
			done = 1;
			break;
		default:
			printf("Error unknown state\n");
			break;
		}

		printf("[%3d] ", devno);
		userconf_pdevnam(devno);
		if (done) {
			printf(" already");
		} else {
			userconf_hist_cmd('E');
			userconf_hist_int(devno);
			userconf_hist_eoc();
		}
		printf(" enabled\n");
	} else {
		printf("Incorrect devno %d (valid: [0,%d])\n",
			devno, userconf_maxdev);
	}
}

static void
userconf_help(void)
{
	int i, pad;
	const struct alias *a;

	userconf_cnt = 0;

	printf("pattern: [*]substr[*][**|num]\n");
	if (userconf_more())
		return;	
	printf("selector: devno | devname[*|num] | -p pattern | -a alias\n");
	if (userconf_more())
		return;
	printf("selectors: selector [selector...]\n\n");
	if (userconf_more())
		return;
	printf("\ndevno: [0,%d]\n", userconf_maxdev);
	if (userconf_more())
		return;
	printf("\nkey|command   args                description\n");
	if (userconf_more())
		return;
	i = 0;
	while (userconf_cmds[i].name != NULL) {
		if (userconf_more())
			break;
		printf("%c|%s", userconf_cmds[i].key, userconf_cmds[i].name);
		pad = userconf_cmds[i].namelen;
		while (pad < 10) {
			printf(" ");
			pad++;
		}
		switch (userconf_cmds[i].key) {
		case '0':
			printf("                    shows the config(1) generated cmds");
			break;
		case '1':
			printf("                    shows mem and structs allocation state");
			break;
		case '2':
			printf("                    shows aliases defined");
			break;                  
		case 'W':
			printf("[count]             number of lines before more");
			break;
		case 'A':
			printf("name selectors      adds selectors to the definition of name\n");
			printf("-c name cmdline	    adds cmdline to the definition of macro name");
			break;
		case 'B':
			printf("8|10|16             base on large numbers");
			break;
		case 'C':
			printf("selectors           change devices");
			break;
		case 'D':
			printf("selectors           disable devices");
			break;
		case 'E':
			printf("selectors           enable devices");
			break;
		case 'F':
			printf("selectors           find devices");
			break;
		case 'H':
		case '?':
			printf("                    this message");
			break;
		case 'L':
			printf("[selectors]         list configuration");
			break;
		case 'P':
			printf("[str|-a name...]    print the arguments");
			break;
		case 'Q':
			printf("                    end current session of userconf");
			break;
		case 'U':
			printf("name [name...]      undefine the alias");
			break;
		case 'V':
			printf("name                print def of alias");
			break;
		default:
			printf("                    don't know");
			break;
		}
		printf("\n");
		i++;
	}

	if (userconf_more())
		return;
	printf("\nAliases:\n");
	for (a = UC_ALIASES; a != NULL; a = a->next) {
		if (userconf_more())
			break;
		if (UC_ALIAS_IS_NAME(a))
			printf("\t%s%s\n", UC_ALIAS_IS_MACRO(a)? "*" : "",
				a->s);
	}
}

static int
userconf_pattern_match(const char *s, const char *pattern, int pattern_len,
	int pattern_flags)
{
	int len;
	int shift, first, last;

	if (pattern_len == 0)	/* anything matches nothing */
		return 1;

	len = strlen(s);
	if ( len < pattern_len
		|| ( (pattern_flags & UC_ARG_PATTERN_ANCHORED_BEGIN)
			&& (pattern_flags & UC_ARG_PATTERN_ANCHORED_END)
			&& len != pattern_len )
		)
		return 0;

	first = (pattern_flags & UC_ARG_PATTERN_ANCHORED_END)?
		len - pattern_len : 0;

	last = (pattern_flags & UC_ARG_PATTERN_ANCHORED_BEGIN)?
		0 : len - pattern_len;

	for (shift = first; shift <= last; ++shift) {
		if (strncasecmp(s + shift, pattern, pattern_len) == 0)
			return 1;
	}

	return 0;
}


/*
* Condition for device name to match according to the way it was
* given:
*  If state == FSTATE_FOUND, look for "dev" (plain devname)
*  If state == FSTATE_STAR, look for "dev*" (STARred device)
*  If state == FSTATE_NOTFOUND, look for "dev#" (specific unit)
*/
#define UC_DEVICE_COND_MATCH					\
(								\
state == FSTATE_FOUND						\
|| ( state == FSTATE_STAR					\
	&& (cfdata[i].cf_fstate == FSTATE_STAR			\
		|| cfdata[i].cf_fstate == FSTATE_DSTAR))	\
|| (state == FSTATE_NOTFOUND					\
	&& cfdata[i].cf_unit == unit				\
	&& (cfdata[i].cf_fstate == FSTATE_NOTFOUND		\
		|| cfdata[i].cf_fstate == FSTATE_DNOTFOUND))	\
)

static void
userconf_devname_dev(const char *dev, int len, char routine)
{
	int i;
	short unit, state;

	unit = 0;
	state = FSTATE_FOUND;

	if (dev[len-1] == '*') {
		state = FSTATE_STAR;
		--len;
	} else {
		for (i = len - 1; dev[i] >= '0' && dev[i] <= '9'; i--)
			;
		if (i < (len - 1)) {	/* unit numbers */
			state = FSTATE_NOTFOUND;
			for (i++; i < len; i++)
				unit = unit*10 + dev[i] - '0';
		}
	}

	switch (routine) {
	case 'C':
		break;
	default:
		userconf_cnt = 0;
		break;
	}

	i = 0;
	while (cfdata[i].cf_name != NULL) {
		if (strlen(cfdata[i].cf_name) == len) {
			if (strncasecmp(dev, cfdata[i].cf_name, len) == 0
				 && UC_DEVICE_COND_MATCH) {
				if (userconf_more())
					break;
				switch (routine) {
				case 'C':
					userconf_change(i);
					break;
				case 'D':
					userconf_disable(i);
					break;
				case 'E':
					userconf_enable(i);
					break;
				case 'F':
					userconf_pdev(i);
					break;
				default:
					printf("Unknown routine /%c/\n",
					    routine);
					break;
				}
			}
		}
		i++;
	}

	switch (routine) {
	case 'C':
		break;
	default:
		userconf_cnt = -1;
		break;
	}
}

static void
userconf_number_dev(int devno, char routine)
{
	switch (routine) {
	case 'C':
		userconf_change(devno);
		break;
	case 'D':
		userconf_disable(devno);
		break;
	case 'E':
		userconf_enable(devno);
		break;
	case 'F':
	case 'L':
		userconf_pdev(devno);
		break;
	default:
		printf("Unknown routine /%c/\n", routine);
		break;
	}
}

static void
userconf_pattern_dev(const char *pattern, int plen, int pflags, short unit,
	char routine)
{
	int i;
	short state;

	state = FSTATE_FOUND;
	if (pflags & UC_ARG_PATTERN_STARRED)
		state = FSTATE_STAR;
	else if (unit)
		state = FSTATE_NOTFOUND;

	switch (routine) {
	case 'C':
		break;
	default:
		userconf_cnt = 0;
		break;
	}

	i = 0;
	while (cfdata[i].cf_name != NULL) {
		if ( userconf_pattern_match(cfdata[i].cf_name,
			pattern, plen, pflags) && UC_DEVICE_COND_MATCH ) {
			if (userconf_more())
				break;
			switch (routine) {
			case 'C':
				userconf_change(i);
				break;
			case 'D':
				userconf_disable(i);
				break;
			case 'E':
				userconf_enable(i);
				break;
			case 'F':
			case 'L':
				userconf_pdev(i);
				break;
			default:
				printf("Unknown routine /%c/\n",
				    routine);
				break;
			}
		}
		i++;
	}

	switch (routine) {
	case 'C':
		break;
	default:
		userconf_cnt = -1;
		break;
	}
}

/*
 * [TL]
 * This is the most complicated part of the whole process, because
 * we are dealing with read-only strings, because of recursion of
 * macros (not built-ins) and interpolation of definitions.
 *
 * There is only one command executed at any time. But this command
 * can take multiple arguments and some arguments may be aliases 
 * whose definition is to be interpolated instead (and, in turn, this
 * definition can have "-a name" components leading to interpolation
 * of other * definitions and so on...).
 *
 * Each command line is treated in a single execution of config_parse(), 
 * so the arguments have to be in the frame stack for this instance
 * i.e. local.
 *
 * A line of arguments is processed one argument at a time. This starts
 * by tokenization based on blanks and new line separators: .barg is
 * the index of the beginning of the token; .earg is the index after
 * the token, and .cursor is on the next non blank character from which
 * to resume parsing.
 */
static void
userconf_args_next_token(struct args args[], int *nargs)
{
	const char *c;

	if (*nargs == 0)
		return;

	c = args[*nargs-1].list + args[*nargs-1].cursor;
	args[*nargs-1].barg = args[*nargs-1].cursor;
	while (*c != ' ' && *c != '\t' && *c != '\n' && *c != '\0') {
		c++;
		args[*nargs-1].cursor++;
	}
	args[*nargs-1].earg = args[*nargs-1].cursor;
	while (*c == ' ' || *c == '\t' || *c == '\n') {
		c++;
		args[*nargs-1].cursor++; /* next token or EOL */
	}
}

/* 
 * OK: above gives us the next token. But we do not deal with tokens:
 * if the token parsed is "-a", the next token is processed and
 * considered as an alias whose definition has to be interpolated.
 * This is done by "pushing" the defs of the alias (that can not be
 * a macro), in order, one line at a time in the args array; the cursor
 * on the current depth holds the position to resume parsing; the
 * pushed line has to be parsed starting from the beginning.
 *
 * But we have to initiate the process: the first line pushed is the
 * cmdline (from whatever source: we don't know it is just the
 * argument of userconf_parse()). We use the same facilities to
 * isolate the command: the very first token. But the command is not
 * an argument and is not handled by the  args_next() routine (a
 * macro is not called with a leading "-a" flag but just by its name).
 *
 */
static int
userconf_args_init(struct args args[], int *nargs, const char *cmdline,
	bool *noargs)
{
	if (UC_MAX_INDIRECT <= 0) {
		printf("Nothing can be executed.\n");
		return -1;
	}

	args[0].list = cmdline;
	args[0].a = NULL;
	args[0].flags = 0;
	args[0].val = 0;
	/* skip leading blanks */
	for (args[0].cursor = 0; *cmdline == ' ' || *cmdline == '\t'
		|| *cmdline == '\n'; args[0].cursor++, cmdline++)
		;	/* skip initial blanks and newline */
	for(; *cmdline != '\0'; cmdline++)
		;	/* go to end for length */
	args[0].len = cmdline - args[0].list;
	*nargs = 1;
	/*
	 * We use the facilities but next is the cmd, not an arg.
	 */
	userconf_args_next_token(args, nargs);

	if (args[0].barg == args[0].earg) /* empty cmdline */
		*nargs = 0;

	if (UC_REMAINSLENP == 0)
		*noargs = true;
	else
		*noargs = false;
		
	return 0;
}

/*
 * Except for initialization (that is done with the command line),
 * pushing is always pushing an alias. So the command line is always
 * at index 0, and upper levels are always aliases' definitions.
 * So here, we have parsed an alias (the name) so we eventually push
 * the first line of its definition on the args "stack" but, of
 * course, only if the alias is defined; if it is not defined, it is
 * ignored and we will continue parsing remaining arguments.
 *
 * Security: we could just let the maximum depth of indirections be
 * reached. But we can verify that the alias struct we add is not
 * already present. If it is (even if it's only a def line and not
 * the name struct of the alias), there is a loop. In this case, we
 * invalidate args.
 *
 * For an alias to not be defined is not fatal. For an alias of the
 * wrong type (a macro) to be called as an argument is an error.
 * [TL]
 */
static void
userconf_args_push_alias(struct args args[], int *nargs)
{
	const struct alias *a;
	int i;

	if (*nargs == UC_MAX_INDIRECT - 1) {
		printf("Maximum depth of indirection reached\n");
		*nargs = 0;
		return;
	}

	if ( (a = userconf_alias_find(UC_ARGP, UC_ARGLENP)) != NULL ) {
		if (UC_ALIAS_IS_MACRO(a)) {
			printf("An executable can't be an argument\n");
			*nargs = 0;
			return;
		}
		for (i = 0; i < *nargs; i++) {
			if (args[i].a == a) {	/* loop: aborting */
				printf("An alias is looping\n");
				*nargs = 0;
				return;
			}
		}
		/* There must be a definition next. */
		args[*nargs].a = a->next;
		args[*nargs].list = a->next->s;
		args[*nargs].len = a->next->size;
		args[*nargs].flags = 0;
		args[*nargs].val = 0;
		/* The definition doesn't start by blanks. */
		args[*nargs].cursor = 0;
		(*nargs)++;
	}
}

/*
 * When an argument looks like a possible number, it has to be parsed.
 * This is a special routine, adapted from the other one (for modify
 * interaction) since here we know where the argument stops (there are no
 * blanks or newline).
 */
static int
userconf_args_parse_number(const char *c, int len, int *val)
{
	u_int num = 0;
	int i;
	int neg = 0;
	int base = 10;

	i = 0;
	if (c[i] == '-') {
		neg = 1;
		i++;
	}

	if (i == len)	/* works for len == 0 and string is "-" */
		return -1;

	if (i < len && c[i] == '0') {
		base = 8;
		i++;
		if ( i < len && (c[i] == 'x' || c[i] == 'X') ) {
			base = 16;
			i++;
		}
	}
	for (; i < len; i++) {
		u_char cc = c[i];

		if (cc >= '0' && cc <= '9')
			cc = cc - '0';
		else if (cc >= 'a' && cc <= 'f')
			cc = cc - 'a' + 10;
		else if (cc >= 'A' && cc <= 'F')
			cc = cc - 'A' + 10;
		else
			return -1;

		if (cc >= base)	/* not a valid digit */
			return -1;

		num = num * base + cc;
	}

	if (neg && num > INT_MAX)	/* overflow */
		return 1;
	*val = neg ? - num : num;

	return 0;
}

/*
 * Finally, we are getting to the work horse.
 *
 * When userconf_args_next() is called, parsing resume at .cursor.
 * If we encounter an alias, its first def is pushed and we will
 * continue parsing it. If it is not, it will be of one of the four
 * primitive types (userconf_parse(), except for the command itself
 * of the line, never sees an alias: this routine silently substitute
 * the definition for the alias): flag, number, pattern, string. Not
 * every flag can be returned since the "-a" flag is absorbed by
 * treating the next token as an alias; the "-p" is passed qualifying
 * the next token as a pattern (but no information is lost: the type
 * set in the flags as well as the pattern flags raised allow
 * for example the "print" built-in to rebuild the original string).
 *
 * For number, parsing is attempted and the result, if valid, is in
 * .val; for a flag, if it is not absorbed ("-a", "-p") it is passed
 * including the leading hyphen; for a pattern, the fixed
 * substring is isolated as the token and flags are raised; the string
 * alone is returned as is.
 */
static int
userconf_args_next(struct args args[], int *nargs)
{
	while (*nargs) {
		args[*nargs-1].flags = 0;
		userconf_args_next_token(args, nargs);

		/*
		 * If hitting end of this list...
		 */
		if (UC_ARGLENP == 0) {
			if (args[*nargs-1].a != NULL) { /* alias */
				if (args[*nargs-1].a->next != NULL
					&& UC_ALIAS_IS_DEF(args[*nargs-1].a->next)) { /* more */
					args[*nargs-1].a = args[*nargs-1].a->next;
					args[*nargs-1].list = args[*nargs-1].a->s;
					args[*nargs-1].len = args[*nargs-1].a->size;
					args[*nargs-1].cursor = 0;
				} else
					(*nargs)--; /* end of alias def */
				continue;
			} else {
				(*nargs)--;	/* EOL */
				continue;
			}
		}
		if (UC_ARGP[0] == '-' && UC_ARGLENP > 1) {
			if (userconf_args_parse_number(UC_ARGP, UC_ARGLENP,
				&args[*nargs-1].val) == 0) {
				printf("number\n");
				args[*nargs-1].flags = UC_ARG_NUMBER;
				return 1;
			} else if (UC_ARGLENP == 2) { /* a flag */
				if (UC_ARGP[1] == 'a' || UC_ARGP[1] == 'A') {
					userconf_args_next_token(args, nargs);
					if (UC_ARGLENP == 0) {
						printf("Syntax error for alias in arguments list\n");
						*nargs = 0; /* the end */
						return 0;
					}
					userconf_args_push_alias(args, nargs);
					continue;
				} else if (UC_ARGP[1] == 'p'
					|| UC_ARGP[1] == 'P') {
					userconf_args_next_token(args, nargs);
					if ( UC_ARGLENP == 0
						|| userconf_args_set_pattern(args, nargs) == -1 ) {
						printf("Syntax error for pattern in arguments list\n");
						*nargs = 0; /* the end */
						return 0;
					} else
						return 1;
				} else {
					args[*nargs-1].flags = UC_ARG_FLAG;
					return 1;
				}
			}
		} else if ( UC_ARGP[0] >= '0' && UC_ARGP[0] <= '9'
			&& userconf_args_parse_number(UC_ARGP, UC_ARGLENP,
				&args[*nargs-1].val) == 0 ) {
			args[*nargs-1].flags = UC_ARG_NUMBER;
			return 1;
		} else {
			args[*nargs-1].flags = UC_ARG_STRING;
			return 1;
		}
	}

	return 0;
}

/*
 * A pattern is a perhaps anchored substring.
 * A leading `*' means the substring can start anywhere; without it,
 * the substring must start the string matched against.
 * A trailing '*' means that the substring can appear not terminating
 * the string matched. If not, the substr must be a suffix of the
 * string matched against.
 * A trailing double star `**' means the pattern will match a
 * STARred device. So a pattern can have 0, 1, 2 or 3 trailing stars.
 * A single star `*' matches anything.
 *
 * Assumption: We are not called with an empty pattern: it is a syntax
 * error caught in userconf_args_next() stopping interpretation of the
 * whole command line. [TL]
 */
static int
userconf_args_set_pattern(struct args args[], int *nargs)
{
	const char *c;
	int i;

	args[*nargs-1].flags = UC_ARG_PATTERN;

	/*
	 * Start by the end to catch the optional trailing unit number.
	 * Assertion: UC_ARGLEN >= 1
	 */
	args[*nargs-1].val = 0;
	c = args[*nargs-1].list + args[*nargs-1].earg - 1;
	for (i = 0; i < UC_ARGLENP && *c >= '0' && *c <= '9'; i++, c--)
		;
	if (i) {	/* unit to parse and truncate */
		args[*nargs-1].flags |= UC_ARG_PATTERN_UNIT;
		args[*nargs-1].earg -= i;
		for (c++; i; c++, i--)
			args[*nargs-1].val = args[*nargs-1].val * 10
				+ *c - '0';
	}

	/*
	 * Now look for stars.
	 */
	c = args[*nargs-1].list + args[*nargs-1].earg - 1;
	for (i = 0; i < UC_ARGLENP && i <= 3 && *c == '*'; i++, c--)
		;

	if (i == UC_ARGLENP) {	/* The '*' all pattern */
		if (i >= 2) {
			args[*nargs-1].flags |= UC_ARG_PATTERN_STARRED;
		}
		args[*nargs-1].barg = args[*nargs-1].earg;
		if ( (args[*nargs-1].flags & UC_ARG_PATTERN_UNIT)
			&& (args[*nargs-1].flags & UC_ARG_PATTERN_STARRED) ) {
			printf("Pattern can't be both STARred and with unit\n");
			return -1;
		} else
			return 0;
	}

	/* Not the All pattern */
	if (i != 1 && i != 3)
		args[*nargs-1].flags |= UC_ARG_PATTERN_ANCHORED_END;
	if (i == 2 || i == 3)
		args[*nargs-1].flags |= UC_ARG_PATTERN_STARRED;

	/* truncate this suffix */
	args[*nargs-1].earg -= i;

	/* prefix */
	if (UC_ARGP[0] == '*')
		args[*nargs-1].barg++;
	else
		args[*nargs-1].flags |= UC_ARG_PATTERN_ANCHORED_BEGIN;

	if ( (args[*nargs-1].flags & UC_ARG_PATTERN_UNIT)
		&& (args[*nargs-1].flags & UC_ARG_PATTERN_STARRED) ) {
		printf("Pattern can't be both STARred and with unit\n");
		return -1;
	} else
		return 0;
}

/*
 * There is only one cmd at any time. So per cmd, we recurse. But
 * since we can recurse, the treatment of the series of arguments
 * has to be local to the function.
 *
 * There can be several arguments and an argument can be an alias.
 * Whenever we encounter an alias we register where is the next token
 * on the current list and we "push" the alias definition on
 * the stack args (of limited depth).
 *
 * But for macro, we raise the UC_ALIAS_PARSING flag in order to 
 * detect loops (a macro called while already executing).
 *
 * Returns:
 *	 0	Normal processing, even if error
 *	-1	Quitting
 */
#define UC_NEXT_ARG	userconf_args_next(args, &nargs)
int
userconf_parse(const char *cmdline)
{
	int icmd;	
	int i, j;		/* all purposes local junk counters */
	int nargs;		/* number of args lists pushed */
	bool noargs;
	struct args args[UC_MAX_INDIRECT];

	if (userconf_args_init(args, &nargs, cmdline, &noargs) == -1)
		return -1;	/* unable to proceed: quit */

	if (nargs == 0)
		return 0;	/* empty lines allowed */

	icmd = 0;
	while (userconf_cmds[icmd].name != NULL) {
		if (UC_ARGLEN == 1) {
			if (userconf_cmds[icmd].key == *UC_ARG
				|| ( userconf_cmds[icmd].key >= 'A'
					&& userconf_cmds[icmd].key <= 'Z'
					&& (userconf_cmds[icmd].key+'a'-'A') == *UC_ARG )
			)
				break;
		} else if (userconf_cmds[icmd].namelen == UC_ARGLEN) {
			if (strncasecmp(UC_ARG, userconf_cmds[icmd].name, UC_ARGLEN) == 0) 
				break;
		}
		icmd++;
	}

	if (userconf_cmds[icmd].name == NULL) {
		/* Trying macros */
		struct alias *a, *n;

		if ( (a = userconf_alias_find(UC_ARG, UC_ARGLEN)) != NULL ) {
			if (!noargs) {
				printf("A macro doesn't take parameters.\n");
				return 0;
			}
			if (!UC_ALIAS_IS_MACRO(a)) {
				printf("Alias is not executable\n");
				return 0;
			}
			if (a->flags & UC_ALIAS_PARSING) {
				printf("Macro loop detected; aborting\n");
				return 0;
			}
			a->flags |= UC_ALIAS_PARSING;
			for (n = a->next; n != NULL && UC_ALIAS_IS_DEF(n);
				n = n->next) {
				if (userconf_parse(n->s) == -1) {
					a->flags &= ~UC_ALIAS_PARSING;
					return -1;
				}
			}
			a->flags &= ~UC_ALIAS_PARSING;
			return 0;
		} else {
			printf("Unknown command, try help\n");
			return 0;
		}
	}

	if (noargs && userconf_cmds[icmd].min_args != 0) {
		printf("Built-in: %s ('%c') takes args; try help\n",
			userconf_cmds[icmd].name, userconf_cmds[icmd].key);
		return 0;
	}

	/*
	 * Built-ins only. Treating arguments ignoring surnumerous
	 * ones but stopping on syntax error.
	 */
	switch (userconf_cmds[icmd].key) {
	case '0':
		userconf_debug0();	/* uckconf */
		break;
	case '1':
		userconf_debug1();	/* mem */
		break;
	case '2':
		userconf_debug2();	/* aliases */
		break;
	case 'W':
		if (UC_NEXT_ARG && UC_ARG_TYPE == UC_ARG_NUMBER)
			userconf_lines = args[nargs-1].val;
		else
			printf("Unknown argument\n");
		break;
	case 'A':
		{
		int is_macro;

		(void)UC_NEXT_ARG;
		is_macro = 0;
		if (UC_ARG_TYPE == UC_ARG_FLAG
			&& (UC_ARG[1] == 'c' || UC_ARG[1] == 'C') ) {
			is_macro = 1;
			if (!UC_NEXT_ARG) {
				printf("Missing argument\n");
				break;
			}
		}
		if (UC_ARG_TYPE != UC_ARG_STRING) {
			printf("Incorrect alias name\n");
			break;
		}
		if (UC_REMAINSLEN == 0) {
			printf("An alias has to have a definition\n");
			break;
		}
		userconf_alias(UC_ARG, UC_ARGLEN, is_macro, UC_REMAINS, UC_REMAINSLEN);
		}
		break;
	case 'B':
		if (UC_NEXT_ARG && UC_ARG_TYPE == UC_ARG_NUMBER
			&& ( args[nargs-1].val == 8
				|| args[nargs-1].val == 10
				|| args[nargs-1].val == 16 ) )
			userconf_base = args[nargs-1].val;
		else
			printf("8|10|16 expected\n");
		break;
	case 'C':
	case 'D':
	case 'E':
	case 'F':
	case 'L':
		if (noargs) {
			/*
			 * Assertion: only L allows no args; other
			 * cases caught above.	
			 */
			userconf_pattern_dev("", 0, 0, 0, userconf_cmds[icmd].key);
			break;
		}
		while (UC_NEXT_ARG) {
			switch (UC_ARG_TYPE) {
			case UC_ARG_NUMBER:
				userconf_number_dev(args[nargs-1].val,
					userconf_cmds[icmd].key);
				break;
			case UC_ARG_PATTERN:
				userconf_pattern_dev(UC_ARG, UC_ARGLEN,
					args[nargs-1].flags,
					(short) args[nargs-1].val,
					userconf_cmds[icmd].key);
				break;
			case UC_ARG_STRING:
				userconf_devname_dev(UC_ARG, UC_ARGLEN,
					userconf_cmds[icmd].key);
				break;
			default:
				printf("Unknown argument\n");
				break;
			}
		}
		break;
	case 'H':
	case '?':
		/* Even if syntax errors, user needs help.  */
		userconf_help();
		break;
	case 'K':
		if (noargs) {
			cncmapreset();
			break;
		}
		while (UC_NEXT_ARG) {
			const char *c;
			char buf[17];

			if (UC_ARGLEN % 2) {	/* odd length */
				printf("Incorrect mapping string\n");
				continue;
			}
			/*
			 * Strings are read-only. Pass them by 16 max
			 * chunks. A nul length argument doesn't exist.
			 */
			for (i = j = 0, c = UC_ARG; i < UC_ARGLEN; ++i) {
				if (j == 16) {
					buf[j] = '\0';
					cncmap(buf);
					j = 0;
				}
				buf[j++] = c[i];
			}
			buf[j] = '\0';
			cncmap(buf);
		}
		break;	
	case 'P':
		{
		const struct alias *a;
		int firstcmd, notfirstarg;

		a = NULL;
		firstcmd = 1;
		notfirstarg = 0;
		while (UC_NEXT_ARG) {
			/* A macro def is a cmdline; insert a '\n' */
			if (args[nargs-1].a != a) {
				if (UC_ALIAS_IS_MACRO(args[nargs-1].a)
					&& !firstcmd)
					printf("\n");
				firstcmd = 0;
				notfirstarg = 0;
				a = args[nargs-1].a;
			}
			if (UC_ARG_TYPE == UC_ARG_PATTERN) {
				printf("%s-p ", notfirstarg++? " " : "");
				printf("%s", (args[nargs-1].flags
					 & UC_ARG_PATTERN_ANCHORED_BEGIN)?	"*" : "");
				if (UC_ARGLEN)
					printf("%.*s", UC_ARGLEN, UC_ARG);
				else
					printf("*");
				printf("%s", (args[nargs-1].flags
					 & UC_ARG_PATTERN_ANCHORED_END)?	"*" : "");
				printf("%s", (args[nargs-1].flags
					 & UC_ARG_PATTERN_STARRED)? "**" : "");
			} else
				printf("%s%.*s", notfirstarg++? " " : "",
					UC_ARGLEN, UC_ARG);
		}
		printf("\n");
		}
		break;
	case 'Q':	
		/* Imperative; ignore everything else. */
		userconf_hist_cmd('Q');
		userconf_hist_eoc();
		return -1;
	case 'U':
		while (UC_NEXT_ARG)
			userconf_unalias(UC_ARG, UC_ARGLEN);
		break;
	case 'V':
		(void) UC_NEXT_ARG;
		if (UC_ARG_TYPE != UC_ARG_STRING)
			printf("Invalid argument; try help\n");
		else {
			const struct alias *a;

			if ( (a = userconf_alias_find(UC_ARG, UC_ARGLEN))
				!= NULL ) {
				printf("%s%s:\n",
					UC_ALIAS_IS_MACRO(a)? "*" : "",
					a->s);
				for (a = a->next; a != NULL
					&& UC_ALIAS_IS_DEF(a);
					a = a->next)
					printf("%s\n", a->s);
			} else
				printf("Unknown alias\n");
		}
		break;
	default:
			printf("Unknown command\n");
			break;
	}
	return 0;
}

void
userconf_prompt(void)
{
	const char prompt[] = "uc> ";

	printf("userconf: configure system autoconfiguration:\n");

	while (1) {
		printf(prompt);
		if ( cngetsn(userconf_cmdbuf, sizeof(userconf_cmdbuf)) > 0 &&
		    userconf_parse(userconf_cmdbuf) ==  -1 )
			break;
	}
	printf("Continuing...\n");
}

/*
 * MEMORY HANDLING
 *
 * There is a fixed chunk of scratch data to allocate.
 * Only aliases are allocated and they require at least 2 mem objects:
 * one for the name; one for the first definition. Every additional
 * definition is added by allocating and linking another UC_ALIAS_DEF
 * object.
 * Freeing is deallocating, starting by the end, all the memory and
 * structures that are the definition and the name of the alias.
 *
 * userconf_mem_obj[] is a fixed size array of structures, used both for
 * aliases and memory allocation management.
 *
 * userconf_aliases is a linked list of allocated structures in
 * userconf_mem_obj[] with .s pointing to allocated place in ucmem[].
 *
 * userconf_mem_free is a linked list of structures recording free
 * memory chunks in userconf_mem[]. The list is ordered according to 
 * increasing free memory addresses. .size is then defined with the
 * size of the free chunk.
 *
 * Initially, userconf_aliases is NULL (nothing allocated); userconf_mem_free has
 * one uniq structure defined with the address of the mem chunk and its
 * size; and all the other structs are unused with userconf_next_free_alias
 * pointing to the first unused one.
 */
static void
userconf_mem_init(void)
{
	int i;

	UC_ALIASES = NULL;

	UC_MEMOBJ[0].s = (char *) UC_MEM;
	UC_MEMOBJ[0].size = UC_MEMSIZE;
	UC_MEMOBJ[0].next = NULL;
	UC_MEMOBJ[0].flags = UC_ALIAS_ALLOCATED;

	UC_MEMFREE = UC_MEMOBJ;

	for (i = 1; i < UC_MAX_MEM_OBJECTS; i++) {
		UC_MEMOBJ[i].s = NULL;
		UC_MEMOBJ[i].size = 0;
		UC_MEMOBJ[i].next = NULL;
		UC_MEMOBJ[i].flags = 0;
	}
}

/*
 * We always allocate a struct with the asked for memory rounded up
 * to the nearest multiple of sizeof(void *).
 * This is simple: we walk down the list of UC_MEMFREE to find a
 * chunk with at least the requested real size chunk.
 *
 * Optimization: If what was found is exactly the requested size, this 
 * struct is extracted from the list and returned.
 *
 * If it is not, one available is searched in UC_MEMOBJ. If none at
 * disposal, error. If OK, it is defined with the str_len passed, and
 * the real allocation is substracted from the free mem chunk.
 */ 
#define UC_REALSIZE(str_len)					\
	( ((str_len) + 1) / UC_MEMUNIT			\
	+ ((((str_len) + 1) % UC_MEMUNIT)? 1 : 0) )

static struct alias *
userconf_malloc(int str_len)
{
	struct alias *a, *p, *m;
	int real_size;

	real_size = UC_REALSIZE(str_len);

	for (p = NULL, m = UC_MEMFREE;
		 m != NULL && m->size < real_size; p = m, m = m->next)
		;

	if (m == NULL)	/* not enough space left */
		return NULL;

	if (m->size == real_size) {
		/* Extract from mem free list. */
		if (p == NULL)
			UC_MEMFREE = m->next;
		else
			p->next = m->next;

		/* UC_ALIAS_ALLOCATED is already set */
		m->size = str_len;

		return m;
	} else {
		int i;

		for (i = 0; i < UC_MAX_MEM_OBJECTS &&
			(UC_MEMOBJ[i].flags & UC_ALIAS_ALLOCATED);
			i++)
			;
		if (i == UC_MAX_MEM_OBJECTS)	/* no struct left */
			return NULL;

		a = &UC_MEMOBJ[i];
		a->s = m->s;
		a->flags = UC_ALIAS_ALLOCATED;
		a->size = str_len;
		m->s += real_size * UC_MEMUNIT;
		m->size -= real_size;

		return a;
	}
}

/*
 * Freeing a struct is returning the memory to the free pool and
 * perhaps marking the struct as available (not allocated). But it
 * depends.
 *
 * Optimization: If the freed memory is contiguous to a free chunk, the
 * freed memory is added to the chunk, and the struct can be freed.
 * Since a gap can have been totally filled, we look if we can
 * coalesce the next chunk too (in this case, we gain one more 
 * available struct.
 *
 * If it is not, the struct stays allocated but is inserted in the
 * list of free mem chunks, in its place according to the address
 * (so that coalescing can work). 
 *
 * In all cases, the real_size is redefined.
 */
static void
userconf_free(struct alias *a)
{
	struct alias *p, *m;

	a->size = UC_REALSIZE(a->size);
	a->next = NULL;

	for (p = NULL, m = UC_MEMFREE;
		 m != NULL && (m->s + m->size * UC_MEMUNIT) < a->s; p = m, m = m->next)
		;

	if (m != NULL && (m->s + m->size * UC_MEMUNIT) == a->s) {	/* coalesce */
		m->size += a->size;
		a->flags = 0;
		a->s = NULL;
		a->size = 0;
	} else {	/* insert */
		a->flags = UC_ALIAS_ALLOCATED;
		a->next = m;
		if (p == NULL) 
			UC_MEMFREE = a;
		else
			p->next = a;
		m = a;	/* for tail coalescing */
	}

	/*
	 * Now m is pointing to the chunk modified. We have coalesce
	 * before, look if we can coalesce after.
	 */
	if ( m->next != NULL
		&& m->next->s == (m->s + m->size * UC_MEMUNIT) ) {
		a = m->next;
		m->size += a->size;
		m->next = a->next;
		a->flags = 0;
		a->s = NULL;
		a->size = 0;
		a->next = NULL;
	}
}

/*
 * An alias is searched in a UC_ALIAS_NAME struct in a case
 * insensitive way.
 */
static struct alias *
userconf_alias_find(const char *name, int namelen)
{
	struct alias *a;

	for (a = UC_ALIASES; a != NULL; a = a->next) {
		if ( (a->flags & UC_ALIAS_NAME)
			&& (a->size == namelen)
			&& (strncasecmp(a->s, name, namelen) == 0) )
				break;
	}

	return a;
}

/*
 * Giving a definition to an alias is whether creating a new alias with
 * a first definition, or adding a supplementary definition to an
 * existing alias (that must be of the same type).
 *
 * A name is alphanumeric, starting by a letter and has to be at least
 * two chars long since single letters are reserved for built-ins.
 * A macro with the same name as a built-in will never run. We catch
 * it here and return error; but we could have ignored it, just wasting
 * memory.
 * 
 * If the alias does not exist, we will need two userconf_mem_obj: one
 * for the name, the other for the definition; if it exists, one
 * supplementary struct only is necessary.
 *
 * A created alias is put first in the aliases list.
 *
 * A message is printed but since the parsing doesn't care (for return)
 * about a failure), nothing is returned. [TL]
 */
static void
userconf_alias(const char *name, int namelen, int is_macro,
	const char *def, int deflen)
{
	int i;
	struct alias *a, *aname, *adef;
	char c;

	c = name[0];
	if (namelen < 2 || deflen == 0
		|| !((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) ) {
		printf("An alias must be at least 2 chars starting by letter");
		return;
	}
	for (i = 1; i < namelen; i++) {
		c = name[i];
		if ( c < '0' || (c > '9' && c < 'A')
			|| (c > 'Z' && c < 'a') || c > 'z' ) {
			printf("An alias must be alphanumeric");
			return;  
		}
	}

	aname = userconf_alias_find(name, namelen);

	if (aname != NULL && ( (is_macro && !UC_ALIAS_IS_MACRO(aname))
		|| (!is_macro && UC_ALIAS_IS_MACRO(aname)) ) ) {
		printf("Def is incompatible with existing alias type");
		return;
	}

	/*
	 * Start by allocating def, always needed. Simplifies
	 * list management.
	 */
	if ( (adef = userconf_malloc(deflen)) == NULL )
		return;

	/* Defined but not linked yet. */
	adef->flags |= UC_ALIAS_DEF;
	if (is_macro)
		adef->flags |= UC_ALIAS_MACRO;
	for (i = 0; i < deflen; i++)
		adef->s[i] = def[i];
	adef->s[i] = '\0';

	if (aname == NULL) {
		if ( (aname = userconf_malloc(namelen)) == NULL ) {
			userconf_free(adef);
			return;
		}
		aname->flags |= UC_ALIAS_NAME;
		if (is_macro)
			aname->flags |= UC_ALIAS_MACRO;
		for (i = 0; i < namelen; i++)
			aname->s[i] = name[i];
		aname->s[i] = '\0';
		aname->next = UC_ALIASES;
		UC_ALIASES = aname;
	}

	/* Insert this new definition last in definitions of this alias. */
	for (a = aname; a->next != NULL && UC_ALIAS_IS_DEF(a->next);
		a = a->next)
		;
	adef->next = a->next;
	a->next = adef;
}

/*
 * Unaliasing is releasing every struct (and memory) defining the
 * alias. If the alias doesn't exist, it is not an error (unaliasing
 * already done). Since we don't provide for now return status, one
 * can unconditionnally unalias something he wants to define from
 * scratch.
 */
static void
userconf_unalias(const char *name, int namelen)
{
	struct alias *a, *p, *n;

	if ( (a = userconf_alias_find(name, namelen)) == NULL )
		return;	/* de facto done... */

	for (p = UC_ALIASES; p != NULL && p->next != a; p = p->next)
		;

	if (p == a)	/* a == UC_ALIASES */
		p = NULL;

	for (; a->next != NULL && UC_ALIAS_IS_DEF(a->next);) {
		n = a->next;
		a->next = n->next;
		userconf_free(n);	
	}

	if (p != NULL)
		p->next = a->next;
	else
		UC_ALIASES = a->next;

	userconf_free(a);
}

/*
 * Debugging routines.
 */
static void
userconf_debug0(void)
{
	int i;

	userconf_cnt = 0;

	printf("userconf_kconf[] (config(1) generated):\n");
	if (userconf_more())
		return;

	for (i = 0; userconf_kconf[i] != NULL; i++) {
		if (userconf_more())
			break;
		printf("\t%s\n", userconf_kconf[i]);
	}
}

static void
userconf_debug1(void)
{
	struct alias *a;
	int nfree, free_size, nalloced, alloced_size, nnot_alloced;
	int i;

	userconf_cnt = 0;

	printf("Memory: unit: %d bytes; start: %p\n",
		(int) sizeof(void *), userconf_mem);	
	if (userconf_more())
		return;

	for (alloced_size = nalloced = 0, a = UC_ALIASES;
		 a != NULL; a = a->next) {
		alloced_size += UC_REALSIZE(a->size); 
		++nalloced;
	}
	for (free_size = nfree = 0, a = UC_MEMFREE;
		a != NULL; a = a->next) {
		free_size += a->size; 
		++nfree;
	}

	if (userconf_more())
		return;
	printf("\ttotal\tallocated\tfree\n\t%d\t%d\t%d",
		UC_MEMSIZE, alloced_size, free_size);
	if (UC_MEMSIZE == alloced_size + free_size)
		printf("\tOK\n");
	else
		printf("\tINCONSISTENT\n");

	if (userconf_more())
		return;
	printf("Structs allocation:\n");
	if (userconf_more())
		return;
	printf("\ttotal\tallocated\tfree\n");
	for (i = nnot_alloced = 0; i < UC_MAX_MEM_OBJECTS; i++) {
		if (!(UC_MEMOBJ[i].flags & UC_ALIAS_ALLOCATED))
			++nnot_alloced;
	}
	if (userconf_more())
		return;
	printf("\t%d\t%d\t%d", UC_MAX_MEM_OBJECTS,
		nalloced + nfree, nnot_alloced);
	if (UC_MAX_MEM_OBJECTS == nalloced + nfree + nnot_alloced)
		printf("\tOK\n");
	else
		printf("\tINCONSISTENT\n");

	if (userconf_more())
		return;
	printf("Memory map:\n");
	for (a = UC_MEMFREE; a != NULL; a = a->next) {
		if (userconf_more())
			break;
		printf("%s%p (%d)\n", (a == UC_MEMFREE)? "" : "\t",
			a->s, a->size);
	}
}

static void
userconf_debug2(void)
{
	struct alias *a;

	userconf_cnt = 0;

	printf("Aliases (address, size bytes (size in mem units)):\n");
	if (userconf_more())
		return;
	for (a = UC_ALIASES; a != NULL; a = a->next) {
		if (userconf_more())
			break;
		if (UC_ALIAS_IS_NAME(a)) 
			printf("%s%s (%p, %db (%du)):\n",
				(UC_ALIAS_IS_MACRO(a))? "*" : "",
				a->s, a->s, a->size, (int) UC_REALSIZE(a->size));
		else	/* def of name */
			printf("\t(%p, %db (%du)) %s\n",
				a->s, a->size, (int) UC_REALSIZE(a->size), a->s);
	}
}
