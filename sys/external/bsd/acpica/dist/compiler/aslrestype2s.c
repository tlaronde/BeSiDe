/******************************************************************************
 *
 * Module Name: aslrestype2s - Serial Large resource descriptors
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
#include "aslcompiler.y.h"
#include "amlcode.h"

#define _COMPONENT          ACPI_COMPILER
        ACPI_MODULE_NAME    ("aslrestype2s")


static UINT16
RsGetBufferDataLength (
    ACPI_PARSE_OBJECT       *InitializerOp);

static UINT16
RsGetInterruptDataLength (
    ACPI_PARSE_OBJECT       *InitializerOp,
    UINT32                  StartIndex);

static BOOLEAN
RsGetVendorData (
    ACPI_PARSE_OBJECT       *InitializerOp,
    UINT8                   *VendorData,
    ACPI_SIZE               DescriptorOffset);

static UINT16
RsGetStringDataLengthAt (
    ACPI_PARSE_OBJECT       *InitializerOp,
    UINT32                  StartIndex);

/*
 * This module contains descriptors for serial buses and GPIO:
 *
 * GpioInt
 * GpioIo
 * I2cSerialBus
 * SpiSerialBus
 * UartSerialBus
 * PinFunction
 * PinConfig
 * PinGroup
 * PinGroupFunction
 * PinGroupConfig
 */


/*******************************************************************************
 *
 * FUNCTION:    RsGetBufferDataLength
 *
 * PARAMETERS:  InitializerOp       - Current parse op, start of the resource
 *                                    descriptor
 *
 * RETURN:      Length of the data buffer
 *
 * DESCRIPTION: Get the length of a RawDataBuffer, used for vendor data.
 *
 ******************************************************************************/

static UINT16
RsGetBufferDataLength (
    ACPI_PARSE_OBJECT       *InitializerOp)
{
    UINT16                  ExtraDataSize = 0;
    ACPI_PARSE_OBJECT       *DataList;


    /* Find the byte-initializer list */

    while (InitializerOp)
    {
        if (InitializerOp->Asl.ParseOpcode == PARSEOP_DATABUFFER)
        {
            /* First child is the optional length (ignore it here) */

            DataList = InitializerOp->Asl.Child;
            DataList = ASL_GET_PEER_NODE (DataList);

            /* Count the data items (each one is a byte of data) */

            while (DataList)
            {
                ExtraDataSize++;
                DataList = ASL_GET_PEER_NODE (DataList);
            }

            return (ExtraDataSize);
        }

        InitializerOp = ASL_GET_PEER_NODE (InitializerOp);
    }

    return (ExtraDataSize);
}


/*******************************************************************************
 *
 * FUNCTION:    RsGetInterruptDataLength
 *
 * PARAMETERS:  InitializerOp       - Current parse op, start of the resource
 *                                    descriptor
 *              StartIndex          - Start index of interrupt/pin list
 *
 * RETURN:      Length of the interrupt data list
 *
 * DESCRIPTION: Get the length of a list of interrupt DWORDs for the GPIO
 *              descriptors.
 *
 ******************************************************************************/

static UINT16
RsGetInterruptDataLength (
    ACPI_PARSE_OBJECT       *InitializerOp,
    UINT32                  StartIndex)
{
    UINT16                  InterruptLength;
    UINT32                  i;


    /* Count the interrupt numbers */

    InterruptLength = 0;
    for (i = 0; InitializerOp; i++)
    {
        InitializerOp = ASL_GET_PEER_NODE (InitializerOp);

        /* Interrupt list starts at offset StartIndex (Gpio descriptors) */

        if (i >= StartIndex)
        {
            InterruptLength += 2;
        }
    }

    return (InterruptLength);
}


/*******************************************************************************
 *
 * FUNCTION:    RsGetVendorData
 *
 * PARAMETERS:  InitializerOp       - Current parse op, start of the resource
 *                                    descriptor.
 *              VendorData          - Where the vendor data is returned
 *              DescriptorOffset    - Where vendor data begins in descriptor
 *
 * RETURN:      TRUE if valid vendor data was returned, FALSE otherwise.
 *
 * DESCRIPTION: Extract the vendor data and construct a vendor data buffer.
 *
 ******************************************************************************/

static BOOLEAN
RsGetVendorData (
    ACPI_PARSE_OBJECT       *InitializerOp,
    UINT8                   *VendorData,
    ACPI_SIZE               DescriptorOffset)
{
    ACPI_PARSE_OBJECT       *BufferOp;
    UINT32                  SpecifiedLength = ACPI_UINT32_MAX;
    UINT16                  ActualLength = 0;


    /* Vendor Data field is always optional */

    if (InitializerOp->Asl.ParseOpcode == PARSEOP_DEFAULT_ARG)
    {
        return (FALSE);
    }

    BufferOp = InitializerOp->Asl.Child;
    if (!BufferOp)
    {
        AslError (ASL_ERROR, ASL_MSG_SYNTAX, InitializerOp, "");
        return (FALSE);
    }

    /* First child is the optional buffer length (WORD) */

    if (BufferOp->Asl.ParseOpcode != PARSEOP_DEFAULT_ARG)
    {
        SpecifiedLength = (UINT16) BufferOp->Asl.Value.Integer;
    }

    /* Insert field tag _VEN */

    RsCreateByteField (InitializerOp, ACPI_RESTAG_VENDORDATA,
        (UINT16) DescriptorOffset);

    /* Walk the list of buffer initializers (each is one byte) */

    BufferOp = RsCompleteNodeAndGetNext (BufferOp);
    if (BufferOp->Asl.ParseOpcode != PARSEOP_DEFAULT_ARG)
    {
        while (BufferOp)
        {
            *VendorData = (UINT8) BufferOp->Asl.Value.Integer;
            VendorData++;
            ActualLength++;
            BufferOp = RsCompleteNodeAndGetNext (BufferOp);
        }
    }

    /* Length validation. Buffer cannot be of zero length */

    if ((SpecifiedLength == 0) ||
        ((SpecifiedLength == ACPI_UINT32_MAX) && (ActualLength == 0)))
    {
        AslError (ASL_ERROR, ASL_MSG_BUFFER_LENGTH, InitializerOp, NULL);
        return (FALSE);
    }

    if (SpecifiedLength != ACPI_UINT32_MAX)
    {
        /* ActualLength > SpecifiedLength -> error */

        if (ActualLength > SpecifiedLength)
        {
            AslError (ASL_ERROR, ASL_MSG_LIST_LENGTH_LONG, InitializerOp, NULL);
            return (FALSE);
        }

        /* ActualLength < SpecifiedLength -> remark */

        else if (ActualLength < SpecifiedLength)
        {
            AslError (ASL_REMARK, ASL_MSG_LIST_LENGTH_SHORT, InitializerOp, NULL);
            return (FALSE);
        }
    }

    return (TRUE);
}


/*******************************************************************************
 *
 * FUNCTION:    RsGetStringDataLengthAt
 *
 * PARAMETERS:  InitializerOp     - Start of a subtree of init nodes
 *              StartIndex        - Starting index of the string node
 *
 * RETURN:      Valid string length if a string node is found at given
 *               StartIndex or 0 otherwise.
 *
 * DESCRIPTION: In a list of peer nodes, find the first one at given index
 *              that contains a string and return length.
 *
 ******************************************************************************/

static UINT16
RsGetStringDataLengthAt (
    ACPI_PARSE_OBJECT       *InitializerOp,
    UINT32                  StartIndex)
{
    UINT32 i;

    for (i = 0; InitializerOp; i++)
    {
        if (i == StartIndex &&
            InitializerOp->Asl.ParseOpcode == PARSEOP_STRING_LITERAL)
        {
            return ((UINT16) (strlen (InitializerOp->Asl.Value.String) + 1));
        }

        InitializerOp = ASL_GET_PEER_NODE (InitializerOp);
    }

    return (0);
}


/*******************************************************************************
 *
 * FUNCTION:    RsDoGpioIntDescriptor
 *
 * PARAMETERS:  Info                - Parse Op and resource template offset
 *
 * RETURN:      Completed resource node
 *
 * DESCRIPTION: Construct a long "GpioInt" descriptor
 *
 ******************************************************************************/

