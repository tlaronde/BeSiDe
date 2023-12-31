/*	$NetBSD: bcm2835_spi.c,v 1.13 2023/09/03 11:36:52 tnn Exp $	*/

/*
 * Copyright (c) 2012 Jonathan A. Kollasch
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bcm2835_spi.c,v 1.13 2023/09/03 11:36:52 tnn Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/mutex.h>
#include <sys/bus.h>
#include <sys/intr.h>
#include <sys/kernel.h>

#include <sys/bitops.h>
#include <dev/spi/spivar.h>

#include <arm/broadcom/bcm2835reg.h>
#include <arm/broadcom/bcm2835_spireg.h>

#include <dev/fdt/fdtvar.h>

#include <arm/fdt/arm_fdtvar.h>

struct bcmspi_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	void			*sc_intrh;
	struct spi_controller	sc_spi;
	kmutex_t                sc_mutex;
	SIMPLEQ_HEAD(,spi_transfer) sc_q;
	struct spi_transfer	*sc_transfer;
	struct spi_chunk	*sc_wchunk;
	struct spi_chunk	*sc_rchunk;
	uint32_t		sc_CS;
	volatile bool		sc_running;
};

static int bcmspi_match(device_t, cfdata_t, void *);
static void bcmspi_attach(device_t, device_t, void *);

static int bcmspi_configure(void *, int, int, int);
static int bcmspi_transfer(void *, struct spi_transfer *);

static void bcmspi_start(struct bcmspi_softc * const);
static int bcmspi_intr(void *);

static void bcmspi_send(struct bcmspi_softc * const);
static void bcmspi_recv(struct bcmspi_softc * const);

CFATTACH_DECL_NEW(bcmspi, sizeof(struct bcmspi_softc),
    bcmspi_match, bcmspi_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "brcm,bcm2835-spi" },
	DEVICE_COMPAT_EOL
};

static int
bcmspi_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
bcmspi_attach(device_t parent, device_t self, void *aux)
{
	struct bcmspi_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	struct spibus_attach_args sba;

	aprint_naive("\n");
	aprint_normal(": SPI\n");

	sc->sc_dev = self;
	sc->sc_iot = faa->faa_bst;
	SIMPLEQ_INIT(&sc->sc_q);

	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": missing 'reg' property\n");
		return;
	}

	if (bus_space_map(sc->sc_iot, addr, size, 0, &sc->sc_ioh) != 0) {
		aprint_error_dev(sc->sc_dev, "unable to map device\n");
		return;
	}

	char intrstr[128];
	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": failed to decode interrupt\n");
		return;
	}

	sc->sc_intrh = fdtbus_intr_establish_xname(phandle, 0, IPL_VM, 0,
	    bcmspi_intr, sc, device_xname(self));
	if (sc->sc_intrh == NULL) {
		aprint_error_dev(sc->sc_dev, "unable to establish interrupt\n");
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	sc->sc_spi.sct_cookie = sc;
	mutex_init(&sc->sc_mutex, MUTEX_DEFAULT, IPL_VM);
	sc->sc_spi.sct_configure = bcmspi_configure;
	sc->sc_spi.sct_transfer = bcmspi_transfer;
	sc->sc_spi.sct_nslaves = 3;

	memset(&sba, 0, sizeof(sba));
	sba.sba_controller = &sc->sc_spi;

	config_found(self, &sba, spibus_print, CFARGS_NONE);
}

static int
bcmspi_configure(void *cookie, int slave, int mode, int speed)
{
	struct bcmspi_softc * const sc = cookie;
	uint32_t cs, clk;

	cs = SPI_CS_INTR | SPI_CS_INTD;

	if (slave > 2)
		return EINVAL;

	if (speed <= 0)
		return EINVAL;

	switch (mode) {
	case SPI_MODE_0:
		cs |= 0;
		break;
	case SPI_MODE_1:
		cs |= SPI_CS_CPHA;
		break;
	case SPI_MODE_2:
		cs |= SPI_CS_CPOL;
		break;
	case SPI_MODE_3:
		cs |= SPI_CS_CPHA|SPI_CS_CPOL;
		break;
	default:
		return EINVAL;
	}

	sc->sc_CS = cs;

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SPI_CS, cs);

	clk = 2 * 250000000 / speed; /* XXX 250MHz */
	clk = (clk / 2) + (clk & 1);
	clk = roundup(clk, 2);
	if (clk >= 0xfffe)
		clk = 0xfffe;
	clk = __SHIFTIN(clk, SPI_CLK_CDIV);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SPI_CLK, clk);

	return 0;
}

