# $NetBSD: t_ssp.sh,v 1.8 2023/11/10 23:03:25 christos Exp $
#
# Copyright (c) 2008 The NetBSD Foundation, Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
# ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

h_pass()
{
	echo "Executing command [ $2$1 ]"
	eval $2 atf_check -s exit:0 -o ignore -e ignore $1
}

h_fail()
{
	echo "Executing command [ $2$1 ]"
	eval $2 atf_check -s signal:6 -o ignore -e ignore $1
}

atf_test_case sprintf
sprintf_head()
{
	atf_set "descr" "Checks sprintf(3)"
}
sprintf_body()
{
	prog="$(atf_get_srcdir)/h_sprintf"

	h_pass "$prog ok"
	h_fail "$prog 0123456789"
}

atf_test_case vsprintf
vsprintf_head()
{
	atf_set "descr" "Checks vsprintf(3)"
}
vsprintf_body()
{
	prog="$(atf_get_srcdir)/h_vsprintf"

	h_pass "$prog ok"
	h_fail "$prog 0123456789"
}

atf_test_case snprintf
snprintf_head()
{
	atf_set "descr" "Checks snprintf(3)"
}
snprintf_body()
{
	prog="$(atf_get_srcdir)/h_snprintf"

	h_pass "$prog 10"
	h_fail "$prog 11"
}

atf_test_case vsnprintf
vsnprintf_head()
{
	atf_set "descr" "Checks vsnprintf(3)"
}
vsnprintf_body()
{
	prog="$(atf_get_srcdir)/h_vsnprintf"

	h_pass "$prog 10"
	h_fail "$prog 11"
}

atf_test_case gets
gets_head()
{
	atf_set "descr" "Checks gets(3)"
}
gets_body()
{
	prog="$(atf_get_srcdir)/h_gets"

	h_pass "$prog" "echo ok |"
	h_fail "$prog" "echo 0123456789 |"
}

atf_test_case fgets
fgets_head()
{
	atf_set "descr" "Checks fgets(3)"
}
fgets_body()
{
	prog="$(atf_get_srcdir)/h_fgets"

	h_pass "$prog 10" "echo ok |"
	h_fail "$prog 11" "echo busted |"
}

atf_test_case memcpy
memcpy_head()
{
	atf_set "descr" "Checks memcpy(3)"
}
memcpy_body()
{
	prog="$(atf_get_srcdir)/h_memcpy"

	h_pass "$prog 10"
	h_fail "$prog 11"
}

atf_test_case memmove
memmove_head()
{
	atf_set "descr" "Checks memmove(3)"
}
memmove_body()
{
	prog="$(atf_get_srcdir)/h_memmove"

	h_pass "$prog 10"
	h_fail "$prog 11"
}

atf_test_case memset
memset_head()
{
	atf_set "descr" "Checks memset(3)"
}
memset_body()
{
	prog="$(atf_get_srcdir)/h_memset"

	h_pass "$prog 10"
	h_fail "$prog 11"
}

atf_test_case strcpy
strcpy_head()
{
	atf_set "descr" "Checks strcpy(3)"
}
strcpy_body()
{
	prog="$(atf_get_srcdir)/h_strcpy"

	h_pass "$prog 0123456"
	h_fail "$prog 0123456789"
}

atf_test_case stpcpy
stpcpy_head()
{
	atf_set "descr" "Checks stpcpy(3)"
}
stpcpy_body()
{
	prog="$(atf_get_srcdir)/h_stpcpy"

	h_pass "$prog 0123456"
	h_fail "$prog 0123456789"
}

atf_test_case strcat
strcat_head()
{
	atf_set "descr" "Checks strcat(3)"
}
strcat_body()
{
	prog="$(atf_get_srcdir)/h_strcat"

	h_pass "$prog 0123456"
	h_fail "$prog 0123456789ABCDEF"
}

atf_test_case strncpy
strncpy_head()
{
	atf_set "descr" "Checks strncpy(3)"
}
strncpy_body()
{
	prog="$(atf_get_srcdir)/h_strncpy"

	h_pass "$prog 10"
	h_fail "$prog 11"
}

atf_test_case stpncpy
stpncpy_head()
{
	atf_set "descr" "Checks stpncpy(3)"
}
stpncpy_body()
{
	prog="$(atf_get_srcdir)/h_stpncpy"

	h_pass "$prog 10"
	h_fail "$prog 11"
}

atf_test_case strncat
strncat_head()
{
	atf_set "descr" "Checks strncat(3)"
}
strncat_body()
{
	prog="$(atf_get_srcdir)/h_strncat"

	h_pass "$prog 8"
	h_fail "$prog 9"
}

atf_test_case raw
raw_head()
{
	atf_set "descr" "Checks raw array overflow"
}
raw_body()
{
	prog="$(atf_get_srcdir)/h_raw"

	h_pass "$prog 9"
	h_fail "$prog 10"
}

atf_test_case read
read_head()
{
	atf_set "descr" "Checks read(2)"
}
read_body()
{
	prog="$(atf_get_srcdir)/h_read"

	h_pass "$prog 1024" "echo foo |"
	h_fail "$prog 1025" "echo bar |"
}

atf_test_case readlink
readlink_head()
{
	atf_set "descr" "Checks readlink(2)"
}
readlink_body()
{
	prog="$(atf_get_srcdir)/h_readlink"

	h_pass "$prog 1024"
	h_fail "$prog 1025"
}

atf_test_case getcwd
getcwd_head()
{
	atf_set "descr" "Checks getcwd(3)"
}
getcwd_body()
{
	prog="$(atf_get_srcdir)/h_getcwd"

	h_pass "$prog 1024"
	h_fail "$prog 1025"
}

atf_test_case getcwd2
getcwd2_head()
{
	atf_set "descr" "Checks getcwd(3) override"
}
getcwd2_body()
{
	prog="$(atf_get_srcdir)/h_getcwd2"

	atf_check -s exit:1 -o ignore \
	    -e 'match:.*getcwd failed: Function not implemented$' $prog
}

atf_init_test_cases()
{
	atf_add_test_case sprintf
	atf_add_test_case vsprintf
	atf_add_test_case snprintf
	atf_add_test_case vsnprintf
	atf_add_test_case gets
	atf_add_test_case fgets
	atf_add_test_case memcpy
	atf_add_test_case memmove
	atf_add_test_case memset
	atf_add_test_case stpcpy
	atf_add_test_case stpncpy
	atf_add_test_case strcat
	atf_add_test_case strcpy
	atf_add_test_case strncat
	atf_add_test_case strncpy
	atf_add_test_case raw
	atf_add_test_case read
	atf_add_test_case readlink
	atf_add_test_case getcwd
	atf_add_test_case getcwd2
}
