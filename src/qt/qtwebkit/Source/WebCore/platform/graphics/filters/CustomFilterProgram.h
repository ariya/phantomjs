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

#ifndef CustomFilterProgram_h
#define CustomFilterProgram_h

#if ENABLE(CSS_SHADERS)

#include "CustomFilterProgramInfo.h"

#include <wtf/HashCountedSet.h>
#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class GraphicsContext3D;
class CustomFilterProgramClient;
class CustomFilterValidatedProgram;

// This is the base class for the StyleCustomFilterProgram class which knows how to keep
// references to the cached shaders.
class CustomFilterProgram: public RefCounted<CustomFilterProgram> {
public:
    virtual ~CustomFilterProgram();

    virtual bool isLoaded() const = 0;

    void addClient(CustomFilterProgramClient*);
    void removeClient(CustomFilterProgramClient*);
    
    CustomFilterProgramInfo programInfo() const;

    virtual String vertexShaderString() const = 0;
    virtual String fragmentShaderString() const = 0;
    CustomFilterProgramType programType() const { return m_programType; }
    CustomFilterProgramMixSettings mixSettings() const { return m_mixSettings; }
    CustomFilterMeshType meshType() const { return m_meshType; }

    PassRefPtr<CustomFilterValidatedProgram> validatedProgram();
    void setValidatedProgram(PassRefPtr<CustomFilterValidatedProgram>);

protected:
    // StyleCustomFilterProgram can notify the clients that the cached resources are
    // loaded and it is ready to create CustomFilterCompiledProgram objects.
    void notifyClients();

    virtual void willHaveClients() = 0;
    virtual void didRemoveLastClient() = 0;

    // Keep the constructor protected to prevent creating this object directly.
    CustomFilterProgram(CustomFilterProgramType, const CustomFilterProgramMixSettings&, CustomFilterMeshType);

private:
    // CustomFilterPrograms are unique combinations of shaders and can be 
    // compared using just the pointer value instead.
    // These will catch anyone doing a value equal comparison.
    bool operator==(const CustomFilterProgram&) const;
    bool operator!=(const CustomFilterProgram&) const;

    typedef HashCountedSet<CustomFilterProgramClient*> CustomFilterProgramClientList;
    CustomFilterProgramClientList m_clients;
    CustomFilterProgramType m_programType;
    CustomFilterProgramMixSettings m_mixSettings;
    CustomFilterMeshType m_meshType;
    RefPtr<CustomFilterValidatedProgram> m_validatedProgram;
};

}

#endif // ENABLE(CSS_SHADERS)

#endif
