/******************************************************************************
 *
 * Module Name: asltypes.h - compiler data types and struct definitions
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

#ifndef __ASLTYPES_H
#define __ASLTYPES_H


/*******************************************************************************
 *
 * Structure definitions
 *
 ******************************************************************************/


/* Op flags for the ACPI_PARSE_OBJECT */

#define OP_VISITED                  0x00000001
#define OP_AML_PACKAGE              0x00000002
#define OP_IS_TARGET                0x00000004
#define OP_IS_RESOURCE_DESC         0x00000008
#define OP_IS_RESOURCE_FIELD        0x00000010
#define OP_HAS_NO_EXIT              0x00000020
#define OP_IF_HAS_NO_EXIT           0x00000040
#define OP_NAME_INTERNALIZED        0x00000080
#define OP_METHOD_NO_RETVAL         0x00000100
#define OP_METHOD_SOME_NO_RETVAL    0x00000200
#define OP_RESULT_NOT_USED          0x00000400
#define OP_METHOD_TYPED             0x00000800
#define OP_COULD_NOT_REDUCE         0x00001000
#define OP_COMPILE_TIME_CONST       0x00002000
#define OP_IS_TERM_ARG              0x00004000
#define OP_WAS_ONES_OP              0x00008000
#define OP_IS_NAME_DECLARATION      0x00010000
#define OP_COMPILER_EMITTED         0x00020000
#define OP_IS_DUPLICATE             0x00040000
#define OP_IS_RESOURCE_DATA         0x00080000
#define OP_IS_NULL_RETURN           0x00100000
#define OP_NOT_FOUND_DURING_LOAD    0x00200000

#define ACPI_NUM_OP_FLAGS           0x22

/* Keeps information about individual control methods */

typedef struct asl_method_info
{
    ACPI_PARSE_OBJECT       *Op;
    ACPI_PARSE_OBJECT       *CurrentOp;
    struct asl_method_info  *Next;
    UINT32                  ValidArgTypes[ACPI_METHOD_NUM_ARGS];
    UINT32                  ValidReturnTypes;
    UINT32                  NumReturnNoValue;
    UINT32                  NumReturnWithValue;
    UINT8                   NumArguments;
    UINT8                   LocalInitialized[ACPI_METHOD_NUM_LOCALS];
    UINT8                   ArgInitialized[ACPI_METHOD_NUM_ARGS];
    UINT8                   HasBeenTyped;
    UINT8                   ShouldBeSerialized;
    UINT8                   CreatesNamedObjects;

} ASL_METHOD_INFO;


/* Parse tree walk info for control method analysis */

typedef struct asl_analysis_walk_info
{
    ASL_METHOD_INFO         *MethodStack;

} ASL_ANALYSIS_WALK_INFO;


/* An entry in the ParseOpcode to AmlOpcode mapping table */

typedef struct asl_mapping_entry
{
    UINT32                  Value;
    UINT32                  AcpiBtype;   /* Object type or return type */
    UINT16                  AmlOpcode;
    UINT8                   Flags;

} ASL_MAPPING_ENTRY;


/* Parse tree walk info structure */

typedef struct asl_walk_info
{
    ACPI_PARSE_OBJECT       **NodePtr;
    UINT32                  *LevelPtr;

} ASL_WALK_INFO;


/* File info */

typedef struct asl_file_info
{
    FILE                    *Handle;
    char                    *Filename;

} ASL_FILE_INFO;

typedef struct asl_file_status
{
    UINT32                  Line;
    UINT32                  Offset;

} ASL_FILE_STATUS;


typedef UINT32                      ASL_FILE_SWITCH_STATUS;    /* File switch status */
#define SWITCH_TO_DIFFERENT_FILE    0
#define SWITCH_TO_SAME_FILE         1
#define FILE_NOT_FOUND              2

/*
 * File types. Note: Any changes to this table must also be reflected
 * in the Gbl_Files array.
 *
 * Corresponding filename suffixes are in comments
 *
 * NOTE: Don't move the first 4 file types
 *
 * .xxx file extension: this is used as a temporary .aml file for
 * the ASL/ASL+ converter and is deleted after conversion. This file
 * should never be used in the interpreter.
 */
