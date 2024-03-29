/**
 * @file    GoDataTypes.c
 *
 * @internal
 * Copyright (C) 2016-2019 by LMI Technologies Inc.
 * Licensed under the MIT License.
 * Redistributed files must retain the above copyright notice.
 */
#include <GoSdk/Messages/GoDataTypes.h>
#include <GoSdk/Messages/GoHealth.h>
#include <GoSdk/Internal/GoSerializer.h>
#include <GoSdk/GoSdkDef.h>
#include <kApi/Data/kMath.h>
#include <math.h>

kBeginClassEx(Go, GoDataMsg)
    kAddVMethod(GoDataMsg, GoDataMsg, VInit)
    kAddVMethod(GoDataMsg, kObject, VInitClone)
    kAddVMethod(GoDataMsg, kObject, VRelease)
kEndClassEx()

GoFx(kStatus) GoDataMsg_VInit(GoDataMsg msg, kType type, kAlloc alloc)
{
    return kERROR_UNIMPLEMENTED;
}

GoFx(GoDataStep) GoDataMsg_StreamStep(GoDataMsg msg)
{
    kObj(GoDataMsg, msg);

    return obj->streamStep;
}

GoFx(k32s) GoDataMsg_StreamStepId(GoDataMsg msg)
{
    kObj(GoDataMsg, msg);

    return obj->streamStepId;
}

GoFx(kStatus) GoDataMsg_SetStreamStep(GoDataMsg msg, GoDataStep streamStep)
{
    kObj(GoDataMsg, msg);

    obj->streamStep = streamStep;

    return kOK;
}

GoFx(kStatus) GoDataMsg_SetStreamStepId(GoDataMsg msg, k32s streamStepId)
{
    kObj(GoDataMsg, msg);

    obj->streamStepId = streamStepId;

    return kOK;
}

GoFx(kStatus) GoDataMsg_Init(GoDataMsg msg, kType type, GoDataMessageType typeId, kAlloc alloc)
{
    kObjR(GoDataMsg, msg);

    kCheck(kObject_Init(msg, type, alloc));

    obj->typeId = typeId;

    // These are not meaningful for data messages in general. They will be set
    // to meaningful values for messages that use them, but will have these known values
    // for other messages.
    obj->streamStep = -1;
    obj->streamStepId = -1;

    return kOK;
}

GoFx(kStatus) GoDataMsg_VInitClone(GoDataMsg msg, GoDataMsg source, kAlloc alloc)
{
    kObj(GoDataMsg, msg);
    kObjN(GoDataMsg, src, source);

    kCheck(GoDataMsg_Init(msg, kObject_Type(source), src->typeId, alloc));

    obj->streamStep = GoDataMsg_StreamStep(source);
    obj->streamStepId = GoDataMsg_StreamStepId(source);

    return kOK;
}

GoFx(kStatus) GoDataMsg_VRelease(GoDataMsg messsage)
{
    kObj(GoDataMsg, messsage);

    return kObject_VRelease(messsage);
}

GoFx(GoDataMessageType) GoDataMsg_Type(GoDataMsg message)
{
    kObj(GoDataMsg, message);

    return obj->typeId;
}

GoFx(kStatus) GoDataMsg_ReadStreamStepAndId(GoDataMsg msg, kSerializer serializer)
{
    GoDataStep streamStep;
    k32s streamStepId;

    kCheck(kSerializer_Read32s(serializer, &streamStep));
    kCheck(GoDataMsg_SetStreamStep(msg, streamStep));

    kCheck(kSerializer_Read32s(serializer, &streamStepId));
    kCheck(GoDataMsg_SetStreamStepId(msg, streamStepId));

    return kOK;
}


/*
 * GoStamp
 */
kBeginValueEx(Go, GoStamp)
    kAddField(GoStamp, k64u, frameIndex)
    kAddField(GoStamp, k64u, timestamp)
    kAddField(GoStamp, k64s, encoder)
    kAddField(GoStamp, k64s, encoderAtZ)
    kAddField(GoStamp, k64u, status)
    kAddField(GoStamp, k32u, id)
    kAddField(GoStamp, k32u, reserved32u)
    kAddField(GoStamp, k64u, reserved64u)
    kAddField(GoStamp, k64u, ptpTime)
kEndValueEx()

/*
 * GoStampMsg
 */

kBeginClassEx(Go, GoStampMsg)

    // Serialization versions. Serialzier Type Id string must match the sensor's
    // value of the corresonding message type.
    kAddVersion(GoStampMsg, GO_SERIALIZATION_FORMAT_NAME, GO_SERIALIZATION_FORMAT_VERSION, GoSerializerTypeIdStr(GO_COMPACT_MESSAGE_STAMP), WriteV1, ReadV1)
    kAddVersion(GoStampMsg, "kdat6", "4.0.0.0", "GoStampMsg-1", WriteV1, ReadV1)

    //virtual methods
    kAddVMethod(GoStampMsg, GoDataMsg, VInit)
    kAddVMethod(GoStampMsg, kObject, VInitClone)
    kAddVMethod(GoStampMsg, kObject, VRelease)
    kAddVMethod(GoStampMsg, kObject, VSize)

kEndClassEx()

GoFx(kStatus) GoStampMsg_Construct(GoStampMsg* msg, kAlloc allocator)
{
    kAlloc alloc = kAlloc_Fallback(allocator);
    kStatus status;

    kCheck(kAlloc_GetObject(alloc, kTypeOf(GoStampMsg), msg));

    if (!kSuccess(status = GoStampMsg_VInit(*msg, kTypeOf(GoStampMsg), alloc)))
    {
        kAlloc_FreeRef(alloc, msg);
    }

    return status;
}

GoFx(kStatus) GoStampMsg_VInit(GoStampMsg msg, kType type, kAlloc alloc)
{
    kObjR(GoStampMsg, msg);

    kCheck(GoDataMsg_Init(msg, type, GO_DATA_MESSAGE_TYPE_STAMP, alloc));

    obj->stamps = kNULL;
    obj->source = GO_DATA_SOURCE_TOP;

    return kOK;
}

GoFx(kStatus) GoStampMsg_VInitClone(GoStampMsg msg, GoStampMsg source, kAlloc alloc)
{
    kObjR(GoStampMsg, msg);
    kObjNR(GoStampMsg, srcObj, source);
    kStatus status;

    kCheck(GoStampMsg_VInit(msg, kObject_Type(source), alloc));

    kTry
    {
        obj->source = srcObj->source;

        kTest(kObject_Clone(&obj->stamps, srcObj->stamps, alloc));
    }
    kCatch(&status)
    {
        GoStampMsg_VRelease(msg);
        kEndCatch(status);
    }

    return kOK;
}

GoFx(kStatus) GoStampMsg_Allocate(GoStampMsg msg, kSize count)
{
    kObjR(GoStampMsg, msg);

    kCheck(kDestroyRef(&obj->stamps));
    kCheck(kArray1_Construct(&obj->stamps, kTypeOf(GoStamp), count, kObject_Alloc(msg)));

    return kOK;
}

GoFx(kStatus) GoStampMsg_VRelease(GoStampMsg msg)
{
    kObjR(GoStampMsg, msg);

    kCheck(kObject_Destroy(obj->stamps));

    kCheck(kObject_VRelease(msg));

    return kOK;
}

GoFx(kSize) GoStampMsg_VSize(GoStampMsg msg)
{
    kObjR(GoStampMsg, msg);

    return sizeof(GoStampMsgClass) + (kIsNull(obj->stamps) ? 0 : kObject_Size(obj->stamps));
}

GoFx(kStatus) GoStampMsg_WriteV1(GoStampMsg msg, kSerializer serializer)
{
    kObjR(GoStampMsg, msg);
    k32u count = (k32u) kArray1_Count(obj->stamps);
    k32u i;

    kCheck(kSerializer_Write32u(serializer, count));
    kCheck(kSerializer_Write16u(serializer, GO_STAMP_MSG_STAMP_SIZE_1_64));
    kCheck(kSerializer_Write8u(serializer, (k8u) obj->source));
    kCheck(kSerializer_Write8u(serializer, 0));

    for (i = 0; i < count; ++i)
    {
        const GoStamp* stamp = kArray1_AtT(obj->stamps, i, GoStamp);

        kCheck(kSerializer_Write64u(serializer, stamp->frameIndex));
        kCheck(kSerializer_Write64u(serializer, stamp->timestamp));
        kCheck(kSerializer_Write64s(serializer, stamp->encoder));
        kCheck(kSerializer_Write64s(serializer, stamp->encoderAtZ));
        kCheck(kSerializer_Write64u(serializer, stamp->status));
        kCheck(kSerializer_Write32u(serializer, stamp->id));
        kCheck(kSerializer_Write32u(serializer, stamp->reserved32u));
        kCheck(kSerializer_Write64u(serializer, stamp->reserved64u));
        kCheck(kSerializer_Write64u(serializer, stamp->ptpTime));
    }

    return kOK;
}

GoFx(kStatus) GoStampMsg_ReadV1(GoStampMsg msg, kSerializer serializer, kAlloc alloc)
{
    kObjR(GoStampMsg, msg);
    kStatus status;
    k16u stampSize = 0;
    k8u source = 0;
    k8u reserved;
    k32u count = 0;
    k32u i;

    kCheck(GoStampMsg_VInit(msg, kTypeOf(GoStampMsg), alloc));

    kTry
    {
        kTest(kSerializer_Read32u(serializer, &count));
        kTest(kSerializer_Read16u(serializer, &stampSize));
        kTest(kSerializer_Read8u(serializer, &source));
        kTest(kSerializer_Read8u(serializer, &reserved));

        obj->source = (GoDataSource)source;

        kTest(kArray1_Construct(&obj->stamps, kTypeOf(GoStamp), count, alloc));

        for (i = 0; i < count; ++i)
        {
            GoStamp* stamp = kArray1_AtT(obj->stamps, i, GoStamp);

            kTest(kSerializer_Read64u(serializer, &stamp->frameIndex));
            kTest(kSerializer_Read64u(serializer, &stamp->timestamp));
            kTest(kSerializer_Read64s(serializer, &stamp->encoder));
            kTest(kSerializer_Read64s(serializer, &stamp->encoderAtZ));
            kTest(kSerializer_Read64u(serializer, &stamp->status));
            kTest(kSerializer_Read32u(serializer, &stamp->id));
            kTest(kSerializer_Read32u(serializer, &stamp->reserved32u));
            kTest(kSerializer_Read64u(serializer, &stamp->reserved64u));

            // This is more than a little icky, but there's really no way currently
            // to know the size since the writer was not initially using begin/end
            // write.
            // Further, the logic below could be simplified a bit, but this format
            // allows for obvious extension should yet another field be added.
            // Effectively, the size is being used as a "version".
            if (stampSize >= GO_STAMP_MSG_STAMP_SIZE_1_64)
            {
                kTest(kSerializer_Read64u(serializer, &stamp->ptpTime));
            }

            // This comparison should always be against the maximum stamp size handled
            // by this version of the reader.
            if (stampSize > GO_STAMP_MSG_STAMP_SIZE_1_64)
            {
                kTest(kSerializer_AdvanceRead(serializer, stampSize - GO_STAMP_MSG_STAMP_SIZE_1_64));
            }
        }
    }
    kCatch(&status)
    {
        GoStampMsg_VRelease(msg);
        kEndCatch(status);
    }

    return kOK;
}

GoFx(GoDataSource) GoStampMsg_Source(GoStampMsg msg)
{
    return xGoStampMsg_CastRaw(msg)->source;
}

GoFx(kSize) GoStampMsg_Count(GoStampMsg msg)
{
    return kArray1_Count(GoStampMsg_Content_(msg));
}

GoFx(GoStamp*) GoStampMsg_At(GoStampMsg msg, kSize index)
{
    kAssert(index < kArray1_Count(GoStampMsg_Content_(msg)));

    return kArray1_AtT(GoStampMsg_Content_(msg), index, GoStamp);
}


/*
 * GoVideoMsg
 */

kBeginClassEx(Go, GoVideoMsg)

    // Serialization versions. Serialzier Type Id string must match the sensor's
    // value of the corresonding message type.
    kAddVersion(GoVideoMsg, GO_SERIALIZATION_FORMAT_NAME, GO_SERIALIZATION_FORMAT_VERSION, GoSerializerTypeIdStr(GO_COMPACT_MESSAGE_VIDEO), WriteV2, ReadV2)
    kAddVersion(GoVideoMsg, "kdat6", "4.0.0.0", "GoVideoMsg-2", WriteV2, ReadV2)

    //virtual methods
    kAddVMethod(GoVideoMsg, GoDataMsg, VInit)
    kAddVMethod(GoVideoMsg, kObject, VInitClone)
    kAddVMethod(GoVideoMsg, kObject, VRelease)
    kAddVMethod(GoVideoMsg, kObject, VSize)

kEndClassEx()

GoFx(kStatus) GoVideoMsg_Construct(GoVideoMsg* msg, kAlloc allocator)
{
    kAlloc alloc = kAlloc_Fallback(allocator);
    kStatus status;

    kCheck(kAlloc_GetObject(alloc, kTypeOf(GoVideoMsg), msg));

    if (!kSuccess(status = GoVideoMsg_VInit(*msg, kTypeOf(GoVideoMsg), alloc)))
    {
        kAlloc_FreeRef(alloc, msg);
    }

    return status;
}

GoFx(kStatus) GoVideoMsg_VInit(GoVideoMsg msg, kType type, kAlloc alloc)
{
    kObjR(GoVideoMsg, msg);

    kCheck(GoDataMsg_Init(msg, type, GO_DATA_MESSAGE_TYPE_VIDEO, alloc));

    obj->source = GO_DATA_SOURCE_TOP;
    obj->cameraIndex = 0;
    kZero(obj->content);
    obj->exposureIndex = 0;
    obj->exposure = 0;
    obj->isFlippedX = kFALSE;
    obj->isFlippedY = kFALSE;

    return kOK;
}

GoFx(kStatus) GoVideoMsg_VInitClone(GoVideoMsg msg, GoVideoMsg source, kAlloc alloc)
{
    kObjR(GoVideoMsg, msg);
    kObjNR(GoVideoMsg, srcObj, source);
    kStatus status;

    kCheck(GoVideoMsg_VInit(msg, kObject_Type(source), alloc));

    kTry
    {
        obj->source = srcObj->source;
        obj->exposureIndex = srcObj->exposureIndex;
        obj->exposure = srcObj->exposure;
        obj->isFlippedX = srcObj->isFlippedX;
        obj->isFlippedY = srcObj->isFlippedY;
        obj->base.streamStep = GoDataMsg_StreamStep(source);
        obj->base.streamStepId = GoDataMsg_StreamStepId(source);

        kTest(kObject_Clone(&obj->content, srcObj->content, alloc));
    }
    kCatch(&status)
    {
        GoVideoMsg_VRelease(msg);
        kEndCatch(status);
    }

    return kOK;
}

GoFx(kStatus) GoVideoMsg_Allocate(GoVideoMsg msg, kType pixelType, kSize width, kSize height)
{
    kObjR(GoVideoMsg, msg);

    kCheck(kDestroyRef(&obj->content));
    kCheck(kImage_Construct(&obj->content, pixelType, width, height, kObject_Alloc(msg)));

    return kOK;
}

GoFx(kSize) GoVideoMsg_VSize(GoVideoMsg msg)
{
    kObjR(GoVideoMsg, msg);

    return sizeof(GoVideoMsgClass) + (kIsNull(obj->content) ? 0 : kObject_Size(obj->content));
}

GoFx(kStatus) GoVideoMsg_VRelease(GoVideoMsg msg)
{
    kObjR(GoVideoMsg, msg);

    kCheck(kObject_Destroy(obj->content));

    kCheck(kObject_VRelease(msg));

    return kOK;
}

GoFx(kStatus) GoVideoMsg_WriteV2(GoVideoMsg msg, kSerializer serializer)
{
    kObjR(GoVideoMsg, msg);
    k32u width = (k32u) kImage_Width(obj->content);
    k32u height = (k32u) kImage_Height(obj->content);
    k32u i;
    kSize rowSize;

    kCheck(kSerializer_BeginWrite(serializer, kTypeOf(k16u), kFALSE));

    kCheck(kSerializer_Write32u(serializer, height));
    kCheck(kSerializer_Write32u(serializer, width));
    kCheck(kSerializer_Write8u(serializer, (k8u) kImage_PixelSize(obj->content)));
    kCheck(kSerializer_Write8u(serializer, (k8u) kImage_PixelFormat(obj->content)));
    kCheck(kSerializer_Write8u(serializer, (k8u) kImage_Cfa(obj->content)));
    kCheck(kSerializer_Write8u(serializer, (k8u) obj->source));
    kCheck(kSerializer_Write8u(serializer, (k8u) obj->cameraIndex));
    kCheck(kSerializer_Write8u(serializer, (k8u) obj->exposureIndex));
    kCheck(kSerializer_Write32u(serializer, obj->exposure));
    kCheck(kSerializer_Write8u(serializer, (k8u) obj->isFlippedX));
    kCheck(kSerializer_Write8u(serializer, (k8u) obj->isFlippedY));
    kCheck(kSerializer_Write32s(serializer, GoDataMsg_StreamStep(msg)));
    kCheck(kSerializer_Write32s(serializer, GoDataMsg_StreamStepId(msg)));


    kCheck(kSerializer_EndWrite(serializer));

    if (kImage_PixelType(obj->content) == kTypeOf(k8u))
    {
        rowSize = width * kImage_PixelSize(obj->content);

        for (i = 0; i < height; ++i)
        {
            void* pixels = kImage_RowAtT(obj->content, i, k8u);
            kCheck(kSerializer_WriteByteArray(serializer, pixels, rowSize));
        }
    }
    else if (kImage_PixelType(obj->content) == kTypeOf(kRgb))
    {
        for (i = 0; i < height; ++i)
        {
            void* pixels = kImage_RowAtT(obj->content, i, kRgb);
            kCheck(kSerializer_Write8uArray(serializer, pixels, 4*width));
        }
    }
    else
    {
        return kERROR_UNIMPLEMENTED;
    }

    return kOK;
}

GoFx(kStatus) GoVideoMsg_ReadV2(GoVideoMsg msg, kSerializer serializer, kAlloc alloc)
{
    kObjR(GoVideoMsg, msg);
    kStatus status;
    k32u width, height;
    k8u pixelSize, pixelFormat, colorMask, source, cameraIndex, exposureIndex, isFlippedX, isFlippedY;
    k32u i, exposure;
    kSize rowSize;

    kCheck(GoVideoMsg_VInit(msg, kTypeOf(GoVideoMsg), alloc));

    kTry
    {
        // Read and store the message attributes data length ("attrSize")
        // to determine how many attributes to read.
        kTest(kSerializer_BeginRead(serializer, kTypeOf(k16u), kFALSE));

        kTest(kSerializer_Read32u(serializer, &height));
        kTest(kSerializer_Read32u(serializer, &width));
        kTest(kSerializer_Read8u(serializer, &pixelSize));
        kTest(kSerializer_Read8u(serializer, &pixelFormat));
        kTest(kSerializer_Read8u(serializer, &colorMask));
        kTest(kSerializer_Read8u(serializer, &source));
        kTest(kSerializer_Read8u(serializer, &cameraIndex));
        kTest(kSerializer_Read8u(serializer, &exposureIndex));
        kTest(kSerializer_Read32u(serializer, &exposure));
        kTest(kSerializer_Read8u(serializer, &isFlippedX));
        kTest(kSerializer_Read8u(serializer, &isFlippedY));

        kTest(GoVideoMsg_ReadV2AttrProtoVer2(msg, serializer));

        // Skip over attributes this version of the SDK protocol does not
        // know about.
        kTest(kSerializer_EndRead(serializer));

        if ((pixelFormat == kPIXEL_FORMAT_8BPP_GREYSCALE) || (pixelFormat == kPIXEL_FORMAT_8BPP_CFA))
        {
            kTest(kImage_Construct(&obj->content, kTypeOf(k8u), width, height, alloc));

            rowSize = pixelSize * width;

            for (i = 0; i < height; ++i)
            {
                void* pixels = kImage_RowAtT(obj->content, i, k8u);
                kTest(kSerializer_ReadByteArray(serializer, pixels, rowSize));
            }
        }
        else if (pixelFormat == kPIXEL_FORMAT_8BPC_BGRX)
        {
            kTest(kImage_Construct(&obj->content, kTypeOf(kRgb), width, height, alloc));

            for (i = 0; i < height; ++i)
            {
                void* pixels = kImage_RowAtT(obj->content, i, kRgb);
                kTest(kSerializer_Read8uArray(serializer, pixels, 4*width));
            }
        }
        else
        {
            kThrow(kERROR_UNIMPLEMENTED);
        }

        kTest(kImage_SetPixelFormat(obj->content, (kPixelFormat)pixelFormat));
        kTest(kImage_SetCfa(obj->content, (kCfa)colorMask));
        obj->cameraIndex = cameraIndex;
        obj->source = (GoDataSource)source;
        obj->exposureIndex = exposureIndex;
        obj->exposure = exposure;
        obj->isFlippedX = (kBool)isFlippedX;
        obj->isFlippedY = (kBool)isFlippedY;
    }
    kCatch(&status)
    {
        GoVideoMsg_VRelease(msg);
        kEndCatch(status);
    }

    return kOK;
}

GoFx(kStatus) GoVideoMsg_ReadV2AttrProtoVer2(GoVideoMsg msg, kSerializer serializer)
{
    if (!kSerializer_ReadCompleted(serializer))
    {
        return GoDataMsg_ReadStreamStepAndId(msg, serializer);
    }

    return kOK;
}

GoFx(GoDataSource) GoVideoMsg_Source(GoVideoMsg msg)
{
    return xGoVideoMsg_CastRaw(msg)->source;
}

GoFx(kSize) GoVideoMsg_CameraIndex(GoVideoMsg msg)
{
    return xGoVideoMsg_CastRaw(msg)->cameraIndex;
}

GoFx(kSize) GoVideoMsg_Width(GoVideoMsg msg)
{
    return kImage_Width(GoVideoMsg_Content_(msg));
}

GoFx(kSize) GoVideoMsg_Height(GoVideoMsg msg)
{
    return kImage_Height(GoVideoMsg_Content_(msg));
}

GoFx(GoPixelType) GoVideoMsg_PixelType(GoVideoMsg msg)
{
    kType pixelType = kImage_PixelType(GoVideoMsg_Content_(msg));
    GoPixelType typeToReturn = GO_PIXEL_TYPE_UNKNOWN;

    if (pixelType == kTypeOf(k8u))
    {
        typeToReturn = GO_PIXEL_TYPE_8U;
    }
    else if (pixelType == kTypeOf(kRgb))
    {
        typeToReturn = GO_PIXEL_TYPE_RGB;
    }

    return typeToReturn;
}

GoFx(kSize) GoVideoMsg_PixelSize(GoVideoMsg msg)
{
    return kImage_PixelSize(GoVideoMsg_Content_(msg));
}

GoFx(kPixelFormat) GoVideoMsg_PixelFormat(GoVideoMsg msg)
{
    return kImage_PixelFormat(GoVideoMsg_Content_(msg));
}

GoFx(kCfa) GoVideoMsg_Cfa(GoVideoMsg msg)
{
    return kImage_Cfa(GoVideoMsg_Content_(msg));
}

GoFx(void*) GoVideoMsg_RowAt(GoVideoMsg msg, kSize rowIndex)
{
    kAssert(rowIndex < GoVideoMsg_Height(msg));

    return kImage_RowAt(GoVideoMsg_Content_(msg), rowIndex);
}

GoFx(kSize) GoVideoMsg_ExposureIndex(GoVideoMsg msg)
{
    return xGoVideoMsg_CastRaw(msg)->exposureIndex;
}

GoFx(k32u) GoVideoMsg_Exposure(GoVideoMsg msg)
{
    return xGoVideoMsg_CastRaw(msg)->exposure;
}

GoFx(kBool) GoVideoMsg_IsFlippedX(GoVideoMsg msg)
{
    return xGoVideoMsg_CastRaw(msg)->isFlippedX;
}

GoFx(kBool) GoVideoMsg_IsFlippedY(GoVideoMsg msg)
{
    return xGoVideoMsg_CastRaw(msg)->isFlippedY;
}

/*
 * GoRangeMsg
 */

kBeginClassEx(Go, GoRangeMsg)

    // Serialization versions. Serialzier Type Id string must match the sensor's
    // value of the corresonding message type.
    kAddVersion(GoRangeMsg, GO_SERIALIZATION_FORMAT_NAME, GO_SERIALIZATION_FORMAT_VERSION, GoSerializerTypeIdStr(GO_COMPACT_MESSAGE_RANGE), WriteV3, ReadV3)
    kAddVersion(GoRangeMsg, "kdat6", "4.0.0.0", "GoRangeMsg-3", WriteV3, ReadV3)

    //virtual methods
    kAddVMethod(GoRangeMsg, GoDataMsg, VInit)
    kAddVMethod(GoRangeMsg, kObject, VInitClone)
    kAddVMethod(GoRangeMsg, kObject, VRelease)
    kAddVMethod(GoRangeMsg, kObject, VSize)

kEndClassEx()

GoFx(kStatus) GoRangeMsg_Construct(GoRangeMsg* msg, kAlloc allocator)
{
    kAlloc alloc = kAlloc_Fallback(allocator);
    kStatus status;

    kCheck(kAlloc_GetObject(alloc, kTypeOf(GoRangeMsg), msg));

    if (!kSuccess(status = GoRangeMsg_VInit(*msg, kTypeOf(GoRangeMsg), alloc)))
    {
        kAlloc_FreeRef(alloc, msg);
    }

    return status;
}

GoFx(kStatus) GoRangeMsg_VInit(GoRangeMsg msg, kType type, kAlloc alloc)
{
    kObjR(GoRangeMsg, msg);

    kCheck(GoDataMsg_Init(msg, type, GO_DATA_MESSAGE_TYPE_RANGE, alloc));

    obj->source = GO_DATA_SOURCE_TOP;
    obj->zResolution = 0;
    obj->zOffset = 0;
    kZero(obj->content);
    obj->exposure = 0;

    return kOK;
}

GoFx(kStatus) GoRangeMsg_VInitClone(GoRangeMsg msg, GoRangeMsg source, kAlloc alloc)
{
    kObjR(GoRangeMsg, msg);
    kObjNR(GoRangeMsg, srcObj, source);
    kStatus status;

    kCheck(GoRangeMsg_VInit(msg, kObject_Type(source), alloc));

    kTry
    {
        obj->source = srcObj->source;
        obj->zResolution = srcObj->zResolution;
        obj->zOffset = srcObj->zOffset;
        obj->exposure = srcObj->exposure;
        obj->base.streamStep = GoDataMsg_StreamStep(source);
        obj->base.streamStepId = GoDataMsg_StreamStepId(source);

        kTest(kObject_Clone(&obj->content, srcObj->content, alloc));
    }
    kCatch(&status)
    {
        GoRangeMsg_VRelease(msg);
        kEndCatch(status);
    }

    return kOK;
}

GoFx(kStatus) GoRangeMsg_Allocate(GoRangeMsg msg, kSize count)
{
    kObjR(GoRangeMsg, msg);

    kCheck(kDestroyRef(&obj->content));
    kCheck(kArray1_Construct(&obj->content, kTypeOf(k16s), count, kObject_Alloc(msg)));

    return kOK;
}

GoFx(kSize) GoRangeMsg_VSize(GoRangeMsg msg)
{
    kObjR(GoRangeMsg, msg);

    return sizeof(GoRangeMsgClass) + (kIsNull(obj->content) ? 0 : kObject_Size(obj->content));
}

GoFx(kStatus) GoRangeMsg_VRelease(GoRangeMsg msg)
{
    kObjR(GoRangeMsg, msg);

    kCheck(kObject_Destroy(obj->content));

    kCheck(kObject_VRelease(msg));

    return kOK;
}

