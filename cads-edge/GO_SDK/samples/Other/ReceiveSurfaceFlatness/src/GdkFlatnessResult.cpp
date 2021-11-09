/**
* @file    GdkFlatnessResult.cpp
*
* @internal
* Copyright (C) 2016-2019 by LMI Technologies Inc.
* Licensed under the MIT License.
* Redistributed files must retain the above copyright notice.
*/
#include "GdkFlatnessResult.h"

#include <kApi/Data/kArray1.h>
#include <kApi/Io/kMemory.h>
#include <kApi/Io/kSerializer.h>


kStatus GdkSurfaceFlatness_SerializeGenericOutput(const std::vector<GdkFlatnessResultStruct>& gridElements, size_t & ElementNum, kArray1* outputBuffer)
{

    if (outputBuffer != kNULL)
    {
        kMemory memory;
        kSerializer serializer;
        k8u* out;
        k8u* in;
        k64u len;

        kSize i;
        kMemory_Construct(&memory, kNULL);
        kSerializer_Construct(&serializer, memory, kNULL, kNULL);

        kCheck(kSerializer_BeginWrite(serializer, kTypeOf(k16u), kFALSE));

        // information 1: output how many grid elements, including global max/min/flatness
        //size_t ElementNum = gridElements.size();
        kCheck(kSerializer_WriteSize(serializer, ElementNum));

        // only when the gridElements is not empty, output the following information
        if (ElementNum != 0)
        {
            // information 2: increase version if a change occurs in the struct, and implement a different reader for each version.
            k32u version = 1;
            kCheck(kSerializer_Write32u(serializer, version));

            // information 3: local and global results
            for (i = 0; i < ElementNum; i++)
            {
                kCheck(kSerializer_Write64f(serializer, gridElements[i].maxValue));
                kCheck(kSerializer_Write64f(serializer, gridElements[i].minValue));
                kCheck(kSerializer_Write64f(serializer, gridElements[i].flatnessValue));
            }
            // Note since we also add the global result to the generic output, so here still need to repeat these code 
            kCheck(kSerializer_Write64f(serializer, gridElements[ElementNum].maxValue));
            kCheck(kSerializer_Write64f(serializer, gridElements[ElementNum].minValue));
            kCheck(kSerializer_Write64f(serializer, gridElements[ElementNum].flatnessValue));

        }

        kCheck(kSerializer_EndWrite(serializer));
        kSerializer_Flush(serializer);

        len = kMemory_Length(memory);
        kArray1_Construct(outputBuffer, kTypeOf(k8u), len, kNULL);
        out = (k8u*)kArray1_Data(*outputBuffer);
        in = (k8u*)kMemory_At(memory, 0);
        kMemCopy(out, in, len);

        kDestroyRef(&memory);
        kDestroyRef(&serializer);
    }


    return kOK;

}


kStatus GdkFlatnessResult_DeserializeGenericOutput(std::vector<GdkFlatnessResultStruct>& gridElements, size_t & localElementNum, void* inputBuffer, kSize length, kAlloc alloc)
{
    k32s i;
    //kSize ElementNum = 0;
    kSerializer serializer;
    k32u version;

    kMemory memory;
    kMemory_Construct(&memory, alloc);
    kMemory_Attach(memory, inputBuffer, 0, length, length);

    kSerializer_Construct(&serializer, memory, kNULL, alloc);
    kCheck(kSerializer_BeginRead(serializer, kTypeOf(k16u), kFALSE));

    // read the ElementNum
    kSerializer_ReadSize(serializer, &localElementNum);

    // only when the gridElements is not empty, read the following information
    if (localElementNum != 0)
    {
        // read the version 
        kCheck(kSerializer_Read32u(serializer, &version));

        switch (version)
        {
        default:
            return kERROR;

        case 1:
            for (i = 0; i < localElementNum + 1; ++i) // Note here we need to add 1
            {
                GdkFlatnessResultStruct output;
                kZero(output);

                kCheck(kSerializer_Read64f(serializer, &output.maxValue));
                kCheck(kSerializer_Read64f(serializer, &output.minValue));
                kCheck(kSerializer_Read64f(serializer, &output.flatnessValue));
                gridElements.push_back(output);
            }
            break;
        }
    }

    kCheck(kSerializer_EndRead(serializer));
    kDestroyRef(&serializer);
    kDestroyRef(&memory);

    return kOK;
}

#ifndef GDKAPP

kBool GdkFlatnessResult_TypeValid(GoGenericMsg genMsg)
{
    if (genMsg)
    {
        k32u utype = GoGenericMsg_UserType(genMsg);
        if (utype == GDK_FLATNESS_DATA_TYPE)
        {
            return kTRUE;
        }
    }

    return kFALSE;
}

kStatus GdkFlatnessResult_DeserializeGridElements(GoGenericMsg genMsg, std::vector<GdkFlatnessResultStruct>& gridElements, size_t & localElementNum)
{
    if (GdkFlatnessResult_TypeValid(genMsg))
    {
        void* ptr = (void*)GoGenericMsg_BufferData(genMsg);
        GdkFlatnessResult_DeserializeGenericOutput(gridElements, localElementNum, ptr, GoGenericMsg_BufferSize(genMsg), kNULL);

        return kOK;
    }

    return kERROR;
}

#endif
