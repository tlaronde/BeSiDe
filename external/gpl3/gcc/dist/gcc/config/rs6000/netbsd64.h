/* Definitions of target machine for GNU compiler,
   for 64 bit PowerPC NetBSD.
   Copyright (C) 2006-2023 Free Software Foundation, Inc.
   Contributed by Matthew Green (mrg@eterna.com.au).

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published
   by the Free Software Foundation; either version 2, or (at your
   option) any later version.

   GCC is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with GCC; see the file COPYING.  If not, write to the
   Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301, USA.  */

/* Undef gnu-user.h macros we don't want.  */
#undef CPLUSPLUS_CPP_SPEC
#undef LINK_GCC_C_SEQUENCE_SPEC

#ifndef RS6000_BI_ARCH

#undef	DEFAULT_ABI
#define	DEFAULT_ABI ABI_AIX

#undef	TARGET_64BIT
#define	TARGET_64BIT 1

#define	DEFAULT_ARCH64_P 1
#define	RS6000_BI_ARCH_P 0

#else

#define	DEFAULT_ARCH64_P (TARGET_DEFAULT & MASK_64BIT)
#define	RS6000_BI_ARCH_P 1

#endif

#ifdef IN_LIBGCC2
#undef TARGET_64BIT
#ifdef __powerpc64__
#define TARGET_64BIT 1
#else
#define TARGET_64BIT 0
#endif
#endif

#undef	TARGET_AIX
#define	TARGET_AIX TARGET_64BIT

#ifdef HAVE_LD_NO_DOT_SYMS
/* New ABI uses a local sym for the function entry point.  */
extern int dot_symbols;
#undef DOT_SYMBOLS
#define DOT_SYMBOLS dot_symbols
#endif

#define TARGET_PROFILE_KERNEL profile_kernel

#define TARGET_USES_LINUX64_OPT 1
#ifdef HAVE_LD_LARGE_TOC
#undef TARGET_CMODEL
#define TARGET_CMODEL rs6000_current_cmodel
#define SET_CMODEL(opt) rs6000_current_cmodel = opt
#else
#define SET_CMODEL(opt) do {} while (0)
#endif

#undef  PROCESSOR_DEFAULT
#define PROCESSOR_DEFAULT PROCESSOR_POWER4
#undef  PROCESSOR_DEFAULT64
#define PROCESSOR_DEFAULT64 PROCESSOR_POWER4

/* We don't need to generate entries in .fixup, except when
   -mrelocatable or -mrelocatable-lib is given.  */
#undef RELOCATABLE_NEEDS_FIXUP
#define RELOCATABLE_NEEDS_FIXUP \
  (rs6000_isa_flags & rs6000_isa_flags_explicit & OPTION_MASK_RELOCATABLE)

#undef	RS6000_ABI_NAME
#define	RS6000_ABI_NAME "netbsd"

#define INVALID_64BIT "-m%s not supported in this configuration"
#define INVALID_32BIT INVALID_64BIT

#define ELFv2_ABI_CHECK (rs6000_elf_abi == 2)                   

#undef	SUBSUBTARGET_OVERRIDE_OPTIONS
#define	SUBSUBTARGET_OVERRIDE_OPTIONS				\
  do rs6000_linux64_override_options (); while (0)

#undef	ASM_DEFAULT_SPEC
#undef	ASM_SPEC
#undef	LINK_OS_NETBSD_SPEC
#undef	LINK_SECURE_PLT_SPEC

#ifndef	RS6000_BI_ARCH
#define	ASM_DEFAULT_SPEC "-mppc64"
#define	ASM_SPEC	 "%(asm_spec64) %(asm_spec_common)"
#define	LINK_OS_NETBSD_SPEC "%(link_os_netbsd_spec64)"
#define	LINK_SECURE_PLT_SPEC ""
#else
#if DEFAULT_ARCH64_P
#define	ASM_DEFAULT_SPEC "-mppc%{!m32:64}"
#define	ASM_SPEC	 "%{m32:%(asm_spec32)}%{!m32:%(asm_spec64)} %(asm_spec_common)"
#define	LINK_OS_NETBSD_SPEC "%{m32:%(link_os_netbsd_spec32)}%{!m32:%(link_os_netbsd_spec64)}"
#define	LINK_SECURE_PLT_SPEC "%{m32: " LINK_SECURE_PLT_DEFAULT_SPEC "}"
#else
#define	ASM_DEFAULT_SPEC "-mppc%{m64:64}"
#define	ASM_SPEC	 "%{!m64:%(asm_spec32)}%{m64:%(asm_spec64)} %(asm_spec_common)"
#define	LINK_OS_NETBSD_SPEC "%{!m64:%(link_os_netbsd_spec32)}%{m64:%(link_os_netbsd_spec64)}"
#define	LINK_SECURE_PLT_SPEC "%{!m64: " LINK_SECURE_PLT_DEFAULT_SPEC "}"
#endif
#endif