ASL_RESOURCE_NODE *
RsDoGpioIntDescriptor (
    ASL_RESOURCE_INFO       *Info)
{
    AML_RESOURCE            *Descriptor;
    ACPI_PARSE_OBJECT       *InitializerOp;
    ASL_RESOURCE_NODE       *Rnode;
    char                    *ResourceSource = NULL;
    UINT8                   *VendorData = NULL;
    UINT16                  *InterruptList = NULL;
    UINT16                  *PinList = NULL;
    UINT16                  ResSourceLength;
    UINT16                  VendorLength;
    UINT16                  InterruptLength;
    UINT16                  DescriptorSize;
    UINT32                  CurrentByteOffset;
    UINT32                  PinCount = 0;
    UINT32                  i;


    InitializerOp = Info->DescriptorTypeOp->Asl.Child;
    CurrentByteOffset = Info->CurrentByteOffset;

    /*
     * Calculate lengths for fields that have variable length:
     * 1) Resource Source string
     * 2) Vendor Data buffer
     * 3) PIN (interrupt) list
     */
    ResSourceLength = RsGetStringDataLength (InitializerOp);
    VendorLength = RsGetBufferDataLength (InitializerOp);
    InterruptLength = RsGetInterruptDataLength (InitializerOp, 10);

    DescriptorSize = ACPI_AML_SIZE_LARGE (AML_RESOURCE_GPIO) +
        ResSourceLength + VendorLength + InterruptLength;

    /* Allocate the local resource node and initialize */

    Rnode = RsAllocateResourceNode (DescriptorSize +
        sizeof (AML_RESOURCE_LARGE_HEADER));

    Descriptor = Rnode->Buffer;
    Descriptor->Gpio.ResourceLength = DescriptorSize;
    Descriptor->Gpio.DescriptorType = ACPI_RESOURCE_NAME_GPIO;
    Descriptor->Gpio.RevisionId = AML_RESOURCE_GPIO_REVISION;
    Descriptor->Gpio.ConnectionType = AML_RESOURCE_GPIO_TYPE_INT;

    /* Build pointers to optional areas */

    InterruptList = ACPI_ADD_PTR (UINT16, Descriptor,
        sizeof (AML_RESOURCE_GPIO));
    PinList = InterruptList;
    ResourceSource = ACPI_ADD_PTR (char, InterruptList, InterruptLength);
    VendorData = ACPI_ADD_PTR (UINT8, ResourceSource, ResSourceLength);

    /* Setup offsets within the descriptor */

    Descriptor->Gpio.PinTableOffset = (UINT16)
        ACPI_PTR_DIFF (InterruptList, Descriptor);

    Descriptor->Gpio.ResSourceOffset = (UINT16)
        ACPI_PTR_DIFF (ResourceSource, Descriptor);

    /* Process all child initialization nodes */

    for (i = 0; InitializerOp; i++)
    {
        switch (i)
        {
        case 0: /* Interrupt Mode - edge/level [Flag] (_MOD) */

            RsSetFlagBits16 (&Descriptor->Gpio.IntFlags, InitializerOp, 0, 0);
            RsCreateBitField (InitializerOp, ACPI_RESTAG_MODE,
                CurrentByteOffset + ASL_RESDESC_OFFSET (Gpio.IntFlags), 0);
            break;

        case 1: /* Interrupt Polarity - Active high/low [Flags] (_POL) */

            RsSetFlagBits16 (&Descriptor->Gpio.IntFlags, InitializerOp, 1, 0);
            RsCreateMultiBitField (InitializerOp, ACPI_RESTAG_POLARITY,
                CurrentByteOffset + ASL_RESDESC_OFFSET (Gpio.IntFlags), 1, 2);
            break;

        case 2: /* Share Type - Default: exclusive (0) [Flags] (_SHR) */

            RsSetFlagBits16 (&Descriptor->Gpio.IntFlags, InitializerOp, 3, 0);
            RsCreateMultiBitField (InitializerOp, ACPI_RESTAG_INTERRUPTSHARE,
                CurrentByteOffset + ASL_RESDESC_OFFSET (Gpio.IntFlags), 3, 2);
            break;

        case 3: /* Pin Config [BYTE] (_PPI) */

            Descriptor->Gpio.PinConfig = (UINT8) InitializerOp->Asl.Value.Integer;
            RsCreateByteField (InitializerOp, ACPI_RESTAG_PINCONFIG,
                CurrentByteOffset + ASL_RESDESC_OFFSET (Gpio.PinConfig));
            break;

        case 4: /* Debounce Timeout [WORD] (_DBT) */

            Descriptor->Gpio.DebounceTimeout = (UINT16) InitializerOp->Asl.Value.Integer;
            RsCreateWordField (InitializerOp, ACPI_RESTAG_DEBOUNCETIME,
                CurrentByteOffset + ASL_RESDESC_OFFSET (Gpio.DebounceTimeout));
            break;

        case 5: /* ResSource [Optional Field - STRING] */

            if (ResSourceLength)
            {
                /* Copy string to the descriptor */

                strcpy (ResourceSource,
                    InitializerOp->Asl.Value.String);
            }
            break;

        case 6: /* Resource Index */

            if (InitializerOp->Asl.ParseOpcode != PARSEOP_DEFAULT_ARG)
            {
                Descriptor->Gpio.ResSourceIndex =
                    (UINT8) InitializerOp->Asl.Value.Integer;
            }
            break;

        case 7: /* Resource Usage (consumer/producer) */

            RsSetFlagBits16 (&Descriptor->Gpio.Flags, InitializerOp, 0, 1);
            break;

        case 8: /* Resource Tag (Descriptor Name) */

            UtAttachNamepathToOwner (Info->DescriptorTypeOp, InitializerOp);
            break;

        case 9: /* Vendor Data (Optional - Buffer of BYTEs) (_VEN) */

            /*
             * Always set the VendorOffset even if there is no Vendor Data.
             * This field is required in order to calculate the length
             * of the ResourceSource at runtime.
             */
            Descriptor->Gpio.VendorOffset = (UINT16)
                ACPI_PTR_DIFF (VendorData, Descriptor);

            if (RsGetVendorData (InitializerOp, VendorData,
                (CurrentByteOffset +  Descriptor->Gpio.VendorOffset)))
            {
                Descriptor->Gpio.VendorLength = VendorLength;
            }
            break;

        default:
            /*
             * PINs come through here, repeatedly. Each PIN must be a WORD.
             * NOTE: there is no "length" field for this, so from ACPI spec:
             *  The number of pins in the table can be calculated from:
             *  PinCount = (Resource Source Name Offset - Pin Table Offset) / 2
             *  (implies resource source must immediately follow the pin list.)
             *  Name: _PIN
             */
            *InterruptList = (UINT16) InitializerOp->Asl.Value.Integer;
            InterruptList++;
            PinCount++;

            /* Case 10: First interrupt number in list */

            if (i == 10)
            {
                if (InitializerOp->Asl.ParseOpcode == PARSEOP_DEFAULT_ARG)
                {
                    /* Must be at least one interrupt */

                    AslError (ASL_ERROR, ASL_MSG_EX_INTERRUPT_LIST_MIN,
                        InitializerOp, NULL);
                }

                /* Check now for duplicates in list */

                RsCheckListForDuplicates (InitializerOp);

                /* Create a named field at the start of the list */

                RsCreateWordField (InitializerOp, ACPI_RESTAG_PIN,
                    CurrentByteOffset + Descriptor->Gpio.PinTableOffset);
            }
            break;
        }

        InitializerOp = RsCompleteNodeAndGetNext (InitializerOp);
    }

    MpSaveGpioInfo (Info->MappingOp, Descriptor,
        PinCount, PinList, ResourceSource);
    return (Rnode);
}


/*******************************************************************************
 *
 * FUNCTION:    RsDoGpioIoDescriptor
 *
 * PARAMETERS:  Info                - Parse Op and resource template offset
 *
 * RETURN:      Completed resource node
 *
 * DESCRIPTION: Construct a long "GpioIo" descriptor
 *
 ******************************************************************************/

ASL_RESOURCE_NODE *
RsDoGpioIoDescriptor (
    ASL_RESOURCE_INFO       *Info)
{
    AML_RESOURCE            *Descriptor;
    ACPI_PARSE_OBJECT       *InitializerOp;
    ASL_RESOURCE_NODE       *Rnode;
    char                    *ResourceSource = NULL;
    UINT8                   *VendorData = NULL;
    UINT16                  *InterruptList = NULL;
    UINT16                  *PinList = NULL;
    UINT16                  ResSourceLength;
    UINT16                  VendorLength;
    UINT16                  InterruptLength;
    UINT16                  DescriptorSize;
    UINT32                  CurrentByteOffset;
    UINT32                  PinCount = 0;
    UINT32                  i;


    InitializerOp = Info->DescriptorTypeOp->Asl.Child;
    CurrentByteOffset = Info->CurrentByteOffset;

    /*
     * Calculate lengths for fields that have variable length:
     * 1) Resource Source string
     * 2) Vendor Data buffer
     * 3) PIN (interrupt) list
     */
    ResSourceLength = RsGetStringDataLength (InitializerOp);
    VendorLength = RsGetBufferDataLength (InitializerOp);
    InterruptLength = RsGetInterruptDataLength (InitializerOp, 10);

    DescriptorSize = ACPI_AML_SIZE_LARGE (AML_RESOURCE_GPIO) +
        ResSourceLength + VendorLength + InterruptLength;

    /* Allocate the local resource node and initialize */

    Rnode = RsAllocateResourceNode (DescriptorSize +
        sizeof (AML_RESOURCE_LARGE_HEADER));

    Descriptor = Rnode->Buffer;
    Descriptor->Gpio.ResourceLength = DescriptorSize;
    Descriptor->Gpio.DescriptorType = ACPI_RESOURCE_NAME_GPIO;
    Descriptor->Gpio.RevisionId = AML_RESOURCE_GPIO_REVISION;
    Descriptor->Gpio.ConnectionType = AML_RESOURCE_GPIO_TYPE_IO;

    /* Build pointers to optional areas */

    InterruptList = ACPI_ADD_PTR (UINT16, Descriptor, sizeof (AML_RESOURCE_GPIO));
    PinList = InterruptList;
    ResourceSource = ACPI_ADD_PTR (char, InterruptList, InterruptLength);
    VendorData = ACPI_ADD_PTR (UINT8, ResourceSource, ResSourceLength);

    /* Setup offsets within the descriptor */

    Descriptor->Gpio.PinTableOffset = (UINT16)
        ACPI_PTR_DIFF (InterruptList, Descriptor);

    Descriptor->Gpio.ResSourceOffset = (UINT16)
        ACPI_PTR_DIFF (ResourceSource, Descriptor);

    /* Process all child initialization nodes */

    for (i = 0; InitializerOp; i++)
    {
        switch (i)
        {
        case 0: /* Share Type [Flags] (_SHR) */

            RsSetFlagBits16 (&Descriptor->Gpio.IntFlags, InitializerOp, 3, 0);
            RsCreateBitField (InitializerOp, ACPI_RESTAG_INTERRUPTSHARE,
                CurrentByteOffset + ASL_RESDESC_OFFSET (Gpio.IntFlags), 3);
            break;

        case 1: /* Pin Config [BYTE] (_PPI) */

            Descriptor->Gpio.PinConfig = (UINT8) InitializerOp->Asl.Value.Integer;
            RsCreateByteField (InitializerOp, ACPI_RESTAG_PINCONFIG,
                CurrentByteOffset + ASL_RESDESC_OFFSET (Gpio.PinConfig));
            break;

        case 2: /* Debounce Timeout [WORD] (_DBT) */

            Descriptor->Gpio.DebounceTimeout = (UINT16) InitializerOp->Asl.Value.Integer;
            RsCreateWordField (InitializerOp, ACPI_RESTAG_DEBOUNCETIME,
                CurrentByteOffset + ASL_RESDESC_OFFSET (Gpio.DebounceTimeout));
            break;

        case 3: /* Drive Strength [WORD] (_DRS) */

            Descriptor->Gpio.DriveStrength = (UINT16) InitializerOp->Asl.Value.Integer;
            RsCreateWordField (InitializerOp, ACPI_RESTAG_DRIVESTRENGTH,
                CurrentByteOffset + ASL_RESDESC_OFFSET (Gpio.DriveStrength));
            break;

        case 4: /* I/O Restriction [Flag] (_IOR) */

            RsSetFlagBits16 (&Descriptor->Gpio.IntFlags, InitializerOp, 0, 0);
            RsCreateMultiBitField (InitializerOp, ACPI_RESTAG_IORESTRICTION,
                CurrentByteOffset + ASL_RESDESC_OFFSET (Gpio.IntFlags), 0, 2);
            break;

        case 5: /* ResSource [Optional Field - STRING] */

            if (ResSourceLength)
            {
                /* Copy string to the descriptor */

                strcpy (ResourceSource,
                    InitializerOp->Asl.Value.String);
            }
            break;

        case 6: /* Resource Index */

            if (InitializerOp->Asl.ParseOpcode != PARSEOP_DEFAULT_ARG)
            {
                Descriptor->Gpio.ResSourceIndex = (UINT8) InitializerOp->Asl.Value.Integer;
            }
            break;

        case 7: /* Resource Usage (consumer/producer) */

            RsSetFlagBits16 (&Descriptor->Gpio.Flags, InitializerOp, 0, 1);
            break;

        case 8: /* Resource Tag (Descriptor Name) */

            UtAttachNamepathToOwner (Info->DescriptorTypeOp, InitializerOp);
            break;

        case 9: /* Vendor Data (Optional - Buffer of BYTEs) (_VEN) */
            /*
             * Always set the VendorOffset even if there is no Vendor Data.
             * This field is required in order to calculate the length
             * of the ResourceSource at runtime.
             */
            Descriptor->Gpio.VendorOffset = (UINT16)
                ACPI_PTR_DIFF (VendorData, Descriptor);

            if (RsGetVendorData (InitializerOp, VendorData,
                (CurrentByteOffset + Descriptor->Gpio.VendorOffset)))
            {
                Descriptor->Gpio.VendorLength = VendorLength;
            }
            break;

        default:
            /*
             * PINs come through here, repeatedly. Each PIN must be a WORD.
             * NOTE: there is no "length" field for this, so from ACPI spec:
             *  The number of pins in the table can be calculated from:
             *  PinCount = (Resource Source Name Offset - Pin Table Offset) / 2
             *  (implies resource source must immediately follow the pin list.)
             *  Name: _PIN
             */
            *InterruptList = (UINT16) InitializerOp->Asl.Value.Integer;
            InterruptList++;
            PinCount++;

            /* Case 10: First interrupt number in list */

            if (i == 10)
            {
                if (InitializerOp->Asl.ParseOpcode == PARSEOP_DEFAULT_ARG)
                {
                    /* Must be at least one interrupt */

                    AslError (ASL_ERROR, ASL_MSG_EX_INTERRUPT_LIST_MIN,
                        InitializerOp, NULL);
                }

                /* Check now for duplicates in list */

                RsCheckListForDuplicates (InitializerOp);

                /* Create a named field at the start of the list */

                RsCreateWordField (InitializerOp, ACPI_RESTAG_PIN,
                    CurrentByteOffset + Descriptor->Gpio.PinTableOffset);
            }
            break;
        }

        InitializerOp = RsCompleteNodeAndGetNext (InitializerOp);
    }

    MpSaveGpioInfo (Info->MappingOp, Descriptor,
        PinCount, PinList, ResourceSource);
    return (Rnode);
}


