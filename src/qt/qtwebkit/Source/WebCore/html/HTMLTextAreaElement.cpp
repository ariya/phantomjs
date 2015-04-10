/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2010 Apple Inc. All rights reserved.
 *           (C) 2006 Alexey Proskuryakov (ap@nypop.com)
 * Copyright (C) 2007 Samuel Weinig (sam@webkit.org)
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

#include "config.h"
#include "HTMLTextAreaElement.h"

#include "Attribute.h"
#include "BeforeTextInsertedEvent.h"
#include "CSSValueKeywords.h"
#include "Document.h"
#include "Editor.h"
#include "ElementShadow.h"
#include "Event.h"
#include "EventHandler.h"
#include "EventNames.h"
#include "ExceptionCode.h"
#include "ExceptionCodePlaceholder.h"
#include "FormController.h"
#include "FormDataList.h"
#include "Frame.h"
#include "FrameSelection.h"
#include "HTMLNames.h"
#include "LocalizedStrings.h"
#include "RenderTextControlMultiLine.h"
#include "ShadowRoot.h"
#include "Text.h"
#include "TextControlInnerElements.h"
#include "TextIterator.h"
#include <wtf/StdLibExtras.h>
#include <wtf/text/StringBuilder.h>

namespace WebCore {

using namespace HTMLNames;

static const int defaultRows = 2;
static const int defaultCols = 20;

// On submission, LF characters are converted into CRLF.
// This function returns number of characters considering this.
static inline unsigned computeLengthForSubmission(const String& text, unsigned numberOfLineBreaks)
{
    return numGraphemeClusters(text) + numberOfLineBreaks;
}

static unsigned numberOfLineBreaks(const String& text)
{
    unsigned length = text.length();
    unsigned count = 0;
    for (unsigned i = 0; i < length; i++) {
        if (text[i] == '\n')
            count++;
    }
    return count;
}

static inline unsigned computeLengthForSubmission(const String& text)
{
    return numGraphemeClusters(text) + numberOfLineBreaks(text);
}

static inline unsigned upperBoundForLengthForSubmission(const String& text, unsigned numberOfLineBreaks)
{
    return text.length() + numberOfLineBreaks;
}

HTMLTextAreaElement::HTMLTextAreaElement(const QualifiedName& tagName, Document* document, HTMLFormElement* form)
    : HTMLTextFormControlElement(tagName, document, form)
    , m_rows(defaultRows)
    , m_cols(defaultCols)
    , m_wrap(SoftWrap)
    , m_placeholder(0)
    , m_isDirty(false)
    , m_wasModifiedByUser(false)
{
    ASSERT(hasTagName(textareaTag));
    setFormControlValueMatchesRenderer(true);
}

PassRefPtr<HTMLTextAreaElement> HTMLTextAreaElement::create(const QualifiedName& tagName, Document* document, HTMLFormElement* form)
{
    RefPtr<HTMLTextAreaElement> textArea = adoptRef(new HTMLTextAreaElement(tagName, document, form));
    textArea->ensureUserAgentShadowRoot();
    return textArea.release();
}

void HTMLTextAreaElement::didAddUserAgentShadowRoot(ShadowRoot* root)
{
    root->appendChild(TextControlInnerTextElement::create(document()), ASSERT_NO_EXCEPTION);
}

const AtomicString& HTMLTextAreaElement::formControlType() const
{
    DEFINE_STATIC_LOCAL(const AtomicString, textarea, ("textarea", AtomicString::ConstructFromLiteral));
    return textarea;
}

FormControlState HTMLTextAreaElement::saveFormControlState() const
{
    return m_isDirty ? FormControlState(value()) : FormControlState();
}

void HTMLTextAreaElement::restoreFormControlState(const FormControlState& state)
{
    setValue(state[0]);
}

void HTMLTextAreaElement::childrenChanged(bool changedByParser, Node* beforeChange, Node* afterChange, int childCountDelta)
{
    HTMLElement::childrenChanged(changedByParser, beforeChange, afterChange, childCountDelta);
    setLastChangeWasNotUserEdit();
    if (m_isDirty)
        setInnerTextValue(value());
    else
        setNonDirtyValue(defaultValue());
}

bool HTMLTextAreaElement::isPresentationAttribute(const QualifiedName& name) const
{
    if (name == alignAttr) {
        // Don't map 'align' attribute.  This matches what Firefox, Opera and IE do.
        // See http://bugs.webkit.org/show_bug.cgi?id=7075
        return false;
    }

    if (name == wrapAttr)
        return true;
    return HTMLTextFormControlElement::isPresentationAttribute(name);
}

void HTMLTextAreaElement::collectStyleForPresentationAttribute(const QualifiedName& name, const AtomicString& value, MutableStylePropertySet* style)
{
    if (name == wrapAttr) {
        if (shouldWrapText()) {
            addPropertyToPresentationAttributeStyle(style, CSSPropertyWhiteSpace, CSSValuePreWrap);
            addPropertyToPresentationAttributeStyle(style, CSSPropertyWordWrap, CSSValueBreakWord);
        } else {
            addPropertyToPresentationAttributeStyle(style, CSSPropertyWhiteSpace, CSSValuePre);
            addPropertyToPresentationAttributeStyle(style, CSSPropertyWordWrap, CSSValueNormal);
        }
    } else
        HTMLTextFormControlElement::collectStyleForPresentationAttribute(name, value, style);
}

void HTMLTextAreaElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    if (name == rowsAttr) {
        int rows = value.toInt();
        if (rows <= 0)
            rows = defaultRows;
        if (m_rows != rows) {
            m_rows = rows;
            if (renderer())
                renderer()->setNeedsLayoutAndPrefWidthsRecalc();
        }
    } else if (name == colsAttr) {
        int cols = value.toInt();
        if (cols <= 0)
            cols = defaultCols;
        if (m_cols != cols) {
            m_cols = cols;
            if (renderer())
                renderer()->setNeedsLayoutAndPrefWidthsRecalc();
        }
    } else if (name == wrapAttr) {
        // The virtual/physical values were a Netscape extension of HTML 3.0, now deprecated.
        // The soft/hard /off values are a recommendation for HTML 4 extension by IE and NS 4.
        WrapMethod wrap;
        if (equalIgnoringCase(value, "physical") || equalIgnoringCase(value, "hard") || equalIgnoringCase(value, "on"))
            wrap = HardWrap;
        else if (equalIgnoringCase(value, "off"))
            wrap = NoWrap;
        else
            wrap = SoftWrap;
        if (wrap != m_wrap) {
            m_wrap = wrap;
            if (renderer())
                renderer()->setNeedsLayoutAndPrefWidthsRecalc();
        }
    } else if (name == accesskeyAttr) {
        // ignore for the moment
    } else if (name == maxlengthAttr)
        setNeedsValidityCheck();
    else
        HTMLTextFormControlElement::parseAttribute(name, value);
}