#define ASM_SPEC32 "-a32 \
%{mrelocatable} %{mrelocatable-lib} %{" FPIE_OR_FPIC_SPEC ":-K PIC} \
%{memb|msdata=eabi: -memb}"

#define ASM_SPEC64 "-a64"

#define ASM_SPEC_COMMON "%(asm_cpu) \
%{,assembler|,assembler-with-cpp: %{mregnames} %{mno-regnames}}" \
  ENDIAN_SELECT(" -mbig", " -mlittle", DEFAULT_ASM_ENDIAN)

#undef	SUBSUBTARGET_EXTRA_SPECS
#define SUBSUBTARGET_EXTRA_SPECS \
  { "asm_spec_common",		ASM_SPEC_COMMON },			\
  { "asm_spec32",		ASM_SPEC32 },				\
  { "asm_spec64",		ASM_SPEC64 },				\
  { "link_os_netbsd_spec32",	LINK_OS_NETBSD_SPEC32 },		\
  { "link_os_netbsd_spec64",	LINK_OS_NETBSD_SPEC64 },

#undef	MULTILIB_DEFAULTS
#if DEFAULT_ARCH64_P
#define MULTILIB_DEFAULTS { "m64" }
#else
#define MULTILIB_DEFAULTS { "m32" }
#endif

#ifndef RS6000_BI_ARCH

/* 64-bit PowerPC NetBSD is always big-endian.  */
#undef	TARGET_LITTLE_ENDIAN
#define TARGET_LITTLE_ENDIAN	0

/* 64-bit PowerPC NetBSD always has a TOC.  */
#undef  TARGET_TOC
#define	TARGET_TOC		1

/* Some things from sysv4.h we don't do when 64 bit.  */
#undef	TARGET_RELOCATABLE
#define	TARGET_RELOCATABLE	0
#undef	TARGET_EABI
#define	TARGET_EABI		0
#undef	TARGET_PROTOTYPE
#define	TARGET_PROTOTYPE	0
#undef RELOCATABLE_NEEDS_FIXUP
#define RELOCATABLE_NEEDS_FIXUP 0

#endif

/* We use NetBSD libc _mcount for profiling.  */
#define NO_PROFILE_COUNTERS 1
#define PROFILE_HOOK(LABEL) \
  do { if (TARGET_64BIT) output_profile_hook (LABEL); } while (0)

/* PowerPC64 NetBSD word-aligns FP doubles when -malign-power is given.  */
#undef  ADJUST_FIELD_ALIGN
#define ADJUST_FIELD_ALIGN(FIELD, TYPE, COMPUTED) \
  ((TARGET_64BIT							\
    && TARGET_ALIGN_NATURAL == 0					\
    && TYPE_MODE (strip_array_types (TYPE)) == DFmode)   		\
   ? MIN ((COMPUTED), 32)						\
   : (COMPUTED))

/* PowerPC64 NetBSD increases natural record alignment to doubleword if
   the first field is an FP double, only if in power alignment mode.  */
#undef  ROUND_TYPE_ALIGN
#define ROUND_TYPE_ALIGN(STRUCT, COMPUTED, SPECIFIED)			\
  ((TARGET_64BIT							\
    && (TREE_CODE (STRUCT) == RECORD_TYPE				\
	|| TREE_CODE (STRUCT) == UNION_TYPE				\
	|| TREE_CODE (STRUCT) == QUAL_UNION_TYPE)			\
    && TARGET_ALIGN_NATURAL == 0)					\
   ? rs6000_special_round_type_align (STRUCT, COMPUTED, SPECIFIED)	\
   : MAX ((COMPUTED), (SPECIFIED)))

/* Use the default for compiling target libs.  */
#ifdef IN_TARGET_LIBS
#undef TARGET_ALIGN_NATURAL
#define TARGET_ALIGN_NATURAL 1
#endif

/* Indicate that jump tables go in the text section.  */
#undef  JUMP_TABLES_IN_TEXT_SECTION
#define JUMP_TABLES_IN_TEXT_SECTION TARGET_64BIT

