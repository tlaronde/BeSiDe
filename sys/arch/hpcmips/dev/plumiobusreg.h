/*	$NetBSD: plumiobusreg.h,v 1.4 2023/12/08 22:11:15 andvar Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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

/* (CS3) */
#define PLUM_IOBUS_REGBASE		0x6000
#define PLUM_IOBUS_REGSIZE		0x1000

/* I/O bus width setting */
#define PLUM_IOBUS_IOXBSZ_REG		0x000
#define PLUM_IOBUS_IOXBSZ_IO5BE5	0x00000020
#define PLUM_IOBUS_IOXBSZ_IO5BE4	0x00000010
#define PLUM_IOBUS_IOXBSZ_IO5BE3	0x00000008
#define PLUM_IOBUS_IOXBSZ_IO5BE2	0x00000004
#define PLUM_IOBUS_IOXBSZ_IO5BE1	0x00000002
#define PLUM_IOBUS_IOXBSZ_IO5BE0	0x00000001

/* I/O bus wait control 1 (# of wait from the access beginning) */
#define PLUM_IOBUS_IOXCCNT_REG		0x004
#define PLUM_IOBUS_IOXCCNT_MASK		0x7
/* I/O bus wait control 2 (# of wait in access) */
#define PLUM_IOBUS_IOXACNT_REG		0x008
#define PLUM_IOBUS_IOXACNT_MASK		0x1f
#define PLUM_IOBUS_IOXACNT_SHIFT	5
/* I/O bus wait control 3 (# of wait during access) */
#define PLUM_IOBUS_IOXSCNT_REG		0x00c
#define PLUM_IOBUS_IOXSCNT_MASK		0x7
/* IDE mode setting */
#define PLUM_IOBUS_IDEMODE_REG		0x010
#define PLUM_IOBUS_IDEMODE		0x00000001

/* (MCS0) */
#define PLUM_IOBUS_IOBASE		0x00410000
#define PLUM_IOBUS_IOSIZE		0x6000

#define PLUM_IOBUS_IO5CS0BASE		0x0000
#define PLUM_IOBUS_IO5CS1BASE		0x1000
#define PLUM_IOBUS_IO5CS2BASE		0x2000
#define PLUM_IOBUS_IO5CS3BASE		0x3000
#define PLUM_IOBUS_IO5CS4BASE		0x4000
#define PLUM_IOBUS_IO5CS5BASE		0x5000
#define PLUM_IOBUS_IO5SIZE		0x1000
