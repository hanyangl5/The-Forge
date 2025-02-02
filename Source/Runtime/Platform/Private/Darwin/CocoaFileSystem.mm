/*
 * Copyright (c) 2017-2024 The Forge Interactive Inc.
 *
 * This file is part of The-Forge
 * (see https://github.com/ConfettiFX/The-Forge).
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <Core/IConfig.h>

#import <Foundation/Foundation.h>

#include <Core/IFileSystem.h>
#include <Core/ILog.h>
#include <Platform/IOperatingSystem.h>

#include <Core/IMemory.h>

static bool        gInitialized = false;
static const char* gResourceMounts[RM_COUNT];
const char*        getResourceMount(ResourceMount mount) { return gResourceMounts[mount]; }

static char gSaveUrl[FS_MAX_PATH] = {};
#ifndef TARGET_IOS
static char gApplicationPath[FS_MAX_PATH] = {};
#else
static char gDebugUrl[FS_MAX_PATH] = {};
#endif

extern "C"
{
    void fsGetParentPath(const char* path, char* output);
}

bool initFileSystem(FileSystemInitDesc* pDesc)
{
    if (gInitialized)
    {
        LOGF(LogLevel::eWARNING, "FileSystem already initialized.");
        return true;
    }
    ASSERT(pDesc);
    pSystemFileIO->GetResourceMount = getResourceMount;

    for (uint32_t i = 0; i < RM_COUNT; ++i)
        gResourceMounts[i] = "";

    NSFileManager* fileManager = [NSFileManager defaultManager];
    // Get application directory
    gResourceMounts[RM_CONTENT] = [[[[NSBundle mainBundle] resourceURL] relativePath] UTF8String];

    if (!pDesc->pResourceMounts[RM_CONTENT])
    {
        [fileManager changeCurrentDirectoryPath:[[NSBundle mainBundle] bundlePath]];
    }

    // Get save directory
    NSError* error = nil;
    NSURL*   pSaveUrl = [fileManager URLForDirectory:NSDocumentDirectory
                                          inDomain:NSUserDomainMask
                                 appropriateForURL:nil
                                            create:true
                                             error:&error];

    if (!error)
    {
        const char* pSaveUrlStr = [[pSaveUrl path] UTF8String];
        strcpy(gSaveUrl, pSaveUrlStr);
        gResourceMounts[RM_DOCUMENTS] = gSaveUrl;
    }
    else
    {
        LOGF(LogLevel::eERROR, "Error retrieving user documents directory: %s", [[error description] UTF8String]);
        return false;
    }

    // Get debug directory
#ifdef TARGET_IOS
    // Place log files in the application support directory on iOS.
    NSURL* pDebugUrl = [fileManager URLForDirectory:NSApplicationSupportDirectory
                                           inDomain:NSUserDomainMask
                                  appropriateForURL:nil
                                             create:true
                                              error:&error];
    if (!error)
    {
        const char* pDebugUrlStr = [[pDebugUrl path] UTF8String];
        strcpy(gDebugUrl, pDebugUrlStr);
        gResourceMounts[RM_DEBUG] = gDebugUrl;
        gResourceMounts[RM_SAVE_0] = gDebugUrl;
    }
    else
    {
        LOGF(LogLevel::eERROR, "Error retrieving application support directory: %s", [[error description] UTF8String]);
    }
#else
    const char* path = [[[NSBundle mainBundle] bundlePath] UTF8String];
    fsGetParentPath(path, gApplicationPath);
    gResourceMounts[RM_DEBUG] = gApplicationPath;
    gResourceMounts[RM_SAVE_0] = gApplicationPath;
#endif

    // Override Resource mounts
    for (uint32_t i = 0; i < RM_COUNT; ++i)
    {
        if (pDesc->pResourceMounts[i])
            gResourceMounts[i] = pDesc->pResourceMounts[i];
    }

    gInitialized = true;
    return true;
}

void exitFileSystem() { gInitialized = false; }
