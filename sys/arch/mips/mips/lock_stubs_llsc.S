/*	$NetBSD: lock_stubs_llsc.S,v 1.18 2023/12/05 17:38:40 andvar Exp $	*/

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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

#include "opt_cputype.h"
#include "opt_lockdebug.h"

#include <sys/errno.h>

#include <machine/asm.h>

RCSID("$NetBSD: lock_stubs_llsc.S,v 1.18 2023/12/05 17:38:40 andvar Exp $")

#include "assym.h"

/*
 * Set ISA level for the assembler.
 * XXX Clean up with a macro?  Same code fragment is in mipsX_subr.S too.
 * XXX Key off build abi instead of processor type?
 */
#if defined(MIPS3)
	.set	mips3
#endif

#if defined(MIPS32)
	.set	mips32
#endif

#if defined(MIPS64)
	.set	mips64
#endif

	.set	noreorder
	.set	noat

/*
 * unsigned long atomic_cas_ulong_llsc(volatile unsigned long *val,
 *     unsigned long old, unsigned long new);
 *
 *	For hysterical raisins in sys/arch/mips/include/lock.h, success
 *	implies load-acquire.  The SYNC_ACQ here could be moved there
 *	instead.
 */
STATIC_LEAF(llsc_atomic_cas_ulong)
	LLSCSYNC
1:
	LONG_LL	t0, (a0)
	bne	t0, a1, 2f
	 move t1, a2
	LONG_SC	t1, (a0)
	beqz	t1, 1b
	 nop
	SYNC_ACQ
	j	ra
	 move	v0, a1
2:
	j	ra
	 move	v0, t0
END(llsc_atomic_cas_ulong)

/*
 * unsigned int _atomic_cas_uint_llsc(volatile unsigned int *val,
 *    unsigned int old, unsigned int new);
 *
 *	For hysterical raisins in sys/arch/mips/include/lock.h, success
 *	implies load-acquire.  The SYNC_ACQ here could be moved there
 *	instead.
 */
STATIC_LEAF(llsc_atomic_cas_uint)
	LLSCSYNC
1:
	INT_LL	t0, (a0)
	bne	t0, a1, 2f
	 move	t1, a2
	INT_SC	t1, (a0)
	beqz	t1, 1b
	 nop
	SYNC_ACQ
	j	ra
	 move	v0, a1
2:
	j	ra
	 move	v0, t0
END(llsc_atomic_cas_uint)

/*
 * int llsc_ucas_32(volatile uint32_t *ptr, uint32_t old,
 *	uint32_t new, uint32_t *ret)
 *
 *	Implies release/acquire barriers until someone tells me
 *	otherwise about _ucas_32/64.
 */
STATIC_LEAF(llsc_ucas_32)
	.set at
	PTR_LA	v0, _C_LABEL(llsc_ucaserr)
	.set noat
	PTR_L	v1, L_PCB(MIPS_CURLWP)
	PTR_S	v0, PCB_ONFAULT(v1)
	bltz	a0, _C_LABEL(llsc_ucaserr)
	 nop
	move	v0, zero
	SYNC_REL

	LLSCSYNC
1:	ll	t0, 0(a0)
	bne	t0, a1, 2f
	 move	t1, a2
	sc	t1, 0(a0)
	beqz	t1, 1b
	 nop
	SYNC_ACQ

2:	PTR_S	zero, PCB_ONFAULT(v1)
	j	ra
	 sw	t0, 0(a3)
END(llsc_ucas_32)

#ifdef _LP64
/*
 * int llsc_ucas_64(volatile uint64_t *ptr, uint64_t old,
 *	uint64_t new, uint64_t *ret)
 */
STATIC_LEAF(llsc_ucas_64)
	.set at
	PTR_LA	v0, _C_LABEL(llsc_ucaserr)
	.set noat
	PTR_L	v1, L_PCB(MIPS_CURLWP)
	PTR_S	v0, PCB_ONFAULT(v1)
	bltz	a0, _C_LABEL(llsc_ucaserr)
	 nop
	move	v0, zero
	SYNC_REL

	LLSCSYNC
