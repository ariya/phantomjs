/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 * Copyright (C) 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef UserActionElementSet_h
#define UserActionElementSet_h

#include <wtf/HashMap.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class Element;

class UserActionElementSet {
public:
    static PassOwnPtr<UserActionElementSet> create() { return adoptPtr(new UserActionElementSet()); }

    bool isFocused(const Element* element) { return hasFlags(element, IsFocusedFlag); }
    bool isActive(const Element* element) { return hasFlags(element, IsActiveFlag); }
    bool isInActiveChain(const Element* element) { return hasFlags(element, InActiveChainFlag); }
    bool isHovered(const Element* element) { return hasFlags(element, IsHoveredFlag); }
    void setFocused(Element* element, bool enable) { setFlags(element, enable, IsFocusedFlag); }
    void setActive(Element* element, bool enable) { setFlags(element, enable, IsActiveFlag); }
    void setInActiveChain(Element* element, bool enable) { setFlags(element, enable, InActiveChainFlag); }
    void setHovered(Element* element, bool enable) { setFlags(element, enable, IsHoveredFlag); }

    UserActionElementSet();
    ~UserActionElementSet();

    void didDetach(Element*);
    void documentDidRemoveLastRef();

private:
    enum ElementFlags {
        IsActiveFlag      = 1 ,
        InActiveChainFlag = 1 << 1,
        IsHoveredFlag     = 1 << 2,
        IsFocusedFlag     = 1 << 3
    };

    void setFlags(Element* element, bool enable, unsigned flags) { enable ? setFlags(element, flags) : clearFlags(element, flags); }
    void setFlags(Element*, unsigned);
    void clearFlags(Element*, unsigned);
    bool hasFlags(const Element*, unsigned flags) const;

    typedef HashMap<RefPtr<Element>, unsigned> ElementFlagMap;
    ElementFlagMap m_elements;
};

} // namespace

#endif // UserActionElementSet_h
