# $NetBSD: Makefile,v 1.6 2023/10/12 17:18:39 riastradh Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

.PATH: ${HEIMDIST}/lib/kadm5

PROG= iprop-log
MAN=  iprop-log.8

HEIMSRCS= iprop-log.c iprop-commands.in

CPPFLAGS+= -I${DESTDIR}/usr/include/kadm5 -I${HEIMDIST}/lib/krb5

PROGDPLIBS += ${DPLIBKADM5SRV} ${DPLIBHDB} ${DPLIBSL}

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>

LDADD+= -ledit -lterminfo ${KRB5LDADD} ${HDBLDADD}
DPADD+= ${LIBEDIT} ${LIBTERMINFO} ${KRB5DPADD} ${HDBDPADD}
