/*
 * Copyright (C) 2008, 2009, 2010, 2013 Apple Inc. All Rights Reserved.
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

#ifndef AccessibleBase_h
#define AccessibleBase_h

#include "WebKit.h"
#include <WebCore/AccessibilityObject.h>
#include <WebCore/AccessibilityObjectWrapperWin.h>

class DECLSPEC_UUID("3dbd565b-db22-4d88-8e0e-778bde54524a") AccessibleBase : public IAccessibleComparable, public IServiceProvider, public WebCore::AccessibilityObjectWrapper {
public:
    static AccessibleBase* createInstance(WebCore::AccessibilityObject*, HWND);

    // IServiceProvider
    virtual HRESULT STDMETHODCALLTYPE QueryService(REFGUID guidService, REFIID riid, void **ppv);

    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void) { return ++m_refCount; }
    virtual ULONG STDMETHODCALLTYPE Release(void);

    // IAccessible2_2
    virtual HRESULT STDMETHODCALLTYPE get_attribute(BSTR name, VARIANT* attribute);
    virtual HRESULT STDMETHODCALLTYPE get_accessibleWithCaret(IUnknown** accessible, long* caretOffset);
    virtual HRESULT STDMETHODCALLTYPE get_relationTargetsOfType(BSTR type, long maxTargets, IUnknown*** targets, long* nTargets);

    // IAccessible2
    virtual HRESULT STDMETHODCALLTYPE get_nRelations(long*);
    virtual HRESULT STDMETHODCALLTYPE get_relation(long relationIndex, IAccessibleRelation**);
    virtual HRESULT STDMETHODCALLTYPE get_relations(long maxRelations, IAccessibleRelation** relations, long* nRelations);
    virtual HRESULT STDMETHODCALLTYPE role(long*);
    virtual HRESULT STDMETHODCALLTYPE scrollTo(IA2ScrollType);
    virtual HRESULT STDMETHODCALLTYPE scrollToPoint(IA2CoordinateType, long x, long y);
    virtual HRESULT STDMETHODCALLTYPE get_groupPosition(long* groupLevel, long* similarItemsInGroup, long* positionInGroup);
    virtual HRESULT STDMETHODCALLTYPE get_states(AccessibleStates*);
    virtual HRESULT STDMETHODCALLTYPE get_extendedRole(BSTR*);
    virtual HRESULT STDMETHODCALLTYPE get_localizedExtendedRole(BSTR*);
    virtual HRESULT STDMETHODCALLTYPE get_nExtendedStates(long*);
    virtual HRESULT STDMETHODCALLTYPE get_extendedStates(long maxExtendedStates, BSTR** extendedStates, long* nExtendedStates);
    virtual HRESULT STDMETHODCALLTYPE get_localizedExtendedStates(long maxLocalizedExtendedStates, BSTR** localizedExtendedStates, long* nLocalizedExtendedStates);
    virtual HRESULT STDMETHODCALLTYPE get_uniqueID(long*);
    virtual HRESULT STDMETHODCALLTYPE get_windowHandle(HWND*);
    virtual HRESULT STDMETHODCALLTYPE get_indexInParent(long*);
    virtual HRESULT STDMETHODCALLTYPE get_locale(IA2Locale*);
    virtual HRESULT STDMETHODCALLTYPE get_attributes(BSTR*);

    // IAccessible
    virtual HRESULT STDMETHODCALLTYPE get_accParent(IDispatch**);
    virtual HRESULT STDMETHODCALLTYPE get_accChildCount(long*);
    virtual HRESULT STDMETHODCALLTYPE get_accChild(VARIANT vChild, IDispatch** ppChild);
    virtual HRESULT STDMETHODCALLTYPE get_accName(VARIANT vChild, BSTR*);
    virtual HRESULT STDMETHODCALLTYPE get_accValue(VARIANT vChild, BSTR*);
    virtual HRESULT STDMETHODCALLTYPE get_accDescription(VARIANT, BSTR*);
    virtual HRESULT STDMETHODCALLTYPE get_accRole(VARIANT vChild, VARIANT* pvRole);
    virtual HRESULT STDMETHODCALLTYPE get_accState(VARIANT vChild, VARIANT* pvState);
    virtual HRESULT STDMETHODCALLTYPE get_accHelp(VARIANT vChild, BSTR* helpText);
    virtual HRESULT STDMETHODCALLTYPE get_accKeyboardShortcut(VARIANT vChild, BSTR*);
    virtual HRESULT STDMETHODCALLTYPE get_accFocus(VARIANT* pvFocusedChild);
    virtual HRESULT STDMETHODCALLTYPE get_accSelection(VARIANT* pvSelectedChild);
    virtual HRESULT STDMETHODCALLTYPE get_accDefaultAction(VARIANT vChild, BSTR* actionDescription);
    virtual HRESULT STDMETHODCALLTYPE accSelect(long selectionFlags, VARIANT vChild);
    virtual HRESULT STDMETHODCALLTYPE accLocation(long* left, long* top, long* width, long* height, VARIANT vChild);
    virtual HRESULT STDMETHODCALLTYPE accNavigate(long direction, VARIANT vFromChild, VARIANT* pvNavigatedTo);
    virtual HRESULT STDMETHODCALLTYPE accHitTest(long x, long y, VARIANT* pvChildAtPoint);
    virtual HRESULT STDMETHODCALLTYPE accDoDefaultAction(VARIANT vChild);

    // IAccessible - Not to be implemented.
    virtual HRESULT STDMETHODCALLTYPE put_accName(VARIANT, BSTR) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE put_accValue(VARIANT, BSTR) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE get_accHelpTopic(BSTR* helpFile, VARIANT, long* topicID)
    {
        *helpFile = 0;
        *topicID = 0;
        return E_NOTIMPL;
    }

    // IDispatch - Not to be implemented.
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT* count)
    {
        *count = 0;
        return E_NOTIMPL;
    }
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT, LCID, ITypeInfo** ppTInfo)
    {
        *ppTInfo = 0;
        return E_NOTIMPL;
    }
    virtual HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID, LPOLESTR*, UINT, LCID, DISPID*) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE Invoke(DISPID, REFIID, LCID, WORD, DISPPARAMS*, VARIANT*, EXCEPINFO*, UINT*) { return E_NOTIMPL; }

    // WebCore::AccessiblityObjectWrapper
    virtual void detach() {
        ASSERT(m_object);
        m_object = 0;
    }

    // IAccessibleComparable
    virtual HRESULT STDMETHODCALLTYPE isSameObject(IAccessibleComparable* other, BOOL* result);

protected:
    AccessibleBase(WebCore::AccessibilityObject*, HWND);
    virtual ~AccessibleBase();

    virtual WTF::String name() const;
    virtual WTF::String value() const;
    virtual long role() const;
    virtual long state() const;

    HRESULT getAccessibilityObjectForChild(VARIANT vChild, WebCore::AccessibilityObject*&) const;

    AccessibleBase* wrapper(WebCore::AccessibilityObject*) const;

    HWND m_window;
    int m_refCount;

private:
    AccessibleBase() { }
};

#endif // AccessibleBase_h

