/******************************************************************************
 *
 * Module Name: aslmessages.c - Compiler error/warning message strings
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

#define _COMPONENT          ACPI_COMPILER
        ACPI_MODULE_NAME    ("aslmessages")


/*
 * Strings for message reporting levels, must match error
 * type string tables in aslmessages.c
 */
const char              *AslErrorLevel [ASL_NUM_REPORT_LEVELS] = {
    "Optimize",
    "Remark  ",
    "Warning ",
    "Warning ",
    "Warning ",
    "Error   "
};

/* All lowercase versions for IDEs */

const char              *AslErrorLevelIde [ASL_NUM_REPORT_LEVELS] = {
    "optimize",
    "remark  ",
    "warning ",
    "warning ",
    "warning ",
    "error   "
};


/*
 * Actual message strings for each compiler message ID. There are currently
 * three distinct blocks of error messages (so that they can be expanded
 * individually):
 *      Main ASL compiler
 *      Data Table compiler
 *      Preprocessor
 *
 * NOTE1: These tables must match the enum list of message IDs in the file
 * aslmessages.h exactly.
 *
 * NOTE2: With the introduction of the -vw option to disable specific messages,
 * new messages should only be added to the end of this list, so that values
 * for existing messages are not disturbed. As important, obsolete messages
 * cannot be removed from this list, as it will affect the -vw option.
 */

/* ASL compiler */

