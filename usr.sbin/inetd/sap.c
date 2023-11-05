/*	$Id: sap.c,v 1.6 2023/07/03 16:09:47 tlaronde Exp $	*/

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

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$Id: sap.c,v 1.6 2023/07/03 16:09:47 tlaronde Exp $");
#endif /* not lint */

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "sap.h"

static int	hex2char(const char *, char *);
static int	s2i(int, void *, ...);

/*
 * Here is a String Auxiliary Processor, making it a... sap!
 *
 * The idea was simple: to be able to manipulate variable strings---i.e.
 * "values" and not keywords or fixed chars in a syntax---without having
 * to deal, during manipulations, with allocations and checking of
 * length, and to be able to interpret strings (integers etc.) and
 * to keep only pure strings.
 *
 * This is a simple processor that handles only pushing a new string
 * on the stack; storing a string i.e. converting a string to a binary
 * value or allocating memory for a string and then pop'ing off the
 * string; or manipulating strings on the stack.
 *
 * The manipulations are only unary---the last string on stack---or
 * binary---the last string on stack and the previous one.
 *
 * We "push" on the stack and not "load" because the string can come
 * from whatever so it is an external call that pushes. But we store
 * because we do manipulate, internally, the representation of the
 * string on the stack.
 *
 * The... sap, is _string_ oriented, does know only the C locale and
 * considers strings with the following conventions, a mix between
 * POSIX.2 sh(1) conventions about single quoting and double quoting,
 * C definitions for escape sequences and POSIX.2 printf(1)
 * convetions: 
 *	- Single quoted strings are taken as is, without interpretation;
 *	- Outside single quoting, escape sequences are interpreted,
 *	for C special escape sequences, including hex and octal escape
 *	sequences. This works too inside of double quoted strings
 *	(contrary to POSIX.2 sh(1) defined behavior).
 */

/*
 * The stack is a fixed size buffer. Since we must be able to manipulate
 * two strings, the stack is twice the max size of a line. This is
 * compatible with the stmt[] size in inetd:parse.c
 * The very first byte (char) of the stack is always '\0'. See the
 * the definition of SAP_EMPTY_STRING in sap.h. But we do not use
 * this empty string internally: an empty string pushed will have its
 * place in the stack so that a caller has not to take care of special
 * cases when manipulating the strings (there are as many strings, maybe
 * empty, that have been successfully pushed on the stack).
 */
char	sap_stack[2*LINE_MAX];
#define SAP_SZ_MAX	((int)sizeof sap_stack)

/*
 * Our processor has two administrative features: a state register,
 * that allows to set the status of the result of an operation; and a
 * list of structures allowing to identify the strings pushed on the
 * stack. We need to keep also for the latter, the cardinal of strings
 * pushed and the total length of strings pushed. The size occupied by
 * the strings is defined by a macro since it can be deduced from the
 * index and length of the last string on stack.
 */
int	sap_state;	/* see definition of states macros in sap.h */
int	sap_nstr;	/* the cardinal of strings on stack */	
struct sap_str sap[SAP_NSTR_MAX];

void
sap_clear_state(void)
{
	sap_state = SAP_OK;
}

/*
 * Resetting values in simply putting all related variables to 0. In
 * fact, we simply reset sap_nstr without erasing anything.
 */
void
sap_reset(void)
{
	sap_nstr = 0;
	sap_state = SAP_OK;
}

/*
 * We will start by the basic procedure: pop'ing the last value. We
 * erase strictly nothing. We just redefine the number of vals on
 * the stack and the state. Pop'ing always succeed and clears the
 * state.
 */
void
sap_pop(void)
{
	if (sap_nstr > 0) --sap_nstr;
	sap_state = SAP_OK;
}

/*
 * Duplicating is putting on the stack a copy of what is last on the
 * stack. If nothing, does nothing---not an error.
 * But can error on overflow or too long.
 * This can be used to duplicate a value before storing, since storing
 * pops.
 */
void
sap_dup(void)
{
	char *p, *q;

	sap_state = SAP_OK;

	if (sap_nstr == 0)
		return;	/* nothing to do so done */

	if (sap_nstr == SAP_NSTR_MAX) {
		sap_state = SAP_EOVERFLOW;
		return;
	}

	if (SAP_SZ + SAP_CURLEN + 1 > SAP_SZ_MAX) {
		sap_state = SAP_ETOOLONG;
		return;
	}


	sap[sap_nstr].istr = SAP_SZ;
	sap[sap_nstr].len = SAP_CURLEN;

	for (p = SAP_CURSTR, q = sap_stack + sap[sap_nstr].istr;
		(*q++ = *p++);)
		;

	++sap_nstr;
}

