/*	$NetBSD: parse.c,v 1.5 2022/08/10 08:37:53 christos Exp $	*/

/*-
 * Copyright (c) 1998, 2003, 2023 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center and by Matthias Scheler.
 *
 * The parsing has been substantialy (almost totally) rewritten
 * by Thierry Laronde on June 2023, to fix the v2 syntax and add
 * syntax information for the added checker mode as well as not exiting
 * here for the new added resilient mode.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1983, 1991, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)inetd.c	8.4 (Berkeley) 4/13/94";
#else
__RCSID("$NetBSD: parse.c,v 1.5 2022/08/10 08:37:53 christos Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/stat.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <glob.h>
#include <libgen.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

#include "inetd.h"

/*
 * Fixed strings; may be keywords or not in the sense that they can be
 * keywords expected as such by the general syntax, or fixed substrings
 * of a value (susceptible of quoting and escaping). Hence the name
 * keystring instead of keyword.
 */
#define INVALID_KS	-1	/* by convention; last NULL entry */
struct keystring {
	const char	*const ks;	/* the string by itself */
	size_t		len;	/* its length */
	int		val;	/* the integer value to return if keyword is matching */
	int		min_argc;	/* de dicto */
	int		max_argc;	/* de dicto */
};

static int	assemble_escaped_nl(void);
static int	fshift(void);
static int	fpush(const char *);
static void	fpop(void);
static int	ipsec_get_ck(void);
static void	get_keyword(void);
static int	glob_error(const char *, int);
static int	load_stmt(void);
static void	load_value(void);
static int	next_line(void);
static int	parse_accept_filter(void);
static int	parse_directive(void);
static int	parse_protocol(bool);
static int	parse_server(void);
static int	parse_socktype(void);
static int	parse_srv(void);
static int	parse_srv_v1(void);
static int	parse_srv_v2(void);
static bool	is_same_service(const struct servtab *, const struct servtab *);
static int	skip_to_future_token(void);
static void	tsep_defaults(void);
static int	tsep_ck(void);
static int	tsep_store(void);
static int	wait_ck(void);

static char TCPMUX_TOKEN[] = "tcpmux/";
#define MUX_LEN		(sizeof(TCPMUX_TOKEN)-1)

/*
 * Our error messages. Instead of spreading strings all around, the
 * config() subroutines return an error integer going back to config()
 * that, finally, print the related message, perhaps using strerror(3)
 * if the error is flagged CFG_FLAG_ESYS, and the context (unwinding
 * the stack), before returning a sysexits(3) code to the main caller.
 *
 * A routine can still return directly a sysexits(3), that starts at
 * EX__BASE (64) and this one will go with a message indicating just
 * the number. But if there is errno set, use the CFG_* equivalent
 * in order for strerror(3) to be also displayed.
 */
#define CFG_OK				0
#define CFG_EDEPTH			1
#define CFG_EGLOBBING			2
#define CFG_EFNMAX			3
#define CFG_ELOOP			4
#define CFG_EREAD			5	/* will call strerror */
#define CFG_EPERM			6	
#define CFG_EMEM			7	/* will call strerror */
#define CFG_ETAMPERING_WITH_FILE	8
#define CFG_ETOO_LONG			9
#define CFG_ECFG			10	
#define CFG_EMISSING_ARGS		11
#define CFG_ETOO_MANY_ARGS		12
#define CFG_ESRV			13
#define CFG_FINISHED			14
#define CFG_EPROGFAULT			15
#ifdef IPSEC
#define CFG_EIPSEC			16	/* will call ipsec_strerror */
#endif

#define CFG_FLAG_ESYS	0x01	/* using errno and strerror() */
#define CFG_FLAG_EIPSEC	0x02	/* using ipsec_strerror() */
static struct cfg_error {
	int flags;
	int sysexit;	/* matching sysexits(3) exit value */ 
	const char * const msg;
} cfg_errors[] = {
	{ 0, EX_OK, NULL },
	{ 0, EX_RESOURCE, "Maximum depth of inclusion reached" },
	{ 0, EX_SOFTWARE, "Error when globbing" },
	{ 0, EX_RESOURCE, "Maximum number of config files to read passed" },
	{ 0, EX_CONFIG, "LOOP: The file to input is already on the stack" },
	{ CFG_FLAG_ESYS, EX_IOERR, "Error getting the next line" },
	{ 0, EX_NOPERM, "SECURITY ERROR: config writable by not root" },
	{ CFG_FLAG_ESYS, EX_RESOURCE, "Memory error" },
	{ 0, EX_SOFTWARE, "Tampering was done with a file I was reading" },
	{ 0, EX_CONFIG, "Statement too long" },
	{ 0, EX_CONFIG, "Syntax error" },
	{ 0, EX_CONFIG, "Missing arguments for the statement" },
	{ 0, EX_CONFIG, "Too many arguments in srv statement" },
	{ 0, EX_CONFIG, "Incorrect service definition" },
	{ 0, EX_OK, "Config successfully parsed" },
	{ 0, EX_SOFTWARE, "Wrong code path! Programmer's fault" },
#ifdef IPSEC
	{ CFG_FLAG_EIPSEC, EX_CONFIG, "The ipsec spec is incorrect" }
#endif
};
	
/*
 * A config is made up, from the root (absolute path), by assembling
 * in line by line order the statements, eventually "dot'ing" files.
 * Default address and current ipsec are global context for the
 * config as well as the flags indicating, when dot'ing files, if the
 * defhost and defipsec must be restored after quitting a dot'ed file.
 */
static int	flags;	/* grestoring */
#define CFG_GRESTORE_DEFHOST_FLAG	0x01
static const char	* const defhost_default = "*";	/* de dicto */
static char		*defhost; /* current defhost; NULL is default */
#ifdef IPSEC
#define CFG_GRESTORE_IPSEC_FLAG	0x02
static const char	* const defipsec_default = "";	/* de dicto */
static char		*defipsec; /* current ipsec policy; NULL is default */
#endif

/*
 * We must ensure that THE config is unambiguous: the root is given as
 * absolute, the included files are given whether as absolute or as
 * relative _to the dirname of the file they are directly included 
 * from_.
 *
 * We must ensure that the process will finish; so we don't take files
 * that are not regular files (pipe); we verify that their size is
 * finite; we don't input an indefinite cardinal of files (CFG_F_NMAX);
 * we don't allow either an indefinite depth (CFG_FDEPTHMAX); and we
 * must catch loops too.
 *
 * Globbing adds to the burden, since from the simple one dimension
 * stack we go with a two dimensions problem: inputing in order
 * (horizontaly) several files (that, in their turn, can also include
 * and so on...).
 *
 * So we have a vertical stack for a series of "horizontal" (at the
 * same depth and included by the same globbing directive) files that
 * we will process in turn.
 *
 * So we use "shift" for meaning avancing horizontally in the series; 
 * "push" to increase depth of the stack by adding an "horizontal"
 * series of files and "pop" to eject this frame when done with all
 * the files in the horizontal series.
 *
 * And we must provide also a more informative way of displaying syntax
 * error by putting reference to the file containing the fault, and the
 * line number in this file---more informations will be available inside
 * the line when parsing the statement is done.
 *
 * So we handle a stack of opened files. When a file is included, we
 * push the file on the stack. What is a loop then? Simply the pushing
 * of a file already opened _at this moment_ (others on horizontal
 * lists are not concerned) on the file stack. [TL]
 */
static int nfread;	/* total number of files read */
static int nfentry;	/* cardinal pushed on fstack */
#define CFG_FNMAX	32	/* maximum number of files we will open */
#define CFG_FDEPTHMAX	4	/* maximum depth in file stack */

/*
 * The fentry struct must allow the management we need and keep
 * information about the global values (defhost and ipsec)
 * as well as what allows us to display syntax error messages in an
 * useful way.
 */
struct	fentry {
	struct fentry	*prev;
	int	npath; /* cardinal in "horizontal" dimension (globbing) */
	int	ipath;	/* index in "horizontal" dimension */
	char	**paths;	/* "horizontal" series */
	FILE	*fp;	/* this and seq are for this ipath in this frame */
	int	fd;
	dev_t	st_dev; /* this and the following are the file id */
	ino_t	st_ino;
	long	size;	/* this size at opening; should not change */
	long	read;	/* this amount read so far */
	long	stmt_size;	/* the total size written in stmt buffer */
	int	iline;	/* current first line of stmt from cur ipath */	
	int	nline;	/* #lines constituting the stmt */
	int	line_start;	/* index in stmt where last line starts */
	char	*defhost;	/* not NULL if need to be restored */
#ifdef IPSEC
	char	*defipsec;		/* idem */
#endif
};
static struct fentry *cur_fe;	/* a pointer to last file struct on stack */

static int
glob_error(const char *path, int error)
{
	return 1;	/* globbing will fail on error */
}

