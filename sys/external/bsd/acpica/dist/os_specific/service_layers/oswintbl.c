/******************************************************************************
 *
 * Module Name: oswintbl - Windows OSL for obtaining ACPI tables
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

#include "acpi.h"
#include "accommon.h"
#include "acutils.h"
#include <stdio.h>

#ifdef WIN32
#pragma warning(disable:4115)   /* warning C4115: (caused by rpcasync.h) */
#include <windows.h>

#elif WIN64
#include <windowsx.h>
#endif

#define _COMPONENT          ACPI_OS_SERVICES
        ACPI_MODULE_NAME    ("oswintbl")

/* Local prototypes */

static char *
WindowsFormatException (
    LONG                WinStatus);

/* Globals */

#define LOCAL_BUFFER_SIZE           64

static char             KeyBuffer[LOCAL_BUFFER_SIZE];
static char             ErrorBuffer[LOCAL_BUFFER_SIZE];

/*
 * List of table signatures reported by EnumSystemFirmwareTables ()
 */
UINT32                  *Gbl_AvailableTableSignatures;
UINT32                  Gbl_TableCount = 0;
UINT32                  Gbl_SsdtInstance = 0;

BOOLEAN                 Gbl_TableListInitialized = FALSE;

static ACPI_STATUS
OslTableInitialize (
    void);


/******************************************************************************
 *
 * FUNCTION:    WindowsFormatException
 *
 * PARAMETERS:  WinStatus       - Status from a Windows system call
 *
 * RETURN:      Formatted (ascii) exception code. Front-end to Windows
 *              FormatMessage interface.
 *
 * DESCRIPTION: Decode a windows exception
 *
 *****************************************************************************/