/* The linux ppc64 ABI isn't explicit on whether aggregates smaller
   than a doubleword should be padded upward or downward.  You could
   reasonably assume that they follow the normal rules for structure
   layout treating the parameter area as any other block of memory,
   then map the reg param area to registers.  i.e. pad upward.
   Setting both of the following defines results in this behavior.
   Setting just the first one will result in aggregates that fit in a
   doubleword being padded downward, and others being padded upward.
   Not a bad idea as this results in struct { int x; } being passed
   the same way as an int.  */
#define AGGREGATE_PADDING_FIXED TARGET_64BIT
#define AGGREGATES_PAD_UPWARD_ALWAYS 0

/* Specify padding for the last element of a block move between
   registers and memory.  FIRST is nonzero if this is the only
   element.  */
#define BLOCK_REG_PADDING(MODE, TYPE, FIRST) \
  (!(FIRST) ? PAD_UPWARD : targetm.calls.function_arg_padding (MODE, TYPE))

/* NetBSD doesn't support saving and restoring 64-bit regs in a 32-bit
   process.  */
#define OS_MISSING_POWERPC64 (!TARGET_64BIT)

#undef  TARGET_OS_CPP_BUILTINS
#define TARGET_OS_CPP_BUILTINS()			\
  do							\
    {							\
      NETBSD_OS_CPP_BUILTINS_ELF();			\
      builtin_define ("__PPC__");			\
      builtin_define ("__ppc__");			\
      builtin_define ("__powerpc__");			\
      if (TARGET_ISEL)					\
	builtin_define ("__PPC_ISEL__");		\
      if (TARGET_64BIT)					\
	{						\
	  builtin_define ("__arch64__");		\
	  builtin_define ("__PPC64__");			\
	  builtin_define ("__powerpc64__");		\
	  builtin_define_with_int_value ("__PIC__", 2);	\
	  builtin_assert ("cpu=powerpc64");		\
	  builtin_assert ("machine=powerpc64");		\
	}						\
      else						\
	{						\
	  builtin_define_std ("PPC");			\
	  builtin_define_std ("powerpc");		\
	  builtin_assert ("cpu=powerpc");		\
	  builtin_assert ("machine=powerpc");		\
	  TARGET_OS_SYSV_CPP_BUILTINS ();		\
	}						\
    }							\
  while (0)

/* Override the default from rs6000.h to avoid conflicts with macros
   defined in NetBSD header files.  */

#undef  RS6000_CPU_CPP_ENDIAN_BUILTINS
#define RS6000_CPU_CPP_ENDIAN_BUILTINS()	\
  do						\
    {						\
      builtin_define ("__BIG_ENDIAN__");	\
      builtin_assert ("machine=bigendian");	\
    }						\
  while (0)

#undef CC1_OS_NETBSD_SPEC
#define CC1_OS_NETBSD_SPEC \
  NETBSD_CC1_AND_CC1PLUS_SPEC \
  "%{!m32: %{!mrelocatable: %{!fno-pie: %{!fno-pic: \
     %{!fpie: %{!fpic: \
       %{!fPIE: %{!fPIC:-fPIC}}}}}}}}"
/* %{!m32: %{!mcmodel*: -mcmodel=medium}}" */

#undef CC1PLUS_SPEC
#define CC1PLUS_SPEC CC1_OS_NETBSD_SPEC

#undef  CPP_OS_DEFAULT_SPEC
#define CPP_OS_DEFAULT_SPEC "%(cpp_os_netbsd)"

#undef  LINK_SHLIB_SPEC
#define LINK_SHLIB_SPEC "%{shared:-shared} %{!shared: %{static:-static}}"

#undef  LIB_DEFAULT_SPEC
#define LIB_DEFAULT_SPEC "%(lib_netbsd)"

#undef  STARTFILE_DEFAULT_SPEC
#define STARTFILE_DEFAULT_SPEC "%(startfile_netbsd)"

#undef	ENDFILE_DEFAULT_SPEC
#define ENDFILE_DEFAULT_SPEC "%(endfile_netbsd)"

#undef	LINK_START_DEFAULT_SPEC
#define LINK_START_DEFAULT_SPEC "%(link_start_netbsd)"

#undef	LINK_OS_DEFAULT_SPEC
#define LINK_OS_DEFAULT_SPEC "%(link_os_netbsd)"

#define LINK_OS_NETBSD_SPEC32 "-m elf32ppc %{!shared: %{!static: \
  %{rdynamic:-export-dynamic} \
  %{!dynamic-linker:-dynamic-linker /usr/libexec/ld.elf_so}}}"

#define LINK_OS_NETBSD_SPEC64 "-m elf64ppc %{!shared: %{!static: \
  %{rdynamic:-export-dynamic} \
  %{!dynamic-linker:-dynamic-linker /usr/libexec/ld.elf_so}}}"

