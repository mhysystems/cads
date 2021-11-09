/**
* @file    GdkPartSegmentResult.cpp
*
* @internal
* Copyright (C) 2016-2019 by LMI Technologies Inc.
* Licensed under the MIT License.
* Redistributed files must retain the above copyright notice.
*/
#include "GdkPartSegmentResult.h"

#include <kApi/Data/kArray1.h>
#include <kApi/Io/kMemory.h>
#include <kApi/Io/kSerializer.h>

kStatus GdkPartSegmentResult_SerializePart(GdkPartSegmentResult output, kSerializer serializer)
{
    GdkPartSegmentResultClass* obj = (GdkPartSegmentResultClass*)output;

    k32u version = 1;
    kCheck(kSerializer_Write32u(serializer, version));

    kCheck(kSerializer_Write64f(serializer, obj->area));

    kCheck(kSerializer_Write64f(serializer, obj->aspect));
    kCheck(kSerializer_Write64f(serializer, obj->heightMax));
    kCheck(kSerializer_Write64f(serializer, obj->angle));

    kCheck(kSerializer_Write32f(serializer, obj->center.x));
    kCheck(kSerializer_Write32f(serializer, obj->center.y));
    kCheck(kSerializer_Write32f(serializer, obj->centerHoriz.x));
    kCheck(kSerializer_Write32f(serializer, obj->centerHoriz.y));
    kCheck(kSerializer_Write32f(serializer, obj->centerContourPts.x));
    kCheck(kSerializer_Write32f(serializer, obj->centerContourPts.y));

    kCheck(kSerializer_Write64f(serializer, obj->width));
    kCheck(kSerializer_Write64f(serializer, obj->length));
    kCheck(kSerializer_WriteSize(serializer, obj->contourPtCount));

    kCheck(kSerializer_Write32f(serializer, obj->minAreaRectCorners[0].x));
    kCheck(kSerializer_Write32f(serializer, obj->minAreaRectCorners[0].y));
    kCheck(kSerializer_Write32f(serializer, obj->minAreaRectCorners[1].x));
    kCheck(kSerializer_Write32f(serializer, obj->minAreaRectCorners[1].y));
    kCheck(kSerializer_Write32f(serializer, obj->minAreaRectCorners[2].x));
    kCheck(kSerializer_Write32f(serializer, obj->minAreaRectCorners[2].y));
    kCheck(kSerializer_Write32f(serializer, obj->minAreaRectCorners[3].x));
    kCheck(kSerializer_Write32f(serializer, obj->minAreaRectCorners[3].y));

    kCheck(kSerializer_Write32s(serializer, obj->boundingBox.x));
    kCheck(kSerializer_Write32s(serializer, obj->boundingBox.y));
    kCheck(kSerializer_Write32s(serializer, obj->boundingBox.width));
    kCheck(kSerializer_Write32s(serializer, obj->boundingBox.height));

    kCheck(kSerializer_WriteSize(serializer, (kSize)obj->contourPoints.size()));

    for(auto point: obj->contourPoints)
    {
        kCheck(kSerializer_Write64f(serializer, point.x));
        kCheck(kSerializer_Write64f(serializer, point.y));
    }

    return kOK;
}
kStatus GdkPartSegmentResult_Serialize(GdkPartSegmentResult output, kArray1* outputBuffer)
{
    GdkPartSegmentResultClass* obj = (GdkPartSegmentResultClass*)output;

    kMemory memory;
    kSerializer serializer;
    kMemory_Construct(&memory, kNULL);
    kSerializer_Construct(&serializer, memory, kNULL, kNULL);

    GdkPartSegmentResult_SerializePart(output, serializer);

    kSerializer_Flush(serializer);

    if (outputBuffer != kNULL)
    {
        k64u len = kMemory_Length(memory);
        kArray1_Construct(outputBuffer, kTypeOf(k8u), len, kNULL);
        k8u* out = (k8u*)kArray1_Data(*outputBuffer);
        k8u* in = (k8u*)kMemory_At(memory, 0);
        kMemCopy(out, in, len);
    }

    kDestroyRef(&memory);
    kDestroyRef(&serializer);

    return kOK;
}
kStatus GdkPartSegmentResult_SerializePartsInArray(kArray1 parts, kArray1* outputBuffer)
{
    if (parts != kNULL && kArray1_Count(parts) > 0 && outputBuffer != kNULL)
    {
        kMemory memory;
        kSerializer serializer;
        k8u* out;
        k8u* in;
        k64u len;

        kMemory_Construct(&memory, kNULL);
        kSerializer_Construct(&serializer, memory, kNULL, kNULL);

        kCheck(kSerializer_WriteSize(serializer, kArray1_Count(parts)));

        for (k32s i = 0; i < kArray1_Count(parts); ++i)
        {
            GdkPartSegmentResult* output = (GdkPartSegmentResult*)kArray1_At(parts, i);
            GdkPartSegmentResult_SerializePart(*output, serializer);
        }

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

kStatus GdkPartSegmentResult_SerializeParts(const std::vector<GdkPartSegmentResultClass>& parts, kArray1* outputBuffer)
{
    if (!parts.empty() && outputBuffer != kNULL)
    {
        kMemory memory;
        kSerializer serializer;
        k8u* out;
        k8u* in;
        k64u len;

        kMemory_Construct(&memory, kNULL);
        kSerializer_Construct(&serializer, memory, kNULL, kNULL);

        kCheck(kSerializer_WriteSize(serializer, parts.size()));

        for (auto part:parts)
        {
            GdkPartSegmentResult_SerializePart(&part, serializer);
        }

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

kStatus GdkPartSegmentResult_DeserializePart(GdkPartSegmentResult output, kSerializer serializer)
{
    // Increase version if a change occurs in the struct, and implement a different reader for each version.
    GdkPartSegmentResultClass* obj = (GdkPartSegmentResultClass*)output;
    k32u version;
    kSize ctrPointCount;

    kCheck(kSerializer_Read32u(serializer, &version));

    switch (version)
    {
    default:
        return kERROR;

    case 1:

        kCheck(kSerializer_Read64f(serializer, &obj->area));
        kCheck(kSerializer_Read64f(serializer, &obj->aspect));
        kCheck(kSerializer_Read64f(serializer, &obj->heightMax));
        kCheck(kSerializer_Read64f(serializer, &obj->angle));

        kCheck(kSerializer_Read32f(serializer, &obj->center.x));
        kCheck(kSerializer_Read32f(serializer, &obj->center.y));
        kCheck(kSerializer_Read32f(serializer, &obj->centerHoriz.x));
        kCheck(kSerializer_Read32f(serializer, &obj->centerHoriz.y));
        kCheck(kSerializer_Read32f(serializer, &obj->centerContourPts.x));
        kCheck(kSerializer_Read32f(serializer, &obj->centerContourPts.y));

        kCheck(kSerializer_Read64f(serializer, &obj->width));
        kCheck(kSerializer_Read64f(serializer, &obj->length));
        kCheck(kSerializer_ReadSize(serializer, &obj->contourPtCount));

        kCheck(kSerializer_Read32f(serializer, &obj->minAreaRectCorners[0].x));
        kCheck(kSerializer_Read32f(serializer, &obj->minAreaRectCorners[0].y));
        kCheck(kSerializer_Read32f(serializer, &obj->minAreaRectCorners[1].x));
        kCheck(kSerializer_Read32f(serializer, &obj->minAreaRectCorners[1].y));
        kCheck(kSerializer_Read32f(serializer, &obj->minAreaRectCorners[2].x));
        kCheck(kSerializer_Read32f(serializer, &obj->minAreaRectCorners[2].y));
        kCheck(kSerializer_Read32f(serializer, &obj->minAreaRectCorners[3].x));
        kCheck(kSerializer_Read32f(serializer, &obj->minAreaRectCorners[3].y));

        kCheck(kSerializer_Read32s(serializer, &obj->boundingBox.x));
        kCheck(kSerializer_Read32s(serializer, &obj->boundingBox.y));
        kCheck(kSerializer_Read32s(serializer, &obj->boundingBox.width));
        kCheck(kSerializer_Read32s(serializer, &obj->boundingBox.height));

        kCheck(kSerializer_ReadSize(serializer, &ctrPointCount));
        for (k32s i = 0; i < ctrPointCount; ++i)
        {
            kPoint64f point;
            kCheck(kSerializer_Read64f(serializer, &point.x));
            kCheck(kSerializer_Read64f(serializer, &point.y));
            obj->contourPoints.push_back(point);
        }
        break;
    }

    return kOK;
}

kStatus GdkPartSegmentResult_DeserializeInternal(GdkPartSegmentResult output, kMemory memory, kAlloc alloc)
{
    GdkPartSegmentResultClass* obj = (GdkPartSegmentResultClass*)output;
    kSerializer serializer;

    kSerializer_Construct(&serializer, memory, kNULL, alloc);

    GdkPartSegmentResult_DeserializePart(output, serializer);

    kDestroyRef(&serializer);
    return kOK;
}
kStatus GdkPartSegmentResult_DeserializeArray(GdkPartSegmentResult output, kArray1 inputBuffer, kAlloc alloc)
{
    kMemory memory;
    kMemory_Construct(&memory, alloc);
    kMemory_Attach(memory, kArray1_Data(inputBuffer), 0, kArray1_Length(inputBuffer), kArray1_Length(inputBuffer));

    GdkPartSegmentResult_DeserializeInternal(output, memory, alloc);

    kDestroyRef(&memory);
    return kOK;
}
kStatus GdkPartSegmentResult_DeserializeBuffer(GdkPartSegmentResult output, void* inputBuffer, kSize length, kAlloc alloc)
{
    kMemory memory;
    kMemory_Construct(&memory, alloc);
    kMemory_Attach(memory, inputBuffer, 0, length, length);

    GdkPartSegmentResult_DeserializeInternal(output, memory, alloc);

    kDestroyRef(&memory);
    return kOK;
}

kStatus GdkPartSegmentResult_DeserializeParts(std::vector<GdkPartSegmentResultClass>& parts, void* inputBuffer, kSize length, kAlloc alloc)
{
    k32s i;
    kSize count = 0;
    kSerializer serializer;

    kMemory memory;
    kMemory_Construct(&memory, alloc);
    kMemory_Attach(memory, inputBuffer, 0, length, length);

    kSerializer_Construct(&serializer, memory, kNULL, alloc);

    kSerializer_ReadSize(serializer, &count);

    for (i = 0; i < count; ++i)
    {
        GdkPartSegmentResultClass output;
        kZero(output);
        GdkPartSegmentResult_DeserializePart(&output, serializer);
        parts.push_back(output);
    }

    kDestroyRef(&serializer);
    kDestroyRef(&memory);

    return kOK;
}

#ifndef GDKAPP

kBool GdkPartSegmentResult_TypeValid(GoGenericMsg genMsg)
{
    if (genMsg)
    {
        k32u utype = GoGenericMsg_UserType(genMsg);
        if (utype == GDK_PART_SEGEMENT_DATA_TYPE)
        {
            return kTRUE;
        }
    }

    return kFALSE;
}

kStatus GdkPartSegmentResult_DeserializeParts(GoGenericMsg genMsg, std::vector<GdkPartSegmentResultClass>& parts)
{
    if (GdkPartSegmentResult_TypeValid(genMsg))
    {
        void* ptr = (void*)GoGenericMsg_BufferData(genMsg);
        GdkPartSegmentResult_DeserializeParts(parts, ptr, GoGenericMsg_BufferSize(genMsg), kNULL);

        return kOK;
    }

    return kERROR;
}

#endif
