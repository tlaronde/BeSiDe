/* $NetBSD: pci_swiz_bus_io_chipdep.c,v 1.45 2023/12/06 01:46:34 thorpej Exp $ */

/*-
 * Copyright (c) 1998, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 * Copyright (c) 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

/*
 * Common PCI Chipset "bus I/O" functions, for chipsets which have to
 * deal with only a single PCI interface chip in a machine.
 *
 * uses:
 *	CHIP		name of the 'chip' it's being compiled for.
 *	CHIP_IO_BASE	Sparse I/O space base to use.
 *	CHIP_IO_ARENA_STORE
 *			If defined, device-provided static storage area
 *			for the I/O space arena.  If this is defined,
 *			CHIP_IO_BTAG_STORE and CHIP_IO_BTAG_COUNT must
 *			also be defined.  If this is not defined, a
 *			static area will be declared.
 *	CHIP_IO_BTAG_STORE
 *			Device-provided static storage area for the
 *			I/O space arena's boundary tags.  Ignored
 *			unless CHIP_IO_ARENA_STORE is defined.
 *	CHIP_IO_BTAG_COUNT
 *			The number of device-provided static I/O
 *			space boundary tags.  Ignored unless
 *			CHIP_IO_ARENA_STORE is defined.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(1, "$NetBSD: pci_swiz_bus_io_chipdep.c,v 1.45 2023/12/06 01:46:34 thorpej Exp $");

#include <sys/vmem_impl.h>

#define	__C(A,B)	__CONCAT(A,B)
#define	__S(S)		__STRING(S)

/* mapping/unmapping */
static int	__C(CHIP,_io_map)(void *, bus_addr_t, bus_size_t, int,
		    bus_space_handle_t *, int);
static void	__C(CHIP,_io_unmap)(void *, bus_space_handle_t,
		    bus_size_t, int);
static int	__C(CHIP,_io_subregion)(void *, bus_space_handle_t,
		    bus_size_t, bus_size_t, bus_space_handle_t *);

static int	__C(CHIP,_io_translate)(void *, bus_addr_t, bus_size_t,
		    int, struct alpha_bus_space_translation *);
static int	__C(CHIP,_io_get_window)(void *, int,
		    struct alpha_bus_space_translation *);

/* allocation/deallocation */
static int	__C(CHIP,_io_alloc)(void *, bus_addr_t, bus_addr_t,
		    bus_size_t, bus_size_t, bus_addr_t, int, bus_addr_t *,
	            bus_space_handle_t *);
static void	__C(CHIP,_io_free)(void *, bus_space_handle_t,
		    bus_size_t);

/* get kernel virtual address */
static void *	__C(CHIP,_io_vaddr)(void *, bus_space_handle_t);

/* mmap for user */
static paddr_t	__C(CHIP,_io_mmap)(void *, bus_addr_t, off_t, int, int);

/* barrier */
static inline void __C(CHIP,_io_barrier)(void *, bus_space_handle_t,
		    bus_size_t, bus_size_t, int);

/* read (single) */
static inline uint8_t __C(CHIP,_io_read_1)(void *, bus_space_handle_t,
		    bus_size_t);
static inline uint16_t __C(CHIP,_io_read_2)(void *, bus_space_handle_t,
		    bus_size_t);
static inline uint32_t __C(CHIP,_io_read_4)(void *, bus_space_handle_t,
		    bus_size_t);
static inline uint64_t __C(CHIP,_io_read_8)(void *, bus_space_handle_t,
		    bus_size_t);

/* read multiple */
static void	__C(CHIP,_io_read_multi_1)(void *, bus_space_handle_t,
		    bus_size_t, uint8_t *, bus_size_t);
static void	__C(CHIP,_io_read_multi_2)(void *, bus_space_handle_t,
		    bus_size_t, uint16_t *, bus_size_t);
static void	__C(CHIP,_io_read_multi_4)(void *, bus_space_handle_t,
		    bus_size_t, uint32_t *, bus_size_t);
static void	__C(CHIP,_io_read_multi_8)(void *, bus_space_handle_t,
		    bus_size_t, uint64_t *, bus_size_t);

/* read region */
static void	__C(CHIP,_io_read_region_1)(void *, bus_space_handle_t,
		    bus_size_t, uint8_t *, bus_size_t);
static void	__C(CHIP,_io_read_region_2)(void *, bus_space_handle_t,
		    bus_size_t, uint16_t *, bus_size_t);