1:	lld	t0, 0(a0)
	bne	t0, a1, 2f
	 move	t1, a2
	scd	t1, 0(a0)
	beqz	t1, 1b
	 nop
	SYNC_ACQ

2:	PTR_S	zero, PCB_ONFAULT(v1)
	j	ra
	 sd	t0, 0(a3)
END(llsc_ucas_64)
#endif /* _LP64 */

STATIC_LEAF_NOPROFILE(llsc_ucaserr)
	PTR_S	zero, PCB_ONFAULT(v1)		# reset fault handler
	j	ra
	 li	v0, EFAULT			# return EFAULT on error
END(llsc_ucaserr)

#ifndef LOCKDEBUG

/*
 * void	mutex_enter(kmutex_t *mtx);
 */
STATIC_LEAF(llsc_mutex_enter)
	LLSCSYNC
	PTR_LL	t0, MTX_OWNER(a0)
1:
	bnez	t0, 2f
	 move	t2, MIPS_CURLWP
	PTR_SC	t2, MTX_OWNER(a0)
	beqz	t2, 1b
	 PTR_LL	t0, MTX_OWNER(a0)
	j	ra
	 BDSYNC_ACQ
2:
	j	_C_LABEL(mutex_vector_enter)
	 nop
END(llsc_mutex_enter)

/*
 * void	mutex_exit(kmutex_t *mtx);
 */
STATIC_LEAF(llsc_mutex_exit)
	SYNC_REL
	LLSCSYNC
	PTR_LL	t0, MTX_OWNER(a0)
1:
	bne	t0, MIPS_CURLWP, 2f
	 move	t2, zero
	PTR_SC	t2, MTX_OWNER(a0)
	beqz	t2, 1b
	 PTR_LL	t0, MTX_OWNER(a0)
	j	ra
	 BDSYNC_PLUNGER
2:
	j	_C_LABEL(mutex_vector_exit)
	 nop
END(llsc_mutex_exit)

/*
 * void	mutex_spin_enter(kmutex_t *mtx);
 */
STATIC_NESTED(llsc_mutex_spin_enter, CALLFRAME_SIZ, ra)
	move	t0, a0
	PTR_L	t2, L_CPU(MIPS_CURLWP)
	INT_L	a0, MTX_IPL(t0)
#ifdef PARANOIA
	INT_L	ta1, CPU_INFO_CPL(t2)
#endif

	/*
	 * We need to raise our IPL.  But it means calling another routine
	 * but it's written to have little overhead.  call splraise
	 * (only uses a0-a3 and v0-v1)
	 */
	move	t3, ra			# need to save ra
	jal	_C_LABEL(splraise)
	 nop
	move	ra, t3			# move ra back
#ifdef PARANOIA
10:	bne	ta1, v0, 10b		# loop forever if v0 != ta1
	 nop
#endif /* PARANOIA */

	/*
	 * If this is the first lock of the mutex, store the previous IPL for
	 * exit.  Even if an interrupt happens, the mutex count will not change.
	 */
1:
	INT_L	ta2, CPU_INFO_MTX_COUNT(t2)
	INT_ADDU ta3, ta2, -1
	INT_S	ta3, CPU_INFO_MTX_COUNT(t2)
	bltz	ta2, 2f
	 nop
	INT_S	v0, CPU_INFO_MTX_OLDSPL(t2)	/* returned by splraise */
2:
#ifdef PARANOIA
	INT_L	ta1, CPU_INFO_MTX_OLDSPL(t2)
	INT_L	ta2, CPU_INFO_CPL(t2)	# get updated CPL
	sltu	v0, ta2, ta0		# v0 = cpl < mtx_ipl
	sltu	v1, ta2, ta1		# v1 = cpl < oldspl
	sll	v0, 1
	or	v0, v1