/*******************************************************************************
 *
 * FUNCTION:    RsDoI2cSerialBusDescriptor
 *
 * PARAMETERS:  Info                - Parse Op and resource template offset
 *
 * RETURN:      Completed resource node
 *
 * DESCRIPTION: Construct a long "I2cSerialBus" descriptor
 *
 ******************************************************************************/

ASL_RESOURCE_NODE *
RsDoI2cSerialBusDescriptor (
    ASL_RESOURCE_INFO       *Info)
{
    AML_RESOURCE            *Descriptor;
    ACPI_PARSE_OBJECT       *InitializerOp;
    ASL_RESOURCE_NODE       *Rnode;
    char                    *ResourceSource = NULL;
    UINT8                   *VendorData = NULL;
    UINT16                  ResSourceLength;
    UINT16                  VendorLength;
    UINT16                  DescriptorSize;
    UINT32                  CurrentByteOffset;
    UINT32                  i;


    InitializerOp = Info->DescriptorTypeOp->Asl.Child;
    CurrentByteOffset = Info->CurrentByteOffset;

    /*
     * Calculate lengths for fields that have variable length:
     * 1) Resource Source string
     * 2) Vendor Data buffer
     */
    ResSourceLength = RsGetStringDataLength (InitializerOp);
    VendorLength = RsGetBufferDataLength (InitializerOp);

    DescriptorSize = ACPI_AML_SIZE_LARGE (AML_RESOURCE_I2C_SERIALBUS) +
        ResSourceLength + VendorLength;

    /* Allocate the local resource node and initialize */

    Rnode = RsAllocateResourceNode (DescriptorSize +
        sizeof (AML_RESOURCE_LARGE_HEADER));

    Descriptor = Rnode->Buffer;
    Descriptor->I2cSerialBus.ResourceLength = DescriptorSize;
    Descriptor->I2cSerialBus.DescriptorType = ACPI_RESOURCE_NAME_SERIAL_BUS;
    Descriptor->I2cSerialBus.RevisionId = AML_RESOURCE_I2C_REVISION;
    Descriptor->I2cSerialBus.TypeRevisionId = AML_RESOURCE_I2C_TYPE_REVISION;
    Descriptor->I2cSerialBus.Type = AML_RESOURCE_I2C_SERIALBUSTYPE;
    Descriptor->I2cSerialBus.TypeDataLength = AML_RESOURCE_I2C_MIN_DATA_LEN + VendorLength;

    if (Info->DescriptorTypeOp->Asl.ParseOpcode == PARSEOP_I2C_SERIALBUS_V2)
    {
        Descriptor->I2cSerialBus.RevisionId = 2;
    }

    /* Build pointers to optional areas */

    VendorData = ACPI_ADD_PTR (UINT8, Descriptor, sizeof (AML_RESOURCE_I2C_SERIALBUS));
    ResourceSource = ACPI_ADD_PTR (char, VendorData, VendorLength);

    /* Process all child initialization nodes */

    for (i = 0; InitializerOp; i++)
    {
        switch (i)
        {
        case 0: /* Slave Address [WORD] (_ADR) */

            Descriptor->I2cSerialBus.SlaveAddress = (UINT16) InitializerOp->Asl.Value.Integer;
            RsCreateWordField (InitializerOp, ACPI_RESTAG_ADDRESS,
                CurrentByteOffset + ASL_RESDESC_OFFSET (I2cSerialBus.SlaveAddress));
            break;

        case 1: /* Slave Mode [Flag] (_SLV) */

            RsSetFlagBits (&Descriptor->I2cSerialBus.Flags, InitializerOp, 0, 0);
            RsCreateBitField (InitializerOp, ACPI_RESTAG_SLAVEMODE,
                CurrentByteOffset + ASL_RESDESC_OFFSET (I2cSerialBus.Flags), 0);
            break;

        case 2: /* Connection Speed [DWORD] (_SPE) */

            Descriptor->I2cSerialBus.ConnectionSpeed = (UINT32) InitializerOp->Asl.Value.Integer;
            RsCreateDwordField (InitializerOp, ACPI_RESTAG_SPEED,
                CurrentByteOffset + ASL_RESDESC_OFFSET (I2cSerialBus.ConnectionSpeed));
            break;

        case 3: /* Addressing Mode [Flag] (_MOD) */

            RsSetFlagBits16 (&Descriptor->I2cSerialBus.TypeSpecificFlags, InitializerOp, 0, 0);
            RsCreateBitField (InitializerOp, ACPI_RESTAG_MODE,
                CurrentByteOffset + ASL_RESDESC_OFFSET (I2cSerialBus.TypeSpecificFlags), 0);
            break;

        case 4: /* ResSource [Optional Field - STRING] */

            if (ResSourceLength)
            {
                /* Copy string to the descriptor */

                strcpy (ResourceSource,
                    InitializerOp->Asl.Value.String);
            }
            break;

        case 5: /* Resource Index */

            if (InitializerOp->Asl.ParseOpcode != PARSEOP_DEFAULT_ARG)
            {
                Descriptor->I2cSerialBus.ResSourceIndex =
                    (UINT8) InitializerOp->Asl.Value.Integer;
            }
            break;

        case 6: /* Resource Usage (consumer/producer) */

            RsSetFlagBits (&Descriptor->I2cSerialBus.Flags, InitializerOp, 1, 1);
            break;

        case 7: /* Resource Tag (Descriptor Name) */

            UtAttachNamepathToOwner (Info->DescriptorTypeOp, InitializerOp);
            break;

        case 8:
            /*
             * Connection Share - Added for V2 (ACPI 6.0) version of the descriptor
             * Note: For V1, the share bit will be zero (Op is DEFAULT_ARG from
             * the ASL parser)
             */
            RsSetFlagBits (&Descriptor->I2cSerialBus.Flags, InitializerOp, 2, 0);
            RsCreateBitField (InitializerOp, ACPI_RESTAG_INTERRUPTSHARE,
                CurrentByteOffset + ASL_RESDESC_OFFSET (I2cSerialBus.Flags), 2);
            break;

        case 9: /* Vendor Data (Optional - Buffer of BYTEs) (_VEN) */

            RsGetVendorData (InitializerOp, VendorData,
                CurrentByteOffset + sizeof (AML_RESOURCE_I2C_SERIALBUS));
            break;

        default:    /* Ignore any extra nodes */

            break;
        }

        InitializerOp = RsCompleteNodeAndGetNext (InitializerOp);
    }

    MpSaveSerialInfo (Info->MappingOp, Descriptor, ResourceSource);
    return (Rnode);
}


/*******************************************************************************
 *
 * FUNCTION:    RsDoSpiSerialBusDescriptor
 *
 * PARAMETERS:  Info                - Parse Op and resource template offset
 *
 * RETURN:      Completed resource node
 *
 * DESCRIPTION: Construct a long "SPI Serial Bus" descriptor
 *
 ******************************************************************************/