GoFx(kStatus) GoRangeMsg_WriteV3(GoRangeMsg msg, kSerializer serializer)
{
    kObjR(GoRangeMsg, msg);
    k32u count = (k32u) kArray1_Length(obj->content);

    kCheck(kSerializer_BeginWrite(serializer, kTypeOf(k16u), kFALSE));

    kCheck(kSerializer_Write32u(serializer, count));
    kCheck(kSerializer_Write32u(serializer, obj->zResolution));
    kCheck(kSerializer_Write32s(serializer, obj->zOffset));
    kCheck(kSerializer_Write8u(serializer, (k8u) obj->source));
    kCheck(kSerializer_Write32u(serializer, obj->exposure));
    kCheck(kSerializer_Write8u(serializer, (k8u) 0));
    kCheck(kSerializer_Write8u(serializer, (k8u) 0));
    kCheck(kSerializer_Write8u(serializer, (k8u) 0));
    kCheck(kSerializer_Write32s(serializer, GoDataMsg_StreamStep(msg)));
    kCheck(kSerializer_Write32s(serializer, GoDataMsg_StreamStepId(msg)));

    kCheck(kSerializer_EndWrite(serializer));

    kCheck(kSerializer_Write16sArray(serializer, kArray1_DataT(obj->content, k16s), kArray1_Count(obj->content)));

    return kOK;
}

GoFx(kStatus) GoRangeMsg_ReadV3(GoRangeMsg msg, kSerializer serializer, kAlloc alloc)
{
    kObjR(GoRangeMsg, msg);
    kStatus status;
    k32u count;
    k8u source, reserved;

    kCheck(GoRangeMsg_VInit(msg, kTypeOf(GoRangeMsg), alloc));

    kTry
    {
        // Read and store the message attributes data length ("attrSize")
        // to determine how many attributes to read.
        kTest(kSerializer_BeginRead(serializer, kTypeOf(k16u), kFALSE));

        kTest(kSerializer_Read32u(serializer, &count));
        kTest(kSerializer_Read32u(serializer, &obj->zResolution));
        kTest(kSerializer_Read32s(serializer, &obj->zOffset));
        kTest(kSerializer_Read8u(serializer, &source));
        kTest(kSerializer_Read32u(serializer, &obj->exposure));
        kTest(kSerializer_Read8u(serializer, &reserved));
        kTest(kSerializer_Read8u(serializer, &reserved));
        kTest(kSerializer_Read8u(serializer, &reserved));

        kTest(GoRangeMsg_ReadV3AttrProtoVer2(msg, serializer));

        // Skip over attributes this version of the SDK protocol does not
        // know about.
        kTest(kSerializer_EndRead(serializer));

        kTest(kArray1_Construct(&obj->content, kTypeOf(k16s), count, alloc));
        kTest(kSerializer_Read16sArray(serializer, kArray1_DataT(obj->content, k16s), kArray1_Count(obj->content)));

        obj->source = (GoDataSource)source;
    }
    kCatch(&status)
    {
        GoRangeMsg_VRelease(msg);
        kEndCatch(status);
    }

    return kOK;
}

GoFx(kStatus) GoRangeMsg_ReadV3AttrProtoVer2(GoRangeMsg msg, kSerializer serializer)
{
    if (!kSerializer_ReadCompleted(serializer))
    {
        return GoDataMsg_ReadStreamStepAndId(msg, serializer);
    }

    return kOK;
}

GoFx(GoDataSource) GoRangeMsg_Source(GoRangeMsg msg)
{
    return xGoRangeMsg_CastRaw(msg)->source;
}

GoFx(kSize) GoRangeMsg_Count(GoRangeMsg msg)
{
    return kArray1_Length(GoRangeMsg_Content_(msg));
}

GoFx(k32u) GoRangeMsg_ZResolution(GoRangeMsg msg)
{
    return xGoRangeMsg_CastRaw(msg)->zResolution;
}

GoFx(k32s) GoRangeMsg_ZOffset(GoRangeMsg msg)
{
    return xGoRangeMsg_CastRaw(msg)->zOffset;
}

GoFx(k16s*) GoRangeMsg_At(GoRangeMsg msg, kSize index)
{
    kAssert(index < GoRangeMsg_Count(msg));

    return kArray1_AtT(GoRangeMsg_Content_(msg), index, k16s);
}

GoFx(k32u) GoRangeMsg_Exposure(GoRangeMsg msg)
{
    return xGoRangeMsg_CastRaw(msg)->exposure;
}

/*
 * GoRangeIntensityMsg
 */

kBeginClassEx(Go, GoRangeIntensityMsg)

    // Serialization versions. Serialzier Type Id string must match the sensor's
    // value of the corresonding message type.
    kAddVersion(GoRangeIntensityMsg, GO_SERIALIZATION_FORMAT_NAME, GO_SERIALIZATION_FORMAT_VERSION, GoSerializerTypeIdStr(GO_COMPACT_MESSAGE_RANGE_INTENSITY), WriteV4, ReadV4)
    kAddVersion(GoRangeIntensityMsg, "kdat6", "4.0.0.0", "GoRangeIntensityMsg-4", WriteV4, ReadV4)

    //virtual methods
    kAddVMethod(GoRangeIntensityMsg, GoDataMsg, VInit)
    kAddVMethod(GoRangeIntensityMsg, kObject, VInitClone)
    kAddVMethod(GoRangeIntensityMsg, kObject, VRelease)
    kAddVMethod(GoRangeIntensityMsg, kObject, VSize)

kEndClassEx()

GoFx(kStatus) GoRangeIntensityMsg_Construct(GoRangeIntensityMsg* msg, kAlloc allocator)
{
    kAlloc alloc = kAlloc_Fallback(allocator);
    kStatus status;

    kCheck(kAlloc_GetObject(alloc, kTypeOf(GoRangeIntensityMsg), msg));

    if (!kSuccess(status = GoRangeIntensityMsg_VInit(*msg, kTypeOf(GoRangeIntensityMsg), alloc)))
    {
        kAlloc_FreeRef(alloc, msg);
    }

    return status;
}

GoFx(kStatus) GoRangeIntensityMsg_VInit(GoRangeIntensityMsg msg, kType type, kAlloc alloc)
{
    kObjR(GoRangeIntensityMsg, msg);

    kCheck(GoDataMsg_Init(msg, type, GO_DATA_MESSAGE_TYPE_RANGE_INTENSITY, alloc));

    obj->source = GO_DATA_SOURCE_TOP;
    kZero(obj->content);
    obj->exposure = 0;

    return kOK;
}

GoFx(kStatus) GoRangeIntensityMsg_VInitClone(GoRangeIntensityMsg msg, GoRangeIntensityMsg source, kAlloc alloc)
{
    kObjR(GoRangeIntensityMsg, msg);
    kObjNR(GoRangeIntensityMsg, srcObj, source);
    kStatus status;

    kCheck(GoRangeIntensityMsg_VInit(msg, kObject_Type(source), alloc));

    kTry
    {
        obj->source = srcObj->source;
        obj->exposure = srcObj->exposure;
        obj->base.streamStep = GoDataMsg_StreamStep(source);
        obj->base.streamStepId = GoDataMsg_StreamStepId(source);

        kTest(kObject_Clone(&obj->content, srcObj->content, alloc));
    }
    kCatch(&status)
    {
        GoRangeIntensityMsg_VRelease(msg);
        kEndCatch(status);
    }

    return kOK;
}

GoFx(kStatus) GoRangeIntensityMsg_Allocate(GoRangeIntensityMsg msg, kSize count)
{
    kObjR(GoRangeIntensityMsg, msg);

    kCheck(kDestroyRef(&obj->content));
    kCheck(kArray1_Construct(&obj->content, kTypeOf(k8u), count, kObject_Alloc(msg)));

    return kOK;
}

GoFx(kSize) GoRangeIntensityMsg_VSize(GoRangeIntensityMsg msg)
{
    kObjR(GoRangeIntensityMsg, msg);

    return sizeof(GoRangeIntensityMsgClass) + (kIsNull(obj->content) ? 0 : kObject_Size(obj->content));
}

GoFx(kStatus) GoRangeIntensityMsg_VRelease(GoRangeIntensityMsg msg)
{
    kObjR(GoRangeIntensityMsg, msg);

    kCheck(kObject_Destroy(obj->content));

    kCheck(kObject_VRelease(msg));

    return kOK;
}

GoFx(kStatus) GoRangeIntensityMsg_WriteV4(GoRangeIntensityMsg msg, kSerializer serializer)
{
    kObjR(GoRangeIntensityMsg, msg);
    k32u count = (k32u) kArray1_Length(obj->content);

    kCheck(kSerializer_BeginWrite(serializer, kTypeOf(k16u), kFALSE));

    kCheck(kSerializer_Write32u(serializer, count));
    kCheck(kSerializer_Write8u(serializer, (k8u) obj->source));
    kCheck(kSerializer_Write32u(serializer, obj->exposure));
    kCheck(kSerializer_Write8u(serializer, 0));
    kCheck(kSerializer_Write8u(serializer, 0));
    kCheck(kSerializer_Write8u(serializer, 0));
    kCheck(kSerializer_Write32s(serializer, GoDataMsg_StreamStep(msg)));
    kCheck(kSerializer_Write32s(serializer, GoDataMsg_StreamStepId(msg)));

    kCheck(kSerializer_EndWrite(serializer));

    kCheck(kSerializer_Write8uArray(serializer, kArray1_DataT(obj->content, k8u), kArray1_Count(obj->content)));

    return kOK;
}

GoFx(kStatus) GoRangeIntensityMsg_ReadV4(GoRangeIntensityMsg msg, kSerializer serializer, kAlloc alloc)
{
    kObjR(GoRangeIntensityMsg, msg);
    kStatus status;
    k32u count;
    k8u source, reserved;

    kCheck(GoRangeIntensityMsg_VInit(msg, kTypeOf(GoRangeIntensityMsg), alloc));

    kTry
    {
        // Read and store the message attributes data length ("attrSize")
        // to determine how many attributes to read.
        kTest(kSerializer_BeginRead(serializer, kTypeOf(k16u), kFALSE));

        kTest(kSerializer_Read32u(serializer, &count));
        kTest(kSerializer_Read8u(serializer, &source));
        kTest(kSerializer_Read32u(serializer, &obj->exposure));
        kTest(kSerializer_Read8u(serializer, &reserved));
        kTest(kSerializer_Read8u(serializer, &reserved));
        kTest(kSerializer_Read8u(serializer, &reserved));

        kTest(GoRangeIntensityMsg_ReadV4AttrProtoVer2(msg, serializer));

        // Skip over attributes this version of the SDK protocol does not
        // know about.
        kTest(kSerializer_EndRead(serializer));

        kTest(kArray1_Construct(&obj->content, kTypeOf(k8u), count, alloc));
        kTest(kSerializer_Read8uArray(serializer, kArray1_DataT(obj->content, k8u), kArray1_Count(obj->content)));

        obj->source = (GoDataSource)source;
    }
    kCatch(&status)
    {
        GoRangeIntensityMsg_VRelease(msg);
        kEndCatch(status);
    }

    return kOK;
}

GoFx(kStatus) GoRangeIntensityMsg_ReadV4AttrProtoVer2(GoRangeIntensityMsg msg, kSerializer serializer)
{
    if (!kSerializer_ReadCompleted(serializer))
    {
        return GoDataMsg_ReadStreamStepAndId(msg, serializer);
    }

    return kOK;
}

GoFx(GoDataSource) GoRangeIntensityMsg_Source(GoRangeIntensityMsg msg)
{
    return xGoRangeIntensityMsg_CastRaw(msg)->source;
}

GoFx(kSize) GoRangeIntensityMsg_Count(GoRangeIntensityMsg msg)
{
    return kArray1_Length(GoRangeIntensityMsg_Content_(msg));
}

GoFx(k8u*) GoRangeIntensityMsg_At(GoRangeIntensityMsg msg, kSize index)
{
    kAssert(index < GoRangeIntensityMsg_Count(msg));

    return kArray1_AtT(GoRangeIntensityMsg_Content_(msg), index, k8u);
}

GoFx(k32u) GoRangeIntensityMsg_Exposure(GoRangeIntensityMsg msg)
{
    return xGoRangeIntensityMsg_CastRaw(msg)->exposure;
}

/*
 * GoProfilePointCloudMsg
 */

kBeginClassEx(Go, GoProfilePointCloudMsg)

    // Serialization versions. Serialzier Type Id string must match the sensor's
    // value of the corresonding message type.
    kAddVersion(GoProfilePointCloudMsg, GO_SERIALIZATION_FORMAT_NAME, GO_SERIALIZATION_FORMAT_VERSION, GoSerializerTypeIdStr(GO_COMPACT_MESSAGE_PROFILE_POINT_CLOUD), WriteV5, ReadV5)
    kAddVersion(GoProfilePointCloudMsg, "kdat6", "4.0.0.0", "GoProfilePointCloudMsg-5", WriteV5, ReadV5)

    //virtual methods
    kAddVMethod(GoProfilePointCloudMsg, GoDataMsg, VInit)
    kAddVMethod(GoProfilePointCloudMsg, kObject, VInitClone)
    kAddVMethod(GoProfilePointCloudMsg, kObject, VRelease)
    kAddVMethod(GoProfilePointCloudMsg, kObject, VSize)

kEndClassEx()

GoFx(kStatus) GoProfilePointCloudMsg_Construct(GoProfilePointCloudMsg* msg, kAlloc allocator)
{
    kAlloc alloc = kAlloc_Fallback(allocator);
    kStatus status;

    kCheck(kAlloc_GetObject(alloc, kTypeOf(GoProfilePointCloudMsg), msg));

    if (!kSuccess(status = GoProfilePointCloudMsg_VInit(*msg, kTypeOf(GoProfilePointCloudMsg), alloc)))
    {
        kAlloc_FreeRef(alloc, msg);
    }

    return status;
}

GoFx(kStatus) GoProfilePointCloudMsg_VInit(GoProfilePointCloudMsg msg, kType type, kAlloc alloc)
{
    kObjR(GoProfilePointCloudMsg, msg);

    kCheck(GoDataMsg_Init(msg, type, GO_DATA_MESSAGE_TYPE_PROFILE_POINT_CLOUD, alloc));

    obj->source = GO_DATA_SOURCE_TOP;
    obj->xResolution = 0;
    obj->zResolution = 0;
    obj->xOffset = 0;
    obj->zOffset = 0;
    kZero(obj->content);
    obj->exposure = 0;
    obj->cameraIndex = 0;

    return kOK;
}

GoFx(kStatus) GoProfilePointCloudMsg_VInitClone(GoProfilePointCloudMsg msg, GoProfilePointCloudMsg source, kAlloc alloc)
{
    kObjR(GoProfilePointCloudMsg, msg);
    kObjNR(GoProfilePointCloudMsg, srcObj, source);
    kStatus status;

    kCheck(GoProfilePointCloudMsg_VInit(msg, kObject_Type(source), alloc));

    kTry
    {
        obj->source = srcObj->source;
        obj->xResolution = srcObj->xResolution;
        obj->zResolution = srcObj->zResolution;
        obj->xOffset = srcObj->xOffset;
        obj->zOffset = srcObj->zOffset;
        obj->exposure = srcObj->exposure;
        obj->cameraIndex = srcObj->cameraIndex;
        obj->base.streamStep = GoDataMsg_StreamStep(source);
        obj->base.streamStepId = GoDataMsg_StreamStepId(source);

        kTest(kObject_Clone(&obj->content, srcObj->content, alloc));
    }
    kCatch(&status)
    {
        GoProfilePointCloudMsg_VRelease(msg);
        kEndCatch(status);
    }

    return kOK;
}

GoFx(kStatus) GoProfilePointCloudMsg_Allocate(GoProfilePointCloudMsg msg, kSize count, kSize width)
{
    kObjR(GoProfilePointCloudMsg, msg);

    kCheck(kDestroyRef(&obj->content));
    kCheck(kArray2_Construct(&obj->content, kTypeOf(kPoint16s), count, width, kObject_Alloc(msg)));

    return kOK;
}

GoFx(kSize) GoProfilePointCloudMsg_VSize(GoProfilePointCloudMsg msg)
{
    kObjR(GoProfilePointCloudMsg, msg);

    return sizeof(GoProfilePointCloudMsgClass) + (kIsNull(obj->content) ? 0 : kObject_Size(obj->content));
}

GoFx(kStatus) GoProfilePointCloudMsg_VRelease(GoProfilePointCloudMsg msg)
{
    kObjR(GoProfilePointCloudMsg, msg);

    kCheck(kObject_Destroy(obj->content));

    kCheck(kObject_VRelease(msg));

    return kOK;
}

GoFx(kStatus) GoProfilePointCloudMsg_WriteV5(GoProfilePointCloudMsg msg, kSerializer serializer)
{
    kObjR(GoProfilePointCloudMsg, msg);
    k32u count = (k32u) kArray2_Length(obj->content, 0);
    k32u width = (k32u) kArray2_Length(obj->content, 1);
    const kPoint16s* points;

    kCheck(kSerializer_BeginWrite(serializer, kTypeOf(k16u), kFALSE));

    kCheck(kSerializer_Write32u(serializer, count));
    kCheck(kSerializer_Write32u(serializer, width));
    kCheck(kSerializer_Write32u(serializer, obj->xResolution));
    kCheck(kSerializer_Write32u(serializer, obj->zResolution));
    kCheck(kSerializer_Write32s(serializer, obj->xOffset));
    kCheck(kSerializer_Write32s(serializer, obj->zOffset));
    kCheck(kSerializer_Write8u(serializer, (k8u) obj->source));
    kCheck(kSerializer_Write32u(serializer, obj->exposure));
    kCheck(kSerializer_Write8u(serializer, obj->cameraIndex));
    kCheck(kSerializer_Write8u(serializer, (k8u) 0));
    kCheck(kSerializer_Write8u(serializer, (k8u) 0));
    kCheck(kSerializer_Write32s(serializer, GoDataMsg_StreamStep(msg)));
    kCheck(kSerializer_Write32s(serializer, GoDataMsg_StreamStepId(msg)));

    kCheck(kSerializer_EndWrite(serializer));

    points = kArray2_DataT(obj->content, kPoint16s);

    // Treat the kPoint16s array as an array of 16s.
    kCheck(kSerializer_Write16sArray(serializer, (const k16s*)points, count * width * 2));

    return kOK;
}

GoFx(kStatus) GoProfilePointCloudMsg_ReadV5(GoProfilePointCloudMsg msg, kSerializer serializer, kAlloc alloc)
{
    kObjR(GoProfilePointCloudMsg, msg);
    kStatus status;
    k32u count, width;
    k8u source, reserved;
    const kPoint16s* points;

    kCheck(GoProfilePointCloudMsg_VInit(msg, kTypeOf(GoProfilePointCloudMsg), alloc));

    kTry
    {
        // Read and store the message attributes data length ("attrSize")
        // to determine how many attributes to read.
        kTest(kSerializer_BeginRead(serializer, kTypeOf(k16u), kFALSE));

        kTest(kSerializer_Read32u(serializer, &count));
        kTest(kSerializer_Read32u(serializer, &width));
        kTest(kSerializer_Read32u(serializer, &obj->xResolution));
        kTest(kSerializer_Read32u(serializer, &obj->zResolution));
        kTest(kSerializer_Read32s(serializer, &obj->xOffset));
        kTest(kSerializer_Read32s(serializer, &obj->zOffset));
        kTest(kSerializer_Read8u(serializer, &source));
        kTest(kSerializer_Read32u(serializer, &obj->exposure));
        kTest(kSerializer_Read8u(serializer, &obj->cameraIndex));
        kTest(kSerializer_Read8u(serializer, &reserved));
        kTest(kSerializer_Read8u(serializer, &reserved));

        kTest(GoProfilePointCloudMsg_ReadV5AttrProtoVer2(msg, serializer));

        // Skip over attributes this version of the SDK protocol does not
        // know about.
        kTest(kSerializer_EndRead(serializer));

        kTest(kArray2_Construct(&obj->content, kTypeOf(kPoint16s), count, width, alloc));

        points = kArray2_DataT(obj->content, kPoint16s);

        // Treat the kPoint16s array as an array of 16s.
        kTest(kSerializer_Read16sArray(serializer, (k16s*)points, count * width * 2));

        obj->source = (GoDataSource)source;
    }
    kCatch(&status)
    {
        GoProfilePointCloudMsg_VRelease(msg);
        kEndCatch(status);
    }

    return kOK;
}

GoFx(kStatus) GoProfilePointCloudMsg_ReadV5AttrProtoVer2(GoProfilePointCloudMsg msg, kSerializer serializer)
{
    if (!kSerializer_ReadCompleted(serializer))
    {
        return GoDataMsg_ReadStreamStepAndId(msg, serializer);
    }

    return kOK;
}

GoFx(GoDataSource) GoProfilePointCloudMsg_Source(GoProfilePointCloudMsg msg)
{
    return xGoProfilePointCloudMsg_CastRaw(msg)->source;
}

GoFx(kSize) GoProfilePointCloudMsg_Count(GoProfilePointCloudMsg msg)
{
    return kArray2_Length(GoProfilePointCloudMsg_Content_(msg), 0);
}

GoFx(kSize) GoProfilePointCloudMsg_Width(GoProfilePointCloudMsg msg)
{
    return kArray2_Length(GoProfilePointCloudMsg_Content_(msg), 1);
}

GoFx(k32u) GoProfilePointCloudMsg_XResolution(GoProfilePointCloudMsg msg)
{
    return xGoProfilePointCloudMsg_CastRaw(msg)->xResolution;
}

GoFx(k32u) GoProfilePointCloudMsg_ZResolution(GoProfilePointCloudMsg msg)
{
    return xGoProfilePointCloudMsg_CastRaw(msg)->zResolution;
}

GoFx(k32s) GoProfilePointCloudMsg_XOffset(GoProfilePointCloudMsg msg)
{
    return xGoProfilePointCloudMsg_CastRaw(msg)->xOffset;
}

GoFx(k32s) GoProfilePointCloudMsg_ZOffset(GoProfilePointCloudMsg msg)
{
    return xGoProfilePointCloudMsg_CastRaw(msg)->zOffset;
}

GoFx(kPoint16s*) GoProfilePointCloudMsg_At(GoProfilePointCloudMsg msg, kSize index)
{
    kAssert(index < GoProfilePointCloudMsg_Count(msg));

    return kArray2_AtT(GoProfilePointCloudMsg_Content_(msg), index, 0, kPoint16s);
}

GoFx(k32u) GoProfilePointCloudMsg_Exposure(GoProfilePointCloudMsg msg)
{
    return xGoProfilePointCloudMsg_CastRaw(msg)->exposure;
}

GoFx(k8u) GoProfilePointCloudMsg_CameraIndex(GoProfilePointCloudMsg msg)
{
    return xGoProfilePointCloudMsg_CastRaw(msg)->cameraIndex;
}

/*
 * GoUniformProfileMsg
 */
kBeginClassEx(Go, GoUniformProfileMsg)

    // Serialization versions. Serialzier Type Id string must match the sensor's
    // value of the corresonding message type.
    kAddVersion(GoUniformProfileMsg, GO_SERIALIZATION_FORMAT_NAME, GO_SERIALIZATION_FORMAT_VERSION, GoSerializerTypeIdStr(GO_COMPACT_MESSAGE_UNIFORM_PROFILE), WriteV6, ReadV6)
    kAddVersion(GoUniformProfileMsg, "kdat6", "4.0.0.0", "GoUniformProfileMsg-6", WriteV6, ReadV6)

    //virtual methods
    kAddVMethod(GoUniformProfileMsg, GoDataMsg, VInit)
    kAddVMethod(GoUniformProfileMsg, kObject, VInitClone)
    kAddVMethod(GoUniformProfileMsg, kObject, VRelease)
    kAddVMethod(GoUniformProfileMsg, kObject, VSize)

kEndClassEx()

GoFx(kStatus) GoUniformProfileMsg_Construct(GoUniformProfileMsg* msg, kAlloc allocator)
{
    kAlloc alloc = kAlloc_Fallback(allocator);
    kStatus status;

    kCheck(kAlloc_GetObject(alloc, kTypeOf(GoUniformProfileMsg), msg));

    if (!kSuccess(status = GoUniformProfileMsg_VInit(*msg, kTypeOf(GoUniformProfileMsg), alloc)))
    {
        kAlloc_FreeRef(alloc, msg);
    }

    return status;
}

GoFx(kStatus) GoUniformProfileMsg_VInit(GoUniformProfileMsg msg, kType type, kAlloc alloc)
{
    kObjR(GoUniformProfileMsg, msg);

    kCheck(GoDataMsg_Init(msg, type, GO_DATA_MESSAGE_TYPE_UNIFORM_PROFILE, alloc));

    obj->source = GO_DATA_SOURCE_TOP;
    obj->xResolution = 0;
    obj->zResolution = 0;
    obj->xOffset = 0;
    obj->zOffset = 0;
    kZero(obj->content);
    obj->exposure = 0;

    return kOK;
}

GoFx(kStatus) GoUniformProfileMsg_VInitClone(GoUniformProfileMsg msg, GoUniformProfileMsg source, kAlloc alloc)
{
    kObjR(GoUniformProfileMsg, msg);
    kObjNR(GoUniformProfileMsg, srcObj, source);
    kStatus status;

    kCheck(GoUniformProfileMsg_VInit(msg, kObject_Type(source), alloc));

    kTry
    {
        obj->source = srcObj->source;
        obj->xResolution = srcObj->xResolution;
        obj->zResolution = srcObj->zResolution;
        obj->xOffset = srcObj->xOffset;
        obj->zOffset = srcObj->zOffset;
        obj->exposure = srcObj->exposure;
        obj->base.streamStep = GoDataMsg_StreamStep(source);
        obj->base.streamStepId= GoDataMsg_StreamStepId(source);

        kTest(kObject_Clone(&obj->content, srcObj->content, alloc));
    }
    kCatch(&status)
    {
        GoUniformProfileMsg_VRelease(msg);
        kEndCatch(status);
    }

    return kOK;
}

GoFx(kStatus) GoUniformProfileMsg_Allocate(GoUniformProfileMsg msg, kSize count, kSize width)
{
    kObjR(GoUniformProfileMsg, msg);

    kCheck(kDestroyRef(&obj->content));
    kCheck(kArray2_Construct(&obj->content, kTypeOf(k16s), count, width, kObject_Alloc(msg)));

    return kOK;
}

GoFx(kSize) GoUniformProfileMsg_VSize(GoUniformProfileMsg msg)
{
    kObjR(GoUniformProfileMsg, msg);

    return sizeof(GoUniformProfileMsgClass) + (kIsNull(obj->content) ? 0 : kObject_Size(obj->content));
}

GoFx(kStatus) GoUniformProfileMsg_VRelease(GoUniformProfileMsg msg)
{
    kObjR(GoUniformProfileMsg, msg);

    kCheck(kObject_Destroy(obj->content));

    kCheck(kObject_VRelease(msg));

    return kOK;
}

