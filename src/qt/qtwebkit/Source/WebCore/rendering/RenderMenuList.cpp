/*
 * This file is part of the select element renderer in WebCore.
 *
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 * Copyright (C) 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
 *               2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
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
#include "RenderMenuList.h"

#include "AXObjectCache.h"
#include "AccessibilityMenuList.h"
#include "CSSFontSelector.h"
#include "Chrome.h"
#include "FontCache.h"
#include "Frame.h"
#include "FrameView.h"
#include "HTMLNames.h"
#include "HTMLOptionElement.h"
#include "HTMLOptGroupElement.h"
#include "HTMLSelectElement.h"
#include "NodeRenderStyle.h"
#include "Page.h"
#include "PopupMenu.h"
#include "RenderBR.h"
#include "RenderScrollbar.h"
#include "RenderTheme.h"
#include "Settings.h"
#include "StyleResolver.h"
#include "TextRun.h"
#include <math.h>

using namespace std;

namespace WebCore {

using namespace HTMLNames;

RenderMenuList::RenderMenuList(Element* element)
    : RenderFlexibleBox(element)
    , m_buttonText(0)
    , m_innerBlock(0)
    , m_needsOptionsWidthUpdate(true)
    , m_optionsWidth(0)
    , m_lastActiveIndex(-1)
    , m_popupIsVisible(false)
{
    ASSERT(element);
    ASSERT(element->isHTMLElement());
    ASSERT(element->hasTagName(HTMLNames::selectTag));
}

RenderMenuList::~RenderMenuList()
{
    if (m_popup)
        m_popup->disconnectClient();
    m_popup = 0;
}

bool RenderMenuList::canBeReplacedWithInlineRunIn() const
{
    return false;
}

void RenderMenuList::createInnerBlock()
{
    if (m_innerBlock) {
        ASSERT(firstChild() == m_innerBlock);
        ASSERT(!m_innerBlock->nextSibling());
        return;
    }

    // Create an anonymous block.
    ASSERT(!firstChild());
    m_innerBlock = createAnonymousBlock();
    adjustInnerStyle();
    RenderFlexibleBox::addChild(m_innerBlock);
}

void RenderMenuList::adjustInnerStyle()
{
    RenderStyle* innerStyle = m_innerBlock->style();
    innerStyle->setFlexGrow(1);
    innerStyle->setFlexShrink(1);
    // min-width: 0; is needed for correct shrinking.
    // FIXME: Remove this line when https://bugs.webkit.org/show_bug.cgi?id=111790 is fixed.
    innerStyle->setMinWidth(Length(0, Fixed));
    // Use margin:auto instead of align-items:center to get safe centering, i.e.
    // when the content overflows, treat it the same as align-items: flex-start.
    // But we only do that for the cases where html.css would otherwise use center.
    if (style()->alignItems() == AlignCenter) {
        innerStyle->setMarginTop(Length());
        innerStyle->setMarginBottom(Length());
        innerStyle->setAlignSelf(AlignFlexStart);
    }

    innerStyle->setPaddingLeft(Length(theme()->popupInternalPaddingLeft(style()), Fixed));
    innerStyle->setPaddingRight(Length(theme()->popupInternalPaddingRight(style()), Fixed));
    innerStyle->setPaddingTop(Length(theme()->popupInternalPaddingTop(style()), Fixed));
    innerStyle->setPaddingBottom(Length(theme()->popupInternalPaddingBottom(style()), Fixed));

    if (document()->page()->chrome().selectItemWritingDirectionIsNatural()) {
        // Items in the popup will not respect the CSS text-align and direction properties,
        // so we must adjust our own style to match.
        innerStyle->setTextAlign(LEFT);
        TextDirection direction = (m_buttonText && m_buttonText->text()->defaultWritingDirection() == WTF::Unicode::RightToLeft) ? RTL : LTR;
        innerStyle->setDirection(direction);
    } else if (m_optionStyle && document()->page()->chrome().selectItemAlignmentFollowsMenuWritingDirection()) {
        if ((m_optionStyle->direction() != innerStyle->direction() || m_optionStyle->unicodeBidi() != innerStyle->unicodeBidi()))
            m_innerBlock->setNeedsLayoutAndPrefWidthsRecalc();
        innerStyle->setTextAlign(style()->isLeftToRightDirection() ? LEFT : RIGHT);
        innerStyle->setDirection(m_optionStyle->direction());
        innerStyle->setUnicodeBidi(m_optionStyle->unicodeBidi());
    }
}

inline HTMLSelectElement* RenderMenuList::selectElement() const
{
    return toHTMLSelectElement(node());
}

void RenderMenuList::addChild(RenderObject* newChild, RenderObject* beforeChild)
{
    createInnerBlock();
    m_innerBlock->addChild(newChild, beforeChild);
    ASSERT(m_innerBlock == firstChild());

    if (AXObjectCache* cache = document()->existingAXObjectCache())
        cache->childrenChanged(this);
}

void RenderMenuList::removeChild(RenderObject* oldChild)
{
    if (oldChild == m_innerBlock || !m_innerBlock) {
        RenderFlexibleBox::removeChild(oldChild);
        m_innerBlock = 0;
    } else
        m_innerBlock->removeChild(oldChild);
}

void RenderMenuList::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    RenderBlock::styleDidChange(diff, oldStyle);

    if (m_buttonText)
        m_buttonText->setStyle(style());
    if (m_innerBlock) // RenderBlock handled updating the anonymous block's style.
        adjustInnerStyle();

    bool fontChanged = !oldStyle || oldStyle->font() != style()->font();
    if (fontChanged) {
        updateOptionsWidth();
        m_needsOptionsWidthUpdate = false;
    }
}

void RenderMenuList::updateOptionsWidth()
{
    float maxOptionWidth = 0;
    const Vector<HTMLElement*>& listItems = selectElement()->listItems();
    int size = listItems.size();    
    FontCachePurgePreventer fontCachePurgePreventer;

    for (int i = 0; i < size; ++i) {
        HTMLElement* element = listItems[i];
        if (!isHTMLOptionElement(element))
            continue;

        String text = toHTMLOptionElement(element)->textIndentedToRespectGroupLabel();
        applyTextTransform(style(), text, ' ');
        if (theme()->popupOptionSupportsTextIndent()) {
            // Add in the option's text indent.  We can't calculate percentage values for now.
            float optionWidth = 0;
            if (RenderStyle* optionStyle = element->renderStyle())
                optionWidth += minimumValueForLength(optionStyle->textIndent(), 0, view());
            if (!text.isEmpty())
                optionWidth += style()->font().width(text);
            maxOptionWidth = max(maxOptionWidth, optionWidth);
        } else if (!text.isEmpty())
            maxOptionWidth = max(maxOptionWidth, style()->font().width(text));
    }

    int width = static_cast<int>(ceilf(maxOptionWidth));
    if (m_optionsWidth == width)
        return;

    m_optionsWidth = width;
    if (parent())
        setNeedsLayoutAndPrefWidthsRecalc();
}

void RenderMenuList::updateFromElement()
{
    if (m_needsOptionsWidthUpdate) {
        updateOptionsWidth();
        m_needsOptionsWidthUpdate = false;
    }

    if (m_popupIsVisible)
        m_popup->updateFromElement();
    else
        setTextFromOption(selectElement()->selectedIndex());
}

void RenderMenuList::setTextFromOption(int optionIndex)
{
    HTMLSelectElement* select = selectElement();
    const Vector<HTMLElement*>& listItems = select->listItems();
    int size = listItems.size();

    int i = select->optionToListIndex(optionIndex);
    String text = emptyString();
    if (i >= 0 && i < size) {
        Element* element = listItems[i];
        if (isHTMLOptionElement(element)) {
            text = toHTMLOptionElement(element)->textIndentedToRespectGroupLabel();
            m_optionStyle = element->renderStyle();
        }
    }

    setText(text.stripWhiteSpace());
    didUpdateActiveOption(optionIndex);
}

void RenderMenuList::setText(const String& s)
{
    if (s.isEmpty()) {
        if (!m_buttonText || !m_buttonText->isBR()) {
            if (m_buttonText)
                m_buttonText->destroy();
            m_buttonText = new (renderArena()) RenderBR(document());
            m_buttonText->setStyle(style());
            addChild(m_buttonText);
        }
    } else {
        if (m_buttonText && !m_buttonText->isBR())
            m_buttonText->setText(s.impl(), true);
        else {
            if (m_buttonText)
                m_buttonText->destroy();
            m_buttonText = new (renderArena()) RenderText(document(), s.impl());
            m_buttonText->setStyle(style());
            addChild(m_buttonText);
        }
        adjustInnerStyle();
    }
}

String RenderMenuList::text() const
{
    return m_buttonText ? m_buttonText->text() : 0;
}

LayoutRect RenderMenuList::controlClipRect(const LayoutPoint& additionalOffset) const
{
    // Clip to the intersection of the content box and the content box for the inner box
    // This will leave room for the arrows which sit in the inner box padding,
    // and if the inner box ever spills out of the outer box, that will get clipped too.
    LayoutRect outerBox(additionalOffset.x() + borderLeft() + paddingLeft(), 
                   additionalOffset.y() + borderTop() + paddingTop(),
                   contentWidth(), 
                   contentHeight());
    
    LayoutRect innerBox(additionalOffset.x() + m_innerBlock->x() + m_innerBlock->paddingLeft(), 
                   additionalOffset.y() + m_innerBlock->y() + m_innerBlock->paddingTop(),
                   m_innerBlock->contentWidth(), 
                   m_innerBlock->contentHeight());

    return intersection(outerBox, innerBox);
}

void RenderMenuList::computeIntrinsicLogicalWidths(LayoutUnit& minLogicalWidth, LayoutUnit& maxLogicalWidth) const
{
    maxLogicalWidth = max(m_optionsWidth, theme()->minimumMenuListSize(style())) + m_innerBlock->paddingLeft() + m_innerBlock->paddingRight();
    if (!style()->width().isPercent())
        minLogicalWidth = maxLogicalWidth;
}

void RenderMenuList::computePreferredLogicalWidths()
{
    m_minPreferredLogicalWidth = 0;
    m_maxPreferredLogicalWidth = 0;
    
    if (style()->width().isFixed() && style()->width().value() > 0)
        m_minPreferredLogicalWidth = m_maxPreferredLogicalWidth = adjustContentBoxLogicalWidthForBoxSizing(style()->width().value());
    else
        computeIntrinsicLogicalWidths(m_minPreferredLogicalWidth, m_maxPreferredLogicalWidth);

    if (style()->minWidth().isFixed() && style()->minWidth().value() > 0) {
        m_maxPreferredLogicalWidth = max(m_maxPreferredLogicalWidth, adjustContentBoxLogicalWidthForBoxSizing(style()->minWidth().value()));
        m_minPreferredLogicalWidth = max(m_minPreferredLogicalWidth, adjustContentBoxLogicalWidthForBoxSizing(style()->minWidth().value()));
    }

    if (style()->maxWidth().isFixed()) {
        m_maxPreferredLogicalWidth = min(m_maxPreferredLogicalWidth, adjustContentBoxLogicalWidthForBoxSizing(style()->maxWidth().value()));
        m_minPreferredLogicalWidth = min(m_minPreferredLogicalWidth, adjustContentBoxLogicalWidthForBoxSizing(style()->maxWidth().value()));
    }

    LayoutUnit toAdd = borderAndPaddingWidth();
    m_minPreferredLogicalWidth += toAdd;
    m_maxPreferredLogicalWidth += toAdd;

    setPreferredLogicalWidthsDirty(false);
}

void RenderMenuList::showPopup()
{
    if (m_popupIsVisible)
        return;

    if (document()->page()->chrome().hasOpenedPopup())
        return;

    // Create m_innerBlock here so it ends up as the first child.
    // This is important because otherwise we might try to create m_innerBlock
    // inside the showPopup call and it would fail.
    createInnerBlock();
    if (!m_popup)
        m_popup = document()->page()->chrome().createPopupMenu(this);
    m_popupIsVisible = true;

    // Compute the top left taking transforms into account, but use
    // the actual width of the element to size the popup.
    FloatPoint absTopLeft = localToAbsolute(FloatPoint(), UseTransforms);
    IntRect absBounds = absoluteBoundingBoxRectIgnoringTransforms();
    absBounds.setLocation(roundedIntPoint(absTopLeft));
    HTMLSelectElement* select = selectElement();
    m_popup->show(absBounds, document()->view(), select->optionToListIndex(select->selectedIndex()));
}

void RenderMenuList::hidePopup()
{
    if (m_popup)
        m_popup->hide();
}

void RenderMenuList::valueChanged(unsigned listIndex, bool fireOnChange)
{
    // Check to ensure a page navigation has not occurred while
    // the popup was up.
    Document* doc = toElement(node())->document();
    if (!doc || doc != doc->frame()->document())
        return;
    
    HTMLSelectElement* select = selectElement();
    select->optionSelectedByUser(select->listToOptionIndex(listIndex), fireOnChange);
}

void RenderMenuList::listBoxSelectItem(int listIndex, bool allowMultiplySelections, bool shift, bool fireOnChangeNow)
{
    selectElement()->listBoxSelectItem(listIndex, allowMultiplySelections, shift, fireOnChangeNow);
}

bool RenderMenuList::multiple() const
{
    return selectElement()->multiple();
}

void RenderMenuList::didSetSelectedIndex(int listIndex)
{
    didUpdateActiveOption(selectElement()->listToOptionIndex(listIndex));
}

void RenderMenuList::didUpdateActiveOption(int optionIndex)
{
    if (!AXObjectCache::accessibilityEnabled() || !document()->existingAXObjectCache())
        return;

    if (m_lastActiveIndex == optionIndex)
        return;
    m_lastActiveIndex = optionIndex;

    HTMLSelectElement* select = selectElement();
    int listIndex = select->optionToListIndex(optionIndex);
    if (listIndex < 0 || listIndex >= static_cast<int>(select->listItems().size()))
        return;

    ASSERT(select->listItems()[listIndex]);

    if (AccessibilityMenuList* menuList = static_cast<AccessibilityMenuList*>(document()->axObjectCache()->get(this)))
        menuList->didUpdateActiveOption(optionIndex);
}

String RenderMenuList::itemText(unsigned listIndex) const
{
    HTMLSelectElement* select = selectElement();
    const Vector<HTMLElement*>& listItems = select->listItems();
    if (listIndex >= listItems.size())
        return String();

    String itemString;
    Element* element = listItems[listIndex];
    if (isHTMLOptGroupElement(element))
        itemString = toHTMLOptGroupElement(element)->groupLabelText();
    else if (isHTMLOptionElement(element))
        itemString = toHTMLOptionElement(element)->textIndentedToRespectGroupLabel();

    applyTextTransform(style(), itemString, ' ');
    return itemString;
}

String RenderMenuList::itemLabel(unsigned) const
{
    return String();
}

String RenderMenuList::itemIcon(unsigned) const
{
    return String();
}

String RenderMenuList::itemAccessibilityText(unsigned listIndex) const
{
    // Allow the accessible name be changed if necessary.
    const Vector<HTMLElement*>& listItems = selectElement()->listItems();
    if (listIndex >= listItems.size())
        return String();
    return listItems[listIndex]->fastGetAttribute(aria_labelAttr);
}
    
String RenderMenuList::itemToolTip(unsigned listIndex) const
{
    const Vector<HTMLElement*>& listItems = selectElement()->listItems();
    if (listIndex >= listItems.size())
        return String();
    return listItems[listIndex]->title();
}

bool RenderMenuList::itemIsEnabled(unsigned listIndex) const
{
    const Vector<HTMLElement*>& listItems = selectElement()->listItems();
    if (listIndex >= listItems.size())
        return false;
    HTMLElement* element = listItems[listIndex];
    if (!isHTMLOptionElement(element))
        return false;

    bool groupEnabled = true;
    if (Element* parentElement = element->parentElement()) {
        if (isHTMLOptGroupElement(parentElement))
            groupEnabled = !parentElement->isDisabledFormControl();
    }
    if (!groupEnabled)
        return false;

    return !element->isDisabledFormControl();
}

PopupMenuStyle RenderMenuList::itemStyle(unsigned listIndex) const
{
    const Vector<HTMLElement*>& listItems = selectElement()->listItems();
    if (listIndex >= listItems.size()) {
        // If we are making an out of bounds access, then we want to use the style
        // of a different option element (index 0). However, if there isn't an option element
        // before at index 0, we fall back to the menu's style.
        if (!listIndex)
            return menuStyle();

        // Try to retrieve the style of an option element we know exists (index 0).
        listIndex = 0;
    }
    HTMLElement* element = listItems[listIndex];

    Color itemBackgroundColor;
    bool itemHasCustomBackgroundColor;
    getItemBackgroundColor(listIndex, itemBackgroundColor, itemHasCustomBackgroundColor);

    RenderStyle* style = element->renderStyle() ? element->renderStyle() : element->computedStyle();
    return style ? PopupMenuStyle(style->visitedDependentColor(CSSPropertyColor), itemBackgroundColor, style->font(), style->visibility() == VISIBLE,
        style->display() == NONE, style->textIndent(), style->direction(), isOverride(style->unicodeBidi()),
        itemHasCustomBackgroundColor ? PopupMenuStyle::CustomBackgroundColor : PopupMenuStyle::DefaultBackgroundColor) : menuStyle();
}

void RenderMenuList::getItemBackgroundColor(unsigned listIndex, Color& itemBackgroundColor, bool& itemHasCustomBackgroundColor) const
{
    const Vector<HTMLElement*>& listItems = selectElement()->listItems();
    if (listIndex >= listItems.size()) {
        itemBackgroundColor = style()->visitedDependentColor(CSSPropertyBackgroundColor);
        itemHasCustomBackgroundColor = false;
        return;
    }
    HTMLElement* element = listItems[listIndex];

    Color backgroundColor;
    if (element->renderStyle())
        backgroundColor = element->renderStyle()->visitedDependentColor(CSSPropertyBackgroundColor);
    itemHasCustomBackgroundColor = backgroundColor.isValid() && backgroundColor.alpha();
    // If the item has an opaque background color, return that.
    if (!backgroundColor.hasAlpha()) {
        itemBackgroundColor = backgroundColor;
        return;
    }

    // Otherwise, the item's background is overlayed on top of the menu background.
    backgroundColor = style()->visitedDependentColor(CSSPropertyBackgroundColor).blend(backgroundColor);
    if (!backgroundColor.hasAlpha()) {
        itemBackgroundColor = backgroundColor;
        return;
    }

    // If the menu background is not opaque, then add an opaque white background behind.
    itemBackgroundColor = Color(Color::white).blend(backgroundColor);
}

PopupMenuStyle RenderMenuList::menuStyle() const
{
    RenderStyle* s = m_innerBlock ? m_innerBlock->style() : style();
    return PopupMenuStyle(s->visitedDependentColor(CSSPropertyColor), s->visitedDependentColor(CSSPropertyBackgroundColor), s->font(), s->visibility() == VISIBLE,
        s->display() == NONE, s->textIndent(), style()->direction(), isOverride(style()->unicodeBidi()));
}

HostWindow* RenderMenuList::hostWindow() const
{
    return document()->view()->hostWindow();
}

PassRefPtr<Scrollbar> RenderMenuList::createScrollbar(ScrollableArea* scrollableArea, ScrollbarOrientation orientation, ScrollbarControlSize controlSize)
{
    RefPtr<Scrollbar> widget;
    bool hasCustomScrollbarStyle = style()->hasPseudoStyle(SCROLLBAR);
    if (hasCustomScrollbarStyle)
        widget = RenderScrollbar::createCustomScrollbar(scrollableArea, orientation, this->node());
    else
        widget = Scrollbar::createNativeScrollbar(scrollableArea, orientation, controlSize);
    return widget.release();
}

int RenderMenuList::clientInsetLeft() const
{
    return 0;
}

int RenderMenuList::clientInsetRight() const
{
    return 0;
}

LayoutUnit RenderMenuList::clientPaddingLeft() const
{
    return paddingLeft() + m_innerBlock->paddingLeft();
}

const int endOfLinePadding = 2;
LayoutUnit RenderMenuList::clientPaddingRight() const
{
    if (style()->appearance() == MenulistPart || style()->appearance() == MenulistButtonPart) {
        // For these appearance values, the theme applies padding to leave room for the
        // drop-down button. But leaving room for the button inside the popup menu itself
        // looks strange, so we return a small default padding to avoid having a large empty
        // space appear on the side of the popup menu.
        return endOfLinePadding;
    }

    // If the appearance isn't MenulistPart, then the select is styled (non-native), so
    // we want to return the user specified padding.
    return paddingRight() + m_innerBlock->paddingRight();
}

int RenderMenuList::listSize() const
{
    return selectElement()->listItems().size();
}

int RenderMenuList::selectedIndex() const
{
    HTMLSelectElement* select = selectElement();
    return select->optionToListIndex(select->selectedIndex());
}

void RenderMenuList::popupDidHide()
{
    m_popupIsVisible = false;
}

bool RenderMenuList::itemIsSeparator(unsigned listIndex) const
{
    const Vector<HTMLElement*>& listItems = selectElement()->listItems();
    return listIndex < listItems.size() && listItems[listIndex]->hasTagName(hrTag);
}

bool RenderMenuList::itemIsLabel(unsigned listIndex) const
{
    const Vector<HTMLElement*>& listItems = selectElement()->listItems();
    return listIndex < listItems.size() && isHTMLOptGroupElement(listItems[listIndex]);
}

bool RenderMenuList::itemIsSelected(unsigned listIndex) const
{
    const Vector<HTMLElement*>& listItems = selectElement()->listItems();
    if (listIndex >= listItems.size())
        return false;
    HTMLElement* element = listItems[listIndex];
    return isHTMLOptionElement(element) && toHTMLOptionElement(element)->selected();
}

void RenderMenuList::setTextFromItem(unsigned listIndex)
{
    setTextFromOption(selectElement()->listToOptionIndex(listIndex));
}

FontSelector* RenderMenuList::fontSelector() const
{
    return document()->ensureStyleResolver()->fontSelector();
}

}