static void	__C(CHIP,_io_read_region_4)(void *, bus_space_handle_t,
		    bus_size_t, uint32_t *, bus_size_t);
static void	__C(CHIP,_io_read_region_8)(void *, bus_space_handle_t,
		    bus_size_t, uint64_t *, bus_size_t);

/* write (single) */
static inline void __C(CHIP,_io_write_1)(void *, bus_space_handle_t,
		    bus_size_t, uint8_t);
static inline void __C(CHIP,_io_write_2)(void *, bus_space_handle_t,
		    bus_size_t, uint16_t);
static inline void __C(CHIP,_io_write_4)(void *, bus_space_handle_t,
		    bus_size_t, uint32_t);
static inline void __C(CHIP,_io_write_8)(void *, bus_space_handle_t,
		    bus_size_t, uint64_t);

/* write multiple */
static void	__C(CHIP,_io_write_multi_1)(void *, bus_space_handle_t,
		    bus_size_t, const uint8_t *, bus_size_t);
static void	__C(CHIP,_io_write_multi_2)(void *, bus_space_handle_t,
		    bus_size_t, const uint16_t *, bus_size_t);
static void	__C(CHIP,_io_write_multi_4)(void *, bus_space_handle_t,
		    bus_size_t, const uint32_t *, bus_size_t);
static void	__C(CHIP,_io_write_multi_8)(void *, bus_space_handle_t,
		    bus_size_t, const uint64_t *, bus_size_t);

/* write region */
static void	__C(CHIP,_io_write_region_1)(void *, bus_space_handle_t,
		    bus_size_t, const uint8_t *, bus_size_t);
static void	__C(CHIP,_io_write_region_2)(void *, bus_space_handle_t,
		    bus_size_t, const uint16_t *, bus_size_t);
static void	__C(CHIP,_io_write_region_4)(void *, bus_space_handle_t,
		    bus_size_t, const uint32_t *, bus_size_t);
static void	__C(CHIP,_io_write_region_8)(void *, bus_space_handle_t,
		    bus_size_t, const uint64_t *, bus_size_t);

/* set multiple */
static void	__C(CHIP,_io_set_multi_1)(void *, bus_space_handle_t,
		    bus_size_t, uint8_t, bus_size_t);
static void	__C(CHIP,_io_set_multi_2)(void *, bus_space_handle_t,
		    bus_size_t, uint16_t, bus_size_t);
static void	__C(CHIP,_io_set_multi_4)(void *, bus_space_handle_t,
		    bus_size_t, uint32_t, bus_size_t);
static void	__C(CHIP,_io_set_multi_8)(void *, bus_space_handle_t,
		    bus_size_t, uint64_t, bus_size_t);

/* set region */
static void	__C(CHIP,_io_set_region_1)(void *, bus_space_handle_t,
		    bus_size_t, uint8_t, bus_size_t);
static void	__C(CHIP,_io_set_region_2)(void *, bus_space_handle_t,
		    bus_size_t, uint16_t, bus_size_t);
static void	__C(CHIP,_io_set_region_4)(void *, bus_space_handle_t,
		    bus_size_t, uint32_t, bus_size_t);
static void	__C(CHIP,_io_set_region_8)(void *, bus_space_handle_t,
		    bus_size_t, uint64_t, bus_size_t);

/* copy */
static void	__C(CHIP,_io_copy_region_1)(void *, bus_space_handle_t,
		    bus_size_t, bus_space_handle_t, bus_size_t, bus_size_t);
static void	__C(CHIP,_io_copy_region_2)(void *, bus_space_handle_t,
		    bus_size_t, bus_space_handle_t, bus_size_t, bus_size_t);
static void	__C(CHIP,_io_copy_region_4)(void *, bus_space_handle_t,
		    bus_size_t, bus_space_handle_t, bus_size_t, bus_size_t);
static void	__C(CHIP,_io_copy_region_8)(void *, bus_space_handle_t,
		    bus_size_t, bus_space_handle_t, bus_size_t, bus_size_t);

#ifndef CHIP_IO_ARENA_STORE

#ifdef CHIP_IO_W1_BUS_START
#define	CHIP_IO_W1_BUS_SPAN	1
#else
#define	CHIP_IO_W1_BUS_SPAN	0
#endif /* CHIP_IO_W1_BUS_START */

#ifdef CHIP_IO_W2_BUS_START
#define	CHIP_IO_W2_BUS_SPAN	1
#else
#define	CHIP_IO_W2_BUS_SPAN	0
#endif /* CHIP_IO_W2_BUS_START */