ASL_RESOURCE_NODE *
RsDoSpiSerialBusDescriptor (
    ASL_RESOURCE_INFO       *Info)
{
    AML_RESOURCE            *Descriptor;
    ACPI_PARSE_OBJECT       *InitializerOp;
    ASL_RESOURCE_NODE       *Rnode;
    char                    *ResourceSource = NULL;
    UINT8                   *VendorData = NULL;
    UINT16                  ResSourceLength;
    UINT16                  VendorLength;
    UINT16                  DescriptorSize;
    UINT32                  CurrentByteOffset;
    UINT32                  i;


    InitializerOp = Info->DescriptorTypeOp->Asl.Child;
    CurrentByteOffset = Info->CurrentByteOffset;

    /*
     * Calculate lengths for fields that have variable length:
     * 1) Resource Source string
     * 2) Vendor Data buffer
     */
    ResSourceLength = RsGetStringDataLength (InitializerOp);
    VendorLength = RsGetBufferDataLength (InitializerOp);

    DescriptorSize = ACPI_AML_SIZE_LARGE (AML_RESOURCE_SPI_SERIALBUS) +
        ResSourceLength + VendorLength;

    /* Allocate the local resource node and initialize */

    Rnode = RsAllocateResourceNode (DescriptorSize +
        sizeof (AML_RESOURCE_LARGE_HEADER));

    Descriptor = Rnode->Buffer;
    Descriptor->SpiSerialBus.ResourceLength = DescriptorSize;
    Descriptor->SpiSerialBus.DescriptorType = ACPI_RESOURCE_NAME_SERIAL_BUS;
    Descriptor->SpiSerialBus.RevisionId = AML_RESOURCE_SPI_REVISION;
    Descriptor->SpiSerialBus.TypeRevisionId = AML_RESOURCE_SPI_TYPE_REVISION;
    Descriptor->SpiSerialBus.Type = AML_RESOURCE_SPI_SERIALBUSTYPE;
    Descriptor->SpiSerialBus.TypeDataLength = AML_RESOURCE_SPI_MIN_DATA_LEN + VendorLength;

    if (Info->DescriptorTypeOp->Asl.ParseOpcode == PARSEOP_SPI_SERIALBUS_V2)
    {
        Descriptor->I2cSerialBus.RevisionId = 2;
    }

    /* Build pointers to optional areas */

    VendorData = ACPI_ADD_PTR (UINT8, Descriptor,
        sizeof (AML_RESOURCE_SPI_SERIALBUS));
    ResourceSource = ACPI_ADD_PTR (char, VendorData, VendorLength);

    /* Process all child initialization nodes */

    for (i = 0; InitializerOp; i++)
    {
        switch (i)
        {
        case 0: /* Device Selection [WORD] (_ADR) */

            Descriptor->SpiSerialBus.DeviceSelection = (UINT16) InitializerOp->Asl.Value.Integer;
            RsCreateWordField (InitializerOp, ACPI_RESTAG_ADDRESS,
                CurrentByteOffset + ASL_RESDESC_OFFSET (SpiSerialBus.DeviceSelection));
            break;

        case 1: /* Device Polarity [Flag] (_DPL) */

            RsSetFlagBits16 (&Descriptor->SpiSerialBus.TypeSpecificFlags, InitializerOp, 1, 0);
            RsCreateBitField (InitializerOp, ACPI_RESTAG_DEVICEPOLARITY,
                CurrentByteOffset + ASL_RESDESC_OFFSET (SpiSerialBus.TypeSpecificFlags), 1);
            break;

        case 2: /* Wire Mode [Flag] (_MOD) */

            RsSetFlagBits16 (&Descriptor->SpiSerialBus.TypeSpecificFlags, InitializerOp, 0, 0);
            RsCreateBitField (InitializerOp, ACPI_RESTAG_MODE,
                CurrentByteOffset + ASL_RESDESC_OFFSET (SpiSerialBus.TypeSpecificFlags), 0);
            break;

        case 3: /* Device Bit Length [BYTE] (_LEN) */

            Descriptor->SpiSerialBus.DataBitLength = (UINT8) InitializerOp->Asl.Value.Integer;
            RsCreateByteField (InitializerOp, ACPI_RESTAG_LENGTH,
                CurrentByteOffset + ASL_RESDESC_OFFSET (SpiSerialBus.DataBitLength));
            break;

        case 4: /* Slave Mode [Flag] (_SLV) */

            RsSetFlagBits (&Descriptor->SpiSerialBus.Flags, InitializerOp, 0, 0);
            RsCreateBitField (InitializerOp, ACPI_RESTAG_SLAVEMODE,
                CurrentByteOffset + ASL_RESDESC_OFFSET (SpiSerialBus.Flags), 0);
            break;

        case 5: /* Connection Speed [DWORD] (_SPE) */

            Descriptor->SpiSerialBus.ConnectionSpeed = (UINT32) InitializerOp->Asl.Value.Integer;
            RsCreateDwordField (InitializerOp, ACPI_RESTAG_SPEED,
                CurrentByteOffset + ASL_RESDESC_OFFSET (SpiSerialBus.ConnectionSpeed));
            break;

        case 6: /* Clock Polarity [BYTE] (_POL) */

            Descriptor->SpiSerialBus.ClockPolarity = (UINT8) InitializerOp->Asl.Value.Integer;
            RsCreateByteField (InitializerOp, ACPI_RESTAG_POLARITY,
                CurrentByteOffset + ASL_RESDESC_OFFSET (SpiSerialBus.ClockPolarity));
            break;

        case 7: /* Clock Phase [BYTE] (_PHA) */

            Descriptor->SpiSerialBus.ClockPhase = (UINT8) InitializerOp->Asl.Value.Integer;
            RsCreateByteField (InitializerOp, ACPI_RESTAG_PHASE,
                CurrentByteOffset + ASL_RESDESC_OFFSET (SpiSerialBus.ClockPhase));
            break;

        case 8: /* ResSource [Optional Field - STRING] */

            if (ResSourceLength)
            {
                /* Copy string to the descriptor */

                strcpy (ResourceSource,
                    InitializerOp->Asl.Value.String);
            }
            break;

        case 9: /* Resource Index */

            if (InitializerOp->Asl.ParseOpcode != PARSEOP_DEFAULT_ARG)
            {
                Descriptor->SpiSerialBus.ResSourceIndex =
                    (UINT8) InitializerOp->Asl.Value.Integer;
            }
            break;

        case 10: /* Resource Usage (consumer/producer) */

            RsSetFlagBits (&Descriptor->SpiSerialBus.Flags, InitializerOp, 1, 1);
            break;

        case 11: /* Resource Tag (Descriptor Name) */

            UtAttachNamepathToOwner (Info->DescriptorTypeOp, InitializerOp);
            break;

        case 12:
            /*
             * Connection Share - Added for V2 (ACPI 6.0) version of the descriptor
             * Note: For V1, the share bit will be zero (Op is DEFAULT_ARG from
             * the ASL parser)
             */
            RsSetFlagBits (&Descriptor->SpiSerialBus.Flags, InitializerOp, 2, 0);
            RsCreateBitField (InitializerOp, ACPI_RESTAG_INTERRUPTSHARE,
                CurrentByteOffset + ASL_RESDESC_OFFSET (SpiSerialBus.Flags), 2);
            break;

        case 13: /* Vendor Data (Optional - Buffer of BYTEs) (_VEN) */

            RsGetVendorData (InitializerOp, VendorData,
                CurrentByteOffset + sizeof (AML_RESOURCE_SPI_SERIALBUS));
            break;

        default:    /* Ignore any extra nodes */

            break;
        }

        InitializerOp = RsCompleteNodeAndGetNext (InitializerOp);
    }

    MpSaveSerialInfo (Info->MappingOp, Descriptor, ResourceSource);
    return (Rnode);
}


/*******************************************************************************
 *
 * FUNCTION:    RsDoUartSerialBusDescriptor
 *
 * PARAMETERS:  Info                - Parse Op and resource template offset
 *
 * RETURN:      Completed resource node
 *
 * DESCRIPTION: Construct a long "UART Serial Bus" descriptor
 *
 ******************************************************************************/

ASL_RESOURCE_NODE *
RsDoUartSerialBusDescriptor (
    ASL_RESOURCE_INFO       *Info)
{
    AML_RESOURCE            *Descriptor;
    ACPI_PARSE_OBJECT       *InitializerOp;
    ASL_RESOURCE_NODE       *Rnode;
    char                    *ResourceSource = NULL;
    UINT8                   *VendorData = NULL;
    UINT16                  ResSourceLength;
    UINT16                  VendorLength;
    UINT16                  DescriptorSize;
    UINT32                  CurrentByteOffset;
    UINT32                  i;


    InitializerOp = Info->DescriptorTypeOp->Asl.Child;
    CurrentByteOffset = Info->CurrentByteOffset;

    /*
     * Calculate lengths for fields that have variable length:
     * 1) Resource Source string
     * 2) Vendor Data buffer
     */
    ResSourceLength = RsGetStringDataLength (InitializerOp);
    VendorLength = RsGetBufferDataLength (InitializerOp);

    DescriptorSize = ACPI_AML_SIZE_LARGE (AML_RESOURCE_UART_SERIALBUS) +
        ResSourceLength + VendorLength;

    /* Allocate the local resource node and initialize */

    Rnode = RsAllocateResourceNode (DescriptorSize +
        sizeof (AML_RESOURCE_LARGE_HEADER));

    Descriptor = Rnode->Buffer;
    Descriptor->UartSerialBus.ResourceLength = DescriptorSize;
    Descriptor->UartSerialBus.DescriptorType = ACPI_RESOURCE_NAME_SERIAL_BUS;
    Descriptor->UartSerialBus.RevisionId = AML_RESOURCE_UART_REVISION;
    Descriptor->UartSerialBus.TypeRevisionId = AML_RESOURCE_UART_TYPE_REVISION;
    Descriptor->UartSerialBus.Type = AML_RESOURCE_UART_SERIALBUSTYPE;
    Descriptor->UartSerialBus.TypeDataLength = AML_RESOURCE_UART_MIN_DATA_LEN + VendorLength;

    if (Info->DescriptorTypeOp->Asl.ParseOpcode == PARSEOP_UART_SERIALBUS_V2)
    {
        Descriptor->I2cSerialBus.RevisionId = 2;
    }

    /* Build pointers to optional areas */

    VendorData = ACPI_ADD_PTR (UINT8, Descriptor, sizeof (AML_RESOURCE_UART_SERIALBUS));
    ResourceSource = ACPI_ADD_PTR (char, VendorData, VendorLength);

    /* Process all child initialization nodes */

    for (i = 0; InitializerOp; i++)
    {
        switch (i)
        {
        case 0: /* Connection Speed (Baud Rate) [DWORD] (_SPE) */

            Descriptor->UartSerialBus.DefaultBaudRate = (UINT32) InitializerOp->Asl.Value.Integer;
            RsCreateDwordField (InitializerOp, ACPI_RESTAG_SPEED,
                CurrentByteOffset + ASL_RESDESC_OFFSET (UartSerialBus.DefaultBaudRate));
            break;

        case 1: /* Bits Per Byte [Flags] (_LEN) */

            RsSetFlagBits16 (&Descriptor->UartSerialBus.TypeSpecificFlags, InitializerOp, 4, 3);
            RsCreateMultiBitField (InitializerOp, ACPI_RESTAG_LENGTH,
                CurrentByteOffset + ASL_RESDESC_OFFSET (UartSerialBus.TypeSpecificFlags), 4, 3);
            break;

        case 2: /* Stop Bits [Flags] (_STB) */

            RsSetFlagBits16 (&Descriptor->UartSerialBus.TypeSpecificFlags, InitializerOp, 2, 1);
            RsCreateMultiBitField (InitializerOp, ACPI_RESTAG_STOPBITS,
                CurrentByteOffset + ASL_RESDESC_OFFSET (UartSerialBus.TypeSpecificFlags), 2, 2);
            break;

        case 3: /* Lines In Use [BYTE] (_LIN) */

            Descriptor->UartSerialBus.LinesEnabled = (UINT8) InitializerOp->Asl.Value.Integer;
            RsCreateByteField (InitializerOp, ACPI_RESTAG_LINE,
                CurrentByteOffset + ASL_RESDESC_OFFSET (UartSerialBus.LinesEnabled));
            break;

        case 4: /* Endianness [Flag] (_END) */

            RsSetFlagBits16 (&Descriptor->UartSerialBus.TypeSpecificFlags, InitializerOp, 7, 0);
            RsCreateBitField (InitializerOp, ACPI_RESTAG_ENDIANNESS,
                CurrentByteOffset + ASL_RESDESC_OFFSET (UartSerialBus.TypeSpecificFlags), 7);
            break;

        case 5: /* Parity [BYTE] (_PAR) */

            Descriptor->UartSerialBus.Parity = (UINT8) InitializerOp->Asl.Value.Integer;
            RsCreateByteField (InitializerOp, ACPI_RESTAG_PARITY,
                CurrentByteOffset + ASL_RESDESC_OFFSET (UartSerialBus.Parity));
            break;

        case 6: /* Flow Control [Flags] (_FLC) */

            RsSetFlagBits16 (&Descriptor->UartSerialBus.TypeSpecificFlags, InitializerOp, 0, 0);
            RsCreateMultiBitField (InitializerOp, ACPI_RESTAG_FLOWCONTROL,
                CurrentByteOffset + ASL_RESDESC_OFFSET (UartSerialBus.TypeSpecificFlags), 0, 2);
            break;

        case 7: /* Rx Buffer Size [WORD] (_RXL) */

            Descriptor->UartSerialBus.RxFifoSize = (UINT16) InitializerOp->Asl.Value.Integer;
            RsCreateWordField (InitializerOp, ACPI_RESTAG_LENGTH_RX,
                CurrentByteOffset + ASL_RESDESC_OFFSET (UartSerialBus.RxFifoSize));
            break;

        case 8: /* Tx Buffer Size [WORD] (_TXL) */

            Descriptor->UartSerialBus.TxFifoSize = (UINT16) InitializerOp->Asl.Value.Integer;
            RsCreateWordField (InitializerOp, ACPI_RESTAG_LENGTH_TX,
                CurrentByteOffset + ASL_RESDESC_OFFSET (UartSerialBus.TxFifoSize));
            break;

        case 9: /* ResSource [Optional Field - STRING] */

            if (ResSourceLength)
            {
                /* Copy string to the descriptor */

                strcpy (ResourceSource,
                    InitializerOp->Asl.Value.String);
            }
            break;

        case 10: /* Resource Index */

            if (InitializerOp->Asl.ParseOpcode != PARSEOP_DEFAULT_ARG)
            {
                Descriptor->UartSerialBus.ResSourceIndex =
                    (UINT8) InitializerOp->Asl.Value.Integer;
            }
            break;

        case 11: /* Resource Usage (consumer/producer) */

            RsSetFlagBits (&Descriptor->UartSerialBus.Flags, InitializerOp, 1, 1);

            /*
             * Slave Mode [Flag] (_SLV)
             *
             * Note: There is no SlaveMode argument to the UartSerialBus macro, but
             * we add this name anyway to allow the flag to be set by ASL in the
             * rare case where there is a slave mode associated with the UART.
             */
            RsCreateBitField (InitializerOp, ACPI_RESTAG_SLAVEMODE,
                CurrentByteOffset + ASL_RESDESC_OFFSET (UartSerialBus.Flags), 0);
            break;

        case 12: /* Resource Tag (Descriptor Name) */

            UtAttachNamepathToOwner (Info->DescriptorTypeOp, InitializerOp);
            break;

        case 13:
            /*
             * Connection Share - Added for V2 (ACPI 6.0) version of the descriptor
             * Note: For V1, the share bit will be zero (Op is DEFAULT_ARG from
             * the ASL parser)
             */
            RsSetFlagBits (&Descriptor->UartSerialBus.Flags, InitializerOp, 2, 0);
            RsCreateBitField (InitializerOp, ACPI_RESTAG_INTERRUPTSHARE,
                CurrentByteOffset + ASL_RESDESC_OFFSET (UartSerialBus.Flags), 2);
            break;

        case 14: /* Vendor Data (Optional - Buffer of BYTEs) (_VEN) */

            RsGetVendorData (InitializerOp, VendorData,
                CurrentByteOffset + sizeof (AML_RESOURCE_UART_SERIALBUS));
            break;

        default:    /* Ignore any extra nodes */

            break;
        }

        InitializerOp = RsCompleteNodeAndGetNext (InitializerOp);
    }

    MpSaveSerialInfo (Info->MappingOp, Descriptor, ResourceSource);
    return (Rnode);
}