GoFx(kStatus) GoUniformProfileMsg_WriteV6(GoUniformProfileMsg msg, kSerializer serializer)
{
    kObjR(GoUniformProfileMsg, msg);
    k32u count = (k32u) kArray2_Length(obj->content, 0);
    k32u width = (k32u) kArray2_Length(obj->content, 1);

    kCheck(kSerializer_BeginWrite(serializer, kTypeOf(k16u), kFALSE));

    kCheck(kSerializer_Write32u(serializer, count));
    kCheck(kSerializer_Write32u(serializer, width));
    kCheck(kSerializer_Write32u(serializer, obj->xResolution));
    kCheck(kSerializer_Write32u(serializer, obj->zResolution));
    kCheck(kSerializer_Write32s(serializer, obj->xOffset));
    kCheck(kSerializer_Write32s(serializer, obj->zOffset));
    kCheck(kSerializer_Write8u(serializer, (k8u) obj->source));
    kCheck(kSerializer_Write32u(serializer, obj->exposure));
    kCheck(kSerializer_Write8u(serializer, (k8u) 0));
    kCheck(kSerializer_Write8u(serializer, (k8u) 0));
    kCheck(kSerializer_Write8u(serializer, (k8u) 0));
    kCheck(kSerializer_Write32s(serializer, GoDataMsg_StreamStep(msg)));
    kCheck(kSerializer_Write32s(serializer, GoDataMsg_StreamStepId(msg)));

    kCheck(kSerializer_EndWrite(serializer));

    kCheck(kSerializer_Write16sArray(serializer, kArray2_DataT(obj->content, k16s), count * width));

    return kOK;
}

GoFx(kStatus) GoUniformProfileMsg_ReadV6(GoUniformProfileMsg msg, kSerializer serializer, kAlloc alloc)
{
    kObjR(GoUniformProfileMsg, msg);
    kStatus status;
    k32u count;
    k32u width;

    kCheck(GoUniformProfileMsg_VInit(msg, kTypeOf(GoUniformProfileMsg), alloc));

    kTry
    {
        // Read and store the message attributes data length ("attrSize")
        // to determine how many attributes to read.
        kTest(kSerializer_BeginRead(serializer, kTypeOf(k16u), kFALSE));

        // Read the attributes corresponding to different protocol versions
        // of this message type.
        kTest(GoUniformProfileMsg_ReadV6AttrProtoVer1(msg, serializer, &count, &width));

        kTest(GoUniformProfileMsg_ReadV6AttrProtoVer2(msg, serializer));

        // Skip over attributes this version of the SDK protocol does not
        // know about.
        kTest(kSerializer_EndRead(serializer));

        // Read the message data content that follows the attributes.
        kTest(kArray2_Construct(&obj->content, kTypeOf(k16s), count, width, alloc));
        kTest(kSerializer_Read16sArray(serializer, kArray2_DataT(obj->content, k16s), count * width));
    }
    kCatch(&status)
    {
        GoUniformProfileMsg_VRelease(msg);
        kEndCatch(status);
    }

    return kOK;
}

GoFx(kStatus) GoUniformProfileMsg_ReadV6AttrProtoVer1(GoUniformProfileMsg msg, kSerializer serializer, k32u* countPtr, k32u* widthPtr)
{
    kObjR(GoUniformProfileMsg, msg);
    k8u source;
    k8u reserved;

    kCheck(kSerializer_Read32u(serializer, countPtr));
    kCheck(kSerializer_Read32u(serializer, widthPtr));
    kCheck(kSerializer_Read32u(serializer, &obj->xResolution));
    kCheck(kSerializer_Read32u(serializer, &obj->zResolution));
    kCheck(kSerializer_Read32s(serializer, &obj->xOffset));
    kCheck(kSerializer_Read32s(serializer, &obj->zOffset));
    kCheck(kSerializer_Read8u(serializer, &source));
    obj->source = (GoDataSource) source;
    kCheck(kSerializer_Read32u(serializer, &obj->exposure));
    kCheck(kSerializer_Read8u(serializer, &reserved));
    kCheck(kSerializer_Read8u(serializer, &reserved));
    kCheck(kSerializer_Read8u(serializer, &reserved));

    return kOK;
}

GoFx(kStatus) GoUniformProfileMsg_ReadV6AttrProtoVer2(GoUniformProfileMsg msg, kSerializer serializer)
{
    if (!kSerializer_ReadCompleted(serializer))
    {
        return GoDataMsg_ReadStreamStepAndId(msg, serializer);
    }

    return kOK;
}

GoFx(GoDataSource) GoUniformProfileMsg_Source(GoUniformProfileMsg msg)
{
    return xGoUniformProfileMsg_CastRaw(msg)->source;
}

GoFx(kSize) GoUniformProfileMsg_Count(GoUniformProfileMsg msg)
{
    return kArray2_Length(GoUniformProfileMsg_Content_(msg), 0);
}

GoFx(kSize) GoUniformProfileMsg_Width(GoUniformProfileMsg msg)
{
    return kArray2_Length(GoUniformProfileMsg_Content_(msg), 1);
}

GoFx(k32u) GoUniformProfileMsg_XResolution(GoUniformProfileMsg msg)
{
    return xGoUniformProfileMsg_CastRaw(msg)->xResolution;
}

GoFx(k32u) GoUniformProfileMsg_ZResolution(GoUniformProfileMsg msg)
{
    return xGoUniformProfileMsg_CastRaw(msg)->zResolution;
}

GoFx(k32s) GoUniformProfileMsg_XOffset(GoUniformProfileMsg msg)
{
    return xGoUniformProfileMsg_CastRaw(msg)->xOffset;
}

GoFx(k32s) GoUniformProfileMsg_ZOffset(GoUniformProfileMsg msg)
{
    return xGoUniformProfileMsg_CastRaw(msg)->zOffset;
}

GoFx(k16s*) GoUniformProfileMsg_At(GoUniformProfileMsg msg, kSize index)
{
    kAssert(index < GoUniformProfileMsg_Count(msg));

    return kArray2_AtT(GoUniformProfileMsg_Content_(msg), index, 0, k16s);
}

GoFx(k32u) GoUniformProfileMsg_Exposure(GoUniformProfileMsg msg)
{
    return xGoUniformProfileMsg_CastRaw(msg)->exposure;
}


GoFx(GoDataStep) GoUniformProfileMsg_StreamStep(GoUniformProfileMsg msg)
{
    return GoDataMsg_StreamStep(msg);
}

GoFx(k32s) GoUniformProfileMsg_StreamStepId(GoUniformProfileMsg msg)
{
    return GoDataMsg_StreamStepId(msg);
}

/*
 * GoProfileIntensityMsg
 */

kBeginClassEx(Go, GoProfileIntensityMsg)

    // Serialization versions. Serialzier Type Id string must match the sensor's
    // value of the corresonding message type.
    kAddVersion(GoProfileIntensityMsg, GO_SERIALIZATION_FORMAT_NAME, GO_SERIALIZATION_FORMAT_VERSION, GoSerializerTypeIdStr(GO_COMPACT_MESSAGE_PROFILE_INTENSITY), WriteV7, ReadV7)
    kAddVersion(GoProfileIntensityMsg, "kdat6", "4.0.0.0", "GoProfileIntensityMsg-7", WriteV7, ReadV7)

    //virtual methods
    kAddVMethod(GoProfileIntensityMsg, GoDataMsg, VInit)
    kAddVMethod(GoProfileIntensityMsg, kObject, VInitClone)
    kAddVMethod(GoProfileIntensityMsg, kObject, VRelease)
    kAddVMethod(GoProfileIntensityMsg, kObject, VSize)

kEndClassEx()

GoFx(kStatus) GoProfileIntensityMsg_Construct(GoProfileIntensityMsg* msg, kAlloc allocator)
{
    kAlloc alloc = kAlloc_Fallback(allocator);
    kStatus status;

    kCheck(kAlloc_GetObject(alloc, kTypeOf(GoProfileIntensityMsg), msg));

    if (!kSuccess(status = GoProfileIntensityMsg_VInit(*msg, kTypeOf(GoProfileIntensityMsg), alloc)))
    {
        kAlloc_FreeRef(alloc, msg);
    }

    return status;
}

GoFx(kStatus) GoProfileIntensityMsg_VInit(GoProfileIntensityMsg msg, kType type, kAlloc alloc)
{
    kObjR(GoProfileIntensityMsg, msg);

    kCheck(GoDataMsg_Init(msg, type, GO_DATA_MESSAGE_TYPE_PROFILE_INTENSITY, alloc));

    obj->source = GO_DATA_SOURCE_TOP;
    obj->xResolution = 0;
    obj->xOffset = 0;
    kZero(obj->content);
    obj->exposure = 0;
    obj->cameraIndex = 0;

    return kOK;
}

GoFx(kStatus) GoProfileIntensityMsg_VInitClone(GoProfileIntensityMsg msg, GoProfileIntensityMsg source, kAlloc alloc)
{
    kObjR(GoProfileIntensityMsg, msg);
    kObjNR(GoProfileIntensityMsg, srcObj, source);
    kStatus status;

    kCheck(GoProfileIntensityMsg_VInit(msg, kObject_Type(source), alloc));

    kTry
    {
        obj->source = srcObj->source;
        obj->xResolution = srcObj->xResolution;
        obj->xOffset = srcObj->xOffset;
        obj->exposure = srcObj->exposure;
        obj->cameraIndex = srcObj->cameraIndex;
        obj->base.streamStep = GoDataMsg_StreamStep(source);
        obj->base.streamStepId = GoDataMsg_StreamStepId(source);

        kTest(kObject_Clone(&obj->content, srcObj->content, alloc));
    }
    kCatch(&status)
    {
        GoProfileIntensityMsg_VRelease(msg);
        kEndCatch(status);
    }

    return kOK;
}

GoFx(kStatus) GoProfileIntensityMsg_Allocate(GoProfileIntensityMsg msg, kSize count, kSize width)
{
    kObjR(GoProfileIntensityMsg, msg);

    kCheck(kDestroyRef(&obj->content));
    kCheck(kArray2_Construct(&obj->content, kTypeOf(k8u), count, width, kObject_Alloc(msg)));

    return kOK;
}

GoFx(kSize) GoProfileIntensityMsg_VSize(GoProfileIntensityMsg msg)
{
    kObjR(GoProfileIntensityMsg, msg);

    return sizeof(GoProfileIntensityMsgClass) + (kIsNull(obj->content) ? 0 : kObject_Size(obj->content));
}

GoFx(kStatus) GoProfileIntensityMsg_VRelease(GoProfileIntensityMsg msg)
{
    kObjR(GoProfileIntensityMsg, msg);

    kCheck(kObject_Destroy(obj->content));

    kCheck(kObject_VRelease(msg));

    return kOK;
}

GoFx(kStatus) GoProfileIntensityMsg_WriteV7(GoProfileIntensityMsg msg, kSerializer serializer)
{
    kObjR(GoProfileIntensityMsg, msg);
    k32u count = (k32u) kArray2_Length(obj->content, 0);
    k32u width = (k32u) kArray2_Length(obj->content, 1);

    kCheck(kSerializer_BeginWrite(serializer, kTypeOf(k16u), kFALSE));

    kCheck(kSerializer_Write32u(serializer, count));
    kCheck(kSerializer_Write32u(serializer, width));
    kCheck(kSerializer_Write32u(serializer, obj->xResolution));
    kCheck(kSerializer_Write32s(serializer, obj->xOffset));
    kCheck(kSerializer_Write8u(serializer, (k8u) obj->source));
    kCheck(kSerializer_Write32u(serializer, obj->exposure));
    kCheck(kSerializer_Write8u(serializer, obj->cameraIndex));
    kCheck(kSerializer_Write8u(serializer, 0));
    kCheck(kSerializer_Write8u(serializer, 0));
    kCheck(kSerializer_Write32s(serializer, GoDataMsg_StreamStep(msg)));
    kCheck(kSerializer_Write32s(serializer, GoDataMsg_StreamStepId(msg)));

    kCheck(kSerializer_EndWrite(serializer));

    kCheck(kSerializer_Write8uArray(serializer, kArray2_DataT(obj->content, k8u), kArray2_Count(obj->content)));

    return kOK;
}

GoFx(kStatus) GoProfileIntensityMsg_ReadV7(GoProfileIntensityMsg msg, kSerializer serializer, kAlloc alloc)
{
    kObjR(GoProfileIntensityMsg, msg);
    kStatus status;
    k32u count, width;
    k8u source, reserved;

    kCheck(GoProfileIntensityMsg_VInit(msg, kTypeOf(GoProfileIntensityMsg), alloc));

    kTry
    {
        // Read and store the message attributes data length ("attrSize")
        // to determine how many attributes to read.
        kTest(kSerializer_BeginRead(serializer, kTypeOf(k16u), kFALSE));

        kTest(kSerializer_Read32u(serializer, &count));
        kTest(kSerializer_Read32u(serializer, &width));
        kTest(kSerializer_Read32u(serializer, &obj->xResolution));
        kTest(kSerializer_Read32s(serializer, &obj->xOffset));
        kTest(kSerializer_Read8u(serializer, &source));
        kTest(kSerializer_Read32u(serializer, &obj->exposure));
        kTest(kSerializer_Read8u(serializer, &obj->cameraIndex));
        kTest(kSerializer_Read8u(serializer, &reserved));
        kTest(kSerializer_Read8u(serializer, &reserved));

        kTest(GoProfileIntensityMsg_ReadV7AttrProtoVer2(msg, serializer));

        // Skip over attributes this version of the SDK protocol does not
        // know about.
        kTest(kSerializer_EndRead(serializer));

        kTest(kArray2_Construct(&obj->content, kTypeOf(k8u), count, width, alloc));
        kTest(kSerializer_Read8uArray(serializer, kArray2_DataT(obj->content, k8u), kArray2_Count(obj->content)));

        obj->source = (GoDataSource)source;
    }
    kCatch(&status)
    {
        GoProfileIntensityMsg_VRelease(msg);
        kEndCatch(status);
    }

    return kOK;
}

GoFx(kStatus) GoProfileIntensityMsg_ReadV7AttrProtoVer2(GoProfileIntensityMsg msg, kSerializer serializer)
{
    if (!kSerializer_ReadCompleted(serializer))
    {
        return GoDataMsg_ReadStreamStepAndId(msg, serializer);
    }

    return kOK;
}

GoFx(GoDataSource) GoProfileIntensityMsg_Source(GoProfileIntensityMsg msg)
{
    return xGoProfileIntensityMsg_CastRaw(msg)->source;
}

GoFx(kSize) GoProfileIntensityMsg_Count(GoProfileIntensityMsg msg)
{
    return kArray2_Length(GoProfileIntensityMsg_Content_(msg), 0);
}

GoFx(kSize) GoProfileIntensityMsg_Width(GoProfileIntensityMsg msg)
{
    return kArray2_Length(GoProfileIntensityMsg_Content_(msg), 1);
}

GoFx(k32u) GoProfileIntensityMsg_XResolution(GoProfileIntensityMsg msg)
{
    return xGoProfileIntensityMsg_CastRaw(msg)->xResolution;
}

GoFx(k32s) GoProfileIntensityMsg_XOffset(GoProfileIntensityMsg msg)
{
    return xGoProfileIntensityMsg_CastRaw(msg)->xOffset;
}

GoFx(k8u*) GoProfileIntensityMsg_At(GoProfileIntensityMsg msg, kSize index)
{
    kAssert(index < GoProfileIntensityMsg_Count(msg));

    return kArray2_AtT(GoProfileIntensityMsg_Content_(msg), index, 0, k8u);
}

GoFx(k32u) GoProfileIntensityMsg_Exposure(GoProfileIntensityMsg msg)
{
    return xGoProfileIntensityMsg_CastRaw(msg)->exposure;
}

GoFx(k8u) GoProfileIntensityMsg_CameraIndex(GoProfileMsg msg)
{
    return xGoProfileIntensityMsg_CastRaw(msg)->cameraIndex;
}


/*
 * GoUniformSurfaceMsg
 */

kBeginClassEx(Go, GoUniformSurfaceMsg)

    // Serialization versions. Serialzier Type Id string must match the sensor's
    // value of the corresonding message type.
    kAddVersion(GoUniformSurfaceMsg, GO_SERIALIZATION_FORMAT_NAME, GO_SERIALIZATION_FORMAT_VERSION, GoSerializerTypeIdStr(GO_COMPACT_MESSAGE_UNIFORM_SURFACE), WriteV8, ReadV8)
    kAddVersion(GoUniformSurfaceMsg, "kdat6", "4.0.0.0", "GoUniformSurfaceMsg-8", WriteV8, ReadV8)

    //virtual methods
    kAddVMethod(GoUniformSurfaceMsg, GoDataMsg, VInit)
    kAddVMethod(GoUniformSurfaceMsg, kObject, VInitClone)
    kAddVMethod(GoUniformSurfaceMsg, kObject, VRelease)
    kAddVMethod(GoUniformSurfaceMsg, kObject, VSize)

kEndClassEx()

GoFx(kStatus) GoUniformSurfaceMsg_Construct(GoUniformSurfaceMsg* msg, kAlloc allocator)
{
    kAlloc alloc = kAlloc_Fallback(allocator);
    kStatus status;

    kCheck(kAlloc_GetObject(alloc, kTypeOf(GoUniformSurfaceMsg), msg));

    if (!kSuccess(status = GoUniformSurfaceMsg_VInit(*msg, kTypeOf(GoUniformSurfaceMsg), alloc)))
    {
        kAlloc_FreeRef(alloc, msg);
    }

    return status;
}

GoFx(kStatus) GoUniformSurfaceMsg_VInit(GoUniformSurfaceMsg msg, kType type, kAlloc alloc)
{
    kObjR(GoUniformSurfaceMsg, msg);

    kCheck(GoDataMsg_Init(msg, type, GO_DATA_MESSAGE_TYPE_UNIFORM_SURFACE, alloc));

    obj->source = GO_DATA_SOURCE_TOP;
    obj->xResolution = 0;
    obj->yResolution = 0;
    obj->zResolution = 0;
    obj->xOffset = 0;
    obj->yOffset = 0;
    obj->zOffset = 0;
    kZero(obj->content);
    obj->exposure = 0;

    return kOK;
}

GoFx(kStatus) GoUniformSurfaceMsg_VInitClone(GoUniformSurfaceMsg msg, GoUniformSurfaceMsg source, kAlloc alloc)
{
    kObjR(GoUniformSurfaceMsg, msg);
    kObjNR(GoUniformSurfaceMsg, srcObj, source);
    kStatus status;

    kCheck(GoUniformSurfaceMsg_VInit(msg, kObject_Type(source), alloc));

    kTry
    {
        obj->source         = srcObj->source;
        obj->xResolution    = srcObj->xResolution;
        obj->yResolution    = srcObj->yResolution;
        obj->zResolution    = srcObj->zResolution;
        obj->xOffset        = srcObj->xOffset;
        obj->yOffset        = srcObj->yOffset;
        obj->zOffset        = srcObj->zOffset;
        obj->exposure       = srcObj->exposure;
        obj->base.streamStep     = GoDataMsg_StreamStep(source);
        obj->base.streamStepId   = GoDataMsg_StreamStepId(source);

        kTest(kObject_Clone(&obj->content, srcObj->content, alloc));
    }
    kCatch(&status)
    {
        GoUniformSurfaceMsg_VRelease(msg);
        kEndCatch(status);
    }

    return kOK;
}

GoFx(kStatus) GoUniformSurfaceMsg_Allocate(GoUniformSurfaceMsg msg, kSize length, kSize width)
{
    kObjR(GoUniformSurfaceMsg, msg);

    kCheck(kDestroyRef(&obj->content));
    kCheck(kArray2_Construct(&obj->content, kTypeOf(k16s), length, width, kObject_Alloc(msg)));

    return kOK;
}

GoFx(kSize) GoUniformSurfaceMsg_VSize(GoUniformSurfaceMsg msg)
{
    kObjR(GoUniformSurfaceMsg, msg);

    return sizeof(GoUniformSurfaceMsgClass) + (kIsNull(obj->content) ? 0 : kObject_Size(obj->content));
}

GoFx(kStatus) GoUniformSurfaceMsg_VRelease(GoUniformSurfaceMsg msg)
{
    kObjR(GoUniformSurfaceMsg, msg);

    kCheck(kObject_Destroy(obj->content));

    kCheck(kObject_VRelease(msg));

    return kOK;
}

GoFx(kStatus) GoUniformSurfaceMsg_WriteV8(GoUniformSurfaceMsg msg, kSerializer serializer)
{
    kObjR(GoUniformSurfaceMsg, msg);
    k32u length = (k32u) kArray2_Length(obj->content, 0);
    k32u width = (k32u) kArray2_Length(obj->content, 1);

    kCheck(kSerializer_BeginWrite(serializer, kTypeOf(k16u), kFALSE));

    kCheck(kSerializer_Write32u(serializer, length));
    kCheck(kSerializer_Write32u(serializer, width));
    kCheck(kSerializer_Write32u(serializer, obj->xResolution));
    kCheck(kSerializer_Write32u(serializer, obj->yResolution));
    kCheck(kSerializer_Write32u(serializer, obj->zResolution));
    kCheck(kSerializer_Write32s(serializer, obj->xOffset));
    kCheck(kSerializer_Write32s(serializer, obj->yOffset));
    kCheck(kSerializer_Write32s(serializer, obj->zOffset));
    kCheck(kSerializer_Write8u(serializer, (k8u) obj->source));
    kCheck(kSerializer_Write32u(serializer, obj->exposure));

    kCheck(kSerializer_Write32u(serializer, (k32u) 0));
    kCheck(kSerializer_Write8u(serializer, (k8u) 0));
    kCheck(kSerializer_Write8u(serializer, (k8u) 0));
    kCheck(kSerializer_Write8u(serializer, (k8u) 0));

    kCheck(kSerializer_Write32s(serializer, GoDataMsg_StreamStep(msg)));
    kCheck(kSerializer_Write32s(serializer, GoDataMsg_StreamStepId(msg)));

    kCheck(kSerializer_EndWrite(serializer));

    kCheck(kSerializer_Write16sArray(serializer, kArray2_DataT(obj->content, k16s), length * width));

    return kOK;
}

GoFx(kStatus) GoUniformSurfaceMsg_ReadV8(GoUniformSurfaceMsg msg, kSerializer serializer, kAlloc alloc)
{
    kObjR(GoUniformSurfaceMsg, msg);
    kStatus status;
    k32u length;
    k32u width;

    kCheck(GoUniformSurfaceMsg_VInit(msg, kTypeOf(GoUniformSurfaceMsg), alloc));

    kTry
    {
        // Read and store the message attributes data length ("attrSize")
        // to determine how many attributes to read.
        kTest(kSerializer_BeginRead(serializer, kTypeOf(k16u), kFALSE));

        // Read the attributes corresponding to different protocol versions
        // of this message type.
        kTest(GoUniformSurfaceMsg_ReadV8AttrProtoVer1(msg, serializer, &length, &width));

        kTest(GoUniformSurfaceMsg_ReadV8AttrProtoVer2(msg, serializer));

        // Skip over attributes this version of the SDK protocol does not
        // know about.
        kTest(kSerializer_EndRead(serializer));

        // Read the message data content that follows the attributes.
        kTest(kArray2_Construct(&obj->content, kTypeOf(k16s), length, width, alloc));
        kTest(kSerializer_Read16sArray(serializer, kArray2_DataT(obj->content, k16s), length * width));
    }
    kCatch(&status)
    {
        GoUniformSurfaceMsg_VRelease(msg);
        kEndCatch(status);
    }

    return kOK;
}

GoFx(kStatus) GoUniformSurfaceMsg_ReadV8AttrProtoVer1(GoUniformSurfaceMsg msg, kSerializer serializer, k32u* lenPtr, k32u* widthPtr)
{
    kObjR(GoUniformSurfaceMsg, msg);
    k8u source;
    k8u reserved8;
    k32u reserved32;

    kCheck(kSerializer_Read32u(serializer, lenPtr));
    kCheck(kSerializer_Read32u(serializer, widthPtr));
    kCheck(kSerializer_Read32u(serializer, &obj->xResolution));
    kCheck(kSerializer_Read32u(serializer, &obj->yResolution));
    kCheck(kSerializer_Read32u(serializer, &obj->zResolution));
    kCheck(kSerializer_Read32s(serializer, &obj->xOffset));
    kCheck(kSerializer_Read32s(serializer, &obj->yOffset));
    kCheck(kSerializer_Read32s(serializer, &obj->zOffset));
    kCheck(kSerializer_Read8u(serializer, &source));
    obj->source = (GoDataSource) source;
    kCheck(kSerializer_Read32u(serializer, &obj->exposure));

    kCheck(kSerializer_Read32u(serializer, &reserved32));

    kCheck(kSerializer_Read8u(serializer, &reserved8));
    kCheck(kSerializer_Read8u(serializer, &reserved8));
    kCheck(kSerializer_Read8u(serializer, &reserved8));

    return kOK;
}

GoFx(kStatus) GoUniformSurfaceMsg_ReadV8AttrProtoVer2(GoUniformSurfaceMsg msg, kSerializer serializer)
{
    if (!kSerializer_ReadCompleted(serializer))
    {
        return GoDataMsg_ReadStreamStepAndId(msg, serializer);
    }

    return kOK;
}

GoFx(GoDataSource) GoUniformSurfaceMsg_Source(GoUniformSurfaceMsg msg)
{
    return xGoUniformSurfaceMsg_CastRaw(msg)->source;
}

GoFx(kSize) GoUniformSurfaceMsg_Length(GoUniformSurfaceMsg msg)
{
    return kArray2_Length(GoUniformSurfaceMsg_Content_(msg), 0);
}

GoFx(kSize) GoUniformSurfaceMsg_Width(GoUniformSurfaceMsg msg)
{
    return kArray2_Length(GoUniformSurfaceMsg_Content_(msg), 1);
}

GoFx(k32u) GoUniformSurfaceMsg_XResolution(GoUniformSurfaceMsg msg)
{
    return xGoUniformSurfaceMsg_CastRaw(msg)->xResolution;
}

GoFx(k32u) GoUniformSurfaceMsg_YResolution(GoUniformSurfaceMsg msg)
{
    return xGoUniformSurfaceMsg_CastRaw(msg)->yResolution;
}

GoFx(k32u) GoUniformSurfaceMsg_ZResolution(GoUniformSurfaceMsg msg)
{
    return xGoUniformSurfaceMsg_CastRaw(msg)->zResolution;
}

GoFx(k32s) GoUniformSurfaceMsg_XOffset(GoUniformSurfaceMsg msg)
{
    return xGoUniformSurfaceMsg_CastRaw(msg)->xOffset;
}

GoFx(k32s) GoUniformSurfaceMsg_YOffset(GoUniformSurfaceMsg msg)
{
    return xGoUniformSurfaceMsg_CastRaw(msg)->yOffset;
}

GoFx(k32s) GoUniformSurfaceMsg_ZOffset(GoUniformSurfaceMsg msg)
{
    return xGoUniformSurfaceMsg_CastRaw(msg)->zOffset;
}

GoFx(k16s*) GoUniformSurfaceMsg_RowAt(GoUniformSurfaceMsg msg, kSize index)
{
    kAssert(index < GoUniformSurfaceMsg_Length(msg));

    return kArray2_AtT(GoUniformSurfaceMsg_Content_(msg), index, 0, k16s);
}

GoFx(k32u) GoUniformSurfaceMsg_Exposure(GoUniformSurfaceMsg msg)
{
    return xGoUniformSurfaceMsg_CastRaw(msg)->exposure;
}

GoFx(GoDataStep) GoUniformSurfaceMsg_StreamStep(GoUniformSurfaceMsg msg)
{
    return GoDataMsg_StreamStep(msg);
}

GoFx(k32s) GoUniformSurfaceMsg_StreamStepId(GoUniformSurfaceMsg msg)
{
    return GoDataMsg_StreamStepId(msg);
}

/*
* GoSurfacePointCloudMsg
*/

kBeginClassEx(Go, GoSurfacePointCloudMsg)

// Serialization versions. Serialzier Type Id string must match the sensor's
// value of the corresonding message type.
kAddVersion(GoSurfacePointCloudMsg, GO_SERIALIZATION_FORMAT_NAME, GO_SERIALIZATION_FORMAT_VERSION, GoSerializerTypeIdStr(GO_COMPACT_MESSAGE_SURFACE_POINT_CLOUD), WriteV8, ReadV8)
kAddVersion(GoSurfacePointCloudMsg, "kdat6", "4.0.0.0", "GoSurfacePointCloudMsg-28", WriteV8, ReadV8)

