/*
 * This file is part of the popup menu implementation for <select> elements in WebCore.
 *
 * Copyright (C) 2008, 2009, 2010 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2006 Apple Computer, Inc.
 * Copyright (C) 2006 Michael Emmel mike.emmel@gmail.com
 * Coypright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
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
#include "PopupMenuQt.h"

#include "ChromeClientQt.h"
#include "FrameView.h"
#include "PopupMenuClient.h"
#include "QtFallbackWebPopup.h"

#include "qwebkitplatformplugin.h"

class SelectData : public QWebSelectData {
public:
    SelectData(WebCore::PopupMenuClient*& data) : d(data) {}

    virtual ItemType itemType(int) const;
    virtual QString itemText(int idx) const { return QString(d ? d->itemText(idx) : ""); }
    virtual QString itemToolTip(int idx) const { return QString(d ? d->itemToolTip(idx) : ""); }
    virtual bool itemIsEnabled(int idx) const { return d ? d->itemIsEnabled(idx) : false; }
    virtual int itemCount() const { return d ? d->listSize() : 0; }
    virtual bool itemIsSelected(int idx) const { return d ? d->itemIsSelected(idx) : false; }
    virtual bool multiple() const;
    virtual QColor backgroundColor() const { return d ? QColor(d->menuStyle().backgroundColor()) : QColor(); }
    virtual QColor foregroundColor() const { return d ? QColor(d->menuStyle().foregroundColor()) : QColor(); }
    virtual QColor itemBackgroundColor(int idx) const { return d ? QColor(d->itemStyle(idx).backgroundColor()) : QColor(); }
    virtual QColor itemForegroundColor(int idx) const { return d ? QColor(d->itemStyle(idx).foregroundColor()) : QColor(); }

private:
    WebCore::PopupMenuClient*& d;
};

bool SelectData::multiple() const
{
    if (!d)
        return false;

#if ENABLE(NO_LISTBOX_RENDERING)
    WebCore::ListPopupMenuClient* client = static_cast<WebCore::ListPopupMenuClient*>(d);
    return client && client->multiple();
#else
    return false;
#endif
}

SelectData::ItemType SelectData::itemType(int idx) const
{
    if (!d)
        return SelectData::Option;

    if (d->itemIsSeparator(idx))
        return SelectData::Separator;
    if (d->itemIsLabel(idx))
        return SelectData::Group;
    return SelectData::Option;
}

namespace WebCore {

PopupMenuQt::PopupMenuQt(PopupMenuClient* client, const ChromeClientQt* chromeClient)
    : m_popupClient(client)
    , m_popup(0)
    , m_selectData(0)
    , m_chromeClient(chromeClient)
{
}

PopupMenuQt::~PopupMenuQt()
{
    delete m_selectData;
    delete m_popup;
}

void PopupMenuQt::disconnectClient()
{
    m_popupClient = 0;
}

void PopupMenuQt::show(const IntRect& rect, FrameView* view, int index)
{
#ifndef QT_NO_COMBOBOX
    if (!m_popupClient)
        return;

    if (!m_popup) {
        m_popup = m_chromeClient->createSelectPopup();
        connect(m_popup, SIGNAL(didHide()), this, SLOT(didHide()));
        connect(m_popup, SIGNAL(selectItem(int, bool, bool)), this, SLOT(selectItem(int, bool, bool)));
    }

    if (QtFallbackWebPopup* fallback = qobject_cast<QtFallbackWebPopup*>(m_popup)) {
        QRect geometry(rect);
        geometry.moveTopLeft(view->contentsToWindow(rect.location()));
        fallback->setGeometry(geometry);
        fallback->setFont(m_popupClient->menuStyle().font().font());
    }

    if (m_selectData)
        delete m_selectData;
    m_selectData = new SelectData(m_popupClient);
    m_popup->show(*m_selectData);
#endif
}

void PopupMenuQt::didHide()
{
    if (m_popupClient)
        m_popupClient->popupDidHide();
}

void PopupMenuQt::hide()
{
    if (m_popup)
        m_popup->hide();
}

void PopupMenuQt::updateFromElement()
{
    if (m_popupClient)
        m_popupClient->setTextFromItem(m_popupClient->selectedIndex());
}

void PopupMenuQt::selectItem(int index, bool ctrl, bool shift)
{
    if (!m_popupClient)
        return;

#if ENABLE(NO_LISTBOX_RENDERING)
    ListPopupMenuClient* client = static_cast<ListPopupMenuClient*>(m_popupClient);
    if (client) {
        client->listBoxSelectItem(index, ctrl, shift);
        return;
    }
#endif

    m_popupClient->valueChanged(index);
}

}

#include "moc_PopupMenuQt.cpp"

// vim: ts=4 sw=4 et
