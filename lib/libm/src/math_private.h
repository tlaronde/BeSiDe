/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

/*
 * from: @(#)fdlibm.h 5.1 93/09/24
 * $NetBSD: math_private.h,v 1.27 2023/08/03 20:45:49 andvar Exp $
 */

#ifndef _MATH_PRIVATE_H_
#define _MATH_PRIVATE_H_

#include <sys/types.h>

/* The original fdlibm code used statements like:
	n0 = ((*(int*)&one)>>29)^1;		* index of high word *
	ix0 = *(n0+(int*)&x);			* high word of x *
	ix1 = *((1-n0)+(int*)&x);		* low word of x *
   to dig two 32 bit words out of the 64 bit IEEE floating point
   value.  That is non-ANSI, and, moreover, the gcc instruction
   scheduler gets it wrong.  We instead use the following macros.
   Unlike the original code, we determine the endianness at compile
   time, not at run time; I don't see much benefit to selecting
   endianness at run time.  */

/* A union which permits us to convert between a double and two 32 bit
   ints.  */

/*
 * The ARM ports are little endian except for the FPA word order which is
 * big endian.
 */

#if (BYTE_ORDER == BIG_ENDIAN) || (defined(__arm__) && !defined(__VFP_FP__))

typedef union
{
  double value;
  struct
  {
    u_int32_t msw;
    u_int32_t lsw;
  } parts;
  struct {
    u_int64_t w;
  } xparts;
} ieee_double_shape_type;

#endif

#if (BYTE_ORDER == LITTLE_ENDIAN) && \
    !(defined(__arm__) && !defined(__VFP_FP__))

typedef union
{
  double value;
  struct
  {
    u_int32_t lsw;
    u_int32_t msw;
  } parts;
  struct {
    u_int64_t w;
  } xparts;
} ieee_double_shape_type;

#endif

/* Get two 32 bit ints from a double.  */

#define EXTRACT_WORDS(ix0,ix1,d)				\
do {								\
  ieee_double_shape_type ew_u;					\
  ew_u.value = (d);						\
  (ix0) = ew_u.parts.msw;					\
  (ix1) = ew_u.parts.lsw;					\
} while (0)

/* Get a 64-bit int from a double. */
#define EXTRACT_WORD64(ix,d)					\
do {								\
  ieee_double_shape_type ew_u;					\
  ew_u.value = (d);						\
  (ix) = ew_u.xparts.w;						\
} while (0)


/* Get the more significant 32 bit int from a double.  */

#define GET_HIGH_WORD(i,d)					\
do {								\
  ieee_double_shape_type gh_u;					\
  gh_u.value = (d);						\
  (i) = gh_u.parts.msw;						\
} while (0)

/* Get the less significant 32 bit int from a double.  */

#define GET_LOW_WORD(i,d)					\
do {								\
  ieee_double_shape_type gl_u;					\
  gl_u.value = (d);						\
  (i) = gl_u.parts.lsw;						\
} while (0)

/* Set a double from two 32 bit ints.  */

#define INSERT_WORDS(d,ix0,ix1)					\
do {								\
  ieee_double_shape_type iw_u;					\
  iw_u.parts.msw = (ix0);					\
  iw_u.parts.lsw = (ix1);					\
  (d) = iw_u.value;						\
} while (0)

/* Set a double from a 64-bit int. */
#define INSERT_WORD64(d,ix)					\
do {								\
  ieee_double_shape_type iw_u;					\
  iw_u.xparts.w = (ix);						\
  (d) = iw_u.value;						\
} while (0)


/* Set the more significant 32 bits of a double from an int.  */

#define SET_HIGH_WORD(d,v)					\
do {								\
  ieee_double_shape_type sh_u;					\
  sh_u.value = (d);						\
  sh_u.parts.msw = (v);						\
  (d) = sh_u.value;						\
} while (0)

/* Set the less significant 32 bits of a double from an int.  */

#define SET_LOW_WORD(d,v)					\
do {								\
  ieee_double_shape_type sl_u;					\
  sl_u.value = (d);						\
  sl_u.parts.lsw = (v);						\
  (d) = sl_u.value;						\
} while (0)

/* A union which permits us to convert between a float and a 32 bit
   int.  */

