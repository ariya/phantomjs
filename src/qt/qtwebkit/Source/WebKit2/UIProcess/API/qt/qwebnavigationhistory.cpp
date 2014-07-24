/*
 * Copyright (C) 2010 Juha Savolainen (juha.savolainen@weego.fi)
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *  notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "config.h"
#include "qwebnavigationhistory_p.h"

#include "WKBackForwardList.h"
#include "WKStringQt.h"
#include "WKURL.h"
#include "WKURLQt.h"

#include "qwebnavigationhistory_p_p.h"
#include <QString>
#include <QUrl>
#include <QtQml/QQmlEngine>
#include <WebKit2/WKArray.h>
#include <WebKit2/WKBackForwardListItem.h>
#include <WebKit2/WKBase.h>
#include <WebKit2/WKPage.h>
#include <WebKit2/WKRetainPtr.h>
#include <wtf/PassOwnPtr.h>

using namespace WebKit;

QWebNavigationListModelPrivate::QWebNavigationListModelPrivate(WKBackForwardListRef list)
    : m_backForwardList(list)
    , indexSign(0)
{
}

QWebNavigationListModel* QWebNavigationListModelPrivate::createWebNavigationModel(WKBackForwardListRef list)
{
    QWebNavigationListModel* model = new QWebNavigationListModel();
    model->d = new QWebNavigationListModelPrivate(list);
    return model;
}


QWebNavigationHistoryPrivate::QWebNavigationHistoryPrivate(WKPageRef page)
    : m_page(page)
    , m_backForwardList(WKPageGetBackForwardList(page))
    , m_backNavigationModel(adoptPtr(QWebNavigationListModelPrivate::createWebNavigationModel(m_backForwardList.get())))
    , m_forwardNavigationModel(adoptPtr(QWebNavigationListModelPrivate::createWebNavigationModel(m_backForwardList.get())))
{
    m_backNavigationModel->d->count = &WKBackForwardListGetBackListCount;
    m_backNavigationModel->d->indexSign = -1;
    m_forwardNavigationModel->d->count = &WKBackForwardListGetForwardListCount;
    m_forwardNavigationModel->d->indexSign = 1;
}

QWebNavigationHistory* QWebNavigationHistoryPrivate::createHistory(WKPageRef page)
{
    QWebNavigationHistory* history = new QWebNavigationHistory();
    history->d = new QWebNavigationHistoryPrivate(page);
    return history;
}

void QWebNavigationHistoryPrivate::reset()
{
    m_backNavigationModel->reset();
    m_forwardNavigationModel->reset();
}

void QWebNavigationHistoryPrivate::goBackTo(int index)
{
    WKRetainPtr<WKBackForwardListItemRef> itemRef = WKBackForwardListGetItemAtIndex(m_backForwardList.get(), -(index + 1));
    if (itemRef && m_page)
        WKPageGoToBackForwardListItem(m_page.get(), itemRef.get());
}

void QWebNavigationHistoryPrivate::goForwardTo(int index)
{
    WKRetainPtr<WKBackForwardListItemRef> itemRef = WKBackForwardListGetItemAtIndex(m_backForwardList.get(), index + 1);
    if (itemRef && m_page)
        WKPageGoToBackForwardListItem(m_page.get(), itemRef.get());
}

QHash<int, QByteArray> QWebNavigationListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[QWebNavigationHistory::UrlRole] = "url";
    roles[QWebNavigationHistory::TitleRole] = "title";
    return roles;
}

QWebNavigationListModel::~QWebNavigationListModel()
{
    delete d;
}

int QWebNavigationListModel::rowCount(const QModelIndex&) const
{
    return d->count(d->m_backForwardList.get());
}

QVariant QWebNavigationListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role < QWebNavigationHistory::UrlRole || role > QWebNavigationHistory::TitleRole)
        return QVariant();

    WKRetainPtr<WKBackForwardListItemRef> itemRef = WKBackForwardListGetItemAtIndex(d->m_backForwardList.get(), (index.row() + 1) * d->indexSign);
    if (role == QWebNavigationHistory::UrlRole) {
        WKRetainPtr<WKURLRef> url(AdoptWK, WKBackForwardListItemCopyURL(itemRef.get()));
        return WKURLCopyQUrl(url.get());
    }

    if (role == QWebNavigationHistory::TitleRole) {
        WKRetainPtr<WKStringRef> title(AdoptWK, WKBackForwardListItemCopyTitle(itemRef.get()));
        return WKStringCopyQString(title.get());
    }

    return QVariant();
}

void QWebNavigationListModel::reset()
{
    beginResetModel();
    endResetModel();
}

QWebNavigationHistory::QWebNavigationHistory()
    : QObject()
{
}

QWebNavigationHistory::~QWebNavigationHistory()
{
    delete d;
}

QWebNavigationListModel* QWebNavigationHistory::backItems() const
{
    return d->m_backNavigationModel.get();
}

QWebNavigationListModel* QWebNavigationHistory::forwardItems() const
{
    return d->m_forwardNavigationModel.get();
}

#include "moc_qwebnavigationhistory_p.cpp"