12:	bnez	v0, 12b			# loop forever if any are true
	 nop
#endif /* PARANOIA */

	LLSCSYNC
	INT_LL	t3, MTX_LOCK(t0)
3:
	bnez	t3, 4f
	 li	t1, 1
	INT_SC	t1, MTX_LOCK(t0)
	beqz	t1, 3b
	 INT_LL	t3, MTX_LOCK(t0)
	j	ra
	 BDSYNC_ACQ
4:
	j	_C_LABEL(mutex_spin_retry)
	 move	a0, t0
END(llsc_mutex_spin_enter)

/*
 * void	mutex_spin_exit(kmutex_t *mtx);
 */
LEAF(llsc_mutex_spin_exit)
	SYNC_REL
	PTR_L	t2, L_CPU(MIPS_CURLWP)
#if defined(DIAGNOSTIC)
	INT_L	t0, MTX_LOCK(a0)
	beqz	t0, 2f
	 nop
#endif
	INT_S	zero, MTX_LOCK(a0)

	/*
	 * We need to grab this before the mutex count is incremented
	 * because if we get an interrupt, it may see the count as zero
	 * and overwrite the oldspl value with a bogus value.
	 */
#ifdef PARANOIA
	INT_L	a2, MTX_IPL(a0)
#endif
	INT_L	a0, CPU_INFO_MTX_OLDSPL(t2)

	/*
	 * Increment the mutex count
	 */
	INT_L	t0, CPU_INFO_MTX_COUNT(t2)
	INT_ADDU t0, t0, 1
	INT_S	t0, CPU_INFO_MTX_COUNT(t2)

	/*
	 * If the IPL doesn't change, nothing to do
	 */
	INT_L	a1, CPU_INFO_CPL(t2)

#ifdef PARANOIA
	sltu	v0, a1, a2		# v0 = cpl < mtx_ipl
	sltu	v1, a1, a0		# v1 = cpl < oldspl
	sll	v0, 1
	or	v0, v1
12:	bnez	v0, 12b			# loop forever if either is true
	 nop
#endif /* PARANOIA */

	beq	a0, a1, 1f		# if oldspl == cpl
	 nop				#   no reason to drop ipl

	bltz	t0, 1f			# there are still holders
	 nop				# so don't drop IPL

	/*
	 * Mutex count is zero so we need to restore the old IPL
	 */
#ifdef PARANOIA
	sltiu	v0, a0, IPL_HIGH+1
13:	beqz	v0, 13b			# loop forever if ipl > IPL_HIGH
	 nop
#endif
	j	 _C_LABEL(splx)
	 BDSYNC_PLUNGER
1:
	j	ra
	 BDSYNC_PLUNGER
#if defined(DIAGNOSTIC)
2:
	j	_C_LABEL(mutex_vector_exit)
	 nop
#endif
END(llsc_mutex_spin_exit)
#endif	/* !LOCKDEBUG */

	.rdata
EXPORT_OBJECT(mips_llsc_locore_atomicvec)
	PTR_WORD 	llsc_atomic_cas_uint
	PTR_WORD 	llsc_atomic_cas_ulong
	PTR_WORD	llsc_ucas_32
#ifdef _LP64
	PTR_WORD	llsc_ucas_64
#else
	PTR_WORD	0
#endif /* _LP64 */
#ifdef LOCKDEBUG
	PTR_WORD	mutex_vector_enter
	PTR_WORD	mutex_vector_exit
	PTR_WORD	mutex_vector_enter
	PTR_WORD	mutex_vector_exit
#else
	PTR_WORD	llsc_mutex_enter
	PTR_WORD	llsc_mutex_exit
	PTR_WORD	llsc_mutex_spin_enter
	PTR_WORD	llsc_mutex_spin_exit
#endif	/* !LOCKDEBUG */