typedef union
{
  float value;
  u_int32_t word;
} ieee_float_shape_type;

/* Get a 32 bit int from a float.  */

#define GET_FLOAT_WORD(i,d)					\
do {								\
  ieee_float_shape_type gf_u;					\
  gf_u.value = (d);						\
  (i) = gf_u.word;						\
} while (0)

/* Set a float from a 32 bit int.  */

#define SET_FLOAT_WORD(d,i)					\
do {								\
  ieee_float_shape_type sf_u;					\
  sf_u.word = (i);						\
  (d) = sf_u.value;						\
} while (0)

/*
 * Attempt to get strict C99 semantics for assignment with non-C99 compilers.
 */
#if FLT_EVAL_METHOD == 0 || __GNUC__ == 0
#define	STRICT_ASSIGN(type, lval, rval)	((lval) = (rval))
#else
#define	STRICT_ASSIGN(type, lval, rval) do {	\
	volatile type __lval;			\
						\
	if (sizeof(type) >= sizeof(long double))	\
		(lval) = (rval);		\
	else {					\
		__lval = (rval);		\
		(lval) = __lval;		\
	}					\
} while (0)
#endif

/* Support switching the mode to FP_PE if necessary. */
#if defined(__i386__) && !defined(NO_FPSETPREC)
#define	ENTERI() ENTERIT(long double)
#define	ENTERIT(returntype)			\
	returntype __retval;			\
	fp_prec_t __oprec;			\
						\
	if ((__oprec = fpgetprec()) != FP_PE)	\
		fpsetprec(FP_PE)
#define	RETURNI(x) do {				\
	__retval = (x);				\
	if (__oprec != FP_PE)			\
		fpsetprec(__oprec);		\
	RETURNF(__retval);			\
} while (0)
#define	ENTERV()				\
	fp_prec_t __oprec;			\
						\
	if ((__oprec = fpgetprec()) != FP_PE)	\
		fpsetprec(FP_PE)
#define	RETURNV() do {				\
	if (__oprec != FP_PE)			\
		fpsetprec(__oprec);		\
	return;			\
} while (0)
#else
#define	ENTERI()
#define	ENTERIT(x)
#define	RETURNI(x)	RETURNF(x)
#define	ENTERV()
#define	RETURNV()	return
#endif

#ifdef	_COMPLEX_H

/*
 * Quoting from ISO/IEC 9899:TC2:
 *
 * 6.2.5.13 Types
 * Each complex type has the same representation and alignment requirements as
 * an array type containing exactly two elements of the corresponding real type;
 * the first element is equal to the real part, and the second element to the
 * imaginary part, of the complex number.
 */
typedef union {
	float complex z;
	float parts[2];
} float_complex;

typedef union {
	double complex z;
	double parts[2];
} double_complex;

typedef union {
	long double complex z;
	long double parts[2];
} long_double_complex;

#define	REAL_PART(z)	((z).parts[0])
#define	IMAG_PART(z)	((z).parts[1])

/*
 * Inline functions that can be used to construct complex values.
 *
 * The C99 standard intends x+I*y to be used for this, but x+I*y is
 * currently unusable in general since gcc introduces many overflow,
 * underflow, sign and efficiency bugs by rewriting I*y as
 * (0.0+I)*(y+0.0*I) and laboriously computing the full complex product.
 * In particular, I*Inf is corrupted to NaN+I*Inf, and I*-0 is corrupted
 * to -0.0+I*0.0.
 *
 * The C11 standard introduced the macros CMPLX(), CMPLXF() and CMPLXL()
 * to construct complex values.  Compilers that conform to the C99
 * standard require the following functions to avoid the above issues.
 */

#ifndef CMPLXF
static __inline float complex
CMPLXF(float x, float y)
{
	float_complex z;

	REAL_PART(z) = x;
	IMAG_PART(z) = y;
	return (z.z);
}
#endif

#ifndef CMPLX
static __inline double complex
CMPLX(double x, double y)
{
	double_complex z;

	REAL_PART(z) = x;
	IMAG_PART(z) = y;
	return (z.z);
}
#endif

