/** 
 * @file    kApiVersion.h
 * @brief   Provides Zen version macros. 
 *
 * @internal
 * Copyright (C) 2008-2019 by LMI Technologies Inc.
 * Licensed under the MIT License.
 * Redistributed files must retain the above copyright notice.
 */
#ifndef K_API_VERSION_H
#define K_API_VERSION_H

#include <kApi/kApiDef.h>

#define kAPI_VERSION_MAJOR 7
#define kAPI_VERSION_MINOR 5
#define kAPI_VERSION_RELEASE 0
#define kAPI_VERSION_BUILD 54

#define kAPI_VERSION            kVersion_Stringify_(kAPI_VERSION_MAJOR, kAPI_VERSION_MINOR, kAPI_VERSION_RELEASE, kAPI_VERSION_BUILD)
#define kAPI_VERSION_NUMBER     kVersion_Create(kAPI_VERSION_MAJOR, kAPI_VERSION_MINOR, kAPI_VERSION_RELEASE, kAPI_VERSION_BUILD)

#endif