static char *
WindowsFormatException (
    LONG                WinStatus)
{

    ErrorBuffer[0] = 0;
    FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM, NULL, WinStatus, 0,
        ErrorBuffer, LOCAL_BUFFER_SIZE, NULL);

    return (ErrorBuffer);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsGetTableByAddress
 *
 * PARAMETERS:  Address         - Physical address of the ACPI table
 *              Table           - Where a pointer to the table is returned
 *
 * RETURN:      Status; Table buffer is returned if AE_OK.
 *              AE_NOT_FOUND: A valid table was not found at the address
 *
 * DESCRIPTION: Get an ACPI table via a physical memory address.
 *
 * NOTE:        Cannot be implemented without a Windows device driver.
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsGetTableByAddress (
    ACPI_PHYSICAL_ADDRESS   Address,
    ACPI_TABLE_HEADER       **Table)
{

    fprintf (stderr, "Get table by address is not supported on Windows\n");
    return (AE_SUPPORT);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsGetTableByIndex
 *
 * PARAMETERS:  Index           - Which table to get
 *              Table           - Where a pointer to the table is returned
 *              Instance        - Where a pointer to the table instance no. is
 *                                returned
 *              Address         - Where the table physical address is returned
 *
 * RETURN:      Status; Table buffer and physical address returned if AE_OK.
 *              AE_LIMIT: Index is beyond valid limit
 *
 * DESCRIPTION: Get an ACPI table via an index value (0 through n). Returns
 *              AE_LIMIT when an invalid index is reached. Index is not
 *              necessarily an index into the RSDT/XSDT.
 *              SSDT tables are obtained from the Windows registry. All other
 *              tables are obtained through GetSystemFirmwareTable ().
 *
 * NOTE:        Cannot get the physical address from the windows registry;
 *              zero is returned instead.
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsGetTableByIndex (
    UINT32                  Index,
    ACPI_TABLE_HEADER       **Table,
    UINT32                  *Instance,
    ACPI_PHYSICAL_ADDRESS   *Address)
{
    ACPI_STATUS             Status;
    char                    *Signature;
    UINT32                  CurrentInstance;


    /* Enumerate all ACPI table signatures on first invocation of this function */

    Status = OslTableInitialize ();
    if (ACPI_FAILURE (Status))
    {
        return (Status);
    }

    /* Validate Index */

    if (Index < Gbl_TableCount)
    {
        Signature = malloc (ACPI_NAMESEG_SIZE + 1);
        if (!Signature)
        {
            return (AE_NO_MEMORY);
        }

        Signature = memmove (Signature, &Gbl_AvailableTableSignatures[Index], ACPI_NAMESEG_SIZE);
    }
    else
    {
        return (AE_LIMIT);
    }

    if (ACPI_COMPARE_NAMESEG (Signature, ACPI_SIG_SSDT))
    {
        CurrentInstance = Gbl_SsdtInstance;
        Gbl_SsdtInstance++;
    }
    else
    {
        CurrentInstance = 0;
    }

    Status = AcpiOsGetTableByName (Signature, CurrentInstance, Table, Address);
    if (ACPI_SUCCESS (Status))
    {
        *Instance = CurrentInstance;
    }
    else if (Status == AE_NOT_FOUND &&
        ACPI_COMPARE_NAMESEG (Signature, ACPI_SIG_SSDT))
    {
        /* Treat SSDTs that are not found as invalid index. */
        Status = AE_LIMIT;
    }

    free (Signature);
    return (Status);
}

/******************************************************************************
 *
 * FUNCTION:    OslTableInitialize
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Initialize ACPI table data. Enumerate all ACPI table signatures
 *              and save them to a global list.
 *
 *****************************************************************************/
static ACPI_STATUS
OslTableInitialize (
    void)
{
    UINT32                  ResultSize;
    UINT32                  DataSize;

    if (Gbl_TableListInitialized)
    {
        return (AE_OK);
    }

    /*
     * ACPI table signatures are always 4 characters. Therefore, the data size
     * buffer should be a multiple of 4
     */
    DataSize = EnumSystemFirmwareTables ('ACPI', NULL, 0);
    if (DataSize % ACPI_NAMESEG_SIZE)
    {
        return (AE_ERROR);
    }

    /*
     * EnumSystemFirmwareTables () does not report the DSDT or XSDT. Work around this
     * by adding these entries manually.
     */
    Gbl_TableCount = 2 + DataSize / ACPI_NAMESEG_SIZE;
    Gbl_AvailableTableSignatures = malloc (Gbl_TableCount * ACPI_NAMESEG_SIZE);
    if (!Gbl_AvailableTableSignatures)
    {
        return (AE_NO_MEMORY);
    }

    ResultSize = EnumSystemFirmwareTables ('ACPI', Gbl_AvailableTableSignatures, DataSize);
    if (ResultSize > DataSize)
    {
        return (AE_ERROR);
    }

    /* Insert the DSDT and XSDT tables signatures */

    Gbl_AvailableTableSignatures [Gbl_TableCount - 1] = 'TDSD';
    Gbl_AvailableTableSignatures [Gbl_TableCount - 2] = 'TDSX';

    Gbl_TableListInitialized = TRUE;
    return (AE_OK);
}


/******************************************************************************
 *
 * FUNCTION:    WindowsGetTableFromRegistry
 *
 * PARAMETERS:  Signature       - ACPI Signature for desired table. Must be
 *                                a null terminated 4-character string.
 *              Instance        - For SSDTs (0...n). Use 0 otherwise.
 *              Table           - Where a pointer to the table is returned
 *              Address         - Where the table physical address is returned
 *
 * RETURN:      Status; Table buffer and physical address returned if AE_OK.
 *              AE_LIMIT: Instance is beyond valid limit
 *              AE_NOT_FOUND: A table with the signature was not found
 *
 * DESCRIPTION: Get an ACPI table via a table signature (4 ASCII characters).
 *              Returns AE_LIMIT when an invalid instance is reached.
 *              Table is obtained from the Windows registry.
 *
 * NOTE:        Assumes the input signature is uppercase.
 *              Cannot get the physical address from the windows registry;
 *              zero is returned instead.
 *
 *****************************************************************************/

static ACPI_STATUS
WindowsGetTableFromRegistry (
    char                    *Signature,
    UINT32                  Instance,
    ACPI_TABLE_HEADER       **Table,
    ACPI_PHYSICAL_ADDRESS   *Address)
{
    HKEY                    Handle = NULL;
    LONG                    WinStatus;
    ULONG                   Type;
    ULONG                   NameSize;
    ULONG                   DataSize;
    HKEY                    SubKey;
    ULONG                   i;
    ACPI_TABLE_HEADER       *ReturnTable;
    ACPI_STATUS             Status = AE_OK;


    /* Get a handle to the table key */

    while (1)
    {
        strcpy(KeyBuffer, "HARDWARE\\ACPI\\");
        if (AcpiUtSafeStrcat(KeyBuffer, sizeof(KeyBuffer), Signature))
        {
            return (AE_BUFFER_OVERFLOW);
        }

        /*
         * Windows stores SSDT at SSDT, SSD1, ..., SSD9, SSDA, ..., SSDS, SSDT,
         * SSDU, ..., SSDY. If the first (0th) and the 29th tables have the same
         * OEM ID, Table ID and Revision, then the 29th entry will overwrite the
         * first entry... Let's hope that we do not have that many entries.
         */
        if (Instance > 0 && ACPI_COMPARE_NAMESEG(Signature, ACPI_SIG_SSDT))
        {
            if (Instance < 10)
            {
                KeyBuffer[strlen(KeyBuffer) - 1] = '0' + (char)Instance;
            }
            else if (Instance < 29)
            {
                KeyBuffer[strlen(KeyBuffer) - 1] = 'A' + (char)(Instance - 10);
            }
            else
            {
                return (AE_LIMIT);
            }
        }

        WinStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE, KeyBuffer,
            0L, KEY_READ, &Handle);

        if (WinStatus != ERROR_SUCCESS)
        {
            /*
             * Somewhere along the way, MS changed the registry entry for
             * the FADT from
             * HARDWARE/ACPI/FACP  to
             * HARDWARE/ACPI/FADT.
             *
             * This code allows for both.
             */
            if (ACPI_COMPARE_NAMESEG(Signature, "FACP"))
            {
                Signature = "FADT";
            }
            else if (ACPI_COMPARE_NAMESEG(Signature, "XSDT"))
            {
                Signature = "RSDT";
            }
            else if (ACPI_COMPARE_NAMESEG(Signature, ACPI_SIG_SSDT))
            {
                /*
                 * SSDT may not be present on older Windows versions, but it is
                 * also possible that the index is not found.
                 */
                return (AE_NOT_FOUND);
            }
            else
            {
                fprintf(stderr,
                    "Could not find %s in registry at %s: %s (WinStatus=0x%X)\n",
                    Signature, KeyBuffer, WindowsFormatException(WinStatus), WinStatus);
                return (AE_NOT_FOUND);
            }
        }
        else
        {
            break;
        }
    }

    /* Actual data for the table is down a couple levels */

    for (i = 0; ;)
    {
        WinStatus = RegEnumKey(Handle, i, KeyBuffer, sizeof(KeyBuffer));
        i++;
        if (WinStatus == ERROR_NO_MORE_ITEMS)
        {
            break;
        }

        WinStatus = RegOpenKey(Handle, KeyBuffer, &SubKey);
        if (WinStatus != ERROR_SUCCESS)
        {
            fprintf(stderr, "Could not open %s entry: %s\n",
                Signature, WindowsFormatException(WinStatus));
            Status = AE_ERROR;
            goto Cleanup;
        }

        RegCloseKey(Handle);
        Handle = SubKey;
        i = 0;
    }

    /* Find the (binary) table entry */

    for (i = 0; ; i++)
    {
        NameSize = sizeof(KeyBuffer);
        WinStatus = RegEnumValue(Handle, i, KeyBuffer, &NameSize, NULL,
            &Type, NULL, 0);
        if (WinStatus != ERROR_SUCCESS)
        {
            fprintf(stderr, "Could not get %s registry entry: %s\n",
                Signature, WindowsFormatException(WinStatus));
            Status = AE_ERROR;
            goto Cleanup;
        }

        if (Type == REG_BINARY)
        {
            break;
        }
    }

    /* Get the size of the table */

    WinStatus = RegQueryValueEx(Handle, KeyBuffer, NULL, NULL,
        NULL, &DataSize);
    if (WinStatus != ERROR_SUCCESS)
    {
        fprintf(stderr, "Could not read the %s table size: %s\n",
            Signature, WindowsFormatException(WinStatus));
        Status = AE_ERROR;
        goto Cleanup;
    }

    /* Allocate a new buffer for the table */

    ReturnTable = malloc(DataSize);
    if (!ReturnTable)
    {
        Status = AE_NO_MEMORY;
        goto Cleanup;
    }

    /* Get the actual table from the registry */

    WinStatus = RegQueryValueEx(Handle, KeyBuffer, NULL, NULL,
        (UCHAR *)ReturnTable, &DataSize);

    if (WinStatus != ERROR_SUCCESS)
    {
        fprintf(stderr, "Could not read %s data: %s\n",
            Signature, WindowsFormatException(WinStatus));
        free(ReturnTable);
        Status = AE_ERROR;
        goto Cleanup;
    }

    *Table = ReturnTable;
    *Address = 0;

Cleanup:
    RegCloseKey(Handle);
    return (Status);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsGetTableByName
 *
 * PARAMETERS:  Signature       - ACPI Signature for desired table. Must be
 *                                a null terminated 4-character string.
 *              Instance        - For SSDTs (0...n). Use 0 otherwise.
 *              Table           - Where a pointer to the table is returned
 *              Address         - Where the table physical address is returned
 *
 * RETURN:      Status; Table buffer and physical address returned if AE_OK.
 *              AE_LIMIT: Instance is beyond valid limit
 *              AE_NOT_FOUND: A table with the signature was not found
 *
 * DESCRIPTION: Get an ACPI table via a table signature (4 ASCII characters).
 *              Returns AE_LIMIT when an invalid instance is reached.
 *              Table is obtained from the Windows registry.
 *
 * NOTE:        Assumes the input signature is uppercase.
 *              Cannot get the physical address from the windows registry;
 *              zero is returned instead.
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsGetTableByName(
    char                    *Signature,
    UINT32                  Instance,
    ACPI_TABLE_HEADER       **Table,
    ACPI_PHYSICAL_ADDRESS   *Address)
{
    LONG                    Result;
    ACPI_STATUS             Status = AE_OK;
    UINT32                  DataSize;
    ACPI_TABLE_HEADER       *ReturnTable;
    UINT32                  UIntSignature = 0;


    /* Multiple instances are only supported for SSDT tables. */

    if (Instance > 0 && !ACPI_COMPARE_NAMESEG (Signature, ACPI_SIG_SSDT))
    {
        return (AE_LIMIT);
    }

    if (ACPI_COMPARE_NAMESEG (Signature, ACPI_SIG_SSDT))
    {
        Status = WindowsGetTableFromRegistry ("SSDT", Instance, Table, Address);
        return (Status);
    }

    /* GetSystemFirmwareTable requires the table signature to be UINT32 */

    UIntSignature = *ACPI_CAST_PTR (UINT32, Signature);
    DataSize = GetSystemFirmwareTable('ACPI', UIntSignature, NULL, 0);
    if (!DataSize)
    {
        fprintf(stderr, "The table signature %s does not exist.", Signature);
        return (AE_ERROR);
    }

    ReturnTable = malloc(DataSize);
    if (!ReturnTable)
    {
        return (AE_NO_MEMORY);
    }

    Result = GetSystemFirmwareTable('ACPI', UIntSignature, ReturnTable, DataSize);
    if (Result > (LONG) DataSize)
    {
        /* Clean up */

        fprintf (stderr, "Could not read %s data\n", Signature);
        free (ReturnTable);
        return (AE_ERROR);
    }

    *Table = ReturnTable;
    return (Status);
}


/* These are here for acpidump only, so we don't need to link oswinxf */

#ifdef ACPI_DUMP_APP
/******************************************************************************
 *
 * FUNCTION:    AcpiOsMapMemory
 *
 * PARAMETERS:  Where               - Physical address of memory to be mapped
 *              Length              - How much memory to map
 *
 * RETURN:      Pointer to mapped memory. Null on error.
 *
 * DESCRIPTION: Map physical memory into caller's address space
 *
 *****************************************************************************/

void *
AcpiOsMapMemory (
    ACPI_PHYSICAL_ADDRESS   Where,
    ACPI_SIZE               Length)
{

    return (ACPI_TO_POINTER ((ACPI_SIZE) Where));
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsUnmapMemory
 *
 * PARAMETERS:  Where               - Logical address of memory to be unmapped
 *              Length              - How much memory to unmap
 *
 * RETURN:      None.
 *
 * DESCRIPTION: Delete a previously created mapping. Where and Length must
 *              correspond to a previous mapping exactly.
 *
 *****************************************************************************/

void
AcpiOsUnmapMemory (
    void                    *Where,
    ACPI_SIZE               Length)
{

    return;
}
#endif