RenderObject* HTMLTextAreaElement::createRenderer(RenderArena* arena, RenderStyle*)
{
    return new (arena) RenderTextControlMultiLine(this);
}

bool HTMLTextAreaElement::appendFormData(FormDataList& encoding, bool)
{
    if (name().isEmpty())
        return false;

    document()->updateLayout();

    const String& text = (m_wrap == HardWrap) ? valueWithHardLineBreaks() : value();
    encoding.appendData(name(), text);

    const AtomicString& dirnameAttrValue = fastGetAttribute(dirnameAttr);
    if (!dirnameAttrValue.isNull())
        encoding.appendData(dirnameAttrValue, directionForFormData());
    return true;    
}

void HTMLTextAreaElement::reset()
{
    setNonDirtyValue(defaultValue());
}

bool HTMLTextAreaElement::hasCustomFocusLogic() const
{
    return true;
}

bool HTMLTextAreaElement::isKeyboardFocusable(KeyboardEvent*) const
{
    // If a given text area can be focused at all, then it will always be keyboard focusable.
    return isFocusable();
}

bool HTMLTextAreaElement::isMouseFocusable() const
{
    return isFocusable();
}

void HTMLTextAreaElement::updateFocusAppearance(bool restorePreviousSelection)
{
    if (!restorePreviousSelection || !hasCachedSelection()) {
        // If this is the first focus, set a caret at the beginning of the text.  
        // This matches some browsers' behavior; see bug 11746 Comment #15.
        // http://bugs.webkit.org/show_bug.cgi?id=11746#c15
        setSelectionRange(0, 0);
    } else
        restoreCachedSelection();

    if (document()->frame())
        document()->frame()->selection()->revealSelection();
}

void HTMLTextAreaElement::defaultEventHandler(Event* event)
{
    if (renderer() && (event->isMouseEvent() || event->isDragEvent() || event->hasInterface(eventNames().interfaceForWheelEvent) || event->type() == eventNames().blurEvent))
        forwardEvent(event);
    else if (renderer() && event->isBeforeTextInsertedEvent())
        handleBeforeTextInsertedEvent(static_cast<BeforeTextInsertedEvent*>(event));

    HTMLTextFormControlElement::defaultEventHandler(event);
}