/*
 * Taking two chars that has to be [0-9a-fA-F] in order to convert to
 * an unsigned byte value.
 * Returns 0 if OK, -1 on error.
 */
static int
hex2char(const char *h, char *cval)
{
	unsigned char n, c;
	int i;

	c = 0;
	for (i = 0; i < 2; ++i) {
		n = (unsigned char) toupper((unsigned char)h[i]);
		if ( n < '0' || n > 'F' || ( n > '9' && n < 'A') )
			return -1;
		n = (n < 'A')? n - '0' : n - 'A' + 10;
		c += (unsigned char) ( n << (4 * (1 - i)) );
	}

	*cval = (char) c;
	return 0;
}

#define UNQUOTE (flags & SAP_PUSH_UNQUOTE)
#define QUOTE	(flags & SAP_PUSH_QUOTE)
#define RAW	(flags & SAP_PUSH_AS_IS)
void
sap_push(const char *s, int len, int flags)
{
	int	r, w, max;	/* read; written; max */
	char	*p;
	char	quote;		/* the type of quote used */

	sap_state = SAP_EPUSH;

	if (s == NULL)
		return;

	if ( sap_nstr == SAP_NSTR_MAX && !(flags & SAP_PUSH_APPEND) ) {
		sap_state = SAP_EOVERFLOW;
		return;
	}

	/*
	 * Check or set len so that we will not overflow.
	 */
	max = SAP_SZ_MAX - SAP_SZ - 1;

	/*
	 * If QUOTE requested, we enforce UNQUOTE the src.
	 */
	if (QUOTE)
		flags |= SAP_PUSH_UNQUOTE;
		
	/*
	 * Ensure at least quotes place if asked for. Because of
	 * escaping or unescaping, we can not guess more...
	 */
	if ( QUOTE && max < 2 ) {
		sap_state = SAP_ETOOLONG;
		return;
	}

	if (len > 0) {
		if (len > max) {
			sap_state = SAP_ETOOLONG;
			return; 
		}
	} else /* len < 0: take all until '\0' */
		len = max;
	
	if (sap_nstr == 0)	/* nothing to append to... */
		flags &= ~SAP_PUSH_APPEND;

	if ( !(flags & SAP_PUSH_APPEND) ) {
		sap[sap_nstr].istr = SAP_SZ;
		sap[sap_nstr].len = 0;
		p = sap_stack + SAP_SZ;
	} else
		p = sap_stack + SAP_SZ - 1;	/* overwrite '\0' */


	r = w = 0;
 
	/*
	 * SAP_PUSH_QUOTE has nothing to do with quote: quote is for the
	 * src; if we are quoting the dst, this is only single quotes
	 * and we have only to preview a single quote in src, in order
	 * to end quoting, escape the single quote, reput a single quote
	 * and continue.
	 */
	if (QUOTE) {
		*p++ = '\'';
		++w;
	}

	/*
	 * quote is the quote char in src: nothing to do with our
	 * quoting on stack.
	 * r (read) is always increase at least by one; if more preview
	 * chars are needed, the code has to increase these supplements.
	 * We stop on C eos i.e. '\0'. But it is possible to create a
	 * string with embedded '\0' for storing by sap_join'ing with
	 * '\0', after...
	 */
	for (quote = '\0'; r < len && w < max && *s; ++s, ++r) {
		/* start by removing from input */
		if ( UNQUOTE && ( *s == '\'' || *s == '"' ) ) {
			if (*s == quote) {
				quote = '\0';
				continue;
			} else if (quote == '\0') {
				quote = *s;
				continue;
			}
		}
		if (QUOTE && *s == '\'') {
			if ( (max - w) < 4 ) {
				sap_state = SAP_ETOOLONG;
				return;
			}
			*p++ = '\'', ++w;
			*p++ = '\\', ++w;
			*p++ = '\'', ++w;
			*p++ = '\'', ++w;
			continue;
		}
		/* The escaping club... */
		if (!RAW && quote != '\'' && *s == '\\') {
			++s, ++r;
			switch (*s) {
			case '\\':
				if (!UNQUOTE) {
					if ( (max - w ) < 2 ) {
						sap_state = SAP_ETOOLONG;
						return;
					}
					*p++ = '\\';
					++w;
				}
				*p = '/';
				break;
			case '"':
				if (!UNQUOTE) {
					if ( (max - w ) < 2 ) {
						sap_state = SAP_ETOOLONG;
						return;
					}
					*p++ = '\\';
					++w;
				}
				*p = '"';
				break;
			case 'a':
				*p = '\a';
				break;
			case 'b':
				*p = '\b';
				break;
			case 'f':
				*p = '\f';
				break;
			case 'n':
				*p = '\n';
				break;
			case 'r':
				*p = '\r';
				break;
			case 't':
				*p = '\t';
				break;
			case 'v':
				*p = '\v';
				break;
			case 'x':
				if ( r >= (len - 2)
					|| hex2char(++s, p) < 0 )
					return;
				++s;
				r += 2;
				break;
			case '0': case '1': case '2': case '3':
			case '4': case '5': case '6': case '7': {
				int n;

				for (n = 1; n < 3 && (r + n) <= len
					&& *(s+n) >= '0'
					&& *(s+n) <= '7';
					++n)
					;
				*p = *s - '0';
				*p = '\0';
				for (; n; ++s, ++r, --n)
					*p = (char) ( ((*p) << 3)
						+ *s - '0' );	
				}
				break;
			default:
				*p = *s;
				break;
			} 
			if (*p == '\0') { /* No! */
				if ( (max - w) < ((QUOTE)? 6 : 4) ) {
					sap_state = SAP_ETOOLONG;
					return;
				}
				if (QUOTE) {
					*p++ = '\'';
					++w;
				}
				*p++ = '\\', ++w;
				*p++ = '0', ++w;
				*p++ = '0', ++w;
				*p = '0';	/* this write no accounted for */
				if (QUOTE) {
					*(++p) = '\'';
					++w;
				}
			} else if ( !UNQUOTE
				&& (*p == '"' || *p == '\\') ) {
				if ( (max - w) < ((QUOTE)? 4 : 2) ) {
					sap_state = SAP_ETOOLONG;
					return;
				}
				*p++ = '\\', ++w;
				*p = *s;
			}
		} else {
			*p = *s;
		}
		++p;	/* because of general rule: accounted here */
		++w;
	}

	if ( *s && r != len ) {	/* stopped before exhausting src */
		sap_state = SAP_ETOOLONG;
		return;
	}
	
	if ( (flags & SAP_PUSH_QUOTE) ) {
		if (w < max) {
			*p++ = '\'';
			++w;
		} else {
			sap_state = SAP_ETOOLONG;
			return;
		}
	}

	*p = '\0';		

	if ( sap_nstr == 0 || !(flags & SAP_PUSH_APPEND) )
		++sap_nstr;

	SAP_CURLEN += w;

	sap_state = SAP_OK;
}
#undef UNQUOTE
#undef QUOTE
#undef RAW