const char                      *AslCompilerMsgs [] =
{
/*    The zeroth message is reserved */    "",
/*    ASL_MSG_ALIGNMENT */                  "Must be a multiple of alignment/granularity value",
/*    ASL_MSG_ALPHANUMERIC_STRING */        "String must be entirely alphanumeric",
/*    ASL_MSG_AML_NOT_IMPLEMENTED */        "Opcode is not implemented in compiler AML code generator",
/*    ASL_MSG_ARG_COUNT_HI */               "Too many arguments",
/*    ASL_MSG_ARG_COUNT_LO */               "Too few arguments",
/*    ASL_MSG_ARG_INIT */                   "Method argument is not initialized",
/*    ASL_MSG_BACKWARDS_OFFSET */           "Invalid backwards offset",
/*    ASL_MSG_BUFFER_LENGTH */              "Effective AML buffer length is zero",
/*    ASL_MSG_CLOSE */                      "Could not close file",
/*    ASL_MSG_COMPILER_INTERNAL */          "Internal compiler error",
/*    ASL_MSG_COMPILER_RESERVED */          "Use of compiler reserved name",
/*    ASL_MSG_CONNECTION_MISSING */         "A Connection operator is required for this field SpaceId",
/*    ASL_MSG_CONNECTION_INVALID */         "Invalid OpRegion SpaceId for use of Connection operator",
/*    ASL_MSG_CONSTANT_EVALUATION */        "Could not evaluate constant expression",
/*    ASL_MSG_CONSTANT_FOLDED */            "Constant expression evaluated and reduced",
/*    ASL_MSG_CORE_EXCEPTION */             "From ACPICA Subsystem",
/*    ASL_MSG_DEBUG_FILE_OPEN */            "Could not open debug file",
/*    ASL_MSG_DEBUG_FILENAME */             "Could not create debug filename",
/*    ASL_MSG_DEPENDENT_NESTING */          "Dependent function macros cannot be nested",
/*    ASL_MSG_DMA_CHANNEL */                "Invalid DMA channel (must be 0-7)",
/*    ASL_MSG_DMA_LIST */                   "Too many DMA channels (8 max)",
/*    ASL_MSG_DUPLICATE_CASE */             "Case value already specified",
/*    ASL_MSG_DUPLICATE_ITEM */             "Duplicate value in list",
/*    ASL_MSG_EARLY_EOF */                  "Premature end-of-file reached",
/*    ASL_MSG_ENCODING_LENGTH */            "Package length too long to encode",
/*    ASL_MSG_EX_INTERRUPT_LIST */          "Too many interrupts (255 max)",
/*    ASL_MSG_EX_INTERRUPT_LIST_MIN */      "Too few interrupts (1 minimum required)",
/*    ASL_MSG_EX_INTERRUPT_NUMBER */        "Invalid interrupt number (must be 32 bits)",
/*    ASL_MSG_FIELD_ACCESS_WIDTH */         "Access width is greater than region size",
/*    ASL_MSG_FIELD_UNIT_ACCESS_WIDTH */    "Access width of Field Unit extends beyond region limit",
/*    ASL_MSG_FIELD_UNIT_OFFSET */          "Field Unit extends beyond region limit",
/*    ASL_MSG_GPE_NAME_CONFLICT */          "Name conflicts with a previous GPE method",
/*    ASL_MSG_HID_LENGTH */                 "_HID string must be exactly 7 or 8 characters",
/*    ASL_MSG_HID_PREFIX */                 "_HID prefix must be all uppercase or decimal digits",
/*    ASL_MSG_HID_SUFFIX */                 "_HID suffix must be all hex digits",
/*    ASL_MSG_INCLUDE_FILE_OPEN */          "Could not open include file",
/*    ASL_MSG_INPUT_FILE_OPEN */            "Could not open input file",
/*    ASL_MSG_INTEGER_LENGTH */             "Truncating 64-bit constant found in 32-bit table",
/*    ASL_MSG_INTEGER_OPTIMIZATION */       "Integer optimized to single-byte AML opcode",
/*    ASL_MSG_INTERRUPT_LIST */             "Too many interrupts (16 max)",
/*    ASL_MSG_INTERRUPT_NUMBER */           "Invalid interrupt number (must be 0-15)",
/*    ASL_MSG_INVALID_ACCESS_SIZE */        "Invalid AccessSize (Maximum is 4 - QWord access)",
/*    ASL_MSG_INVALID_ADDR_FLAGS */         "Invalid combination of Length and Min/Max fixed flags",
/*    ASL_MSG_INVALID_CONSTANT_OP */        "Invalid operator in constant expression (not type 3/4/5)",
/*    ASL_MSG_INVALID_EISAID */             "EISAID string must be of the form \"UUUXXXX\" (3 uppercase, 4 hex digits)",
/*    ASL_MSG_INVALID_ESCAPE */             "Invalid or unknown escape sequence",
/*    ASL_MSG_INVALID_GRAN_FIXED */         "Granularity must be zero for fixed Min/Max",
/*    ASL_MSG_INVALID_GRANULARITY */        "Granularity must be zero or a power of two minus one",
/*    ASL_MSG_INVALID_LENGTH */             "Length is larger than Min/Max window",
/*    ASL_MSG_INVALID_LENGTH_FIXED */       "Length is not equal to fixed Min/Max window",
/*    ASL_MSG_INVALID_MIN_MAX */            "Address Min is greater than Address Max",
/*    ASL_MSG_INVALID_OPERAND */            "Invalid operand",
/*    ASL_MSG_INVALID_PERFORMANCE */        "Invalid performance/robustness value",
/*    ASL_MSG_INVALID_PRIORITY */           "Invalid priority value",
/*    ASL_MSG_INVALID_STRING */             "Invalid Hex/Octal Escape - Non-ASCII or NULL",
/*    ASL_MSG_INVALID_TARGET */             "Target operand not allowed in constant expression",
/*    ASL_MSG_INVALID_TIME */               "Time parameter too long (255 max)",
/*    ASL_MSG_INVALID_TYPE */               "Invalid type",
/*    ASL_MSG_INVALID_UUID */               "UUID string must be of the form \"aabbccdd-eeff-gghh-iijj-kkllmmnnoopp\"",
/*    ASL_MSG_ISA_ADDRESS */                "Maximum 10-bit ISA address (0x3FF)",
/*    ASL_MSG_LEADING_ASTERISK */           "Invalid leading asterisk",
/*    ASL_MSG_LIST_LENGTH_LONG */           "Initializer list longer than declared package length",
/*    ASL_MSG_LIST_LENGTH_SHORT */          "Initializer list shorter than declared package length",
/*    ASL_MSG_LISTING_FILE_OPEN */          "Could not open listing file",
/*    ASL_MSG_LISTING_FILENAME */           "Could not create listing filename",
/*    ASL_MSG_LOCAL_INIT */                 "Method local variable is not initialized",
/*    ASL_MSG_LOCAL_OUTSIDE_METHOD */       "Local or Arg used outside a control method",
/*    ASL_MSG_LONG_LINE */                  "Splitting long input line",
/*    ASL_MSG_MEMORY_ALLOCATION */          "Memory allocation failure",
/*    ASL_MSG_MISSING_ENDDEPENDENT */       "Missing EndDependentFn() macro in dependent resource list",
/*    ASL_MSG_MISSING_STARTDEPENDENT */     "Missing StartDependentFn() macro in dependent resource list",
/*    ASL_MSG_MULTIPLE_DEFAULT */           "More than one Default statement within Switch construct",
/*    ASL_MSG_MULTIPLE_TYPES */             "Multiple types",
/*    ASL_MSG_NAME_EXISTS */                "Name already exists in scope",
/*    ASL_MSG_NAME_OPTIMIZATION */          "NamePath optimized",
/*    ASL_MSG_NAMED_OBJECT_IN_WHILE */      "Creating a named object in a While loop",
/*    ASL_MSG_NESTED_COMMENT */             "Nested comment found",
/*    ASL_MSG_NO_CASES */                   "No Case statements under Switch",
/*    ASL_MSG_NO_REGION */                  "_REG has no corresponding Operation Region",
/*    ASL_MSG_NO_RETVAL */                  "Called method returns no value",
/*    ASL_MSG_NO_WHILE */                   "No enclosing While statement",
/*    ASL_MSG_NON_ASCII */                  "Invalid characters found in file",
/*    ASL_MSG_BUFFER_FIELD_LENGTH */        "Field length must be non-zero",
/*    ASL_MSG_NOT_EXIST */                  "Object does not exist",
/*    ASL_MSG_NOT_FOUND */                  "Object not found or not accessible from current scope",
/*    ASL_MSG_NOT_METHOD */                 "Not a control method, cannot invoke",
/*    ASL_MSG_NOT_PARAMETER */              "Not a parameter, used as local only",
/*    ASL_MSG_NOT_REACHABLE */              "Object is not accessible from this scope",
/*    ASL_MSG_NOT_REFERENCED */             "Object is not referenced",
/*    ASL_MSG_NULL_DESCRIPTOR */            "Min/Max/Length/Gran are all zero, but no resource tag",
/*    ASL_MSG_NULL_STRING */                "Invalid zero-length (null) string",
/*    ASL_MSG_OPEN */                       "Could not open file",
/*    ASL_MSG_OUTPUT_FILE_OPEN */           "Could not open output AML file",
/*    ASL_MSG_OUTPUT_FILENAME */            "Could not create output filename",
/*    ASL_MSG_PACKAGE_LENGTH */             "Effective AML package length is zero",
/*    ASL_MSG_PREPROCESSOR_FILENAME */      "Could not create preprocessor filename",
/*    ASL_MSG_READ */                       "Could not read file",
/*    ASL_MSG_RECURSION */                  "Recursive method call",
/*    ASL_MSG_REGION_BUFFER_ACCESS */       "Host Operation Region requires BufferAcc access",
/*    ASL_MSG_REGION_BYTE_ACCESS */         "Host Operation Region requires ByteAcc access",
/*    ASL_MSG_RESERVED_ARG_COUNT_HI */      "Reserved method has too many arguments",
/*    ASL_MSG_RESERVED_ARG_COUNT_LO */      "Reserved method has too few arguments",
/*    ASL_MSG_RESERVED_METHOD */            "Reserved name must be a control method",
/*    ASL_MSG_RESERVED_NO_RETURN_VAL */     "Reserved method should not return a value",
/*    ASL_MSG_RESERVED_OPERAND_TYPE */      "Invalid object type for reserved name",
/*    ASL_MSG_RESERVED_PACKAGE_LENGTH */    "Invalid package length for reserved name",
/*    ASL_MSG_RESERVED_RETURN_VALUE */      "Reserved method must return a value",
/*    ASL_MSG_RESERVED_USE */               "Invalid use of reserved name",
/*    ASL_MSG_RESERVED_WORD */              "Use of reserved name",
/*    ASL_MSG_RESOURCE_FIELD */             "Resource field name cannot be used as a target",
/*    ASL_MSG_RESOURCE_INDEX */             "Missing ResourceSourceIndex (required)",
/*    ASL_MSG_RESOURCE_LIST */              "Too many resource items (internal error)",
/*    ASL_MSG_RESOURCE_SOURCE */            "Missing ResourceSource string (required)",
/*    ASL_MSG_RESULT_NOT_USED */            "Result is not used, operator has no effect",
/*    ASL_MSG_RETURN_TYPES */               "Not all control paths return a value",
/*    ASL_MSG_SCOPE_FWD_REF */              "Forward references from Scope operator not allowed",
/*    ASL_MSG_SCOPE_TYPE */                 "Existing object has invalid type for Scope operator",
/*    ASL_MSG_SEEK */                       "Could not seek file",
/*    ASL_MSG_SERIALIZED */                 "Control Method marked Serialized",
/*    ASL_MSG_SERIALIZED_REQUIRED */        "Control Method should be made Serialized due to creation of named objects within",
/*    ASL_MSG_SINGLE_NAME_OPTIMIZATION */   "NamePath optimized to NameSeg (uses run-time search path)",
/*    ASL_MSG_SOME_NO_RETVAL */             "Called method may not always return a value",
/*    ASL_MSG_STRING_LENGTH */              "String literal too long",
/*    ASL_MSG_SWITCH_TYPE */                "Switch expression is not a static Integer/Buffer/String data type, defaulting to Integer",
/*    ASL_MSG_SYNC_LEVEL */                 "SyncLevel must be in the range 0-15",
/*    ASL_MSG_SYNTAX */                     "",
/*    ASL_MSG_TABLE_SIGNATURE */            "Invalid Table Signature",
/*    ASL_MSG_TAG_LARGER */                 "ResourceTag larger than Field",
/*    ASL_MSG_TAG_SMALLER */                "ResourceTag smaller than Field",
/*    ASL_MSG_TIMEOUT */                    "Result is not used, possible operator timeout will be missed",
/*    ASL_MSG_TOO_MANY_TEMPS */             "Method requires too many temporary variables (_T_x)",
/*    ASL_MSG_TRUNCATION */                 "64-bit return value will be truncated to 32 bits (DSDT or SSDT version < 2)",
/*    ASL_MSG_UNKNOWN_RESERVED_NAME */      "Unknown reserved name",
/*    ASL_MSG_UNREACHABLE_CODE */           "Statement is unreachable",
/*    ASL_MSG_UNSUPPORTED */                "Unsupported feature",
/*    ASL_MSG_UPPER_CASE */                 "Non-hex letters must be upper case",
/*    ASL_MSG_VENDOR_LIST */                "Too many vendor data bytes (7 max)",
/*    ASL_MSG_WRITE */                      "Could not write file",
/*    ASL_MSG_RANGE */                      "Constant out of range",
/*    ASL_MSG_BUFFER_ALLOCATION */          "Could not allocate line buffer",
/*    ASL_MSG_MISSING_DEPENDENCY */         "Missing dependency",
/*    ASL_MSG_ILLEGAL_FORWARD_REF */        "Illegal forward reference",
/*    ASL_MSG_ILLEGAL_METHOD_REF */         "Object is declared in a different method",
/*    ASL_MSG_LOCAL_NOT_USED */             "Method Local is set but never used",
/*    ASL_MSG_ARG_AS_LOCAL_NOT_USED */      "Method Argument (as a local) is set but never used",
/*    ASL_MSG_ARG_NOT_USED */               "Method Argument is never used",
/*    ASL_MSG_CONSTANT_REQUIRED */          "Non-reducible expression",
/*    ASL_MSG_CROSS_TABLE_SCOPE */          "Illegal open scope on external object from within DSDT",
/*    ASL_MSG_EXCEPTION_NOT_RECEIVED */     "Expected remark, warning, or error did not occur. Message ID:",
/*    ASL_MSG_NULL_RESOURCE_TEMPLATE */     "Empty Resource Template (END_TAG only)",
/*    ASL_MSG_FOUND_HERE */                 "Original name creation/declaration below: ",
/*    ASL_MSG_ILLEGAL_RECURSION */          "Illegal recursive call to method that creates named objects",
/*    ASL_MSG_DUPLICATE_INPUT_FILE */       "Duplicate input files detected:",
/*    ASL_MSG_WARNING_AS_ERROR */           "Warnings detected during compilation",
/*    ASL_MSG_OEM_TABLE_ID */               "Invalid OEM Table ID",
/*    ASL_MSG_OEM_ID */                     "Invalid OEM ID",
/*    ASL_MSG_UNLOAD */                     "Unload is not supported by all operating systems",
/*    ASL_MSG_OFFSET */                     "Unnecessary/redundant use of Offset operator",
/*    ASL_MSG_LONG_SLEEP */                 "Very long Sleep, greater than 1 second",
/*    ASL_MSG_PREFIX_NOT_EXIST */           "One or more prefix Scopes do not exist",
/*    ASL_MSG_NAMEPATH_NOT_EXIST */         "One or more objects within the Pathname do not exist",
/*    ASL_MSG_REGION_LENGTH */              "Operation Region declared with zero length",
/*    ASL_MSG_TEMPORARY_OBJECT */           "Object is created temporarily in another method and cannot be accessed",
/*    ASL_MSG_UNDEFINED_EXTERNAL */         "Named object was declared external but the actual definition does not exist",
/*    ASL_MSG_BUFFER_FIELD_OVERFLOW */      "Buffer field extends beyond end of target buffer",
/*    ASL_MSG_INVALID_SPECIAL_NAME */       "declaration of this named object outside root scope is illegal",
/*    ASL_MSG_INVALID_PROCESSOR_UID */      "_UID inside processor declaration must be an integer",
/*    ASL_MSG_LEGACY_PROCESSOR_OP */        "Legacy Processor() keyword detected. Use Device() keyword instead.",
/*    ASL_MSG_NAMESTRING_LENGTH */          "NameString contains too many NameSegs (>255)",
/*    ASL_MSG_CASE_FOUND_HERE */            "Original Case value below:",
/*    ASL_MSG_EXTERN_INVALID_RET_TYPE */    "Return type is only allowed for Externals declared as MethodObj",
/*    ASL_MSG_EXTERN_INVALID_PARAM_TYPE */  "Parameter type is only allowed for Externals declared as MethodObj",
/*    ASL_MSG_NAMED_OBJECT_CREATION */      "Creation of named objects within a method is highly inefficient, use globals or method local variables instead",
/*    ASL_MSG_ARG_COUNT_MISMATCH */         "Method NumArgs count does not match length of ParameterTypes list",
/*    ASL_MSG_STATIC_OPREGION_IN_METHOD */  "Static OperationRegion should be declared outside control method",
/*    ASL_MSG_DECLARATION_TYPE_MISMATCH */  "Type mismatch between external declaration and actual object declaration detected",
/*    ASL_MSG_TYPE_MISMATCH_FOUND_HERE */   "Actual object declaration:",
/*    ASL_MSG_DUPLICATE_EXTERN_MISMATCH */  "Type mismatch between multiple external declarations detected",
/*    ASL_MSG_DUPLICATE_EXTERN_FOUND_HERE */"Duplicate external declaration:",
/*    ASL_MSG_CONDREF_NEEDS_EXTERNAL_DECL */"CondRefOf parameter requires External() declaration",
/*    ASL_MSG_EXTERNAL_FOUND_HERE */        "External declaration below ",
/*    ASL_MSG_LOWER_CASE_NAMESEG */         "At least one lower case letter found in NameSeg, ASL is case insensitive - converting to upper case",
/*    ASL_MSG_LOWER_CASE_NAMEPATH */        "At least one lower case letter found in NamePath, ASL is case insensitive - converting to upper case",
/*    ASL_MSG_UUID_NOT_FOUND */             "Unknown UUID string",
/*    ASL_MSG_LEGACY_DDB_TYPE */            "DDBHandleObj has been deprecated along with the Unload operator. DDBHandlObj objects are only used in Unload"
};

