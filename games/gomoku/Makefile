#	$NetBSD: Makefile,v 1.13 2023/10/07 12:15:52 rin Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/24/94

PROG=	gomoku
SRCS=	bdinit.c bdisp.c main.c makemove.c pickmove.c stoc.c
MAN=	gomoku.6
DPADD=	${LIBCURSES} ${LIBTERMINFO}
LDADD=	-lcurses -lterminfo
HIDEGAME=hidegame
CPPFLAGS+=	${DEBUG:D-DDEBUG}

#WARNS=		6	# would produce warnings about small integer types
LINTFLAGS+=	-w	# treat warnings as errors
LINTFLAGS+=	-T	# strict bool mode
LINTFLAGS+=	-e	# strict enum checks

.include <bsd.prog.mk>
