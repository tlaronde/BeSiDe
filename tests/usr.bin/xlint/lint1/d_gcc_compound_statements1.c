/*	$NetBSD: d_gcc_compound_statements1.c,v 1.14 2023/07/02 22:56:13 rillig Exp $	*/
# 3 "d_gcc_compound_statements1.c"

/* GCC compound statement with expression */

/* lint1-extra-flags: -X 351 */

/*
 * Compound statements are only allowed in functions, not at file scope.
 *
 * Before decl.c 1.283 from 2022-05-31, lint crashed with a segmentation
 * fault due to the unused label.
 */
int invalid_gcc_statement_expression = ({
unused_label:
	3;
/* expect+2: error: syntax error 'labels are only valid inside a function' [249] */
/* expect+1: error: cannot initialize 'int' from 'void' [185] */
});

void foo(unsigned long z)
{
	z = ({
		unsigned long tmp;
		tmp = 1;
		tmp;
	});
	foo(z);
}

/*
 * Compound statements are only allowed in functions, not at file scope.
 *
 * Before decl.c 1.186 from 2021-06-19, lint crashed with a segmentation
 * fault.
 */
int c = ({
	/* expect+1: error: syntax error 'return outside function' [249] */
	return 3;
});
/* expect-1: error: cannot initialize 'int' from 'void' [185] */

void
function(void)
{
	/*
	 * Before cgram.y 1.229 from 2021-06-20, lint crashed due to the
	 * syntax error, which made an expression NULL.
	 */
	({
		/* expect+1: error: type 'int' does not have member 'e' [101] */
		0->e;
	});
}

void
crash(void)
{
	/*
	 * Before tree.c 1.418 from 2022-04-03, lint dereferenced a null
	 * pointer in do_statement_expr.
	 */
	({
		;
	});
}

/*
 * Before cgram.y 1.445 from 2023-07-03, lint did not accept empty statements
 * in GCC statement expressions.  These empty statements can be generated by a
 * disabled 'assert' macro.
 */
unsigned int
empty_statement(void)
{
	return ({
		unsigned int mega = 1 << 20;
		;
		mega;
	});
}