/* Use standard DWARF numbering for DWARF debugging information.  */
#define RS6000_USE_DWARF_NUMBERING

#undef  TOC_SECTION_ASM_OP
#define TOC_SECTION_ASM_OP \
  (TARGET_64BIT						\
   ? "\t.section\t\".toc\",\"aw\""			\
   : "\t.section\t\".got\",\"aw\"")

#undef  MINIMAL_TOC_SECTION_ASM_OP
#define MINIMAL_TOC_SECTION_ASM_OP \
  (TARGET_64BIT						\
   ? "\t.section\t\".toc1\",\"aw\""			\
   : (flag_pic						\
      ? "\t.section\t\".got2\",\"aw\""			\
      : "\t.section\t\".got1\",\"aw\""))

/* This is how to declare the size of a function.  */
#undef  ASM_DECLARE_FUNCTION_SIZE
#define ASM_DECLARE_FUNCTION_SIZE(FILE, FNAME, DECL)                    \
  do                                                                    \
    {                                                                   \
      if (!flag_inhibit_size_directive)                                 \
        {                                                               \
          fputs ("\t.size\t", (FILE));                                  \
          if (TARGET_64BIT && DOT_SYMBOLS)                              \
            putc ('.', (FILE));                                         \
          assemble_name ((FILE), (FNAME));                              \
          fputs (",.-", (FILE));                                        \
          rs6000_output_function_entry (FILE, FNAME);                   \
          putc ('\n', (FILE));                                          \
        }                                                               \
    }                                                                   \
  while (0)

#undef  ASM_OUTPUT_SPECIAL_POOL_ENTRY_P
#define ASM_OUTPUT_SPECIAL_POOL_ENTRY_P(X, MODE)                        \
  (TARGET_TOC                                                           \
   && (SYMBOL_REF_P (X)							\
       || (GET_CODE (X) == CONST && GET_CODE (XEXP (X, 0)) == PLUS      \
           && SYMBOL_REF_P (XEXP (XEXP (X, 0), 0)))			\
       || GET_CODE (X) == LABEL_REF                                     \
       || (CONST_INT_P (X)						\
           && GET_MODE_BITSIZE (MODE) <= GET_MODE_BITSIZE (Pmode))      \
       || (CONST_DOUBLE_P (X)						\
           && ((TARGET_64BIT                                            \
                && (TARGET_MINIMAL_TOC                                  \
                    || (SCALAR_FLOAT_MODE_P (GET_MODE (X))              \
                        && ! TARGET_NO_FP_IN_TOC)))                     \
               || (!TARGET_64BIT                                        \
                   && !TARGET_NO_FP_IN_TOC                              \
                   && SCALAR_FLOAT_MODE_P (GET_MODE (X))                \
                   && BITS_PER_WORD == HOST_BITS_PER_INT)))))

/* Make GCC agree with <machine/ansi.h>.  */

#undef	SIZE_TYPE
#define	SIZE_TYPE (LONG_TYPE_SIZE == 64 ? "long unsigned int" : "unsigned int")

#undef	PTRDIFF_TYPE
#define	PTRDIFF_TYPE (LONG_TYPE_SIZE == 64 ? "long int" : "int")

#undef INTPTR_TYPE
#define INTPTR_TYPE PTRDIFF_TYPE

#undef UINTPTR_TYPE
#define UINTPTR_TYPE SIZE_TYPE

#undef	WCHAR_TYPE
#define	WCHAR_TYPE "int"

#undef INT8_TYPE
#define INT8_TYPE "signed char"

#undef INT16_TYPE
#define INT16_TYPE "short int"

#undef INT32_TYPE
#define INT32_TYPE "int"

#undef INT64_TYPE
#define	INT64_TYPE (LONG_TYPE_SIZE == 64 ? "long int" : "long long int")

#undef UINT8_TYPE
#define UINT8_TYPE "unsigned char"

#undef UINT16_TYPE
#define UINT16_TYPE "short unsigned int"

#undef UINT32_TYPE
#define UINT32_TYPE "unsigned int"

#undef UINT64_TYPE
#define	UINT64_TYPE (LONG_TYPE_SIZE == 64 ? "long unsigned int" : "long long unsigned int")

#undef INT_FAST8_TYPE
#define INT_FAST8_TYPE "int"

#undef INT_FAST16_TYPE
#define INT_FAST16_TYPE "int"

#undef INT_FAST32_TYPE
#define INT_FAST32_TYPE "int"

#undef INT_FAST64_TYPE
#define INT_FAST64_TYPE INT64_TYPE