/* Table compiler */

const char                      *AslTableCompilerMsgs [] =
{
/*    ASL_MSG_BUFFER_ELEMENT */             "Invalid element in buffer initializer list",
/*    ASL_MSG_DIVIDE_BY_ZERO */             "Expression contains divide-by-zero",
/*    ASL_MSG_FLAG_VALUE */                 "Flag value is too large",
/*    ASL_MSG_INTEGER_SIZE */               "Integer too large for target",
/*    ASL_MSG_INVALID_EXPRESSION */         "Invalid expression",
/*    ASL_MSG_INVALID_FIELD_NAME */         "Invalid Field Name",
/*    ASL_MSG_INVALID_HEX_INTEGER */        "Invalid hex integer constant",
/*    ASL_MSG_OEM_TABLE */                  "OEM table - unknown contents",
/*    ASL_MSG_RESERVED_FIELD */             "Reserved field",
/*    ASL_MSG_UNKNOWN_LABEL */              "Label is undefined",
/*    ASL_MSG_UNKNOWN_SUBTABLE */           "Unknown subtable type",
/*    ASL_MSG_UNKNOWN_TABLE */              "Unknown ACPI table signature",
/*    ASL_MSG_ZERO_VALUE */                 "Value must be non-zero",
/*    ASL_MSG_INVALID_LABEL */              "Invalid field label detected",
/*    ASL_MSG_BUFFER_LIST */                "Invalid buffer initializer list",
/*    ASL_MSG_ENTRY_LIST */                 "Invalid entry initializer list",
/*    ASL_MSG_UNKNOWN_FORMAT */             "Unknown format value",
/*    ASL_MSG_RESERVED_VALUE */             "Value for field is reserved or unknown",
/*    ASL_MSG_TWO_ZERO_VALUES */            "32-bit DSDT Address and 64-bit X_DSDT Address cannot both be zero",
/*    ASL_MSG_BAD_PARSE_TREE */             "Parse tree appears to be ill-defined"
};

