%{
/******************************************************************************
 *
 * Module Name: prparser.l - Flex input file for preprocessor lexer
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2023, Intel Corp.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce at minimum a disclaimer
 *    substantially similar to the "NO WARRANTY" disclaimer below
 *    ("Disclaimer") and any redistribution must be conditioned upon
 *    including a substantially similar Disclaimer requirement for further
 *    binary redistribution.
 * 3. Neither the names of the above-listed copyright holders nor the names
 *    of any contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * NO WARRANTY
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES.
 */

#include "aslcompiler.h"
#include "prparser.y.h"

/* Buffer to pass strings to the parser */

#define STRING_SETUP    strcpy (AslGbl_StringBuffer, PrParsertext);\
    PrParserlval.str = AslGbl_StringBuffer

#define _COMPONENT          ACPI_COMPILER
        ACPI_MODULE_NAME    ("prscanner")


/* Local prototypes */

static char
PrDoCommentType1 (
    void);

static char
PrDoCommentType2 (
    void);
%}

%option noyywrap

Number          [0-9a-fA-F]+
HexNumber       0[xX][0-9a-fA-F]+
WhiteSpace      [ \t\v\r]+
NewLine         [\n]
Identifier      [a-zA-Z][0-9a-zA-Z]*

%%
"/*"            { if (!PrDoCommentType1 ()) {yyterminate ();} }
"//"            { if (!PrDoCommentType2 ()) {yyterminate ();} }

\(              return (EXPOP_PAREN_OPEN);
\)              return (EXPOP_PAREN_CLOSE);
\~              return (EXPOP_ONES_COMPLIMENT);
\!              return (EXPOP_LOGICAL_NOT);
\*              return (EXPOP_MULTIPLY);
\/              return (EXPOP_DIVIDE);
\%              return (EXPOP_MODULO);
\+              return (EXPOP_ADD);
\-              return (EXPOP_SUBTRACT);
">>"            return (EXPOP_SHIFT_RIGHT);
"<<"            return (EXPOP_SHIFT_LEFT);
\<              return (EXPOP_LESS);
\>              return (EXPOP_GREATER);
"<="            return (EXPOP_LESS_EQUAL);
">="            return (EXPOP_GREATER_EQUAL);
"=="            return (EXPOP_EQUAL);
"!="            return (EXPOP_NOT_EQUAL);
\&              return (EXPOP_AND);
\^              return (EXPOP_XOR);
\|              return (EXPOP_OR);
"&&"            return (EXPOP_LOGICAL_AND);
"||"            return (EXPOP_LOGICAL_OR);

"defined"       return (EXPOP_DEFINE);
{Identifier}    {STRING_SETUP; return (EXPOP_IDENTIFIER);}

<<EOF>>         return (EXPOP_EOF); /* null end-of-string */

{Number}        return (EXPOP_NUMBER);
{HexNumber}     return (EXPOP_HEX_NUMBER);
{NewLine}       return (EXPOP_NEW_LINE);
{WhiteSpace}    /* Ignore */

.               return (EXPOP_EOF);
%%

/*
 * Local support functions
 */
static YY_BUFFER_STATE         LexBuffer;


/******************************************************************************
 *
 * FUNCTION:    PrInitLexer
 *
 * PARAMETERS:  String              - Input string to be parsed
 *
 * RETURN:      TRUE if parser returns NULL. FALSE otherwise.
 *
 * DESCRIPTION: Initialization routine for lexer. The lexer needs
 *              a buffer to handle strings instead of a file.
 *
 *****************************************************************************/

int
PrInitLexer (
    char                    *String)
{

    LexBuffer = yy_scan_string (String);
    return (LexBuffer == NULL);
}


/******************************************************************************
 *
 * FUNCTION:    PrTerminateLexer
 *
 * PARAMETERS:  None
 *
 * RETURN:      None
 *
 * DESCRIPTION: Termination routine for thelexer.
 *
 *****************************************************************************/

void
PrTerminateLexer (
    void)
{

    yy_delete_buffer (LexBuffer);
}


/********************************************************************************
 *
 * FUNCTION:    PrDoCommentType1
 *
 * PARAMETERS:  none
 *
 * RETURN:      none
 *
 * DESCRIPTION: Process a new legacy comment. Just toss it.
 *
 ******************************************************************************/

static char
PrDoCommentType1 (
    void)
{
    int                 c;


Loop:
    while (((c = input ()) != '*') && (c != EOF))
    {
    }
    if (c == EOF)
    {
        return (FALSE);
    }

    if (((c = input ()) != '/') && (c != EOF))
    {
        unput (c);
        goto Loop;
    }
    if (c == EOF)
    {
        return (FALSE);
    }

    return (TRUE);
}


/********************************************************************************
 *
 * FUNCTION:    PrDoCommentType2
 *
 * PARAMETERS:  none
 *
 * RETURN:      none
 *
 * DESCRIPTION: Process a new "//" comment. Just toss it.
 *
 ******************************************************************************/

static char
PrDoCommentType2 (
    void)
{
    int                 c;


    while (((c = input ()) != '\n') && (c != EOF))
    {
    }
    if (c == EOF)
    {
        return (FALSE);
    }

    return (TRUE);
}