//virtual methods
kAddVMethod(GoSurfacePointCloudMsg, GoDataMsg, VInit)
kAddVMethod(GoSurfacePointCloudMsg, kObject, VInitClone)
kAddVMethod(GoSurfacePointCloudMsg, kObject, VRelease)
kAddVMethod(GoSurfacePointCloudMsg, kObject, VSize)

kEndClassEx()

GoFx(kStatus) GoSurfacePointCloudMsg_Construct(GoSurfacePointCloudMsg* msg, kAlloc allocator)
{
    kAlloc alloc = kAlloc_Fallback(allocator);
    kStatus status;

    kCheck(kAlloc_GetObject(alloc, kTypeOf(GoSurfacePointCloudMsg), msg));

    if (!kSuccess(status = GoSurfacePointCloudMsg_VInit(*msg, kTypeOf(GoSurfacePointCloudMsg), alloc)))
    {
        kAlloc_FreeRef(alloc, msg);
    }

    return status;
}

GoFx(kStatus) GoSurfacePointCloudMsg_VInit(GoSurfacePointCloudMsg msg, kType type, kAlloc alloc)
{
    kObjR(GoSurfacePointCloudMsg, msg);

    kCheck(GoDataMsg_Init(msg, type, GO_DATA_MESSAGE_TYPE_SURFACE_POINT_CLOUD, alloc));

    obj->source = GO_DATA_SOURCE_TOP;
    obj->xResolution = 0;
    obj->yResolution = 0;
    obj->zResolution = 0;
    obj->xOffset = 0;
    obj->yOffset = 0;
    obj->zOffset = 0;
    kZero(obj->content);
    obj->exposure = 0;
    obj->isAdjacent = kFALSE;

    return kOK;
}

GoFx(kStatus) GoSurfacePointCloudMsg_VInitClone(GoSurfacePointCloudMsg msg, GoSurfacePointCloudMsg source, kAlloc alloc)
{
    kObjR(GoSurfacePointCloudMsg, msg);
    kObjNR(GoSurfacePointCloudMsg, srcObj, source);
    kStatus status;

    kCheck(GoSurfacePointCloudMsg_VInit(msg, kObject_Type(source), alloc));

    kTry
    {
        obj->source = srcObj->source;
        obj->xResolution = srcObj->xResolution;
        obj->yResolution = srcObj->yResolution;
        obj->zResolution = srcObj->zResolution;
        obj->xOffset = srcObj->xOffset;
        obj->yOffset = srcObj->yOffset;
        obj->zOffset = srcObj->zOffset;
        obj->exposure = srcObj->exposure;
        obj->base.streamStep = GoDataMsg_StreamStep(source);
        obj->base.streamStepId = GoDataMsg_StreamStepId(source);
        obj->isAdjacent = srcObj->isAdjacent;

        kTest(kObject_Clone(&obj->content, srcObj->content, alloc));
    }
    kCatch(&status)
    {
        GoSurfacePointCloudMsg_VRelease(msg);
        kEndCatch(status);
    }

    return kOK;
}

GoFx(kStatus) GoSurfacePointCloudMsg_Allocate(GoSurfacePointCloudMsg msg, kSize length, kSize width)
{
    kObjR(GoSurfacePointCloudMsg, msg);

    kCheck(kDestroyRef(&obj->content));
    kCheck(kArray2_Construct(&obj->content, kTypeOf(k16s), length, width, kObject_Alloc(msg)));

    return kOK;
}

GoFx(kSize) GoSurfacePointCloudMsg_VSize(GoSurfacePointCloudMsg msg)
{
    kObjR(GoSurfacePointCloudMsg, msg);

    return sizeof(GoSurfacePointCloudMsgClass) + (kIsNull(obj->content) ? 0 : kObject_Size(obj->content));
}

GoFx(kStatus) GoSurfacePointCloudMsg_VRelease(GoSurfacePointCloudMsg msg)
{
    kObjR(GoSurfacePointCloudMsg, msg);

    kCheck(kObject_Destroy(obj->content));

    kCheck(kObject_VRelease(msg));

    return kOK;
}

GoFx(kStatus) GoSurfacePointCloudMsg_WriteV8(GoSurfacePointCloudMsg msg, kSerializer serializer)
{
    kObjR(GoSurfacePointCloudMsg, msg);
    k32u length = (k32u)kArray2_Length(obj->content, 0);
    k32u width = (k32u)kArray2_Length(obj->content, 1);

    kCheck(kSerializer_BeginWrite(serializer, kTypeOf(k16u), kFALSE));

    kCheck(kSerializer_Write32u(serializer, length));
    kCheck(kSerializer_Write32u(serializer, width));
    kCheck(kSerializer_Write32u(serializer, obj->xResolution));
    kCheck(kSerializer_Write32u(serializer, obj->yResolution));
    kCheck(kSerializer_Write32u(serializer, obj->zResolution));
    kCheck(kSerializer_Write32s(serializer, obj->xOffset));
    kCheck(kSerializer_Write32s(serializer, obj->yOffset));
    kCheck(kSerializer_Write32s(serializer, obj->zOffset));
    kCheck(kSerializer_Write8u(serializer, (k8u)obj->source));
    kCheck(kSerializer_Write32u(serializer, obj->exposure));

    kCheck(kSerializer_Write8u(serializer, (k8u)obj->isAdjacent));

    kCheck(kSerializer_Write32s(serializer, GoDataMsg_StreamStep(msg)));
    kCheck(kSerializer_Write32s(serializer, GoDataMsg_StreamStepId(msg)));

    kCheck(kSerializer_EndWrite(serializer));

    kCheck(kSerializer_Write16sArray(serializer, kArray2_DataT(obj->content, k16s), length * width * 3));

    return kOK;
}

GoFx(kStatus) GoSurfacePointCloudMsg_ReadV8(GoSurfacePointCloudMsg msg, kSerializer serializer, kAlloc alloc)
{
    kObjR(GoSurfacePointCloudMsg, msg);
    kStatus status;
    k32u length;
    k32u width;

    kCheck(GoSurfacePointCloudMsg_VInit(msg, kTypeOf(GoSurfacePointCloudMsg), alloc));

    kTry
    {
        // Read and store the message attributes data length ("attrSize")
        // to determine how many attributes to read.
        kTest(kSerializer_BeginRead(serializer, kTypeOf(k16u), kFALSE));

        // Read the attributes corresponding to different protocol versions
        // of this message type.
        kTest(GoSurfacePointCloudMsg_ReadV8Attr(msg, serializer, &length, &width));

        // Skip over attributes this version of the SDK protocol does not
        // know about.
        kTest(kSerializer_EndRead(serializer));

        // Read the message data content that follows the attributes.
        kTest(kArray2_Construct(&obj->content, kTypeOf(kPoint3d16s), length, width, alloc));
        kTest(kSerializer_Read16sArray(serializer, (k16s*) kArray2_DataT(obj->content, kPoint3d16s), length * width * 3));
    }
    kCatch(&status)
    {
        GoSurfacePointCloudMsg_VRelease(msg);
        kEndCatch(status);
    }

    return kOK;
}

GoFx(kStatus) GoSurfacePointCloudMsg_ReadV8Attr(GoSurfacePointCloudMsg msg, kSerializer serializer, k32u* lenPtr, k32u* widthPtr)
{
    kObjR(GoSurfacePointCloudMsg, msg);
    k8u source;

    kCheck(kSerializer_Read32u(serializer, lenPtr));
    kCheck(kSerializer_Read32u(serializer, widthPtr));
    kCheck(kSerializer_Read32u(serializer, &obj->xResolution));
    kCheck(kSerializer_Read32u(serializer, &obj->yResolution));
    kCheck(kSerializer_Read32u(serializer, &obj->zResolution));
    kCheck(kSerializer_Read32s(serializer, &obj->xOffset));
    kCheck(kSerializer_Read32s(serializer, &obj->yOffset));
    kCheck(kSerializer_Read32s(serializer, &obj->zOffset));
    kCheck(kSerializer_Read8u(serializer, &source));
    obj->source = (GoDataSource)source;
    kCheck(kSerializer_Read32u(serializer, &obj->exposure));

    kCheck(kSerializer_Read8u(serializer, (k8u*)&obj->isAdjacent));

    kCheck(kSerializer_Read32s(serializer, &obj->base.streamStep));
    kCheck(kSerializer_Read32s(serializer, &obj->base.streamStepId));

    return kOK;
}

GoFx(GoDataSource) GoSurfacePointCloudMsg_Source(GoSurfacePointCloudMsg msg)
{
    return xGoSurfacePointCloudMsg_CastRaw(msg)->source;
}

GoFx(kSize) GoSurfacePointCloudMsg_Length(GoSurfacePointCloudMsg msg)
{
    return kArray2_Length(GoSurfacePointCloudMsg_Content_(msg), 0);
}

GoFx(kSize) GoSurfacePointCloudMsg_Width(GoSurfacePointCloudMsg msg)
{
    return kArray2_Length(GoSurfacePointCloudMsg_Content_(msg), 1);
}

GoFx(k32u) GoSurfacePointCloudMsg_XResolution(GoSurfacePointCloudMsg msg)
{
    return xGoSurfacePointCloudMsg_CastRaw(msg)->xResolution;
}

GoFx(k32u) GoSurfacePointCloudMsg_YResolution(GoSurfacePointCloudMsg msg)
{
    return xGoSurfacePointCloudMsg_CastRaw(msg)->yResolution;
}

GoFx(k32u) GoSurfacePointCloudMsg_ZResolution(GoSurfacePointCloudMsg msg)
{
    return xGoSurfacePointCloudMsg_CastRaw(msg)->zResolution;
}

GoFx(k32s) GoSurfacePointCloudMsg_XOffset(GoSurfacePointCloudMsg msg)
{
    return xGoSurfacePointCloudMsg_CastRaw(msg)->xOffset;
}

GoFx(k32s) GoSurfacePointCloudMsg_YOffset(GoSurfacePointCloudMsg msg)
{
    return xGoSurfacePointCloudMsg_CastRaw(msg)->yOffset;
}

GoFx(k32s) GoSurfacePointCloudMsg_ZOffset(GoSurfacePointCloudMsg msg)
{
    return xGoSurfacePointCloudMsg_CastRaw(msg)->zOffset;
}

GoFx(kPoint3d16s*) GoSurfacePointCloudMsg_RowAt(GoSurfacePointCloudMsg msg, kSize index)
{
    kAssert(index < GoSurfacePointCloudMsg_Length(msg));

    return kArray2_AtT(GoSurfacePointCloudMsg_Content_(msg), index, 0, kPoint3d16s);
}

GoFx(k32u) GoSurfacePointCloudMsg_Exposure(GoSurfacePointCloudMsg msg)
{
    return xGoSurfacePointCloudMsg_CastRaw(msg)->exposure;
}

GoFx(GoDataStep) GoSurfacePointCloudMsg_StreamStep(GoSurfacePointCloudMsg msg)
{
    return GoDataMsg_StreamStep(msg);
}

GoFx(k32s) GoSurfacePointCloudMsg_StreamStepId(GoSurfacePointCloudMsg msg)
{
    return GoDataMsg_StreamStepId(msg);
}

GoFx(kBool) GoSurfacePointCloudMsg_IsAdjacent(GoSurfacePointCloudMsg msg)
{
    return xGoSurfacePointCloudMsg_CastRaw(msg)->isAdjacent;
}


/*
 * GoSurfaceIntensityMsg
 */

kBeginClassEx(Go, GoSurfaceIntensityMsg)

    // Serialization versions. Serialzier Type Id string must match the sensor's
    // value of the corresonding message type.
    kAddVersion(GoSurfaceIntensityMsg, GO_SERIALIZATION_FORMAT_NAME, GO_SERIALIZATION_FORMAT_VERSION, GoSerializerTypeIdStr(GO_COMPACT_MESSAGE_SURFACE_INTENSITY), WriteV9, ReadV9)
    kAddVersion(GoSurfaceIntensityMsg, "kdat6", "4.0.0.0", "GoSurfaceIntensityMsg-9", WriteV9, ReadV9)

    //virtual methods
    kAddVMethod(GoSurfaceIntensityMsg, GoDataMsg, VInit)
    kAddVMethod(GoSurfaceIntensityMsg, kObject, VInitClone)
    kAddVMethod(GoSurfaceIntensityMsg, kObject, VRelease)
    kAddVMethod(GoSurfaceIntensityMsg, kObject, VSize)

kEndClassEx()

GoFx(kStatus) GoSurfaceIntensityMsg_Construct(GoSurfaceIntensityMsg* msg, kAlloc allocator)
{
    kAlloc alloc = kAlloc_Fallback(allocator);
    kStatus status;

    kCheck(kAlloc_GetObject(alloc, kTypeOf(GoSurfaceIntensityMsg), msg));

    if (!kSuccess(status = GoSurfaceIntensityMsg_VInit(*msg, kTypeOf(GoSurfaceIntensityMsg), alloc)))
    {
        kAlloc_FreeRef(alloc, msg);
    }

    return status;
}

GoFx(kStatus) GoSurfaceIntensityMsg_VInit(GoSurfaceIntensityMsg msg, kType type, kAlloc alloc)
{
    kObjR(GoSurfaceIntensityMsg, msg);

    kCheck(GoDataMsg_Init(msg, type, GO_DATA_MESSAGE_TYPE_SURFACE_INTENSITY, alloc));

    obj->source = GO_DATA_SOURCE_TOP;
    obj->xResolution = 0;
    obj->yResolution = 0;
    obj->xOffset = 0;
    obj->yOffset = 0;
    kZero(obj->content);
    obj->exposure = 0;

    return kOK;
}

GoFx(kStatus) GoSurfaceIntensityMsg_VInitClone(GoSurfaceIntensityMsg msg, GoSurfaceIntensityMsg source, kAlloc alloc)
{
    kObjR(GoSurfaceIntensityMsg, msg);
    kObjNR(GoSurfaceIntensityMsg, srcObj, source);
    kStatus status;

    kCheck(GoSurfaceIntensityMsg_VInit(msg, kObject_Type(source), alloc));

    kTry
    {
        obj->source = srcObj->source;
        obj->xResolution = srcObj->xResolution;
        obj->yResolution = srcObj->yResolution;
        obj->xOffset = srcObj->xOffset;
        obj->yOffset = srcObj->yOffset;
        obj->exposure = srcObj->exposure;
        obj->base.streamStep = GoDataMsg_StreamStep(source);
        obj->base.streamStepId = GoDataMsg_StreamStepId(source);

        kTest(kObject_Clone(&obj->content, srcObj->content, alloc));
    }
    kCatch(&status)
    {
        GoSurfaceIntensityMsg_VRelease(msg);
        kEndCatch(status);
    }

    return kOK;
}

GoFx(kStatus) GoSurfaceIntensityMsg_Allocate(GoSurfaceIntensityMsg msg, kSize length, kSize width)
{
    kObjR(GoSurfaceIntensityMsg, msg);

    kCheck(kDestroyRef(&obj->content));
    kCheck(kArray2_Construct(&obj->content, kTypeOf(k8u), length, width, kObject_Alloc(msg)));

    return kOK;
}

GoFx(kSize) GoSurfaceIntensityMsg_VSize(GoSurfaceIntensityMsg msg)
{
    kObjR(GoSurfaceIntensityMsg, msg);

    return sizeof(GoSurfaceIntensityMsgClass) + (kIsNull(obj->content) ? 0 : kObject_Size(obj->content));
}

GoFx(kStatus) GoSurfaceIntensityMsg_VRelease(GoSurfaceIntensityMsg msg)
{
    kObjR(GoSurfaceIntensityMsg, msg);

    kCheck(kObject_Destroy(obj->content));

    kCheck(kObject_VRelease(msg));

    return kOK;
}

GoFx(kStatus) GoSurfaceIntensityMsg_WriteV9(GoSurfaceIntensityMsg msg, kSerializer serializer)
{
    kObjR(GoSurfaceIntensityMsg, msg);
    k32u length = (k32u) kArray2_Length(obj->content, 0);
    k32u width = (k32u) kArray2_Length(obj->content, 1);

    kCheck(kSerializer_BeginWrite(serializer, kTypeOf(k16u), kFALSE));

    kCheck(kSerializer_Write32u(serializer, length));
    kCheck(kSerializer_Write32u(serializer, width));
    kCheck(kSerializer_Write32u(serializer, obj->xResolution));
    kCheck(kSerializer_Write32u(serializer, obj->yResolution));
    kCheck(kSerializer_Write32s(serializer, obj->xOffset));
    kCheck(kSerializer_Write32s(serializer, obj->yOffset));
    kCheck(kSerializer_Write8u(serializer, (k8u) obj->source));
    kCheck(kSerializer_Write32u(serializer, obj->exposure));
    kCheck(kSerializer_Write8u(serializer, 0));
    kCheck(kSerializer_Write8u(serializer, 0));
    kCheck(kSerializer_Write8u(serializer, 0));
    kCheck(kSerializer_Write32s(serializer, GoDataMsg_StreamStep(msg)));
    kCheck(kSerializer_Write32s(serializer, GoDataMsg_StreamStepId(msg)));

    kCheck(kSerializer_EndWrite(serializer));

    kCheck(kSerializer_Write8uArray(serializer, kArray2_DataT(obj->content, k8u), kArray2_Count(obj->content)));

    return kOK;
}

GoFx(kStatus) GoSurfaceIntensityMsg_ReadV9(GoSurfaceIntensityMsg msg, kSerializer serializer, kAlloc alloc)
{
    kObjR(GoSurfaceIntensityMsg, msg);
    kStatus status;
    k32u length, width;
    k8u source, reserved;

    kCheck(GoSurfaceIntensityMsg_VInit(msg, kTypeOf(GoSurfaceIntensityMsg), alloc));

    kTry
    {
        // Read and store the message attributes data length ("attrSize")
        // to determine how many attributes to read.
        kTest(kSerializer_BeginRead(serializer, kTypeOf(k16u), kFALSE));

        kTest(kSerializer_Read32u(serializer, &length));
        kTest(kSerializer_Read32u(serializer, &width));
        kTest(kSerializer_Read32u(serializer, &obj->xResolution));
        kTest(kSerializer_Read32u(serializer, &obj->yResolution));
        kTest(kSerializer_Read32s(serializer, &obj->xOffset));
        kTest(kSerializer_Read32s(serializer, &obj->yOffset));
        kTest(kSerializer_Read8u(serializer, &source));
        kTest(kSerializer_Read32u(serializer, &obj->exposure));
        kTest(kSerializer_Read8u(serializer, &reserved));
        kTest(kSerializer_Read8u(serializer, &reserved));
        kTest(kSerializer_Read8u(serializer, &reserved));

        kTest(GoSurfaceIntensityMsg_ReadV9AttrProtoVer2(msg, serializer));

        // Skip over attributes this version of the SDK protocol does not
        // know about.
        kTest(kSerializer_EndRead(serializer));

        kTest(kArray2_Construct(&obj->content, kTypeOf(k8u), length, width, alloc));
        kTest(kSerializer_Read8uArray(serializer, kArray2_DataT(obj->content, k8u), kArray2_Count(obj->content)));

        obj->source = (GoDataSource)source;
    }
    kCatch(&status)
    {
        GoSurfaceIntensityMsg_VRelease(msg);
        kEndCatch(status);
    }

    return kOK;
}

GoFx(kStatus) GoSurfaceIntensityMsg_ReadV9AttrProtoVer2(GoSurfaceIntensityMsg msg, kSerializer serializer)
{
    if (!kSerializer_ReadCompleted(serializer))
    {
        return GoDataMsg_ReadStreamStepAndId(msg, serializer);
    }

    return kOK;
}

GoFx(GoDataSource) GoSurfaceIntensityMsg_Source(GoSurfaceIntensityMsg msg)
{
    return xGoSurfaceIntensityMsg_CastRaw(msg)->source;
}

GoFx(kSize) GoSurfaceIntensityMsg_Length(GoSurfaceIntensityMsg msg)
{
    return kArray2_Length(GoSurfaceIntensityMsg_Content_(msg), 0);
}

GoFx(kSize) GoSurfaceIntensityMsg_Width(GoSurfaceIntensityMsg msg)
{
    return kArray2_Length(GoSurfaceIntensityMsg_Content_(msg), 1);
}

GoFx(k32u) GoSurfaceIntensityMsg_XResolution(GoSurfaceIntensityMsg msg)
{
    return xGoSurfaceIntensityMsg_CastRaw(msg)->xResolution;
}

GoFx(k32u) GoSurfaceIntensityMsg_YResolution(GoSurfaceIntensityMsg msg)
{
    return xGoSurfaceIntensityMsg_CastRaw(msg)->yResolution;
}

GoFx(k32s) GoSurfaceIntensityMsg_XOffset(GoSurfaceIntensityMsg msg)
{
    return xGoSurfaceIntensityMsg_CastRaw(msg)->xOffset;
}

GoFx(k32s) GoSurfaceIntensityMsg_YOffset(GoSurfaceIntensityMsg msg)
{
    return xGoSurfaceIntensityMsg_CastRaw(msg)->yOffset;
}

GoFx(k8u*) GoSurfaceIntensityMsg_RowAt(GoSurfaceIntensityMsg msg, kSize index)
{
    kAssert(index < GoSurfaceIntensityMsg_Length(msg));

    return kArray2_AtT(GoSurfaceIntensityMsg_Content_(msg), index, 0, k8u);
}

GoFx(k32u) GoSurfaceIntensityMsg_Exposure(GoSurfaceIntensityMsg msg)
{
    return xGoSurfaceIntensityMsg_CastRaw(msg)->exposure;
}

/*
 * GoSectionMsg
 */
kBeginClassEx(Go, GoSectionMsg)

    // Serialization versions. Serialzier Type Id string must match the sensor's
    // value of the corresonding message type.
    kAddVersion(GoSectionMsg, GO_SERIALIZATION_FORMAT_NAME, GO_SERIALIZATION_FORMAT_VERSION, GoSerializerTypeIdStr(GO_COMPACT_MESSAGE_SECTION), WriteV20, ReadV20)
    kAddVersion(GoSectionMsg, "kdat6", "4.0.0.0", "GoSectionMsg-20", WriteV20, ReadV20)

    //virtual methods
    kAddVMethod(GoSectionMsg, GoDataMsg, VInit)
    kAddVMethod(GoSectionMsg, kObject, VInitClone)
    kAddVMethod(GoSectionMsg, kObject, VRelease)
    kAddVMethod(GoSectionMsg, kObject, VSize)

kEndClassEx()

GoFx(kStatus) GoSectionMsg_Construct(GoSectionMsg* msg, kAlloc allocator)
{
    kAlloc alloc = kAlloc_Fallback(allocator);
    kStatus status;

    kCheck(kAlloc_GetObject(alloc, kTypeOf(GoSectionMsg), msg));

    if (!kSuccess(status = GoSectionMsg_VInit(*msg, kTypeOf(GoSectionMsg), alloc)))
    {
        kAlloc_FreeRef(alloc, msg);
    }

    return status;
}

GoFx(kStatus) GoSectionMsg_VInit(GoSectionMsg msg, kType type, kAlloc alloc)
{
    kObjR(GoSectionMsg, msg);

    kCheck(GoDataMsg_Init(msg, type, GO_DATA_MESSAGE_TYPE_SECTION, alloc));

    obj->id = 0;
    obj->source = GO_DATA_SOURCE_TOP;
    obj->xResolution = 0;
    obj->zResolution = 0;
    obj->xOffset = 0;
    obj->zOffset = 0;
    obj->xPose = 0;
    obj->yPose = 0;
    obj->anglePose = 0;
    kZero(obj->content);
    obj->exposure = 0;

    GoDataMsg_SetStreamStep(msg, GO_DATA_STEP_SECTION);
    GoDataMsg_SetStreamStepId(msg, obj->id);

    return kOK;
}

GoFx(kStatus) GoSectionMsg_VInitClone(GoSectionMsg msg, GoSectionMsg source, kAlloc alloc)
{
    kObjR(GoSectionMsg, msg);
    kObjNR(GoSectionMsg, srcObj, source);
    kStatus status;

    kCheck(GoSectionMsg_VInit(msg, kObject_Type(source), alloc));

    kTry
    {
        obj->id = srcObj->id;
        obj->source = srcObj->source;
        obj->xResolution = srcObj->xResolution;
        obj->zResolution = srcObj->zResolution;
        obj->xPose = srcObj->xPose;
        obj->yPose = srcObj->yPose;
        obj->anglePose = srcObj->anglePose;
        obj->xOffset = srcObj->xOffset;
        obj->zOffset = srcObj->zOffset;
        obj->exposure = srcObj->exposure;
        obj->base.streamStep = GoDataMsg_StreamStep(source);
        obj->base.streamStepId = GoDataMsg_StreamStepId(source);

        kTest(kObject_Clone(&obj->content, srcObj->content, alloc));
    }
    kCatch(&status)
    {
        GoSectionMsg_VRelease(msg);
        kEndCatch(status);
    }

    return kOK;
}

GoFx(kStatus) GoSectionMsg_Allocate(GoSectionMsg msg, kSize count, kSize width)
{
    kObjR(GoSectionMsg, msg);

    kCheck(kDestroyRef(&obj->content));
    kCheck(kArray2_Construct(&obj->content, kTypeOf(k16s), count, width, kObject_Alloc(msg)));

    return kOK;
}

GoFx(kSize) GoSectionMsg_VSize(GoSectionMsg msg)
{
    kObjR(GoSectionMsg, msg);

    return sizeof(GoSectionMsgClass) + (kIsNull(obj->content) ? 0 : kObject_Size(obj->content));
}

GoFx(kStatus) GoSectionMsg_VRelease(GoSectionMsg msg)
{
    kObjR(GoSectionMsg, msg);

    kCheck(kObject_Destroy(obj->content));

    kCheck(kObject_VRelease(msg));

    return kOK;
}

GoFx(kStatus) GoSectionMsg_WriteV20(GoSectionMsg msg, kSerializer serializer)
{
    kObjR(GoSectionMsg, msg);
    k32u count = (k32u) kArray2_Length(obj->content, 0);
    k32u width = (k32u) kArray2_Length(obj->content, 1);

    kCheck(kSerializer_BeginWrite(serializer, kTypeOf(k16u), kFALSE));

    kCheck(kSerializer_Write32u(serializer, count));
    kCheck(kSerializer_Write32u(serializer, width));
    kCheck(kSerializer_Write32u(serializer, obj->xResolution));
    kCheck(kSerializer_Write32u(serializer, obj->zResolution));
    kCheck(kSerializer_Write32s(serializer, obj->xOffset));
    kCheck(kSerializer_Write32s(serializer, obj->zOffset));
    kCheck(kSerializer_Write8u(serializer, (k8u)obj->source));
    kCheck(kSerializer_Write32u(serializer, obj->id));
    kCheck(kSerializer_Write32u(serializer, obj->exposure));
    kCheck(kSerializer_Write32s(serializer, obj->anglePose));
    kCheck(kSerializer_Write32s(serializer, obj->xPose));
    kCheck(kSerializer_Write32s(serializer, obj->yPose));
    kCheck(kSerializer_Write32s(serializer, GoDataMsg_StreamStep(msg)));
    kCheck(kSerializer_Write32s(serializer, GoDataMsg_StreamStepId(msg)));

    kCheck(kSerializer_EndWrite(serializer));

    kCheck(kSerializer_Write16sArray(serializer, kArray2_DataT(obj->content, k16s), count * width));

    return kOK;
}

