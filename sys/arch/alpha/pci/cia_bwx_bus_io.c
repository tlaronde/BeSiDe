/* $NetBSD: cia_bwx_bus_io.c,v 1.7 2023/12/04 00:32:10 thorpej Exp $ */

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

__KERNEL_RCSID(1, "$NetBSD: cia_bwx_bus_io.c,v 1.7 2023/12/04 00:32:10 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/syslog.h>
#include <sys/device.h>

#include <sys/bus.h>

#include <alpha/pci/ciareg.h>
#include <alpha/pci/ciavar.h>

#define	CHIP		cia_bwx

#define	CHIP_IO_ARENA(v)	(((struct cia_config *)(v))->cc_io_arena)

#define	CHIP_IO_SYS_START(v)	CIA_EV56_BWIO

/*
 * CIA core logic with BWX enabled appears on EV56 and PCA56.  We
 * require at least EV56 support for the assembler to emit BWX opcodes.
 */
__asm(".arch ev56");

#include <alpha/pci/pci_bwx_bus_io_chipdep.c>