/* Preprocessor */

const char                      *AslPreprocessorMsgs [] =
{
/*    ASL_MSG_DIRECTIVE_SYNTAX */           "Invalid directive syntax",
/*    ASL_MSG_ENDIF_MISMATCH */             "Mismatched #endif",
/*    ASL_MSG_ERROR_DIRECTIVE */            "#error",
/*    ASL_MSG_EXISTING_NAME */              "Name is already defined",
/*    ASL_MSG_INVALID_INVOCATION */         "Invalid macro invocation",
/*    ASL_MSG_MACRO_SYNTAX */               "Invalid macro syntax",
/*    ASL_MSG_TOO_MANY_ARGUMENTS */         "Too many macro arguments",
/*    ASL_MSG_UNKNOWN_DIRECTIVE */          "Unknown directive",
/*    ASL_MSG_UNKNOWN_PRAGMA */             "Unknown pragma",
/*    ASL_MSG_WARNING_DIRECTIVE */          "#warning",
/*    ASL_MSG_INCLUDE_FILE */               "Found a # preprocessor directive in ASL Include() file"
};


/*******************************************************************************
 *
 * FUNCTION:    AeDecodeMessageId
 *
 * PARAMETERS:  MessageId               - ASL message ID (exception code) to be
 *                                        formatted. Possibly fully encoded.
 *
 * RETURN:      A string containing the exception message text.
 *
 * DESCRIPTION: This function validates and translates an ASL message ID into
 *              an ASCII string.
 *
 ******************************************************************************/

