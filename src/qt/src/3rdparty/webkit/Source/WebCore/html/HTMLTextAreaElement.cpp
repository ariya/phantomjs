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
#include "Chrome.h"
#include "ChromeClient.h"
#include "Document.h"
#include "Event.h"
#include "EventNames.h"
#include "ExceptionCode.h"
#include "FocusController.h"
#include "FormDataList.h"
#include "Frame.h"
#include "HTMLNames.h"
#include "InputElement.h"
#include "Page.h"
#include "RenderStyle.h"
#include "RenderTextControlMultiLine.h"
#include "ScriptEventListener.h"
#include "Text.h"
#include "TextIterator.h"
#include "VisibleSelection.h"
#include <wtf/StdLibExtras.h>

namespace WebCore {

using namespace HTMLNames;

static const int defaultRows = 2;
static const int defaultCols = 20;

static inline void notifyFormStateChanged(const HTMLTextAreaElement* element)
{
    Frame* frame = element->document()->frame();
    if (!frame)
        return;
    frame->page()->chrome()->client()->formStateDidChange(element);
}

HTMLTextAreaElement::HTMLTextAreaElement(const QualifiedName& tagName, Document* document, HTMLFormElement* form)
    : HTMLTextFormControlElement(tagName, document, form)
    , m_rows(defaultRows)
    , m_cols(defaultCols)
    , m_wrap(SoftWrap)
    , m_cachedSelectionStart(-1)
    , m_cachedSelectionEnd(-1)
    , m_isDirty(false)
{
    ASSERT(hasTagName(textareaTag));
    setFormControlValueMatchesRenderer(true);
}

PassRefPtr<HTMLTextAreaElement> HTMLTextAreaElement::create(const QualifiedName& tagName, Document* document, HTMLFormElement* form)
{
    return adoptRef(new HTMLTextAreaElement(tagName, document, form));
}

const AtomicString& HTMLTextAreaElement::formControlType() const
{
    DEFINE_STATIC_LOCAL(const AtomicString, textarea, ("textarea"));
    return textarea;
}

bool HTMLTextAreaElement::saveFormControlState(String& result) const
{
    String currentValue = value();
    if (currentValue == defaultValue())
        return false;
    result = currentValue;
    return true;
}

void HTMLTextAreaElement::restoreFormControlState(const String& state)
{
    setValue(state);
}

void HTMLTextAreaElement::childrenChanged(bool changedByParser, Node* beforeChange, Node* afterChange, int childCountDelta)
{
    if (!m_isDirty)
        setNonDirtyValue(defaultValue());
    HTMLElement::childrenChanged(changedByParser, beforeChange, afterChange, childCountDelta);
}
    
void HTMLTextAreaElement::parseMappedAttribute(Attribute* attr)
{
    if (attr->name() == rowsAttr) {
        int rows = attr->value().toInt();
        if (rows <= 0)
            rows = defaultRows;
        if (m_rows != rows) {
            m_rows = rows;
            if (renderer())
                renderer()->setNeedsLayoutAndPrefWidthsRecalc();
        }
    } else if (attr->name() == colsAttr) {
        int cols = attr->value().toInt();
        if (cols <= 0)
            cols = defaultCols;
        if (m_cols != cols) {
            m_cols = cols;
            if (renderer())
                renderer()->setNeedsLayoutAndPrefWidthsRecalc();
        }
    } else if (attr->name() == wrapAttr) {
        // The virtual/physical values were a Netscape extension of HTML 3.0, now deprecated.
        // The soft/hard /off values are a recommendation for HTML 4 extension by IE and NS 4.
        WrapMethod wrap;
        if (equalIgnoringCase(attr->value(), "physical") || equalIgnoringCase(attr->value(), "hard") || equalIgnoringCase(attr->value(), "on"))
            wrap = HardWrap;
        else if (equalIgnoringCase(attr->value(), "off"))
            wrap = NoWrap;
        else
            wrap = SoftWrap;
        if (wrap != m_wrap) {
            m_wrap = wrap;

            if (shouldWrapText()) {
                addCSSProperty(attr, CSSPropertyWhiteSpace, CSSValuePreWrap);
                addCSSProperty(attr, CSSPropertyWordWrap, CSSValueBreakWord);
            } else {
                addCSSProperty(attr, CSSPropertyWhiteSpace, CSSValuePre);
                addCSSProperty(attr, CSSPropertyWordWrap, CSSValueNormal);
            }

            if (renderer())
                renderer()->setNeedsLayoutAndPrefWidthsRecalc();
        }
    } else if (attr->name() == accesskeyAttr) {
        // ignore for the moment
    } else if (attr->name() == alignAttr) {
        // Don't map 'align' attribute.  This matches what Firefox, Opera and IE do.
        // See http://bugs.webkit.org/show_bug.cgi?id=7075
    } else if (attr->name() == maxlengthAttr)
        setNeedsValidityCheck();
    else
        HTMLTextFormControlElement::parseMappedAttribute(attr);
}

RenderObject* HTMLTextAreaElement::createRenderer(RenderArena* arena, RenderStyle*)
{
    return new (arena) RenderTextControlMultiLine(this, placeholderShouldBeVisible());
}

bool HTMLTextAreaElement::appendFormData(FormDataList& encoding, bool)
{
    if (name().isEmpty())
        return false;

    document()->updateLayout();

    // FIXME: It's not acceptable to ignore the HardWrap setting when there is no renderer.
    // While we have no evidence this has ever been a practical problem, it would be best to fix it some day.
    RenderTextControl* control = toRenderTextControl(renderer());
    const String& text = (m_wrap == HardWrap && control) ? control->textWithHardLineBreaks() : value();
    encoding.appendData(name(), text);
    return true;
}

void HTMLTextAreaElement::reset()
{
    setNonDirtyValue(defaultValue());
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
    ASSERT(renderer());
    ASSERT(!document()->childNeedsAndNotInStyleRecalc());

    if (!restorePreviousSelection || m_cachedSelectionStart < 0) {
#if ENABLE(ON_FIRST_TEXTAREA_FOCUS_SELECT_ALL)
        // Devices with trackballs or d-pads may focus on a textarea in route
        // to another focusable node. By selecting all text, the next movement
        // can more readily be interpreted as moving to the next node.
        select();
#else
        // If this is the first focus, set a caret at the beginning of the text.  
        // This matches some browsers' behavior; see bug 11746 Comment #15.
        // http://bugs.webkit.org/show_bug.cgi?id=11746#c15
        setSelectionRange(0, 0);
#endif
    } else {
        // Restore the cached selection.  This matches other browsers' behavior.
        setSelectionRange(m_cachedSelectionStart, m_cachedSelectionEnd);
    }

    if (document()->frame())
        document()->frame()->selection()->revealSelection();
}

void HTMLTextAreaElement::defaultEventHandler(Event* event)
{
    if (renderer() && (event->isMouseEvent() || event->isDragEvent() || event->isWheelEvent() || event->type() == eventNames().blurEvent))
        toRenderTextControlMultiLine(renderer())->forwardEvent(event);
    else if (renderer() && event->isBeforeTextInsertedEvent())
        handleBeforeTextInsertedEvent(static_cast<BeforeTextInsertedEvent*>(event));

    HTMLFormControlElementWithState::defaultEventHandler(event);
}

void HTMLTextAreaElement::handleBeforeTextInsertedEvent(BeforeTextInsertedEvent* event) const
{
    ASSERT(event);
    ASSERT(renderer());
    int signedMaxLength = maxLength();
    if (signedMaxLength < 0)
        return;
    unsigned unsignedMaxLength = static_cast<unsigned>(signedMaxLength);

    unsigned currentLength = numGraphemeClusters(toRenderTextControl(renderer())->text());
    // selectionLength represents the selection length of this text field to be
    // removed by this insertion.
    // If the text field has no focus, we don't need to take account of the
    // selection length. The selection is the source of text drag-and-drop in
    // that case, and nothing in the text field will be removed.
    unsigned selectionLength = focused() ? numGraphemeClusters(plainText(document()->frame()->selection()->selection().toNormalizedRange().get())) : 0;
    ASSERT(currentLength >= selectionLength);
    unsigned baseLength = currentLength - selectionLength;
    unsigned appendableLength = unsignedMaxLength > baseLength ? unsignedMaxLength - baseLength : 0;
    event->setText(sanitizeUserInputValue(event->text(), appendableLength));
}

String HTMLTextAreaElement::sanitizeUserInputValue(const String& proposedValue, unsigned maxLength)
{
    return proposedValue.left(numCharactersInGraphemeClusters(proposedValue, maxLength));
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
    m_value = toRenderTextControl(renderer())->text();
    const_cast<HTMLTextAreaElement*>(this)->setFormControlValueMatchesRenderer(true);
    notifyFormStateChanged(this);
    m_isDirty = true;
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
    setTextAsOfLastFormControlChangeEvent(value);
}

void HTMLTextAreaElement::setNonDirtyValue(const String& value)
{
    setValueCommon(value);
    m_isDirty = false;
    setNeedsValidityCheck();
    setTextAsOfLastFormControlChangeEvent(value);
}

void HTMLTextAreaElement::setValueCommon(const String& value)
{
    // Code elsewhere normalizes line endings added by the user via the keyboard or pasting.
    // We normalize line endings coming from JavaScript here.
    String normalizedValue = value.isNull() ? "" : value;
    normalizedValue.replace("\r\n", "\n");
    normalizedValue.replace('\r', '\n');

    // Return early because we don't want to move the caret or trigger other side effects
    // when the value isn't changing. This matches Firefox behavior, at least.
    if (normalizedValue == this->value())
        return;

    m_value = normalizedValue;
    updatePlaceholderVisibility(false);
    setNeedsStyleRecalc();
    setFormControlValueMatchesRenderer(true);

    // Set the caret to the end of the text value.
    if (document()->focusedNode() == this) {
        unsigned endOfString = m_value.length();
        setSelectionRange(endOfString, endOfString);
    }

    notifyFormStateChanged(this);
}

String HTMLTextAreaElement::defaultValue() const
{
    String value = "";

    // Since there may be comments, ignore nodes other than text nodes.
    for (Node* n = firstChild(); n; n = n->nextSibling()) {
        if (n->isTextNode())
            value += static_cast<Text*>(n)->data();
    }

    return value;
}

void HTMLTextAreaElement::setDefaultValue(const String& defaultValue)
{
    // To preserve comments, remove only the text nodes, then add a single text node.

    Vector<RefPtr<Node> > textNodes;
    for (Node* n = firstChild(); n; n = n->nextSibling()) {
        if (n->isTextNode())
            textNodes.append(n);
    }
    ExceptionCode ec;
    size_t size = textNodes.size();
    for (size_t i = 0; i < size; ++i)
        removeChild(textNodes[i].get(), ec);

    // Normalize line endings.
    String value = defaultValue;
    value.replace("\r\n", "\n");
    value.replace('\r', '\n');

    insertBefore(document()->createTextNode(value), firstChild(), ec);

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

bool HTMLTextAreaElement::tooLong(const String& value, NeedsToCheckDirtyFlag check) const
{
    // Return false for the default value even if it is longer than maxLength.
    if (check == CheckDirtyFlag && !m_isDirty)
        return false;

    int max = maxLength();
    if (max < 0)
        return false;
    return numGraphemeClusters(value) > static_cast<unsigned>(max);
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

bool HTMLTextAreaElement::lastChangeWasUserEdit() const
{
    if (!renderer())
        return false;
    return toRenderTextControl(renderer())->lastChangeWasUserEdit();
}

bool HTMLTextAreaElement::shouldUseInputMethod() const
{
    return true;
}

} // namespace
