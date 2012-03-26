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
#include "HTMLElement.h"

namespace WebCore {

class FormDataList;
class HTMLFormElement;
class RenderTextControl;
class ValidationMessage;
class ValidityState;
class VisibleSelection;

// HTMLFormControlElement is the default implementation of FormAssociatedElement,
// and form-associated element implementations should use HTMLFormControlElement
// unless there is a special reason.
class HTMLFormControlElement : public HTMLElement, public FormAssociatedElement {
public:
    virtual ~HTMLFormControlElement();

    HTMLFormElement* form() const { return FormAssociatedElement::form(); }

    bool formNoValidate() const;

    virtual void reset() { }

    virtual bool formControlValueMatchesRenderer() const { return m_valueMatchesRenderer; }
    virtual void setFormControlValueMatchesRenderer(bool b) { m_valueMatchesRenderer = b; }

    virtual bool wasChangedSinceLastFormControlChangeEvent() const;
    virtual void setChangedSinceLastFormControlChangeEvent(bool);

    virtual void dispatchFormControlChangeEvent();
    virtual void dispatchFormControlInputEvent();

    virtual bool disabled() const { return m_disabled; }
    void setDisabled(bool);

    virtual bool isFocusable() const;
    virtual bool isEnumeratable() const { return false; }

    // Determines whether or not a control will be automatically focused.
    virtual bool autofocus() const;

    bool required() const;

    const AtomicString& type() const { return formControlType(); }

    void setName(const AtomicString& name);

    virtual bool isEnabledFormControl() const { return !disabled(); }
    virtual bool isReadOnlyFormControl() const { return readOnly(); }

    virtual bool isRadioButton() const { return false; }
    virtual bool canTriggerImplicitSubmission() const { return false; }

    // Override in derived classes to get the encoded name=value pair for submitting.
    // Return true for a successful control (see HTML4-17.13.2).
    virtual bool appendFormData(FormDataList&, bool) { return false; }

    virtual bool isSuccessfulSubmitButton() const { return false; }
    virtual bool isActivatedSubmit() const { return false; }
    virtual void setActivatedSubmit(bool) { }

    virtual bool willValidate() const;
    String validationMessage();
    void updateVisibleValidationMessage();
    void hideVisibleValidationMessage();
    bool checkValidity(Vector<RefPtr<FormAssociatedElement> >* unhandledInvalidControls = 0);
    // This must be called when a validation constraint or control value is changed.
    void setNeedsValidityCheck();
    void setCustomValidity(const String&);

    bool isLabelable() const;
    PassRefPtr<NodeList> labels();

    bool readOnly() const { return m_readOnly; }

    virtual void attributeChanged(Attribute*, bool preserveDecls = false);

    using TreeShared<ContainerNode>::ref;
    using TreeShared<ContainerNode>::deref;

protected:
    HTMLFormControlElement(const QualifiedName& tagName, Document*, HTMLFormElement*);

    virtual void parseMappedAttribute(Attribute*);
    virtual void attach();
    virtual void insertedIntoTree(bool deep);
    virtual void removedFromTree(bool deep);
    virtual void insertedIntoDocument();
    virtual void removedFromDocument();
    virtual void willMoveToNewOwnerDocument();

    virtual bool isKeyboardFocusable(KeyboardEvent*) const;
    virtual bool isMouseFocusable() const;

    virtual void recalcStyle(StyleChange);

    virtual void dispatchFocusEvent();
    virtual void dispatchBlurEvent();
    virtual void detach();

    // This must be called any time the result of willValidate() has changed.
    void setNeedsWillValidateCheck();
    virtual bool recalcWillValidate() const;

private:
    virtual const AtomicString& formControlName() const;
    virtual const AtomicString& formControlType() const = 0;

    virtual void refFormAssociatedElement() { ref(); }
    virtual void derefFormAssociatedElement() { deref(); }