GoFx(kStatus) GoSectionMsg_ReadV20(GoSectionMsg msg, kSerializer serializer, kAlloc alloc)
{
    kObjR(GoSectionMsg, msg);
    kStatus status;
    k32u count, width;
    k8u source;

    kCheck(GoSectionMsg_VInit(msg, kTypeOf(GoSectionMsg), alloc));

    kTry
    {
        // Read and store the message attributes data length ("attrSize")
        // to determine how many attributes to read.
        kTest(kSerializer_BeginRead(serializer, kTypeOf(k16u), kFALSE));

        kTest(kSerializer_Read32u(serializer, &count));
        kTest(kSerializer_Read32u(serializer, &width));
        kTest(kSerializer_Read32u(serializer, &obj->xResolution));
        kTest(kSerializer_Read32u(serializer, &obj->zResolution));
        kTest(kSerializer_Read32s(serializer, &obj->xOffset));
        kTest(kSerializer_Read32s(serializer, &obj->zOffset));
        kTest(kSerializer_Read8u(serializer, &source));
        kTest(kSerializer_Read32u(serializer, &obj->id));
        kTest(kSerializer_Read32u(serializer, &obj->exposure));
        kTest(kSerializer_Read32s(serializer, &obj->anglePose));
        kTest(kSerializer_Read32s(serializer, &obj->xPose));
        kTest(kSerializer_Read32s(serializer, &obj->yPose));

        kTest(GoSectionMsg_ReadV20AttrProtoVer2(msg, serializer));

        // Skip over attributes this version of the SDK protocol does not
        // know about.
        kTest(kSerializer_EndRead(serializer));

        kTest(kArray2_Construct(&obj->content, kTypeOf(k16s), count, width, alloc));
        kTest(kSerializer_Read16sArray(serializer, kArray2_DataT(obj->content, k16s), count * width));

        obj->source = (GoDataSource)source;
    }
    kCatch(&status)
    {
        GoSectionMsg_VRelease(msg);
        kEndCatch(status);
    }

    return kOK;
}

GoFx(k32u) GoSectionMsg_Id(GoSectionMsg msg)
{
    return xGoSectionMsg_CastRaw(msg)->id;
}

GoFx(kStatus) GoSectionMsg_ReadV20AttrProtoVer2(GoSectionMsg msg, kSerializer serializer)
{
    if (!kSerializer_ReadCompleted(serializer))
    {
        return GoDataMsg_ReadStreamStepAndId(msg, serializer);
    }

    return kOK;
}

GoFx(GoDataSource) GoSectionMsg_Source(GoSectionMsg msg)
{
    return xGoSectionMsg_CastRaw(msg)->source;
}

GoFx(kSize) GoSectionMsg_Count(GoSectionMsg msg)
{
    return kArray2_Length(GoSectionMsg_Content_(msg), 0);
}

GoFx(kSize) GoSectionMsg_Width(GoSectionMsg msg)
{
    return kArray2_Length(GoSectionMsg_Content_(msg), 1);
}

GoFx(k32u) GoSectionMsg_XResolution(GoSectionMsg msg)
{
    return xGoSectionMsg_CastRaw(msg)->xResolution;
}

GoFx(k32u) GoSectionMsg_ZResolution(GoSectionMsg msg)
{
    return xGoSectionMsg_CastRaw(msg)->zResolution;
}

GoFx(k32s) GoSectionMsg_XPose(GoSectionMsg msg)
{
    return xGoSectionMsg_CastRaw(msg)->xPose;
}

GoFx(k32s) GoSectionMsg_YPose(GoSectionMsg msg)
{
    return xGoSectionMsg_CastRaw(msg)->yPose;
}

GoFx(k32s) GoSectionMsg_AnglePose(GoSectionMsg msg)
{
    return xGoSectionMsg_CastRaw(msg)->anglePose;
}

GoFx(k32s) GoSectionMsg_XOffset(GoSectionMsg msg)
{
    return xGoSectionMsg_CastRaw(msg)->xOffset;
}

GoFx(k32s) GoSectionMsg_ZOffset(GoSectionMsg msg)
{
    return xGoSectionMsg_CastRaw(msg)->zOffset;
}

GoFx(k16s*) GoSectionMsg_At(GoSectionMsg msg, kSize index)
{
    kAssert(index < GoSectionMsg_Count(msg));

    return kArray2_AtT(GoSectionMsg_Content_(msg), index, 0, k16s);
}

GoFx(k32u) GoSectionMsg_Exposure(GoSectionMsg msg)
{
    return xGoSectionMsg_CastRaw(msg)->exposure;
}

/*
 * GoSectionIntensityMsg
 */
kBeginClassEx(Go, GoSectionIntensityMsg)

    // Serialization versions. Serialzier Type Id string must match the sensor's
    // value of the corresonding message type.
    kAddVersion(GoSectionIntensityMsg, GO_SERIALIZATION_FORMAT_NAME, GO_SERIALIZATION_FORMAT_VERSION, GoSerializerTypeIdStr(GO_COMPACT_MESSAGE_SECTION_INTENSITY), WriteV21, ReadV21)
    kAddVersion(GoSectionIntensityMsg, "kdat6", "4.0.0.0", "GoSectionIntensityMsg-21", WriteV21, ReadV21)

    //virtual methods
    kAddVMethod(GoSectionIntensityMsg, GoDataMsg, VInit)
    kAddVMethod(GoSectionIntensityMsg, kObject, VInitClone)
    kAddVMethod(GoSectionIntensityMsg, kObject, VRelease)
    kAddVMethod(GoSectionIntensityMsg, kObject, VSize)

kEndClassEx()

GoFx(kStatus) GoSectionIntensityMsg_Construct(GoSectionIntensityMsg* msg, kAlloc allocator)
{
    kAlloc alloc = kAlloc_Fallback(allocator);
    kStatus status;

    kCheck(kAlloc_GetObject(alloc, kTypeOf(GoSectionIntensityMsg), msg));

    if (!kSuccess(status = GoSectionIntensityMsg_VInit(*msg, kTypeOf(GoSectionIntensityMsg), alloc)))
    {
        kAlloc_FreeRef(alloc, msg);
    }

    return status;
}

GoFx(kStatus) GoSectionIntensityMsg_VInit(GoSectionIntensityMsg msg, kType type, kAlloc alloc)
{
    kObjR(GoSectionIntensityMsg, msg);

    kCheck(GoDataMsg_Init(msg, type, GO_DATA_MESSAGE_TYPE_SECTION_INTENSITY, alloc));

    obj->id = 0;
    obj->source = GO_DATA_SOURCE_TOP;
    obj->xResolution = 0;
    obj->xOffset = 0;
    obj->xPose = 0;
    obj->yPose = 0;
    obj->anglePose = 0;
    kZero(obj->content);
    obj->exposure = 0;

    GoDataMsg_SetStreamStep(msg, GO_DATA_STEP_SECTION);
    GoDataMsg_SetStreamStepId(msg, obj->id);

    return kOK;
}

GoFx(kStatus) GoSectionIntensityMsg_VInitClone(GoSectionIntensityMsg msg, GoSectionIntensityMsg source, kAlloc alloc)
{
    kObjR(GoSectionIntensityMsg, msg);
    kObjNR(GoSectionIntensityMsg, srcObj, source);
    kStatus status;

    kCheck(GoSectionIntensityMsg_VInit(msg, kObject_Type(source), alloc));

    kTry
    {
        obj->id = srcObj->id;
        obj->source = srcObj->source;
        obj->xResolution = srcObj->xResolution;
        obj->xPose = srcObj->xPose;
        obj->yPose = srcObj->yPose;
        obj->anglePose = srcObj->anglePose;
        obj->xOffset = srcObj->xOffset;
        obj->exposure = srcObj->exposure;
        obj->base.streamStep = GoDataMsg_StreamStep(source);
        obj->base.streamStepId= GoDataMsg_StreamStepId(source);

        kTest(kObject_Clone(&obj->content, srcObj->content, alloc));
    }
    kCatch(&status)
    {
        GoSectionIntensityMsg_VRelease(msg);
        kEndCatch(status);
    }

    return kOK;
}

GoFx(kStatus) GoSectionIntensityMsg_Allocate(GoSectionIntensityMsg msg, kSize count, kSize width)
{
    kObjR(GoSectionIntensityMsg, msg);

    kCheck(kDestroyRef(&obj->content));
    kCheck(kArray2_Construct(&obj->content, kTypeOf(k8u), count, width, kObject_Alloc(msg)));

    return kOK;
}

GoFx(kSize) GoSectionIntensityMsg_VSize(GoSectionIntensityMsg msg)
{
    kObjR(GoSectionIntensityMsg, msg);

    return sizeof(GoSectionIntensityMsgClass) + (kIsNull(obj->content) ? 0 : kObject_Size(obj->content));
}

GoFx(kStatus) GoSectionIntensityMsg_VRelease(GoSectionIntensityMsg msg)
{
    kObjR(GoSectionIntensityMsg, msg);

    kCheck(kObject_Destroy(obj->content));

    kCheck(kObject_VRelease(msg));

    return kOK;
}

GoFx(kStatus) GoSectionIntensityMsg_WriteV21(GoSectionIntensityMsg msg, kSerializer serializer)
{
    kObjR(GoSectionIntensityMsg, msg);
    k32u count = (k32u) kArray2_Length(obj->content, 0);
    k32u width = (k32u) kArray2_Length(obj->content, 1);

    kCheck(kSerializer_BeginWrite(serializer, kTypeOf(k16u), kFALSE));

    kCheck(kSerializer_Write32u(serializer, count));
    kCheck(kSerializer_Write32u(serializer, width));
    kCheck(kSerializer_Write32u(serializer, obj->xResolution));
    kCheck(kSerializer_Write32s(serializer, obj->xOffset));
    kCheck(kSerializer_Write8u(serializer, (k8u)obj->source));
    kCheck(kSerializer_Write32u(serializer, obj->id));
    kCheck(kSerializer_Write32u(serializer, obj->exposure));
    kCheck(kSerializer_Write32s(serializer, obj->anglePose));
    kCheck(kSerializer_Write32s(serializer, obj->xPose));
    kCheck(kSerializer_Write32s(serializer, obj->yPose));
    kCheck(kSerializer_Write32s(serializer, GoDataMsg_StreamStep(msg)));
    kCheck(kSerializer_Write32s(serializer, GoDataMsg_StreamStepId(msg)));

    kCheck(kSerializer_EndWrite(serializer));

    kCheck(kSerializer_Write8uArray(serializer, kArray2_DataT(obj->content, k8u), count * width));

    return kOK;
}

GoFx(kStatus) GoSectionIntensityMsg_ReadV21(GoSectionIntensityMsg msg, kSerializer serializer, kAlloc alloc)
{
    kObjR(GoSectionIntensityMsg, msg);
    kStatus status;
    k32u count, width;
    k8u source;

    kCheck(GoSectionIntensityMsg_VInit(msg, kTypeOf(GoSectionIntensityMsg), alloc));

    kTry
    {
        // Read and store the message attributes data length ("attrSize")
        // to determine how many attributes to read.
        kTest(kSerializer_BeginRead(serializer, kTypeOf(k16u), kFALSE));

        kTest(kSerializer_Read32u(serializer, &count));
        kTest(kSerializer_Read32u(serializer, &width));
        kTest(kSerializer_Read32u(serializer, &obj->xResolution));
        kTest(kSerializer_Read32s(serializer, &obj->xOffset));
        kTest(kSerializer_Read8u(serializer, &source));
        kTest(kSerializer_Read32u(serializer, &obj->id));
        kTest(kSerializer_Read32u(serializer, &obj->exposure));
        kTest(kSerializer_Read32s(serializer, &obj->anglePose));
        kTest(kSerializer_Read32s(serializer, &obj->xPose));
        kTest(kSerializer_Read32s(serializer, &obj->yPose));

        kTest(GoSectionIntensityMsg_ReadV21AttrProtoVer2(msg, serializer));

        // Skip over attributes this version of the SDK protocol does not
        // know about.
        kTest(kSerializer_EndRead(serializer));

        kTest(kArray2_Construct(&obj->content, kTypeOf(k8u), count, width, alloc));
        kTest(kSerializer_Read8uArray(serializer, kArray2_DataT(obj->content, k8u), count * width));

        obj->source = (GoDataSource)source;
    }
    kCatch(&status)
    {
        GoSectionIntensityMsg_VRelease(msg);
        kEndCatch(status);
    }

    return kOK;
}

GoFx(k32u) GoSectionIntensityMsg_Id(GoSectionIntensityMsg msg)
{
    return xGoSectionIntensityMsg_CastRaw(msg)->id;
}

GoFx(kStatus) GoSectionIntensityMsg_ReadV21AttrProtoVer2(GoSectionIntensityMsg msg, kSerializer serializer)
{
    if (!kSerializer_ReadCompleted(serializer))
    {
        return GoDataMsg_ReadStreamStepAndId(msg, serializer);
    }

    return kOK;
}

GoFx(GoDataSource) GoSectionIntensityMsg_Source(GoSectionIntensityMsg msg)
{
    return xGoSectionIntensityMsg_CastRaw(msg)->source;
}

GoFx(kSize) GoSectionIntensityMsg_Count(GoSectionIntensityMsg msg)
{
    return kArray2_Length(GoSectionIntensityMsg_Content_(msg), 0);
}

GoFx(kSize) GoSectionIntensityMsg_Width(GoSectionIntensityMsg msg)
{
    return kArray2_Length(GoSectionIntensityMsg_Content_(msg), 1);
}

GoFx(k32u) GoSectionIntensityMsg_XResolution(GoSectionIntensityMsg msg)
{
    return xGoSectionIntensityMsg_CastRaw(msg)->xResolution;
}

GoFx(k32s) GoSectionIntensityMsg_XPose(GoSectionIntensityMsg msg)
{
    return xGoSectionIntensityMsg_CastRaw(msg)->xPose;
}

GoFx(k32s) GoSectionIntensityMsg_YPose(GoSectionIntensityMsg msg)
{
    return xGoSectionIntensityMsg_CastRaw(msg)->yPose;
}

GoFx(k32s) GoSectionIntensityMsg_AnglePose(GoSectionIntensityMsg msg)
{
    return xGoSectionIntensityMsg_CastRaw(msg)->anglePose;
}

GoFx(k32s) GoSectionIntensityMsg_XOffset(GoSectionIntensityMsg msg)
{
    return xGoSectionIntensityMsg_CastRaw(msg)->xOffset;
}

GoFx(k8u*) GoSectionIntensityMsg_At(GoSectionIntensityMsg msg, kSize index)
{
    kAssert(index < GoSectionIntensityMsg_Count(msg));

    return kArray2_AtT(GoSectionIntensityMsg_Content_(msg), index, 0, k8u);
}

GoFx(k32u) GoSectionIntensityMsg_Exposure(GoSectionIntensityMsg msg)
{
    return xGoSectionIntensityMsg_CastRaw(msg)->exposure;
}

/*
 * GoMeasurementData
 */
kBeginValueEx(Go, GoMeasurementData)
    kAddField(GoMeasurementData, k64f, value)
    kAddField(GoMeasurementData, k8u, decision)
    kAddField(GoMeasurementData, k8u, decisionCode)
kEndValueEx()

GoFx(k64f) GoMeasurementData_Value(GoMeasurementData data)
{
    return data.value;
}

GoFx(GoDecision) GoMeasurementData_Decision(GoMeasurementData data)
{
    return data.decision;
}

GoFx(GoDecisionCode) GoMeasurementData_DecisionCode(GoMeasurementData data)
{
    return data.decisionCode;
}

/*
 * GoMeasurementMsg
 */

kBeginClassEx(Go, GoMeasurementMsg)

    // Serialization versions. Serialzier Type Id string must match the sensor's
    // value of the corresonding message type.
    kAddVersion(GoMeasurementMsg, GO_SERIALIZATION_FORMAT_NAME, GO_SERIALIZATION_FORMAT_VERSION, GoSerializerTypeIdStr(GO_COMPACT_MESSAGE_MEASUREMENT), WriteV10, ReadV10)
    kAddVersion(GoMeasurementMsg, "kdat6", "4.0.0.0", "GoMeasurementMsg-10", WriteV10, ReadV10)

    //virtual methods
    kAddVMethod(GoMeasurementMsg, GoDataMsg, VInit)
    kAddVMethod(GoMeasurementMsg, kObject, VInitClone)
    kAddVMethod(GoMeasurementMsg, kObject, VRelease)
    kAddVMethod(GoMeasurementMsg, kObject, VSize)

kEndClassEx()

GoFx(kStatus) GoMeasurementMsg_Construct(GoMeasurementMsg* msg, kAlloc allocator)
{
    kAlloc alloc = kAlloc_Fallback(allocator);
    kStatus status;

    kCheck(kAlloc_GetObject(alloc, kTypeOf(GoMeasurementMsg), msg));

    if (!kSuccess(status = GoMeasurementMsg_VInit(*msg, kTypeOf(GoMeasurementMsg), alloc)))
    {
        kAlloc_FreeRef(alloc, msg);
    }

    return status;
}

GoFx(kStatus) GoMeasurementMsg_VInit(GoMeasurementMsg msg, kType type, kAlloc alloc)
{
    kObjR(GoMeasurementMsg, msg);

    kCheck(GoDataMsg_Init(msg, type, GO_DATA_MESSAGE_TYPE_MEASUREMENT, alloc));

    obj->measurements = kNULL;
    obj->id = 0;

    return kOK;
}

GoFx(kStatus) GoMeasurementMsg_VInitClone(GoMeasurementMsg msg, GoMeasurementMsg source, kAlloc alloc)
{
    kObjR(GoMeasurementMsg, msg);
    kObjNR(GoMeasurementMsg, srcObj, source);
    kStatus status;

    kCheck(GoMeasurementMsg_VInit(msg, kObject_Type(source), alloc));

    kTry
    {
        obj->id = srcObj->id;

        kTest(kObject_Clone(&obj->measurements, srcObj->measurements, alloc));
    }
    kCatch(&status)
    {
        GoMeasurementMsg_VRelease(msg);
        kEndCatch(status);
    }

    return kOK;
}

GoFx(kStatus) GoMeasurementMsg_Allocate(GoMeasurementMsg msg, kSize count)
{
    kObjR(GoMeasurementMsg, msg);

    kCheck(kDestroyRef(&obj->measurements));
    kCheck(kArray1_Construct(&obj->measurements, kTypeOf(GoMeasurementData), count, kObject_Alloc(msg)));

    return kOK;
}

GoFx(kStatus) GoMeasurementMsg_VRelease(GoMeasurementMsg msg)
{
    kObjR(GoMeasurementMsg, msg);

    kCheck(kObject_Destroy(obj->measurements));

    kCheck(kObject_VRelease(msg));

    return kOK;
}

GoFx(kSize) GoMeasurementMsg_VSize(GoMeasurementMsg msg)
{
    kObjR(GoMeasurementMsg, msg);

    return sizeof(GoMeasurementMsgClass) + (kIsNull(obj->measurements) ? 0 : kObject_Size(obj->measurements));
}

GoFx(kStatus) GoMeasurementMsg_WriteV10(GoMeasurementMsg msg, kSerializer serializer)
{
    kObjR(GoMeasurementMsg, msg);
    k32u count = (k32u) kArray1_Count(obj->measurements);
    k32u i;

    kCheck(kSerializer_Write32u(serializer, count));
    kCheck(kSerializer_Write8u(serializer, 0));
    kCheck(kSerializer_Write8u(serializer, 0));
    kCheck(kSerializer_Write16u(serializer, obj->id));

    for (i = 0; i < count; ++i)
    {
        const GoMeasurementData* measurement = kArray1_AtT(obj->measurements, i, GoMeasurementData);

        if (measurement->value != k64F_NULL)
        {
            kCheck(kSerializer_Write32s(serializer, (k32s)(measurement->value * 1000.0)));
        }
        else
        {
            kCheck(kSerializer_Write32s(serializer, k32S_NULL));
        }

        kCheck(kSerializer_Write8u(serializer, (measurement->decision | (measurement->decisionCode << 1))));
        kCheck(kSerializer_Write8u(serializer, 0));
        kCheck(kSerializer_Write8u(serializer, 0));
        kCheck(kSerializer_Write8u(serializer, 0));
    }

    return kOK;
}

GoFx(kStatus) GoMeasurementMsg_ReadV10(GoMeasurementMsg msg, kSerializer serializer, kAlloc alloc)
{
    kObjR(GoMeasurementMsg, msg);
    kStatus status;
    k32u count = 0;
    k32u i;
    k32s intVal;
    k8u decision, reserved;

    kCheck(GoMeasurementMsg_VInit(msg, kTypeOf(GoMeasurementMsg), alloc));

    kTry
    {
        kTest(kSerializer_Read32u(serializer, &count));
        kTest(kSerializer_Read8u(serializer, &reserved));
        kTest(kSerializer_Read8u(serializer, &reserved));
        kTest(kSerializer_Read16u(serializer, &obj->id));

        kTest(kArray1_Construct(&obj->measurements, kTypeOf(GoMeasurementData), count, alloc));

        for (i = 0; i < count; ++i)
        {
            GoMeasurementData* measurement = kArray1_AtT(obj->measurements, i, GoMeasurementData);

            kTest(kSerializer_Read32s(serializer, &intVal));
            if (intVal != k32S_NULL)
            {
                measurement->value = intVal / 1000.0;
            }
            else
            {
                measurement->value = k64F_NULL;
            }

            kTest(kSerializer_Read8u(serializer, &decision));
            kTest(kSerializer_Read8u(serializer, &reserved));
            kTest(kSerializer_Read8u(serializer, &reserved));
            kTest(kSerializer_Read8u(serializer, &reserved));

            measurement->decision = (decision & GO_DECISION_PASS);
            measurement->decisionCode = (decision >> 1);
        }
    }
    kCatch(&status)
    {
        GoMeasurementMsg_VRelease(msg);
        kEndCatch(status);
    }

    return kOK;
}

GoFx(k16u) GoMeasurementMsg_Id(GoMeasurementMsg msg)
{
    return xGoMeasurementMsg_CastRaw(msg)->id;
}

GoFx(kSize) GoMeasurementMsg_Count(GoMeasurementMsg msg)
{
    return kArray1_Count(GoMeasurementMsg_Content_(msg));
}

GoFx(GoMeasurementData*) GoMeasurementMsg_At(GoMeasurementMsg msg, kSize index)
{
    kAssert(index < GoMeasurementMsg_Count(msg));

    return kArray1_AtT(GoMeasurementMsg_Content_(msg), index, GoMeasurementData);
}


/*
 * GoCalMsg
 */

kBeginClassEx(Go, GoAlignMsg)

    // Serialization versions. Serialzier Type Id string must match the sensor's
    // value of the corresonding message type.
    kAddVersion(GoAlignMsg, GO_SERIALIZATION_FORMAT_NAME, GO_SERIALIZATION_FORMAT_VERSION, GoSerializerTypeIdStr(GO_COMPACT_MESSAGE_OPERATION_RESULT), WriteV11, ReadV11)
    kAddVersion(GoAlignMsg, "kdat6", "4.0.0.0", "GoAlignMsg-11", WriteV11, ReadV11)

    //virtual methods
    kAddVMethod(GoAlignMsg, GoDataMsg, VInit)
    kAddVMethod(GoAlignMsg, kObject, VInitClone)
    kAddVMethod(GoAlignMsg, kObject, VRelease)
    kAddVMethod(GoAlignMsg, kObject, VSize)

kEndClassEx()

GoFx(kStatus) GoAlignMsg_Construct(GoAlignMsg* msg, kAlloc allocator)
{
    kAlloc alloc = kAlloc_Fallback(allocator);
    kStatus status;

    kCheck(kAlloc_GetObject(alloc, kTypeOf(GoAlignMsg), msg));

    if (!kSuccess(status = GoAlignMsg_VInit(*msg, kTypeOf(GoAlignMsg), alloc)))
    {
        kAlloc_FreeRef(alloc, msg);
    }

    return status;
}

GoFx(kStatus) GoAlignMsg_VInit(GoAlignMsg msg, kType type, kAlloc alloc)
{
    kObjR(GoAlignMsg, msg);

    kCheck(GoDataMsg_Init(msg, type, GO_DATA_MESSAGE_TYPE_ALIGNMENT, alloc));

    obj->status = kOK;

    return kOK;
}

GoFx(kStatus) GoAlignMsg_VInitClone(GoAlignMsg msg, GoAlignMsg source, kAlloc alloc)
{
    kObjR(GoAlignMsg, msg);
    kObjNR(GoAlignMsg, srcObj, source);

    kCheck(GoAlignMsg_VInit(msg, kObject_Type(source), alloc));

    obj->status = srcObj->status;

    return kOK;
}

GoFx(kStatus) GoAlignMsg_VRelease(GoAlignMsg msg)
{
    kCheck(kObject_VRelease(msg));

    return kOK;
}

GoFx(kSize) GoAlignMsg_VSize(GoAlignMsg msg)
{
    return sizeof(GoAlignMsgClass);
}

GoFx(kStatus) GoAlignMsg_WriteV11(GoAlignMsg msg, kSerializer serializer)
{
    kObjR(GoAlignMsg, msg);

    kCheck(kSerializer_BeginWrite(serializer, kTypeOf(k16u), kFALSE));

    kCheck(kSerializer_Write32u(serializer, obj->opId));
    kCheck(kSerializer_Write32s(serializer, obj->status));

    kCheck(kSerializer_EndWrite(serializer));

    return kOK;
}

GoFx(kStatus) GoAlignMsg_ReadV11(GoAlignMsg msg, kSerializer serializer, kAlloc alloc)
{
    kObjR(GoAlignMsg, msg);
    kStatus status;

    kCheck(GoAlignMsg_VInit(msg, kTypeOf(GoAlignMsg), alloc));

    kTry
    {
        // Read and store the message attributes data length ("attrSize")
        // to determine how many attributes to read.
        kTest(kSerializer_BeginRead(serializer, kTypeOf(k16u), kFALSE));

        kTest(kSerializer_Read32u(serializer, &obj->opId));
        kTest(kSerializer_Read32s(serializer, &obj->status));

        // Skip over attributes this version of the SDK protocol does not
        // know about.
        kTest(kSerializer_EndRead(serializer));
    }
    kCatch(&status)
    {
        GoAlignMsg_VRelease(msg);
        kEndCatch(status);
    }

    return kOK;
}

GoFx(GoAlignmentStatus) GoAlignMsg_Status(GoAlignMsg msg)
{
    return xGoAlignMsg_CastRaw(msg)->status;
}



/*
 * GoExposureCalMsg
 */

kBeginClassEx(Go, GoExposureCalMsg)

    // Serialization versions. Serialzier Type Id string must match the sensor's
    // value of the corresonding message type.
    kAddVersion(GoExposureCalMsg, GO_SERIALIZATION_FORMAT_NAME, GO_SERIALIZATION_FORMAT_VERSION, GoSerializerTypeIdStr(GO_COMPACT_MESSAGE_EXPOSURE_CAL_RESULT), WriteV12, ReadV12)
    kAddVersion(GoExposureCalMsg, "kdat6", "4.0.0.0", "GoExposureCalMsg-12", WriteV12, ReadV12)

    //virtual methods
    kAddVMethod(GoExposureCalMsg, GoDataMsg, VInit)
    kAddVMethod(GoExposureCalMsg, kObject, VInitClone)
    kAddVMethod(GoExposureCalMsg, kObject, VRelease)
    kAddVMethod(GoExposureCalMsg, kObject, VSize)

kEndClassEx()

GoFx(kStatus) GoExposureCalMsg_Construct(GoExposureCalMsg* msg, kAlloc allocator)
{
    kAlloc alloc = kAlloc_Fallback(allocator);
    kStatus status;

    kCheck(kAlloc_GetObject(alloc, kTypeOf(GoExposureCalMsg), msg));

    if (!kSuccess(status = GoExposureCalMsg_VInit(*msg, kTypeOf(GoExposureCalMsg), alloc)))
    {
        kAlloc_FreeRef(alloc, msg);
    }

    return status;
}