void HTMLTextAreaElement::subtreeHasChanged()
{
    setChangedSinceLastFormControlChangeEvent(true);
    setFormControlValueMatchesRenderer(false);
    setNeedsValidityCheck();

    if (!focused())
        return;

    if (Frame* frame = document()->frame())
        frame->editor().textDidChangeInTextArea(this);
    // When typing in a textarea, childrenChanged is not called, so we need to force the directionality check.
    calculateAndAdjustDirectionality();
}

void HTMLTextAreaElement::handleBeforeTextInsertedEvent(BeforeTextInsertedEvent* event) const
{
    ASSERT(event);
    ASSERT(renderer());
    int signedMaxLength = maxLength();
    if (signedMaxLength < 0)
        return;
    unsigned unsignedMaxLength = static_cast<unsigned>(signedMaxLength);

    const String& currentValue = innerTextValue();
    unsigned numberOfLineBreaksInCurrentValue = numberOfLineBreaks(currentValue);
    if (upperBoundForLengthForSubmission(currentValue, numberOfLineBreaksInCurrentValue)
        + upperBoundForLengthForSubmission(event->text(), numberOfLineBreaks(event->text())) < unsignedMaxLength)
        return;

    unsigned currentLength = computeLengthForSubmission(currentValue, numberOfLineBreaksInCurrentValue);
    // selectionLength represents the selection length of this text field to be
    // removed by this insertion.
    // If the text field has no focus, we don't need to take account of the
    // selection length. The selection is the source of text drag-and-drop in
    // that case, and nothing in the text field will be removed.
    unsigned selectionLength = focused() ? computeLengthForSubmission(plainText(document()->frame()->selection()->selection().toNormalizedRange().get())) : 0;
    ASSERT(currentLength >= selectionLength);
    unsigned baseLength = currentLength - selectionLength;
    unsigned appendableLength = unsignedMaxLength > baseLength ? unsignedMaxLength - baseLength : 0;
    event->setText(sanitizeUserInputValue(event->text(), appendableLength));
}

String HTMLTextAreaElement::sanitizeUserInputValue(const String& proposedValue, unsigned maxLength)
{
    return proposedValue.left(numCharactersInGraphemeClusters(proposedValue, maxLength));
}

HTMLElement* HTMLTextAreaElement::innerTextElement() const
{
    Node* node = userAgentShadowRoot()->firstChild();
    ASSERT(!node || node->hasTagName(divTag));
    return toHTMLElement(node);
}

void HTMLTextAreaElement::rendererWillBeDestroyed()
{
    updateValue();
}

void HTMLTextAreaElement::updateValue() const
{
    if (formControlValueMatchesRenderer())
        return;

    ASSERT(renderer());
    m_value = innerTextValue();
    const_cast<HTMLTextAreaElement*>(this)->setFormControlValueMatchesRenderer(true);
    const_cast<HTMLTextAreaElement*>(this)->notifyFormStateChanged();
    m_isDirty = true;
    m_wasModifiedByUser = true;
    const_cast<HTMLTextAreaElement*>(this)->updatePlaceholderVisibility(false);
}

String HTMLTextAreaElement::value() const
{
    updateValue();
    return m_value;
}

void HTMLTextAreaElement::setValue(const String& value)
{
    setValueCommon(value);
    m_isDirty = true;
    setNeedsValidityCheck();
}

void HTMLTextAreaElement::setNonDirtyValue(const String& value)
{
    setValueCommon(value);
    m_isDirty = false;
    setNeedsValidityCheck();
}

void HTMLTextAreaElement::setValueCommon(const String& newValue)
{
    m_wasModifiedByUser = false;
    // Code elsewhere normalizes line endings added by the user via the keyboard or pasting.
    // We normalize line endings coming from JavaScript here.
    String normalizedValue = newValue.isNull() ? "" : newValue;
    normalizedValue.replace("\r\n", "\n");
    normalizedValue.replace('\r', '\n');

    // Return early because we don't want to move the caret or trigger other side effects
    // when the value isn't changing. This matches Firefox behavior, at least.
    if (normalizedValue == value())
        return;

    m_value = normalizedValue;
    setInnerTextValue(m_value);
    setLastChangeWasNotUserEdit();
    updatePlaceholderVisibility(false);
    setNeedsStyleRecalc();
    setFormControlValueMatchesRenderer(true);

    // Set the caret to the end of the text value.
    if (document()->focusedElement() == this) {
        unsigned endOfString = m_value.length();
        setSelectionRange(endOfString, endOfString);
    }

    notifyFormStateChanged();
    setTextAsOfLastFormControlChangeEvent(normalizedValue);
}

String HTMLTextAreaElement::defaultValue() const
{
    StringBuilder value;

    // Since there may be comments, ignore nodes other than text nodes.
    for (Node* n = firstChild(); n; n = n->nextSibling()) {
        if (n->isTextNode())
            value.append(toText(n)->data());
    }

    return value.toString();
}

