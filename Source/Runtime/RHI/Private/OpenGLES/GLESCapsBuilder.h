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

#pragma once

#include <ThirdParty/tinyimageformat/tinyimageformat_apis.h>
#include <ThirdParty/tinyimageformat/tinyimageformat_base.h>
#include "../../Resources/ResourceLoader/ThirdParty/OpenSource/tinyimageformat/tinyimageformat_query.h"

inline void glCapsBuilder(GpuInfo* pGpu, const char* availableExtensions)
{
    const bool supportFloatTexture = strstr(availableExtensions, "GL_OES_texture_float") != nullptr;
    const bool supportHalfFloatTexture = strstr(availableExtensions, "GL_OES_texture_half_float") != nullptr;
    const bool supportFloatColorBuffer = strstr(availableExtensions, "GL_EXT_color_buffer_float") != nullptr;
    const bool supportHalfFloatColorBuffer = strstr(availableExtensions, "GL_EXT_color_buffer_half_float") != nullptr;
    const bool supportPackedDepthStencil = strstr(availableExtensions, "GL_OES_packed_depth_stencil") != nullptr;
    const bool supportDepth32 = strstr(availableExtensions, "GL_OES_depth32") != nullptr;
    const bool supportSampleFloatLinear = strstr(availableExtensions, "GL_OES_texture_float_linear") != nullptr;

    GLint nCompressedTextureFormats = 0;
    glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &nCompressedTextureFormats);
    GLint* pCompressedTextureSupport = (GLint*)tf_calloc(1, nCompressedTextureFormats * sizeof(GLint));
    glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, pCompressedTextureSupport);

    GLuint format, typeSize, internalFormat, type;
    for (uint32_t i = 0; i < TinyImageFormat_Count; ++i)
    {
        TinyImageFormat imgFormat = (TinyImageFormat)i;
        if (TinyImageFormat_IsCompressed(imgFormat))
        {
            TinyImageFormat_ToGL_FORMAT(imgFormat, &format, &type, &internalFormat, &typeSize);
            uint32_t j = 0;
            for (; j < nCompressedTextureFormats; ++j)
            {
                if (pCompressedTextureSupport[j] == internalFormat)
                    break;
            }

            if (j == nCompressedTextureFormats)
                continue;
        }

        bool shaderResult = 1;
        bool renderTargetResult = 1;

        if (TinyImageFormat_IsDepthAndStencil(imgFormat) && !supportPackedDepthStencil)
        {
            shaderResult = 0;
            renderTargetResult = 0;
        }

        if (TinyImageFormat_IsFloat(imgFormat))
        {
            if (TinyImageFormat_MaxAtPhysical(imgFormat, 0) == 65504.000000)
            {
                shaderResult = supportHalfFloatTexture;
                renderTargetResult = supportHalfFloatColorBuffer;
            }
            else
            {
                shaderResult = supportFloatTexture;
                renderTargetResult = supportFloatColorBuffer;
            }

            if (supportSampleFloatLinear)
            {
                pGpu->mCapBits.mFormatCaps[i] |= FORMAT_CAP_LINEAR_FILTER;
            }
        }
        else
        {
            pGpu->mCapBits.mFormatCaps[i] |= FORMAT_CAP_LINEAR_FILTER;
        }

        if (imgFormat == TinyImageFormat_D32_SFLOAT && !supportDepth32)
        {
            shaderResult = 0;
            renderTargetResult = 0;
        }

        if (shaderResult)
        {
            pGpu->mCapBits.mFormatCaps[i] |= FORMAT_CAP_READ | FORMAT_CAP_WRITE | FORMAT_CAP_READ_WRITE;
        }

        if (renderTargetResult)
        {
            pGpu->mCapBits.mFormatCaps[i] |= FORMAT_CAP_RENDER_TARGET;
        }
    }

    tf_free(pCompressedTextureSupport);
}
