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

#include "config.h"

#include "qwebnavigationrequest_p.h"

#include "qquickwebview_p.h"

class QWebNavigationRequestPrivate {
public:
    QWebNavigationRequestPrivate(const QUrl& url, Qt::MouseButton mouseButton, Qt::KeyboardModifiers keyboardModifiers, QQuickWebView::NavigationType navigationType)
        : url(url)
        , mouseButton(mouseButton)
        , keyboardModifiers(keyboardModifiers)
        , action(QQuickWebView::AcceptRequest)
        , navigationType(navigationType)
    {
    }

    ~QWebNavigationRequestPrivate()
    {
    }

    QUrl url;
    Qt::MouseButton mouseButton;
    Qt::KeyboardModifiers keyboardModifiers;
    QQuickWebView::NavigationRequestAction action;
    QQuickWebView::NavigationType navigationType;
};

QWebNavigationRequest::QWebNavigationRequest(const QUrl& url, Qt::MouseButton mouseButton, Qt::KeyboardModifiers keyboardModifiers, QQuickWebView::NavigationType navigationType, QObject* parent)
    : QObject(parent)
    , d(new QWebNavigationRequestPrivate(url, mouseButton, keyboardModifiers, navigationType))
{
}

QWebNavigationRequest::~QWebNavigationRequest()
{
    delete d;
}

void QWebNavigationRequest::setAction(QQuickWebView::NavigationRequestAction action)
{
    if (d->action == action)
        return;

    d->action = action;
    emit actionChanged();
}

QUrl QWebNavigationRequest::url() const
{
    return d->url;
}

int QWebNavigationRequest::mouseButton() const
{
    return int(d->mouseButton);
}

int QWebNavigationRequest::keyboardModifiers() const
{
    return int(d->keyboardModifiers);
}

QQuickWebView::NavigationRequestAction QWebNavigationRequest::action() const
{
    return d->action;
}

QQuickWebView::NavigationType QWebNavigationRequest::navigationType() const
{
    return d->navigationType;
}