GoFx(kStatus) GoExposureCalMsg_VInit(GoExposureCalMsg msg, kType type, kAlloc alloc)
{
    kObjR(GoExposureCalMsg, msg);

    kCheck(GoDataMsg_Init(msg, type, GO_DATA_MESSAGE_TYPE_EXPOSURE_CAL, alloc));

    obj->status = kOK;
    obj->exposure = k32U_MAX;

    return kOK;
}

GoFx(kStatus) GoExposureCalMsg_VInitClone(GoExposureCalMsg msg, GoExposureCalMsg source, kAlloc alloc)
{
    kObjR(GoExposureCalMsg, msg);
    kObjNR(GoExposureCalMsg, srcObj, source);

    kCheck(GoExposureCalMsg_VInit(msg, kObject_Type(source), alloc));

    obj->status = srcObj->status;
    obj->exposure = srcObj->exposure;

    return kOK;
}

GoFx(kStatus) GoExposureCalMsg_VRelease(GoExposureCalMsg msg)
{
    kCheck(kObject_VRelease(msg));

    return kOK;
}

GoFx(kSize) GoExposureCalMsg_VSize(GoExposureCalMsg msg)
{
    return sizeof(GoExposureCalMsgClass);
}

GoFx(kStatus) GoExposureCalMsg_WriteV12(GoExposureCalMsg msg, kSerializer serializer)
{
    kObjR(GoExposureCalMsg, msg);

    kCheck(kSerializer_BeginWrite(serializer, kTypeOf(k16u), kFALSE));

    kCheck(kSerializer_Write32u(serializer, obj->opId));
    kCheck(kSerializer_Write32s(serializer, obj->status));

    kCheck(kSerializer_EndWrite(serializer));

    kCheck(kSerializer_Write32u(serializer, obj->exposure));


    return kOK;
}

GoFx(kStatus) GoExposureCalMsg_ReadV12(GoExposureCalMsg msg, kSerializer serializer, kAlloc alloc)
{
    kObjR(GoExposureCalMsg, msg);
    kStatus status;

    kCheck(GoExposureCalMsg_VInit(msg, kTypeOf(GoExposureCalMsg), alloc));

    kTry
    {
        // Read and store the message attributes data length ("attrSize")
        // to determine how many attributes to read.
        kTest(kSerializer_BeginRead(serializer, kTypeOf(k16u), kFALSE));

        kTest(kSerializer_Read32u(serializer, &obj->opId));
        kTest(kSerializer_Read32s(serializer, &obj->status));

        // Skip over attributes this version of the SDK protocol does not
        // know about.
        kTest(kSerializer_EndRead(serializer));

        kTest(kSerializer_Read32u(serializer, &obj->exposure));
    }
    kCatch(&status)
    {
        GoExposureCalMsg_VRelease(msg);
        kEndCatch(status);
    }

    return kOK;
}

GoFx(kStatus) GoExposureCalMsg_Status(GoExposureCalMsg msg)
{
    return xGoExposureCalMsg_CastRaw(msg)->status;
}

GoFx(k64f) GoExposureCalMsg_Exposure(GoExposureCalMsg msg)
{
    if (xGoExposureCalMsg_CastRaw(msg)->exposure == k32U_NULL)
    {
        return k64F_NULL;
    }

    return (k64f)(xGoExposureCalMsg_CastRaw(msg)->exposure / 1000.0);
}


/*
 * GoEdgeMatchMsg
 */

kBeginClassEx(Go, GoEdgeMatchMsg)

    // Serialization versions. Serialzier Type Id string must match the sensor's
    // value of the corresonding message type.
    kAddVersion(GoEdgeMatchMsg, GO_SERIALIZATION_FORMAT_NAME, GO_SERIALIZATION_FORMAT_VERSION, GoSerializerTypeIdStr(GO_COMPACT_MESSAGE_EDGE_MATCH), WriteV16, ReadV16)
    kAddVersion(GoEdgeMatchMsg, "kdat6", "4.0.0.0", "GoEdgeMatchMsg-16", WriteV16, ReadV16)

    //virtual methods
    kAddVMethod(GoEdgeMatchMsg, GoDataMsg, VInit)
    kAddVMethod(GoEdgeMatchMsg, kObject, VInitClone)
    kAddVMethod(GoEdgeMatchMsg, kObject, VRelease)
    kAddVMethod(GoEdgeMatchMsg, kObject, VSize)

kEndClassEx()

GoFx(kStatus) GoEdgeMatchMsg_Construct(GoEdgeMatchMsg* msg, kAlloc allocator)
{
    kAlloc alloc = kAlloc_Fallback(allocator);
    kStatus status;

    kCheck(kAlloc_GetObject(alloc, kTypeOf(GoEdgeMatchMsg), msg));

    if (!kSuccess(status = GoEdgeMatchMsg_VInit(*msg, kTypeOf(GoEdgeMatchMsg), alloc)))
    {
        kAlloc_FreeRef(alloc, msg);
    }

    return status;
}

GoFx(kStatus) GoEdgeMatchMsg_VInit(GoEdgeMatchMsg msg, kType type, kAlloc alloc)
{
    kObjR(GoEdgeMatchMsg, msg);

    kCheck(GoDataMsg_Init(msg, type, GO_DATA_MESSAGE_TYPE_EDGE_MATCH, alloc));
    obj->decision = 0;
    obj->xOffset = 0.0;
    obj->yOffset = 0.0;
    obj->zAngle = 0.0;
    obj->qualityValue = 0.0;
    obj->qualityDecision = 0;

    return kOK;
}

GoFx(kStatus) GoEdgeMatchMsg_VInitClone(GoEdgeMatchMsg msg, GoEdgeMatchMsg source, kAlloc alloc)
{
    kObjR(GoEdgeMatchMsg, msg);
    kObjNR(GoEdgeMatchMsg, srcObj, source);

    kCheck(GoEdgeMatchMsg_VInit(msg, kObject_Type(source), alloc));

    obj->decision = srcObj->decision;
    obj->xOffset = srcObj->xOffset;
    obj->yOffset = srcObj->yOffset;
    obj->zAngle = srcObj->zAngle;
    obj->qualityValue = srcObj->qualityValue;
    obj->qualityDecision = srcObj->qualityDecision;

    return kOK;
}

GoFx(kStatus) GoEdgeMatchMsg_VRelease(GoEdgeMatchMsg msg)
{
    kCheck(kObject_VRelease(msg));

    return kOK;
}

GoFx(kSize) GoEdgeMatchMsg_VSize(GoEdgeMatchMsg msg)
{
    return sizeof(GoEdgeMatchMsgClass);
}

GoFx(kStatus) GoEdgeMatchMsg_WriteV16(GoEdgeMatchMsg msg, kSerializer serializer)
{
    kObjR(GoEdgeMatchMsg, msg);

    kCheck(kSerializer_Write8u(serializer, obj->decision));
    kCheck(kSerializer_Write32s(serializer, (k32s)(obj->xOffset * 1000)));
    kCheck(kSerializer_Write32s(serializer, (k32s)(obj->yOffset * 1000)));
    kCheck(kSerializer_Write32s(serializer, (k32s)(obj->zAngle * 1000)));
    kCheck(kSerializer_Write32s(serializer, (k32s)(obj->qualityValue * 1000)));
    kCheck(kSerializer_Write8u(serializer, obj->qualityDecision));
    kCheck(kSerializer_Write8u(serializer, 0));
    kCheck(kSerializer_Write8u(serializer, 0));

    return kOK;
}

GoFx(kStatus) GoEdgeMatchMsg_ReadV16(GoEdgeMatchMsg msg, kSerializer serializer, kAlloc alloc)
{
    kObjR(GoEdgeMatchMsg, msg);
    kStatus status;
    k8u reserved;
    k32s temp;

    kCheck(GoEdgeMatchMsg_VInit(msg, kTypeOf(GoEdgeMatchMsg), alloc));

    kTry
    {
        kTest(kSerializer_Read8u(serializer, &obj->decision));
        kTest(kSerializer_Read32s(serializer, &temp));
        obj->xOffset = temp / 1000.0;
        kTest(kSerializer_Read32s(serializer, &temp));
        obj->yOffset = temp / 1000.0;
        kTest(kSerializer_Read32s(serializer, &temp));
        obj->zAngle = temp / 1000.0;
        kTest(kSerializer_Read32s(serializer, &temp));
        obj->qualityValue = temp / 1000.0;
        kTest(kSerializer_Read8u(serializer, &obj->qualityDecision));
        kTest(kSerializer_Read8u(serializer, &reserved));
        kTest(kSerializer_Read8u(serializer, &reserved));
    }
    kCatch(&status)
    {
        GoEdgeMatchMsg_VRelease(msg);
        kEndCatch(status);
    }

    return kOK;
}

GoFx(k8u) GoEdgeMatchMsg_Decision(GoEdgeMatchMsg msg)
{
    return xGoEdgeMatchMsg_CastRaw(msg)->decision;
}

GoFx(k64f) GoEdgeMatchMsg_XOffset(GoEdgeMatchMsg msg)
{
    return xGoEdgeMatchMsg_CastRaw(msg)->xOffset;
}

GoFx(k64f) GoEdgeMatchMsg_YOffset(GoEdgeMatchMsg msg)
{
    return xGoEdgeMatchMsg_CastRaw(msg)->yOffset;
}

GoFx(k64f) GoEdgeMatchMsg_ZAngle(GoEdgeMatchMsg msg)
{
    return xGoEdgeMatchMsg_CastRaw(msg)->zAngle;
}

GoFx(k64f) GoEdgeMatchMsg_QualityValue(GoEdgeMatchMsg msg)
{
    return xGoEdgeMatchMsg_CastRaw(msg)->qualityValue;
}

GoFx(k8u) GoEdgeMatchMsg_QualityDecision(GoEdgeMatchMsg msg)
{
    return xGoEdgeMatchMsg_CastRaw(msg)->qualityDecision;
}



/*
 * GoBoundingBoxMatchMsg
 */

kBeginClassEx(Go, GoBoundingBoxMatchMsg)

    // Serialization versions. Serialzier Type Id string must match the sensor's
    // value of the corresonding message type.
    kAddVersion(GoBoundingBoxMatchMsg, GO_SERIALIZATION_FORMAT_NAME, GO_SERIALIZATION_FORMAT_VERSION, GoSerializerTypeIdStr(GO_COMPACT_MESSAGE_BOX_MATCH), WriteV17, ReadV17)
    kAddVersion(GoBoundingBoxMatchMsg, "kdat6", "4.0.0.0", "GoBoundingBoxMatchMsg-17", WriteV17, ReadV17)

    //virtual methods
    kAddVMethod(GoBoundingBoxMatchMsg, GoDataMsg, VInit)
    kAddVMethod(GoBoundingBoxMatchMsg, kObject, VInitClone)
    kAddVMethod(GoBoundingBoxMatchMsg, kObject, VRelease)
    kAddVMethod(GoBoundingBoxMatchMsg, kObject, VSize)

kEndClassEx()

GoFx(kStatus) GoBoundingBoxMatchMsg_Construct(GoBoundingBoxMatchMsg* msg, kAlloc allocator)
{
    kAlloc alloc = kAlloc_Fallback(allocator);
    kStatus status;

    kCheck(kAlloc_GetObject(alloc, kTypeOf(GoBoundingBoxMatchMsg), msg));

    if (!kSuccess(status = GoBoundingBoxMatchMsg_VInit(*msg, kTypeOf(GoBoundingBoxMatchMsg), alloc)))
    {
        kAlloc_FreeRef(alloc, msg);
    }

    return status;
}

GoFx(kStatus) GoBoundingBoxMatchMsg_VInit(GoBoundingBoxMatchMsg msg, kType type, kAlloc alloc)
{
    kObjR(GoBoundingBoxMatchMsg, msg);

    kCheck(GoDataMsg_Init(msg, type, GO_DATA_MESSAGE_TYPE_BOUNDING_BOX_MATCH, alloc));
    obj->decision = 0;
    obj->xOffset = 0.0;
    obj->yOffset = 0.0;
    obj->zAngle = 0.0;
    obj->lengthValue = 0.0;
    obj->lengthDecision = 0;
    obj->widthValue = 0.0;
    obj->widthDecision = 0;

    return kOK;
}

GoFx(kStatus) GoBoundingBoxMatchMsg_VInitClone(GoBoundingBoxMatchMsg msg, GoBoundingBoxMatchMsg source, kAlloc alloc)
{
    kObjR(GoBoundingBoxMatchMsg, msg);
    kObjNR(GoBoundingBoxMatchMsg, srcObj, source);

    kCheck(GoBoundingBoxMatchMsg_VInit(msg, kObject_Type(source), alloc));

    obj->decision = srcObj->decision;
    obj->xOffset = srcObj->xOffset;
    obj->yOffset = srcObj->yOffset;
    obj->zAngle = srcObj->zAngle;
    obj->lengthValue = srcObj->lengthValue;
    obj->lengthDecision = srcObj->lengthDecision;
    obj->widthValue = srcObj->widthValue;
    obj->widthDecision = srcObj->widthDecision;

    return kOK;
}

GoFx(kStatus) GoBoundingBoxMatchMsg_VRelease(GoBoundingBoxMatchMsg msg)
{
    kCheck(kObject_VRelease(msg));

    return kOK;
}

GoFx(kSize) GoBoundingBoxMatchMsg_VSize(GoBoundingBoxMatchMsg msg)
{
    return sizeof(GoBoundingBoxMatchMsgClass);
}

GoFx(kStatus) GoBoundingBoxMatchMsg_WriteV17(GoBoundingBoxMatchMsg msg, kSerializer serializer)
{
    kObjR(GoBoundingBoxMatchMsg, msg);

    kCheck(kSerializer_Write8u(serializer, obj->decision));
    kCheck(kSerializer_Write32s(serializer, (k32s)(obj->xOffset * 1000)));
    kCheck(kSerializer_Write32s(serializer, (k32s)(obj->yOffset * 1000)));
    kCheck(kSerializer_Write32s(serializer, (k32s)(obj->zAngle * 1000)));
    kCheck(kSerializer_Write32s(serializer, (k32s)(obj->widthValue * 1000)));
    kCheck(kSerializer_Write8u(serializer, obj->widthDecision));
    kCheck(kSerializer_Write32s(serializer, (k32s)(obj->lengthValue * 1000)));
    kCheck(kSerializer_Write8u(serializer, obj->lengthDecision));

    return kOK;
}

GoFx(kStatus) GoBoundingBoxMatchMsg_ReadV17(GoBoundingBoxMatchMsg msg, kSerializer serializer, kAlloc alloc)
{
    kObjR(GoBoundingBoxMatchMsg, msg);
    kStatus status;
    k32s temp;

    kCheck(GoBoundingBoxMatchMsg_VInit(msg, kTypeOf(GoBoundingBoxMatchMsg), alloc));

    kTry
    {
        kTest(kSerializer_Read8s(serializer, &obj->decision));
        kTest(kSerializer_Read32s(serializer, &temp));
        obj->xOffset = temp / 1000.0;
        kTest(kSerializer_Read32s(serializer, &temp));
        obj->yOffset = temp / 1000.0;
        kTest(kSerializer_Read32s(serializer, &temp));
        obj->zAngle = temp / 1000.0;
        kTest(kSerializer_Read32s(serializer, &temp));
        obj->widthValue = temp / 1000.0;
        kTest(kSerializer_Read8u(serializer, &obj->widthDecision));
        kTest(kSerializer_Read32s(serializer, &temp));
        obj->lengthValue = temp / 1000.0;
        kTest(kSerializer_Read8u(serializer, &obj->lengthDecision));
    }
    kCatch(&status)
    {
        GoBoundingBoxMatchMsg_VRelease(msg);
        kEndCatch(status);
    }

    return kOK;
}

GoFx(k8u) GoBoundingBoxMatchMsg_Decision(GoBoundingBoxMatchMsg msg)
{
    return xGoBoundingBoxMatchMsg_CastRaw(msg)->decision;
}

GoFx(k64f) GoBoundingBoxMatchMsg_XOffset(GoBoundingBoxMatchMsg msg)
{
    return xGoBoundingBoxMatchMsg_CastRaw(msg)->xOffset;
}

GoFx(k64f) GoBoundingBoxMatchMsg_YOffset(GoBoundingBoxMatchMsg msg)
{
    return xGoBoundingBoxMatchMsg_CastRaw(msg)->yOffset;
}

GoFx(k64f) GoBoundingBoxMatchMsg_ZAngle(GoBoundingBoxMatchMsg msg)
{
    return xGoBoundingBoxMatchMsg_CastRaw(msg)->zAngle;
}

GoFx(k64f) GoBoundingBoxMatchMsg_LengthValue(GoBoundingBoxMatchMsg msg)
{
    return xGoBoundingBoxMatchMsg_CastRaw(msg)->lengthValue;
}

GoFx(k8u) GoBoundingBoxMatchMsg_LengthDecision(GoBoundingBoxMatchMsg msg)
{
    return xGoBoundingBoxMatchMsg_CastRaw(msg)->lengthDecision;
}

GoFx(k64f) GoBoundingBoxMatchMsg_WidthValue(GoBoundingBoxMatchMsg msg)
{
    return xGoBoundingBoxMatchMsg_CastRaw(msg)->widthValue;
}

GoFx(k8u) GoBoundingBoxMatchMsg_WidthDecision(GoBoundingBoxMatchMsg msg)
{
    return xGoBoundingBoxMatchMsg_CastRaw(msg)->widthDecision;
}


/*
 * GoEllipseMatchMsg
 */
kBeginClassEx(Go, GoEllipseMatchMsg)

    // Serialization versions. Serialzier Type Id string must match the sensor's
    // value of the corresonding message type.
    kAddVersion(GoEllipseMatchMsg, GO_SERIALIZATION_FORMAT_NAME, GO_SERIALIZATION_FORMAT_VERSION, GoSerializerTypeIdStr(GO_COMPACT_MESSAGE_ELLIPSE_MATCH), WriteV18, ReadV18)
    kAddVersion(GoEllipseMatchMsg, "kdat6", "4.0.0.0", "GoEllipseMatchMsg-18", WriteV18, ReadV18)

    //virtual methods
    kAddVMethod(GoEllipseMatchMsg, GoDataMsg, VInit)
    kAddVMethod(GoEllipseMatchMsg, kObject, VInitClone)
    kAddVMethod(GoEllipseMatchMsg, kObject, VRelease)
    kAddVMethod(GoEllipseMatchMsg, kObject, VSize)

kEndClassEx()

GoFx(kStatus) GoEllipseMatchMsg_Construct(GoEllipseMatchMsg* msg, kAlloc allocator)
{
    kAlloc alloc = kAlloc_Fallback(allocator);
    kStatus status;

    kCheck(kAlloc_GetObject(alloc, kTypeOf(GoEllipseMatchMsg), msg));

    if (!kSuccess(status = GoEllipseMatchMsg_VInit(*msg, kTypeOf(GoEllipseMatchMsg), alloc)))
    {
        kAlloc_FreeRef(alloc, msg);
    }

    return status;
}

GoFx(kStatus) GoEllipseMatchMsg_VInit(GoEllipseMatchMsg msg, kType type, kAlloc alloc)
{
    kObjR(GoEllipseMatchMsg, msg);

    kCheck(GoDataMsg_Init(msg, type, GO_DATA_MESSAGE_TYPE_ELLIPSE_MATCH, alloc));
    obj->decision = 0;
    obj->xOffset = 0.0;
    obj->yOffset = 0.0;
    obj->zAngle = 0.0;
    obj->majorValue = 0.0;
    obj->majorDecision = 0;
    obj->minorValue = 0.0;
    obj->minorDecision = 0;

    return kOK;
}

GoFx(kStatus) GoEllipseMatchMsg_VInitClone(GoEllipseMatchMsg msg, GoEllipseMatchMsg source, kAlloc alloc)
{
    kObjR(GoEllipseMatchMsg, msg);
    kObjNR(GoEllipseMatchMsg, srcObj, source);

    kCheck(GoEllipseMatchMsg_VInit(msg, kObject_Type(source), alloc));

    obj->decision = srcObj->decision;
    obj->xOffset = srcObj->xOffset;
    obj->yOffset = srcObj->yOffset;
    obj->zAngle = srcObj->zAngle;
    obj->majorValue = srcObj->majorValue;
    obj->majorDecision = srcObj->majorDecision;
    obj->minorValue = srcObj->minorValue;
    obj->minorDecision = srcObj->minorDecision;

    return kOK;
}

GoFx(kStatus) GoEllipseMatchMsg_VRelease(GoEllipseMatchMsg msg)
{
    kCheck(kObject_VRelease(msg));

    return kOK;
}

GoFx(kSize) GoEllipseMatchMsg_VSize(GoEllipseMatchMsg msg)
{
    return sizeof(GoEllipseMatchMsgClass);
}

GoFx(kStatus) GoEllipseMatchMsg_WriteV18(GoEllipseMatchMsg msg, kSerializer serializer)
{
    kObjR(GoEllipseMatchMsg, msg);

    kCheck(kSerializer_Write8u(serializer, obj->decision));
    kCheck(kSerializer_Write32s(serializer, (k32s)(obj->xOffset * 1000)));
    kCheck(kSerializer_Write32s(serializer, (k32s)(obj->yOffset * 1000)));
    kCheck(kSerializer_Write32s(serializer, (k32s)(obj->zAngle * 1000)));
    kCheck(kSerializer_Write32s(serializer, (k32s)(obj->minorValue * 1000)));
    kCheck(kSerializer_Write8u(serializer, obj->minorDecision));
    kCheck(kSerializer_Write32s(serializer, (k32s)(obj->majorValue * 1000)));
    kCheck(kSerializer_Write8u(serializer, obj->majorDecision));

    return kOK;
}

GoFx(kStatus) GoEllipseMatchMsg_ReadV18(GoEllipseMatchMsg msg, kSerializer serializer, kAlloc alloc)
{
    kObjR(GoEllipseMatchMsg, msg);
    kStatus status;
    k32s temp;

    kCheck(GoEllipseMatchMsg_VInit(msg, kTypeOf(GoEllipseMatchMsg), alloc));

    kTry
    {
        kTest(kSerializer_Read8s(serializer, &obj->decision));
        kTest(kSerializer_Read32s(serializer, &temp));
        obj->xOffset = temp / 1000.0;
        kTest(kSerializer_Read32s(serializer, &temp));
        obj->yOffset = temp / 1000.0;
        kTest(kSerializer_Read32s(serializer, &temp));
        obj->zAngle = temp / 1000.0;
        kTest(kSerializer_Read32s(serializer, &temp));
        obj->minorValue = temp / 1000.0;
        kTest(kSerializer_Read8u(serializer, &obj->minorDecision));
        kTest(kSerializer_Read32s(serializer, &temp));
        obj->majorValue = temp / 1000.0;
        kTest(kSerializer_Read8u(serializer, &obj->majorDecision));
    }
    kCatch(&status)
    {
        GoEllipseMatchMsg_VRelease(msg);
        kEndCatch(status);
    }

    return kOK;
}

GoFx(k8u) GoEllipseMatchMsg_Decision(GoEllipseMatchMsg msg)
{
    return xGoEllipseMatchMsg_CastRaw(msg)->decision;
}

GoFx(k64f) GoEllipseMatchMsg_XOffset(GoEllipseMatchMsg msg)
{
    return xGoEllipseMatchMsg_CastRaw(msg)->xOffset;
}

GoFx(k64f) GoEllipseMatchMsg_YOffset(GoEllipseMatchMsg msg)
{
    return xGoEllipseMatchMsg_CastRaw(msg)->yOffset;
}

GoFx(k64f) GoEllipseMatchMsg_ZAngle(GoEllipseMatchMsg msg)
{
    return xGoEllipseMatchMsg_CastRaw(msg)->zAngle;
}

GoFx(k64f) GoEllipseMatchMsg_MajorValue(GoEllipseMatchMsg msg)
{
    return xGoEllipseMatchMsg_CastRaw(msg)->majorValue;
}

GoFx(k8u) GoEllipseMatchMsg_MajorDecision(GoEllipseMatchMsg msg)
{
    return xGoEllipseMatchMsg_CastRaw(msg)->majorDecision;
}

GoFx(k64f) GoEllipseMatchMsg_MinorValue(GoEllipseMatchMsg msg)
{
    return xGoEllipseMatchMsg_CastRaw(msg)->minorValue;
}

GoFx(k8u) GoEllipseMatchMsg_MinorDecision(GoEllipseMatchMsg msg)
{
    return xGoEllipseMatchMsg_CastRaw(msg)->minorDecision;
}


/*
 * GoEventMsg
 */
kBeginClassEx(Go, GoEventMsg)

    // Serialization versions. Serialzier Type Id string must match the sensor's
    // value of the corresonding message type.
    kAddVersion(GoEventMsg, GO_SERIALIZATION_FORMAT_NAME, GO_SERIALIZATION_FORMAT_VERSION, GoSerializerTypeIdStr(GO_COMPACT_MESSAGE_EVENT), WriteV22, ReadV22)
    kAddVersion(GoEventMsg, "kdat6", "4.0.0.0", "GoEventMsg-22", WriteV22, ReadV22)

    //virtual methods
    kAddVMethod(GoEventMsg, GoDataMsg, VInit)
    kAddVMethod(GoEventMsg, kObject, VInitClone)
    kAddVMethod(GoEventMsg, kObject, VRelease)
    kAddVMethod(GoEventMsg, kObject, VSize)

kEndClassEx()

GoFx(kStatus) GoEventMsg_Construct(GoEventMsg* msg, kAlloc allocator)
{
    kAlloc alloc = kAlloc_Fallback(allocator);
    kStatus status;

    kCheck(kAlloc_GetObject(alloc, kTypeOf(GoEventMsg), msg));

    if (!kSuccess(status = GoEventMsg_VInit(*msg, kTypeOf(GoEventMsg), alloc)))
    {
        kAlloc_FreeRef(alloc, msg);
    }

    return status;
}

GoFx(kStatus) GoEventMsg_VInit(GoEventMsg msg, kType type, kAlloc alloc)
{
    kObjR(GoEventMsg, msg);

    kCheck(GoDataMsg_Init(msg, type, GO_DATA_MESSAGE_TYPE_EVENT, alloc));
    obj->type = 0;

    return kOK;
}

GoFx(kStatus) GoEventMsg_VInitClone(GoEventMsg msg, GoEventMsg source, kAlloc alloc)
{
    kObjR(GoEventMsg, msg);
    kObjNR(GoEventMsg, srcObj, source);

    kCheck(GoEventMsg_VInit(msg, kObject_Type(source), alloc));

    obj->type = srcObj->type;

    return kOK;
}

GoFx(kStatus) GoEventMsg_VRelease(GoEventMsg msg)
{
    kCheck(kObject_VRelease(msg));

    return kOK;
}

GoFx(kSize) GoEventMsg_VSize(GoEventMsg msg)
{
    return sizeof(GoEventMsgClass);
}

GoFx(kStatus) GoEventMsg_WriteV22(GoEventMsg msg, kSerializer serializer)
{
    kObjR(GoEventMsg, msg);

    kCheck(kSerializer_BeginWrite(serializer, kTypeOf(k16u), kFALSE));

    kCheck(kSerializer_Write32s(serializer, obj->type));
    kCheck(kSerializer_Write32u(serializer, 0));

    kCheck(kSerializer_EndWrite(serializer));

    return kOK;
}

