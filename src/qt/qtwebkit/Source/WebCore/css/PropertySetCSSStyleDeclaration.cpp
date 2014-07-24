/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "PropertySetCSSStyleDeclaration.h"

#include "CSSParser.h"
#include "CSSStyleSheet.h"
#include "HTMLNames.h"
#include "InspectorInstrumentation.h"
#include "MutationObserverInterestGroup.h"
#include "MutationRecord.h"
#include "StylePropertySet.h"
#include "StyledElement.h"

using namespace std;

namespace WebCore {

namespace {

class StyleAttributeMutationScope {
    WTF_MAKE_NONCOPYABLE(StyleAttributeMutationScope);
public:
    StyleAttributeMutationScope(PropertySetCSSStyleDeclaration* decl)
    {
        ++s_scopeCount;

        if (s_scopeCount != 1) {
            ASSERT(s_currentDecl == decl);
            return;
        }

        ASSERT(!s_currentDecl);
        s_currentDecl = decl;

        if (!s_currentDecl->parentElement())
            return;

        bool shouldReadOldValue = false;

        m_mutationRecipients = MutationObserverInterestGroup::createForAttributesMutation(s_currentDecl->parentElement(), HTMLNames::styleAttr);
        if (m_mutationRecipients && m_mutationRecipients->isOldValueRequested())
            shouldReadOldValue = true;

        AtomicString oldValue;
        if (shouldReadOldValue)
            oldValue = s_currentDecl->parentElement()->getAttribute(HTMLNames::styleAttr);

        if (m_mutationRecipients) {
            AtomicString requestedOldValue = m_mutationRecipients->isOldValueRequested() ? oldValue : nullAtom;
            m_mutation = MutationRecord::createAttributes(s_currentDecl->parentElement(), HTMLNames::styleAttr, requestedOldValue);
        }
    }

    ~StyleAttributeMutationScope()
    {
        --s_scopeCount;
        if (s_scopeCount)
            return;

        if (m_mutation && s_shouldDeliver)
            m_mutationRecipients->enqueueMutationRecord(m_mutation);

        s_shouldDeliver = false;
        if (!s_shouldNotifyInspector) {
            s_currentDecl = 0;
            return;
        }
        // We have to clear internal state before calling Inspector's code.
        PropertySetCSSStyleDeclaration* localCopyStyleDecl = s_currentDecl;
        s_currentDecl = 0;
        s_shouldNotifyInspector = false;
        if (localCopyStyleDecl->parentElement() && localCopyStyleDecl->parentElement()->document())
            InspectorInstrumentation::didInvalidateStyleAttr(localCopyStyleDecl->parentElement()->document(), localCopyStyleDecl->parentElement());
    }

    void enqueueMutationRecord()
    {
        s_shouldDeliver = true;
    }

    void didInvalidateStyleAttr()
    {
        s_shouldNotifyInspector = true;
    }

private:
    static unsigned s_scopeCount;
    static PropertySetCSSStyleDeclaration* s_currentDecl;
    static bool s_shouldNotifyInspector;
    static bool s_shouldDeliver;