static int
bcmspi_transfer(void *cookie, struct spi_transfer *st)
{
	struct bcmspi_softc * const sc = cookie;

	mutex_enter(&sc->sc_mutex);
	spi_transq_enqueue(&sc->sc_q, st);
	if (sc->sc_running == false) {
		bcmspi_start(sc);
	}
	mutex_exit(&sc->sc_mutex);
	return 0;
}

static void
bcmspi_start(struct bcmspi_softc * const sc)
{
	struct spi_transfer *st;
	uint32_t cs;

	while ((st = spi_transq_first(&sc->sc_q)) != NULL) {

		spi_transq_dequeue(&sc->sc_q);

		KASSERT(sc->sc_transfer == NULL);
		sc->sc_transfer = st;
		sc->sc_rchunk = sc->sc_wchunk = st->st_chunks;

		cs = sc->sc_CS;
		cs |= SPI_CS_TA;
		cs |= SPI_CS_CLEAR_TX;
		cs |= SPI_CS_CLEAR_RX;
		KASSERT(st->st_slave <= 2);
		cs |= __SHIFTIN(st->st_slave, SPI_CS_CS);
		sc->sc_running = true;
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, SPI_CS, cs);

		if (!cold)
			return;

		for (;;) {
		        mutex_exit(&sc->sc_mutex);
			bcmspi_intr(sc);
			mutex_enter(&sc->sc_mutex);
			if (ISSET(st->st_flags, SPI_F_DONE))
				break;
		}
	}

	sc->sc_running = false;
}

static void
bcmspi_send(struct bcmspi_softc * const sc)
{
	uint32_t fd;
	uint32_t cs;
	struct spi_chunk *chunk;

	while ((chunk = sc->sc_wchunk) != NULL) {
		while (chunk->chunk_wresid) {
			cs = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SPI_CS);
			if ((cs & SPI_CS_TXD) == 0)
				return;
			if (chunk->chunk_wptr) {
				fd = *chunk->chunk_wptr++;
			} else {
				fd = '\0';
			}
			bus_space_write_4(sc->sc_iot, sc->sc_ioh, SPI_FIFO, fd);
			chunk->chunk_wresid--;
		}
		sc->sc_wchunk = sc->sc_wchunk->chunk_next;
	}
}

static void
bcmspi_recv(struct bcmspi_softc * const sc)
{
	uint32_t fd;
	uint32_t cs;
	struct spi_chunk *chunk;

	while ((chunk = sc->sc_rchunk) != NULL) {
		while (chunk->chunk_rresid) {
			cs = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SPI_CS);
			if ((cs & SPI_CS_RXD) == 0)
				return;
			fd = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SPI_FIFO);
			if (chunk->chunk_rptr) {
				*chunk->chunk_rptr++ = fd & 0xff;
			}
			chunk->chunk_rresid--;
		}
		sc->sc_rchunk = sc->sc_rchunk->chunk_next;
	}
}

static int
bcmspi_intr(void *cookie)
{
	struct bcmspi_softc * const sc = cookie;
	struct spi_transfer *st;
	uint32_t cs;

	mutex_enter(&sc->sc_mutex);
	cs = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SPI_CS);
	if (ISSET(cs, SPI_CS_DONE)) {
		if (sc->sc_wchunk != NULL) {
			bcmspi_send(sc);
		} else {
			bus_space_write_4(sc->sc_iot, sc->sc_ioh, SPI_CS,
			    sc->sc_CS);
			bcmspi_recv(sc);
			sc->sc_rchunk = sc->sc_wchunk = NULL;
			st = sc->sc_transfer;
			sc->sc_transfer = NULL;
			KASSERT(st != NULL);
			spi_done(st, 0);
			sc->sc_running = false;
		}
	} else if (ISSET(cs, SPI_CS_RXR)) {
		bcmspi_recv(sc);
		bcmspi_send(sc);
	}

	mutex_exit(&sc->sc_mutex);
	return ISSET(cs, SPI_CS_DONE|SPI_CS_RXR);
}
