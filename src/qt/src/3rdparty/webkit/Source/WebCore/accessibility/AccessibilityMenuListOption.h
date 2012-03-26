/*
 * Copyright (C) 2010 Apple Inc. All Rights Reserved.
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

#ifndef AccessibilityMenuListOption_h
#define AccessibilityMenuListOption_h

#include "AccessibilityObject.h"

namespace WebCore {

class AccessibilityMenuListPopup;
class HTMLElement;

class AccessibilityMenuListOption : public AccessibilityObject {
public:
    static PassRefPtr<AccessibilityMenuListOption> create() { return adoptRef(new AccessibilityMenuListOption); }

    void setElement(HTMLElement*);
    void setParent(AccessibilityMenuListPopup* popup) { m_popup = popup; }

private:
    AccessibilityMenuListOption();

    virtual bool isMenuListOption() const { return true; }

    virtual AccessibilityRole roleValue() const { return MenuListOptionRole; }
    virtual bool canHaveChildren() const { return false; }
    virtual IntSize size() const { return elementRect().size(); }

    virtual Element* actionElement() const;
    virtual AccessibilityObject* parentObject() const;
    virtual bool isEnabled() const;
    virtual bool isVisible() const;
    virtual bool isOffScreen() const;
    virtual bool isSelected() const;
    virtual String nameForMSAA() const;
    virtual void setSelected(bool);
    virtual bool canSetSelectedAttribute() const;
    virtual IntRect elementRect() const;
    virtual String stringValue() const;

    RefPtr<HTMLElement> m_element;
    AccessibilityMenuListPopup* m_popup;
};

} // namespace WebCore

#endif // AccessibilityMenuListOption_h
