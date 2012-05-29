/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef AccessibilitySlider_h
#define AccessibilitySlider_h

#include "AccessibilityRenderObject.h"

namespace WebCore {

class HTMLInputElement;

class AccessibilitySlider : public AccessibilityRenderObject {
    
public:
    static PassRefPtr<AccessibilitySlider> create(RenderObject*);
    virtual ~AccessibilitySlider() { }

protected:
    AccessibilitySlider(RenderObject*);

private:
    HTMLInputElement* element() const;
    virtual bool accessibilityIsIgnored() const;
    virtual AccessibilityObject* elementAccessibilityHitTest(const IntPoint&) const;

    virtual AccessibilityRole roleValue() const { return SliderRole; }    
    virtual bool isSlider() const { return true; }
    virtual bool isInputSlider() const { return true; }
    
    virtual const AccessibilityChildrenVector& children();
    virtual void addChildren();
    
    virtual bool canSetValueAttribute() const { return true; }
    const AtomicString& getAttribute(const QualifiedName& attribute) const;
    
    virtual void setValue(const String&);
    virtual float valueForRange() const;
    virtual float maxValueForRange() const;
    virtual float minValueForRange() const;
    virtual AccessibilityOrientation orientation() const;    
};

class AccessibilitySliderThumb : public AccessibilityObject {
    
public:
    static PassRefPtr<AccessibilitySliderThumb> create();
    virtual ~AccessibilitySliderThumb() { }

    virtual AccessibilityRole roleValue() const { return SliderThumbRole; }

    void setParentObject(AccessibilitySlider* slider) { m_parentSlider = slider; }
    virtual AccessibilityObject* parentObject() const { return m_parentSlider; }

    virtual IntSize size() const;
    virtual IntRect elementRect() const;

    virtual bool accessibilityIsIgnored() const;

private:
    AccessibilitySliderThumb();

    AccessibilitySlider* m_parentSlider;
};


} // namespace WebCore

#endif // AccessibilitySlider_h