const char *
AeDecodeMessageId (
    UINT16                  MessageId)
{
    UINT32                  Index;
    const char              **MessageTable;


    /* Main ASL Compiler messages */

    if (MessageId <= ASL_MSG_MAIN_COMPILER_END)
    {
        MessageTable = AslCompilerMsgs;
        Index = MessageId;

        if (Index >= ACPI_ARRAY_LENGTH (AslCompilerMsgs))
        {
            return ("[Unknown iASL Compiler exception ID]");
        }
    }

    /* Data Table Compiler messages */

    else if (MessageId <= ASL_MSG_TABLE_COMPILER_END)
    {
        MessageTable = AslTableCompilerMsgs;
        Index = MessageId - ASL_MSG_TABLE_COMPILER;

        if (Index >= ACPI_ARRAY_LENGTH (AslTableCompilerMsgs))
        {
            return ("[Unknown iASL Table Compiler exception ID]");
        }
    }

    /* Preprocessor messages */

    else if (MessageId <= ASL_MSG_PREPROCESSOR_END)
    {
        MessageTable = AslPreprocessorMsgs;
        Index = MessageId - ASL_MSG_PREPROCESSOR;

        if (Index >= ACPI_ARRAY_LENGTH (AslPreprocessorMsgs))
        {
            return ("[Unknown iASL Preprocessor exception ID]");
        }
    }

    /* Everything else is unknown */

    else
    {
        return ("[Unknown iASL exception ID]");
    }

    return (MessageTable[Index]);
}


