/*
    Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)

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

#ifndef qwebloadrequest_p_h
#define qwebloadrequest_p_h

#include "qquickwebview_p.h"
#include <QtCore/QObject>
#include <QtCore/QScopedPointer>
#include <QtCore/QUrl>

class QWebLoadRequestPrivate;

class QWEBKIT_EXPORT QWebLoadRequest : public QObject {
    Q_OBJECT
    Q_PROPERTY(QUrl url READ url)
    Q_PROPERTY(QQuickWebView::LoadStatus status READ status)
    Q_PROPERTY(QString errorString READ errorString)
    Q_PROPERTY(QQuickWebView::ErrorDomain errorDomain READ errorDomain)
    Q_PROPERTY(int errorCode READ errorCode)

public:
    QWebLoadRequest(const QUrl& url, QQuickWebView::LoadStatus status, const QString& errorString = QString(), QQuickWebView::ErrorDomain errorDomain = QQuickWebView::NoErrorDomain, int errorCode = 0, QObject* parent = 0);
    ~QWebLoadRequest();
    QUrl url() const;
    QQuickWebView::LoadStatus status() const;
    QString errorString() const;
    QQuickWebView::ErrorDomain errorDomain() const;
    int errorCode() const;

private:
    QScopedPointer<QWebLoadRequestPrivate> d;
};

#endif // qwebloadrequest_p_h