static int
fpush(const char *path)
{
	char	*p;
	struct fentry	*fe;
	glob_t	globbed;

	if (nfentry == CFG_FDEPTHMAX)
		return CFG_EDEPTH;

	if (path[0] != '/') { /* rooting the pattern */
		char	*edirname;	/* end of dirname of previous file */

		assert(nfentry != 0); /* root file should be caught by usage */

		edirname = strrchr(cur_fe->paths[cur_fe->ipath], '/');
		assert(edirname != NULL);

		++edirname;	/* points after the '/' */
		if ( (p = malloc(strlen(path)
			+ (size_t)(edirname - cur_fe->paths[cur_fe->ipath]) + 1))
			== NULL )
			return CFG_EMEM;
		p[0] = '\0';
		(void)strncat(p, edirname,
			 (size_t)(edirname - cur_fe->paths[cur_fe->ipath]));
		(void)strcat(p, path);
	} else {
		if ( (p = strdup(path)) == NULL )
			return CFG_EMEM;
	}
	
	/*
	 * We treat the pathname as a pattern and we could rely on flags
	 * set when globbing to test if this was a constant pattern (a
	 * defined file) or a wildcard pattern.
	 * The behavior was, in the precedent implementation,
	 * different: it was an error to no be able to read a defined
	 * file; but it was not if there were wildcards. Meaning that
	 * the acceptance of the configuration would vary: "it depends".
	 * For security reasons, we do not allow this.
	 * Still for definit behavior, the files are included in a
	 * defined order: lexicographically.
	 */
	if ( glob(p, 0, glob_error, &globbed) != 0
		|| globbed.gl_matchc == 0) {
		free(p);
		return CFG_EGLOBBING;
	}
	
	if ( globbed.gl_matchc >= INT_MAX
		 || ((unsigned int)nfread + globbed.gl_matchc) > CFG_FNMAX )
		return CFG_EFNMAX;

	if ( (fe = malloc(sizeof(struct fentry))) == NULL ) {
		free(p);	/* only thing allocated */
		return CFG_EMEM;
	}

	/* From now on, we must be able to pop; so it is pushed. */
	fe->prev = cur_fe;
	fe->npath = (int)globbed.gl_matchc;
	nfread += fe->npath;
	fe->ipath = -1;	/* no horizontal file handled yet */
	fe->paths = globbed.gl_pathv;
	fe->fp = NULL;
	fe->size = fe->read = 0;
	cur_fe = fe, ++nfentry;

	/*
	 * Stmt buffer related members not initialized: this is
	 * load_stmt() responsability.
	 */
	if ((flags & CFG_GRESTORE_DEFHOST_FLAG)) {
		if ( (fe->defhost = strdup(
			(defhost == NULL)? defhost_default : defhost) )
			 == NULL )
			return CFG_EMEM;
	} else
		fe->defhost = NULL; /* here NULL means don't restore */
#ifdef IPSEC
	if ((flags & CFG_GRESTORE_IPSEC_FLAG)) {
		if ( (fe->defipsec = strdup(
			(defipsec == NULL)? defipsec_default : defipsec) )
			 == NULL )
			return CFG_EMEM;
	} else
		fe->defipsec = NULL; /* here NULL means don't restore */
#endif

	return fshift();
}

/*
 * Shifting will have the side effect of popping if the horizontal
 * series is exhausted.
 */
static int
fshift(void)
{
	struct fentry	*fe;
	struct stat	sb;

	/* Done with current if defined. */
	if (cur_fe->ipath != -1) {
		DPRINTF("Done with '%s'.", cur_fe->paths[cur_fe->ipath]);
		if (cur_fe->fp != NULL) 
			(void)fclose(cur_fe->fp);
		cur_fe->fp = NULL;
		if (cur_fe->fd != -1)
			(void)close(cur_fe->fd);
		cur_fe->fd = -1;
		cur_fe->size = cur_fe->read = 0;
	}

	/* If defhost or ipsec are not NULL, they have to be restored. */
	if (cur_fe->defhost != NULL) {
		free(defhost);
		if (strcmp(cur_fe->defhost, defhost_default) != 0) {
			if ( (defhost = strdup(cur_fe->defhost))
				== NULL )
				return CFG_EMEM;	
		} else
			defhost = NULL;	/* NULL means default */
	}
#ifdef IPSEC
	if (cur_fe->defipsec != NULL) {
		free(defipsec);
		if (strcmp(cur_fe->defipsec, defipsec_default) != 0) {
			if ( (defipsec = strdup(cur_fe->defipsec))
				== NULL )
				return CFG_EMEM;	
		} else
			defipsec = NULL;	/* NULL means default */
	}
#endif

	if (++cur_fe->ipath < cur_fe->npath) { /* horizontal not exhausted */
		if ( (cur_fe->fd = open(cur_fe->paths[cur_fe->ipath],
			O_RDONLY | O_REGULAR)) < 0 
			|| fstat(cur_fe->fd, &sb) < 0 ) {
			return CFG_EREAD;
		}

		/* Identifying the file. */
		cur_fe->st_dev = sb.st_dev, cur_fe->st_ino = sb.st_ino;

		/* Detecting the loop. */
		for (fe = cur_fe->prev; fe != NULL; fe = fe->prev) {
			if ( fe->st_dev == cur_fe->st_dev
				&& fe->st_ino == cur_fe->st_ino ) {
				return CFG_ELOOP;
			}
		}
		/*
		 * If not in checking mode and running as root, and
		 * strict checking required, forbid a file writable by
		 * someone else than root.
		 */
		if (!checker && strict && getuid() == 0) {
			if ( (sb.st_uid != 0)
				|| (sb.st_mode & (S_IWGRP | S_IWOTH)) )
				return CFG_EPERM;
		}

		/* We keep track of the size at opening to prevent tampering. */
		cur_fe->size = (long) sb.st_size;
		cur_fe->read = 0;

		/* And for fgets(3), we want a FILE *. */
		if ( (cur_fe->fp = fdopen(cur_fe->fd, "r")) == NULL ) {
			return CFG_EREAD;
		}
	} else /* done with the vertical frame */
		fpop();

	return CFG_OK;
}

/*
 * We are dealing in fact with a two dimensions structure; we explore an
 * horizontal list (paths in lexicographical order), shifting meaning
 * closing the current and opening the next, until the horizontal
 * list is exhausted, when we do pop the vertical frame.
 */
static void
fpop(void)
{
	struct fentry	*fe;

	assert(cur_fe != NULL);

	for (cur_fe->ipath = 0; cur_fe->ipath < cur_fe->npath;
		++cur_fe->ipath)
		free(cur_fe->paths[cur_fe->ipath]);

	free(cur_fe->paths);
	free(cur_fe->defhost);
	free(cur_fe->defipsec);
	if (cur_fe->fp != NULL)
		(void)fclose(cur_fe->fp);
	if (cur_fe->fd != -1)
		(void)close(cur_fe->fd);
	fe = cur_fe;
	cur_fe = cur_fe->prev;
	free(fe);
	--nfentry;
	assert( nfentry >= 0 && (nfentry != 0 || cur_fe == NULL) );
}

/*
 * STATEMENT BUFFERING.
 */
/*
 * We treat statement by statement.
 *
 * A valid statement is whether '^\n': the empty statement, or a
 * series of lines starting with an non empty statement.
 *
 * An end of statement is whether '\n' or '[[:blank:]]*;[[:blank:]]*',
 * except, if the statement is not the empty statement, that we allow
 * continuation lines '\n[[:blank:]]' for non empty statement.
 *
 * The parser will treat a complete statement and load_stmt() will
 * simply sets everything to the beginning of the next valid statement.
 *
 * There is a fixed limited length stmt buffer. The next token character
 * to scan in the buffer (whether '\0' or the start of a token: keyword
 * or value; operator or separator are treated with booleans but are
 * by-passed) in the buffer is defined by itoscan.
 */

/*
 * sap_stack has to be (2 * sizeof stmt). So if one changes this, it
 * has to be adjusted in sap.c.
 */
static char	stmt[LINE_MAX];	/* whole statement read from current file */
static int	itoscan;	/* index in stmt buffer to next token */
#define CHARTOSCAN	(stmt[itoscan])

/*
 * The statement parsing routines will set the boolean eof_stmt when
 * the end of the current statement is reached---this does not mean
 * that the stmt buffer is exhausted: there can be another statement
 * beginning. But if eof_stmt is true whether CHARTOSCAN == '\0' or
 * CHARTOSCAN points to the first char of the new statement, ';'
 * being considered as '[[:blank:]]*;[[:blank:]]*' (trailing blanks
 * are not part of the beginning of the next statement).
 */
static bool	eof_stmt;

/*
 * For the key '=' value[s] expression, we will set too the 
 * has_assign ('=') and has_separator (',') when after reading a
 * keyword or a value with have encountered (and by-passed) an operator
 * or a separator.
 * We discuss later how the tokenization routine handles operators,
 * separators and terminators.
 */
static bool	has_assign;
static bool	has_separator;

/*
 * We set the type of statement we are parsing in stmt_type.
 */
#define STMT_INVALID	-1
#define STMT_EMPTY	0
#define STMT_DRTV	1	
#define STMT_SRV	2
static int	stmt_type;	/* set by load_stmt() to one of above */

/*
 * And instead of having the obligation to return something, we put
 * an integer defining the status of the parsing in stmt_state.
 */
#define STMT_EINVALID		-1
#define STMT_OK			0
#define STMT_ENL		1
#define STMT_EKEYWORD		2
#define STMT_EQUOTE		3
#define STMT_EVALUE		4
static int	stmt_state;	/* set on error to one of the above */

/*
 * For backward compatibility, we need to take care of abuse of '^#'
 * for IPsec policies.
 *
 * The stmt buffer will, eventually, buffer a whole statement including
 * continuation lines. But, initially, we just load the next_line in
 * order to have the beginning of another statement.
 * The problem is that the v2 syntax allows to put another statement on
 * the same line after a semi-colon. So some things are reset (the
 * information about a statement), but we need to check if the current
 * buffer holds the beginning of another statement.
 */

/*
 * The load_stmt() function is called when a previous statement is
 * exhausted. This is the responsability of the tokenization routines
 * to ensure that when a statement is exhausted whether
 * CHARTOSCAN == '\0'---line series exhausted: load afresh---or not:
 * there was a '[[:blank:]]*;[[:blank:]]*' end of statement and
 * itoscan points to what follows.
 */