#ifndef CMPLXL
static __inline long double complex
CMPLXL(long double x, long double y)
{
	long_double_complex z;

	REAL_PART(z) = x;
	IMAG_PART(z) = y;
	return (z.z);
}
#endif

#endif	/* _COMPLEX_H */

/* ieee style elementary functions */
extern double __ieee754_sqrt __P((double));
extern double __ieee754_acos __P((double));
extern double __ieee754_acosh __P((double));
extern double __ieee754_log __P((double));
extern double __ieee754_atanh __P((double));
extern double __ieee754_asin __P((double));
extern double __ieee754_atan2 __P((double,double));
extern double __ieee754_exp __P((double));
extern double __ieee754_cosh __P((double));
extern double __ieee754_fmod __P((double,double));
extern double __ieee754_pow __P((double,double));
extern double __ieee754_lgamma_r __P((double,int *));
extern double __ieee754_gamma_r __P((double,int *));
extern double __ieee754_lgamma __P((double));
extern double __ieee754_gamma __P((double));
extern double __ieee754_log10 __P((double));
extern double __ieee754_log2 __P((double));
extern double __ieee754_sinh __P((double));
extern double __ieee754_hypot __P((double,double));
extern double __ieee754_j0 __P((double));
extern double __ieee754_j1 __P((double));
extern double __ieee754_y0 __P((double));
extern double __ieee754_y1 __P((double));
extern double __ieee754_jn __P((int,double));
extern double __ieee754_yn __P((int,double));
extern double __ieee754_remainder __P((double,double));
extern int32_t __ieee754_rem_pio2 __P((double,double*));
extern double __ieee754_scalb __P((double,double));

/* fdlibm kernel function */
extern double __kernel_standard __P((double,double,int));
extern double __kernel_sin __P((double,double,int));
extern double __kernel_cos __P((double,double));
extern double __kernel_tan __P((double,double,int));
extern int    __kernel_rem_pio2 __P((double*,double*,int,int,int));


/* ieee style elementary float functions */
extern float __ieee754_sqrtf __P((float));
extern float __ieee754_acosf __P((float));
extern float __ieee754_acoshf __P((float));
extern float __ieee754_logf __P((float));
extern float __ieee754_atanhf __P((float));
extern float __ieee754_asinf __P((float));
extern float __ieee754_atan2f __P((float,float));
extern float __ieee754_expf __P((float));
extern float __ieee754_coshf __P((float));
extern float __ieee754_fmodf __P((float,float));
extern float __ieee754_powf __P((float,float));
extern float __ieee754_lgammaf_r __P((float,int *));
extern float __ieee754_gammaf_r __P((float,int *));
extern float __ieee754_lgammaf __P((float));
extern float __ieee754_gammaf __P((float));
extern float __ieee754_log10f __P((float));
extern float __ieee754_log2f __P((float));
extern float __ieee754_sinhf __P((float));
extern float __ieee754_hypotf __P((float,float));
extern float __ieee754_j0f __P((float));
extern float __ieee754_j1f __P((float));
extern float __ieee754_y0f __P((float));
extern float __ieee754_y1f __P((float));
extern float __ieee754_jnf __P((int,float));
extern float __ieee754_ynf __P((int,float));
extern float __ieee754_remainderf __P((float,float));
extern int32_t __ieee754_rem_pio2f __P((float,float*));
extern float __ieee754_scalbf __P((float,float));

/* float versions of fdlibm kernel functions */
extern float __kernel_sinf __P((float,float,int));
extern float __kernel_cosf __P((float,float));
extern float __kernel_tanf __P((float,float,int));
extern int   __kernel_rem_pio2f __P((float*,float*,int,int,int,const int32_t*));

/* ieee style elementary long double functions */
extern long double __ieee754_fmodl(long double, long double);
extern long double __ieee754_sqrtl(long double);

/*
 * TRUNC() is a macro that sets the trailing 27 bits in the mantissa of an
 * IEEE double variable to zero.  It must be expression-like for syntactic
 * reasons, and we implement this expression using an inline function
 * instead of a pure macro to avoid depending on the gcc feature of
 * statement-expressions.
 */
#define	TRUNC(d)	(_b_trunc(&(d)))

