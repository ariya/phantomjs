/*
 * Copyright (C) 2011 Adobe Systems Incorporated. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER “AS IS” AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(CSS_SHADERS)

#include "CachedShader.h"
#include "ResourceBuffer.h"
#include "TextResourceDecoder.h"
#include <wtf/text/StringBuilder.h>

namespace WebCore {

CachedShader::CachedShader(const ResourceRequest& resourceRequest)
    : CachedResource(resourceRequest, ShaderResource)
    , m_decoder(TextResourceDecoder::create("application/shader"))
{
}

CachedShader::~CachedShader()
{
}

const String& CachedShader::shaderString()
{
    if (m_shaderString.isNull() && m_data) {
        StringBuilder builder;
        builder.append(m_decoder->decode(m_data->data(), m_data->size()));
        builder.append(m_decoder->flush());
        m_shaderString = builder.toString();
    }

    return m_shaderString;
}

void CachedShader::finishLoading(ResourceBuffer* data)
{
    m_data = data;
    CachedResource::finishLoading(0);
}

} // namespace WebCore

#endif // ENABLE(CSS_SHADERS)
