/*	$NetBSD: ipsec.c,v 1.8 2021/09/03 21:02:04 rillig Exp $	*/

/*
 * Copyright (C) 1999 WIDE Project.
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#ifdef IPSEC
#include <netipsec/ipsec.h>
#ifndef IPSEC_POLICY_IPSEC	/* no ipsec support on old ipsec */
#undef IPSEC
#endif
#endif

#include "sap.h"
#include "ipsec.h"

#ifdef IPSEC
/*
 * Works with the last string pushed on SAP stack.
 */
int
ipsec_ck(void)
{
	int	status;
	int	npolicy;
	int	istr;
	char	*buf;

	npolicy = sap_split(';', -1);
	buf = NULL;
	status = 0;
	for (istr = sap_nstr - npolicy - 1; istr < sap_nstr; ++istr) {
		free(buf);
		if ( ( buf = ipsec_set_policy(SAP_STR(istr),
			SAP_LEN(istr)) ) == NULL ) {
			status = -1;
			break;
		} 
	}

	free(buf);
	sap_join(';', npolicy);

	return status;
}

int
ipsec_set(sa_family_t af, int fd, const char *ipsec)
{
	int	status;
	int	npolicy;
	int	istr;
	int	level, optname;
	char	*buf;

	if (ipsec == NULL)
		return 0;

	switch (af) {
	case AF_INET:
		level = IPPROTO_IP;
		optname = IP_IPSEC_POLICY;
		break;
#ifdef INET6
	case AF_INET6:
		level = IPPROTO_IPV6;
		optname = IPV6_IPSEC_POLICY;
		break;
#endif
	default:
		return 0;
	}

	sap_push(ipsec, -1, 0);
	if (sap_state != SAP_OK)
		return -1;
	npolicy = sap_split(';', -1);
	buf = NULL;
	status = 0;
	for (istr = sap_nstr - npolicy - 1; istr < sap_nstr; ++istr) {
		free(buf);
		/* can't failed: verified by parsing */
		buf = ipsec_set_policy(SAP_STR(istr), SAP_LEN(istr));
		assert(buf != NULL);
		if (setsockopt(fd, level, optname, buf,
			(socklen_t)ipsec_get_policylen(buf)) < 0) {
			status = -1;
			break;
		}	
	}

	free(buf);
	sap_join(';', npolicy);
	sap_pop();

	return status;
}
#endif
