#	$NetBSD: Makefile,v 1.7 2023/08/08 06:27:32 mrg Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/sys/crypto/aes

TESTS_C=	t_aes

AFLAGS+=	-D_LOCORE

.PATH:	${NETBSDSRCDIR}/sys/crypto/aes
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys

SRCS.t_aes+=	t_aes.c

SRCS.t_aes+=	aes_bear.c
SRCS.t_aes+=	aes_ct.c
SRCS.t_aes+=	aes_ct_dec.c
SRCS.t_aes+=	aes_ct_enc.c
SRCS.t_aes+=	aes_selftest.c

.if !empty(MACHINE_ARCH:Mearmv7*) || !empty(MACHINE_ARCH:Maarch64*)

.PATH:	${NETBSDSRCDIR}/sys/crypto/aes/arch/arm
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/crypto/aes/arch/arm

.if !empty(MACHINE_ARCH:Maarch64*)	# XXX no AESE/AESD in 32-bit mode yet
SRCS.t_aes+=	aes_armv8.c
SRCS.t_aes+=	aes_armv8_64.S
.endif

CLANG_NO_NONPORTABLE_VECTOR_INITIALIZATION= ${${ACTIVE_CC} == "clang":? -Wno-nonportable-vector-initialization :}

SRCS.t_aes+=	aes_neon.c
COPTS.aes_neon.c+=${CLANG_NO_NONPORTABLE_VECTOR_INITIALIZATION}
SRCS.t_aes+=	aes_neon_impl.c
SRCS.t_aes+=	aes_neon_subr.c
COPTS.aes_neon_subr.c+=${CLANG_NO_NONPORTABLE_VECTOR_INITIALIZATION}
.if !empty(MACHINE_ARCH:Mearmv7*)
SRCS.t_aes+=	aes_neon_32.S
.endif

.if !empty(MACHINE_ARCH:Mearmv7*) && empty(MACHINE_ARCH:Mearmv7hf*)
COPTS.aes_neon.c+=	-mfloat-abi=softfp -mfpu=neon
COPTS.aes_neon_subr.c+=	-mfloat-abi=softfp -mfpu=neon
AOPTS.aes_neon_32.S+=	-D__SOFTFP__
.endif

.endif				# earmv7/aarch64

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"

.PATH:	${NETBSDSRCDIR}/sys/crypto/aes/arch/x86
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/crypto/aes/arch/x86

.if ${MACHINE_ARCH} == "x86_64"		# XXX no AES-NI in 32-bit mode yet
SRCS.t_aes+=	aes_ni.c
SRCS.t_aes+=	aes_ni_64.S
.endif

SRCS.t_aes+=	aes_sse2.c
SRCS.t_aes+=	aes_sse2_dec.c
SRCS.t_aes+=	aes_sse2_enc.c
SRCS.t_aes+=	aes_sse2_impl.c
SRCS.t_aes+=	aes_sse2_subr.c
COPTS.aes_sse2.c+=	-msse -msse2
COPTS.aes_sse2_dec.c+=	-msse -msse2
COPTS.aes_sse2_enc.c+=	-msse -msse2
COPTS.aes_sse2_subr.c+=	-msse -msse2

SRCS.t_aes+=	aes_ssse3.c
SRCS.t_aes+=	aes_ssse3_impl.c
SRCS.t_aes+=	aes_ssse3_subr.c
COPTS.aes_ssse3.c+=		-msse -msse2 -msse3 -mssse3
COPTS.aes_ssse3_subr.c+=	-msse -msse2 -msse3 -mssse3

SRCS.t_aes+=	aes_via.c

.endif				# x86

WARNS=		5

# Many of these are probably GCC bugs, see
#   https://gcc.gnu.org/bugzilla/show_bug.cgi?id=110878
COPTS.aes_armv8.c+=	${CC_WNO_STRINGOP_OVERREAD} ${CC_WNO_STRINGOP_OVERFLOW}
COPTS.aes_bear.c+=	${CC_WNO_STRINGOP_OVERFLOW} ${CC_WNO_ARRAY_BOUNDS}
COPTS.aes_neon_subr.c+=	${CC_WNO_ARRAY_BOUNDS}

COPTS.aes_ni.c+=	${CC_WNO_STRINGOP_OVERREAD} ${CC_WNO_STRINGOP_OVERFLOW}
COPTS.aes_sse2_subr.c+=	${CC_WNO_ARRAY_BOUNDS}
COPTS.aes_ssse3_subr.c+=${CC_WNO_ARRAY_BOUNDS}
COPTS.aes_via.c+=	${CC_WNO_ARRAY_BOUNDS}

.include <bsd.test.mk>
