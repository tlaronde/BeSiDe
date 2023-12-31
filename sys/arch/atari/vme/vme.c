/*	$NetBSD: vme.c,v 1.21 2023/12/20 00:40:43 thorpej Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: vme.c,v 1.21 2023/12/20 00:40:43 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/device.h>

#include <machine/intr.h>

#include <atari/vme/vmevar.h>

static int vmematch(device_t, cfdata_t, void *);
static void vmeattach(device_t, device_t, void *);
static int vmeprint(void *, const char *);

CFATTACH_DECL_NEW(vme, sizeof(struct vme_softc),
    vmematch, vmeattach, NULL, NULL);

static int vmesearch(device_t, cfdata_t, const int *, void *);

static int
vmematch(device_t parent, cfdata_t cf, void *aux)
{
	struct vmebus_attach_args *vba = aux;

	if (strcmp(vba->vba_busname, cf->cf_name))
		return 0;

        return 1;
}

static void
vmeattach(device_t parent, device_t self, void *aux)
{
	struct vme_softc *sc = device_private(self);
	struct vmebus_attach_args *vba = aux;

	aprint_normal("\n");

	sc->sc_dev = self;
	sc->sc_iot  = vba->vba_iot;
	sc->sc_memt = vba->vba_memt;
	sc->sc_vc   = vba->vba_vc;

	config_search(self, NULL,
	    CFARGS(.search = vmesearch));
}

static int
vmeprint(void *aux, const char *vme)
{
	struct vme_attach_args *va = aux;

	if (va->va_iosize)
		aprint_normal(" port 0x%x", va->va_iobase);
	if (va->va_iosize > 1)
		aprint_normal("-0x%x", va->va_iobase + va->va_iosize - 1);
	if (va->va_msize)
		aprint_normal(" iomem 0x%x", va->va_maddr);
	if (va->va_msize > 1)
		aprint_normal("-0x%x", va->va_maddr + va->va_msize - 1);
	if (va->va_irq != IRQUNK)
		aprint_normal(" irq %d", va->va_irq);
	return UNCONF;
}

static int
vmesearch(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct vme_softc *sc = device_private(parent);
	struct vme_attach_args va;

	va.va_iot    = sc->sc_iot;
	va.va_memt   = sc->sc_memt;
	va.va_vc     = sc->sc_vc;
	va.va_iobase = cf->cf_iobase;
	va.va_iosize = cf->cf_iosize;
	va.va_maddr  = cf->cf_maddr;
	va.va_msize  = cf->cf_msize;
	va.va_irq    = cf->cf_irq;

	if (config_probe(parent, cf, &va))
		config_attach(parent, cf, &va, vmeprint, CFARGS_NONE);
	return 0;
}