/*******************************************************************************
 *
 * FUNCTION:    RsDoCsi2SerialBusDescriptor
 *
 * PARAMETERS:  Info                - Parse Op and resource template offset
 *
 * RETURN:      Completed resource node
 *
 * DESCRIPTION: Construct a long "Csi2SerialBus" descriptor
 *
 ******************************************************************************/

ASL_RESOURCE_NODE *
RsDoCsi2SerialBusDescriptor (
    ASL_RESOURCE_INFO       *Info)
{
    AML_RESOURCE            *Descriptor;
    ACPI_PARSE_OBJECT       *InitializerOp;
    ASL_RESOURCE_NODE       *Rnode;
    char                    *ResourceSource = NULL;
    UINT8                   *VendorData = NULL;
    UINT16                  ResSourceLength;
    UINT16                  VendorLength;
    UINT16                  DescriptorSize;
    UINT32                  CurrentByteOffset;
    UINT32                  i;


    InitializerOp = Info->DescriptorTypeOp->Asl.Child;
    CurrentByteOffset = Info->CurrentByteOffset;

    /*
     * Calculate lengths for fields that have variable length:
     * 1) Resource Source string
     * 2) Vendor Data buffer
     */
    ResSourceLength = RsGetStringDataLength (InitializerOp);
    VendorLength = RsGetBufferDataLength (InitializerOp);

    DescriptorSize = ACPI_AML_SIZE_LARGE (AML_RESOURCE_CSI2_SERIALBUS) +
        ResSourceLength + VendorLength;

    /* Allocate the local resource node and initialize */

    Rnode = RsAllocateResourceNode (DescriptorSize +
        sizeof (AML_RESOURCE_LARGE_HEADER));

    Descriptor = Rnode->Buffer;
    Descriptor->Csi2SerialBus.ResourceLength = DescriptorSize;
    Descriptor->Csi2SerialBus.DescriptorType = ACPI_RESOURCE_NAME_SERIAL_BUS;
    Descriptor->Csi2SerialBus.RevisionId = AML_RESOURCE_CSI2_REVISION;
    Descriptor->Csi2SerialBus.TypeRevisionId = AML_RESOURCE_CSI2_TYPE_REVISION;
    Descriptor->Csi2SerialBus.Type = AML_RESOURCE_CSI2_SERIALBUSTYPE;
    Descriptor->Csi2SerialBus.TypeDataLength = AML_RESOURCE_CSI2_MIN_DATA_LEN + VendorLength;

    /* Build pointers to optional areas */

    VendorData = ACPI_ADD_PTR (UINT8, Descriptor, sizeof (AML_RESOURCE_CSI2_SERIALBUS));
    ResourceSource = ACPI_ADD_PTR (char, VendorData, VendorLength);

    /* Process all child initialization nodes */

    for (i = 0; InitializerOp; i++)
    {
        switch (i)
        {
        case 0: /* Slave Mode [Flag] (_SLV) */

            RsSetFlagBits (&Descriptor->Csi2SerialBus.Flags, InitializerOp, 0, 0);
            RsCreateBitField (InitializerOp, ACPI_RESTAG_SLAVEMODE,
                CurrentByteOffset + ASL_RESDESC_OFFSET (I2cSerialBus.Flags), 0);
            break;

        case 1: /* Phy Type [Flag] (_PHY) */

            RsSetFlagBits16 ((UINT16 *) &Descriptor->Csi2SerialBus.TypeSpecificFlags, InitializerOp, 0, 0);
            RsCreateBitField (InitializerOp, ACPI_RESTAG_PHYTYPE,
                CurrentByteOffset + ASL_RESDESC_OFFSET (Csi2SerialBus.TypeSpecificFlags), 0);
            break;

        case 2: /* Local Port Instance [Integer] (_PRT) */

            RsSetFlagBits16 ((UINT16 *) &Descriptor->Csi2SerialBus.TypeSpecificFlags, InitializerOp, 0, 0);
            RsCreateMultiBitField (InitializerOp, ACPI_RESTAG_LOCALPORT,
                CurrentByteOffset + ASL_RESDESC_OFFSET (Csi2SerialBus.TypeSpecificFlags), 2, 6);
            break;

        case 3: /* ResSource [Optional Field - STRING] */

            if (ResSourceLength)
            {
                /* Copy string to the descriptor */

                strcpy (ResourceSource,
                    InitializerOp->Asl.Value.String);
            }
            break;

        case 4: /* Resource Index */

            if (InitializerOp->Asl.ParseOpcode != PARSEOP_DEFAULT_ARG)
            {
                Descriptor->Csi2SerialBus.ResSourceIndex =
                    (UINT8) InitializerOp->Asl.Value.Integer;
            }
            break;

        case 5: /* Resource Usage (consumer/producer) */

            RsSetFlagBits (&Descriptor->Csi2SerialBus.Flags, InitializerOp, 1, 1);
            break;

        case 6: /* Resource Tag (Descriptor Name) */

            UtAttachNamepathToOwner (Info->DescriptorTypeOp, InitializerOp);
            break;

        case 7: /* Vendor Data (Optional - Buffer of BYTEs) (_VEN) */

            RsGetVendorData (InitializerOp, VendorData,
                CurrentByteOffset + sizeof (AML_RESOURCE_CSI2_SERIALBUS));
            break;

        default:    /* Ignore any extra nodes */

            break;
        }

        InitializerOp = RsCompleteNodeAndGetNext (InitializerOp);
    }

    MpSaveSerialInfo (Info->MappingOp, Descriptor, ResourceSource);
    return (Rnode);
}


/*******************************************************************************
 *
 * FUNCTION:    RsDoPinFunctionDescriptor
 *
 * PARAMETERS:  Info                - Parse Op and resource template offset
 *
 * RETURN:      Completed resource node
 *
 * DESCRIPTION: Construct a long "PinFunction" descriptor
 *
 ******************************************************************************/

