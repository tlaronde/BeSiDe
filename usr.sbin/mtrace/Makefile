#	$NetBSD: Makefile,v 1.14 2023/10/07 12:15:54 rin Exp $
#	from: Id: Makefile,v 1.5 1993/06/24 05:11:16 deering Exp

.include <bsd.own.mk>

USE_FORT?= yes	# network client *and* setuid

.PATH: ${NETBSDSRCDIR}/usr.sbin/mrouted
CPPFLAGS+= -I${NETBSDSRCDIR}/usr.sbin/mrouted

PROG=	mtrace
SRCS=	igmp.c inet.c kern.c mtrace.c
MAN=	mtrace.8

BINMODE=4555
BINOWN= root

CWARNFLAGS.gcc+=        ${CC_WNO_ADDRESS_OF_PACKED_MEMBER}

.include <bsd.prog.mk>
