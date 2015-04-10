/**
 * Copyright (C) 2006, 2007, 2010 Apple Inc. All rights reserved.
 *           (C) 2008 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/) 
 * Copyright (C) 2010 Google Inc. All rights reserved.
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
#include "RenderSearchField.h"

#include "CSSFontSelector.h"
#include "CSSValueKeywords.h"
#include "Chrome.h"
#include "Frame.h"
#include "FrameSelection.h"
#include "FrameView.h"
#include "HTMLInputElement.h"
#include "HTMLNames.h"
#include "HitTestResult.h"
#include "LocalizedStrings.h"
#include "Page.h"
#include "PlatformKeyboardEvent.h"
#include "RenderLayer.h"
#include "RenderScrollbar.h"
#include "RenderTheme.h"
#include "SearchPopupMenu.h"
#include "Settings.h"
#include "SimpleFontData.h"
#include "StyleResolver.h"
#include "TextControlInnerElements.h"

using namespace std;

namespace WebCore {

using namespace HTMLNames;

// ----------------------------

RenderSearchField::RenderSearchField(Element* element)
    : RenderTextControlSingleLine(element)
    , m_searchPopupIsVisible(false)
    , m_searchPopup(0)
{
    ASSERT(element->isHTMLElement());
    ASSERT(element->toInputElement());
    ASSERT(element->toInputElement()->isSearchField());
}

RenderSearchField::~RenderSearchField()
{
    if (m_searchPopup) {
        m_searchPopup->popupMenu()->disconnectClient();
        m_searchPopup = 0;
    }
}

inline HTMLElement* RenderSearchField::resultsButtonElement() const
{
    return inputElement()->resultsButtonElement();
}

inline HTMLElement* RenderSearchField::cancelButtonElement() const
{
    return inputElement()->cancelButtonElement();
}

void RenderSearchField::addSearchResult()
{
    HTMLInputElement* input = inputElement();
    if (input->maxResults() <= 0)
        return;

    String value = input->value();
    if (value.isEmpty())
        return;

    Settings* settings = document()->settings();
    if (!settings || settings->privateBrowsingEnabled())
        return;

    int size = static_cast<int>(m_recentSearches.size());
    for (int i = size - 1; i >= 0; --i) {
        if (m_recentSearches[i] == value)
            m_recentSearches.remove(i);
    }

    m_recentSearches.insert(0, value);
    while (static_cast<int>(m_recentSearches.size()) > input->maxResults())
        m_recentSearches.removeLast();

    const AtomicString& name = autosaveName();
    if (!m_searchPopup)
        m_searchPopup = document()->page()->chrome().createSearchPopupMenu(this);

    m_searchPopup->saveRecentSearches(name, m_recentSearches);
}

void RenderSearchField::showPopup()
{
    if (m_searchPopupIsVisible)
        return;

    if (!m_searchPopup)
        m_searchPopup = document()->page()->chrome().createSearchPopupMenu(this);

    if (!m_searchPopup->enabled())
        return;

    m_searchPopupIsVisible = true;

    const AtomicString& name = autosaveName();
    m_searchPopup->loadRecentSearches(name, m_recentSearches);

    // Trim the recent searches list if the maximum size has changed since we last saved.
    HTMLInputElement* input = inputElement();
    if (static_cast<int>(m_recentSearches.size()) > input->maxResults()) {
        do {
            m_recentSearches.removeLast();
        } while (static_cast<int>(m_recentSearches.size()) > input->maxResults());

        m_searchPopup->saveRecentSearches(name, m_recentSearches);
    }

    m_searchPopup->popupMenu()->show(pixelSnappedIntRect(absoluteBoundingBoxRect()), document()->view(), -1);
}

void RenderSearchField::hidePopup()
{
    if (m_searchPopup)
        m_searchPopup->popupMenu()->hide();
}

LayoutUnit RenderSearchField::computeControlLogicalHeight(LayoutUnit lineHeight, LayoutUnit nonContentHeight) const
{
    HTMLElement* resultsButton = resultsButtonElement();
    if (RenderBox* resultsRenderer = resultsButton ? resultsButton->renderBox() : 0) {
        resultsRenderer->updateLogicalHeight();
        nonContentHeight = max(nonContentHeight, resultsRenderer->borderAndPaddingLogicalHeight() + resultsRenderer->marginLogicalHeight());
        lineHeight = max(lineHeight, resultsRenderer->logicalHeight());
    }
    HTMLElement* cancelButton = cancelButtonElement();
    if (RenderBox* cancelRenderer = cancelButton ? cancelButton->renderBox() : 0) {
        cancelRenderer->updateLogicalHeight();
        nonContentHeight = max(nonContentHeight, cancelRenderer->borderAndPaddingLogicalHeight() + cancelRenderer->marginLogicalHeight());
        lineHeight = max(lineHeight, cancelRenderer->logicalHeight());
    }

    return lineHeight + nonContentHeight;
}

void RenderSearchField::updateFromElement()
{
    RenderTextControlSingleLine::updateFromElement();

    if (cancelButtonElement())
        updateCancelButtonVisibility();

    if (m_searchPopupIsVisible)
        m_searchPopup->popupMenu()->updateFromElement();
}

void RenderSearchField::updateCancelButtonVisibility() const
{
    RenderObject* cancelButtonRenderer = cancelButtonElement()->renderer();
    if (!cancelButtonRenderer)
        return;

    const RenderStyle* curStyle = cancelButtonRenderer->style();
    EVisibility buttonVisibility = visibilityForCancelButton();
    if (curStyle->visibility() == buttonVisibility)
        return;

    RefPtr<RenderStyle> cancelButtonStyle = RenderStyle::clone(curStyle);
    cancelButtonStyle->setVisibility(buttonVisibility);
    cancelButtonRenderer->setStyle(cancelButtonStyle);
}

EVisibility RenderSearchField::visibilityForCancelButton() const
{
    return (style()->visibility() == HIDDEN || inputElement()->value().isEmpty()) ? HIDDEN : VISIBLE;
}

const AtomicString& RenderSearchField::autosaveName() const
{
    return toElement(node())->getAttribute(autosaveAttr);
}

// PopupMenuClient methods
void RenderSearchField::valueChanged(unsigned listIndex, bool fireEvents)
{
    ASSERT(static_cast<int>(listIndex) < listSize());
    HTMLInputElement* input = inputElement();
    if (static_cast<int>(listIndex) == (listSize() - 1)) {
        if (fireEvents) {
            m_recentSearches.clear();
            const AtomicString& name = autosaveName();
            if (!name.isEmpty()) {
                if (!m_searchPopup)
                    m_searchPopup = document()->page()->chrome().createSearchPopupMenu(this);
                m_searchPopup->saveRecentSearches(name, m_recentSearches);
            }
        }
    } else {
        input->setValue(itemText(listIndex));
        if (fireEvents)
            input->onSearch();
        input->select();
    }
}

String RenderSearchField::itemText(unsigned listIndex) const
{
    int size = listSize();
    if (size == 1) {
        ASSERT(!listIndex);
        return searchMenuNoRecentSearchesText();
    }
    if (!listIndex)
        return searchMenuRecentSearchesText();
    if (itemIsSeparator(listIndex))
        return String();
    if (static_cast<int>(listIndex) == (size - 1))
        return searchMenuClearRecentSearchesText();
    return m_recentSearches[listIndex - 1];
}

String RenderSearchField::itemLabel(unsigned) const
{
    return String();
}

String RenderSearchField::itemIcon(unsigned) const
{
    return String();
}

bool RenderSearchField::itemIsEnabled(unsigned listIndex) const
{
     if (!listIndex || itemIsSeparator(listIndex))
        return false;
    return true;
}

PopupMenuStyle RenderSearchField::itemStyle(unsigned) const
{
    return menuStyle();
}

PopupMenuStyle RenderSearchField::menuStyle() const
{
    return PopupMenuStyle(style()->visitedDependentColor(CSSPropertyColor), style()->visitedDependentColor(CSSPropertyBackgroundColor), style()->font(), style()->visibility() == VISIBLE,
        style()->display() == NONE, style()->textIndent(), style()->direction(), isOverride(style()->unicodeBidi()), PopupMenuStyle::CustomBackgroundColor);
}

int RenderSearchField::clientInsetLeft() const
{
    // Inset the menu by the radius of the cap on the left so that
    // it only runs along the straight part of the bezel.
    return height() / 2;
}

int RenderSearchField::clientInsetRight() const
{
    // Inset the menu by the radius of the cap on the right so that
    // it only runs along the straight part of the bezel (unless it needs
    // to be wider).
    return height() / 2;
}

LayoutUnit RenderSearchField::clientPaddingLeft() const
{
    LayoutUnit padding = paddingLeft();
    if (RenderBox* box = innerBlockElement() ? innerBlockElement()->renderBox() : 0)
        padding += box->x();
    return padding;
}

LayoutUnit RenderSearchField::clientPaddingRight() const
{
    LayoutUnit padding = paddingRight();
    if (RenderBox* containerBox = containerElement() ? containerElement()->renderBox() : 0) {
        if (RenderBox* innerBlockBox = innerBlockElement() ? innerBlockElement()->renderBox() : 0)
            padding += containerBox->width() - (innerBlockBox->x() + innerBlockBox->width());
    }
    return padding;
}

int RenderSearchField::listSize() const
{
    // If there are no recent searches, then our menu will have 1 "No recent searches" item.
    if (!m_recentSearches.size())
        return 1;
    // Otherwise, leave room in the menu for a header, a separator, and the "Clear recent searches" item.
    return m_recentSearches.size() + 3;
}

int RenderSearchField::selectedIndex() const
{
    return -1;
}

void RenderSearchField::popupDidHide()
{
    m_searchPopupIsVisible = false;
}

bool RenderSearchField::itemIsSeparator(unsigned listIndex) const
{
    // The separator will be the second to last item in our list.
    return static_cast<int>(listIndex) == (listSize() - 2);
}

bool RenderSearchField::itemIsLabel(unsigned listIndex) const
{
    return !listIndex;
}

bool RenderSearchField::itemIsSelected(unsigned) const
{
    return false;
}

void RenderSearchField::setTextFromItem(unsigned listIndex)
{
    inputElement()->setValue(itemText(listIndex));
}

FontSelector* RenderSearchField::fontSelector() const
{
    return document()->ensureStyleResolver()->fontSelector();
}

HostWindow* RenderSearchField::hostWindow() const
{
    return document()->view()->hostWindow();
}

PassRefPtr<Scrollbar> RenderSearchField::createScrollbar(ScrollableArea* scrollableArea, ScrollbarOrientation orientation, ScrollbarControlSize controlSize)
{
    RefPtr<Scrollbar> widget;
    bool hasCustomScrollbarStyle = style()->hasPseudoStyle(SCROLLBAR);
    if (hasCustomScrollbarStyle)
        widget = RenderScrollbar::createCustomScrollbar(scrollableArea, orientation, this->node());
    else
        widget = Scrollbar::createNativeScrollbar(scrollableArea, orientation, controlSize);
    return widget.release();
}

LayoutUnit RenderSearchField::computeLogicalHeightLimit() const
{
    return logicalHeight();
}

void RenderSearchField::centerContainerIfNeeded(RenderBox* containerRenderer) const
{
    if (!containerRenderer)
        return;

    if (containerRenderer->logicalHeight() <= contentLogicalHeight())
        return;

    // A quirk for find-in-page box on Safari Windows.
    // http://webkit.org/b/63157
    LayoutUnit logicalHeightDiff = containerRenderer->logicalHeight() - contentLogicalHeight();
    containerRenderer->setLogicalTop(containerRenderer->logicalTop() - (logicalHeightDiff / 2 + layoutMod(logicalHeightDiff, 2)));
}

}
