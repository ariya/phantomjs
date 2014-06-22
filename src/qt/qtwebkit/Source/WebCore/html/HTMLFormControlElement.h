/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
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
 *
 */

#ifndef HTMLFormControlElement_h
#define HTMLFormControlElement_h

#include "FormAssociatedElement.h"
#include "LabelableElement.h"

namespace WebCore {

class FormDataList;
class HTMLFieldSetElement;
class HTMLFormElement;
class HTMLLegendElement;
class ValidationMessage;
class ValidityState;

// HTMLFormControlElement is the default implementation of FormAssociatedElement,
// and form-associated element implementations should use HTMLFormControlElement
// unless there is a special reason.
class HTMLFormControlElement : public LabelableElement, public FormAssociatedElement {
public:
    virtual ~HTMLFormControlElement();

    HTMLFormElement* form() const { return FormAssociatedElement::form(); }

    String formEnctype() const;
    void setFormEnctype(const String&);
    String formMethod() const;
    void setFormMethod(const String&);
    bool formNoValidate() const;

    void ancestorDisabledStateWasChanged();

    virtual void reset() { }

    virtual bool formControlValueMatchesRenderer() const { return m_valueMatchesRenderer; }
    virtual void setFormControlValueMatchesRenderer(bool b) { m_valueMatchesRenderer = b; }

    bool wasChangedSinceLastFormControlChangeEvent() const { return m_wasChangedSinceLastFormControlChangeEvent; }
    void setChangedSinceLastFormControlChangeEvent(bool);

    virtual void dispatchFormControlChangeEvent();
    void dispatchChangeEvent();
    void dispatchFormControlInputEvent();

    virtual bool isDisabledFormControl() const OVERRIDE;

    virtual bool isFocusable() const OVERRIDE;
    virtual bool isEnumeratable() const { return false; }

    bool isRequired() const;

    const AtomicString& type() const { return formControlType(); }

    virtual const AtomicString& formControlType() const = 0;

    virtual bool canTriggerImplicitSubmission() const { return false; }

    // Override in derived classes to get the encoded name=value pair for submitting.
    // Return true for a successful control (see HTML4-17.13.2).
    virtual bool appendFormData(FormDataList&, bool) { return false; }

    virtual bool isSuccessfulSubmitButton() const { return false; }
    virtual bool isActivatedSubmit() const { return false; }
    virtual void setActivatedSubmit(bool) { }

    virtual bool willValidate() const;
    void updateVisibleValidationMessage();
    void hideVisibleValidationMessage();
    bool checkValidity(Vector<RefPtr<FormAssociatedElement> >* unhandledInvalidControls = 0);
    // This must be called when a validation constraint or control value is changed.
    void setNeedsValidityCheck();
    virtual void setCustomValidity(const String&) OVERRIDE;

    bool isReadOnly() const { return m_isReadOnly; }
    bool isDisabledOrReadOnly() const { return isDisabledFormControl() || m_isReadOnly; }

    bool hasAutofocused() { return m_hasAutofocused; }
    void setAutofocused() { m_hasAutofocused = true; }

    static HTMLFormControlElement* enclosingFormControlElement(Node*);

    using Node::ref;
    using Node::deref;

protected:
    HTMLFormControlElement(const QualifiedName& tagName, Document*, HTMLFormElement*);

    virtual void parseAttribute(const QualifiedName&, const AtomicString&) OVERRIDE;
    virtual void requiredAttributeChanged();
    virtual void disabledAttributeChanged();
    virtual void attach(const AttachContext& = AttachContext()) OVERRIDE;
    virtual InsertionNotificationRequest insertedInto(ContainerNode*) OVERRIDE;
    virtual void removedFrom(ContainerNode*) OVERRIDE;
    virtual void didMoveToNewDocument(Document* oldDocument) OVERRIDE;

    virtual bool supportsFocus() const OVERRIDE;
    virtual bool isKeyboardFocusable(KeyboardEvent*) const OVERRIDE;
    virtual bool isMouseFocusable() const OVERRIDE;

    virtual void didRecalcStyle(StyleChange) OVERRIDE;

    virtual void dispatchBlurEvent(PassRefPtr<Element> newFocusedElement) OVERRIDE;

    // This must be called any time the result of willValidate() has changed.
    void setNeedsWillValidateCheck();
    virtual bool recalcWillValidate() const;

    bool validationMessageShadowTreeContains(Node*) const;

private:
    virtual void refFormAssociatedElement() { ref(); }
    virtual void derefFormAssociatedElement() { deref(); }

    virtual bool isFormControlElement() const { return true; }
    virtual bool alwaysCreateUserAgentShadowRoot() const OVERRIDE { return true; }

    virtual short tabIndex() const OVERRIDE FINAL;

    virtual HTMLFormElement* virtualForm() const;
    virtual bool isDefaultButtonForForm() const;
    virtual bool isValidFormControlElement();
    void updateAncestorDisabledState() const;

    OwnPtr<ValidationMessage> m_validationMessage;
    bool m_disabled : 1;
    bool m_isReadOnly : 1;
    bool m_isRequired : 1;
    bool m_valueMatchesRenderer : 1;

    enum AncestorDisabledState { AncestorDisabledStateUnknown, AncestorDisabledStateEnabled, AncestorDisabledStateDisabled };
    mutable AncestorDisabledState m_ancestorDisabledState;
    enum DataListAncestorState { Unknown, InsideDataList, NotInsideDataList };
    mutable enum DataListAncestorState m_dataListAncestorState;

    // The initial value of m_willValidate depends on the derived class. We can't
    // initialize it with a virtual function in the constructor. m_willValidate
    // is not deterministic as long as m_willValidateInitialized is false.
    mutable bool m_willValidateInitialized: 1;
    mutable bool m_willValidate : 1;

    // Cache of validity()->valid().
    // But "candidate for constraint validation" doesn't affect m_isValid.
    bool m_isValid : 1;

    bool m_wasChangedSinceLastFormControlChangeEvent : 1;

    bool m_hasAutofocused : 1;
};

inline bool isHTMLFormControlElement(const Node* node)
{
    return node->isElementNode() && toElement(node)->isFormControlElement();
}

inline HTMLFormControlElement* toHTMLFormControlElement(Node* node)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!node || isHTMLFormControlElement(node));
    return static_cast<HTMLFormControlElement*>(node);
}

// This will catch anyone doing an unnecessary cast.
void toHTMLFormControlElement(const HTMLFormControlElement*);

} // namespace

#endif