typedef enum
{
    ASL_FILE_STDOUT             = 0,
    ASL_FILE_STDERR,
    ASL_FILE_INPUT,             /* .asl */
    ASL_FILE_AML_OUTPUT,        /* .aml */
    ASL_FILE_SOURCE_OUTPUT,     /* .src */
    ASL_FILE_PREPROCESSOR,      /* .pre */
    ASL_FILE_PREPROCESSOR_USER, /* .i   */
    ASL_FILE_LISTING_OUTPUT,    /* .lst */
    ASL_FILE_HEX_OUTPUT,        /* .hex */
    ASL_FILE_NAMESPACE_OUTPUT,  /* .nsp */
    ASL_FILE_DEBUG_OUTPUT,      /* .txt */
    ASL_FILE_ASM_SOURCE_OUTPUT, /* .asm */
    ASL_FILE_C_SOURCE_OUTPUT,   /* .c   */
    ASL_FILE_ASM_INCLUDE_OUTPUT,/* .inc */
    ASL_FILE_C_INCLUDE_OUTPUT,  /* .h   */
    ASL_FILE_C_OFFSET_OUTPUT,   /* .offset.h */
    ASL_FILE_MAP_OUTPUT,        /* .map */
    ASL_FILE_XREF_OUTPUT,       /* .xrf */
    ASL_FILE_CONV_DEBUG_OUTPUT, /* .cdb */
    ASL_FILE_CONV_OUTPUT        /* .xxx */

} ASL_FILE_TYPES;

#define ASL_MAX_FILE_TYPE       18
#define ASL_NUM_FILES           (ASL_MAX_FILE_TYPE + 1)

typedef struct asl_file_desc
{
    const char              *ShortDescription;
    const char              *Description;

} ASL_FILE_DESC;


/* Name suffixes used to create filenames for output files */

#define FILE_SUFFIX_ASL_CODE        "asl"
#define FILE_SUFFIX_AML_CODE        "aml"
#define FILE_SUFFIX_SOURCE          "src"
#define FILE_SUFFIX_PREPROCESSOR    "pre"
#define FILE_SUFFIX_PREPROC_USER    "i"
#define FILE_SUFFIX_LISTING         "lst"
#define FILE_SUFFIX_HEX_DUMP        "hex"
#define FILE_SUFFIX_NAMESPACE       "nsp"
#define FILE_SUFFIX_DEBUG           "txt"
#define FILE_SUFFIX_ASM_SOURCE      "asm"
#define FILE_SUFFIX_C_SOURCE        "c"
#define FILE_SUFFIX_ASM_INCLUDE     "inc"
#define FILE_SUFFIX_C_INCLUDE       "h"
#define FILE_SUFFIX_C_OFFSET        "offset.h"
#define FILE_SUFFIX_MAP             "map"
#define FILE_SUFFIX_XREF            "xrf"
#define FILE_SUFFIX_CONVERT_AML     "xxx"
#define FILE_SUFFIX_CONVERT_DEBUG   "cdb"


/* Cache block structure for ParseOps and Strings */

typedef struct asl_cache_info
{
    void                    *Next;
    char                    Buffer[];

} ASL_CACHE_INFO;


typedef struct asl_include_dir
{
    char                    *Dir;
    struct asl_include_dir  *Next;

} ASL_INCLUDE_DIR;


/*
 * An entry in the exception list, one for each error/warning
 * Note: SubError nodes would be treated with the same messageId and Level
 * as the parent error node.
 *
 * The source filename represents the name of the .src of where the error
 * occurred. This is useful for errors that occur inside of include files.
 * Since include files aren't recorded as a part of the global files list,
 * this provides a way to get the included file.
 */
typedef struct asl_error_msg
{
    UINT32                      LineNumber;
    UINT32                      LogicalLineNumber;
    UINT32                      LogicalByteOffset;
    UINT32                      Column;
    char                        *Message;
    struct asl_error_msg        *Next;
    struct asl_error_msg        *SubError;
    char                        *Filename;
    char                        *SourceLine;
    char                        *SourceFilename;
    UINT32                      FilenameLength;
    UINT16                      MessageId;
    UINT8                       Level;

} ASL_ERROR_MSG;

/* An entry in the expected messages array */

typedef struct asl_expected_message
{
    UINT32                      MessageId;
    char                        *MessageIdStr;
    BOOLEAN                     MessageReceived;

} ASL_EXPECTED_MESSAGE;

/*
 * An entry in the line-based expected messages list
 *
 * TBD: might be possible to merge this with ASL_EXPECTED_MESSAGE
 */
