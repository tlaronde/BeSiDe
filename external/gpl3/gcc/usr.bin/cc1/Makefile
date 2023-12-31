#	$NetBSD: Makefile,v 1.21 2023/08/01 05:57:54 mrg Exp $

PROG=		cc1
SRCS=		${G_C_OBJS:S,c-family/,,:S,c/,,:S,known-headers.o,known-headers.cc,:S,c-spellcheck.o,c-spellcheck.cc,:Nlibcpp.a:.o=.cc}
SRCS+=		main.cc ${PROG}-checksum.cc
CPPFLAGS+=	-DPREFIX=\"/usr\"

.include "../Makefile.backend"
.include "../Makefile.backtrace"
.include "../Makefile.libcpp"
.include "../Makefile.libdecnumber"

CHECKSUM_OBJS=	${BACKENDOBJ}/libbackend.a \
		main.o \
		${LIBBACKTRACEOBJ}/libbacktrace.a \
		${COMMONTARGETOBJ}/libcommon-target.a \
		${COMMONOBJ}/libcommon.a \
		${LIBCPPOBJ}/libcpp.a \
		${LIBDECNUMBEROBJ}/libdecnumber.a \
		${LIBIBERTYOBJ}/libiberty.a \
		${G_C_OBJS:S,c-family/,,:S,c/,,} \
		checksum-options

.include "../Makefile.checksum"

# XXX check this out, probably need it for plugins?
# LDFLAGS+=	-rdynamic

.if 0
COPTS.c-ada-spec.cc=	-Wno-stack-protector
COPTS.c-attribs.cc=	-Wno-stack-protector
COPTS.c-aux-info.cc=	-Wno-stack-protector
COPTS.c-common.cc=	-Wno-stack-protector
COPTS.c-cppbuiltin.cc=	-Wno-stack-protector
COPTS.c-decl.cc=	-Wno-stack-protector
COPTS.c-format.cc=	-Wno-stack-protector
COPTS.c-lex.cc=		-Wno-stack-protector
COPTS.c-opts.cc=	-Wno-stack-protector
COPTS.c-ppoutput.cc=	-Wno-stack-protector
COPTS.c-pragma.cc=	-Wno-stack-protector
COPTS.c-typeck.cc=	-Wno-stack-protector
.endif

CPPFLAGS.default-c.cc+=	-I${BACKENDOBJ}

.if ${MACHINE_CPU} == "sh3"
BUILDSYMLINKS+= ${GNUHOSTDIST}/gcc/config/sh/sh-c.cc sh-c.c
.endif

.if ${MACHINE_CPU} == "vax" && 0
COPTS.c-decl.c+=-O0
COPTS.c-typeck.c+=-O0
COPTS.c-array-notation.c+=-O0
COPTS.c-common.c+=-O0
COPTS.c-ada-spec.c+=-O0
COPTS.cilk.c+=-O0
COPTS.c-ubsan.c+=-O0
COPTS.cc1-checksum.c+=-O0
.endif

.if ${MACHINE_ARCH} == "mipseb" || ${MACHINE_ARCH} == "mipsel"
#COPTS.c-common.cc+=-O3
.endif

LDADD+=	${LIBMPC} ${LIBMPFR} ${LIBGMP} -lintl -lz -lm
DPADD+=	${LIBMPC} ${LIBMPFR} ${LIBGMP} ${LIBINTL} ${LIBZ} ${LIBM}

.PATH: ${DIST}/gcc ${DIST}/gcc/c ${DIST}/gcc/c-family ${DIST}/gcc/config
.PATH: ${G_out_file:H}