#undef UINT_FAST8_TYPE
#define UINT_FAST8_TYPE "unsigned int"

#undef UINT_FAST16_TYPE
#define UINT_FAST16_TYPE "unsigned int"

#undef UINT_FAST32_TYPE
#define UINT_FAST32_TYPE "unsigned int"

#undef UINT_FAST8_TYPE
#define UINT_FAST8_TYPE "unsigned int"

#undef UINT_FAST16_TYPE
#define UINT_FAST16_TYPE "unsigned int"

#undef UINT_FAST32_TYPE
#define UINT_FAST32_TYPE "unsigned int"

#undef UINT_FAST64_TYPE
#define UINT_FAST64_TYPE UINT64_TYPE

#undef INT_LEAST8_TYPE
#define INT_LEAST8_TYPE INT8_TYPE

#undef INT_LEAST16_TYPE
#define INT_LEAST16_TYPE INT16_TYPE

#undef INT_LEAST32_TYPE
#define INT_LEAST32_TYPE "int"

#undef INT_LEAST64_TYPE
#define INT_LEAST64_TYPE INT64_TYPE

#undef UINT_LEAST8_TYPE
#define UINT_LEAST8_TYPE UINT8_TYPE

#undef UINT_LEAST16_TYPE
#define UINT_LEAST16_TYPE UINT16_TYPE

#undef UINT_LEAST32_TYPE
#define UINT_LEAST32_TYPE "unsigned int"

#undef UINT_LEAST64_TYPE
#define UINT_LEAST64_TYPE UINT64_TYPE

#undef INTMAX_TYPE
#define INTMAX_TYPE INT64_TYPE

#undef UINTMAX_TYPE
#define UINTMAX_TYPE UINT64_TYPE

#if 0
/* Override rs6000.h definition.  */
#undef  ASM_APP_ON
#define ASM_APP_ON "#APP\n"

/* Override rs6000.h definition.  */
#undef  ASM_APP_OFF
#define ASM_APP_OFF "#NO_APP\n"
#endif

#undef  RS6000_MCOUNT
#define RS6000_MCOUNT "_mcount"

#ifdef __powerpc64__
/* _init and _fini functions are built from bits spread across many
   object files, each potentially with a different TOC pointer.  For
   that reason, place a nop after the call so that the linker can
   restore the TOC pointer if a TOC adjusting call stub is needed.  */
#if DOT_SYMBOLS
#define CRT_CALL_STATIC_FUNCTION(SECTION_OP, FUNC)	\
  asm (SECTION_OP "\n"					\
"	bl ." #FUNC "\n"				\
"	nop\n"						\
"	.previous");
#else
#define CRT_CALL_STATIC_FUNCTION(SECTION_OP, FUNC)	\
  asm (SECTION_OP "\n"					\
"	bl " #FUNC "\n"					\
"	nop\n"						\
"	.previous");
#endif
#endif

/* FP save and restore routines.  */
#undef  SAVE_FP_PREFIX
#define SAVE_FP_PREFIX (TARGET_64BIT ? "._savef" : "_savefpr_")
#undef  RESTORE_FP_PREFIX
#define RESTORE_FP_PREFIX (TARGET_64BIT ? "._restf" : "_restfpr_")

/* For backward compatibility, we must continue to use the AIX
   structure return convention.  */
#undef DRAFT_V4_STRUCT_RET
#define DRAFT_V4_STRUCT_RET (!TARGET_64BIT)

/* Use --as-needed -lgcc_s for eh support.  */
#ifdef HAVE_LD_AS_NEEDED
#define USE_LD_AS_NEEDED 1
#endif

/*
 * NetBSD ppc64 used to have 128-bit long double support.
 * But it does not work anymore:
 * (insn 23 22 24 5 (set (reg:CCFP 179)
 *	 (compare:CCFP (reg/v:TF 171 [ a ])
 * 		     (reg:TF 177)))
 *  "/usr/src/sys/external/bsd/compiler_rt/dist/lib/builtins/fixxfti.c":43 -1
 */
#undef	RS6000_DEFAULT_LONG_DOUBLE_SIZE
#define RS6000_DEFAULT_LONG_DOUBLE_SIZE 64

#define POWERPC_NETBSD

/* The IEEE 128-bit emulator is only built on Linux systems.  Flag that we
   should enable the type handling for KFmode on VSX systems even if we are not
   enabling the __float128 keyword.  */
#undef	TARGET_FLOAT128_ENABLE_TYPE
#define TARGET_FLOAT128_ENABLE_TYPE 1
