/*
 * Copyright (C) 2012 Adobe Systems Incorporated. All rights reserved.
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
#include "CustomFilterProgramInfo.h"

#include <wtf/HashFunctions.h>
#include <wtf/text/StringHash.h>

namespace WebCore {

static unsigned hashPossiblyNullString(const String& string)
{
    return string.isNull() ? 0 : DefaultHash<String>::Hash::hash(string);
}

CustomFilterProgramInfo::CustomFilterProgramInfo()
{
}

bool CustomFilterProgramInfo::isEmptyValue() const 
{ 
    return m_vertexShaderString.isNull() 
        && m_fragmentShaderString.isNull();
}

CustomFilterProgramInfo::CustomFilterProgramInfo(WTF::HashTableDeletedValueType)
    : m_vertexShaderString(WTF::HashTableDeletedValue)
    , m_fragmentShaderString(WTF::HashTableDeletedValue)
{
}

bool CustomFilterProgramInfo::isHashTableDeletedValue() const
{
    return m_vertexShaderString.isHashTableDeletedValue() 
        && m_fragmentShaderString.isHashTableDeletedValue();
}

CustomFilterProgramInfo::CustomFilterProgramInfo(const String& vertexShader, const String& fragmentShader, CustomFilterProgramType programType, const CustomFilterProgramMixSettings& mixSettings, CustomFilterMeshType meshType)
    : m_vertexShaderString(vertexShader)
    , m_fragmentShaderString(fragmentShader)
    , m_programType(programType)
    , m_mixSettings(mixSettings)
    , m_meshType(meshType)
{
    // At least one of the shaders needs to be non-null.
    ASSERT(!m_vertexShaderString.isNull() || !m_fragmentShaderString.isNull());
}

unsigned CustomFilterProgramInfo::hash() const
{
    // At least one of the shaders needs to be non-null.
    ASSERT(!m_vertexShaderString.isNull() || !m_fragmentShaderString.isNull());

    bool blendsElementTexture = (m_programType == PROGRAM_TYPE_BLENDS_ELEMENT_TEXTURE);
    uintptr_t hashCodes[6] = {
        hashPossiblyNullString(m_vertexShaderString),
        hashPossiblyNullString(m_fragmentShaderString),
        blendsElementTexture,
        static_cast<uintptr_t>(blendsElementTexture ? m_mixSettings.blendMode : 0),
        static_cast<uintptr_t>(blendsElementTexture ? m_mixSettings.compositeOperator : 0),
        m_meshType
    };
    return StringHasher::hashMemory<sizeof(hashCodes)>(&hashCodes);
}

bool CustomFilterProgramInfo::operator==(const CustomFilterProgramInfo& o) const 
{
    ASSERT(!isHashTableDeletedValue());
    ASSERT(!o.isHashTableDeletedValue());

    return m_programType == o.m_programType
        && (m_programType != PROGRAM_TYPE_BLENDS_ELEMENT_TEXTURE || m_mixSettings == o.m_mixSettings)
        && m_meshType == o.m_meshType
        && m_vertexShaderString == o.m_vertexShaderString
        && m_fragmentShaderString == o.m_fragmentShaderString;
}

} // namespace WebCore

#endif // ENABLE(CSS_SHADERS)