static int
load_stmt(void)
{
	int	status;

	if (cur_fe == NULL)
		return CFG_FINISHED;

	sap_reset();	/* values are explained later */

	stmt_type = STMT_INVALID;
	stmt_state = STMT_EINVALID;
	eof_stmt = has_assign = has_separator = false;

again:
	if (CHARTOSCAN == '\0') {	/* load afresh; true initially */
		cur_fe->stmt_size = 0;
		stmt_type = STMT_INVALID;
		itoscan = 0;
		stmt[0] = '\0';

		while (cur_fe != NULL) {
			if (fgets(stmt, sizeof stmt, cur_fe->fp) == NULL) {
				if (feof(cur_fe->fp)) {
					if (cur_fe->read == cur_fe->size) {
						if ( (status = fshift()) != CFG_OK )
							return status;
						else
							continue;	
					} else	{
						return CFG_ETAMPERING_WITH_FILE;
					}
				} else
					return CFG_EREAD;
			} else {
				++cur_fe->iline;
				cur_fe->nline = 1;
				cur_fe->line_start = 0;
				cur_fe->read = cur_fe->stmt_size
					= ftell(cur_fe->fp);
				if ( cur_fe->stmt_size
					== ( (long) (sizeof(stmt)) - 1)
					&& stmt[cur_fe->stmt_size-1]
					&& stmt[cur_fe->stmt_size-1]
						!= '\n' ) {
					return CFG_ETOO_LONG;
				} else
					break;
			}
		}
		if (cur_fe == NULL)
			return CFG_FINISHED;

	} else {	/* Make room: shift beginning of line at 0 */
		char	*p, *q;

		for (p = stmt, q = stmt+cur_fe->line_start;
			(*p++ = *q++);)
			;
		itoscan -= cur_fe->line_start;
		cur_fe->line_start = 0;
	}

	/* Compatibility */
	if (CHARTOSCAN == '#') {
		if ( stmt[itoscan+1] == '@'
		&& isblank((unsigned char) stmt[itoscan+2]) ) {
			stmt_type = STMT_DRTV;
			++itoscan;
			stmt_state = STMT_OK;
			return CFG_OK;
		} else {	/* comment: ignored */	
			CHARTOSCAN = '\0';
			goto again;
		}
	} else if (CHARTOSCAN == '\n') {	/* empty line: ignored */
			CHARTOSCAN = '\0';
			goto again;
	} else if ( (CHARTOSCAN == '\\' && stmt[itoscan+1] == '\n')
		|| isspace((unsigned char) CHARTOSCAN) 
		|| CHARTOSCAN == ',' || CHARTOSCAN == ';' ) {
		/*
		 * A statement must start directly and there is no
		 * continuation line if a statement is not started.
		 */
		return CFG_ECFG;
	}

	stmt_state = STMT_OK;

	if (CHARTOSCAN == '.') {
		stmt_type = STMT_DRTV;
		itoscan++;	/* next to scan is keyword after dot */
	} else
		stmt_type = STMT_SRV;

	return CFG_OK;
}

/*
 * From now on we will parse a stmt, whose beginning is in the buffer.
 * The main task is to keep itoscan pointing to the next token, a
 * token being whether a keyword (taken as is) or a value (than can
 * have lexicographical acrobatics). What about the operator '=',
 * the separator ',' and the terminator ';'? The operator is always 
 * by-passed and the has_assign then set; the separator and terminator
 * are not always by-passed because the syntax allows to give a null
 * value in an assignment: "keyword = [,\n;]". So, in this case, it
 * depends on the has_assign boolean. If it is true, the separator or
 * terminator is not by-passed.
 *
 * This is the task of skip_to_future_token() to set correctly the
 * booleans so that itoscan points always to a keyword, a value or
 * a separator (for an empty value) or '\0' when the stmt is finished
 * and the buffer is exhausted too.
 * This procedure is called inside get_keyword() and get_value() to
 * prepare things for the remaining.
 *
 * There are not only blanks, '=' and ',' to bypass: there are
 * continuation lines that will be discussed later but that are handled
 * here and also inside get_keyword() (for "\\\n" sequence) and
 * get_value() for escaping and quoting.
 *
 * Returns: 0: OK; < 0: error.
 */

static int
skip_to_future_token(void)
{
	if (eof_stmt) return -1;

	has_assign = has_separator = false;

again:
	for (; isblank((unsigned char)CHARTOSCAN); itoscan++)
		;
	if (assemble_escaped_nl() < 0)
		return -1; 
	if (CHARTOSCAN == '\n') {
		int c;

		c = getc(cur_fe->fp);
		(void) ungetc(c, cur_fe->fp);

		if (isblank(c)) {
			if (next_line() != CFG_OK)
				return -1;
			++itoscan;
			goto again;
		}
	}

	if (CHARTOSCAN == '#') {	/* trailing '\n' does exist */
		for (; CHARTOSCAN != '\n'; ++itoscan)
			;
		goto again;
	}

	if (CHARTOSCAN == '=') {
		if (has_assign || has_separator)
			return -1;	/* syntax error in every case */
		has_assign = true;
		++itoscan;
		goto again;
	} else if (has_assign)
		return 0;	/* load_value() will handled empty value */ 

	/* !has_assign */

	if (CHARTOSCAN == '\n' || CHARTOSCAN == ';') {
		eof_stmt = true;
		++itoscan;
		if (CHARTOSCAN != '\0') /* ';' skip to start of next */
			for (; isblank((unsigned char)CHARTOSCAN);
				itoscan++)
				;
		return 0;
	} else if (CHARTOSCAN == '=') {
		if (has_separator)
			return -1;	/* syntax error in every case */
		has_assign = true;
		++itoscan;
		goto again;
	} else if (CHARTOSCAN == ',') {
		if (has_separator) 
			return -1;	/* empty assignment is error */
		has_separator = true;
		++itoscan;
		goto again;
	}

	return 0;
}

/*
 * A next line is only a continuation line coming from the very same
 * file: it can not bypass a file boundary.
 * It will simply load the next line from the file, putting it
 * after the previous load.
 * A caller has to manipulate cur_fe->stmt_size and itoscan in order
 * to overwrite for example the '\\\n' sequence.
 *
 * It has to be called when we know there is still something to read.
 * A failure to read is not an indication of eof, but an error.
 */
static int
next_line(void)
{
	long remaining;


	remaining = (long) sizeof(stmt) - cur_fe->stmt_size;

	if (fgets(stmt+itoscan, (int) remaining, cur_fe->fp) == NULL)
		return CFG_EREAD;
	
	cur_fe->iline++;
	cur_fe->nline++;
	cur_fe->line_start = itoscan;
	cur_fe->stmt_size += ftell(cur_fe->fp) - cur_fe->read;
	cur_fe->read = ftell(cur_fe->fp);

	if ( cur_fe->stmt_size == ( (long) (sizeof(stmt)) - 1)
		&& stmt[cur_fe->stmt_size-1]
		&& stmt[cur_fe->stmt_size-1] != '\n' )
		return CFG_ETOO_LONG;

	/*
	 * Verifying that nobody tampered with the file and that we will
	 * read a fixed amount of data.
	 */
	if (cur_fe->read > cur_fe->size) {
			return CFG_ETAMPERING_WITH_FILE;
	}

	return CFG_OK;
}

/*
 * Our work is tokenizing.
 *
 * There are two different cases: tokens that have to be given "as is":
 * keywords as well as operators and separators; and values that can be
 * given with a mix of quoting and escaping in order to bypass
 * characters that have a special meaning and would change the
 * tokenization.
 *
 * The stmt buffer is kept unchanged except for continuation lines given
 * by a backslash before a newline: these two characters are erased and
 * the next line is glued to the previous one just after the char
 * preceding the backslash.
 *
 * The current token read is indicated by its start in the buffer
 * and its len.
 */
static int	itok;	/* first char in stmt of current token */
#define TOKSTR	(stmt+itok)
static int	toklen;	/* length of the token */
static int	toktype;/* one of the following */
#define TOK_INVALID	-1
#define TOK_KEYWORD	0
#define TOK_VALUE	1

/*
 * We allow the escaped newline \'a la POSIX.2 sh(1).
 */
static int
assemble_escaped_nl(void)
{
	while (CHARTOSCAN == '\\' && stmt[itoscan+1] == '\n') {
		CHARTOSCAN = '\0';
		cur_fe->stmt_size -= 2;
		if (next_line() != CFG_OK) {
			stmt_state = STMT_ENL;
			return -1;
		}
	}

	return 0;
}

/*
 * A keyword has the same definition as in C: starts by an alpha or
 * '_' and * is composed only of alphanum or '_'. After the keyword
 * there is whether a blank, a not escaped newline, an operator or a
 * terminator (not a separator; separators are for values) or a comment.
 * Callers have to verify !eof_stmt before calling: something is
 * expected.
 */
static void
get_keyword(void)
{
	if (eof_stmt) return;

	itok = itoscan;
	toklen = 0;
	toktype = TOK_INVALID;

	stmt_state = STMT_EKEYWORD;

	if (has_assign) return;	/* syntax error */

	for (; isalnum((unsigned char) CHARTOSCAN) || CHARTOSCAN == '_'
		|| CHARTOSCAN == '\\'; ++itoscan) {
		if (CHARTOSCAN == '\\') {
			if (stmt[itoscan+1] == '\n') {
				if (assemble_escaped_nl() < 0)
					return;
				--itoscan;	/* increased in loop */
			} else
				break;
		}
	}

	/* must start by an alpha or underscore */
	if ( !isalpha((unsigned char)TOKSTR[0]) && TOKSTR[0] != '_' )
		return;	/* incorrect keyword */

	/*
	 * Can only be followed by blanks, operator,
	 * terminator, comment or a newline or end of buffer.
	 */
	if ( !isblank((unsigned char) CHARTOSCAN) && CHARTOSCAN != '\n'
		&& CHARTOSCAN != '=' && CHARTOSCAN != '#'
		&& CHARTOSCAN != ';' && CHARTOSCAN != '\0' )
		return;	/* incorrect keyword or syntax error */

	toktype = TOK_KEYWORD;
	toklen = itoscan - itok;
	stmt_state = STMT_OK;
	if (skip_to_future_token() < 0)
		stmt_state = STMT_EINVALID;
}

