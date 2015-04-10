/*
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "qwebdownloaditem_p.h"

#include "DownloadProxy.h"
#include "qwebdownloaditem_p_p.h"

QWebDownloadItemPrivate::QWebDownloadItemPrivate(QWebDownloadItem* qq)
    : q(qq)
    , downloadProxy(0)
    , expectedContentLength(0)
    , totalBytesReceived(0)
{
}

QWebDownloadItem::QWebDownloadItem(QObject* parent)
    : QObject(parent)
    , d(new QWebDownloadItemPrivate(this))
{
}

QWebDownloadItem::~QWebDownloadItem()
{
    delete d;
}

QUrl QWebDownloadItem::url() const
{
    return d->sourceUrl;
}

QString QWebDownloadItem::destinationPath() const
{
    return d->destinationPath;
}

void QWebDownloadItem::setDestinationPath(const QString& destination)
{
    d->destinationPath = destination;
}

QString QWebDownloadItem::suggestedFilename() const
{
    return d->suggestedFilename;
}

QString QWebDownloadItem::mimeType() const
{
    return d->mimeType;
}

quint64 QWebDownloadItem::expectedContentLength() const
{
    return d->expectedContentLength;
}

quint64 QWebDownloadItem::totalBytesReceived() const
{
    return d->totalBytesReceived;
}

void QWebDownloadItem::cancel()
{
    ASSERT(d->downloadProxy);
    d->downloadProxy->cancel();
}

void QWebDownloadItem::start()
{
    ASSERT(!d->suggestedFilename.isEmpty());

    if (d->destinationPath.isEmpty())
        d->destinationPath = d->suggestedFilename;

    d->downloadProxy->startTransfer(d->destinationPath);
}