    virtual bool isFormControlElement() const { return true; }

    virtual bool supportsFocus() const;

    virtual short tabIndex() const;

    virtual HTMLFormElement* virtualForm() const;
    virtual bool isDefaultButtonForForm() const;
    virtual bool isValidFormControlElement();
    String visibleValidationMessage() const;

    OwnPtr<ValidationMessage> m_validationMessage;
    bool m_disabled : 1;
    bool m_readOnly : 1;
    bool m_required : 1;
    bool m_valueMatchesRenderer : 1;

    // The initial value of m_willValidate depends on the derived class. We can't
    // initialize it with a virtual function in the constructor. m_willValidate
    // is not deterministic as long as m_willValidateInitialized is false.
    mutable bool m_willValidateInitialized: 1;
    mutable bool m_willValidate : 1;

    // Cache of validity()->valid().
    // But "candidate for constraint validation" doesn't affect m_isValid.
    bool m_isValid : 1;

    bool m_wasChangedSinceLastFormControlChangeEvent : 1;
};

// FIXME: Give this class its own header file.
class HTMLFormControlElementWithState : public HTMLFormControlElement {
public:
    virtual ~HTMLFormControlElementWithState();

protected:
    HTMLFormControlElementWithState(const QualifiedName& tagName, Document*, HTMLFormElement*);

    virtual bool autoComplete() const;
    virtual void finishParsingChildren();
    virtual void willMoveToNewOwnerDocument();
    virtual void didMoveToNewOwnerDocument();
    virtual void defaultEventHandler(Event*);

private:
    virtual bool shouldSaveAndRestoreFormControlState() const;
};

// FIXME: Give this class its own header file.
class HTMLTextFormControlElement : public HTMLFormControlElementWithState {
public:
    // Common flag for HTMLInputElement::tooLong() and HTMLTextAreaElement::tooLong().
    enum NeedsToCheckDirtyFlag {CheckDirtyFlag, IgnoreDirtyFlag};

    virtual ~HTMLTextFormControlElement();

    virtual void insertedIntoDocument();

    // The derived class should return true if placeholder processing is needed.
    virtual bool supportsPlaceholder() const = 0;
    String strippedPlaceholder() const;
    bool placeholderShouldBeVisible() const;

    int selectionStart() const;
    int selectionEnd() const;
    void setSelectionStart(int);
    void setSelectionEnd(int);
    void select();
    void setSelectionRange(int start, int end);
    PassRefPtr<Range> selection() const;

    virtual void dispatchFormControlChangeEvent();

    virtual int maxLength() const = 0;
    virtual String value() const = 0;

protected:
    HTMLTextFormControlElement(const QualifiedName&, Document*, HTMLFormElement*);

    void updatePlaceholderVisibility(bool);

    virtual void parseMappedAttribute(Attribute*);
    virtual void setTextAsOfLastFormControlChangeEvent(String text) { m_textAsOfLastFormControlChangeEvent = text; }

private:
    virtual void dispatchFocusEvent();
    virtual void dispatchBlurEvent();

    bool isPlaceholderEmpty() const;

    virtual int cachedSelectionStart() const = 0;
    virtual int cachedSelectionEnd() const = 0;

    // Returns true if user-editable value is empty. Used to check placeholder visibility.
    virtual bool isEmptyValue() const = 0;
    // Returns true if suggested value is empty. Used to check placeholder visibility.
    virtual bool isEmptySuggestedValue() const { return true; }
    // Called in dispatchFocusEvent(), after placeholder process, before calling parent's dispatchFocusEvent().
    virtual void handleFocusEvent() { }
    // Called in dispatchBlurEvent(), after placeholder process, before calling parent's dispatchBlurEvent().
    virtual void handleBlurEvent() { }

    RenderTextControl* textRendererAfterUpdateLayout();

    String m_textAsOfLastFormControlChangeEvent;
};

} // namespace

#endif