/*
 * Values are the most complex but the bulk of the work is done by
 * the String Auxiliary Processor i.e. the... sap! See the dedicated
 * file and its header for details.
 *
 * Here, we need only to know from which index to which index in the
 * stmt buffer this is a token value. We simply pass it (perhaps by
 * chunks) to the SAP.
 */
static void
load_value(void)
{
	char	quote;

	stmt_type = STMT_EVALUE;
	itok = itoscan;
	toktype = TOK_INVALID;
	toklen = 0;

	quote = '\0';	/* '\0' can not appear; reset to opening quote */
	while ( quote != '\0' || !isblank((unsigned char)CHARTOSCAN) ) {
		if (quote == '\0') {
			if ( CHARTOSCAN == '=' /* syntax error */
				|| CHARTOSCAN == '#'
				|| CHARTOSCAN == ','
				|| CHARTOSCAN == ';'
				|| CHARTOSCAN == '\n' )
				break;
		}
		if (CHARTOSCAN == '\'' || CHARTOSCAN == '"') {
			if (quote == '\0') {
				quote = CHARTOSCAN;
			} else if (quote == CHARTOSCAN) {
				quote = '\0';
			}
			++itoscan;
			continue;
		}
		if (CHARTOSCAN == '\\') {
			if ( (quote == '\0' || quote == '"')
				&& stmt[itoscan+1] == '\n') {
				if (assemble_escaped_nl() < 0)
					return;
				stmt_state = STMT_EVALUE;
				continue;
			}
			if ( quote == '"' && stmt[itoscan+1] == '"' )
				++itoscan; /* will be bypassed */
		}
		if (quote != '\0' && CHARTOSCAN == '\n') {
			++itoscan;	/* next line starts here */
			if (next_line() != CFG_OK) {
				stmt_state = STMT_ENL;
				return;
			}
		} else
			++itoscan;	
	}

	if (quote != '\0') {
		stmt_state = STMT_EQUOTE;	
		return;
	}
	
	toklen = itoscan - itok;	/* may be 0: empty value */
	toktype = TOK_VALUE;
	stmt_state = STMT_OK;
	sap_push(TOKSTR, toklen, SAP_PUSH_UNQUOTE);
	if (skip_to_future_token() < 0)
		stmt_state = STMT_EINVALID;	
}

/*
 * The real work of inetd(8) is not to parse the config, but to serve.
 * We finally come to the services.
 * When parsing, a tentative servtab tsep will be defined that will be 
 * finally, if OK, be added to the linked list of services.
 */
	
static struct servtab	tsep;	/* tentative servtab defined from cfg */

/* An init constant servtab. */
static const struct servtab serv_ini = {
	.se_hostaddr = NULL,
	.se_service = NULL,
	.se_proto = NULL,
	.se_user = NULL,
	.se_group = NULL,
	.se_bi = NULL,
	.se_server = NULL,
#ifdef IPSEC
	.se_ipsec = NULL,
#endif
	/*
	 * Set values to max by default or to a defined invalid value.
	 */
	.se_service_max = SERVTAB_SERVICE_MAX,
	.se_ip_max = SERVTAB_UNSPEC_SIZE_T,
	.se_wait = SERVTAB_UNSPEC_VAL,
	.se_socktype = SERVTAB_UNSPEC_VAL,
	.se_rl_ip_list = SLIST_HEAD_INITIALIZER(se_ip_list_head),
	.se_next = NULL,
	.se_allocated = NULL,
	/* Not served yet. */
	.se_fd = -1,
	.se_type = NORM_TYPE,
	.se_rpcprog = -1
	/* All other fields initialized to 0 */
};

/*
 * An empty config is a valid config. So (*cfg_serv == NULL) is not,
 * by itself, a test of failure: the return status is the test.
 */
int
config(const char *cfg_name, struct servtab **cfg)
{
	int status;
	struct servtab 	*fsep, *lsep;	/* first sep, last sep in cfg_serv */

	/*
	 * We do not merge. We parse and caller can make whatever he
	 * wants with a successfully parsed config i.e. a servtab list.
	 */
	assert(*cfg == NULL);
	*cfg = NULL;

	fsep = lsep = NULL;	/* What we are creating here. */

	/*
	 * For compatibility with previous implementation, by default,
	 * defhost and ipsec are reset after include.
	 * By default, defhost is any: "*" and defipsec is
	 * unset: "".
	 */
	free(defhost);
	defhost = NULL;	/* means default */
	flags = CFG_GRESTORE_DEFHOST_FLAG;

#ifdef IPSEC
	free(defipsec);
	defipsec = NULL;	/* means default */
	flags |= CFG_GRESTORE_IPSEC_FLAG;
#endif

	if ( (status = fpush(cfg_name)) != CFG_OK )
		goto done;
	
	while ( (status = load_stmt()) == CFG_OK ) {
		struct servtab *sep;

		switch (stmt_type) {
		case STMT_DRTV:
			if ( (status = parse_directive()) != CFG_OK )
				goto done;
			continue;
			break;
		case STMT_SRV:
			if ( (status = parse_srv()) != CFG_OK )
				goto done;
			break;
		default:
			status = CFG_EPROGFAULT;
			goto done;
			break;
		}

		/* v2 Off entries are parsed but skipped. */
		if (tsep.se_checked == false) /* nothing allocated */
			continue;

		tsep_defaults();

		/*
		 * We are here only if a service has been successfully
		 * parsed (and is "on" if v2 srv syntax definition).
		 */
		/* First check semantics consistency */
		if ( (status = tsep_ck()) != CFG_OK )
			goto done;

		/* Find a possibly already existing service definition */
		for (sep = fsep; sep != NULL; sep = sep->se_next)
			if (is_same_service(sep, &tsep))
				break;

		if (sep != NULL)
			sep_release(sep);
		else {
			if ( (sep = (struct servtab *)
				malloc(sizeof(struct servtab))) == NULL ) {
				status = CFG_EMEM;
				goto done;
			}
			/* Insert the allocated struct last. */
			if (fsep == NULL)
				fsep = lsep = sep;
			else {
				lsep->se_next = sep;
				lsep = sep;
			}
		}

		if ( (status = tsep_store()) != CFG_OK )
			goto done;

		*sep = tsep;
	}

done:
	free(defhost); defhost = NULL;
#ifdef IPSEC
	free(defipsec); defipsec = NULL;
#endif
	assert(status != CFG_OK);
	assert(status != CFG_FINISHED || cur_fe == NULL);
	if (cfg_errors[status].sysexit == EX_CONFIG) {
		int i;
		char *p;
		struct fentry *fep;

		DPRINTF("\nError in stmt in '%s', line %d to %d",
			cur_fe->paths[cur_fe->ipath],
			cur_fe->iline - cur_fe->nline +1,
			cur_fe->iline);	
		if (stmt_state != STMT_OK) {
			if (stmt_state == STMT_EINVALID)
				DPRINTF("Invalid statement.");
			else {
				DPRINTF("Token (%s): '%.*s'",
					(toktype == TOK_INVALID)? "invalid"
						: ( (toktype == TOK_KEYWORD)? "keyword"
							: "value" ),
		 			toklen, TOKSTR);
				if (toktype == TOK_VALUE)
					DPRINTF("Value interpreted: '%s'",
						SAP_CURSTR);
				DPRINTF("Line %d scanned: '%.*s'", cur_fe->iline,
					itoscan - cur_fe->line_start,
					stmt+cur_fe->line_start);
			}

			DPRINTF("State of the stack when error occurred:");
			/*
		 	 * Indent the names with tabs to show the
		 	 * include dependencies.
		 	 */
			for (p = stmt, i = 0; i < nfentry; i++)
				p[i] = '\t';	/* indentation */

			for (i = nfentry - 1, fep = cur_fe; i >= 0 && fep != NULL;
				i--, fep = fep->prev) {
				DPRINTF("%.*s%s, line: %d", i, stmt,
					fep->paths[fep->ipath], fep->iline);
			}
		}
	}
	if (status == CFG_FINISHED)
		*cfg = fsep;
	else
		cfg_drop(fsep);

	/* done with the stack */
	while (cur_fe)
		fpop();

	sap_reset();

	if (status < EX__BASE) {
		if (cfg_errors[status].flags & CFG_FLAG_ESYS)
			DPRINTF("System reported error: %s", strerror(errno));
#ifdef IPSEC
		else if (cfg_errors[status].flags & CFG_FLAG_EIPSEC)
			DPRINTF("IPSEC reported error: %s",
				ipsec_strerror());
#endif

		DPRINTF("%s%s", (status != CFG_FINISHED)? "Error: " : "",
			cfg_errors[status].msg);
		status = cfg_errors[status].sysexit;
	}

	return status;
}

/*
 * DIRECTIVES.
 */