#define	CHIP_IO_SPAN_COUNT	(CHIP_IO_W1_BUS_SPAN + CHIP_IO_W2_BUS_SPAN)

#define	CHIP_IO_BTAG_COUNT(v)	VMEM_EST_BTCOUNT(CHIP_IO_SPAN_COUNT, 8)
#define	CHIP_IO_BTAG_STORE(v)	__C(CHIP,_io_btag_store)
#define	CHIP_IO_ARENA_STORE(v)	(&(__C(CHIP,_io_arena_store)))

static struct vmem __C(CHIP,_io_arena_store);
static struct vmem_btag __C(CHIP,_io_btag_store)[CHIP_IO_BTAG_COUNT(xxx)];

#endif /* CHIP_IO_ARENA_STORE */

#ifndef CHIP_ADDR_SHIFT
#define	CHIP_ADDR_SHIFT		5
#endif

#ifndef CHIP_SIZE_SHIFT
#define	CHIP_SIZE_SHIFT		3
#endif

void
__C(CHIP,_bus_io_init)(bus_space_tag_t t, void *v)
{
	vmem_t *vm;
	int error __diagused;

	/*
	 * Initialize the bus space tag.
	 */

	/* cookie */
	t->abs_cookie =		v;

	/* mapping/unmapping */
	t->abs_map =		__C(CHIP,_io_map);
	t->abs_unmap =		__C(CHIP,_io_unmap);
	t->abs_subregion =	__C(CHIP,_io_subregion);

	t->abs_translate =	__C(CHIP,_io_translate);
	t->abs_get_window =	__C(CHIP,_io_get_window);

	/* allocation/deallocation */
	t->abs_alloc =		__C(CHIP,_io_alloc);
	t->abs_free = 		__C(CHIP,_io_free);

	/* get kernel virtual address */
	t->abs_vaddr =		__C(CHIP,_io_vaddr);

	/* mmap for user */
	t->abs_mmap =		__C(CHIP,_io_mmap);

	/* barrier */
	t->abs_barrier =	__C(CHIP,_io_barrier);
	
	/* read (single) */
	t->abs_r_1 =		__C(CHIP,_io_read_1);
	t->abs_r_2 =		__C(CHIP,_io_read_2);
	t->abs_r_4 =		__C(CHIP,_io_read_4);
	t->abs_r_8 =		__C(CHIP,_io_read_8);
	
	/* read multiple */
	t->abs_rm_1 =		__C(CHIP,_io_read_multi_1);
	t->abs_rm_2 =		__C(CHIP,_io_read_multi_2);
	t->abs_rm_4 =		__C(CHIP,_io_read_multi_4);
	t->abs_rm_8 =		__C(CHIP,_io_read_multi_8);
	
	/* read region */
	t->abs_rr_1 =		__C(CHIP,_io_read_region_1);
	t->abs_rr_2 =		__C(CHIP,_io_read_region_2);
	t->abs_rr_4 =		__C(CHIP,_io_read_region_4);
	t->abs_rr_8 =		__C(CHIP,_io_read_region_8);
	
	/* write (single) */
	t->abs_w_1 =		__C(CHIP,_io_write_1);
	t->abs_w_2 =		__C(CHIP,_io_write_2);
	t->abs_w_4 =		__C(CHIP,_io_write_4);
	t->abs_w_8 =		__C(CHIP,_io_write_8);
	
	/* write multiple */
	t->abs_wm_1 =		__C(CHIP,_io_write_multi_1);
	t->abs_wm_2 =		__C(CHIP,_io_write_multi_2);
	t->abs_wm_4 =		__C(CHIP,_io_write_multi_4);
	t->abs_wm_8 =		__C(CHIP,_io_write_multi_8);
	
	/* write region */
	t->abs_wr_1 =		__C(CHIP,_io_write_region_1);
	t->abs_wr_2 =		__C(CHIP,_io_write_region_2);
	t->abs_wr_4 =		__C(CHIP,_io_write_region_4);
	t->abs_wr_8 =		__C(CHIP,_io_write_region_8);

	/* set multiple */
	t->abs_sm_1 =		__C(CHIP,_io_set_multi_1);
	t->abs_sm_2 =		__C(CHIP,_io_set_multi_2);
	t->abs_sm_4 =		__C(CHIP,_io_set_multi_4);
	t->abs_sm_8 =		__C(CHIP,_io_set_multi_8);
	
	/* set region */
	t->abs_sr_1 =		__C(CHIP,_io_set_region_1);
	t->abs_sr_2 =		__C(CHIP,_io_set_region_2);
	t->abs_sr_4 =		__C(CHIP,_io_set_region_4);
	t->abs_sr_8 =		__C(CHIP,_io_set_region_8);

	/* copy */
	t->abs_c_1 =		__C(CHIP,_io_copy_region_1);
	t->abs_c_2 =		__C(CHIP,_io_copy_region_2);
	t->abs_c_4 =		__C(CHIP,_io_copy_region_4);
	t->abs_c_8 =		__C(CHIP,_io_copy_region_8);

	vm = vmem_init(CHIP_IO_ARENA_STORE(v),
		       __S(__C(CHIP,_bus_io)),		/* name */
		       0,				/* addr */
		       0,				/* size */
		       1,				/* quantum */
		       NULL,				/* importfn */
		       NULL,				/* releasefn */
		       NULL,				/* source */
		       0,				/* qcache_max */
		       VM_NOSLEEP | VM_PRIVTAGS,
		       IPL_NONE);
	KASSERT(vm != NULL);

	vmem_add_bts(vm, CHIP_IO_BTAG_STORE(v), CHIP_IO_BTAG_COUNT(v));

#ifdef CHIP_IO_W1_BUS_START
#ifdef EXTENT_DEBUG
	printf("io: adding span 0x%lx to 0x%lx\n", CHIP_IO_W1_BUS_START(v),
	    CHIP_IO_W1_BUS_END(v));
#endif
	error = vmem_add(vm, CHIP_IO_W1_BUS_START(v),
	    CHIP_IO_W1_BUS_END(v) - CHIP_IO_W1_BUS_START(v) + 1, VM_NOSLEEP);
	KASSERT(error == 0);
#endif /* CHIP_IO_W1_BUS_START */

#ifdef CHIP_IO_W2_BUS_START
#ifdef EXTENT_DEBUG
	printf("io: adding span 0x%lx to 0x%lx\n", CHIP_IO_W2_BUS_START(v),
	    CHIP_IO_W2_BUS_END(v));
#endif
	error = vmem_add(vm, CHIP_IO_W2_BUS_START(v),
	    CHIP_IO_W2_BUS_END(v) - CHIP_IO_W2_BUS_START(v) + 1, VM_NOSLEEP);
#endif /* CHIP_IO_W2_BUS_START */

#ifdef EXTENT_DEBUG
	/* vmem_print(vm);			XXX */
#endif
	CHIP_IO_ARENA(v) = vm;
}

