/*
 * Copyright (C) 2006, 2007, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
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

#ifndef RenderSearchField_h
#define RenderSearchField_h

#include "PopupMenuClient.h"
#include "RenderTextControlSingleLine.h"

namespace WebCore {

class HTMLInputElement;
class SearchPopupMenu;

class RenderSearchField : public RenderTextControlSingleLine, private PopupMenuClient {
public:
    RenderSearchField(Element*);
    virtual ~RenderSearchField();

    void updateCancelButtonVisibility() const;

    void addSearchResult();
    void stopSearchEventTimer();

    bool popupIsVisible() const { return m_searchPopupIsVisible; }
    void showPopup();
    void hidePopup();

private:
    virtual void centerContainerIfNeeded(RenderBox*) const OVERRIDE;
    virtual LayoutUnit computeControlLogicalHeight(LayoutUnit lineHeight, LayoutUnit nonContentHeight) const OVERRIDE;
    virtual LayoutUnit computeLogicalHeightLimit() const OVERRIDE;
    virtual void updateFromElement() OVERRIDE;
    EVisibility visibilityForCancelButton() const;
    const AtomicString& autosaveName() const;

    // PopupMenuClient methods
    virtual void valueChanged(unsigned listIndex, bool fireEvents = true) OVERRIDE;
    virtual void selectionChanged(unsigned, bool) OVERRIDE { }
    virtual void selectionCleared() OVERRIDE { }
    virtual String itemText(unsigned listIndex) const OVERRIDE;
    virtual String itemLabel(unsigned listIndex) const OVERRIDE;
    virtual String itemIcon(unsigned listIndex) const OVERRIDE;
    virtual String itemToolTip(unsigned) const OVERRIDE { return String(); }
    virtual String itemAccessibilityText(unsigned) const OVERRIDE { return String(); }
    virtual bool itemIsEnabled(unsigned listIndex) const OVERRIDE;
    virtual PopupMenuStyle itemStyle(unsigned listIndex) const OVERRIDE;
    virtual PopupMenuStyle menuStyle() const OVERRIDE;
    virtual int clientInsetLeft() const OVERRIDE;
    virtual int clientInsetRight() const OVERRIDE;
    virtual LayoutUnit clientPaddingLeft() const OVERRIDE;
    virtual LayoutUnit clientPaddingRight() const OVERRIDE;
    virtual int listSize() const OVERRIDE;
    virtual int selectedIndex() const OVERRIDE;
    virtual void popupDidHide() OVERRIDE;
    virtual bool itemIsSeparator(unsigned listIndex) const OVERRIDE;
    virtual bool itemIsLabel(unsigned listIndex) const OVERRIDE;
    virtual bool itemIsSelected(unsigned listIndex) const OVERRIDE;
    virtual bool shouldPopOver() const OVERRIDE { return false; }
    virtual bool valueShouldChangeOnHotTrack() const OVERRIDE { return false; }
    virtual void setTextFromItem(unsigned listIndex) OVERRIDE;
    virtual FontSelector* fontSelector() const OVERRIDE;
    virtual HostWindow* hostWindow() const OVERRIDE;
    virtual PassRefPtr<Scrollbar> createScrollbar(ScrollableArea*, ScrollbarOrientation, ScrollbarControlSize) OVERRIDE;

    HTMLElement* resultsButtonElement() const;
    HTMLElement* cancelButtonElement() const;

    bool m_searchPopupIsVisible;
    RefPtr<SearchPopupMenu> m_searchPopup;
    Vector<String> m_recentSearches;
};

inline RenderSearchField* toRenderSearchField(RenderObject* object)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!object || object->isTextField());
    return static_cast<RenderSearchField*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderSearchField(const RenderSearchField*);

}

#endif
