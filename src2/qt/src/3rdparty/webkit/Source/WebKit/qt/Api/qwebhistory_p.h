/*
    Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef QWEBHISTORY_P_H
#define QWEBHISTORY_P_H

#include "BackForwardListImpl.h"
#include "HistoryItem.h"
#include <QtCore/qglobal.h>
#include <QtCore/qshareddata.h>

class QWebPagePrivate;

class Q_AUTOTEST_EXPORT QWebHistoryItemPrivate : public QSharedData {
public:
    static QExplicitlySharedDataPointer<QWebHistoryItemPrivate> get(QWebHistoryItem* q)
    {
        return q->d;
    }
    QWebHistoryItemPrivate(WebCore::HistoryItem* i)
    {
        if (i)
            i->ref();
        item = i;
    }
    ~QWebHistoryItemPrivate()
    {
        if (item)
            item->deref();
    }

    static WebCore::HistoryItem* core(const QWebHistoryItem* q);

    WebCore::HistoryItem* item;
};

class QWebHistoryPrivate : public QSharedData {
public:
    QWebHistoryPrivate(WebCore::BackForwardListImpl* l)
    {
        l->ref();
        lst = l;
    }
    ~QWebHistoryPrivate()
    {
        lst->deref();
    }

    QWebPagePrivate* page();

    WebCore::BackForwardListImpl* lst;
};


#endif
