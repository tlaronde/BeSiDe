/* $NetBSD: ttwoga_bus_io.c,v 1.5 2023/12/04 00:32:10 thorpej Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(1, "$NetBSD: ttwoga_bus_io.c,v 1.5 2023/12/04 00:32:10 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/syslog.h>
#include <sys/device.h>

#include <sys/bus.h>

#include <alpha/pci/ttwogareg.h>
#include <alpha/pci/ttwogavar.h>

#define	CHIP		ttwoga

#define	CHIP_V(v)		((struct ttwoga_config *)(v))

#define	CHIP_IO_ARENA(v)	(CHIP_V(v)->tc_io_arena)
#define	CHIP_IO_ARENA_STORE(v)	(&(CHIP_V(v)->tc_io_arena_store))
#define	CHIP_IO_BTAG_STORE(v)	(CHIP_V(v)->tc_io_btag_store)
#define	CHIP_IO_BTAG_COUNT(v)	TTWOGA_NBTS

/* IO region 1 */
#define	CHIP_IO_W1_BUS_START(v)						\
    (CHIP_V(v)->tc_io_bus_start)
#define	CHIP_IO_W1_BUS_END(v)						\
    (CHIP_IO_W1_BUS_START(v) + CHIP_V(v)->tc_sysmap->tsmap_sio_bussize - 1)
#define	CHIP_IO_W1_SYS_START(v)						\
    (ttwoga_gamma_cbus_bias + CHIP_V(v)->tc_sysmap->tsmap_sio_base)
#define	CHIP_IO_W1_SYS_END(v)						\
    (CHIP_IO_W1_SYS_START(v) + CHIP_V(v)->tc_sysmap->tsmap_sio_syssize - 1)

#include <alpha/pci/pci_swiz_bus_io_chipdep.c>