#define DRTV_DEFHOST		0
#define DRTV_GRESTORE		1
#define DRTV_INCLUDE		2
#define DRTV_IPSEC			3
static struct keystring drtv_ks[] = {
	{ "defhost", 7, DRTV_DEFHOST, 0, 1 },
	{ "grestore", 8, DRTV_GRESTORE, 0, 2 },
	{ "include", 7, DRTV_INCLUDE, 1, 1 },
#ifdef IPSEC
	{ "ipsec", 5, DRTV_IPSEC, 0, MAXARGV },
#endif
	{ NULL, 0, INVALID_KS, 0, 0 } 
};

static int
parse_directive(void)
{
	int	i;

	/* !!! Compatibility */
	if (CHARTOSCAN == '@') {
		my_syslog(LOG_ERR,
			"Deprecated '#@' directive found in '%s', line '%d'.\n"
			"Use '.ipsec' in its stead",
				cur_fe->paths[cur_fe->ipath], cur_fe->iline);
		i = 3;	/* point on "ipsec" */
		++itoscan;
		if (skip_to_future_token() < 0)
			return CFG_ECFG;
	} else {
		get_keyword();
		if (stmt_state != STMT_OK) return CFG_ECFG;
		for (i = 0; drtv_ks[i].ks != NULL && 
			( toklen != (int) drtv_ks[i].len
				|| (strncmp(TOKSTR, drtv_ks[i].ks,
					(size_t) toklen) != 0));
			 ++i)
			;
	}
	switch (drtv_ks[i].val) {
		case DRTV_DEFHOST:
			DPRINTF("Found defhost directive, in '%s', line '%d'",
				cur_fe->paths[cur_fe->ipath], cur_fe->iline);
			free(defhost);
			if (eof_stmt) {
				defhost = NULL; /* means default */
			} else {
				if (has_assign || has_separator)
					return CFG_ECFG;
				load_value();
				if (stmt_state != STMT_OK || !eof_stmt
					|| has_separator)
					return CFG_ECFG;
				sap_store(SAP_STORE_STRING, &defhost);
				if (sap_state == SAP_ESTORE)
					return CFG_EMEM;
			}
			break;
		case DRTV_GRESTORE:
			DPRINTF("Found grestore directive, in '%s', line '%d'",
				cur_fe->paths[cur_fe->ipath], cur_fe->iline);
			flags &= ~(CFG_GRESTORE_DEFHOST_FLAG
				| CFG_GRESTORE_IPSEC_FLAG);
			while (!eof_stmt) {
				if (has_assign || has_separator)
					return CFG_ECFG;
				get_keyword();
				if (stmt_state != STMT_OK)
					return CFG_ECFG;
				if (strncmp(TOKSTR, "defhost", (size_t) toklen) == 0)
					flags |= CFG_GRESTORE_DEFHOST_FLAG;
				else if (strncmp(TOKSTR, "ipsec", (size_t) toklen) == 0)
					flags |= CFG_GRESTORE_IPSEC_FLAG;
				else
					return CFG_ECFG;
			}
			break;
		case DRTV_INCLUDE:
			DPRINTF("Found include directive, in '%s', line '%d'",
				cur_fe->paths[cur_fe->ipath], cur_fe->iline);
			if (eof_stmt)
				return CFG_EMISSING_ARGS;
			if (has_assign || has_separator)
				return CFG_ECFG;
			load_value();
			if ( (stmt_state != STMT_OK) || !eof_stmt 
				|| has_separator)
				return CFG_ECFG;
			return fpush(SAP_CURSTR);
			sap_pop();
			break;
#ifdef IPSEC
		case DRTV_IPSEC:
			DPRINTF("Found IPSEC policy, in '%s', line '%d'",
				cur_fe->paths[cur_fe->ipath], cur_fe->iline);
			free(defipsec);
			defipsec = NULL;
			if (ipsec_get_ck() < 0)
				return CFG_EIPSEC;
			if (!eof_stmt)
				return CFG_ECFG;
			if (SAP_CURLEN) {
				sap_store(SAP_STORE_STRING, &defipsec);
				if (sap_state == SAP_ESTORE)
					return CFG_EMEM;
			} else
				sap_pop();
			break;
#endif
		default:
			return CFG_ECFG;
			break;
	}
	return CFG_OK;
}

/*
 * Finally...
 */
static int
parse_srv(void)
{
	tsep = serv_ini;

	/*
	 * LISTEN-ADDRESS is a common prefix to v1 and v2. [TL]
	 */
	load_value();
	if (stmt_state != STMT_OK)
		return CFG_ECFG;

	/*
	 * We manipulate things so that host is first string, and
	 * service second, in every case.
	 */
	if (sap_split(':', 1) == 1) { /* "host:[service] */
		/* suppress possible '[' ']' surroundings */
		if ( (sap_stack+sap[0].istr)[0] == '[' &&
			(sap_stack+sap[0].istr)[sap[0].len-1] == ']' ) {
			sap_swap();	/* put service first */
			sap_push(SAP_CURSTR+1, SAP_CURLEN-2, 0); /* unbordere */
			/* put modified last; suppress; */
			sap_swap(); sap_pop(); sap_swap();
		}
		if (SAP_CURLEN == 0) { /* default host directive */
			if (!eof_stmt) {
				DPRINTF("Stray deprecated 'defhost:' "
					"directive with trailing garbage");
				return CFG_ECFG;
			}
			DPRINTF("Found deprecated 'defhost:\\n' directive,"
				" in '%s', line '%d'.\n"
				"Use '.defhost' in its stead",
				cur_fe->paths[cur_fe->ipath], cur_fe->iline);
			sap_pop();
			free(defhost);
			if ( SAP_CURLEN == 0
				|| strcmp(defhost_default, SAP_CURSTR) == 0)
				defhost = NULL;
			else {
				sap_store(SAP_STORE_STRING, &defhost);
				if (sap_state == SAP_ESTORE)
					return CFG_EMEM;
			}
			return CFG_OK;
		}
	} else {	/* no host; add default; */
		sap_push((defhost == NULL)? defhost_default : defhost,
			-1, 0);
		if (sap_state == SAP_EPUSH)
			return CFG_EMEM;
		sap_swap();
	}
	
	assert(sap_nstr == 2);
	/*
	 * Service is last.
	 */
	if (strncmp(SAP_CURSTR, TCPMUX_TOKEN, MUX_LEN) == 0) {
		int	delta;

		delta = (int) MUX_LEN;
		if (SAP_CURSTR[delta] == '+') {
			tsep.se_type = MUXPLUS_TYPE;
			++delta;
		} else
			tsep.se_type = MUX_TYPE;
		sap_push(SAP_CURSTR+delta, -1, 0);
		sap_swap(); sap_pop();
	}
	tsep.se_service = SAP_CURSTR;
	sap_join('\0', 1);

	/* Then the host is first with service trailing after '\0' */
	tsep.se_hostaddr = SAP_CURSTR;
	assert(sap_nstr == 1);

	/*
	 * FROM NOW ON V1 AND V2 DIVERGE. [TL]
	 */

	/* on/off/socktype */
	if (eof_stmt)
		return CFG_EMISSING_ARGS;
	else if (has_assign || has_separator)
		return CFG_ECFG;

	load_value();
	if (stmt_state != STMT_OK)
		return CFG_ECFG;

	if (eof_stmt)
		return CFG_EMISSING_ARGS;
	else if (has_assign || has_separator)
		return CFG_ECFG;

	/* We parse off definitions but discard the result after. */
	tsep.se_checked = true;

	if (strcmp(SAP_CURSTR, "off") == 0 || strcmp(SAP_CURSTR, "on") == 0) {
		if (strcmp(SAP_CURSTR, "off") == 0)
			tsep.se_checked = false;
		sap_pop();	/* we do nothing with it */
		return parse_srv_v2();
	} else
		return parse_srv_v1();
}

/*
 * V1 is just a series of values; first are mandatories; last are an
 * ellipsis: variable number of arguments to command. The only
 * difficulty is that the values are not taken as is but have to be
 * split and interpreted.
 */
static int
parse_srv_v1(void)
{
	int	status;
	int	i;

	if ( (status = parse_socktype()) != CFG_OK )
		return status;
	if (tsep.se_socktype == SOCK_STREAM) {
		if (sap_split(':', 1) == 1) {
			if (parse_accept_filter() < 0)
				return CFG_ECFG;
		}
	}

	/* protocol */
	if (eof_stmt)
		return CFG_EMISSING_ARGS;
	else if (has_assign || has_separator)
		return CFG_ECFG;

	load_value();
	if (stmt_state != STMT_OK)
		return CFG_ECFG;

	/*
	 * On top of stack must now be the protocol.
	 */
	if (parse_protocol(true) != 0)
		return CFG_ECFG;

	/* wait/nowait:max */
	if (eof_stmt)
		return CFG_EMISSING_ARGS;
	else if (has_assign || has_separator)
		return CFG_ECFG;
	load_value();
	if (stmt_state != STMT_OK)
		return CFG_ECFG;

	/* Rate limiting parsing */
	if (sap_split(':', 1) == 1) {
		unsigned long u;
		sap_store(SAP_STORE_INT | SAP_STORE_FMT_UNSIGNED
			| SAP_STORE_FMT_LONG, &u, 0, SERVTAB_COUNT_MAX);
		if (sap_state != SAP_OK)
			return -1;
		tsep.se_service_max= (size_t) u;
	}

	tsep.se_wait = (strcmp(SAP_CURSTR, "wait") == 0)? 1 : 0;
	sap_pop();

	/* Parse user:group token */
	if (eof_stmt)
		return CFG_EMISSING_ARGS;
	else if (has_assign || has_separator)
		return CFG_ECFG;
	load_value();
	if (stmt_state != STMT_OK)
		return CFG_ECFG;

	if ( sap_split(':', 1) == 1
		|| sap_split('.', 1) == 1 /* compatibility */ ) {
		tsep.se_group = SAP_CURSTR;
		sap_join('\0', 1);
		tsep.se_user = SAP_CURSTR;
	} else
		tsep.se_user = SAP_CURSTR;
		
	/* Parse server-program (path to binary or "internal") */
	if (eof_stmt)
		return CFG_EMISSING_ARGS;
	else if (has_assign || has_separator)
		return CFG_ECFG;
	load_value();
	if (stmt_state != STMT_OK)
		return CFG_ECFG;

	tsep.se_server = SAP_CURSTR;

	/* The ellipsis. */
	i = 0;
	while (!eof_stmt &&  i < MAXARGV) {	
		if (has_assign || has_separator)
			return CFG_ECFG;
		load_value();
		if (stmt_state != STMT_OK)
			return CFG_ECFG;
		tsep.se_argv[i++] = SAP_CURSTR;
	}
	if (!eof_stmt)
		return CFG_ETOO_MANY_ARGS;

	while (i <= MAXARGV)
		tsep.se_argv[i++] = NULL;

	return CFG_OK;
}

