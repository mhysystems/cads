/** 
 * @file    kCipher.cpp
 *
 * @internal
 * Copyright (C) 2016-2019 by LMI Technologies Inc.
 * Licensed under the MIT License.
 * Redistributed files must retain the above copyright notice.
 */

#include <kApi/Crypto/kCipher.h>
#include <kApi/Data/kArray1.h>

kBeginVirtualClassEx(k, kCipher)
    kAddFlags(kCipher, kTYPE_FLAGS_ABSTRACT)

    kAddPrivateVMethod(kCipher, kObject, VRelease)
    kAddPrivateVMethod(kCipher, kCipher, VEncrypt)
    kAddPrivateVMethod(kCipher, kCipher, VDecrypt)
    kAddPrivateVMethod(kCipher, kCipher, VBlocksize)
kEndVirtualClassEx()

kBeginEnumEx(k, kCipherPadding)
    kAddEnumerator(kCipherPadding, kCIPHER_PADDING_NULL)
    kAddEnumerator(kCipherPadding, kCIPHER_PADDING_ANSIX923)
    kAddEnumerator(kCipherPadding, kCIPHER_PADDING_ISO10126)
    kAddEnumerator(kCipherPadding, kCIPHER_PADDING_NONE)
    kAddEnumerator(kCipherPadding, kCIPHER_PADDING_PKCS7)
    kAddEnumerator(kCipherPadding, kCIPHER_PADDING_ZERO)
kEndEnumEx()

kBeginEnumEx(k, kCipherMode)
    kAddEnumerator(kCipherMode, kCIPHER_CIPHER_NULL)
    kAddEnumerator(kCipherMode, kCIPHER_CIPHER_ECB)
kEndEnumEx()

kFx(kStatus) xkCipher_Init(kCipher cipher, kType type, kCipherPadding padding, kCipherMode mode, kAlloc allocator)
{
    kObjR(kCipher, cipher);

    kCheck(kObject_Init(cipher, type, allocator)); 

    obj->padding = padding;
    obj->mode = mode;

    return kOK; 
}

kFx(kStatus) xkCipher_AddPadding(kCipher cipher, kArray1 encryptArray, kSize paddingByteCount)
{
    kObj(kCipher, cipher);
    kSize encryptBufferLength = kArray1_Length(encryptArray);
    kSize paddingStartPos = encryptBufferLength - paddingByteCount;
    kByte padVal;
    kSize i, j;

    if (paddingByteCount == 0)
    {
        return kOK;
    }
    else if ((obj->padding == kCIPHER_PADDING_NONE))
    {
        return kERROR_PARAMETER;
    }

    if (obj->padding == kCIPHER_PADDING_NULL)
    {
        return kERROR_PARAMETER;
    }

    if (obj->padding == kCIPHER_PADDING_ANSIX923)
    {
        padVal = 0;

        for (i = paddingStartPos; i < encryptBufferLength - 1; i++)
        {
            kCheck(kArray1_SetItem(encryptArray, i, &padVal));
        }

        padVal = (kByte)paddingByteCount;

        kCheck(kArray1_SetItem(encryptArray, encryptBufferLength - 1, &padVal));
    }
    else if (obj->padding == kCIPHER_PADDING_ISO10126)
    {
        for (i = paddingStartPos, j = 0; i < encryptBufferLength; i++, j++)
        {
            kByte r8 = (kByte)(kRandom32u() << (kRandom32u() % 4));
            kCheck(kArray1_SetItem(encryptArray, i, &r8));
        }
    }
    else if (obj->padding == kCIPHER_PADDING_PKCS7)
    {
        padVal = (kByte)paddingByteCount;

        for (i = paddingStartPos; i < encryptBufferLength; i++)
        {
            kCheck(kArray1_SetItem(encryptArray, i, &padVal));
        }
    }
    else if (obj->padding == kCIPHER_PADDING_ZERO)
    {
        padVal = 0;

        for (i = paddingStartPos; i < encryptBufferLength; i++)
        {
            kCheck(kArray1_SetItem(encryptArray, i, &padVal));
        }
    }

    return kOK;
}