GoFx(kStatus) GoEventMsg_ReadV22(GoEventMsg msg, kSerializer serializer, kAlloc alloc)
{
    kObjR(GoEventMsg, msg);
    kStatus status;
    k32u temp;

    kCheck(GoEventMsg_VInit(msg, kTypeOf(GoEventMsg), alloc));

    kTry
    {
        // Read and store the message attributes data length ("attrSize")
        // to determine how many attributes to read.
        kTest(kSerializer_BeginRead(serializer, kTypeOf(k16u), kFALSE));

        kTest(kSerializer_Read32s(serializer, &obj->type));
        kTest(kSerializer_Read32u(serializer, &temp));

        // Skip over attributes this version of the SDK protocol does not
        // know about.
        kTest(kSerializer_EndRead(serializer));
    }
    kCatch(&status)
    {
        GoEventMsg_VRelease(msg);
        kEndCatch(status);
    }

    return kOK;
}

GoFx(GoEventType) GoEventMsg_Type(GoEventMsg msg)
{
    return xGoEventMsg_CastRaw(msg)->type;
}

/*
 * GoTracheidMsg
 */
kBeginClassEx(Go, GoTracheidMsg)

    // Serialization versions. Serialzier Type Id string must match the sensor's
    // value of the corresonding message type.
    kAddVersion(GoTracheidMsg, GO_SERIALIZATION_FORMAT_NAME, GO_SERIALIZATION_FORMAT_VERSION, GoSerializerTypeIdStr(GO_COMPACT_MESSAGE_TRACHEID), WriteV23, ReadV23)
    kAddVersion(GoTracheidMsg, "kdat6", "4.0.0.0", "GoTracheidMsg-23", WriteV23, ReadV23)

    //virtual methods
    kAddVMethod(GoTracheidMsg, GoDataMsg, VInit)
    kAddVMethod(GoTracheidMsg, kObject, VInitClone)
    kAddVMethod(GoTracheidMsg, kObject, VRelease)
    kAddVMethod(GoTracheidMsg, kObject, VSize)

kEndClassEx()

GoFx(kStatus) GoTracheidMsg_Construct(GoTracheidMsg* msg, kAlloc allocator)
{
    kAlloc alloc = kAlloc_Fallback(allocator);
    kStatus status;

    kCheck(kAlloc_GetObject(alloc, kTypeOf(GoTracheidMsg), msg));

    if (!kSuccess(status = GoTracheidMsg_VInit(*msg, kTypeOf(GoTracheidMsg), alloc)))
    {
        kAlloc_FreeRef(alloc, msg);
    }

    return status;
}

GoFx(kStatus) GoTracheidMsg_VInit(GoTracheidMsg msg, kType type, kAlloc alloc)
{
    kObjR(GoTracheidMsg, msg);

    kCheck(GoDataMsg_Init(msg, type, GO_DATA_MESSAGE_TYPE_TRACHEID, alloc));
    obj->source = GO_DATA_SOURCE_TOP;
    obj->cameraIndex = 0;
    kZero(obj->moments);
    kZero(obj->ellipses);

    return kOK;
}

GoFx(kStatus) GoTracheidMsg_VInitClone(GoTracheidMsg msg, GoTracheidMsg source, kAlloc alloc)
{
    kObjR(GoTracheidMsg, msg);
    kObjNR(GoTracheidMsg, srcObj, source);

    kCheck(GoTracheidMsg_VInit(msg, kObject_Type(source), alloc));

    obj->source = srcObj->source;
    obj->cameraIndex = srcObj->cameraIndex;

    kCheck(kObject_Clone(&obj->moments, srcObj->moments, alloc));
    kCheck(kObject_Clone(&obj->ellipses, srcObj->ellipses, alloc));

    return kOK;
}

GoFx(kStatus) GoTracheidMsg_VRelease(GoTracheidMsg msg)
{
    kObjR(GoTracheidMsg, msg);

    kCheck(kDestroyRef(&obj->moments));
    kCheck(kDestroyRef(&obj->ellipses));

    kCheck(kObject_VRelease(msg));

    return kOK;
}

GoFx(kSize) GoTracheidMsg_VSize(GoTracheidMsg msg)
{
    return sizeof(GoTracheidMsgClass);
}

GoFx(kStatus) GoTracheidMsg_WriteV23(GoTracheidMsg msg, kSerializer serializer)
{
    kObjR(GoTracheidMsg, msg);

    kSize count = kArray2_Length(obj->moments, 1);
    kSize momentCount = kArray2_Length(obj->moments, 0);
    kSize spotCount = momentCount / GO_TRACHEID_MOMENTS_PER_SPOT;
    kSize size = count * momentCount;

    kCheck(kSerializer_BeginWrite(serializer, kTypeOf(k16u), kFALSE));

    kCheck(kSerializer_Write32u(serializer, (k32u)count));
    kCheck(kSerializer_Write32u(serializer, (k32u)spotCount));
    kCheck(kSerializer_Write8u(serializer, (k8u)obj->source));
    kCheck(kSerializer_Write8u(serializer, (k8u)obj->cameraIndex));

    kCheck(kSerializer_EndWrite(serializer));

    if (size > 0)
    {
        kCheck(kSerializer_Write32sArray(serializer, kArray2_DataT(obj->moments, k32s), size));
    }

    return kOK;
}

GoFx(kStatus) GoTracheidMsg_ReadV23(GoTracheidMsg msg, kSerializer serializer, kAlloc alloc)
{
    kObjR(GoTracheidMsg, msg);
    kStatus status;
    k32u count, spotCount, momentCount;
    kSize size;
    k8u source;

    kCheck(GoTracheidMsg_VInit(msg, kTypeOf(GoTracheidMsg), alloc));

    kTry
    {
        // Read and store the message attributes data length ("attrSize")
        // to determine how many attributes to read.
        kTest(kSerializer_BeginRead(serializer, kTypeOf(k16u), kFALSE));

        kTest(kSerializer_Read32u(serializer, &count));
        kTest(kSerializer_Read32u(serializer, &spotCount));
        kTest(kSerializer_Read8u(serializer, &source));
        obj->source = (GoDataSource)source;
        kTest(kSerializer_Read8u(serializer, &obj->cameraIndex));

        // Skip over attributes this version of the SDK protocol does not
        // know about.
        kTest(kSerializer_EndRead(serializer));

        // 6 moments per spot
        momentCount = spotCount * GO_TRACHEID_MOMENTS_PER_SPOT;

        kTest(kArray2_Construct(&obj->moments, kTypeOf(k32s), count, momentCount, alloc));
        size = (kSize)(count * momentCount);

        if (size > 0)
        {
            kTest(kSerializer_Read32sArray(serializer, kArray2_DataT(obj->moments, k32s), size));

            // now that we have the moments, calculate the ellipses
            kTest(GoTracheidMsg_GenerateEllipses(obj, alloc));
        }
    }
    kCatch(&status)
    {
        GoTracheidMsg_VRelease(msg);
        kEndCatch(status);
    }

    return kOK;
}

GoFx(GoDataSource) GoTracheidMsg_Source(GoTracheidMsg msg)
{
    return xGoTracheidMsg_CastRaw(msg)->source;
}

GoFx(kSize) GoTracheidMsg_Count(GoTracheidMsg msg)
{
    return kArray2_Length(xGoTracheidMsg_CastRaw(msg)->ellipses, 0);
}

GoFx(kSize) GoTracheidMsg_Width(GoTracheidMsg msg)
{
    return kArray2_Length(xGoTracheidMsg_CastRaw(msg)->ellipses, 1);
}

GoFx(k8u) GoTracheidMsg_CameraIndex(GoTracheidMsg msg)
{
    return xGoTracheidMsg_CastRaw(msg)->cameraIndex;
}

GoFx(GoTracheidEllipse*) GoTracheidMsg_At(GoTracheidMsg msg, kSize index)
{
    kAssert(index < GoTracheidMsg_Count(msg));

    return kArray2_AtT(xGoTracheidMsg_CastRaw(msg)->ellipses, index, 0, GoTracheidEllipse);
}

/*
 * GoTracheidEllipse
 */
kBeginValueEx(Go, GoTracheidEllipse)
    kAddField(GoTracheidEllipse, k64f, area)
    kAddField(GoTracheidEllipse, k64f, angle)
    kAddField(GoTracheidEllipse, k64f, scatter)
    kAddField(GoTracheidEllipse, k64f, minor)
    kAddField(GoTracheidEllipse, k64f, major)
kEndValueEx()

GoFx(kStatus) GoTracheidMsg_GenerateEllipses(GoTracheidMsg msg, kAlloc alloc)
{
    GoTracheidMsgClass* K_ATTRIBUTE_UNUSED obj = xGoTracheidMsg_CastRaw(msg);

    k64f fSumX, fSumY, fSumXX, fSumYY, fSumXY, fSum;
    k64f a, b, c, e, f;
    k64f xc, yc;
    k64f angleOut, areaOut, scatterOut, minorOut, majorOut;
    kSize count = kArray2_Length(obj->moments, 0);
    kSize momentCount = kArray2_Length(obj->moments, 1);
    kSize spotCount = momentCount / GO_TRACHEID_MOMENTS_PER_SPOT;
    kSize i, j, momentIdx;
    GoTracheidEllipse* ellipse;

    kCheck(kArray2_Construct(&obj->ellipses, kTypeOf(GoTracheidEllipse), count, spotCount, alloc));

    for (j = 0; j < count; j++)
    {
        for (i = 0; i < spotCount; i++)
        {
            angleOut = k64F_NULL;
            areaOut = k64F_NULL;
            scatterOut = k64F_NULL;
            minorOut = k64F_NULL;
            majorOut = k64F_NULL;

            momentIdx = i * GO_TRACHEID_MOMENTS_PER_SPOT;

            fSum = (k64f)*kArray2_AtT(obj->moments, j, momentIdx, k32s);
            fSumX = (k64f)*kArray2_AtT(obj->moments, j, momentIdx + 1, k32s);
            fSumY = (k64f)*kArray2_AtT(obj->moments, j, momentIdx + 2, k32s);
            fSumXX = (k64f)*kArray2_AtT(obj->moments, j, momentIdx + 3, k32s);
            fSumXY = (k64f)*kArray2_AtT(obj->moments, j, momentIdx + 4, k32s);
            fSumYY = (k64f)*kArray2_AtT(obj->moments, j, momentIdx + 5, k32s);

            a = b = f = 0;

            if (fSum > 0)
            {
                xc = fSumX / fSum;
                yc = fSumY / fSum;
                a = (fSumXX - xc*xc * fSum) * 4 / kMATH_PI;
                b = (fSumYY - yc*yc * fSum) * 4 / kMATH_PI;
                c = (fSumXY - xc*yc * fSum) * 4 / kMATH_PI;
                e = a - b;
                f = sqrt(e*e + 4 * c*c);

                if ((c != 0) || (e != 0))
                {
                    angleOut = kMath_RadToDeg_(atan2(2 * c, e) / 2);
                    while (angleOut < 0)
                    {
                        angleOut += 180;
                    }
                    while (angleOut >= 180)
                    {
                        angleOut -= 180;
                    }
                }

                if ((fSum > 0) && (f >= 0))
                {
                    minorOut = sqrt(6 * kMATH_PI*(a + b - f) / (fSum * 4));
                    majorOut = sqrt(6 * kMATH_PI*(a + b + f) / (fSum * 4));

                    if ((majorOut > 0) && (minorOut >= 0))
                    {
                        scatterOut = minorOut / majorOut;
                        areaOut = kMATH_PI * (minorOut)* (majorOut) / 4;
                    }
                }
            }

            if (areaOut == k64F_NULL || angleOut == k64F_NULL || scatterOut == k64F_NULL || minorOut == k64F_NULL || majorOut == k64F_NULL)
            {
                areaOut = k64F_NULL;
                angleOut = k64F_NULL;
                scatterOut = k64F_NULL;
                minorOut = k64F_NULL;
                majorOut = k64F_NULL;
            }

            ellipse = kArray2_AtT(obj->ellipses, j, i, GoTracheidEllipse);

            ellipse->area = areaOut;
            ellipse->angle = angleOut;
            ellipse->scatter = scatterOut;
            ellipse->minor = minorOut;
            ellipse->major = majorOut;
        }
    }

    return kOK;
}
/*
* GoFeatureMsg
*/
GoFx(kStatus) GoFeatureMsg_WritePoint(kSerializer serializer, kPoint3d64f* point)
{
    kCheck(kSerializer_Write64s(serializer, kMath_Round64s_(point->x*1.0E6)));
    kCheck(kSerializer_Write64s(serializer, kMath_Round64s_(point->y*1.0E6)));
    kCheck(kSerializer_Write64s(serializer, kMath_Round64s_(point->z*1.0E6)));

    return kOK;
}

GoFx(kStatus) GoFeatureMsg_ReadPoint(kSerializer serializer, kPoint3d64f* point)
{
    k64s temp;

    kCheck(kSerializer_Read64s(serializer, &temp));
    point->x = ((k64f)temp) / 1.0E6;
    kCheck(kSerializer_Read64s(serializer, &temp));
    point->y = ((k64f)temp) / 1.0E6;
    kCheck(kSerializer_Read64s(serializer, &temp));
    point->z = ((k64f)temp) / 1.0E6;

    return kOK;
}

/*
* GoPointFeatureMsg
*/
kBeginClassEx(Go, GoPointFeatureMsg)

    // Serialization versions. Serialzier Type Id string must match the sensor's
    // value of the corresonding message type.
    kAddVersion(GoPointFeatureMsg, GO_SERIALIZATION_FORMAT_NAME, GO_SERIALIZATION_FORMAT_VERSION, GoSerializerTypeIdStr(GO_COMPACT_MESSAGE_FEATURE_POINT), WriteV24, ReadV24)
    kAddVersion(GoPointFeatureMsg, "kdat6", "4.0.0.0", "GoPointFeatureMsg-24", WriteV24, ReadV24)

    //virtual methods
    kAddVMethod(GoPointFeatureMsg, GoDataMsg, VInit)
    kAddVMethod(GoPointFeatureMsg, kObject, VInitClone)
    kAddVMethod(GoPointFeatureMsg, kObject, VRelease)
    kAddVMethod(GoPointFeatureMsg, kObject, VSize)

kEndClassEx()

GoFx(kStatus) GoPointFeatureMsg_Construct(GoPointFeatureMsg* msg, kAlloc allocator)
{
    kAlloc alloc = kAlloc_Fallback(allocator);
    kStatus status;

    kCheck(kAlloc_GetObject(alloc, kTypeOf(GoPointFeatureMsg), msg));

    if (!kSuccess(status = GoPointFeatureMsg_VInit(*msg, kTypeOf(GoPointFeatureMsg), alloc)))
    {
        kAlloc_FreeRef(alloc, msg);
    }

    return status;
}

GoFx(kStatus) GoPointFeatureMsg_VInit(GoPointFeatureMsg msg, kType type, kAlloc alloc)
{
    kObjR(GoPointFeatureMsg, msg);

    kCheck(GoDataMsg_Init(msg, type, GO_DATA_MESSAGE_TYPE_FEATURE_POINT, alloc));

    obj->id = 0;

    obj->point.x = 0;
    obj->point.y = 0;
    obj->point.z = 0;

    return kOK;
}

GoFx(kStatus) GoPointFeatureMsg_VInitClone(GoPointFeatureMsg msg, GoPointFeatureMsg source, kAlloc alloc)
{
    kObjR(GoPointFeatureMsg, msg);
    kObjNR(GoPointFeatureMsg, srcObj, source);

    kCheck(GoPointFeatureMsg_VInit(msg, kObject_Type(source), alloc));

    obj->id = srcObj->id;

    obj->point.x = srcObj->point.x;
    obj->point.y = srcObj->point.y;
    obj->point.z = srcObj->point.z;

    return kOK;
}

GoFx(kStatus) GoPointFeatureMsg_VRelease(GoPointFeatureMsg msg)
{
    kObjR(GoPointFeatureMsg, msg);

    return kObject_VRelease(msg);
}

GoFx(kSize)GoPointFeatureMsg_VSize(GoPointFeatureMsg msg)
{
    kObjR(GoPointFeatureMsg, msg);

    return sizeof(GoPointFeatureMsgClass);
}

GoFx(kStatus) GoPointFeatureMsg_WriteV24(GoPointFeatureMsg msg, kSerializer serializer)
{
    kObjR(GoPointFeatureMsg, msg);

    kCheck(kSerializer_Write16u(serializer, obj->id));
    kCheck(GoFeatureMsg_WritePoint(serializer, &obj->point));

    return kOK;
}

GoFx(kStatus) GoPointFeatureMsg_ReadV24(GoPointFeatureMsg msg, kSerializer serializer, kAlloc alloc)
{
    kObjR(GoPointFeatureMsg, msg);

    kCheck(GoPointFeatureMsg_VInit(msg, kTypeOf(GoPointFeatureMsg), alloc));

    kCheck(kSerializer_Read16u(serializer, &obj->id));
    kCheck(GoFeatureMsg_ReadPoint(serializer, &obj->point));

    return kOK;
}

GoFx(k16u) GoPointFeatureMsg_Id(GoPointFeatureMsg msg)
{
    return xGoPointFeatureMsg_CastRaw(msg)->id;
}

GoFx(kPoint3d64f) GoPointFeatureMsg_Position(GoPointFeatureMsg msg)
{
    return xGoPointFeatureMsg_CastRaw(msg)->point;
}

/*
* GoLineFeatureMsg
*/
kBeginClassEx(Go, GoLineFeatureMsg)

    // Serialization versions. Serialzier Type Id string must match the sensor's
    // value of the corresonding message type.
    kAddVersion(GoLineFeatureMsg, GO_SERIALIZATION_FORMAT_NAME, GO_SERIALIZATION_FORMAT_VERSION, GoSerializerTypeIdStr(GO_COMPACT_MESSAGE_FEATURE_LINE), WriteV25, ReadV25)
    kAddVersion(GoLineFeatureMsg, "kdat6", "4.0.0.0", "GoLineFeatureMsg-25", WriteV25, ReadV25)

    //virtual methods
    kAddVMethod(GoLineFeatureMsg, GoDataMsg, VInit)
    kAddVMethod(GoLineFeatureMsg, kObject, VInitClone)
    kAddVMethod(GoLineFeatureMsg, kObject, VRelease)
    kAddVMethod(GoLineFeatureMsg, kObject, VSize)

kEndClassEx()

GoFx(kStatus) GoLineFeatureMsg_Construct(GoLineFeatureMsg* msg, kAlloc allocator)
{
    kAlloc alloc = kAlloc_Fallback(allocator);
    kStatus status;

    kCheck(kAlloc_GetObject(alloc, kTypeOf(GoLineFeatureMsg), msg));

    if (!kSuccess(status = GoLineFeatureMsg_VInit(*msg, kTypeOf(GoLineFeatureMsg), alloc)))
    {
        kAlloc_FreeRef(alloc, msg);
    }

    return status;
}

GoFx(kStatus) GoLineFeatureMsg_VInit(GoLineFeatureMsg msg, kType type, kAlloc alloc)
{
    kObjR(GoLineFeatureMsg, msg);

    kCheck(GoDataMsg_Init(msg, type, GO_DATA_MESSAGE_TYPE_FEATURE_LINE, alloc));

    obj->id = 0;

    obj->point.x = 0;
    obj->point.y = 0;
    obj->point.z = 0;
    obj->direction.x = 0;
    obj->direction.y = 0;
    obj->direction.z = 0;

    return kOK;
}

GoFx(kStatus) GoLineFeatureMsg_VInitClone(GoLineFeatureMsg msg, GoLineFeatureMsg source, kAlloc alloc)
{
    kObjR(GoLineFeatureMsg, msg);
    kObjNR(GoLineFeatureMsg, srcObj, source);

    kCheck(GoLineFeatureMsg_VInit(msg, kObject_Type(source), alloc));

    obj->id = srcObj->id;

    obj->point.x = srcObj->point.x;
    obj->point.y = srcObj->point.y;
    obj->point.z = srcObj->point.z;
    obj->direction.x = srcObj->direction.x;
    obj->direction.y = srcObj->direction.y;
    obj->direction.z = srcObj->direction.z;

    return kOK;
}

GoFx(kStatus) GoLineFeatureMsg_VRelease(GoLineFeatureMsg msg)
{
    kObjR(GoLineFeatureMsg, msg);

    return kObject_VRelease(msg);
}

GoFx(kSize)GoLineFeatureMsg_VSize(GoPointFeatureMsg msg)
{
    kObjR(GoLineFeatureMsg, msg);

    return sizeof(GoLineFeatureMsgClass);
}

GoFx(kStatus) GoLineFeatureMsg_WriteV25(GoLineFeatureMsg msg, kSerializer serializer)
{
    kObjR(GoLineFeatureMsg, msg);

    kCheck(kSerializer_Write16u(serializer, obj->id));
    kCheck(GoFeatureMsg_WritePoint(serializer, &obj->point));
    kCheck(GoFeatureMsg_WritePoint(serializer, &obj->direction));

    return kOK;
}

GoFx(kStatus) GoLineFeatureMsg_ReadV25(GoLineFeatureMsg msg, kSerializer serializer, kAlloc alloc)
{
    kObjR(GoLineFeatureMsg, msg);

    kCheck(GoLineFeatureMsg_VInit(msg, kTypeOf(GoLineFeatureMsg), alloc));

    kCheck(kSerializer_Read16u(serializer, &obj->id));
    kCheck(GoFeatureMsg_ReadPoint(serializer, &obj->point));
    kCheck(GoFeatureMsg_ReadPoint(serializer, &obj->direction));

    return kOK;
}

GoFx(k16u) GoLineFeatureMsg_Id(GoLineFeatureMsg msg)
{
    return xGoLineFeatureMsg_CastRaw(msg)->id;
}

GoFx(kPoint3d64f) GoLineFeatureMsg_Position(GoLineFeatureMsg msg)
{
    return xGoLineFeatureMsg_CastRaw(msg)->point;
}

GoFx(kPoint3d64f) GoLineFeatureMsg_Direction(GoLineFeatureMsg msg)
{
    return xGoLineFeatureMsg_CastRaw(msg)->direction;
}


/*
* GoPlaneFeatureMsg
*/
kBeginClassEx(Go, GoPlaneFeatureMsg)

    // Serialization versions. Serialzier Type Id string must match the sensor's
    // value of the corresonding message type.
    kAddVersion(GoPlaneFeatureMsg, GO_SERIALIZATION_FORMAT_NAME, GO_SERIALIZATION_FORMAT_VERSION, GoSerializerTypeIdStr(GO_COMPACT_MESSAGE_FEATURE_PLANE), WriteV26, ReadV26)
    kAddVersion(GoPlaneFeatureMsg, "kdat6", "4.0.0.0", "GoPlaneFeatureMsg-26", WriteV26, ReadV26)

    //virtual methods
    kAddVMethod(GoPlaneFeatureMsg, GoDataMsg, VInit)
    kAddVMethod(GoPlaneFeatureMsg, kObject, VInitClone)
    kAddVMethod(GoPlaneFeatureMsg, kObject, VRelease)
    kAddVMethod(GoPlaneFeatureMsg, kObject, VSize)

kEndClassEx()

GoFx(kStatus) GoPlaneFeatureMsg_Construct(GoPlaneFeatureMsg* msg, kAlloc allocator)
{
    kAlloc alloc = kAlloc_Fallback(allocator);
    kStatus status;

    kCheck(kAlloc_GetObject(alloc, kTypeOf(GoPlaneFeatureMsg), msg));

    if (!kSuccess(status = GoPlaneFeatureMsg_VInit(*msg, kTypeOf(GoPlaneFeatureMsg), alloc)))
    {
        kAlloc_FreeRef(alloc, msg);
    }

    return status;
}

GoFx(kStatus) GoPlaneFeatureMsg_VInit(GoPlaneFeatureMsg msg, kType type, kAlloc alloc)
{
    kObjR(GoPlaneFeatureMsg, msg);

    kCheck(GoDataMsg_Init(msg, type, GO_DATA_MESSAGE_TYPE_FEATURE_PLANE, alloc));

    obj->id = 0;

    obj->normal.x = 0;
    obj->normal.y = 0;
    obj->normal.z = 0;
    obj->distanceToOrigin = 0;

    return kOK;
}

GoFx(kStatus) GoPlaneFeatureMsg_VInitClone(GoPlaneFeatureMsg msg, GoPlaneFeatureMsg source, kAlloc alloc)
{
    kObjR(GoPlaneFeatureMsg, msg);
    kObjNR(GoPlaneFeatureMsg, srcObj, source);

    kCheck(GoPlaneFeatureMsg_VInit(msg, kObject_Type(source), alloc));

    obj->id = srcObj->id;

    obj->normal.x = srcObj->normal.x;
    obj->normal.y = srcObj->normal.y;
    obj->normal.z = srcObj->normal.z;
    obj->distanceToOrigin = srcObj->distanceToOrigin;

    return kOK;
}

GoFx(kStatus) GoPlaneFeatureMsg_VRelease(GoPlaneFeatureMsg msg)
{
    kObjR(GoPlaneFeatureMsg, msg);

    return kObject_VRelease(msg);
}

GoFx(kSize)GoPlaneFeatureMsg_VSize(GoPointFeatureMsg msg)
{
    kObjR(GoPlaneFeatureMsg, msg);

    return sizeof(GoPlaneFeatureMsgClass);
}

GoFx(kStatus) GoPlaneFeatureMsg_WriteV26(GoPlaneFeatureMsg msg, kSerializer serializer)
{
    kObjR(GoPlaneFeatureMsg, msg);

    kCheck(kSerializer_Write16u(serializer, obj->id));
    kCheck(GoFeatureMsg_WritePoint(serializer, &obj->normal));
    kCheck(kSerializer_Write64s(serializer, kMath_Round64s_(obj->distanceToOrigin*1.0E6)));

    return kOK;
}

GoFx(kStatus) GoPlaneFeatureMsg_ReadV26(GoPlaneFeatureMsg msg, kSerializer serializer, kAlloc alloc)
{
    kObjR(GoPlaneFeatureMsg, msg);
    k64s temp;

    kCheck(GoPlaneFeatureMsg_VInit(msg, kTypeOf(GoPlaneFeatureMsg), alloc));

    kCheck(kSerializer_Read16u(serializer, &obj->id));
    kCheck(GoFeatureMsg_ReadPoint(serializer, &obj->normal));

    kCheck(kSerializer_Read64s(serializer, &temp));
    obj->distanceToOrigin = ((k64f)temp) / 1.0E6;

    return kOK;
}

GoFx(k16u) GoPlaneFeatureMsg_Id(GoPlaneFeatureMsg msg)
{
    return xGoPlaneFeatureMsg_CastRaw(msg)->id;
}

GoFx(kPoint3d64f) GoPlaneFeatureMsg_Normal(GoPlaneFeatureMsg msg)
{
    return xGoPlaneFeatureMsg_CastRaw(msg)->normal;
}

GoFx(k64f) GoPlaneFeatureMsg_DistanceToOrigin(GoPlaneFeatureMsg msg)
{
    return xGoPlaneFeatureMsg_CastRaw(msg)->distanceToOrigin;
}

/*
* GoCircleFeatureMsg
*/
kBeginClassEx(Go, GoCircleFeatureMsg)

    // Serialization versions. Serialzier Type Id string must match the sensor's
    // value of the corresonding message type.
    kAddVersion(GoCircleFeatureMsg, GO_SERIALIZATION_FORMAT_NAME, GO_SERIALIZATION_FORMAT_VERSION, GoSerializerTypeIdStr(GO_COMPACT_MESSAGE_FEATURE_CIRCLE), WriteV27, ReadV27)
    kAddVersion(GoCircleFeatureMsg, "kdat6", "4.0.0.0", "GoCircleFeatureMsg-27", WriteV27, ReadV27)

    //virtual methods
    kAddVMethod(GoCircleFeatureMsg, GoDataMsg, VInit)
    kAddVMethod(GoCircleFeatureMsg, kObject, VInitClone)
    kAddVMethod(GoCircleFeatureMsg, kObject, VRelease)
    kAddVMethod(GoCircleFeatureMsg, kObject, VSize)

kEndClassEx()