static __inline void
_b_trunc(volatile double *_dp)
{
	uint32_t _lw;

	GET_LOW_WORD(_lw, *_dp);
	SET_LOW_WORD(*_dp, _lw & 0xf8000000);
}

struct Double {
	double	a;
	double	b;
};

/*
 * Functions internal to the math package, yet not static.
 */
double	__exp__D(double, double);
struct Double __log__D(double);

/*
 * The rnint() family rounds to the nearest integer for a restricted range
 * range of args (up to about 2**MANT_DIG).  We assume that the current
 * rounding mode is FE_TONEAREST so that this can be done efficiently.
 * Extra precision causes more problems in practice, and we only centralize
 * this here to reduce those problems, and have not solved the efficiency
 * problems.  The exp2() family uses a more delicate version of this that
 * requires extracting bits from the intermediate value, so it is not
 * centralized here and should copy any solution of the efficiency problems.
 */

static inline double
rnint(double x)
{
	/*
	 * This casts to double to kill any extra precision.  This depends
	 * on the cast being applied to a double_t to avoid compiler bugs
	 * (this is a cleaner version of STRICT_ASSIGN()).  This is
	 * inefficient if there actually is extra precision, but is hard
	 * to improve on.  We use double_t in the API to minimise conversions
	 * for just calling here.  Note that we cannot easily change the
	 * magic number to the one that works directly with double_t, since
	 * the rounding precision is variable at runtime on x86 so the
	 * magic number would need to be variable.  Assuming that the
	 * rounding precision is always the default is too fragile.  This
	 * and many other complications will move when the default is
	 * changed to FP_PE.
	 */
	return ((double)(x + 0x1.8p52) - 0x1.8p52);
}

static inline float
rnintf(float x)
{
	/*
	 * As for rnint(), except we could just call that to handle the
	 * extra precision case, usually without losing efficiency.
	 */
	return ((float)(x + 0x1.8p23F) - 0x1.8p23F);
}

#ifdef LDBL_MANT_DIG
/*
 * The complications for extra precision are smaller for rnintl() since it
 * can safely assume that the rounding precision has been increased from
 * its default to FP_PE on x86.  We don't exploit that here to get small
 * optimizations from limiting the range to double.  We just need it for
 * the magic number to work with long doubles.  ld128 callers should use
 * rnint() instead of this if possible.  ld80 callers should prefer
 * rnintl() since for amd64 this avoids swapping the register set, while
 * for i386 it makes no difference (assuming FP_PE), and for other arches
 * it makes little difference.
 */
static inline long double
rnintl(long double x)
{
	return (x + ___CONCAT(0x1.8p,LDBL_MANT_DIG) / 2 -
	    ___CONCAT(0x1.8p,LDBL_MANT_DIG) / 2);
}
#endif /* LDBL_MANT_DIG */

/*
 * irint() and i64rint() give the same result as casting to their integer
 * return type provided their arg is a floating point integer.  They can
 * sometimes be more efficient because no rounding is required.
 */
#if (defined(amd64) || defined(__i386__)) && defined(__GNUCLIKE_ASM)
#define	irint(x)						\
    (sizeof(x) == sizeof(float) &&				\
    sizeof(__float_t) == sizeof(long double) ? irintf(x) :	\
    sizeof(x) == sizeof(double) &&				\
    sizeof(__double_t) == sizeof(long double) ? irintd(x) :	\
    sizeof(x) == sizeof(long double) ? irintl(x) : (int)(x))
#else
#define	irint(x)	((int)(x))
#endif

#define	i64rint(x)	((int64_t)(x))	/* only needed for ld128 so not opt. */

#if defined(__i386__) && defined(__GNUCLIKE_ASM)
static __inline int
irintf(float x)
{
	int n;

	__asm("fistl %0" : "=m" (n) : "t" (x));
	return (n);
}

static __inline int
irintd(double x)
{
	int n;

	__asm("fistl %0" : "=m" (n) : "t" (x));
	return (n);
}
#endif

#if (defined(__amd64__) || defined(__i386__)) && defined(__GNUCLIKE_ASM)
static __inline int
irintl(long double x)
{
	int n;

	__asm("fistl %0" : "=m" (n) : "t" (x));
	return (n);
}
#endif

#endif /* _MATH_PRIVATE_H_ */