static int
__C(CHIP,_io_translate)(void *v, bus_addr_t ioaddr, bus_size_t iolen,
    int flags, struct alpha_bus_space_translation *abst)
{
	bus_addr_t ioend = ioaddr + (iolen - 1);
	int linear = flags & BUS_SPACE_MAP_LINEAR;

	/*
	 * Can't map i/o space linearly.
	 */
	if (linear)
		return (EOPNOTSUPP);

#ifdef CHIP_IO_W1_BUS_START
	if (ioaddr >= CHIP_IO_W1_BUS_START(v) &&
	    ioend <= CHIP_IO_W1_BUS_END(v))
		return (__C(CHIP,_io_get_window)(v, 0, abst));
#endif

#ifdef CHIP_IO_W2_BUS_START
	if (ioaddr >= CHIP_IO_W2_BUS_START(v) &&
	    ioend <= CHIP_IO_W2_BUS_END(v))
		return (__C(CHIP,_io_get_window)(v, 1, abst));
#endif

#ifdef EXTENT_DEBUG
	printf("\n");
#ifdef CHIP_IO_W1_BUS_START
	printf("%s: window[1]=0x%lx-0x%lx\n",
	    __S(__C(CHIP,_io_map)), CHIP_IO_W1_BUS_START(v),
	    CHIP_IO_W1_BUS_END(v));
#endif
#ifdef CHIP_IO_W2_BUS_START
	printf("%s: window[2]=0x%lx-0x%lx\n",
	    __S(__C(CHIP,_io_map)), CHIP_IO_W2_BUS_START(v),
	    CHIP_IO_W2_BUS_END(v));
#endif
#endif /* EXTENT_DEBUG */
	/* No translation. */
	return (EINVAL);
}

static int
__C(CHIP,_io_get_window)(void *v, int window,
    struct alpha_bus_space_translation *abst)
{