ASL_RESOURCE_NODE *
RsDoPinFunctionDescriptor (
    ASL_RESOURCE_INFO       *Info)
{
    AML_RESOURCE            *Descriptor;
    ACPI_PARSE_OBJECT       *InitializerOp;
    ASL_RESOURCE_NODE       *Rnode;
    char                    *ResourceSource = NULL;
    UINT8                   *VendorData = NULL;
    UINT16                  *PinList = NULL;
    UINT16                  ResSourceLength;
    UINT16                  VendorLength;
    UINT16                  PinListLength;
    UINT16                  DescriptorSize;
    UINT32                  CurrentByteOffset;
    UINT32                  i;

    InitializerOp = Info->DescriptorTypeOp->Asl.Child;
    CurrentByteOffset = Info->CurrentByteOffset;

    /*
     * Calculate lengths for fields that have variable length:
     * 1) Resource Source string
     * 2) Vendor Data buffer
     * 3) PIN (interrupt) list
     */
    ResSourceLength = RsGetStringDataLength (InitializerOp);
    VendorLength = RsGetBufferDataLength (InitializerOp);
    PinListLength = RsGetInterruptDataLength (InitializerOp, 8);

    DescriptorSize = ACPI_AML_SIZE_LARGE (AML_RESOURCE_PIN_FUNCTION) +
        ResSourceLength + VendorLength + PinListLength;

    /* Allocate the local resource node and initialize */

    Rnode = RsAllocateResourceNode (DescriptorSize +
        sizeof (AML_RESOURCE_LARGE_HEADER));

    Descriptor = Rnode->Buffer;
    Descriptor->PinFunction.ResourceLength = DescriptorSize;
    Descriptor->PinFunction.DescriptorType = ACPI_RESOURCE_NAME_PIN_FUNCTION;
    Descriptor->PinFunction.RevisionId = AML_RESOURCE_PIN_FUNCTION_REVISION;

    /* Build pointers to optional areas */

    PinList = ACPI_ADD_PTR (UINT16, Descriptor, sizeof (AML_RESOURCE_PIN_FUNCTION));
    ResourceSource = ACPI_ADD_PTR (char, PinList, PinListLength);
    VendorData = ACPI_ADD_PTR (UINT8, ResourceSource, ResSourceLength);

    /* Setup offsets within the descriptor */

    Descriptor->PinFunction.PinTableOffset = (UINT16)
        ACPI_PTR_DIFF (PinList, Descriptor);

    Descriptor->PinFunction.ResSourceOffset = (UINT16)
        ACPI_PTR_DIFF (ResourceSource, Descriptor);

    /* Process all child initialization nodes */

    for (i = 0; InitializerOp; i++)
    {
        switch (i)
        {
        case 0: /* Share Type [Flags] (_SHR) */

            RsSetFlagBits16 (&Descriptor->PinFunction.Flags, InitializerOp, 0, 0);
            RsCreateBitField (InitializerOp, ACPI_RESTAG_INTERRUPTSHARE,
                CurrentByteOffset + ASL_RESDESC_OFFSET (PinFunction.Flags), 0);
            break;

        case 1: /* Pin Config [BYTE] (_PPI) */

            Descriptor->PinFunction.PinConfig = (UINT8) InitializerOp->Asl.Value.Integer;
            RsCreateByteField (InitializerOp, ACPI_RESTAG_PINCONFIG,
                CurrentByteOffset + ASL_RESDESC_OFFSET (PinFunction.PinConfig));
            break;

        case 2: /* Function Number [WORD] (_FUN) */

            Descriptor->PinFunction.FunctionNumber = (UINT16) InitializerOp->Asl.Value.Integer;
            RsCreateDwordField (InitializerOp, ACPI_RESTAG_FUNCTION,
                CurrentByteOffset + ASL_RESDESC_OFFSET (PinFunction.FunctionNumber));
            break;

        case 3: /* ResSource [Optional Field - STRING] */

            if (ResSourceLength)
            {
                /* Copy string to the descriptor */

                strcpy (ResourceSource, InitializerOp->Asl.Value.String);
            }
            break;

        case 4: /* Resource Index */

            if (InitializerOp->Asl.ParseOpcode != PARSEOP_DEFAULT_ARG)
            {
                Descriptor->PinFunction.ResSourceIndex = (UINT8) InitializerOp->Asl.Value.Integer;
            }
            break;

        case 5: /* Resource Usage (consumer/producer) */

            /* Assumed to be consumer */

            break;

        case 6: /* Resource Tag (Descriptor Name) */

            UtAttachNamepathToOwner (Info->DescriptorTypeOp, InitializerOp);
            break;

        case 7: /* Vendor Data (Optional - Buffer of BYTEs) (_VEN) */
            /*
             * Always set the VendorOffset even if there is no Vendor Data.
             * This field is required in order to calculate the length
             * of the ResourceSource at runtime.
             */
            Descriptor->PinFunction.VendorOffset = (UINT16)
                ACPI_PTR_DIFF (VendorData, Descriptor);

            if (RsGetVendorData (InitializerOp, VendorData,
                (CurrentByteOffset + Descriptor->PinFunction.VendorOffset)))
            {
                Descriptor->PinFunction.VendorLength = VendorLength;
            }
            break;

        default:
            /*
             * PINs come through here, repeatedly. Each PIN must be a WORD.
             *  Name: _PIN
             */
            *PinList = (UINT16) InitializerOp->Asl.Value.Integer;
            PinList++;

            /* Case 8: First pin number in list */

            if (i == 8)
            {
                if (InitializerOp->Asl.ParseOpcode == PARSEOP_DEFAULT_ARG)
                {
                    /* Must be at least one interrupt */

                    AslError (ASL_ERROR, ASL_MSG_EX_INTERRUPT_LIST_MIN,
                        InitializerOp, NULL);
                }

                /* Check now for duplicates in list */

                RsCheckListForDuplicates (InitializerOp);

                /* Create a named field at the start of the list */

                RsCreateWordField (InitializerOp, ACPI_RESTAG_PIN,
                    CurrentByteOffset + Descriptor->PinFunction.PinTableOffset);
            }
            break;
        }

        InitializerOp = RsCompleteNodeAndGetNext (InitializerOp);
    }

    return (Rnode);
}

/*******************************************************************************
 *
 * FUNCTION:    RsDoClockInputDescriptor
 *
 * PARAMETERS:  Info                - Parse Op and resource template offset
 *
 * RETURN:      Completed resource node
 *
 * DESCRIPTION: Construct a long "ClockInput" descriptor
 *
 ******************************************************************************/

ASL_RESOURCE_NODE *
RsDoClockInputDescriptor (
    ASL_RESOURCE_INFO       *Info)
{
    AML_RESOURCE            *Descriptor;
    ACPI_PARSE_OBJECT       *InitializerOp;
    ASL_RESOURCE_NODE       *Rnode;
    char                    *ResourceSourceString = NULL;
    UINT8                   *ResourceSourceIndex = NULL;
    UINT16                  ResSourceLength;
    UINT16                  DescriptorSize;
    UINT32                  i;
    UINT32                  CurrentByteOffset;

    InitializerOp = Info->DescriptorTypeOp->Asl.Child;
    CurrentByteOffset = Info->CurrentByteOffset;

    /*
     * Calculate lengths for fields that have variable length:
     * 1) Resource Source string
     */
    ResSourceLength = RsGetStringDataLength (InitializerOp);

    DescriptorSize = ACPI_AML_SIZE_LARGE (AML_RESOURCE_CLOCK_INPUT) + ResSourceLength + 1;

    /* Allocate the local resource node and initialize */

    Rnode = RsAllocateResourceNode (DescriptorSize +
        sizeof (AML_RESOURCE_LARGE_HEADER));

    Descriptor = Rnode->Buffer;
    Descriptor->ClockInput.ResourceLength = DescriptorSize;
    Descriptor->ClockInput.DescriptorType = ACPI_RESOURCE_NAME_CLOCK_INPUT;
    Descriptor->ClockInput.RevisionId = AML_RESOURCE_CLOCK_INPUT_REVISION;

    /* Build pointers to optional areas */

    if (ResSourceLength){
        ResourceSourceIndex = ACPI_ADD_PTR (UINT8, Descriptor, sizeof (AML_RESOURCE_CLOCK_INPUT));
        ResourceSourceString = ACPI_ADD_PTR (char, Descriptor, sizeof (AML_RESOURCE_CLOCK_INPUT) + 1);
    }

    /* Process all child initialization nodes */

    for (i = 0; InitializerOp; i++)
    {
        switch (i)
        {
        case 0:
            Descriptor->ClockInput.FrequencyNumerator = (UINT32)InitializerOp->Asl.Value.Integer;
            RsCreateDwordField (InitializerOp, ACPI_RESTAG_FQN,
                CurrentByteOffset + ASL_RESDESC_OFFSET (ClockInput.FrequencyNumerator));

            break;

        case 1:
            Descriptor->ClockInput.FrequencyDivisor = (UINT16)InitializerOp->Asl.Value.Integer;
            RsCreateWordField (InitializerOp, ACPI_RESTAG_FQD,
                CurrentByteOffset + ASL_RESDESC_OFFSET (ClockInput.FrequencyDivisor));

            break;

        case 2:
            RsSetFlagBits16 (&Descriptor->ClockInput.Flags, InitializerOp, 1, 0);
            break;

        case 3:
            RsSetFlagBits16 (&Descriptor->ClockInput.Flags, InitializerOp, 0, 0);
            break;

        case 4: /* ResSource String [Optional Field] */

            if (ResourceSourceString)
            {
                /* Copy string to the descriptor */

                strcpy (ResourceSourceString, InitializerOp->Asl.Value.String);
            }
            break;

        case 5: /* ResSource Index [Optional Field] */
            if (ResourceSourceIndex)
            {
                *ResourceSourceIndex = (UINT8) InitializerOp->Asl.Value.Integer;
            }
            break;

        default:
            break;
        }

        InitializerOp = RsCompleteNodeAndGetNext (InitializerOp);
    }

    return (Rnode);
}


/*******************************************************************************
 *
 * FUNCTION:    RsDoPinConfigDescriptor
 *
 * PARAMETERS:  Info                - Parse Op and resource template offset
 *
 * RETURN:      Completed resource node
 *
 * DESCRIPTION: Construct a long "PinConfig" descriptor
 *
 ******************************************************************************/

