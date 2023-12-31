/*	$NetBSD: machdep.c,v 1.31 2023/12/20 14:18:37 thorpej Exp $ */

/*
 * Copyright (c) 2006 Jachym Holecek
 * All rights reserved.
 *
 * Written for DFC Design, s.r.o.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Based on Walnut and Explora.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.31 2023/12/20 14:18:37 thorpej Exp $");

#include "opt_compat_netbsd.h"
#include "opt_ddb.h"
#include "opt_virtex.h"
#include "opt_kgdb.h"

#include <sys/param.h>
#include <sys/boot_flag.h>
#include <sys/buf.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/exec.h>
#include <sys/mbuf.h>
#include <sys/module.h>
#include <sys/mount.h>
#include <sys/msgbuf.h>
#include <sys/kernel.h>
#include <sys/ksyms.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/syscallargs.h>
#include <sys/syslog.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>

#include <machine/powerpc.h>

#include <powerpc/trap.h>
#include <powerpc/pcb.h>

#include <powerpc/spr.h>
#include <powerpc/ibm4xx/spr.h>

#include <powerpc/ibm4xx/cpu.h>

#include <evbppc/virtex/dcr.h>
#include <evbppc/virtex/virtex.h>

#include "ksyms.h"

#if defined(DDB)
#include <powerpc/db_machdep.h>
#include <ddb/db_extern.h>
#endif

#if defined(KGDB)
#include <sys/kgdb.h>
#endif

void initppc(vaddr_t, vaddr_t);

/* BSS segment start & end. */
extern char 		edata[], end[];

/* One region holds all memory, the other is terminator expected by 405 pmap. */
#define MEMREGIONS 	2
struct mem_region 	physmemr[MEMREGIONS];
struct mem_region 	availmemr[MEMREGIONS];

/* Maximum TLB page size. */
#define TLB_PG_SIZE 	(16*1024*1024)

void
initppc(vaddr_t startkernel, vaddr_t endkernel)
{
	paddr_t			addr, memsize;

        /* Initialize cache info for memcpy, memset, etc. */
        cpu_probe_cache();

	memset(physmemr, 0, sizeof(physmemr)); 		/* [1].size = 0 */
	memset(availmemr, 0, sizeof(availmemr)); 	/* [1].size = 0 */

	memsize = (PHYSMEM * 1024 * 1024) & ~PGOFSET;

	physmemr[0].start 	= 0;
	physmemr[0].size 	= memsize;

	availmemr[0].start 	= startkernel;
	availmemr[0].size 	= memsize - availmemr[0].start;

	/* Map kernel memory linearly. */
	for (addr = 0; addr < endkernel; addr += TLB_PG_SIZE)
		ppc4xx_tlb_reserve(addr, addr, TLB_PG_SIZE, TLB_EX);

	/* Give design-specific code a hint for reserved mappings. */
	virtex_machdep_init(roundup(memsize, TLB_PG_SIZE), TLB_PG_SIZE,
	    physmemr, availmemr);

	ibm4xx_init(startkernel, endkernel, pic_ext_intr);

#ifdef DDB
	if (boothowto & RB_KDB)
		Debugger();
#endif

#ifdef KGDB
	/*
	 * Now trap to KGDB
	 */
	kgdb_connect(1);
#endif /* KGDB */
}

/*
 * Machine dependent startup code.
 */

void
cpu_startup(void)
{
	/* For use by propdb. */
	static u_int 	memsize = PHYSMEM * 1024 * 1024;
	static u_int 	cpuspeed = CPUFREQ * 1000 * 1000;
	prop_number_t 	pn;

	vaddr_t 	minaddr, maxaddr;
	char 		pbuf[9];

	curcpu()->ci_khz = cpuspeed / 1000;

	/* Initialize error message buffer. */
	initmsgbuf((void *)msgbuf, round_page(MSGBUFSIZE));

	printf("%s%s", copyright, version);

	format_bytes(pbuf, sizeof(pbuf), ctob(physmem));
	printf("total memory = %s\n", pbuf);

	minaddr = 0;
	/*
	 * Allocate a submap for physio
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				 VM_PHYS_SIZE, 0, false, NULL);

	/*
	 * No need to allocate an mbuf cluster submap.  Mbuf clusters
	 * are allocated via the pool allocator, and we use direct-mapped
	 * pool pages.
	 */

	format_bytes(pbuf, sizeof(pbuf), ptoa(uvm_availmem(false)));
	printf("avail memory = %s\n", pbuf);

	/*
	 * Set up the board properties database.
	 */
	board_info_init();

	pn = prop_number_create_integer(memsize);
	KASSERT(pn != NULL);
	if (prop_dictionary_set(board_properties, "mem-size", pn) == false)
		panic("setting mem-size");
	prop_object_release(pn);

	pn = prop_number_create_integer(cpuspeed);
	KASSERT(pn != NULL);
	if (prop_dictionary_set(board_properties, "processor-frequency",
	    pn) == false)
		panic("setting processor-frequency");
	prop_object_release(pn);

	/*
	 * Now that we have VM, malloc()s are OK in bus_space.
	 */
	bus_space_mallocok();
	fake_mapiodev = 0;
}

/* Hook used by 405 pmap module. */
void
mem_regions(struct mem_region **mem, struct mem_region **avail)
{
	*mem 	= physmemr;
	*avail 	= availmemr;
}
