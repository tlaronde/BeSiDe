/*	$NetBSD: setjmp.S,v 1.14 2023/10/11 09:12:21 skrll Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
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
 *
 * from: Header: setjmp.s,v 1.2 92/06/25 03:18:43 torek Exp
 */

#include <machine/asm.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
	.asciz "@(#)setjmp.s	8.1 (Berkeley) 6/4/93"
#else
	RCSID("$NetBSD: setjmp.S,v 1.14 2023/10/11 09:12:21 skrll Exp $")
#endif
#endif /* LIBC_SCCS and not lint */

/*
 * C library -- setjmp, longjmp
 *
 *	longjmp(a,v)
 * will generate a "return(v)" from
 * the last call to
 *	setjmp(a)
 * by restoring registers from the stack,
 * and a struct sigcontext, see <signal.h>
 */

#include "SYS.h"
#include "assym.h"

ENTRY(__setjmp14)
	save	%sp, -CCFSZ, %sp
	/*
	 * local vars on stack: struct sigstack foo @ offset 0x48
	 * Note: sc_g1 is not used; sc_o0 is set in longjmp()
	 */
	mov	0, %o0		/* 0 - is ignored */
	add	%i0, 32, %o2	/* build sigcontext sc.sc_mask */
	call	_C_LABEL(__sigprocmask14)
	 clr	%o1		/* sigprocmask(0, NULL, &sc.sc_mask) */

	add	%sp, 0x48, %o1	/* (foo being in arg dump area) */
	call	_C_LABEL(__sigaltstack14)
	 clr	%o0		/* sigstack(NULL, &foo) */

	ld	[%sp + 0x48 + 8], %o0	/* foo.ss_flags */
	and	%o0, 1, %o1	/* onstack = foo.ss_flags & 1; */
	st	%o1, [%i0 + _SC_ONSTACK] /* sc.sc_onstack = current onstack; */
	st	%fp, [%i0 + _SC_SP]	/* sc.sc_sp = (caller's) sp */
	add	%i7, 8, %o0
	st	%o0, [%i0 + _SC_PC]	/* sc.sc_pc = return_pc */
	st	%g3, [%i0 + _SC_NPC]	/* sc.sc_npc */
	st	%g0, [%i0 + _SC_PSR]	/* sc.sc_psr = (clean psr) */
	st	%g2, [%i0 + _SC_G1]
	st	%g4, [%i0 + _SIZEOF_SC + _JB_G4]
	st	%g7, [%i0 + _SIZEOF_SC + _JB_G7]
	ret			/* return 0 */
	 restore	%g0, %g0, %o0