/*
 * V2 syntax is (keyword, operator, [values]) series, comma separated.
 * We have switched because of on/off already read. So we start after.
 */
#define	V2_BIND		1
#define	V2_SOCKTYPE	2
#define	V2_ACCEPTFILTER	3
#define	V2_PROTOCOL	4
#define	V2_SNDBUF	5
#define	V2_RCVBUF	6
#define	V2_WAIT		7
#define	V2_SERVICE_MAX	8
#define	V2_IP_MAX	9
#define	V2_USER		10
#define	V2_GROUP	11
#define	V2_EXEC		12
#define	V2_ARGS		13
#ifdef IPSEC
#define	V2_IPSEC	14
#endif
static struct keystring v2_ks[] = {
	{ "bind", 4, V2_BIND, 1, 1 },
	{ "socktype", 8, V2_SOCKTYPE, 1, 1 },
	{ "acceptfilter", 12, V2_ACCEPTFILTER, 1, 1 },
	{ "protocol", 8, V2_PROTOCOL, 1, 1 },
	{ "sndbuf", 6, V2_SNDBUF, 1, 1 },
	{ "rcvbuf", 6, V2_RCVBUF, 1, 1 },
	{ "recvbuf", 7, V2_RCVBUF, 1, 1 },	/* compatibility */
	{ "wait", 4, V2_WAIT, 1, 1 },
	{ "service_max", 11, V2_SERVICE_MAX, 1, 1 },
	{ "ip_max", 6, V2_IP_MAX, 1, 1 },
	{ "user", 4, V2_USER, 1, 1 },
	{ "group", 5, V2_GROUP, 1, 1 },
	{ "exec", 4, V2_EXEC, 1, 1 },
	{ "args", 4, V2_ARGS, 1, MAXARGV },
#ifdef IPSEC
	{ "ipsec", 5, V2_IPSEC, 0, MAXARGV },
#endif
	{ NULL, 0, INVALID_KS, 0, 0 }
};

static int
parse_srv_v2(void)
{
	int		status;
	int		ikw, argc;
	unsigned long	u;

	while (!eof_stmt) {
		get_keyword();
		if (stmt_state != STMT_OK || !has_assign)
			return CFG_ECFG;
		for (ikw = 0; v2_ks[ikw].ks != NULL	
			&& ( toklen != (int) v2_ks[ikw].len
				|| (strncmp(TOKSTR, v2_ks[ikw].ks,
					(size_t) toklen) != 0) );
			 ++ikw)
			;
		if (v2_ks[ikw].min_argc == 1) {
			load_value();
			if (stmt_state != STMT_OK)
				return CFG_ECFG;
			if ( v2_ks[ikw].max_argc == 1
				&& !eof_stmt && !has_separator )
					return CFG_ECFG;
		}
		
		switch (v2_ks[ikw].val) {
		case V2_BIND:
			if (tsep.se_hostaddr != NULL) {
				DPRINTF("Specifying both hostaddr and bind");
				return CFG_ECFG;
			}
			tsep.se_hostaddr = SAP_CURSTR;
			break;
		case V2_SOCKTYPE:
			if ( (status = parse_socktype()) != CFG_OK )
				return status;
			break;
		case V2_ACCEPTFILTER:
			argc = 0;	/* first is pushed */
			while ( !eof_stmt && !has_separator
				&& argc < MAXARGV ) {
				load_value();
				if (stmt_state != STMT_OK)
					return CFG_ECFG;
				++argc;
			}
			if ( argc == MAXARGV && !eof_stmt &&
				!has_separator )
				return CFG_ETOO_MANY_ARGS;
			sap_join(',', argc-1);
			if (parse_accept_filter() < 0)
				return CFG_ECFG;
			break;
		case V2_PROTOCOL:
				if (parse_protocol(false) != 0)
					return CFG_ECFG;
			break;
		case V2_SNDBUF:
			sap_store(SAP_STORE_INT | SAP_STORE_FMT_USER,
				&tsep.se_sndbuf);
			if ( sap_state != SAP_OK
				|| tsep.se_sndbuf < 1 )
				return CFG_ECFG;
			break;
		case V2_RCVBUF:
			sap_store(SAP_STORE_INT | SAP_STORE_FMT_USER,
				&tsep.se_rcvbuf);
			if ( sap_state != SAP_OK
				|| tsep.se_rcvbuf < 1 )
				return CFG_ECFG;
			break;
		case V2_WAIT:
			if (strcmp(SAP_CURSTR, "yes") == 0)
				tsep.se_wait = 1;
			else if (strcmp(SAP_CURSTR, "no") == 0)
				tsep.se_wait = 0;
			else
				return CFG_ECFG;
			sap_pop();
			break;
		case V2_SERVICE_MAX:
			sap_store(SAP_STORE_INT | SAP_STORE_FMT_UNSIGNED
				| SAP_STORE_FMT_LONG, &u,
				0, SERVTAB_COUNT_MAX);
			if (sap_state != SAP_OK)
				return CFG_ECFG;
			tsep.se_service_max= (size_t) u;
			break;
		case V2_IP_MAX:
			sap_store(SAP_STORE_INT | SAP_STORE_FMT_UNSIGNED
				| SAP_STORE_FMT_LONG, &u,
				0, SERVTAB_COUNT_MAX);
			if (sap_state != SAP_OK)
				return CFG_ECFG;
			tsep.se_ip_max= (size_t) u;
			break;
		case V2_USER:
			tsep.se_user = SAP_CURSTR;
			break;
		case V2_GROUP:
			tsep.se_group = SAP_CURSTR;
			break;
		case V2_EXEC:
			tsep.se_server = SAP_CURSTR;
			break;
		case V2_ARGS:
			argc = 0;	/* first is pushed */
			tsep.se_argv[argc++] = SAP_CURSTR;
			while ( !eof_stmt && !has_separator
				&& argc < MAXARGV ) {
				load_value();
				if (stmt_state != STMT_OK)
					return CFG_ECFG;
				tsep.se_argv[argc++] = SAP_CURSTR;
			}
			if ( argc == MAXARGV && !eof_stmt &&
				!has_separator )
				return CFG_ETOO_MANY_ARGS;
			while (argc <= MAXARGV)
				tsep.se_argv[argc++] = NULL;
			break;
#ifdef IPSEC
		case V2_IPSEC:
			if (ipsec_get_ck() < 0)
				return CFG_EIPSEC;
			tsep.se_ipsec = SAP_CURSTR;
			break;
#endif
		default:
			return CFG_ECFG;
			break;
		}
	}

	return CFG_OK;
}

/*
 * Sets sep->se_checked to true if everything is alright.
 * There is no allocation. Allocations are only done if everything is
 * OK, later.
 */