ASL_RESOURCE_NODE *
RsDoPinConfigDescriptor (
    ASL_RESOURCE_INFO       *Info)
{
    AML_RESOURCE            *Descriptor;
    ACPI_PARSE_OBJECT       *InitializerOp;
    ASL_RESOURCE_NODE       *Rnode;
    char                    *ResourceSource = NULL;
    UINT8                   *VendorData = NULL;
    UINT16                  *PinList = NULL;
    UINT16                  ResSourceLength;
    UINT16                  VendorLength;
    UINT16                  PinListLength;
    UINT16                  DescriptorSize;
    UINT32                  CurrentByteOffset;
    UINT32                  i;

    InitializerOp = Info->DescriptorTypeOp->Asl.Child;
    CurrentByteOffset = Info->CurrentByteOffset;

    /*
     * Calculate lengths for fields that have variable length:
     * 1) Resource Source string
     * 2) Vendor Data buffer
     * 3) PIN (interrupt) list
     */
    ResSourceLength = RsGetStringDataLength (InitializerOp);
    VendorLength = RsGetBufferDataLength (InitializerOp);
    PinListLength = RsGetInterruptDataLength (InitializerOp, 8);

    DescriptorSize = ACPI_AML_SIZE_LARGE (AML_RESOURCE_PIN_CONFIG) +
        ResSourceLength + VendorLength + PinListLength;

    /* Allocate the local resource node and initialize */

    Rnode = RsAllocateResourceNode (DescriptorSize +
        sizeof (AML_RESOURCE_LARGE_HEADER));

    Descriptor = Rnode->Buffer;
    Descriptor->PinConfig.ResourceLength = DescriptorSize;
    Descriptor->PinConfig.DescriptorType = ACPI_RESOURCE_NAME_PIN_CONFIG;
    Descriptor->PinConfig.RevisionId = AML_RESOURCE_PIN_CONFIG_REVISION;

    /* Build pointers to optional areas */

    PinList = ACPI_ADD_PTR (UINT16, Descriptor, sizeof (AML_RESOURCE_PIN_CONFIG));
    ResourceSource = ACPI_ADD_PTR (char, PinList, PinListLength);
    VendorData = ACPI_ADD_PTR (UINT8, ResourceSource, ResSourceLength);

    /* Setup offsets within the descriptor */

    Descriptor->PinConfig.PinTableOffset = (UINT16)
        ACPI_PTR_DIFF (PinList, Descriptor);

    Descriptor->PinConfig.ResSourceOffset = (UINT16)
        ACPI_PTR_DIFF (ResourceSource, Descriptor);

    /* Process all child initialization nodes */

    for (i = 0; InitializerOp; i++)
    {
        BOOLEAN isValid;

        switch (i)
        {
        case 0: /* Share Type [Flags] (_SHR) */

            RsSetFlagBits16 (&Descriptor->PinConfig.Flags, InitializerOp, 0, 0);
            RsCreateBitField (InitializerOp, ACPI_RESTAG_INTERRUPTSHARE,
                CurrentByteOffset + ASL_RESDESC_OFFSET (PinConfig.Flags), 0);
            break;

        case 1: /* Pin Config Type [BYTE] (_TYP) */

            isValid = InitializerOp->Asl.Value.Integer <= 0x0d;
            if (!isValid)
            {
                isValid = InitializerOp->Asl.Value.Integer >= 0x80 &&
                          InitializerOp->Asl.Value.Integer <= 0xff;
            }
            if (!isValid)
            {
                    AslError (ASL_ERROR, ASL_MSG_RANGE, InitializerOp, NULL);
            }

            Descriptor->PinConfig.PinConfigType = (UINT8) InitializerOp->Asl.Value.Integer;
            RsCreateByteField (InitializerOp, ACPI_RESTAG_PINCONFIG_TYPE,
                CurrentByteOffset + ASL_RESDESC_OFFSET (PinConfig.PinConfigType));

            break;

        case 2: /* Pin Config Value [DWORD] (_VAL) */

            Descriptor->PinConfig.PinConfigValue = (UINT32) InitializerOp->Asl.Value.Integer;
            RsCreateDwordField (InitializerOp, ACPI_RESTAG_PINCONFIG_VALUE,
                CurrentByteOffset + ASL_RESDESC_OFFSET (PinConfig.PinConfigValue));
            break;

        case 3: /* ResSource [Optional Field - STRING] */

            if (ResSourceLength)
            {
                /* Copy string to the descriptor */

                strcpy (ResourceSource, InitializerOp->Asl.Value.String);
            }
            break;

        case 4: /* Resource Index */

            if (InitializerOp->Asl.ParseOpcode != PARSEOP_DEFAULT_ARG)
            {
                Descriptor->PinConfig.ResSourceIndex = (UINT8) InitializerOp->Asl.Value.Integer;
            }
            break;

        case 5: /* Resource Usage (consumer/producer) */

            RsSetFlagBits16 (&Descriptor->PinConfig.Flags, InitializerOp, 1, 1);

            break;

        case 6: /* Resource Tag (Descriptor Name) */

            UtAttachNamepathToOwner (Info->DescriptorTypeOp, InitializerOp);
            break;

        case 7: /* Vendor Data (Optional - Buffer of BYTEs) (_VEN) */
            /*
             * Always set the VendorOffset even if there is no Vendor Data.
             * This field is required in order to calculate the length
             * of the ResourceSource at runtime.
             */
            Descriptor->PinConfig.VendorOffset = (UINT16)
                ACPI_PTR_DIFF (VendorData, Descriptor);

            if (RsGetVendorData (InitializerOp, VendorData,
                (CurrentByteOffset + Descriptor->PinConfig.VendorOffset)))
            {
                Descriptor->PinConfig.VendorLength = VendorLength;
            }
            break;

        default:
            /*
             * PINs come through here, repeatedly. Each PIN must be a WORD.
             *  Name: _PIN
             */
            *PinList = (UINT16) InitializerOp->Asl.Value.Integer;
            PinList++;

            /* Case 8: First pin number in list */

            if (i == 8)
            {
                if (InitializerOp->Asl.ParseOpcode == PARSEOP_DEFAULT_ARG)
                {
                    /* Must be at least one interrupt */

                    AslError (ASL_ERROR, ASL_MSG_EX_INTERRUPT_LIST_MIN,
                        InitializerOp, NULL);
                }

                /* Check now for duplicates in list */

                RsCheckListForDuplicates (InitializerOp);

                /* Create a named field at the start of the list */

                RsCreateWordField (InitializerOp, ACPI_RESTAG_PIN,
                    CurrentByteOffset + Descriptor->PinConfig.PinTableOffset);
            }
            break;
        }

        InitializerOp = RsCompleteNodeAndGetNext (InitializerOp);
    }

    return (Rnode);
}


/*******************************************************************************
 *
 * FUNCTION:    RsDoPinGroupDescriptor
 *
 * PARAMETERS:  Info                - Parse Op and resource template offset
 *
 * RETURN:      Completed resource node
 *
 * DESCRIPTION: Construct a long "PinGroup" descriptor
 *
 ******************************************************************************/

ASL_RESOURCE_NODE *
RsDoPinGroupDescriptor (
    ASL_RESOURCE_INFO       *Info)
{
    AML_RESOURCE            *Descriptor;
    ACPI_PARSE_OBJECT       *InitializerOp;
    ASL_RESOURCE_NODE       *Rnode;
    UINT8                   *VendorData = NULL;
    UINT16                  *PinList = NULL;
    char                    *Label = NULL;
    UINT16                  LabelLength;
    UINT16                  VendorLength;
    UINT16                  PinListLength;
    UINT16                  DescriptorSize;
    UINT32                  CurrentByteOffset;
    UINT32                  i;

    InitializerOp = Info->DescriptorTypeOp->Asl.Child;
    CurrentByteOffset = Info->CurrentByteOffset;

    /*
     * Calculate lengths for fields that have variable length:
     * 1) Label
     * 2) Vendor Data buffer
     * 3) PIN (interrupt) list
     */
    LabelLength = RsGetStringDataLength (InitializerOp);
    VendorLength = RsGetBufferDataLength (InitializerOp);
    PinListLength = RsGetInterruptDataLength (InitializerOp, 4);

    DescriptorSize = ACPI_AML_SIZE_LARGE (AML_RESOURCE_PIN_GROUP) +
        LabelLength + VendorLength + PinListLength;

    /* Allocate the local resource node and initialize */

    Rnode = RsAllocateResourceNode (DescriptorSize +
        sizeof (AML_RESOURCE_LARGE_HEADER));

    Descriptor = Rnode->Buffer;
    Descriptor->PinGroup.ResourceLength = DescriptorSize;
    Descriptor->PinGroup.DescriptorType = ACPI_RESOURCE_NAME_PIN_GROUP;
    Descriptor->PinGroup.RevisionId = AML_RESOURCE_PIN_GROUP_REVISION;

    /* Build pointers to optional areas */

    PinList = ACPI_ADD_PTR (UINT16, Descriptor, sizeof (AML_RESOURCE_PIN_GROUP));
    Label = ACPI_ADD_PTR (char, PinList, PinListLength);
    VendorData = ACPI_ADD_PTR (UINT8, Label, LabelLength);

    /* Setup offsets within the descriptor */

    Descriptor->PinGroup.PinTableOffset = (UINT16) ACPI_PTR_DIFF (PinList, Descriptor);
    Descriptor->PinGroup.LabelOffset = (UINT16) ACPI_PTR_DIFF (Label, Descriptor);

    /* Process all child initialization nodes */

    for (i = 0; InitializerOp; i++)
    {
        switch (i)
        {
        case 0: /* Resource Label */

            if (LabelLength < 2)
            {
                AslError(ASL_WARNING, ASL_MSG_NULL_STRING, InitializerOp, NULL);
            }
            strcpy (Label, InitializerOp->Asl.Value.String);

            break;

        case 1: /* Resource Usage (consumer/producer) */

            RsSetFlagBits16 (&Descriptor->PinGroup.Flags, InitializerOp, 0, 0);

            break;

        case 2: /* Resource Tag (Descriptor Name) */

            UtAttachNamepathToOwner (Info->DescriptorTypeOp, InitializerOp);
            break;

        case 3: /* Vendor Data (Optional - Buffer of BYTEs) (_VEN) */
            /*
             * Always set the VendorOffset even if there is no Vendor Data.
             * This field is required in order to calculate the length
             * of the ResourceSource at runtime.
             */
            Descriptor->PinGroup.VendorOffset = (UINT16)
                ACPI_PTR_DIFF (VendorData, Descriptor);

            if (RsGetVendorData (InitializerOp, VendorData,
                (CurrentByteOffset + Descriptor->PinGroup.VendorOffset)))
            {
                Descriptor->PinGroup.VendorLength = VendorLength;
            }
            break;

        default:
            /*
             * PINs come through here, repeatedly. Each PIN must be a WORD.
             *  Name: _PIN
             */
            *PinList = (UINT16) InitializerOp->Asl.Value.Integer;
            PinList++;

            /* Case 3: First pin number in list */

            if (i == 4)
            {
                if (InitializerOp->Asl.ParseOpcode == PARSEOP_DEFAULT_ARG)
                {
                    /* Must be at least one interrupt */

                    AslError (ASL_ERROR, ASL_MSG_EX_INTERRUPT_LIST_MIN,
                        InitializerOp, NULL);
                }

                /* Check now for duplicates in list */

                RsCheckListForDuplicates (InitializerOp);

                /* Create a named field at the start of the list */

                RsCreateWordField (InitializerOp, ACPI_RESTAG_PIN,
                    CurrentByteOffset + Descriptor->PinGroup.PinTableOffset);
            }
            break;
        }

        InitializerOp = RsCompleteNodeAndGetNext (InitializerOp);
    }

    return (Rnode);
}


/*******************************************************************************
 *
 * FUNCTION:    RsDoPinGroupFunctionDescriptor
 *
 * PARAMETERS:  Info                - Parse Op and resource template offset
 *
 * RETURN:      Completed resource node
 *
 * DESCRIPTION: Construct a long "PinGroupFunction" descriptor
 *
 ******************************************************************************/

