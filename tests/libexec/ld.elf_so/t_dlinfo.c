/*	$NetBSD: t_dlinfo.c,v 1.8 2023/11/24 17:40:20 riastradh Exp $	*/

/*
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND
 * CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>

#include <atf-c.h>
#include <dlfcn.h>
#include <link_elf.h>

#include "h_macros.h"

ATF_TC(rtld_dlinfo_linkmap_self);
ATF_TC_HEAD(rtld_dlinfo_linkmap_self, tc)
{
	atf_tc_set_md_var(tc, "descr", "dlinfo with RTLD_SELF handle works");
}
ATF_TC_BODY(rtld_dlinfo_linkmap_self, tc)
{
	struct link_map *map;
	int rv;

	rv = dlinfo(RTLD_SELF, RTLD_DI_LINKMAP, &map);
	ATF_REQUIRE_EQ_MSG(rv, 0, "dlinfo: %s", dlerror());
	ATF_CHECK((strstr(map->l_name, "t_dlinfo") != NULL));
}

ATF_TC(rtld_dlinfo_linkmap_inval);
ATF_TC_HEAD(rtld_dlinfo_linkmap_inval, tc)
{
	atf_tc_set_md_var(tc, "descr", "dlinfo with invalid handle fails");
}
ATF_TC_BODY(rtld_dlinfo_linkmap_inval, tc)
{
	void *v;
	int rv;

	rv = dlinfo(NULL, RTLD_DI_LINKMAP, &v);
	ATF_CHECK_EQ_MSG(rv, -1, "rv=%d", rv);
}

ATF_TC(rtld_dlinfo_linkmap_dlopen);
ATF_TC_HEAD(rtld_dlinfo_linkmap_dlopen, tc)
{
	atf_tc_set_md_var(tc, "descr", "dlinfo dlopen'd handle works");
}
ATF_TC_BODY(rtld_dlinfo_linkmap_dlopen, tc)
{
	struct link_map *map;
	void *handle;
	int rv;

	handle = dlopen("libutil.so", RTLD_LAZY);
	ATF_REQUIRE_MSG(handle, "dlopen: %s", dlerror());

	rv = dlinfo(handle, RTLD_DI_LINKMAP, &map);
	ATF_REQUIRE_EQ_MSG(rv, 0, "dlinfo: %s", dlerror());
	ATF_CHECK((strstr(map->l_name, "libutil.so") != NULL));
	ATF_CHECK_EQ_MSG(dlclose(handle), 0, "dlclose: %s", dlerror());
}

ATF_TC(rtld_dlinfo_linkmap_dlopen_iter);
ATF_TC_HEAD(rtld_dlinfo_linkmap_dlopen_iter, tc)
{
	atf_tc_set_md_var(tc, "descr", "dlopen'd dso's show up in the list");
}
ATF_TC_BODY(rtld_dlinfo_linkmap_dlopen_iter, tc)
{
	struct link_map *map;
	void *handle;

	handle = dlopen("libutil.so", RTLD_LAZY);
	ATF_REQUIRE_MSG(handle, "dlopen: %s", dlerror());

	ATF_REQUIRE_EQ_MSG(dlinfo(RTLD_SELF, RTLD_DI_LINKMAP, &map), 0,
	    "dlinfo: %s", dlerror());

	for (; map->l_next; map = map->l_next)
		continue;
	for (; map; map = map->l_prev)
		if (strstr(map->l_name, "libutil.so") != NULL)
			break;

	ATF_REQUIRE_MSG(map, "dlopen()d object not found from linkmap");
	ATF_REQUIRE_MSG(dlopen(map->l_name, RTLD_LAZY) != NULL,
	    "could not dlopen() name in linkmap: %s", dlerror());
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, rtld_dlinfo_linkmap_self);
	ATF_TP_ADD_TC(tp, rtld_dlinfo_linkmap_inval);
	ATF_TP_ADD_TC(tp, rtld_dlinfo_linkmap_dlopen);
	ATF_TP_ADD_TC(tp, rtld_dlinfo_linkmap_dlopen_iter);
	return atf_no_error();
}
