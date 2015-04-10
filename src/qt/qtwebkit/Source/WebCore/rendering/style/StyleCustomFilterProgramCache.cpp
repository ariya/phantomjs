/*
 * Copyright (C) 2013 Adobe Systems Incorporated. All rights reserved.
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

#include "StyleCustomFilterProgramCache.h"

#include "CustomFilterProgramInfo.h"
#include "StyleCustomFilterProgram.h"

namespace WebCore {

static CustomFilterProgramInfo programCacheKey(StyleCustomFilterProgram* program) 
{
    ASSERT(program->vertexShaderURL().isValid() || program->fragmentShaderURL().isValid());
    return CustomFilterProgramInfo(program->vertexShaderURL(), program->fragmentShaderURL(), 
        program->programType(), program->mixSettings(), program->meshType());
}

StyleCustomFilterProgramCache::StyleCustomFilterProgramCache()
{
}

StyleCustomFilterProgramCache::~StyleCustomFilterProgramCache()
{
    // Make sure the programs are not calling back into this object.
    for (CacheMap::iterator iter = m_cache.begin(), end = m_cache.end(); iter != end; ++iter)
        iter->value->setCache(0);
}

StyleCustomFilterProgram* StyleCustomFilterProgramCache::lookup(const CustomFilterProgramInfo& programInfo) const
{
    CacheMap::const_iterator iter = m_cache.find(programInfo);
    return iter != m_cache.end() ? iter->value : 0;
}

StyleCustomFilterProgram* StyleCustomFilterProgramCache::lookup(StyleCustomFilterProgram* program) const
{
    return lookup(programCacheKey(program));
}

void StyleCustomFilterProgramCache::add(StyleCustomFilterProgram* program)
{
    CustomFilterProgramInfo key = programCacheKey(program);
    ASSERT(m_cache.find(key) == m_cache.end());
    m_cache.set(key, program);
    program->setCache(this);
}

void StyleCustomFilterProgramCache::remove(StyleCustomFilterProgram* program)
{
    CacheMap::iterator iter = m_cache.find(programCacheKey(program));
    ASSERT(iter != m_cache.end());
    m_cache.remove(iter);
}


} // namespace WebCore

#endif // ENABLE(CSS_SHADERS)