ASL_RESOURCE_NODE *
RsDoPinGroupFunctionDescriptor (
    ASL_RESOURCE_INFO       *Info)
{
    AML_RESOURCE            *Descriptor;
    ACPI_PARSE_OBJECT       *InitializerOp;
    ASL_RESOURCE_NODE       *Rnode;
    char                    *ResourceSource = NULL;
    char                    *ResourceSourceLabel = NULL;
    UINT8                   *VendorData = NULL;
    UINT16                  ResSourceLength;
    UINT16                  ResSourceLabelLength;
    UINT16                  VendorLength;
    UINT16                  DescriptorSize;
    UINT32                  CurrentByteOffset;
    UINT32                  i;

    InitializerOp = Info->DescriptorTypeOp->Asl.Child;
    CurrentByteOffset = Info->CurrentByteOffset;

    /*
     * Calculate lengths for fields that have variable length:
     * 1) Resource Source string
     * 2) Resource Source Label string
     * 3) Vendor Data buffer
     */
    ResSourceLength = RsGetStringDataLengthAt (InitializerOp, 2);
    ResSourceLabelLength = RsGetStringDataLengthAt (InitializerOp, 4);
    VendorLength = RsGetBufferDataLength (InitializerOp);

    DescriptorSize = ACPI_AML_SIZE_LARGE (AML_RESOURCE_PIN_GROUP_FUNCTION) +
        ResSourceLength + ResSourceLabelLength + VendorLength;

    /* Allocate the local resource node and initialize */

    Rnode = RsAllocateResourceNode (DescriptorSize +
        sizeof (AML_RESOURCE_LARGE_HEADER));

    Descriptor = Rnode->Buffer;
    Descriptor->PinGroupFunction.ResourceLength = DescriptorSize;
    Descriptor->PinGroupFunction.DescriptorType = ACPI_RESOURCE_NAME_PIN_GROUP_FUNCTION;
    Descriptor->PinGroupFunction.RevisionId = AML_RESOURCE_PIN_GROUP_FUNCTION_REVISION;

    /* Build pointers to optional areas */

    ResourceSource = ACPI_ADD_PTR (char, Descriptor, sizeof (AML_RESOURCE_PIN_GROUP_FUNCTION));
    ResourceSourceLabel = ACPI_ADD_PTR (char, ResourceSource, ResSourceLength);
    VendorData = ACPI_ADD_PTR (UINT8, ResourceSourceLabel, ResSourceLabelLength);

    /* Setup offsets within the descriptor */

    Descriptor->PinGroupFunction.ResSourceOffset = (UINT16)
        ACPI_PTR_DIFF (ResourceSource, Descriptor);
    Descriptor->PinGroupFunction.ResSourceLabelOffset = (UINT16)
        ACPI_PTR_DIFF (ResourceSourceLabel, Descriptor);

    /* Process all child initialization nodes */

    for (i = 0; InitializerOp; i++)
    {
        switch (i)
        {
        case 0: /* Share Type [Flags] (_SHR) */

            RsSetFlagBits16 (&Descriptor->PinGroupFunction.Flags, InitializerOp, 0, 0);
            RsCreateBitField (InitializerOp, ACPI_RESTAG_INTERRUPTSHARE,
                CurrentByteOffset + ASL_RESDESC_OFFSET (PinGroupFunction.Flags), 0);
            break;

        case 1: /* Function Number [WORD] */

            Descriptor->PinGroupFunction.FunctionNumber = (UINT16) InitializerOp->Asl.Value.Integer;
            RsCreateDwordField (InitializerOp, ACPI_RESTAG_FUNCTION,
                CurrentByteOffset + ASL_RESDESC_OFFSET (PinGroupFunction.FunctionNumber));
            break;

        case 2: /* ResourceSource [STRING] */

            strcpy (ResourceSource, InitializerOp->Asl.Value.String);
            break;

        case 3: /* Resource Index */

            Descriptor->PinGroupFunction.ResSourceIndex = (UINT8) InitializerOp->Asl.Value.Integer;
            break;

        case 4: /* ResourceSourceLabel [STRING] */

            if (ResSourceLabelLength < 2)
            {
                AslError(ASL_WARNING, ASL_MSG_NULL_STRING, InitializerOp, NULL);
            }

            strcpy (ResourceSourceLabel, InitializerOp->Asl.Value.String);
            break;

        case 5: /* Resource Usage (consumer/producer) */

            RsSetFlagBits16 (&Descriptor->PinGroupFunction.Flags, InitializerOp, 1, 1);

            break;

        case 6: /* Resource Tag (Descriptor Name) */

            UtAttachNamepathToOwner (Info->DescriptorTypeOp, InitializerOp);
            break;

        case 7: /* Vendor Data (Optional - Buffer of BYTEs) (_VEN) */
            /*
             * Always set the VendorOffset even if there is no Vendor Data.
             * This field is required in order to calculate the length
             * of the ResourceSource at runtime.
             */
            Descriptor->PinGroupFunction.VendorOffset = (UINT16)
                ACPI_PTR_DIFF (VendorData, Descriptor);

            if (RsGetVendorData (InitializerOp, VendorData,
                (CurrentByteOffset + Descriptor->PinGroupFunction.VendorOffset)))
            {
                Descriptor->PinGroupFunction.VendorLength = VendorLength;
            }
            break;

        default:
            break;
        }

        InitializerOp = RsCompleteNodeAndGetNext (InitializerOp);
    }

    return (Rnode);
}


/*******************************************************************************
 *
 * FUNCTION:    RsDoPinGroupConfigDescriptor
 *
 * PARAMETERS:  Info                - Parse Op and resource template offset
 *
 * RETURN:      Completed resource node
 *
 * DESCRIPTION: Construct a long "PinGroupConfig" descriptor
 *
 ******************************************************************************/

ASL_RESOURCE_NODE *
RsDoPinGroupConfigDescriptor (
    ASL_RESOURCE_INFO       *Info)
{
    AML_RESOURCE            *Descriptor;
    ACPI_PARSE_OBJECT       *InitializerOp;
    ASL_RESOURCE_NODE       *Rnode;
    char                    *ResourceSource = NULL;
    char                    *ResourceSourceLabel = NULL;
    UINT8                   *VendorData = NULL;
    UINT16                  ResSourceLength;
    UINT16                  ResSourceLabelLength;
    UINT16                  VendorLength;
    UINT16                  DescriptorSize;
    UINT32                  CurrentByteOffset;
    UINT32                  i;

    InitializerOp = Info->DescriptorTypeOp->Asl.Child;
    CurrentByteOffset = Info->CurrentByteOffset;

    /*
     * Calculate lengths for fields that have variable length:
     * 1) Resource Source string
     * 2) Resource Source Label string
     * 3) Vendor Data buffer
     */
    ResSourceLength = RsGetStringDataLengthAt (InitializerOp, 3);
    ResSourceLabelLength = RsGetStringDataLengthAt (InitializerOp, 5);
    VendorLength = RsGetBufferDataLength (InitializerOp);

    DescriptorSize = ACPI_AML_SIZE_LARGE (AML_RESOURCE_PIN_GROUP_CONFIG) +
        ResSourceLength + ResSourceLabelLength + VendorLength;

    /* Allocate the local resource node and initialize */

    Rnode = RsAllocateResourceNode (DescriptorSize +
        sizeof (AML_RESOURCE_LARGE_HEADER));

    Descriptor = Rnode->Buffer;
    Descriptor->PinGroupConfig.ResourceLength = DescriptorSize;
    Descriptor->PinGroupConfig.DescriptorType = ACPI_RESOURCE_NAME_PIN_GROUP_CONFIG;
    Descriptor->PinGroupConfig.RevisionId = AML_RESOURCE_PIN_GROUP_CONFIG_REVISION;

    /* Build pointers to optional areas */

    ResourceSource = ACPI_ADD_PTR (char, Descriptor, sizeof (AML_RESOURCE_PIN_GROUP_CONFIG));
    ResourceSourceLabel = ACPI_ADD_PTR (char, ResourceSource, ResSourceLength);
    VendorData = ACPI_ADD_PTR (UINT8, ResourceSourceLabel, ResSourceLabelLength);

    /* Setup offsets within the descriptor */

    Descriptor->PinGroupConfig.ResSourceOffset = (UINT16)
        ACPI_PTR_DIFF (ResourceSource, Descriptor);
    Descriptor->PinGroupConfig.ResSourceLabelOffset = (UINT16)
        ACPI_PTR_DIFF (ResourceSourceLabel, Descriptor);

    /* Process all child initialization nodes */

    for (i = 0; InitializerOp; i++)
    {
        BOOLEAN isValid;

        switch (i)
        {
        case 0: /* Share Type [Flags] (_SHR) */

            RsSetFlagBits16 (&Descriptor->PinGroupConfig.Flags, InitializerOp, 0, 0);
            RsCreateBitField (InitializerOp, ACPI_RESTAG_INTERRUPTSHARE,
                CurrentByteOffset + ASL_RESDESC_OFFSET (PinGroupConfig.Flags), 0);
            break;

        case 1: /* Pin Config Type [BYTE] (_TYP) */

            isValid = InitializerOp->Asl.Value.Integer <= 0x0d;
            if (!isValid)
            {
                isValid = InitializerOp->Asl.Value.Integer >= 0x80 &&
                          InitializerOp->Asl.Value.Integer <= 0xff;
            }
            if (!isValid)
            {
                    AslError (ASL_ERROR, ASL_MSG_RANGE, InitializerOp, NULL);
            }

            Descriptor->PinGroupConfig.PinConfigType = (UINT8) InitializerOp->Asl.Value.Integer;
            RsCreateByteField (InitializerOp, ACPI_RESTAG_PINCONFIG_TYPE,
                CurrentByteOffset + ASL_RESDESC_OFFSET (PinGroupConfig.PinConfigType));

            break;

        case 2: /* Pin Config Value [DWORD] (_VAL) */

            Descriptor->PinGroupConfig.PinConfigValue = (UINT32) InitializerOp->Asl.Value.Integer;
            RsCreateDwordField (InitializerOp, ACPI_RESTAG_PINCONFIG_VALUE,
                CurrentByteOffset + ASL_RESDESC_OFFSET (PinGroupConfig.PinConfigValue));
            break;

        case 3: /* ResourceSource [STRING] */

            /* Copy string to the descriptor */

            strcpy (ResourceSource, InitializerOp->Asl.Value.String);
            break;

        case 4: /* Resource Index */

            Descriptor->PinGroupConfig.ResSourceIndex = (UINT8) InitializerOp->Asl.Value.Integer;
            break;

        case 5: /* ResourceSourceLabel [STRING] */

            if (ResSourceLabelLength < 2)
            {
                AslError(ASL_WARNING, ASL_MSG_NULL_STRING, InitializerOp, NULL);
            }

            strcpy (ResourceSourceLabel, InitializerOp->Asl.Value.String);
            break;

        case 6: /* Resource Usage (consumer/producer) */

            RsSetFlagBits16 (&Descriptor->PinGroupConfig.Flags, InitializerOp, 1, 1);

            break;

        case 7: /* Resource Tag (Descriptor Name) */

            UtAttachNamepathToOwner (Info->DescriptorTypeOp, InitializerOp);
            break;

        case 8: /* Vendor Data (Optional - Buffer of BYTEs) (_VEN) */
            /*
             * Always set the VendorOffset even if there is no Vendor Data.
             * This field is required in order to calculate the length
             * of the ResourceSource at runtime.
             */
            Descriptor->PinGroupConfig.VendorOffset = (UINT16)
                ACPI_PTR_DIFF (VendorData, Descriptor);

            if (RsGetVendorData (InitializerOp, VendorData,
                (CurrentByteOffset + Descriptor->PinGroupConfig.VendorOffset)))
            {
                Descriptor->PinGroupConfig.VendorLength = VendorLength;
            }
            break;

        default:
            break;
        }

        InitializerOp = RsCompleteNodeAndGetNext (InitializerOp);
    }

    return (Rnode);
}
