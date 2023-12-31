/*	$NetBSD: misc.c,v 1.16 2023/08/10 20:36:28 mrg Exp $	*/

/*-
 * Copyright (c) 1990, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Cimarron D. Taylor of the University of California, Berkeley.
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
static char sccsid[] = "from: @(#)misc.c	8.2 (Berkeley) 4/1/94";
#else
__RCSID("$NetBSD: misc.c,v 1.16 2023/08/10 20:36:28 mrg Exp $");
#endif
#endif /* not lint */

#include <sys/types.h>
#include <sys/stat.h>

#include <err.h>
#include <errno.h>
#include <fts.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <paths.h>
#include <fcntl.h>

#include "find.h"

/*
 * brace_subst --
 *	Replace occurrences of {} in orig with path, and place it in a malloced
 *      area of memory set in store.
 */
void
brace_subst(char *orig, char **store, char *path, size_t *len)
{
	size_t nlen, plen, rest;
	char ch, *p, *ostore;

	plen = strlen(path);
	for (p = *store; (ch = *orig) != '\0'; ++orig)
		if (ch == '{' && orig[1] == '}') {
			/* Length of string after the {}. */
			rest = strlen(&orig[2]);

			nlen = *len;
			while ((p - *store) + plen + rest + 1 > nlen)
				nlen *= 2;

			if (nlen > *len) {
				ptrdiff_t off = p - *store;

				ostore = *store;
				if ((*store = realloc(ostore, nlen)) == NULL)
					err(1, "realloc");
				*len = nlen;
				p = *store + off;	/* Relocate. */
			}
			memmove(p, path, plen);
			p += plen;
			++orig;
		} else
			*p++ = ch;
	*p = '\0';
}

/*
 * queryuser --
 *	print a message to standard error and then read input from standard
 *	input. If the input is 'y' then 1 is returned.
 */
int
queryuser(char **argv)
{
	int ch, first, nl;

	(void)fprintf(stderr, "\"%s", *argv);
	while (*++argv)
		(void)fprintf(stderr, " %s", *argv);
	(void)fprintf(stderr, "\"? ");
	(void)fflush(stderr);

	first = ch = getchar();
	for (nl = 0;;) {
		if (ch == '\n') {
			nl = 1;
			break;
		}
		if (ch == EOF)
			break;
		ch = getchar();
	}

	if (!nl) {
		(void)fprintf(stderr, "\n");
		(void)fflush(stderr);
	}
        return (first == 'y');
}

/*
 * show_path --
 *	called on SIGINFO
 */
/* ARGSUSED */
void
show_path(int sig)
{
	extern FTSENT *g_entry;
	static int ttyfd = -2;
	char buf[2048];
	int oerrno, n;

	if (g_entry == NULL) {
		/*
		 * not initialized yet.
		 * assumption: pointer assignment is atomic.
		 */
		return;
	}

	oerrno = errno;
	if (ttyfd == -2)
		ttyfd = open(_PATH_TTY, O_RDWR | O_CLOEXEC);

	if (ttyfd == -1)
		goto out;

	n = snprintf_ss(buf, sizeof(buf), "%s: path %s\n", getprogname(), 
	    g_entry->fts_path);

	if (n <= 0)
		goto out;

	write(ttyfd, buf, (size_t)n);
out:
	errno = oerrno;
}