/*
 * Another unary procedure: replacing one char by another. We may
 * have a string with embedded '\0' (not really a string anymore, in
 * the C sense) so we use the length.
 */
void
sap_replace(char c, char r)
{
	int len;
	char *p;
	

	sap_state = SAP_EREPLACE;

	if (sap_nstr == 0) return;

	for (len = 0, p = SAP_CURSTR; len < SAP_CURLEN; ++len, ++p) {
		if (*p == c)
			*p = r;
	}

	sap_state = SAP_OK;
}

/*
 * Another unary procedure manipulating the last val on the stack.
 * We split the last value according to a character separator, starting
 * from the end of the string and according to a count of maximum
 * splits to achieve, -1 meaning splits all possible.
 * Sets sap_state with error if any and returns the number of splits
 * achieved (that can be less than the max specified).
 */
int
sap_split(char c, int max)
{
	int	istr;	/* the last will change when adding */
	int 	nc, nsplit;
	char	*p;

	if (sap_nstr == 0 || max == 0) {
		sap_state = SAP_ENONSENSE;
		return 0;
	}

	if (max == -1)
		max = SAP_CURLEN;

	sap_state = SAP_OK;
	istr = sap_nstr - 1;
	for (p = (SAP_CURSTR + SAP_CURLEN - 1), nc = SAP_CURLEN,
		nsplit = 0;
		nsplit < max && nc > 0; --nc, --p) {
		if (*p == c) {
			int i;

			if (sap_nstr == SAP_NSTR_MAX) {
				sap_state = SAP_EOVERFLOW;
				break;
			}
			*p = '\0';
			/* push up already splitted hence added */
			for (i = sap_nstr - 1; i > istr; --i)
				sap[i+1] = sap[i];
			sap[istr+1].istr = (int) (p - sap_stack + 1);
			sap[istr+1].len = (int) (sap_stack +
				SAP_ISTR(istr) + SAP_LEN(istr) -1 - p);
			SAP_LEN(istr) -= sap[istr+1].len + 1;	/* separator suppressed */
			++sap_nstr;
			++nsplit;
		}
	}

	return nsplit;
}

