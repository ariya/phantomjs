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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER "AS IS" AND ANY
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

#ifndef ValidatedCustomFilterOperation_h
#define ValidatedCustomFilterOperation_h

#if ENABLE(CSS_SHADERS)
#include "CustomFilterConstants.h"
#include "CustomFilterParameterList.h"
#include "FilterOperation.h"
#include "LayoutSize.h"

namespace WebCore {

class CustomFilterValidatedProgram;

class ValidatedCustomFilterOperation : public FilterOperation {
public:
    static PassRefPtr<ValidatedCustomFilterOperation> create(PassRefPtr<CustomFilterValidatedProgram> validatedProgram, 
        const CustomFilterParameterList& sortedParameters, unsigned meshRows, unsigned meshColumns, CustomFilterMeshType meshType)
    {
        return adoptRef(new ValidatedCustomFilterOperation(validatedProgram, sortedParameters, meshRows, meshColumns, meshType));
    }

    virtual ~ValidatedCustomFilterOperation();

    virtual bool affectsOpacity() const { return true; }
    virtual bool movesPixels() const { return true; }
    virtual bool blendingNeedsRendererSize() const { return true; }

    virtual PassRefPtr<FilterOperation> blend(const FilterOperation* from, double progress, const LayoutSize&, bool blendToPassthrough = false);

    CustomFilterValidatedProgram* validatedProgram() const { return m_validatedProgram.get(); }
    const CustomFilterParameterList& parameters() const { return m_parameters; }

    unsigned meshRows() const { return m_meshRows; }
    unsigned meshColumns() const { return m_meshColumns; }

    CustomFilterMeshType meshType() const { return m_meshType; }

private:
    virtual bool operator==(const FilterOperation& o) const
    {
        if (!isSameType(o))
            return false;

        const ValidatedCustomFilterOperation* other = static_cast<const ValidatedCustomFilterOperation*>(&o);
        return m_validatedProgram.get() == other->m_validatedProgram.get()
            && m_meshRows == other->m_meshRows
            && m_meshColumns == other->m_meshColumns
            && m_meshType == other->m_meshType
            && m_parameters == other->m_parameters;
    }

    ValidatedCustomFilterOperation(PassRefPtr<CustomFilterValidatedProgram>, const CustomFilterParameterList&, unsigned meshRows, unsigned meshColumns, CustomFilterMeshType);

    RefPtr<CustomFilterValidatedProgram> m_validatedProgram;

    CustomFilterParameterList m_parameters;
    unsigned m_meshRows;
    unsigned m_meshColumns;
    CustomFilterMeshType m_meshType;
};

} // namespace WebCore

#endif // ENABLE(CSS_SHADERS)

#endif // ValidatedCustomFilterOperation_h
