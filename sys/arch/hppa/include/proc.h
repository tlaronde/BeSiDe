/*	$NetBSD: proc.h,v 1.15 2023/07/23 10:09:36 skrll Exp $	*/

/*	$OpenBSD: proc.h,v 1.4 2009/12/29 13:11:40 jsing Exp $ */

/*
 * Copyright (c) 1992, 1993
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
 *
 *	@(#)proc.h      7.1 (Berkeley) 5/15/91
 */

#ifndef _HPPA_PROC_H_
#define _HPPA_PROC_H_

/*
 * Machine-dependent part of the lwp structure for hppa.
 */
struct mdlwp {
	struct		trapframe *md_regs;	/* registers on current frame */
	volatile int	md_astpending;		/* AST pending for this LWP */
	int		md_flags;		/* machine-dependent flags */

	__vaddr_t	md_bpva;
	unsigned int	md_bpsave[2];
};

struct mdproc {
	void	      (*md_syscall)(struct trapframe *, int *);

	int		md_flags;		/* machine-dependent flags */
#define MDP_OLDSPACE	__BIT(0)
};

/* md_flags */

#endif /* _HPPA_PROC_H_ */
