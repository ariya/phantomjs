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

#ifndef JSAPIWrapperObject_h
#define JSAPIWrapperObject_h

#include "JSBase.h"
#include "JSDestructibleObject.h"
#include "WeakReferenceHarvester.h"

#if JSC_OBJC_API_ENABLED

namespace JSC {
    
class JSAPIWrapperObject : public JSDestructibleObject {
public:
    typedef JSDestructibleObject Base;
    
    void finishCreation(VM&);
    static void visitChildren(JSCell*, JSC::SlotVisitor&);
    
    void* wrappedObject() { return m_wrappedObject; }
    void setWrappedObject(void*);

protected:
    static const unsigned StructureFlags = OverridesVisitChildren | Base::StructureFlags;
    
    JSAPIWrapperObject(VM&, Structure*);

private:
    void* m_wrappedObject;
};

} // namespace JSC

#endif // JSC_OBJC_API_ENABLED

#endif // JSAPIWrapperObject_h