static int
tsep_ck(void)
{
	size_t n;

	tsep.se_checked = false;

	/*
	 * These are faults for v2 srv definitions.
	 */
	if ( tsep.se_user == NULL || tsep.se_proto == NULL
		|| tsep.se_socktype == SERVTAB_UNSPEC_VAL ) {
		DPRINTF("Mandatory user '%s', proto '%s',"
			"socktype '%d' not set",
			tsep.se_user, tsep.se_proto,
			tsep.se_socktype);
		return CFG_ESRV;
	}

	if (tsep.se_service_max > SERVTAB_SERVICE_MAX)
		tsep.se_service_max = SERVTAB_SERVICE_MAX;

	if (parse_server() != 0 || wait_ck() != 0)
		return CFG_ESRV;

	/* buf sizes not meaningful for Tcpmux services. */
	if ( ISMUX(&tsep) && (tsep.se_sndbuf != 0 || tsep.se_rcvbuf != 0) ) {
		DPRINTF("%s: can't specify buffer sizes for "
			    "tcpmux services", tsep.se_service);
		return CFG_ESRV;
	}


	switch (tsep.se_family) {
	case AF_LOCAL:
		assert(tsep.se_fd == -1);
		n = strlen(tsep.se_service);
		if (n >= sizeof(tsep.se_ctrladdr_un.sun_path)) {
			DPRINTF("%s/%s: address too long",
			    tsep.se_service, tsep.se_proto);
			break;
		}
		strlcpy(tsep.se_ctrladdr_un.sun_path,
		    tsep.se_service, n + 1);
		tsep.se_ctrladdr_un.sun_family = AF_LOCAL;
		tsep.se_ctrladdr_size = (socklen_t)(n +
		    sizeof(tsep.se_ctrladdr_un) -
		    sizeof(tsep.se_ctrladdr_un.sun_path));
		tsep.se_checked = true;
		break;
	case AF_INET:
#ifdef INET6
	case AF_INET6:
#endif
	    {
		struct addrinfo hints, *res;
		char *host;
		const char *port;
		int error;
		int s;

		if (strcmp(tsep.se_hostaddr, defhost_default) == 0)
			host = NULL;
		else
			host = tsep.se_hostaddr;

		/* check if the family is supported */
		s = socket(tsep.se_family, SOCK_DGRAM, 0);
		if (s < 0) {
			DPRINTF("%s/%s: %s: the address family is not "
			    "supported by the kernel",
			    tsep.se_service, tsep.se_proto, host);
			break;
		}
		close(s);

		memset(&hints, 0, sizeof(hints));
		hints.ai_family = tsep.se_family;
		hints.ai_socktype = tsep.se_socktype;
		hints.ai_flags = AI_PASSIVE;
		if (isrpcservice(&tsep) || ISMUX(&tsep))
			port = "0";
		else
			port = tsep.se_service;
		error = getaddrinfo(host, port, &hints, &res);
		if (error != 0) {
			if (error == EAI_SERVICE) {
				/* gai_strerror not friendly enough */
				DPRINTF(SERV_FMT "unknown service", SERV_PARAMS(&tsep));
			} else {
				DPRINTF(SERV_FMT ": %s: %s",
				    SERV_PARAMS(&tsep), host,
					gai_strerror(error));
			}
			break;
		}
		if (res->ai_next != NULL) {
			DPRINTF(SERV_FMT
			    ": %s: resolved to multiple addr",
			    SERV_PARAMS(&tsep), host);
			freeaddrinfo(res);
			break;
		}
		memcpy(&tsep.se_ctrladdr, res->ai_addr,
			res->ai_addrlen);
		if (ISMUX(&tsep)) {
			tsep.se_fd = -1;
			freeaddrinfo(res);
			tsep.se_checked = true;
			break;
		}
		tsep.se_ctrladdr_size = res->ai_addrlen;
		freeaddrinfo(res);
#ifdef RPC
		if (isrpcservice(&tsep)) {
			struct rpcent *rp;

			tsep.se_rpcprog = atoi(tsep.se_service);
			if (tsep.se_rpcprog == 0) {
				rp = getrpcbyname(tsep.se_service);
				if (rp == 0) {
					DPRINTF(SERV_FMT "unknown service",
						SERV_PARAMS(&tsep));
					break;
				}
				tsep.se_rpcprog = rp->r_number;
			}
		}
#endif /* RPC */
			tsep.se_checked = true;
		}
	}

	if (tsep.se_checked)
		return CFG_OK;
	else
		return CFG_ESRV;
}

static void
tsep_defaults(void)
{
	if (tsep.se_hostaddr == NULL) {
		sap_push((defhost == NULL)? defhost_default : defhost,
			-1, 0);
		tsep.se_hostaddr = SAP_CURSTR;
	}

	if (tsep.se_server == NULL) { 	/* Will try builtin */
		sap_push("internal", -1, 0);
		tsep.se_server = SAP_CURSTR;
	}

	/*
	 * Try to infer the socktype.
	 */
	if ( tsep.se_socktype == SERVTAB_UNSPEC_VAL
		&& tsep.se_proto != NULL ) {
		if (strncmp(tsep.se_proto, "udp", 3) == 0)
			tsep.se_socktype = SOCK_DGRAM;
		else if (strncmp(tsep.se_proto, "tcp", 3) == 0)
			tsep.se_socktype = SOCK_STREAM;
	}

#ifdef IPSEC
	/* An empty string means: don't put default */
	if (tsep.se_ipsec == NULL && defipsec != NULL) {
		sap_push(defipsec, -1, 0);
		tsep.se_ipsec = SAP_CURSTR;
	}
#endif
}

/*
 * The allocations are all in the sap stack. We join all the fields
 * with '\0' and then store the whole record and fix the relative
 * addresses.
 */
static int
tsep_store(void)
{
	int i;

	sap_join('\0', -1);

	sap_store(SAP_STORE_STRING, &tsep.se_allocated);

	if (sap_state == SAP_ESTORE)
		return CFG_EMEM;

	tsep.se_hostaddr += tsep.se_allocated - sap_stack;
	tsep.se_service += tsep.se_allocated - sap_stack;
	tsep.se_proto += tsep.se_allocated - sap_stack;
	tsep.se_user += tsep.se_allocated - sap_stack;
	if (tsep.se_group != NULL)	/* can be NULL */
		tsep.se_group += tsep.se_allocated - sap_stack;
	tsep.se_server += tsep.se_allocated - sap_stack;
	for (i = 0; tsep.se_argv[i] != NULL && i < MAXARGV; i++)
		tsep.se_argv[i] += tsep.se_allocated - sap_stack;
#ifdef IPSEC
	if (tsep.se_ipsec != NULL)
		tsep.se_ipsec += tsep.se_allocated - sap_stack;
#endif

	return CFG_OK;
}

/*
 * This routine releases allocated fields and redefines them to NULL. It
 * doesn't touch non allocated fields (i.e. .se_next for example).
 * It can be called with a NULL (no-op).
 * The rate_limit ip list is supposed to be cleared by the execution
 * (inetd.c).
 */
void
sep_release(struct servtab *sep)
{
	int i;

	if (sep == NULL) return;

	free(sep->se_allocated);
	sep->se_allocated = NULL;
	sep->se_hostaddr = NULL;
	sep->se_service = NULL;
	sep->se_proto = NULL;
	sep->se_user = NULL;
	sep->se_group = NULL;
	sep->se_server = NULL;
	for (i = 0; i < MAXARGV; i++) 
		sep->se_argv[i] = NULL;
#ifdef IPSEC
	sep->se_ipsec = NULL;
#endif
}

/*
 * Dropping all allocations of a linked list of servtab.
 */
void
cfg_drop(struct servtab *cfg)
{
	struct servtab *sep;

	for (sep = cfg; sep != NULL;) {
		cfg = sep->se_next;
		sep_release(sep);
		free(sep);
		sep = cfg;
	}
}

/*
 * V1 and V2 parsing shared routines. [TL]
 */
struct keystring sock_ks[] = {
	{"stream", 6, SOCK_STREAM, 0, 0}, /* may have accept filter */
	{"dgram", 5, SOCK_DGRAM, 0, 0},
	{"raw", 3, SOCK_RAW, 0, 0},
	{"rdm", 3, SOCK_RDM, 0, 0},
	{"seqpacket", 9, SOCK_SEQPACKET, 0, 0},
	/*	{"conn_dgram", 10, SOCK_CONN_DGRAM, 0, 0}, */	/* unsupported */
	{NULL, 0, -1, 0, 0}
};

static int
parse_socktype(void)
{
	int i;

	for (i = 0; sock_ks[i].ks != NULL; i++) {
		if (strncmp(SAP_CURSTR, sock_ks[i].ks, sock_ks[i].len) == 0)
			break;
	}
	if ( sock_ks[i].val == -1 ||
		 ( sock_ks[i].val != SOCK_STREAM
			 && SAP_CURLEN != (int) sock_ks[i].len ) ) {
		stmt_state = STMT_EVALUE;
		return CFG_ECFG;
	}
	tsep.se_socktype = sock_ks[i].val;

	return CFG_OK;
}

static bool
is_same_service(const struct servtab *sep, const struct servtab *cp)
{
	return
	    strcmp(sep->se_service, cp->se_service) == 0 &&
	    strcmp((sep->se_hostaddr == NULL)? defhost : sep->se_hostaddr,
			cp->se_hostaddr) == 0 &&
	    strcmp(sep->se_proto, cp->se_proto) == 0 &&
	    sep->se_family == cp->se_family &&
	    ISMUX(sep) == ISMUX(cp);
}

static int
parse_protocol(bool is_v1)
{
	if (tsep.se_type == NORM_TYPE &&
	    strncmp(SAP_CURSTR, "faith/", strlen("faith/")) == 0) {
		sap_push(SAP_CURSTR+(int)strlen("faith/"), -1, 0);
		sap_swap(); sap_pop();
		tsep.se_type = FAITH_TYPE;
	}

	if (is_v1) {
		/*
		 * Extract the send and receive buffer sizes before parsing
		 * the protocol.
		 */
		int i;

		i = 1;
		while (sap_split(',', 1) == 1 && i <= 2) {
			int *val;
	
			if (sap_split('=', 1) == 0)
				return CFG_ECFG;
			if (strcmp(sap_stack+sap[sap_nstr-2].istr, "sndbuf")
				== 0)
				val = &tsep.se_sndbuf;
			else if (strcmp(sap_stack+sap[sap_nstr-2].istr, "rcvbuf")
				== 0)
				val = &tsep.se_rcvbuf;
			else
				return CFG_ECFG;
			sap_store(SAP_STORE_INT | SAP_STORE_FMT_USER, val);
			if (*val < 1)
				return CFG_ECFG;
			sap_pop();
			++i;
		} 
	}

	if (SAP_CURLEN == 0)
		return -1;	/* invalid protocol */
	if (strcmp(SAP_CURSTR, "unix") == 0) {
		tsep.se_family = AF_LOCAL;
	} else {
		int	ver;

		ver = SAP_CURSTR[SAP_CURLEN-1];
		switch (ver) {
		case '4':	/*tcp4 or udp4*/
			tsep.se_family = AF_INET;
			break;
#ifdef INET6
		case '6':	/*tcp6 or udp6*/
			tsep.se_family = AF_INET6;
			break;
#endif
		default:
			/*
			 * Use 'default' IP version which is IPv4, may
			 * eventually be changed to AF_INET6
			 */
			tsep.se_family = AF_INET;
			break;
		}
		if (strncmp(SAP_CURSTR, "rpc/", 4) == 0) {
#ifdef RPC
			if (sap_split('/', 1) != 1)
				return -1;
			/* perhaps range : low '-' high */
			if (sap_split('-', 1) == 1) {
				sap_store(SAP_STORE_INT, &tsep.se_rpcversh);
				if (sap_state != SAP_OK)
					return -1;
				sap_store(SAP_STORE_INT, &tsep.se_rpcversl);
				if (sap_state != SAP_OK)
					return -1;
			} else {
				sap_store(SAP_STORE_INT, &tsep.se_rpcversl);
				if (sap_state != SAP_OK)
					return -1;
				tsep.se_rpcversh = tsep.se_rpcversl;
			}
#else
			DPRINTF("%s: rpc services not supported",
			    SAP_CURSTR);
			return -1;
#endif /* RPC */
		}
	}

	tsep.se_proto = SAP_CURSTR;

	return 0;
}

