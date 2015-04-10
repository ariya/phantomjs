/*
 * Copyright (C) 2012 Apple Inc.  All rights reserved.
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

#ifndef PasteboardStrategy_h
#define PasteboardStrategy_h

#include "SharedBuffer.h"
#include <wtf/Forward.h>
#include <wtf/RefCounted.h>
#include <wtf/Vector.h>

namespace WebCore {

class Color;
class KURL;
    
class PasteboardStrategy {
public:
#if PLATFORM(MAC)
    virtual void getTypes(Vector<String>& types, const String& pasteboardName) = 0;
    virtual PassRefPtr<SharedBuffer> bufferForType(const String& pasteboardType, const String& pasteboardName) = 0;
    virtual void getPathnamesForType(Vector<String>& pathnames, const String& pasteboardType, const String& pasteboardName) = 0;
    virtual String stringForType(const String& pasteboardType, const String& pasteboardName) = 0;
    virtual int changeCount(const String& pasteboardName) = 0;
    virtual String uniqueName() = 0;
    virtual Color color(const String& pasteboardName) = 0;
    virtual KURL url(const String& pasteboardName) = 0;
    
    virtual void copy(const String& fromPasteboard, const String& toPasteboard) = 0;
    virtual void addTypes(const Vector<String>& pasteboardTypes, const String& pasteboardName) = 0;
    virtual void setTypes(const Vector<String>& pasteboardTypes, const String& pasteboardName) = 0;
    virtual void setBufferForType(PassRefPtr<SharedBuffer>, const String& pasteboardType, const String& pasteboardName) = 0;
    virtual void setPathnamesForType(const Vector<String>&, const String& pasteboardType, const String& pasteboardName) = 0;
    virtual void setStringForType(const String&, const String& pasteboardType, const String& pasteboardName) = 0;
#endif
protected:
    virtual ~PasteboardStrategy()
    {
    }
};

}

#endif // !PasteboardStrategy_h
