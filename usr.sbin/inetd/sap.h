/*	$Id: sap.h,v 1.3 2023/07/03 16:09:47 tlaronde Exp $	*/

/*-
 * Copyright (c) 2023 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Thierry Laronde.
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

#ifndef _SAP_H
#define _SAP_H

extern int	sap_state;
#define SAP_OK		0
#define SAP_EJOIN	1
#define SAP_EMERGE	2
#define SAP_ENONSENSE	3
#define SAP_EOVERFLOW	4
#define SAP_EPUSH	5
#define SAP_EREPLACE	6
#define SAP_ESPLIT	7
#define SAP_ESTORE	8
#define SAP_ESWAP	9
#define SAP_ETOOLONG	10

extern char	sap_stack[];

#define SAP_NSTR_MAX	100	/* more than MAXARGV + NFIELDS */

struct sap_str {
	int	istr;	/* index in sap_stack[] to beginning of value */
	int	len;	/* length of this string in sap_stack[] */
};
extern struct sap_str	sap[];
extern int		sap_nstr;

/*
 * These shall be used only when there is something on the stack!
 */
#define SAP_ISTR(i)	(sap[i].istr)
#define SAP_LEN(i)	(sap[i].len)
#define SAP_STR(i)	(sap_stack+SAP_ISTR(i))
#define SAP_CURISTR	SAP_ISTR(sap_nstr-1)
#define SAP_CURLEN	SAP_LEN(sap_nstr-1)
#define SAP_CURSTR	SAP_STR(sap_nstr-1)

#define SAP_SZ		(sap_nstr? SAP_CURISTR + SAP_CURLEN + 1 : 0)

void	sap_clear_state(void);
void	sap_dup(void);
void	sap_join(char, int);
void	sap_merge(void);
void	sap_pop(void);

#define SAP_PUSH_NEW		0x00	/* push new string */
#define SAP_PUSH_APPEND		0x01	/* append to last string */
#define SAP_PUSH_UNESCAPED	0x00	/* convert escape seq. default */
#define SAP_PUSH_UNQUOTE	0x10	/* unquote src */
#define SAP_PUSH_QUOTE		0x20	/* quote dst; implies unquote src*/
#define SAP_PUSH_AS_IS		0x40	/* don't unescape */
void	sap_push(const char *, int, int);

void	sap_replace(char, char);
void	sap_reset(void);
int	sap_split(char, int);

#define SAP_STORE_STRING	0x00
#define SAP_STORE_INT		0x01
#define SAP_STORE_FILE		0x02
#define SAP_STORE_FMT_UNSIGNED	0x10
#define SAP_STORE_FMT_LONG	0x20
#define SAP_STORE_FMT_BOUNDED	0x40
#define SAP_STORE_FMT_USER	0x80	/* subtype: 'k', 'm', 'g' suffixes */
void	sap_store(int, void *, ...);

void	sap_swap(void);

#endif
