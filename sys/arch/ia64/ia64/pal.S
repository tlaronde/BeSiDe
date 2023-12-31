/*	$NetBSD: pal.S,v 1.2 2023/10/06 11:45:16 skrll Exp $	*/

/*-
 * Copyright (c) 2000-2001 Doug Rabson
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	$FreeBSD$
 */

#include <machine/asm.h>

	.data
	.global ia64_pal_entry
ia64_pal_entry:	.quad 0
	.text

/*
 * struct ia64_pal_result ia64_call_pal_static(u_int64_t proc,
 *	u_int64_t arg1, u_int64_t arg2, u_int64_t arg3)
 */
ENTRY(ia64_call_pal_static, 4)

	.regstk	4,5,0,0
palret	=	loc0
entry	=	loc1
rpsave	=	loc2
pfssave =	loc3
psrsave	=	loc4

	alloc	pfssave=ar.pfs,4,5,0,0
	;;
	mov	rpsave=rp

	movl	entry=@gprel(ia64_pal_entry)
1:	mov	palret=ip		// for return address
	;;
	add	entry=entry,gp
	mov	psrsave=psr
	mov	r28=in0			// procedure number
	;;
	ld8	entry=[entry]		// read entry point
	mov	r29=in1			// copy arguments
	mov	r30=in2
	mov	r31=in3
	;;
	mov	b6=entry
	add	palret=2f-1b,palret	// calculate return address
	;;
	mov	b0=palret
	rsm	psr.i			// disable interrupts
	;;
	br.cond.sptk b6			// call into firmware
2:	mov	psr.l=psrsave
	mov	rp=rpsave
	mov	ar.pfs=pfssave
	;;
	srlz.d
	br.ret.sptk rp

END(ia64_call_pal_static)

#ifdef _KERNEL

/*
 * struct ia64_pal_result ia64_call_pal_static_physical(u_int64_t proc,
 *	u_int64_t arg1, u_int64_t arg2, u_int64_t arg3)
 */
ENTRY(ia64_call_pal_static_physical, 4)

	.regstk	4,5,0,0
palret	=	loc0
entry	=	loc1
rpsave	=	loc2
pfssave =	loc3
psrsave	=	loc4

	alloc	pfssave=ar.pfs,4,5,0,0
	;;
	mov	rpsave=rp

	movl	entry=@gprel(ia64_pal_entry)
1:	mov	palret=ip		// for return address
	;;
	add	entry=entry,gp
	mov	r28=in0			// procedure number
	;;
	ld8	entry=[entry]		// read entry point
	mov	r29=in1			// copy arguments
	mov	r30=in2
	mov	r31=in3
	;;
	dep	entry=0,entry,61,3	// physical address
	dep	palret=0,palret,61,3	// physical address
	br.call.sptk.many rp=ia64_physical_mode
	mov	psrsave=ret0
	;;
	mov	b6=entry
	add	palret=2f-1b,palret	// calculate return address
	;;
	mov	b0=palret
	br.cond.sptk b6			// call into firmware
	;;
2:	mov	r14=psrsave
	;;
	br.call.sptk.many rp=ia64_change_mode
	;;
	mov	rp=rpsave
	mov	ar.pfs=pfssave
	;;
	br.ret.sptk rp

END(ia64_call_pal_static_physical)

#endif

/*
 * struct ia64_pal_result ia64_call_pal_stacked(u_int64_t proc,
 *	u_int64_t arg1, u_int64_t arg2, u_int64_t arg3)
 */
ENTRY(ia64_call_pal_stacked, 4)

	.regstk	4,4,4,0
entry	=	loc0
rpsave	=	loc1
pfssave =	loc2
psrsave	=	loc3

	alloc	pfssave=ar.pfs,4,4,4,0
	;;
	mov	rpsave=rp
	movl	entry=@gprel(ia64_pal_entry)
	;;
	add	entry=entry,gp
	mov	psrsave=psr
	mov	r28=in0			// procedure number
	mov	out0=in0
	;;
	ld8	entry=[entry]		// read entry point
	mov	out1=in1		// copy arguments
	mov	out2=in2
	mov	out3=in3
	;;
	mov	b6=entry
	;;
	rsm	psr.i			// disable interrupts
	;;
	br.call.sptk.many rp=b6		// call into firmware
	mov	psr.l=psrsave
	mov	rp=rpsave
	mov	ar.pfs=pfssave
	;;
	srlz.d
	br.ret.sptk rp

END(ia64_call_pal_stacked)

#ifdef _KERNEL

/*
 * struct ia64_pal_result ia64_call_pal_stacked_physical(u_int64_t proc,
 *	u_int64_t arg1, u_int64_t arg2, u_int64_t arg3)
 */
ENTRY(ia64_call_pal_stacked_physical, 4)

	.regstk	4,4,4,0
entry	=	loc0
rpsave	=	loc1
pfssave =	loc2
psrsave	=	loc3

	alloc	pfssave=ar.pfs,4,4,4,0
	;;
	mov	rpsave=rp
	movl	entry=@gprel(ia64_pal_entry)
	;;
	add	entry=entry,gp
	mov	r28=in0			// procedure number
	mov	out0=in0
	;;
	ld8	entry=[entry]		// read entry point
	mov	out1=in1		// copy arguments
	mov	out2=in2
	mov	out3=in3
	;;
	dep	entry=0,entry,61,3	// physical address
	br.call.sptk.many rp=ia64_physical_mode
	mov	psrsave=ret0
	;;
	mov	b6=entry
	;;
	br.call.sptk.many rp=b6		// call into firmware
	;;
	mov	r14=psrsave
	;;
	br.call.sptk.many rp=ia64_change_mode
	;;
	mov	rp=rpsave
	mov	ar.pfs=pfssave
	;;
	br.ret.sptk rp

END(ia64_call_pal_stacked_physical)

#endif
