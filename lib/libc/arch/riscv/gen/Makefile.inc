#	$NetBSD: Makefile.inc,v 1.3 2023/09/14 03:58:50 rin Exp $

SRCS+=	__setjmp14.S __sigsetjmp14.S _setjmp.S
SRCS+=	bswap16.c bswap32.c bswap64.c
SRCS+=	flt_rounds.c
SRCS+=  _lwp.c makecontext.c resumecontext.c swapcontext.S

# Common ieee754 constants and functions
SRCS+=	infinityf_ieee754.c infinity_ieee754.c infinityl_ieee754.c
SRCS+=	fpclassifyf_ieee754.c fpclassifyd_ieee754.c fpclassifyl_ieee754.c
SRCS+=	isinff_ieee754.c isinfd_ieee754.c isinfl_ieee754.c
SRCS+=	isnanf_ieee754.c isnand_ieee754.c isnanl_ieee754.c
SRCS+=	isfinitef_ieee754.c isfinited_ieee754.c isfinitel_ieee754.c
SRCS+=	signbitf_ieee754.c signbitd_ieee754.c signbitl_ieee754.c
SRCS+=	fabs_ieee754.c

SRCS+=	nanf.c

.if (${MKSOFTFLOAT} == "no")
SRCS+=	fpgetround.c fpsetround.c fpgetmask.c fpsetmask.c
SRCS+=	fpgetsticky.c fpsetsticky.c
.endif

CPPFLAGS._lwp.c	+= -D_LIBC_SOURCE

LSRCS.riscv.gen=Lint_swapcontext.c
LSRCS+=		${LSRCS.riscv.gen}
DPSRCS+=	${LSRCS.riscv.gen}
CLEANFILES+=	${LSRCS.riscv.gen}