    OwnPtr<MutationObserverInterestGroup> m_mutationRecipients;
    RefPtr<MutationRecord> m_mutation;
};

unsigned StyleAttributeMutationScope::s_scopeCount = 0;
PropertySetCSSStyleDeclaration* StyleAttributeMutationScope::s_currentDecl = 0;
bool StyleAttributeMutationScope::s_shouldNotifyInspector = false;
bool StyleAttributeMutationScope::s_shouldDeliver = false;

} // namespace

void PropertySetCSSStyleDeclaration::ref()
{ 
    m_propertySet->ref();
}

void PropertySetCSSStyleDeclaration::deref()
{
    m_propertySet->deref(); 
}

unsigned PropertySetCSSStyleDeclaration::length() const
{
    return m_propertySet->propertyCount();
}

String PropertySetCSSStyleDeclaration::item(unsigned i) const
{
    if (i >= m_propertySet->propertyCount())
        return "";
    return m_propertySet->propertyAt(i).cssName();
}

String PropertySetCSSStyleDeclaration::cssText() const
{
    return m_propertySet->asText();
}
    
void PropertySetCSSStyleDeclaration::setCssText(const String& text, ExceptionCode& ec)
{
    StyleAttributeMutationScope mutationScope(this);
    willMutate();

    ec = 0;
    // FIXME: Detect syntax errors and set ec.
    m_propertySet->parseDeclaration(text, contextStyleSheet());

    didMutate(PropertyChanged);

    mutationScope.enqueueMutationRecord();    
}

PassRefPtr<CSSValue> PropertySetCSSStyleDeclaration::getPropertyCSSValue(const String& propertyName)
{
    CSSPropertyID propertyID = cssPropertyID(propertyName);
    if (!propertyID)
        return 0;
    return cloneAndCacheForCSSOM(m_propertySet->getPropertyCSSValue(propertyID).get());
}

String PropertySetCSSStyleDeclaration::getPropertyValue(const String &propertyName)
{
    CSSPropertyID propertyID = cssPropertyID(propertyName);
    if (!propertyID)
        return String();
    return m_propertySet->getPropertyValue(propertyID);
}

String PropertySetCSSStyleDeclaration::getPropertyPriority(const String& propertyName)
{
    CSSPropertyID propertyID = cssPropertyID(propertyName);
    if (!propertyID)
        return String();
    return m_propertySet->propertyIsImportant(propertyID) ? "important" : "";
}

String PropertySetCSSStyleDeclaration::getPropertyShorthand(const String& propertyName)
{
    CSSPropertyID propertyID = cssPropertyID(propertyName);
    if (!propertyID)
        return String();
    return m_propertySet->getPropertyShorthand(propertyID);
}

bool PropertySetCSSStyleDeclaration::isPropertyImplicit(const String& propertyName)
{
    CSSPropertyID propertyID = cssPropertyID(propertyName);
    if (!propertyID)
        return false;
    return m_propertySet->isPropertyImplicit(propertyID);
}

void PropertySetCSSStyleDeclaration::setProperty(const String& propertyName, const String& value, const String& priority, ExceptionCode& ec)
{
    StyleAttributeMutationScope mutationScope(this);
    CSSPropertyID propertyID = cssPropertyID(propertyName);
    if (!propertyID)
        return;

    bool important = priority.find("important", 0, false) != notFound;

    willMutate();

    ec = 0;
    bool changed = m_propertySet->setProperty(propertyID, value, important, contextStyleSheet());

    didMutate(changed ? PropertyChanged : NoChanges);

    if (changed) {
        // CSS DOM requires raising SYNTAX_ERR of parsing failed, but this is too dangerous for compatibility,
        // see <http://bugs.webkit.org/show_bug.cgi?id=7296>.
        mutationScope.enqueueMutationRecord();
    }
}

String PropertySetCSSStyleDeclaration::removeProperty(const String& propertyName, ExceptionCode& ec)
{
    StyleAttributeMutationScope mutationScope(this);
    CSSPropertyID propertyID = cssPropertyID(propertyName);
    if (!propertyID)
        return String();

    willMutate();

    ec = 0;
    String result;
    bool changed = m_propertySet->removeProperty(propertyID, &result);

    didMutate(changed ? PropertyChanged : NoChanges);

    if (changed)
        mutationScope.enqueueMutationRecord();
    return result;
}

PassRefPtr<CSSValue> PropertySetCSSStyleDeclaration::getPropertyCSSValueInternal(CSSPropertyID propertyID)
{
    return m_propertySet->getPropertyCSSValue(propertyID);
}

String PropertySetCSSStyleDeclaration::getPropertyValueInternal(CSSPropertyID propertyID)
{ 
    return m_propertySet->getPropertyValue(propertyID);
}

void PropertySetCSSStyleDeclaration::setPropertyInternal(CSSPropertyID propertyID, const String& value, bool important, ExceptionCode& ec)
{ 
    StyleAttributeMutationScope mutationScope(this);
    willMutate();

    ec = 0;
    bool changed = m_propertySet->setProperty(propertyID, value, important, contextStyleSheet());

    didMutate(changed ? PropertyChanged : NoChanges);

    if (changed)
        mutationScope.enqueueMutationRecord();
}

CSSValue* PropertySetCSSStyleDeclaration::cloneAndCacheForCSSOM(CSSValue* internalValue)
{
    if (!internalValue)
        return 0;

    // The map is here to maintain the object identity of the CSSValues over multiple invocations.
    // FIXME: It is likely that the identity is not important for web compatibility and this code should be removed.
    if (!m_cssomCSSValueClones)
        m_cssomCSSValueClones = adoptPtr(new HashMap<CSSValue*, RefPtr<CSSValue> >);
    
    RefPtr<CSSValue>& clonedValue = m_cssomCSSValueClones->add(internalValue, RefPtr<CSSValue>()).iterator->value;
    if (!clonedValue)
        clonedValue = internalValue->cloneForCSSOM();
    return clonedValue.get();
}

StyleSheetContents* PropertySetCSSStyleDeclaration::contextStyleSheet() const
{ 
    CSSStyleSheet* cssStyleSheet = parentStyleSheet();
    return cssStyleSheet ? cssStyleSheet->contents() : 0;
}

PassRefPtr<MutableStylePropertySet> PropertySetCSSStyleDeclaration::copyProperties() const
{
    return m_propertySet->mutableCopy();
}
    
StyleRuleCSSStyleDeclaration::StyleRuleCSSStyleDeclaration(MutableStylePropertySet* propertySet, CSSRule* parentRule)
    : PropertySetCSSStyleDeclaration(propertySet)
    , m_refCount(1)
    , m_parentRule(parentRule) 
{
    m_propertySet->ref();
}

StyleRuleCSSStyleDeclaration::~StyleRuleCSSStyleDeclaration()
{
    m_propertySet->deref();
}

void StyleRuleCSSStyleDeclaration::ref()
{ 
    ++m_refCount;
}

void StyleRuleCSSStyleDeclaration::deref()
{ 
    ASSERT(m_refCount);
    if (!--m_refCount)
        delete this;
}

void StyleRuleCSSStyleDeclaration::willMutate()
{
    if (m_parentRule && m_parentRule->parentStyleSheet())
        m_parentRule->parentStyleSheet()->willMutateRules();
}

void StyleRuleCSSStyleDeclaration::didMutate(MutationType type)
{
    if (type == PropertyChanged)
        m_cssomCSSValueClones.clear();

    // Style sheet mutation needs to be signaled even if the change failed. willMutateRules/didMutateRules must pair.
    if (m_parentRule && m_parentRule->parentStyleSheet())
        m_parentRule->parentStyleSheet()->didMutateRules();
}

CSSStyleSheet* StyleRuleCSSStyleDeclaration::parentStyleSheet() const
{
    return m_parentRule ? m_parentRule->parentStyleSheet() : 0;
}

void StyleRuleCSSStyleDeclaration::reattach(MutableStylePropertySet* propertySet)
{
    ASSERT(propertySet);
    m_propertySet->deref();
    m_propertySet = propertySet;
    m_propertySet->ref();
}

void InlineCSSStyleDeclaration::didMutate(MutationType type)
{
    if (type == NoChanges)
        return;

    m_cssomCSSValueClones.clear();

    if (!m_parentElement)
        return;

    m_parentElement->setNeedsStyleRecalc(InlineStyleChange);
    m_parentElement->invalidateStyleAttribute();
    StyleAttributeMutationScope(this).didInvalidateStyleAttr();
}

CSSStyleSheet* InlineCSSStyleDeclaration::parentStyleSheet() const
{
    return m_parentElement ? m_parentElement->document()->elementSheet() : 0;
}

} // namespace WebCore