void HTMLTextAreaElement::setDefaultValue(const String& defaultValue)
{
    RefPtr<Node> protectFromMutationEvents(this);

    // To preserve comments, remove only the text nodes, then add a single text node.
    Vector<RefPtr<Node> > textNodes;
    for (Node* n = firstChild(); n; n = n->nextSibling()) {
        if (n->isTextNode())
            textNodes.append(n);
    }
    size_t size = textNodes.size();
    for (size_t i = 0; i < size; ++i)
        removeChild(textNodes[i].get(), IGNORE_EXCEPTION);

    // Normalize line endings.
    String value = defaultValue;
    value.replace("\r\n", "\n");
    value.replace('\r', '\n');

    insertBefore(document()->createTextNode(value), firstChild(), IGNORE_EXCEPTION);

    if (!m_isDirty)
        setNonDirtyValue(value);
}

int HTMLTextAreaElement::maxLength() const
{
    bool ok;
    int value = getAttribute(maxlengthAttr).string().toInt(&ok);
    return ok && value >= 0 ? value : -1;
}

void HTMLTextAreaElement::setMaxLength(int newValue, ExceptionCode& ec)
{
    if (newValue < 0)
        ec = INDEX_SIZE_ERR;
    else
        setAttribute(maxlengthAttr, String::number(newValue));
}

String HTMLTextAreaElement::validationMessage() const
{
    if (!willValidate())
        return String();

    if (customError())
        return customValidationMessage();

    if (valueMissing())
        return validationMessageValueMissingText();

    if (tooLong())
        return validationMessageTooLongText(computeLengthForSubmission(value()), maxLength());

    return String();
}

bool HTMLTextAreaElement::valueMissing() const
{
    return willValidate() && valueMissing(value());
}

bool HTMLTextAreaElement::tooLong() const
{
    return willValidate() && tooLong(value(), CheckDirtyFlag);
}

bool HTMLTextAreaElement::tooLong(const String& value, NeedsToCheckDirtyFlag check) const
{
    // Return false for the default value or value set by script even if it is
    // longer than maxLength.
    if (check == CheckDirtyFlag && !m_wasModifiedByUser)
        return false;

    int max = maxLength();
    if (max < 0)
        return false;
    unsigned unsignedMax = static_cast<unsigned>(max);
    unsigned numberOfLineBreaksInValue = numberOfLineBreaks(value);
    return upperBoundForLengthForSubmission(value, numberOfLineBreaksInValue) > unsignedMax
        && computeLengthForSubmission(value, numberOfLineBreaksInValue) > unsignedMax;
}

bool HTMLTextAreaElement::isValidValue(const String& candidate) const
{
    return !valueMissing(candidate) && !tooLong(candidate, IgnoreDirtyFlag);
}

void HTMLTextAreaElement::accessKeyAction(bool)
{
    focus();
}

void HTMLTextAreaElement::setCols(int cols)
{
    setAttribute(colsAttr, String::number(cols));
}

void HTMLTextAreaElement::setRows(int rows)
{
    setAttribute(rowsAttr, String::number(rows));
}

bool HTMLTextAreaElement::shouldUseInputMethod()
{
    return true;
}

HTMLElement* HTMLTextAreaElement::placeholderElement() const
{
    return m_placeholder;
}

void HTMLTextAreaElement::attach(const AttachContext& context)
{
    HTMLTextFormControlElement::attach(context);
    fixPlaceholderRenderer(m_placeholder, innerTextElement());
}

bool HTMLTextAreaElement::matchesReadOnlyPseudoClass() const
{
    return isReadOnly();
}

bool HTMLTextAreaElement::matchesReadWritePseudoClass() const
{
    return !isReadOnly();
}

void HTMLTextAreaElement::updatePlaceholderText()
{
    String placeholderText = strippedPlaceholder();
    if (placeholderText.isEmpty()) {
        if (m_placeholder) {
            userAgentShadowRoot()->removeChild(m_placeholder, ASSERT_NO_EXCEPTION);
            m_placeholder = 0;
        }
        return;
    }
    if (!m_placeholder) {
        RefPtr<HTMLDivElement> placeholder = HTMLDivElement::create(document());
        m_placeholder = placeholder.get();
        m_placeholder->setPseudo(AtomicString("-webkit-input-placeholder", AtomicString::ConstructFromLiteral));
        userAgentShadowRoot()->insertBefore(m_placeholder, innerTextElement()->nextSibling(), ASSERT_NO_EXCEPTION);
    }
    m_placeholder->setInnerText(placeholderText, ASSERT_NO_EXCEPTION);
    fixPlaceholderRenderer(m_placeholder, innerTextElement());
}

}
