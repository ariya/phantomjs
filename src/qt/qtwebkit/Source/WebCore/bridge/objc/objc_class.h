/*
 * Copyright (C) 2003, 2012 Apple Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef KJS_BINDINGS_OBJC_CLASS_H
#define KJS_BINDINGS_OBJC_CLASS_H

#include "objc_runtime.h"
#include <wtf/text/WTFString.h>

namespace JSC {
namespace Bindings {

class ObjcClass : public Class
{
protected:
    ObjcClass (ClassStructPtr aClass); // Use classForIsA to create an ObjcClass.
    
public:
    // Return the cached ObjC of the specified name.
    static ObjcClass *classForIsA(ClassStructPtr);
    
    virtual Method* methodNamed(PropertyName, Instance*) const;
    virtual Field *fieldNamed(PropertyName, Instance*) const;

    virtual JSValue fallbackObject(ExecState*, Instance*, PropertyName);
    
    ClassStructPtr isa() { return _isa; }
    
private:
    ClassStructPtr _isa;
    mutable HashMap<String, OwnPtr<Method> > m_methodCache;
    mutable HashMap<String, OwnPtr<Field> > m_fieldCache;
};

} // namespace Bindings
} // namespace JSC

#endif
