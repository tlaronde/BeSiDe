#	$NetBSD: Makefile,v 1.11 2023/08/01 05:57:55 mrg Exp $

HOSTPROG_CXX=	1
LIBISPRIVATE=	yes

.include <bsd.init.mk>

LIB=		cpp

SRCS=		${G_libcpp_a_OBJS:.o=.cc}

CPPFLAGS+=	-I${.CURDIR} -I${.CURDIR}/arch/${GCC_MACHINE_ARCH} \
		-I${GCCARCH} -I${BACKENDOBJ} -I. \
		${G_INCLUDES:M-I*:N-I.*} \
		-DPREFIX=\"/usr\"

.include <bsd.lib.mk>

#COPTS+=	-Wno-stack-protector

# Force using C++ for this
HOST_CC:=	${HOST_CXX}
HOST_CFLAGS:=	${HOST_CXXFLAGS}
CC:=		${CXX}
CFLAGS:=	${CXXFLAGS}

${OBJS}: ${G_LIBCPP_DEPS}

.PATH: ${DIST}/libcpp
# ${DIST}/gcc

localedir.h:
	echo '#define LOCALEDIR "/usr/share/locale"' > ${.TARGET}
CLEANFILES+= localedir.h

init.d init.o: localedir.h