	switch (window) {
#ifdef CHIP_IO_W1_BUS_START
	case 0:
		abst->abst_bus_start = CHIP_IO_W1_BUS_START(v);
		abst->abst_bus_end = CHIP_IO_W1_BUS_END(v);
		abst->abst_sys_start = CHIP_IO_W1_SYS_START(v);
		abst->abst_sys_end = CHIP_IO_W1_SYS_END(v);
		abst->abst_addr_shift = CHIP_ADDR_SHIFT;
		abst->abst_size_shift = CHIP_SIZE_SHIFT;
		abst->abst_flags = 0;
		break;
#endif

#ifdef CHIP_IO_W2_BUS_START
	case 1:
		abst->abst_bus_start = CHIP_IO_W2_BUS_START(v);
		abst->abst_bus_end = CHIP_IO_W2_BUS_END(v);
		abst->abst_sys_start = CHIP_IO_W2_SYS_START(v);
		abst->abst_sys_end = CHIP_IO_W2_SYS_END(v);
		abst->abst_addr_shift = CHIP_ADDR_SHIFT;
		abst->abst_size_shift = CHIP_SIZE_SHIFT;
		abst->abst_flags = 0;
		break;
#endif

	default:
		panic(__S(__C(CHIP,_io_get_window)) ": invalid window %d",
		    window);
	}

	return (0);
}

static int
__C(CHIP,_io_map)(void *v, bus_addr_t ioaddr, bus_size_t iosize,
    int flags, bus_space_handle_t *iohp, int acct)
{
	struct alpha_bus_space_translation abst;
	int error;

	/*
	 * Get the translation for this address.
	 */
	error = __C(CHIP,_io_translate)(v, ioaddr, iosize, flags, &abst);
	if (error)
		return (error);

	if (acct == 0)
		goto mapit;

#ifdef EXTENT_DEBUG
	printf("io: allocating 0x%lx to 0x%lx\n", ioaddr, ioaddr + iosize - 1);
#endif
	error = vmem_xalloc_addr(CHIP_IO_ARENA(v), ioaddr, iosize, VM_NOSLEEP);
	if (error) {
#ifdef EXTENT_DEBUG
		printf("io: allocation failed (%d)\n", error);
		/* vmem_print(CHIP_IO_ARENA(v));	XXX */
#endif
		return (error);
	}

 mapit:
	*iohp = (ALPHA_PHYS_TO_K0SEG(abst.abst_sys_start) >>
	    CHIP_ADDR_SHIFT) + (ioaddr - abst.abst_bus_start);

	return (0);
}

static void
__C(CHIP,_io_unmap)(void *v, bus_space_handle_t ioh,
    bus_size_t iosize, int acct)
{
	bus_addr_t ioaddr;

	if (acct == 0)
		return;

#ifdef EXTENT_DEBUG
	printf("io: freeing handle 0x%lx for 0x%lx\n", ioh, iosize);
#endif

	ioh = ALPHA_K0SEG_TO_PHYS(ioh << CHIP_ADDR_SHIFT) >> CHIP_ADDR_SHIFT;

#ifdef CHIP_IO_W1_BUS_START
	if ((ioh << CHIP_ADDR_SHIFT) >= CHIP_IO_W1_SYS_START(v) &&
	    (ioh << CHIP_ADDR_SHIFT) <= CHIP_IO_W1_SYS_END(v)) {
		ioaddr = CHIP_IO_W1_BUS_START(v) +
		    (ioh - (CHIP_IO_W1_SYS_START(v) >> CHIP_ADDR_SHIFT));
	} else
#endif
#ifdef CHIP_IO_W2_BUS_START
	if ((ioh << CHIP_ADDR_SHIFT) >= CHIP_IO_W2_SYS_START(v) &&
	    (ioh << CHIP_ADDR_SHIFT) <= CHIP_IO_W2_SYS_END(v)) {
		ioaddr = CHIP_IO_W2_BUS_START(v) +
		    (ioh - (CHIP_IO_W2_SYS_START(v) >> CHIP_ADDR_SHIFT));
	} else
#endif
	{
		printf("\n");
#ifdef CHIP_IO_W1_BUS_START
		printf("%s: sys window[1]=0x%lx-0x%lx\n",
		    __S(__C(CHIP,_io_map)), CHIP_IO_W1_SYS_START(v),
		    CHIP_IO_W1_SYS_END(v));
#endif
#ifdef CHIP_IO_W2_BUS_START
		printf("%s: sys window[2]=0x%lx-0x%lx\n",
		    __S(__C(CHIP,_io_map)), CHIP_IO_W2_SYS_START(v),
		    CHIP_IO_W2_SYS_END(v));
#endif
		panic("%s: don't know how to unmap %lx",
		    __S(__C(CHIP,_io_unmap)), (ioh << CHIP_ADDR_SHIFT));
	}

#ifdef EXTENT_DEBUG
	printf("io: freeing 0x%lx to 0x%lx\n", ioaddr, ioaddr + iosize - 1);
#endif
	vmem_xfree(CHIP_IO_ARENA(v), ioaddr, iosize);
}

