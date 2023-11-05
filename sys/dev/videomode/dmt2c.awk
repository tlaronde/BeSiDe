#! /usr/bin/awk -f
#	$NetBSD: modelines2c.awk,v 1.5 2011/03/21 19:32:26 jdc Exp $
#
# Copyright (c) 2023 Thierry Laronde <tlaronde@polynum.com>
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
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
BEGIN {
	printf "/*\t$NetBSD" "$\t*/\n\n"
	printf "/*\n"
	printf " * THIS FILE AUTOMATICALLY GENERATED.  DO NOT EDIT.\n"
	printf " *\n"
	printf " * generated from: %s\n", DMTSOURCES
	printf " */\n\n"

	printf "#include <sys/cdefs.h>\n"
	printf "#include <dev/videomode/videomode.h>\n\n"

	printf "/*\n"
	printf " * These macros improve readability.\n"
	printf " */\n"
	printf "#define HP VID_PHSYNC\n"
	printf "#define HN VID_NHSYNC\n"
	printf "#define VP VID_PVSYNC\n"
	printf "#define VN VID_NVSYNC\n"
	printf "#define I VID_INTERLACE\n"
	printf "#define RB VID_REDUCEDBLANKING\n"
	printf "\n"

	printf "const struct videomode videomode_list[] = {\n"

	nmodes = 0;
}
{
	if ($0 ~ /^#/ || $0 ~ /^[[:blank:]]*$/) next; # skip comments or empty lines

	# In a nutshell: everything is given according to the dot clock i.e.
	# is expressed in dots.
	#
	# One must know how long to draw; how long to wait before sending
	# the Hor Sync signal; how long to send the sync signal; how long to
	# wait before starting to draw again. So these, below, are actually
	# times, taking into account that the specification handles chars
	# as a unit for horizontal lines, that are eventually defined as a
	# number of pixels (dots):
	#
	# hdisplay: the line addressable length i.e. the H resolution
	# hsync_start: hdisplay + H_Right_Border + H_Front_Porch
	# hsync_end: hsync_start + Hor_Sync_Time
	# htotal: hsync_end + H_Back_Porch + H_Left_Border.
	#
	# Vertical is not considering lines, but frames. This is homologous:
	#
	# vdisplay: the vertical addressable length i.e. the V resolution
	# vsync_start: vdisplay + V_Bottom_Border + V_Front_Porch
	# vsync_end: vsync_start + Ver_Sync_Time
	# vtotal: vsync_end + V_Back_Porch + V_Top_Border.
	#
	# So from the definitions above, per direction, the numbers
	# increase.

	printf "\t{"
	printf " %d", int($5 * 1000) # dot clock in kHz

	# Horizontal specification:
	printf ", %d", $3
	printf ", %d", $3 + ($10 + $11) * $6 
	printf ", %d", $3 + ($10 + $11 + $12) * $6
	printf ", %d", $3 + ($10 + $11 + $12 + $13 + $14) * $6

	# Vertical specification:
	printf ", %d", $4
	printf ", %d", $4 + $15 + $16
	printf ", %d", $4 + $15 + $16 + $17
	printf ", %d", $4 + $15 + $16 + $17 + $18 + $19

	if ($8 == "+")
		flags = "HP"
	else
		flags = "HN"
	if ($9 == "+")
		flags = flags "|" "VP"
	else
		flags = flags "|" "VN"

	split($7, flag, ",")
	if (flag[1] == "I") flags = flags "|" "I"
	if (flag[2] == "RB" || flag[2] == "RB2") flags = flags "|" "RB"

	printf ", %s", flags

	printf ", \"%s\"", $1

	printf " },\n"
	++nmodes
}

END {
	printf "};\n\n"
	printf "const int videomode_count = %d;\n", nmodes
}
