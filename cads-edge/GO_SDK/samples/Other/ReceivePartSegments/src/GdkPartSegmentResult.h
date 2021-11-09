/**
* @file    GdkPartSegmentResult.h
* @brief   Declares the GdkPartSegmentResult class.
*
* Copyright (C) 2016-2019 by LMI Technologies Inc.
* Licensed under the MIT License.
* Redistributed files must retain the above copyright notice.
*/
#ifndef GDK_PART_SEGMENT_RESULT_H
#define GDK_PART_SEGMENT_RESULT_H

#include <kApi/kApiDef.h>
#include <vector>

// Include for deserialization
#ifndef GDKAPP
#include <GoSdk/GoSdk.h>
#endif

// Include for type ID
#ifndef GDKAPP
#define GDK_DATA_TYPE_GENERIC_BASE                  (0x80000000)    ///< Generic data base
#else
#include <Gdk/GdkDef.h>
#endif

using namespace std;

const k32u GDK_PART_SEGEMENT_DATA_TYPE = GDK_DATA_TYPE_GENERIC_BASE + 0x00009001;

/**
* @class   GdkPartSegmentResult
* @brief   Represents output for segmented parts.
*/

class GdkPartSegmentResultClass
{
public:
    k64f area;
    k64f aspect;
    k64f heightMax;
    k64f angle;
    kPoint32f center;
    kPoint32f centerHoriz;
    kPoint32f centerContourPts;
    k64f width;
    k64f length;
    kSize contourPtCount;
    kPoint32f minAreaRectCorners[4];
    kRect32s boundingBox;

    std::vector<kPoint64f> contourPoints;

};

typedef GdkPartSegmentResultClass* GdkPartSegmentResult;

#define GdkPartSegmentResult_Cast_(CONTEXT)    (GdkPartSegmentResultClass*) CONTEXT

kStatus GdkPartSegmentResult_SerializeParts(const std::vector<GdkPartSegmentResultClass>& parts, kArray1* outputBuffer);
kStatus GdkPartSegmentResult_DeserializeParts(std::vector<GdkPartSegmentResultClass>& parts, void* inputBuffer, kSize length, kAlloc alloc);


#ifndef GDKAPP
kBool   GdkPartSegmentResult_TypeValid(GoGenericMsg genMsg);
kStatus GdkPartSegmentResult_DeserializeParts(GoGenericMsg genMsg, std::vector<GdkPartSegmentResultClass>& parts);
#endif

#endif
