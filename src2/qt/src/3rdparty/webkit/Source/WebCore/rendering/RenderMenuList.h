/*
 * This file is part of the select element renderer in WebCore.
 *
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 * Copyright (C) 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
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

#ifndef RenderMenuList_h
#define RenderMenuList_h

#include "PopupMenu.h"
#include "PopupMenuClient.h"
#include "RenderFlexibleBox.h"

#if PLATFORM(MAC)
#define POPUP_MENU_PULLS_DOWN 0
#else
#define POPUP_MENU_PULLS_DOWN 1
#endif

namespace WebCore {

class RenderText;

#if ENABLE(NO_LISTBOX_RENDERING)
class RenderMenuList : public RenderFlexibleBox, private ListPopupMenuClient {
#else
class RenderMenuList : public RenderFlexibleBox, private PopupMenuClient {
#endif

public:
    RenderMenuList(Element*);
    virtual ~RenderMenuList();

public:
    bool popupIsVisible() const { return m_popupIsVisible; }
    void showPopup();
    void hidePopup();

    void setOptionsChanged(bool changed) { m_optionsChanged = changed; }

    void didSetSelectedIndex();

    String text() const;

private:
    virtual bool isMenuList() const { return true; }

    virtual void addChild(RenderObject* newChild, RenderObject* beforeChild = 0);
    virtual void removeChild(RenderObject*);
    virtual bool createsAnonymousWrapper() const { return true; }
    virtual bool canHaveChildren() const { return false; }

    virtual void updateFromElement();

    virtual bool hasControlClip() const { return true; }
    virtual IntRect controlClipRect(int tx, int ty) const;

    virtual const char* renderName() const { return "RenderMenuList"; }

    virtual void computePreferredLogicalWidths();

    virtual void styleDidChange(StyleDifference, const RenderStyle* oldStyle);

    virtual bool requiresForcedStyleRecalcPropagation() const { return true; }

    // PopupMenuClient methods
    virtual String itemText(unsigned listIndex) const;
    virtual String itemLabel(unsigned listIndex) const;
    virtual String itemIcon(unsigned listIndex) const;
    virtual String itemToolTip(unsigned listIndex) const;
    virtual String itemAccessibilityText(unsigned listIndex) const;
    virtual bool itemIsEnabled(unsigned listIndex) const;
    virtual PopupMenuStyle itemStyle(unsigned listIndex) const;
    virtual PopupMenuStyle menuStyle() const;
    virtual int clientInsetLeft() const;
    virtual int clientInsetRight() const;
    virtual int clientPaddingLeft() const;
    virtual int clientPaddingRight() const;
    virtual int listSize() const;
    virtual int selectedIndex() const;
    virtual void popupDidHide();
    virtual bool itemIsSeparator(unsigned listIndex) const;
    virtual bool itemIsLabel(unsigned listIndex) const;
    virtual bool itemIsSelected(unsigned listIndex) const;
    virtual void setTextFromItem(unsigned listIndex);
    virtual bool valueShouldChangeOnHotTrack() const { return true; }
    virtual bool shouldPopOver() const { return !POPUP_MENU_PULLS_DOWN; }
    virtual void valueChanged(unsigned listIndex, bool fireOnChange = true);
    virtual void selectionChanged(unsigned, bool) {}
    virtual void selectionCleared() {}
    virtual FontSelector* fontSelector() const;
    virtual HostWindow* hostWindow() const;
    virtual PassRefPtr<Scrollbar> createScrollbar(ScrollableArea*, ScrollbarOrientation, ScrollbarControlSize);

#if ENABLE(NO_LISTBOX_RENDERING)
    virtual void listBoxSelectItem(int listIndex, bool allowMultiplySelections, bool shift, bool fireOnChangeNow = true);
    virtual bool multiple();
#endif

    virtual bool hasLineIfEmpty() const { return true; }

    Color itemBackgroundColor(unsigned listIndex) const;

    void createInnerBlock();
    void adjustInnerStyle();
    void setText(const String&);
    void setTextFromOption(int optionIndex);
    void updateOptionsWidth();

    RenderText* m_buttonText;
    RenderBlock* m_innerBlock;

    bool m_optionsChanged;
    int m_optionsWidth;

    int m_lastSelectedIndex;

    RefPtr<RenderStyle> m_optionStyle;

    RefPtr<PopupMenu> m_popup;
    bool m_popupIsVisible;
};

inline RenderMenuList* toRenderMenuList(RenderObject* object)
{
    ASSERT(!object || object->isMenuList());
    return static_cast<RenderMenuList*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderMenuList(const RenderMenuList*);

}

#endif