/*******************************************************************************
 *
 * FUNCTION:    AeDecodeExceptionLevel
 *
 * PARAMETERS:  Level               - The ASL error level to be decoded
 *
 * RETURN:      A string containing the error level text
 *
 * DESCRIPTION: This function validates and translates an ASL error level into
 *              an ASCII string.
 *
 ******************************************************************************/

const char *
AeDecodeExceptionLevel (
    UINT8                   Level)
{
    /* Range check on Level */

    if (Level >= ACPI_ARRAY_LENGTH (AslErrorLevel))
    {
        return ("Unknown exception level");
    }

    /* Differentiate the string type to be used (IDE is all lower case) */

    if (AslGbl_VerboseErrors)
    {
        return (AslErrorLevel[Level]);
    }

    return (AslErrorLevelIde[Level]);
}


/*******************************************************************************
 *
 * FUNCTION:    AeBuildFullExceptionCode
 *
 * PARAMETERS:  Level               - ASL error level
 *              MessageId           - ASL exception code to be formatted
 *
 * RETURN:      Fully encoded exception code
 *
 * DESCRIPTION: Build the full exception code from the error level and the
 *              actual message ID.
 *
 ******************************************************************************/

UINT16
AeBuildFullExceptionCode (
    UINT8                   Level,
    UINT16                  MessageId)
{

    /*
     * Error level is in the thousands slot (error/warning/remark, etc.)
     * Error codes are 0 - 999
     */
    return (((Level + 1) * 1000) + MessageId);
}


