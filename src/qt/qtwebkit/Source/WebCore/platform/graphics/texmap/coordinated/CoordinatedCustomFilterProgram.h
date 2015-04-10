/*
 * Copyright (C) 2012 Company 100, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef CoordinatedCustomFilterProgram_h
#define CoordinatedCustomFilterProgram_h

#if USE(COORDINATED_GRAPHICS) && ENABLE(CSS_SHADERS)
#include "CustomFilterConstants.h"
#include "CustomFilterProgram.h"

namespace WebCore {

class CoordinatedCustomFilterProgram : public CustomFilterProgram {
public:
    static PassRefPtr<CoordinatedCustomFilterProgram> create(String vertexShaderString, String m_fragmentShaderString, CustomFilterProgramType programType, CustomFilterProgramMixSettings mixSettings, CustomFilterMeshType meshType)
    {
        return adoptRef(new CoordinatedCustomFilterProgram(vertexShaderString, m_fragmentShaderString, programType, mixSettings, meshType));
    }

    virtual bool isLoaded() const OVERRIDE { return true; }

protected:
    virtual String vertexShaderString() const OVERRIDE { return m_vertexShaderString; }
    virtual String fragmentShaderString() const OVERRIDE { return m_fragmentShaderString; }

    virtual void willHaveClients() OVERRIDE { notifyClients(); }
    virtual void didRemoveLastClient() OVERRIDE { }

private:
    CoordinatedCustomFilterProgram(String vertexShaderString, String fragmentShaderString, CustomFilterProgramType programType, CustomFilterProgramMixSettings mixSettings, CustomFilterMeshType meshType)
        : CustomFilterProgram(programType, mixSettings, meshType)
        , m_vertexShaderString(vertexShaderString)
        , m_fragmentShaderString(fragmentShaderString)
    {
    }

    String m_vertexShaderString;
    String m_fragmentShaderString;
};

} // namespace WebCore

#endif // USE(COORDINATED_GRAPHICS) && ENABLE(CSS_SHADERS)

#endif // CoordinatedCustomFilterProgram_h