GoFx(kStatus) GoCircleFeatureMsg_Construct(GoCircleFeatureMsg* msg, kAlloc allocator)
{
    kAlloc alloc = kAlloc_Fallback(allocator);
    kStatus status;

    kCheck(kAlloc_GetObject(alloc, kTypeOf(GoCircleFeatureMsg), msg));

    if (!kSuccess(status = GoCircleFeatureMsg_VInit(*msg, kTypeOf(GoCircleFeatureMsg), alloc)))
    {
        kAlloc_FreeRef(alloc, msg);
    }

    return status;
}

GoFx(kStatus) GoCircleFeatureMsg_VInit(GoCircleFeatureMsg msg, kType type, kAlloc alloc)
{
    kObjR(GoCircleFeatureMsg, msg);

    kCheck(GoDataMsg_Init(msg, type, GO_DATA_MESSAGE_TYPE_FEATURE_CIRCLE, alloc));

    obj->id = 0;

    obj->normal.x = 0;
    obj->normal.y = 0;
    obj->normal.z = 0;
    obj->center.x = 0;
    obj->center.y = 0;
    obj->center.z = 0;
    obj->radius = 0;

    return kOK;
}

GoFx(kStatus) GoCircleFeatureMsg_VInitClone(GoCircleFeatureMsg msg, GoCircleFeatureMsg source, kAlloc alloc)
{
    kObjR(GoCircleFeatureMsg, msg);
    kObjNR(GoCircleFeatureMsg, srcObj, source);

    kCheck(GoCircleFeatureMsg_VInit(msg, kObject_Type(source), alloc));

    obj->id = srcObj->id;

    obj->normal.x = srcObj->normal.x;
    obj->normal.y = srcObj->normal.y;
    obj->normal.z = srcObj->normal.z;
    obj->center.x = srcObj->center.x;
    obj->center.y = srcObj->center.y;
    obj->center.z = srcObj->center.z;
    obj->radius = srcObj->radius;

    return kOK;
}

GoFx(kStatus) GoCircleFeatureMsg_VRelease(GoCircleFeatureMsg msg)
{
    kObjR(GoCircleFeatureMsg, msg);

    return kObject_VRelease(msg);
}

GoFx(kSize)GoCircleFeatureMsg_VSize(GoPointFeatureMsg msg)
{
    kObjR(GoCircleFeatureMsg, msg);

    return sizeof(GoCircleFeatureMsgClass);
}

GoFx(kStatus) GoCircleFeatureMsg_WriteV27(GoCircleFeatureMsg msg, kSerializer serializer)
{
    kObjR(GoCircleFeatureMsg, msg);

    kCheck(kSerializer_Write16u(serializer, obj->id));
    kCheck(GoFeatureMsg_WritePoint(serializer, &obj->center));
    kCheck(GoFeatureMsg_WritePoint(serializer, &obj->normal));
    kCheck(kSerializer_Write64s(serializer, kMath_Round64s_(obj->radius*1.0E6)));

    return kOK;
}

GoFx(kStatus) GoCircleFeatureMsg_ReadV27(GoCircleFeatureMsg msg, kSerializer serializer, kAlloc alloc)
{
    kObjR(GoCircleFeatureMsg, msg);
    k64s temp;

    kCheck(GoCircleFeatureMsg_VInit(msg, kTypeOf(GoCircleFeatureMsg), alloc));

    kCheck(kSerializer_Read16u(serializer, &obj->id));
    kCheck(GoFeatureMsg_ReadPoint(serializer, &obj->center));
    kCheck(GoFeatureMsg_ReadPoint(serializer, &obj->normal));

    kCheck(kSerializer_Read64s(serializer, &temp));
    obj->radius = ((k64f)temp) / 1.0E6;

    return kOK;
}

GoFx(k16u) GoCircleFeatureMsg_Id(GoCircleFeatureMsg msg)
{
    return xGoCircleFeatureMsg_CastRaw(msg)->id;
}

GoFx(kPoint3d64f) GoCircleFeatureMsg_Position(GoCircleFeatureMsg msg)
{
    return xGoCircleFeatureMsg_CastRaw(msg)->center;
}

GoFx(kPoint3d64f) GoCircleFeatureMsg_Normal(GoCircleFeatureMsg msg)
{
    return xGoCircleFeatureMsg_CastRaw(msg)->normal;
}

GoFx(k64f) GoCircleFeatureMsg_Radius(GoCircleFeatureMsg msg)
{
    return xGoCircleFeatureMsg_CastRaw(msg)->radius;
}

/*
* GoGenericMsg
*/
kBeginFullClassEx(Go, GoGenericMsg)
    kAddVersion(GoGenericMsg, GO_SERIALIZATION_FORMAT_NAME, GO_SERIALIZATION_FORMAT_VERSION, GoSerializerTypeIdStr(GO_COMPACT_MESSAGE_GENERIC), Write, Read)
    kAddVersion(GoGenericMsg, "kdat6", "4.0.0.0", "GoGenericMsg-29", Write, Read)

    kAddVMethod(GoGenericMsg, GoDataMsg, VInit)
    kAddVMethod(GoGenericMsg, kObject, VInitClone)
    kAddVMethod(GoGenericMsg, kObject, VRelease)
    kAddVMethod(GoGenericMsg, kObject, VSize)
kEndFullClassEx()

GoFx(kStatus) xGoGenericMsg_InitStatic()
{
    kStaticObj(GoGenericMsg);
    kAlloc alloc = kAlloc_App();

    kCheck(kLock_Construct(&sobj->generalLock, alloc));

    kCheck(kLock_Construct(&sobj->serializerLock, alloc));
    sobj->serializerBuffer = kNULL;
    sobj->serializer = kNULL;

    return kOK;
}

GoFx(kStatus) xGoGenericMsg_ReleaseStatic()
{
    kStaticObj(GoGenericMsg);

    kCheck(kDestroyRef(&sobj->serializer));
    kCheck(kDestroyRef(&sobj->serializerBuffer));
    kCheck(kDestroyRef(&sobj->serializerLock));
    kCheck(kDestroyRef(&sobj->generalLock));

    return kOK;
}

// Delay construction until after static initialization.
// There's a circular dependency that prevents construction of
// kDat6Serializer during static initialization.
// This requires the lock!
GoFx(kStatus) GoGenericMsg_InitSerializer()
{
    kStaticObj(GoGenericMsg);
    kAlloc alloc = kAlloc_App();
    kMemory memory = kNULL;
    kDat6Serializer serializer = kNULL;

    kTry
    {
        if (sobj->serializer == kNULL)
        {
            kTest(kMemory_Construct(&memory, alloc));
            kTest(kDat6Serializer_Construct(&serializer, memory, alloc));
            kTest(kDat6Serializer_EnableDictionary(serializer, kFALSE));

            sobj->serializerBuffer = memory;
            memory = kNULL;

            sobj->serializer = serializer;
            serializer = kNULL;
        }
    }
    kFinally
    {
        kObject_Destroy(serializer);
        kObject_Destroy(memory);

        kEndFinally();
    }

    return kOK;
}

GoFx(kStatus) GoGenericMsg_ReadObjectPayload(const void* data, kSize length, kObject* object, kAlloc alloc)
{
    kStaticObj(GoGenericMsg);

    // Potential performance bottleneck.
    // If ever an issue, create a pool of readers.
    kCheck(kLock_Enter(sobj->serializerLock));

    kTry
    {
        kTest(GoGenericMsg_InitSerializer());

        kTest(kMemory_Attach(sobj->serializerBuffer, (void*)data, 0, length, length));
        kTest(kSerializer_ReadObject(sobj->serializer, object, alloc));
    }
    kFinally
    {
        kLock_Exit(sobj->serializerLock);

        kEndFinally();
    }

    return kOK;
}

GoFx(kStatus) GoGenericMsg_WriteObjectPayload(kObject object, kArray1* data, kAlloc alloc)
{
    kStaticObj(GoGenericMsg);
    kArray1 newBuffer = kNULL;

    // Potential performance bottleneck.
    // If ever an issue, create a pool of writers.
    kCheck(kLock_Enter(sobj->serializerLock));

    kTry
    {
        kTest(GoGenericMsg_InitSerializer());

        kTest(kMemory_Allocate(sobj->serializerBuffer, 16 * 1024));
        kTest(kSerializer_WriteObject(sobj->serializer, object));
        kTest(kSerializer_Flush(sobj->serializer));

        kTest(kArray1_Construct(&newBuffer, kTypeOf(kByte), (kSize)kMemory_Position(sobj->serializerBuffer), alloc));
        kTest(kMemCopy(kArray1_Data(newBuffer),
            kMemory_At(sobj->serializerBuffer, 0),
            kArray1_Length(newBuffer)));

        *data = newBuffer;
        newBuffer = kNULL;
    }
    kFinally
    {
        kLock_Exit(sobj->serializerLock);
        kObject_Destroy(newBuffer);

        kEndFinally();
    }

    return kOK;
}

GoFx(kStatus) GoGenericMsg_Construct(GoGenericMsg* msg, kAlloc allocator)
{
    kAlloc alloc = kAlloc_Fallback(allocator);
    kStatus status;

    kCheck(kAlloc_GetObject(alloc, kTypeOf(GoGenericMsg), msg));

    if (!kSuccess(status = GoGenericMsg_VInit(*msg, kTypeOf(GoGenericMsg), alloc)))
    {
        kAlloc_FreeRef(alloc, msg);
    }

    return status;
}

GoFx(kStatus) GoGenericMsg_VInit(GoGenericMsg msg, kType type, kAlloc alloc)
{
    kObjR(GoGenericMsg, msg);

    kCheck(GoDataMsg_Init(msg, type, GO_DATA_MESSAGE_TYPE_GENERIC, alloc));
    obj->userType = 0;
    obj->isObject = kFALSE;
    kZero(obj->buffer);
    kZero(obj->objectPayload);

    obj->serializerStatus = kOK;

    return kOK;
}

GoFx(kStatus) GoGenericMsg_VInitClone(GoGenericMsg msg, GoGenericMsg source, kAlloc alloc)
{
    kObjR(GoGenericMsg, msg);
    kObjNR(GoGenericMsg, srcObj, source);

    kCheck(GoGenericMsg_VInit(msg, kObject_Type(source), alloc));

    obj->isObject = srcObj->isObject;
    obj->base.streamStep = GoDataMsg_StreamStep(source);
    obj->base.streamStepId = GoDataMsg_StreamStepId(source);

    kCheck(kDisposeRef(&obj->buffer));
    kCheck(kDisposeRef(&obj->objectPayload));

    kCheck(kObject_Clone(&obj->buffer, srcObj->buffer, alloc));
    kCheck(kObject_Clone(&obj->objectPayload, srcObj->objectPayload, alloc));

    return kOK;
}

GoFx(kStatus) GoGenericMsg_VRelease(GoGenericMsg msg)
{
    kObjR(GoGenericMsg, msg);

    kCheck(kDisposeRef(&obj->buffer));
    kCheck(kDisposeRef(&obj->objectPayload));

    return kObject_VRelease(msg);
}

GoFx(kSize) GoGenericMsg_VSize(GoPointFeatureMsg msg)
{
    kObjR(GoGenericMsg, msg);
    kSize size = sizeof(GoGenericMsgClass);

    if (obj->buffer != kNULL)
    {
        size += kObject_Size(obj->buffer);
    }

    if (obj->objectPayload != kNULL)
    {
        size += kObject_Size(obj->objectPayload);
    }

    return size;
}

GoFx(kStatus) GoGenericMsg_Write(GoGenericMsg msg, kSerializer serializer)
{
    kObjR(GoGenericMsg, msg);
    kArray1 serializedBuffer = kNULL;
    kArray1 writeBuffer = kNULL;

    kTry
    {
        if (obj->isObject && obj->objectPayload != kNULL)
        {
            kTest(GoGenericMsg_WriteObjectPayload(obj->objectPayload, &serializedBuffer, kNULL));
            writeBuffer = serializedBuffer;
        }
        else
        {
            writeBuffer = obj->buffer;
        }

        kTest(kSerializer_BeginWrite(serializer, kTypeOf(k16u), kFALSE));
        kTest(kSerializer_Write32s(serializer, GoDataMsg_StreamStep(msg)));
        kTest(kSerializer_Write32s(serializer, GoDataMsg_StreamStepId(msg)));
        kTest(kSerializer_Write32u(serializer, obj->userType));
        kTest(kSerializer_Write8u(serializer, obj->isObject ? 1 : 0));
        kTest(kSerializer_Write32u(serializer, writeBuffer != kNULL ? (k32u)kArray1_Length(writeBuffer) : 0));
        kTest(kSerializer_EndWrite(serializer));

        if (writeBuffer != kNULL)
        {
            kTest(kSerializer_WriteByteArray(serializer, kArray1_Data(writeBuffer), kArray1_Length(writeBuffer)));
        }
    }
    kFinally
    {
        kObject_Destroy(serializedBuffer);

        kEndFinally();
    }

    return kOK;
}

GoFx(kStatus) GoGenericMsg_Read(GoGenericMsg msg, kSerializer serializer, kAlloc alloc)
{
    kObjR(GoGenericMsg, msg);
    k8u val8u;
    k32u dataLength;

    kTry
    {
        kTest(GoGenericMsg_VInit(msg, kTypeOf(GoGenericMsg), alloc));

        kTest(kSerializer_BeginRead(serializer, kTypeOf(k16u), kFALSE));

        kTest(kSerializer_Read32s(serializer, &obj->base.streamStep));
        kTest(kSerializer_Read32s(serializer, &obj->base.streamStepId));
        kTest(kSerializer_Read32u(serializer, &obj->userType));
        kTest(kSerializer_Read8u(serializer, &val8u));
        obj->isObject = (kBool)val8u;
        kTest(kSerializer_Read32u(serializer, &dataLength));

        kTest(kSerializer_EndRead(serializer));

        kTest(kArray1_Construct(&obj->buffer, kTypeOf(kByte), dataLength, alloc));
        kTest(kSerializer_ReadByteArray(serializer, kArray1_Data(obj->buffer), dataLength));
    }
    kFinally
    {
        kEndFinally();
    }

    return kOK;
}

GoFx(GoDataStep) GoGenericMsg_StreamStep(GoGenericMsg msg)
{
    kObjR(GoGenericMsg, msg);

    return GoDataMsg_StreamStep(msg);
}

GoFx(k32s) GoGenericMsg_StreamStepId(GoGenericMsg msg)
{
    kObjR(GoGenericMsg, msg);

    return GoDataMsg_StreamStepId(msg);
}

GoFx(k32u) GoGenericMsg_UserType(GoGenericMsg msg)
{
    kObjR(GoGenericMsg, msg);

    return obj->userType;
}

GoFx(kBool) GoGenericMsg_IsObject(GoGenericMsg msg)
{
    kObjR(GoGenericMsg, msg);

    return obj->isObject;
}

GoFx(kSize) GoGenericMsg_BufferSize(GoGenericMsg msg)
{
    kObjR(GoGenericMsg, msg);

    return obj->buffer != kNULL ? kArray1_Length(obj->buffer) : 0;
}

GoFx(const void*) GoGenericMsg_BufferData(GoGenericMsg msg)
{
    kObjR(GoGenericMsg, msg);

    return obj->buffer != kNULL ? kArray1_Data(obj->buffer) : kNULL;
}

GoFx(kObject) GoGenericMsg_Object(GoGenericMsg msg)
{
    kStaticObj(GoGenericMsg);
    kObjR(GoGenericMsg, msg);

    // Deserialize the object on demand only when necessary.
    if (kSuccess(kLock_Enter(sobj->generalLock)))
    {
        if (kIsNull(obj->objectPayload) &&
            obj->isObject &&
            obj->buffer != kNULL &&
            kArray1_Length(obj->buffer) > 0)
        {
            obj->serializerStatus = GoGenericMsg_ReadObjectPayload(
                kArray1_Data(obj->buffer),
                kArray1_Count(obj->buffer),
                &obj->objectPayload,
                kObject_Alloc(msg));
        }

        kLock_Exit(sobj->generalLock);
    }

    return obj->objectPayload;
}

GoFx(kStatus) GoGenericMsg_SerializerStatus(GoGenericMsg msg)
{
    kObjR(GoGenericMsg, msg);

    return obj->serializerStatus;
}

//Deprecated:
/*
* GoProfileMsg
*/

GoFx(kStatus) GoProfileMsg_Construct(GoProfileMsg* msg, kAlloc allocator)
{
    return GoProfilePointCloudMsg_Construct(msg, allocator);
}

GoFx(kStatus) GoProfileMsg_VInit(GoProfileMsg msg, kType type, kAlloc alloc)
{
    return GoProfilePointCloudMsg_VInit(msg, type, alloc);
}

GoFx(kStatus) GoProfileMsg_VInitClone(GoProfileMsg msg, GoProfileMsg source, kAlloc alloc)
{
    return GoProfilePointCloudMsg_VInitClone(msg, source, alloc);
}

GoFx(kStatus) GoProfileMsg_Allocate(GoProfileMsg msg, kSize count, kSize width)
{
    return GoProfilePointCloudMsg_Allocate(msg, count, width);
}

GoFx(kSize) GoProfileMsg_VSize(GoProfileMsg msg)
{
    return GoProfilePointCloudMsg_VSize(msg);
}

GoFx(kStatus) GoProfileMsg_VRelease(GoProfileMsg msg)
{
    return GoProfilePointCloudMsg_VRelease(msg);
}

GoFx(kStatus) GoProfileMsg_WriteV5(GoProfileMsg msg, kSerializer serializer)
{
    return GoProfilePointCloudMsg_WriteV5(msg, serializer);
}

GoFx(kStatus) GoProfileMsg_ReadV5(GoProfileMsg msg, kSerializer serializer, kAlloc alloc)
{
    return GoProfilePointCloudMsg_ReadV5(msg, serializer, alloc);
}

GoFx(GoDataSource) GoProfileMsg_Source(GoProfileMsg msg)
{
    return GoProfilePointCloudMsg_Source(msg);
}

GoFx(kSize) GoProfileMsg_Count(GoProfileMsg msg)
{
    return GoProfilePointCloudMsg_Count(msg);
}

GoFx(kSize) GoProfileMsg_Width(GoProfileMsg msg)
{
    return GoProfilePointCloudMsg_Width(msg);
}

GoFx(k32u) GoProfileMsg_XResolution(GoProfileMsg msg)
{
    return GoProfilePointCloudMsg_XResolution(msg);
}

GoFx(k32u) GoProfileMsg_ZResolution(GoProfileMsg msg)
{
    return GoProfilePointCloudMsg_ZResolution(msg);
}

GoFx(k32s) GoProfileMsg_XOffset(GoProfileMsg msg)
{
    return GoProfilePointCloudMsg_XOffset(msg);
}

GoFx(k32s) GoProfileMsg_ZOffset(GoProfileMsg msg)
{
    return GoProfilePointCloudMsg_ZOffset(msg);
}

GoFx(kPoint16s*) GoProfileMsg_At(GoProfileMsg msg, kSize index)
{
    return GoProfilePointCloudMsg_At(msg, index);
}

GoFx(k32u) GoProfileMsg_Exposure(GoProfileMsg msg)
{
    return GoProfilePointCloudMsg_Exposure(msg);
}

GoFx(k8u) GoProfileMsg_CameraIndex(GoProfileMsg msg)
{
    return GoProfilePointCloudMsg_CameraIndex(msg);
}

/*
* GoResampledProfileMsg
*/

GoFx(kStatus) GoResampledProfileMsg_Construct(GoResampledProfileMsg* msg, kAlloc allocator)
{
    return GoUniformProfileMsg_Construct(msg, allocator);
}

GoFx(kStatus) GoResampledProfileMsg_VInit(GoResampledProfileMsg msg, kType type, kAlloc alloc)
{
    return GoUniformProfileMsg_VInit(msg, type, alloc);
}

GoFx(kStatus) GoResampledProfileMsg_VInitClone(GoResampledProfileMsg msg, GoResampledProfileMsg source, kAlloc alloc)
{
    return GoUniformProfileMsg_VInitClone(msg, source, alloc);
}

GoFx(kStatus) GoResampledProfileMsg_Allocate(GoResampledProfileMsg msg, kSize count, kSize width)
{
    return GoUniformProfileMsg_Allocate(msg, count, width);
}

GoFx(kSize) GoResampledProfileMsg_VSize(GoResampledProfileMsg msg)
{
    return GoUniformProfileMsg_VSize(msg);
}

GoFx(kStatus) GoResampledProfileMsg_VRelease(GoResampledProfileMsg msg)
{
    return GoUniformProfileMsg_VRelease(msg);
}

GoFx(kStatus) GoResampledProfileMsg_WriteV6(GoResampledProfileMsg msg, kSerializer serializer)
{
    return GoUniformProfileMsg_WriteV6(msg, serializer);
}

GoFx(kStatus) GoResampledProfileMsg_ReadV6(GoResampledProfileMsg msg, kSerializer serializer, kAlloc alloc)
{
    return GoUniformProfileMsg_ReadV6(msg, serializer, alloc);
}

GoFx(kStatus) GoResampledProfileMsg_ReadV6AttrProtoVer1(GoResampledProfileMsg msg, kSerializer serializer, k32u* countPtr, k32u* widthPtr)
{
    return GoUniformProfileMsg_ReadV6AttrProtoVer1(msg, serializer, countPtr, widthPtr);
}

GoFx(kStatus) GoResampledProfileMsg_ReadV6AttrProtoVer2(GoResampledProfileMsg msg, kSerializer serializer)
{
    return GoUniformProfileMsg_ReadV6AttrProtoVer2(msg, serializer);
}

GoFx(GoDataSource) GoResampledProfileMsg_Source(GoResampledProfileMsg msg)
{
    return GoUniformProfileMsg_Source(msg);
}

GoFx(kSize) GoResampledProfileMsg_Count(GoResampledProfileMsg msg)
{
    return GoUniformProfileMsg_Count(msg);
}

GoFx(kSize) GoResampledProfileMsg_Width(GoResampledProfileMsg msg)
{
    return GoUniformProfileMsg_Width(msg);
}

GoFx(k32u) GoResampledProfileMsg_XResolution(GoResampledProfileMsg msg)
{
    return GoUniformProfileMsg_XResolution(msg);
}

GoFx(k32u) GoResampledProfileMsg_ZResolution(GoResampledProfileMsg msg)
{
    return GoUniformProfileMsg_ZResolution(msg);
}

GoFx(k32s) GoResampledProfileMsg_XOffset(GoResampledProfileMsg msg)
{
    return GoUniformProfileMsg_XOffset(msg);
}

GoFx(k32s) GoResampledProfileMsg_ZOffset(GoResampledProfileMsg msg)
{
    return GoUniformProfileMsg_ZOffset(msg);
}

GoFx(k16s*) GoResampledProfileMsg_At(GoResampledProfileMsg msg, kSize index)
{
    return GoUniformProfileMsg_At(msg, index);
}

GoFx(k32u) GoResampledProfileMsg_Exposure(GoResampledProfileMsg msg)
{
    return GoUniformProfileMsg_Exposure(msg);
}

GoFx(GoDataStep) GoResampledProfileMsg_StreamStep(GoResampledProfileMsg msg)
{
    return GoUniformProfileMsg_StreamStep(msg);
}

GoFx(k32s) GoResampledProfileMsg_StreamStepId(GoResampledProfileMsg msg)
{
    return GoUniformProfileMsg_StreamStepId(msg);
}

/*
* GoSurfaceMsg
*/

GoFx(kStatus) GoSurfaceMsg_Construct(GoSurfaceMsg* msg, kAlloc allocator)
{
    return GoUniformSurfaceMsg_Construct(msg, allocator);
}

GoFx(kStatus) GoSurfaceMsg_VInit(GoSurfaceMsg msg, kType type, kAlloc alloc)
{
    return GoUniformSurfaceMsg_VInit(msg, type, alloc);
}

GoFx(kStatus) GoSurfaceMsg_VInitClone(GoSurfaceMsg msg, GoSurfaceMsg source, kAlloc alloc)
{
    return GoUniformSurfaceMsg_VInitClone(msg, source, alloc);
}

GoFx(kStatus) GoSurfaceMsg_Allocate(GoSurfaceMsg msg, kSize length, kSize width)
{
    return GoUniformSurfaceMsg_Allocate(msg, length, width);
}

GoFx(kSize) GoSurfaceMsg_VSize(GoSurfaceMsg msg)
{
    return GoUniformSurfaceMsg_VSize(msg);
}

GoFx(kStatus) GoSurfaceMsg_VRelease(GoSurfaceMsg msg)
{
    return GoUniformSurfaceMsg_VRelease(msg);
}

GoFx(kStatus) GoSurfaceMsg_WriteV8(GoSurfaceMsg msg, kSerializer serializer)
{
    return GoUniformSurfaceMsg_WriteV8(msg, serializer);
}

GoFx(kStatus) GoSurfaceMsg_ReadV8(GoSurfaceMsg msg, kSerializer serializer, kAlloc alloc)
{
    return GoUniformSurfaceMsg_ReadV8(msg, serializer, alloc);
}

GoFx(kStatus) GoSurfaceMsg_ReadV8AttrProtoVer1(GoSurfaceMsg msg, kSerializer serializer, k32u* lenPtr, k32u* widthPtr)
{
    return GoUniformSurfaceMsg_ReadV8AttrProtoVer1(msg, serializer, lenPtr, widthPtr);
}

GoFx(kStatus) GoSurfaceMsg_ReadV8AttrProtoVer2(GoSurfaceMsg msg, kSerializer serializer)
{
    return GoUniformSurfaceMsg_ReadV8AttrProtoVer2(msg, serializer);
}

GoFx(GoDataSource) GoSurfaceMsg_Source(GoSurfaceMsg msg)
{
    return GoUniformSurfaceMsg_Source(msg);
}

GoFx(kSize) GoSurfaceMsg_Length(GoSurfaceMsg msg)
{
    return GoUniformSurfaceMsg_Length(msg);
}

GoFx(kSize) GoSurfaceMsg_Width(GoSurfaceMsg msg)
{
    return GoUniformSurfaceMsg_Width(msg);
}

GoFx(k32u) GoSurfaceMsg_XResolution(GoSurfaceMsg msg)
{
    return GoUniformSurfaceMsg_XResolution(msg);
}

GoFx(k32u) GoSurfaceMsg_YResolution(GoSurfaceMsg msg)
{
    return GoUniformSurfaceMsg_YResolution(msg);
}

GoFx(k32u) GoSurfaceMsg_ZResolution(GoSurfaceMsg msg)
{
    return GoUniformSurfaceMsg_ZResolution(msg);
}

GoFx(k32s) GoSurfaceMsg_XOffset(GoSurfaceMsg msg)
{
    return GoUniformSurfaceMsg_XOffset(msg);
}

GoFx(k32s) GoSurfaceMsg_YOffset(GoSurfaceMsg msg)
{
    return GoUniformSurfaceMsg_YOffset(msg);
}

GoFx(k32s) GoSurfaceMsg_ZOffset(GoSurfaceMsg msg)
{
    return GoUniformSurfaceMsg_ZOffset(msg);
}

GoFx(k16s*) GoSurfaceMsg_RowAt(GoSurfaceMsg msg, kSize index)
{
    return GoUniformSurfaceMsg_RowAt(msg, index);
}

GoFx(k32u) GoSurfaceMsg_Exposure(GoSurfaceMsg msg)
{
    return GoUniformSurfaceMsg_Exposure(msg);
}

GoFx(GoDataStep) GoSurfaceMsg_StreamStep(GoSurfaceMsg msg)
{
    return GoUniformSurfaceMsg_StreamStep(msg);
}

GoFx(k32s) GoSurfaceMsg_StreamStepId(GoSurfaceMsg msg)
{
    return GoUniformSurfaceMsg_StreamStepId(msg);
}