static int
__C(CHIP,_io_subregion)(void *v, bus_space_handle_t ioh,
    bus_size_t offset, bus_size_t size, bus_space_handle_t *nioh)
{

	*nioh = ioh + offset;
	return (0);
}

static int
__C(CHIP,_io_alloc)(void *v, bus_addr_t rstart, bus_addr_t rend,
    bus_size_t size, bus_size_t align, bus_size_t boundary, int flags,
    bus_addr_t *addrp, bus_space_handle_t *bshp)
{
	struct alpha_bus_space_translation abst;
	int linear = flags & BUS_SPACE_MAP_LINEAR;
	vmem_addr_t ioaddr;
	int error;

	/*
	 * Can't map i/o space linearly.
	 */
	if (linear)
		return (EOPNOTSUPP);

	/*
	 * Do the requested allocation.
	 */
#ifdef EXTENT_DEBUG
	printf("io: allocating from 0x%lx to 0x%lx\n", rstart, rend);
#endif
	error = vmem_xalloc(CHIP_IO_ARENA(v), size,
			    align,		/* align */
			    0,			/* phase */
			    boundary,		/* nocross */
			    rstart,		/* minaddr */
			    rend,		/* maxaddr */
			    VM_BESTFIT | VM_NOSLEEP,
			    &ioaddr);
	if (error) {
#ifdef EXTENT_DEBUG
		printf("io: allocation failed (%d)\n", error);
		extent_print(CHIP_IO_EXTENT(v));
#endif
		return (error);
	}

#ifdef EXTENT_DEBUG
	printf("io: allocated 0x%lx to 0x%lx\n", ioaddr, ioaddr + size - 1);
#endif

	error = __C(CHIP,_io_translate)(v, ioaddr, size, flags, &abst);
	if (error) {
		vmem_xfree(CHIP_IO_ARENA(v), ioaddr, size);
		return (error);
	}

	*addrp = ioaddr;
	*bshp = (ALPHA_PHYS_TO_K0SEG(abst.abst_sys_start) >>
	    CHIP_ADDR_SHIFT) + (ioaddr - abst.abst_bus_start);

	return (0);
}

static void
__C(CHIP,_io_free)(void *v, bus_space_handle_t bsh, bus_size_t size)
{

	/* Unmap does all we need to do. */
	__C(CHIP,_io_unmap)(v, bsh, size, 1);
}

static void *
__C(CHIP,_io_vaddr)(void *v, bus_space_handle_t bsh)
{
	/*
	 * _io_translate() catches BUS_SPACE_MAP_LINEAR,
	 * so we shouldn't get here
	 */
	panic("_io_vaddr");
}

static paddr_t
__C(CHIP,_io_mmap)(void *v, bus_addr_t addr, off_t off, int prot, int flags)
{

	/* Not supported for I/O space. */
	return (-1);
}

static inline void
__C(CHIP,_io_barrier)(void *v, bus_space_handle_t h,
    bus_size_t o, bus_size_t l, int f)
{

	if ((f & BUS_SPACE_BARRIER_READ) != 0)
		alpha_mb();
	else if ((f & BUS_SPACE_BARRIER_WRITE) != 0)
		alpha_wmb();
}

static inline uint8_t
__C(CHIP,_io_read_1)(void *v, bus_space_handle_t ioh, bus_size_t off)
{
	register bus_space_handle_t tmpioh;
	register uint32_t *port, val;
	register uint8_t rval;
	register int offset;

	alpha_mb();

	tmpioh = ioh + off;
	offset = tmpioh & 3;
	port = (uint32_t *)((tmpioh << CHIP_ADDR_SHIFT) |
	    (0 << CHIP_SIZE_SHIFT));
	val = *port;
	rval = ((val) >> (8 * offset)) & 0xff;

	return rval;
}