/*
 * The three procedures above are unary. Now we will describe binary
 * procedures, that manipulate the two last values in the stack. And,
 * obviously, they can only work if there is more than one...
 *
 * The first simply join, starting from the last, the two adjacent
 * values, by replacing the
 * intermediary '\0' with another char, updates the previous length
 * and discards the last sap entry. And does the number of times
 * specified (-1 means join all the strings on stack).
 */ 
void
sap_join(char c, int max)
{
	if (max == 0 || sap_nstr == 0 || sap_nstr < (max+1) ) {
		sap_state = SAP_ENONSENSE;
		return;
	}

	if (max == -1)
		max = sap_nstr - 1;

	sap_state = SAP_OK;

	while (max) {
		SAP_CURSTR[-1] = c;
		--sap_nstr;
		SAP_CURLEN += sap[sap_nstr].len + 1;
		--max;
	}
}

/*
 * Merging is concatenating the two adjacent values, shifting the last
 * value to overwrite the separating null char.
 */
void
sap_merge(void)
{
	char *p, *q;

	sap_state = SAP_EMERGE;

	if (sap_nstr < 2) return;

	for (p = SAP_CURSTR - 1, q = SAP_CURSTR; (*p++ = *q++);)
		;
	--sap_nstr;
	SAP_CURLEN += sap[sap_nstr].len;
	sap_state = SAP_OK;
}

/*
 * Swapping is just this: we exchange the last with the last but one.
 * But the verb is misleading: this is not really swapping, this is
 * doing a circular permutation: we "push" the circle so that the first
 * char of the last string comes in the of the first char of the
 * previous string; all the chars in the ring keep their respective
 * position, including the two trailing null chars...
 */
void
sap_swap(void)
{
	char	*p;	/* start of previous string; origin of the ring */
	int	shift;	/* shift for circular permutation */
	int	len;	/* length in chars of the ring including two '\0' */
	int	i;	/* index in ring relative to p */
	int	origin;	/* index of origin of the cycle */
	int	count;	/* counting the transpositions */
	char	c, d;

	sap_state = SAP_ESWAP;

	if (sap_nstr < 2) return;

	/*
	 * The trailing '\0' must be rotated too since, without, the two
	 * strings are concatenated.
	 */
	len = sap[sap_nstr-2].len + 1 + SAP_CURLEN + 1;
	shift = SAP_CURLEN + 1;

	/*
	 * The problem is that a circular permutation can be composed
	 * of several distinct cycles if the shift is a divisor of the
	 * length (if shift and divisor are not relatively "primes").
	 * There is more than one way to do it. Here, we register the
	 * index of the first char we transpose and, altogether, we
	 * have to do as many transposition as there are chars in the
	 * ring. When we are back to this origin of the cycle,
	 * continuing without changing would wreak havoc an already
	 * transposed cycle; so, after setting the new value, we
	 * restart with the next position if we have not transposed
	 * everything.
	 */
	p = sap_stack+sap[sap_nstr-2].istr;	/* reference is previous */
	for (i = origin = 0, c = p[0], count = 0; count < len;
		++count) {
		i = (i+shift) % len;
		d = p[i];
		p[i] = c;
		c = d;
		if (i == origin) {	/* cycle completed; shift */
			i = origin = (i+1) % len;
			c = p[i];
		}
	}

	SAP_CURISTR = sap[sap_nstr-2].istr + SAP_CURLEN + 1;
	len = sap[sap_nstr-2].len;
	sap[sap_nstr-2].len = SAP_CURLEN;
	SAP_CURLEN = len;

	sap_state = SAP_OK;
}

/*
 * Now the last action on the last string pushed: storing it. This
 * means that the string can be interpreted as a primitive type: an
 * int (various sizes), a float (various sizes) or a string: allocating
 * and copying the pushed string.
 * If the storing succeeds, the string is pop'ed. If one doesn't want
 * this, dup it before.
 */
