/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef StaticPropertyAnalysis_h
#define StaticPropertyAnalysis_h

#include "Executable.h"
#include "JSGlobalObject.h"
#include <wtf/HashSet.h>

namespace JSC {

// Reference count indicates number of live registers that alias this object.
class StaticPropertyAnalysis : public RefCounted<StaticPropertyAnalysis> {
public:
    static PassRefPtr<StaticPropertyAnalysis> create(Vector<UnlinkedInstruction, 0, UnsafeVectorOverflow>* instructions, unsigned target)
    {
        return adoptRef(new StaticPropertyAnalysis(instructions, target)); 
    }

    void addPropertyIndex(unsigned propertyIndex) { m_propertyIndexes.add(propertyIndex); }

    void record()
    {
        (*m_instructions)[m_target] = m_propertyIndexes.size();
    }

    int propertyIndexCount() { return m_propertyIndexes.size(); }

private:
    StaticPropertyAnalysis(Vector<UnlinkedInstruction, 0, UnsafeVectorOverflow>* instructions, unsigned target)
        : m_instructions(instructions)
        , m_target(target)
    {
    }

    Vector<UnlinkedInstruction, 0, UnsafeVectorOverflow>* m_instructions;
    unsigned m_target;
    typedef HashSet<unsigned, WTF::IntHash<unsigned>, WTF::UnsignedWithZeroKeyHashTraits<unsigned> > PropertyIndexSet;
    PropertyIndexSet m_propertyIndexes;
};

} // namespace JSC

#endif // StaticPropertyAnalysis_h