static inline uint16_t
__C(CHIP,_io_read_2)(void *v, bus_space_handle_t ioh, bus_size_t off)
{
	register bus_space_handle_t tmpioh;
	register uint32_t *port, val;
	register uint16_t rval;
	register int offset;

	alpha_mb();

	tmpioh = ioh + off;
	offset = tmpioh & 3;
	port = (uint32_t *)((tmpioh << CHIP_ADDR_SHIFT) |
	    (1 << CHIP_SIZE_SHIFT));
	val = *port;
	rval = ((val) >> (8 * offset)) & 0xffff;

	return rval;
}

static inline uint32_t
__C(CHIP,_io_read_4)(void *v, bus_space_handle_t ioh, bus_size_t off)
{
	register bus_space_handle_t tmpioh;
	register uint32_t *port, val;
	register uint32_t rval;

	alpha_mb();

	tmpioh = ioh + off;
	port = (uint32_t *)((tmpioh << CHIP_ADDR_SHIFT) |
	    (3 << CHIP_SIZE_SHIFT));
	val = *port;
#if 0
	int offset = tmpioh & 3;
	rval = ((val) >> (8 * offset)) & 0xffffffff;
#else
	rval = val;
#endif

	return rval;
}

static inline uint64_t
__C(CHIP,_io_read_8)(void *v, bus_space_handle_t ioh, bus_size_t off)
{

	/* XXX XXX XXX */
	panic("%s not implemented", __S(__C(CHIP,_io_read_8)));
}

#define CHIP_io_read_multi_N(BYTES,TYPE)				\
static void								\
__C(__C(CHIP,_io_read_multi_),BYTES)(void *v, bus_space_handle_t h, bus_size_t o, TYPE *a, bus_size_t c)			\
{									\
									\
	while (c-- > 0) {						\
		__C(CHIP,_io_barrier)(v, h, o, sizeof *a,		\
		    BUS_SPACE_BARRIER_READ);				\
		*a++ = __C(__C(CHIP,_io_read_),BYTES)(v, h, o);		\
	}								\
}
CHIP_io_read_multi_N(1,uint8_t)
CHIP_io_read_multi_N(2,uint16_t)
CHIP_io_read_multi_N(4,uint32_t)
CHIP_io_read_multi_N(8,uint64_t)

#define CHIP_io_read_region_N(BYTES,TYPE)				\
static void								\
__C(__C(CHIP,_io_read_region_),BYTES)(void *v, bus_space_handle_t h, bus_size_t o, TYPE *a, bus_size_t c)			\
{									\
									\
	while (c-- > 0) {						\
		*a++ = __C(__C(CHIP,_io_read_),BYTES)(v, h, o);		\
		o += sizeof *a;						\
	}								\
}
CHIP_io_read_region_N(1,uint8_t)
CHIP_io_read_region_N(2,uint16_t)
CHIP_io_read_region_N(4,uint32_t)
CHIP_io_read_region_N(8,uint64_t)

static inline void
__C(CHIP,_io_write_1)(void *v, bus_space_handle_t ioh,
    bus_size_t off, uint8_t val)
{
	register bus_space_handle_t tmpioh;
	register uint32_t *port, nval;
	register int offset;

	tmpioh = ioh + off;
	offset = tmpioh & 3;
	nval = val << (8 * offset);
	port = (uint32_t *)((tmpioh << CHIP_ADDR_SHIFT) |
	    (0 << CHIP_SIZE_SHIFT));
	*port = nval;
	alpha_mb();
}

static inline void
__C(CHIP,_io_write_2)(void *v, bus_space_handle_t ioh,
    bus_size_t off, uint16_t val)
{
	register bus_space_handle_t tmpioh;
	register uint32_t *port, nval;
	register int offset;

	tmpioh = ioh + off;
	offset = tmpioh & 3;
	nval = val << (8 * offset);
	port = (uint32_t *)((tmpioh << CHIP_ADDR_SHIFT) |
	    (1 << CHIP_SIZE_SHIFT));
	*port = nval;
	alpha_mb();
}

static inline void
__C(CHIP,_io_write_4)(void *v, bus_space_handle_t ioh,
    bus_size_t off, uint32_t val)
{
	register bus_space_handle_t tmpioh;
	register uint32_t *port, nval;

	tmpioh = ioh + off;
#if 0
	register int offset;
	offset = tmpioh & 3;
#endif
	nval = val /*<< (8 * offset)*/;
	port = (uint32_t *)((tmpioh << CHIP_ADDR_SHIFT) |
	    (3 << CHIP_SIZE_SHIFT));
	*port = nval;
	alpha_mb();
}