typedef struct asl_expected_msg_node
{
    struct asl_expected_msg_node    *Next;
    UINT32                          MessageId;
    char                            *MessageIdStr;
    struct asl_location_node        *LocationList;

} ASL_EXPECTED_MSG_NODE;

typedef struct asl_location_node
{
    struct asl_location_node    *Next;
    char                        *Filename;
    UINT32                      LineNumber;
    UINT32                      Column;
    UINT32                      LogicalByteOffset;
    BOOLEAN                     MessageReceived;

} ASL_LOCATION_NODE;


/* An entry in the listing file stack (for include files) */

typedef struct asl_listing_node
{
    char                        *Filename;
    UINT32                      LineNumber;
    struct asl_listing_node     *Next;

} ASL_LISTING_NODE;


/* Callback interface for a parse tree walk */

/*
 * TBD - another copy of this is in adisasm.h, fix
 */
#ifndef ASL_WALK_CALLBACK_DEFINED
typedef
ACPI_STATUS (*ASL_WALK_CALLBACK) (
    ACPI_PARSE_OBJECT           *Op,
    UINT32                      Level,
    void                        *Context);
#define ASL_WALK_CALLBACK_DEFINED
#endif


typedef struct asl_event_info
{
    UINT64                      StartTime;
    UINT64                      EndTime;
    char                        *EventName;
    BOOLEAN                     Valid;

} ASL_EVENT_INFO;


/* Hardware mapping file structures */

typedef struct acpi_gpio_info
{
    struct acpi_gpio_info   *Next;
    ACPI_PARSE_OBJECT       *Op;
    char                    *DeviceName;
    ACPI_NAMESPACE_NODE     *TargetNode;
    UINT32                  References;
    UINT32                  PinCount;
    UINT32                  PinIndex;
    UINT16                  PinNumber;
    UINT8                   Type;
    UINT8                   Direction;
    UINT8                   Polarity;

} ACPI_GPIO_INFO;

typedef struct acpi_serial_info
{
    struct acpi_serial_info *Next;
    ACPI_PARSE_OBJECT       *Op;
    char                    *DeviceName;
    ACPI_NAMESPACE_NODE     *TargetNode;
    AML_RESOURCE            *Resource;
    UINT32                  Speed;
    UINT16                  Address;

} ACPI_SERIAL_INFO;

typedef struct asl_method_local
{
    ACPI_PARSE_OBJECT       *Op;
    UINT8                   Flags;

} ASL_METHOD_LOCAL;

/* Values for Flags field above */

#define ASL_LOCAL_INITIALIZED   (1)
#define ASL_LOCAL_REFERENCED    (1<<1)
#define ASL_ARG_IS_LOCAL        (1<<2)
#define ASL_ARG_INITIALIZED     (1<<3)
#define ASL_ARG_REFERENCED      (1<<4)

/* Info used to track method counts for cross reference output file */

typedef struct asl_xref_info
{
    UINT32                  ThisMethodInvocations;
    UINT32                  TotalPredefinedMethods;
    UINT32                  TotalUserMethods;
    UINT32                  TotalUnreferenceUserMethods;
    UINT32                  ThisObjectReferences;
    UINT32                  TotalObjects;
    UINT32                  TotalUnreferencedObjects;
    ACPI_PARSE_OBJECT       *MethodOp;
    ACPI_PARSE_OBJECT       *CurrentMethodOp;

} ASL_XREF_INFO;


typedef struct asl_file_node
{
    FILE                    *File;
    UINT32                  CurrentLineNumber;
    void                    *State;
    char                    *Filename;
    struct asl_file_node    *Next;

} ASL_FILE_NODE;

typedef struct asl_files_node
{
    struct asl_file_info    Files[ASL_NUM_FILES];
    struct asl_files_node   *Next;
    char                    *TableSignature;
    char                    *TableId;
    UINT32                  TotalLineCount;
    UINT32                  OriginalInputFileSize;
    UINT32                  TotalKeywords;
    UINT32                  TotalFields;
    UINT32                  OutputByteLength;
    UINT32                  TotalNamedObjects;
    UINT32                  TotalExecutableOpcodes;
    BOOLEAN                 ParserErrorDetected;
    UINT8                   FileType;

} ASL_GLOBAL_FILE_NODE;

#endif  /* __ASLTYPES_H */