static int
wait_ck(void)
{
	if (!ISMUX(&tsep))
		return 0;

	/*
	 * Silently enforce "nowait" for TCPMUX services since
	 * they don't have an assigned port to listen on.
	 */
	tsep.se_wait = 0;

	if (strncmp(tsep.se_proto, "tcp", 3)) {
		DPRINTF("bad protocol for tcpmux service %s",
			tsep.se_service);
		return -1;
	}
	if (tsep.se_socktype != SOCK_STREAM) {
		DPRINTF("bad socket type for tcpmux service %s",
		    tsep.se_service);
		return -1;
	}
	return 0;
}

static int
parse_server(void)
{
	if (strcmp(SAP_CURSTR, "internal") != 0) {
		tsep.se_bi = NULL;
		return 0;
	}
	
	if (!try_biltin(&tsep)) {
		DPRINTF("Internal service %s unknown", tsep.se_service);
		return -1;
	}
	return 0;
}

/* TODO test to make sure accept filter still works */
static int
parse_accept_filter(void)
{
	if (SAP_CURLEN == 0 || strrchr(SAP_CURSTR, ':') != NULL) {
		/* more than one or nothing */
		tsep.se_socktype = SERVTAB_UNSPEC_VAL;
		return -1;
	}

	if (sap_split(',', 1) == 1) {
		if (sap_split(',', 1) == 1) 	/* more than one: no */
			return -1;
		if (SAP_CURLEN >= (int) sizeof(tsep.se_accf.af_arg))
			return -1;
		strlcpy(tsep.se_accf.af_arg, SAP_CURSTR,
		    sizeof(tsep.se_accf.af_arg));
		sap_pop();
	}

	if (SAP_CURLEN >= (int) sizeof(tsep.se_accf.af_name))
		return -1;
	strlcpy(tsep.se_accf.af_name, SAP_CURSTR,
		sizeof(tsep.se_accf.af_name));
	sap_pop();

	return 0;
}

#ifdef IPSEC
/*
 * For compatibility, an ipsec policy can be passed as a ';' separated
 * directives string (value).
 * We will in fact validate a whole policy and convert it to a
 * (minimal) canonical form and this is this canonical policy that will
 * be saved.
 * This function has to be called with no string already loaded. The
 * ipsec may be the empty string.
 */
static int
ipsec_get_ck(void)
{
	int argc;

	argc = 0;
	do {
		load_value();
		if (stmt_state != STMT_OK)
			return CFG_ECFG;
		++argc;
	} while (!eof_stmt && !has_separator && argc < MAXARGV);

	if (argc == MAXARGV && !eof_stmt && !has_separator)
		return CFG_ETOO_MANY_ARGS;

	sap_join(';', argc-1);

	if (SAP_CURLEN == 0)
		return CFG_OK;
	else
		return ipsec_ck();
}
#endif

/*
 * This procedure can not be used during parsing---it would be useless:
 * whether the parsing succeeds, and this will be printed, or the
 * parsing fails and the relevant infos are in the buffers---because it
 * trashes values[] to compose strings to pass to my_syslog().
 * The size of values[] is the double of the size of stmt in order to
 * accommodate all the strings in the sep, meaning both what was passed
 * in the srv definition, and additional provided defaults: host or
 * ipsec policy. So we know that we will be able to print the sep.
 * The sap_push() comes here handy, because we don't have to care for
 * sizes. We push all the strings, and when the size of the stack is
 * > 72, we rewrite the intermediary null chars by a comma, since
 * except for the beginning of the string, all other strings have a
 * leading blank (a space); this will also do for continuation lines. 
 */
void
sep_print(int prio, struct servtab *sep)
{
	int	i;
	char	*p;

	sap_reset();
	sap_push(sep->se_hostaddr, -1, 0);
	sap_push(":", -1, SAP_PUSH_APPEND);
	switch (sep->se_type) {
	case MUX_TYPE:
		sap_push(TCPMUX_TOKEN, -1, SAP_PUSH_APPEND);
		break;
	case MUXPLUS_TYPE:
		sap_push(TCPMUX_TOKEN, -1, SAP_PUSH_APPEND);
		break;
	case FAITH_TYPE:
		sap_push("faith/", -1, SAP_PUSH_APPEND);
		break;
	default:
		break;
	}
	sap_push(sep->se_service, -1, SAP_PUSH_APPEND);
	if (sep->se_checked)
		sap_push(" on", -1, SAP_PUSH_APPEND);
	else
		sap_push(" off", -1, SAP_PUSH_APPEND);
	
	for (i = 0; sock_ks[i].ks != NULL
		&& sock_ks[i].val != sep->se_socktype; ++i)
		;
	sap_push(" socktype=", -1, SAP_PUSH_APPEND);
	sap_push(sock_ks[i].ks, -1, SAP_PUSH_APPEND);
	if (sep->se_accf.af_name[0] != '\0') {
		sap_push(", acceptfilter=", -1, SAP_PUSH_APPEND);
		sap_push(sep->se_accf.af_name, -1,
			SAP_PUSH_APPEND | SAP_PUSH_QUOTE);
	}

	sap_push(", protocol=", -1, SAP_PUSH_APPEND);
	sap_push(sep->se_proto, -1, SAP_PUSH_APPEND);

	/*
	 * For the buffers, 0 is unspecified.
	 */
	if (sep->se_sndbuf != 0) {
		sap_push(", sndbuf=", -1, SAP_PUSH_APPEND);
		(void) asprintf(&p, "%d", sep->se_sndbuf);
		sap_push(p, -1, SAP_PUSH_APPEND);
		free(p);
	}

	if (sep->se_rcvbuf != 0) {
		sap_push(", rcvbuf=", -1, SAP_PUSH_APPEND);
		(void) asprintf(&p, "%d", sep->se_rcvbuf);
		sap_push(p, -1, SAP_PUSH_APPEND);
		free(p);
	}

	sap_push(", wait=", -1, SAP_PUSH_APPEND);
	sap_push((sep->se_wait? "yes," : "no,"), -1 , SAP_PUSH_APPEND);

	/* publish first part/line */
	if (sap_state != SAP_OK) return;
	my_syslog(prio, "%s", sap_stack);
	sap_reset();


	sap_push("\tservice_max=", -1, 0);
	(void) asprintf(&p, "%zu", sep->se_service_max);
	sap_push(p, -1, SAP_PUSH_APPEND);
	free(p);

	if (sep->se_ip_max != SERVTAB_UNSPEC_SIZE_T) {
		sap_push(", ip_max=", -1, SAP_PUSH_APPEND);
		(void) asprintf(&p, "%zu", sep->se_ip_max);
		sap_push(p, -1, SAP_PUSH_APPEND);
		free(p);
	}

	sap_push(", user=", -1, SAP_PUSH_APPEND);
	sap_push(sep->se_user, -1, SAP_PUSH_APPEND);

	if (sep->se_group != NULL) {
		sap_push(", group=", -1, SAP_PUSH_APPEND);
		sap_push(sep->se_group, -1, SAP_PUSH_APPEND);
	}

	/* publish second part/line */
	sap_push(",", -1, SAP_PUSH_APPEND);
	if (sap_state != SAP_OK) return;
	my_syslog(prio, "%s", sap_stack);
	sap_reset();

	sap_push("\texec=", -1, 0);
	sap_push(sep->se_server, -1, SAP_PUSH_APPEND);

	if (sep->se_argv[0] != NULL) {
		sap_push(", args=", -1, SAP_PUSH_APPEND);
		for (i = 0; sep->se_argv[i] != NULL && i < MAXARGV; i++) {
			sap_push(sep->se_argv[i], -1, SAP_PUSH_QUOTE);
			sap_join(' ', 1);
		}	
	}

#ifdef IPSEC
	if (sep->se_ipsec != NULL) {
		sap_push(",", -1, SAP_PUSH_APPEND);
		my_syslog(prio, "%s", sap_stack);
		sap_reset();
		sap_push("\tipsec=", -1, 0);
		sap_push(sep->se_ipsec, -1,
			SAP_PUSH_APPEND | SAP_PUSH_QUOTE);
	}
#endif
	sap_push(";", -1, SAP_PUSH_APPEND);
	my_syslog(prio, "%s", sap_stack);
	sap_reset();
}

void
cfg_print(struct servtab *sep)
{
	my_syslog(LOG_INFO, "# CONFIG_BEGIN");
	for (; sep != NULL; sep = sep->se_next)
		sep_print(LOG_INFO, sep);
	my_syslog(LOG_INFO, "# CONFIG_END");
}