static inline void
__C(CHIP,_io_write_8)(void *v, bus_space_handle_t ioh,
    bus_size_t off, uint64_t val)
{

	/* XXX XXX XXX */
	panic("%s not implemented", __S(__C(CHIP,_io_write_8)));
	alpha_mb();
}

#define CHIP_io_write_multi_N(BYTES,TYPE)				\
static void								\
__C(__C(CHIP,_io_write_multi_),BYTES)(void *v, bus_space_handle_t h, bus_size_t o, const TYPE *a, bus_size_t c)			\
{									\
									\
	while (c-- > 0) {						\
		__C(__C(CHIP,_io_write_),BYTES)(v, h, o, *a++);		\
		__C(CHIP,_io_barrier)(v, h, o, sizeof *a,		\
		    BUS_SPACE_BARRIER_WRITE);				\
	}								\
}
CHIP_io_write_multi_N(1,uint8_t)
CHIP_io_write_multi_N(2,uint16_t)
CHIP_io_write_multi_N(4,uint32_t)
CHIP_io_write_multi_N(8,uint64_t)

#define CHIP_io_write_region_N(BYTES,TYPE)				\
static void								\
__C(__C(CHIP,_io_write_region_),BYTES)(void *v, bus_space_handle_t h, bus_size_t o, const TYPE *a, bus_size_t c)			\
{									\
									\
	while (c-- > 0) {						\
		__C(__C(CHIP,_io_write_),BYTES)(v, h, o, *a++);		\
		o += sizeof *a;						\
	}								\
}
CHIP_io_write_region_N(1,uint8_t)
CHIP_io_write_region_N(2,uint16_t)
CHIP_io_write_region_N(4,uint32_t)
CHIP_io_write_region_N(8,uint64_t)

#define CHIP_io_set_multi_N(BYTES,TYPE)					\
static void								\
__C(__C(CHIP,_io_set_multi_),BYTES)(void *v, bus_space_handle_t h, bus_size_t o, TYPE val, bus_size_t c)			\
{									\
									\
	while (c-- > 0) {						\
		__C(__C(CHIP,_io_write_),BYTES)(v, h, o, val);		\
		__C(CHIP,_io_barrier)(v, h, o, sizeof val,		\
		    BUS_SPACE_BARRIER_WRITE);				\
	}								\
}
CHIP_io_set_multi_N(1,uint8_t)
CHIP_io_set_multi_N(2,uint16_t)
CHIP_io_set_multi_N(4,uint32_t)
CHIP_io_set_multi_N(8,uint64_t)

#define CHIP_io_set_region_N(BYTES,TYPE)				\
static void								\
__C(__C(CHIP,_io_set_region_),BYTES)(void *v, bus_space_handle_t h, bus_size_t o, TYPE val, bus_size_t c)			\
{									\
									\
	while (c-- > 0) {						\
		__C(__C(CHIP,_io_write_),BYTES)(v, h, o, val);		\
		o += sizeof val;					\
	}								\
}
CHIP_io_set_region_N(1,uint8_t)
CHIP_io_set_region_N(2,uint16_t)
CHIP_io_set_region_N(4,uint32_t)
CHIP_io_set_region_N(8,uint64_t)

#define	CHIP_io_copy_region_N(BYTES)					\
static void								\
__C(__C(CHIP,_io_copy_region_),BYTES)(void *v, bus_space_handle_t h1, bus_size_t o1, bus_space_handle_t h2, bus_size_t o2, bus_size_t c)		\
{									\
	bus_size_t o;							\
									\
	if ((h1 + o1) >= (h2 + o2)) {					\
		/* src after dest: copy forward */			\
		for (o = 0; c != 0; c--, o += BYTES)			\
			__C(__C(CHIP,_io_write_),BYTES)(v, h2, o2 + o,	\
			    __C(__C(CHIP,_io_read_),BYTES)(v, h1, o1 + o)); \
	} else {							\
		/* dest after src: copy backwards */			\
		for (o = (c - 1) * BYTES; c != 0; c--, o -= BYTES)	\
			__C(__C(CHIP,_io_write_),BYTES)(v, h2, o2 + o,	\
			    __C(__C(CHIP,_io_read_),BYTES)(v, h1, o1 + o)); \
	}								\
}
CHIP_io_copy_region_N(1)
CHIP_io_copy_region_N(2)
CHIP_io_copy_region_N(4)
CHIP_io_copy_region_N(8)
