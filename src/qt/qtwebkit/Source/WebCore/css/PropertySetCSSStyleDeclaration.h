/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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

#ifndef PropertySetCSSStyleDeclaration_h
#define PropertySetCSSStyleDeclaration_h

#include "CSSStyleDeclaration.h"
#include <wtf/HashMap.h>
#include <wtf/OwnPtr.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class CSSRule;
class CSSProperty;
class CSSValue;
class MutableStylePropertySet;
class StyleSheetContents;
class StyledElement;

class PropertySetCSSStyleDeclaration : public CSSStyleDeclaration {
public:
    PropertySetCSSStyleDeclaration(MutableStylePropertySet* propertySet) : m_propertySet(propertySet) { }
    
    virtual StyledElement* parentElement() const { return 0; }
    virtual void clearParentElement() { ASSERT_NOT_REACHED(); }
    StyleSheetContents* contextStyleSheet() const;
    
    virtual void ref() OVERRIDE;
    virtual void deref() OVERRIDE;

private:
    virtual CSSRule* parentRule() const OVERRIDE { return 0; };
    virtual unsigned length() const OVERRIDE;
    virtual String item(unsigned index) const OVERRIDE;
    virtual PassRefPtr<CSSValue> getPropertyCSSValue(const String& propertyName) OVERRIDE;
    virtual String getPropertyValue(const String& propertyName) OVERRIDE;
    virtual String getPropertyPriority(const String& propertyName) OVERRIDE;
    virtual String getPropertyShorthand(const String& propertyName) OVERRIDE;
    virtual bool isPropertyImplicit(const String& propertyName) OVERRIDE;
    virtual void setProperty(const String& propertyName, const String& value, const String& priority, ExceptionCode&) OVERRIDE;
    virtual String removeProperty(const String& propertyName, ExceptionCode&) OVERRIDE;
    virtual String cssText() const OVERRIDE;
    virtual void setCssText(const String&, ExceptionCode&) OVERRIDE;
    virtual PassRefPtr<CSSValue> getPropertyCSSValueInternal(CSSPropertyID) OVERRIDE;
    virtual String getPropertyValueInternal(CSSPropertyID) OVERRIDE;
    virtual void setPropertyInternal(CSSPropertyID, const String& value, bool important, ExceptionCode&) OVERRIDE;
    
    virtual PassRefPtr<MutableStylePropertySet> copyProperties() const OVERRIDE;

    CSSValue* cloneAndCacheForCSSOM(CSSValue*);
    
protected:
    enum MutationType { NoChanges, PropertyChanged };
    virtual void willMutate() { }
    virtual void didMutate(MutationType) { }

    MutableStylePropertySet* m_propertySet;
    OwnPtr<HashMap<CSSValue*, RefPtr<CSSValue> > > m_cssomCSSValueClones;
};

class StyleRuleCSSStyleDeclaration : public PropertySetCSSStyleDeclaration
{
public:
    static PassRefPtr<StyleRuleCSSStyleDeclaration> create(MutableStylePropertySet* propertySet, CSSRule* parentRule)
    {
        return adoptRef(new StyleRuleCSSStyleDeclaration(propertySet, parentRule));
    }
    virtual ~StyleRuleCSSStyleDeclaration();

    void clearParentRule() { m_parentRule = 0; }
    
    virtual void ref() OVERRIDE;
    virtual void deref() OVERRIDE;

    void reattach(MutableStylePropertySet*);

private:
    StyleRuleCSSStyleDeclaration(MutableStylePropertySet*, CSSRule*);

    virtual CSSStyleSheet* parentStyleSheet() const OVERRIDE;

    virtual CSSRule* parentRule() const OVERRIDE { return m_parentRule;  }

    virtual void willMutate() OVERRIDE;
    virtual void didMutate(MutationType) OVERRIDE;

    unsigned m_refCount;
    CSSRule* m_parentRule;
};

class InlineCSSStyleDeclaration : public PropertySetCSSStyleDeclaration
{
public:
    InlineCSSStyleDeclaration(MutableStylePropertySet* propertySet, StyledElement* parentElement)
        : PropertySetCSSStyleDeclaration(propertySet)
        , m_parentElement(parentElement) 
    {
    }
    
private:
    virtual CSSStyleSheet* parentStyleSheet() const OVERRIDE;
    virtual StyledElement* parentElement() const OVERRIDE { return m_parentElement; }
    virtual void clearParentElement() OVERRIDE { m_parentElement = 0; }

    virtual void didMutate(MutationType) OVERRIDE;

    StyledElement* m_parentElement;
};

} // namespace WebCore

#endif
