/*	$NetBSD: db_interface.h,v 1.44 2023/11/21 14:35:01 riastradh Exp $	*/

/*-
 * Copyright (c) 1995, 2023 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _DDB_DB_INTERFACE_H_
#define _DDB_DB_INTERFACE_H_

/* arch/<arch>/<arch>/db_disasm.c */
db_addr_t	db_disasm(db_addr_t, bool);

/* arch/<arch>/<arch>/db_trace.c */
/* arch/vax/vax/db_machdep.c */
void		db_stack_trace_print(db_expr_t, bool, db_expr_t, const char *,
    void (*)(const char *, ...) __printflike(1, 2));

/* ddb/db_xxx.c */
void		db_kgdb_cmd(db_expr_t, bool, db_expr_t, const char *);
void		db_show_files_cmd(db_expr_t, bool, db_expr_t, const char *);
void		db_show_panic(db_expr_t, bool, db_expr_t, const char *);

/* kern/kern_proc.c */
void		db_kill_proc(db_expr_t, bool, db_expr_t, const char *);
void		db_show_proc(db_expr_t, bool, db_expr_t, const char *);
void		db_show_all_procs(db_expr_t, bool, db_expr_t, const char *);
void		db_show_all_pools(db_expr_t, bool, db_expr_t, const char *);
void		db_show_sched_qs(db_expr_t, bool, db_expr_t, const char *);

/* kern/kern_timeout.c */
void		db_show_callout(db_expr_t, bool, db_expr_t, const char *);

/* kern/subr_log.c */
void		db_dmesg(db_expr_t, bool, db_expr_t, const char *);

/* net/route.c */
void		db_show_routes(db_expr_t, bool, db_expr_t, const char *);

/* kern/vfs_aio.c */
void		db_show_aio_jobs(db_expr_t, bool, db_expr_t, const char *);

/* kern/sys_mqueue.c */
void		db_show_mqueue_cmd(db_expr_t, bool, db_expr_t, const char *);

/* kern/kern_module.c */
void		db_show_module_cmd(db_expr_t, bool, db_expr_t, const char *);

/* kern/subr_vmem.c */
void		db_show_all_vmems(db_expr_t, bool, db_expr_t, const char *);

/* kern/subr_autoconf.c */
void		db_show_all_devices(db_expr_t, bool, db_expr_t, const char *);
void		db_show_all_device(db_expr_t, bool, db_expr_t, const char *);

/* kern/subr_disk.c, dev/dksubr.c */
void		db_show_disk(db_expr_t, bool, db_expr_t, const char *);

/* kern/kern_sleepq.c */
void		db_show_sleepq(db_expr_t, bool, db_expr_t, const char *);

/* kern/kern_condvar.c */
void		db_show_condvar(db_expr_t, bool, db_expr_t, const char *);

/* kern/sys_select.c */
void		db_show_selinfo(db_expr_t, bool, db_expr_t, const char *);

#ifndef __HAVE_DB_STACK_TRACE_PRINT_RA
#define	db_stack_trace_print_ra(ra, have_ra, addr, have_addr, c, m, pr)	      \
	((void)(ra), (void)(have_ra),					      \
	    db_stack_trace_print(addr, have_addr, c, m, pr))
#endif

/* The db_stacktrace_print macro may be overridden by an MD macro */
#ifndef db_stacktrace_print
#define	db_stacktrace_print(prfunc) \
	db_stack_trace_print_ra(					      \
	    (db_expr_t)(intptr_t)__builtin_return_address(0), true,	      \
	    (db_expr_t)(intptr_t)__builtin_frame_address(0), true,	      \
	    65535, "", prfunc)
#endif	/* !db_stacktrace_print */

#define	db_stacktrace()		db_stacktrace_print(printf);
#define	db_ustacktrace()	db_stacktrace_print(uprintf);

#include <sys/syslog.h>

#ifdef _KERNEL

static __inline__ void
_db_log_wrapper(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vlog(LOG_INFO, fmt, ap);
	va_end(ap);
}

#define	db_lstacktrace()	db_stacktrace_print(_db_log_wrapper);

#endif

#endif /* _DDB_DB_INTERFACE_H_ */