void
sap_store(int type, void *val, ...)
{
	sap_state = SAP_ESTORE;

	if (sap_nstr == 0) return;

	switch (type & 0x0F) {
	case SAP_STORE_STRING:
		{
		int	n;
		char	*p, *q;

		if ( ( *((char **) val) = (char *) malloc((size_t) (SAP_CURLEN+1)) ) == NULL )
			return;

		for (p = SAP_CURSTR, q = *((char **) val), n = 0;
			n <= SAP_CURLEN; ++n) { /* including trailing '\0' */
			*q++ = *p++;
		}
		}
		break;
	case SAP_STORE_INT:
		{
		va_list ap;

		va_start(ap, val);
		if (s2i(type & 0xF0, val, ap) < 0)
			return;	/* error in conversion */
		va_end(ap);
		}
		break;
	case SAP_STORE_FILE:
		(void)fprintf((FILE *)val, "%s", SAP_CURSTR);
		break;	
	default:
		return;	/* unsupported for now */
		break;
	}

	sap_pop();	/* sets sap_state to SAP_OK */
}

/*
 * Try to interpret the string as an integer, with various options.
 * We convert to the maximum size, then verify the range.
 * If bounded, two supplementary arguments are required: low and high
 * values, always expressed as intmax_t for signed, and uintmax_t for
 * unsigned.
 * Returns 0 on success, -1 on error.
 */
static int
s2i(int type, void *val, ...)
{
	char	*ep;
	va_list	ap;

	if (type & SAP_STORE_FMT_UNSIGNED) {
		uintmax_t u;

		errno = 0;
		u = strtoumax(SAP_CURSTR, &ep, 10);
		if (errno) return -1;
		if (ep != NULL) {
			if (type & SAP_STORE_FMT_USER) {
				switch (ep[0]) {
				case 'k': case 'K':
					if (u > (UINTMAX_MAX / 1024))
						return -1;
					u *= 1024;
					break;
				case 'm': case 'M':
					if ( u > (UINTMAX_MAX / (1024 * 1024)) )
						return -1;
					u *= 1024 * 1024;
					break;
				case 'g': case 'G':
					if (u > (UINTMAX_MAX / (1024 * 1024 * 1024)) )
						return -1;
					u *= 1024 * 1024 * 1024;
					break;

				default:	/* unsupported */
					return -1;
					break;
				}
				if (ep[1] != '\0')
					return -1;
			}
		}

		if (type & SAP_STORE_FMT_BOUNDED) {
			uintmax_t lo, hi;

			sap_state = SAP_ENONSENSE;
			va_start(ap, val);
			if (ap == NULL)
				return -1;
			lo = va_arg(ap, uintmax_t);
			if (ap == NULL)
				return -1;
			hi = va_arg(ap, uintmax_t);
			sap_state = SAP_ESTORE;
			va_end(ap);
			if (u < lo || u > hi)
				return -1;
		}

		if (type & SAP_STORE_FMT_LONG)
			*((unsigned long *) val) = (unsigned long) u;
		else
			*((unsigned int *) val) = (unsigned int) u;
	} else {
		intmax_t i;

		errno = 0;
		i = strtoimax(SAP_CURSTR, &ep, 10);		
		if (errno) return -1;
		if (ep != NULL) {
			if (type & SAP_STORE_FMT_USER) {
				switch (ep[0]) {
				case 'k': case 'K':
					if (imaxabs(i) > (INTMAX_MAX / 1024))
						return -1;
					i *= 1024;
					break;
				case 'm': case 'M':
					if ( imaxabs(i) > (INTMAX_MAX / (1024 * 1024)) )
						return -1;
					i *= 1024 * 1024;
					break;
				case 'g': case 'G':
					if ( imaxabs(i) > (INTMAX_MAX / (1024 * 1024 * 1024)) )
						return -1;
					i *= 1024 * 1024 * 1024;
					break;

				default:	/* unsupported */
					return -1;
					break;
				}
				if (ep[1] != '\0')
					return -1;
			}
		}
		if (type & SAP_STORE_FMT_BOUNDED) {
			intmax_t	lo, hi;

			sap_state = SAP_ENONSENSE;
			va_start(ap, val);
			if (ap == NULL)
				return -1;
			lo = va_arg(ap, intmax_t);
			if (ap == NULL)
				return -1;

			hi = va_arg(ap, intmax_t);

			va_end(ap);
			sap_state = SAP_ESTORE;
			if (i < lo || i > hi)
				return -1;
		}
		if (type & SAP_STORE_FMT_LONG)
			*((long *) val) = (long) i;
		else
			*((int *) val) = (int) i;
	}

	return 0;
}

