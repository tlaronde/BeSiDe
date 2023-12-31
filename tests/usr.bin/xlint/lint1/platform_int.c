/*	$NetBSD: platform_int.c,v 1.6 2023/07/09 10:42:07 rillig Exp $	*/
# 3 "platform_int.c"

/*
 * Test features that only apply to platforms on which size_t is unsigned
 * int and ptr_diff is signed int.
 */

/* lint1-only-if: int */
/* lint1-flags: -g -w -c -h -a -p -b -r -z -X 351 */

void to_size(typeof(sizeof(int)));

/* See should_warn_about_prototype_conversion. */
void
convert_unsigned_char_to_size(unsigned char uc)
{
	/*
	 * In this function call, uc is first promoted to INT. It is then
	 * converted to size_t, which is UINT. The portable bit size of INT
	 * and UINT is the same, 32, but the signedness changes, therefore
	 * the warning.
	 */
	/* expect+1: warning: argument 1 is converted from 'unsigned char' to 'unsigned int' due to prototype [259] */
	to_size(uc);
}
