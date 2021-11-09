/**
* @file    GdkSurfaceTrackInspectionResult.cpp
*
* @internal
* Copyright (C) 2016-2019 by LMI Technologies Inc.
* Licensed under the MIT License.
* Redistributed files must retain the above copyright notice.
*/
#include "GdkSurfaceTrackInspectionResult.h"

#include <kApi/Data/kArray1.h>
#include <kApi/Io/kMemory.h>
#include <kApi/Io/kSerializer.h>


kStatus GdkSurfaceTrackInspectionResult_SerializeGenericOutput(const std::vector<GdkSurfaceTrackInspectionResultStruct>& elements, kSize & elementNum, kArray1* outputBuffer)
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

        //size_t elementNum = elements.size();
        kCheck(kSerializer_WriteSize(serializer, elementNum));

        // only when the gridElements is not empty, output the following information
        if (elementNum != 0)
        {
            // information 2: increase version if a change occurs in the struct, and implement a different reader for each version.
            k32u version = 2;
            kCheck(kSerializer_Write32u(serializer, version));

            for (i = 0; i < elementNum; i++)
            {
                kCheck(kSerializer_Write32u(serializer, elements[i].trackID));
                kCheck(kSerializer_Write32u(serializer, elements[i].segmentID));
                kCheck(kSerializer_Write64f(serializer, elements[i].width));
                kCheck(kSerializer_Write64f(serializer, elements[i].peakHeight));
                kCheck(kSerializer_Write64f(serializer, elements[i].offset));
                kCheck(kSerializer_Write64f(serializer, elements[i].centerX));
                kCheck(kSerializer_Write64f(serializer, elements[i].centerY));
                kCheck(kSerializer_Write64f(serializer, elements[i].area));
            }
        }

        kCheck(kSerializer_EndWrite(serializer));
        kSerializer_Flush(serializer);

        len = kMemory_Length(memory);
        kArray1_Construct(outputBuffer, kTypeOf(k8u), (kSize)len, kNULL);
        out = (k8u*)kArray1_Data(*outputBuffer);
        in = (k8u*)kMemory_At(memory, 0);
        kMemCopy(out, in, (kSize)len);

        kDestroyRef(&memory);
        kDestroyRef(&serializer);
    }

    return kOK;
}

kStatus GdkSurfaceTrackInspectionResult_DeserializeGenericOutput(std::vector<GdkSurfaceTrackInspectionResultStruct>& elements, kSize & elementNum, void* inputBuffer, kSize length, kAlloc alloc)
{
    k32u i;
    kSerializer serializer;
    k32u version;

    kMemory memory;
    kMemory_Construct(&memory, alloc);
    kMemory_Attach(memory, inputBuffer, 0, length, length);

    kSerializer_Construct(&serializer, memory, kNULL, alloc);
    kCheck(kSerializer_BeginRead(serializer, kTypeOf(k16u), kFALSE));

    // read the elementNum
    kSerializer_ReadSize(serializer, &elementNum);

    // only when the elements is not empty, read the following information
    if (elementNum != 0)
    {
        // read the version
        kCheck(kSerializer_Read32u(serializer, &version));

        switch (version)
        {
        default:
            return kERROR;
        case 1:
            for (i = 0; i < elementNum; ++i)
            {
                GdkSurfaceTrackInspectionResultStruct output;
                kZero(output);

                kCheck(kSerializer_Read32u(serializer, &output.trackID));
                kCheck(kSerializer_Read32u(serializer, &output.segmentID));
                kCheck(kSerializer_Read64f(serializer, &output.width));
                kCheck(kSerializer_Read64f(serializer, &output.peakHeight));
                kCheck(kSerializer_Read64f(serializer, &output.offset));
                kCheck(kSerializer_Read64f(serializer, &output.centerX));
                kCheck(kSerializer_Read64f(serializer, &output.centerY));
                elements.push_back(output);
            }
            break;
        case 2:
            for (i = 0; i < elementNum; ++i)
            {
                GdkSurfaceTrackInspectionResultStruct output;
                kZero(output);

                kCheck(kSerializer_Read32u(serializer, &output.trackID));
                kCheck(kSerializer_Read32u(serializer, &output.segmentID));
                kCheck(kSerializer_Read64f(serializer, &output.width));
                kCheck(kSerializer_Read64f(serializer, &output.peakHeight));
                kCheck(kSerializer_Read64f(serializer, &output.offset));
                kCheck(kSerializer_Read64f(serializer, &output.centerX));
                kCheck(kSerializer_Read64f(serializer, &output.centerY));
                kCheck(kSerializer_Read64f(serializer, &output.area));
                elements.push_back(output);
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

kBool GdkSurfaceTrackInspectionResult_TypeValid(GoGenericMsg genMsg)
{
    if (genMsg)
    {
        k32u utype = GoGenericMsg_UserType(genMsg);
        if (utype == GDK_SURFACE_TRACK_INSPECTION_DATA_TYPE)
        {
            return kTRUE;
        }
    }

    return kFALSE;
}

kStatus GdkSurfaceTrackInspectionResult_DeserializeElements(GoGenericMsg genMsg, std::vector<GdkSurfaceTrackInspectionResultStruct>& elements, kSize & elementNum)
{
    if (GdkSurfaceTrackInspectionResult_TypeValid(genMsg))
    {
        void* ptr = (void*)GoGenericMsg_BufferData(genMsg);
        GdkSurfaceTrackInspectionResult_DeserializeGenericOutput(elements, elementNum, ptr, GoGenericMsg_BufferSize(genMsg), kNULL);

        return kOK;
    }

    return kERROR;
}

#endif
