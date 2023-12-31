/* $NetBSD: apecs_bus_mem.c,v 1.13 2023/12/04 00:32:10 thorpej Exp $ */

/*
 * Copyright (c) 1996 Carnegie-Mellon University.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(1, "$NetBSD: apecs_bus_mem.c,v 1.13 2023/12/04 00:32:10 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/syslog.h>
#include <sys/device.h>

#include <sys/bus.h>

#include <alpha/pci/apecsreg.h>
#include <alpha/pci/apecsvar.h>

#define	CHIP	apecs

#define	CHIP_D_MEM_ARENA(v)	(((struct apecs_config *)(v))->ac_d_mem_arena)
#define	CHIP_S_MEM_ARENA(v)	(((struct apecs_config *)(v))->ac_s_mem_arena)

/* Dense region 1 */
#define	CHIP_D_MEM_W1_BUS_START(v)	0x00000000UL
#define	CHIP_D_MEM_W1_BUS_END(v)	0xffffffffUL
#define	CHIP_D_MEM_W1_SYS_START(v)	APECS_PCI_DENSE
#define	CHIP_D_MEM_W1_SYS_END(v)	(APECS_PCI_DENSE + 0xffffffffUL)

/* Sparse region 1 */
#define	CHIP_S_MEM_W1_BUS_START(v)	0x00000000UL
#define	CHIP_S_MEM_W1_BUS_END(v)	0x00ffffffUL
#define	CHIP_S_MEM_W1_SYS_START(v)	APECS_PCI_SPARSE
#define	CHIP_S_MEM_W1_SYS_END(v)					\
    (APECS_PCI_SPARSE + (0x01000000UL << 5) - 1)

/* Sparse region 2 */
#define	CHIP_S_MEM_W2_BUS_START(v)					\
    ((((struct apecs_config *)(v))->ac_haxr1 & EPIC_HAXR1_EADDR) +	\
      0x01000000UL)
#define	CHIP_S_MEM_W2_BUS_END(v)					\
    ((((struct apecs_config *)(v))->ac_haxr1 & EPIC_HAXR1_EADDR) +	\
      0x07ffffffUL)
#define	CHIP_S_MEM_W2_SYS_START(v)					\
    (APECS_PCI_SPARSE + (0x01000000UL << 5))
#define	CHIP_S_MEM_W2_SYS_END(v)					\
    (APECS_PCI_SPARSE + (0x08000000UL << 5) - 1)

#include <alpha/pci/pci_swiz_bus_mem_chipdep.c>
