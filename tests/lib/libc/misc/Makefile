# $NetBSD: Makefile,v 1.9 2023/08/10 20:44:37 mrg Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libc/misc

TESTS_C+=	t_ubsan
TESTS_CXX+=	t_ubsanxx

.PATH:		${NETBSDSRCDIR}/common/lib/libc/misc
SRCS.t_ubsan=	t_ubsan.c
SRCS.t_ubsanxx=	t_ubsanxx.cpp

.if ${MKSANITIZER:Uno} != "yes" && ${MKLIBCSANITIZER:Uno} != "yes"
# These tests are designed to be used against micro-UBSan only.
# micro-UBSan is used in these tests as a standalone library only.
CPPFLAGS+=		-DENABLE_TESTS
SRCS.t_ubsan+=		ubsan.c
SRCS.t_ubsanxx+=	ubsan.c
UBSAN_FLAGS=		-fsanitize=undefined
UBSAN_FLAGS+=		${${ACTIVE_CC} == "clang" :? -fsanitize=integer :}
UBSAN_FLAGS+=		${${ACTIVE_CC} == "clang" :? -fsanitize=nullability :}
CFLAGS+=		${UBSAN_FLAGS}
CXXFLAGS+=		${UBSAN_FLAGS}
CWARNFLAGS+=		-Wno-return-type -Wno-strict-aliasing
CWARNFLAGS.clang+=	-Wno-incompatible-pointer-types-discards-qualifiers
CWARNFLAGS.clang+=	-Wno-nullability-completeness 
.endif
COPTS.t_ubsan.c += -Wno-stack-protector
COPTS.t_ubsanxx.cpp += -Wno-stack-protector
COPTS.ubsan.c+=    ${${ACTIVE_CC} == "clang" && ${MACHINE_ARCH} == "powerpc":? -O0 :}

COPTS.t_ubsan.c+=	${${ACTIVE_CC} == "gcc" && ${HAVE_GCC:U0} >= 7:? -Wno-int-in-bool-context :}
COPTS.t_ubsanxx.cpp+=	${${ACTIVE_CC} == "gcc" && ${HAVE_GCC:U0} >= 7:? -Wno-int-in-bool-context :}
COPTS.t_ubsan.c+=	${${ACTIVE_CC} == "gcc" && ${HAVE_GCC:U0} >= 12:? -Wno-uninitialized :}
COPTS.t_ubsanxx.cpp+=	${${ACTIVE_CC} == "gcc" && ${HAVE_GCC:U0} >= 12:? -Wno-uninitialized :}

COPTS.ubsan.c+=    	${${ACTIVE_CC} == "gcc" && ${HAVE_GCC:U0} >= 9:? -Wno-builtin-declaration-mismatch :}

.include <bsd.test.mk>