#ifdef ACPI_HELP_APP
/*******************************************************************************
 *
 * FUNCTION:    AhDecodeAslException
 *
 * PARAMETERS:  HexString           - iASL status string from command line, in
 *                                    hex. If null, display all exceptions.
 *
 * RETURN:      None
 *
 * DESCRIPTION: Decode and display an iASL exception code. Note1: a
 * NULL string for HexString displays all known iASL exceptions. Note2:
 * implements the -x option for AcpiHelp.
 *
 ******************************************************************************/

#define AH_DISPLAY_ASL_EXCEPTION_TEXT(Status, Exception) \
    printf ("%.4X: %s\n", Status, Exception)

#define AH_DISPLAY_EXCEPTION(Status, Name) \
    printf ("%.4X: %s\n", Status, Name)


void
AhDecodeAslException (
    char                    *HexString)
{
    UINT32                  i;
    UINT32                  MessageId;
    const char              *OneException;
    UINT32                  Index = 1;


    /*
     * A null input string means to decode and display all known
     * exception codes.
     */
    if (!HexString)
    {
        printf ("All defined iASL exception codes:\n\n");
        printf ("Main iASL exceptions:\n\n");
        AH_DISPLAY_EXCEPTION (0,
            "AE_OK                        (No error occurred)");

        /* Display codes in each block of exception types */

        for (i = 1; Index < ACPI_ARRAY_LENGTH (AslCompilerMsgs); i++, Index++)
        {
            AH_DISPLAY_ASL_EXCEPTION_TEXT (Index, AslCompilerMsgs[i]);
        }

        printf ("\niASL Table Compiler exceptions:\n\n");
        Index = ASL_MSG_TABLE_COMPILER;
        for (i = 0; i < ACPI_ARRAY_LENGTH (AslTableCompilerMsgs); i++, Index++)
        {
            AH_DISPLAY_ASL_EXCEPTION_TEXT (Index, AslTableCompilerMsgs[i]);
        }

        printf ("\niASL Preprocessor exceptions:\n\n");
        Index = ASL_MSG_PREPROCESSOR;
        for (i = 0; i < ACPI_ARRAY_LENGTH (AslPreprocessorMsgs); i++, Index++)
        {
            AH_DISPLAY_ASL_EXCEPTION_TEXT (Index, AslPreprocessorMsgs[i]);
        }
        return;
    }

    /* HexString is valid - convert it to a MessageId and decode it */

    MessageId = strtol (HexString, NULL, 16);
    OneException = AeDecodeMessageId ((UINT16) MessageId);
    AH_DISPLAY_ASL_EXCEPTION_TEXT (MessageId, OneException);
}
#endif
