/*
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
 */

#ifndef PopupMenuQt_h
#define PopupMenuQt_h

#include "PopupMenu.h"
#include <QObject>

class QWebSelectData;
class QWebSelectMethod;

namespace WebCore {

class ChromeClientQt;
class FrameView;
class PopupMenuClient;

class PopupMenuQt : public QObject, public PopupMenu {
    Q_OBJECT
public:
    PopupMenuQt(PopupMenuClient*, const ChromeClientQt*);
    ~PopupMenuQt();

    virtual void show(const IntRect&, FrameView*, int index);
    virtual void hide();
    virtual void updateFromElement();
    virtual void disconnectClient();

private slots:
    void didHide();
    void selectItem(int index, bool ctrl, bool shift);

private:
    PopupMenuClient* m_popupClient;
    QWebSelectMethod* m_popup;
    QWebSelectData* m_selectData;
    const ChromeClientQt* m_chromeClient;
};

}

#endif // PopupMenuQt_h
