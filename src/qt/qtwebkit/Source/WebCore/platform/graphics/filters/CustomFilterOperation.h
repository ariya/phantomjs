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

#ifndef CustomFilterOperation_h
#define CustomFilterOperation_h

#if ENABLE(CSS_SHADERS)
#include "CustomFilterConstants.h"
#include "CustomFilterParameterList.h"
#include "CustomFilterProgram.h"
#include "FilterOperation.h"
#include "LayoutSize.h"

namespace WebCore {

// CSS Shaders

class CustomFilterOperation : public FilterOperation {
public:
    static PassRefPtr<CustomFilterOperation> create(PassRefPtr<CustomFilterProgram> program, const CustomFilterParameterList& sortedParameters, unsigned meshRows, unsigned meshColumns)
    {
        return adoptRef(new CustomFilterOperation(program, sortedParameters, meshRows, meshColumns));
    }
    
    CustomFilterProgram* program() const { return m_program.get(); }
    void setProgram(PassRefPtr<CustomFilterProgram> program) { m_program = program; }
    
    const CustomFilterParameterList& parameters() const { return m_parameters; }
    
    unsigned meshRows() const { return m_meshRows; }
    unsigned meshColumns() const { return m_meshColumns; }

    CustomFilterMeshType meshType() const { return program() ? program()->meshType() : MeshTypeAttached; }
    
    virtual ~CustomFilterOperation();
    
    virtual bool affectsOpacity() const { return true; }
    virtual bool movesPixels() const { return true; }
    virtual bool blendingNeedsRendererSize() const { return true; }
    
    virtual PassRefPtr<FilterOperation> blend(const FilterOperation* from, double progress, const LayoutSize&, bool blendToPassthrough = false);

protected:
    CustomFilterOperation(PassRefPtr<CustomFilterProgram>, const CustomFilterParameterList&, unsigned meshRows, unsigned meshColumns);
    
private:
    virtual bool operator==(const FilterOperation& o) const
    {
        if (!isSameType(o))
            return false;

        const CustomFilterOperation* other = static_cast<const CustomFilterOperation*>(&o);
        return m_program.get() == other->m_program.get()
            && m_meshRows == other->m_meshRows
            && m_meshColumns == other->m_meshColumns
            && m_parameters == other->m_parameters;
    }

    RefPtr<CustomFilterProgram> m_program;
    CustomFilterParameterList m_parameters;
    
    unsigned m_meshRows;
    unsigned m_meshColumns;
};

} // namespace WebCore

#endif // ENABLE(CSS_SHADERS)

#endif // CustomFilterOperation_h
