/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

//#define QNETWORKACCESSCACHEBACKEND_DEBUG

#include "qnetworkaccesscachebackend_p.h"
#include "qabstractnetworkcache.h"
#include "qfileinfo.h"
#include "qurlinfo.h"
#include "qdir.h"
#include "qcoreapplication.h"

QT_BEGIN_NAMESPACE

QNetworkAccessCacheBackend::QNetworkAccessCacheBackend()
    : QNetworkAccessBackend()
    , device(0)
{
}

QNetworkAccessCacheBackend::~QNetworkAccessCacheBackend()
{
}

void QNetworkAccessCacheBackend::open()
{
    if (operation() != QNetworkAccessManager::GetOperation || !sendCacheContents()) {
        QString msg = QCoreApplication::translate("QNetworkAccessCacheBackend", "Error opening %1")
                                                .arg(this->url().toString());
        error(QNetworkReply::ContentNotFoundError, msg);
    } else {
        setAttribute(QNetworkRequest::SourceIsFromCacheAttribute, true);
    }
    finished();
}

bool QNetworkAccessCacheBackend::sendCacheContents()
{
    setCachingEnabled(false);
    QAbstractNetworkCache *nc = networkCache();
    if (!nc)
        return false;

    QNetworkCacheMetaData item = nc->metaData(url());
    if (!item.isValid())
        return false;

    QNetworkCacheMetaData::AttributesMap attributes = item.attributes();
    setAttribute(QNetworkRequest::HttpStatusCodeAttribute, attributes.value(QNetworkRequest::HttpStatusCodeAttribute));
    setAttribute(QNetworkRequest::HttpReasonPhraseAttribute, attributes.value(QNetworkRequest::HttpReasonPhraseAttribute));

    // set the raw headers
    QNetworkCacheMetaData::RawHeaderList rawHeaders = item.rawHeaders();
    QNetworkCacheMetaData::RawHeaderList::ConstIterator it = rawHeaders.constBegin(),
                                                       end = rawHeaders.constEnd();
    for ( ; it != end; ++it) {
        if (it->first.toLower() == "cache-control" &&
            it->second.toLower().contains("must-revalidate")) {
            return false;
        }
        setRawHeader(it->first, it->second);
    }

    // handle a possible redirect
    QVariant redirectionTarget = attributes.value(QNetworkRequest::RedirectionTargetAttribute);
    if (redirectionTarget.isValid()) {
        setAttribute(QNetworkRequest::RedirectionTargetAttribute, redirectionTarget);
        redirectionRequested(redirectionTarget.toUrl());
    }

    // signal we're open
    metaDataChanged();

    if (operation() == QNetworkAccessManager::GetOperation) {
        QIODevice *contents = nc->data(url());
        if (!contents)
            return false;
        contents->setParent(this);
        writeDownstreamData(contents);
    }

#if defined(QNETWORKACCESSCACHEBACKEND_DEBUG)
    qDebug() << "Successfully sent cache:" << url();
#endif
    return true;
}

void QNetworkAccessCacheBackend::closeDownstreamChannel()
{
    if (operation() == QNetworkAccessManager::GetOperation) {
        device->close();
        delete device;
        device = 0;
    }
}

void QNetworkAccessCacheBackend::closeUpstreamChannel()
{
    Q_ASSERT_X(false, Q_FUNC_INFO, "This function show not have been called!");
}

void QNetworkAccessCacheBackend::upstreamReadyRead()
{
    Q_ASSERT_X(false, Q_FUNC_INFO, "This function show not have been called!");
}

void QNetworkAccessCacheBackend::downstreamReadyWrite()
{
    Q_ASSERT_X(false, Q_FUNC_INFO, "This function show not have been called!");
}

QT_END_NAMESPACE

