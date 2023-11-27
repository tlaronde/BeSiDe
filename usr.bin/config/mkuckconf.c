/*	$NetBSD$	*/

/*
 *  Copyright (c) 2023 The NetBSD Foundation.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 *  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 *  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#include <err.h>
#include <stdio.h>
#include "defs.h"

/*
 * Make uckconf.c.
 */
char **uc_cmd;		/* array of userconf cmd lines */
unsigned int uc_ncmd;	/* cardinal of such cmd lines present */

int
mkuckconf(void)
{
	FILE *fp;
	unsigned int i;

	if ((fp = fopen("uckconf.c.tmp", "w")) == NULL) {
		warn("cannot write uckconf.c.tmp");
		return 1;
	}

	(void)fprintf(fp, "/* config(1) generated userconf instructions */\n");
	(void)fprintf(fp, "#include <sys/null.h>\n");
	(void)fprintf(fp, "\nconst char *const userconf_kconf[] = {\n");
	for (i = 0; i < uc_ncmd; i++)
		(void)fprintf(fp, "\t\"%s\",\n", uc_cmd[i]);

	(void)fprintf(fp, "\tNULL\n};\n");

	fflush(fp);
	if (ferror(fp)) {
		warn("error writing uckconf.c");
		return 1;
	}

	(void)fclose(fp);
	if (moveifchanged("uckconf.c.tmp", "uckconf.c") != 0) {
		warn("error renaming uckconf.c");
		return 1;
	}

	return 0;
}

