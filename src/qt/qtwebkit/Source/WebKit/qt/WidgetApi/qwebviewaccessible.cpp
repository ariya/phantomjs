/*
 * Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.

 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "qwebviewaccessible_p.h"

#include "qwebframe.h"
#include "qwebframe_p.h"
#include "qwebpage.h"
#include "qwebview.h"

QWebFrameAccessible::QWebFrameAccessible(QWebFrame* frame)
    : QAccessibleObject(frame)
{
}

QWebFrame* QWebFrameAccessible::frame() const
{
    return qobject_cast<QWebFrame*>(object());
}

QAccessibleInterface* QWebFrameAccessible::parent() const
{
    return QAccessible::queryAccessibleInterface(object()->parent());
}

QString QWebFrameAccessible::text(QAccessible::Text) const
{
    return QString();
}

int QWebFrameAccessible::childCount() const
{
    return 0;
}

QAccessibleInterface* QWebFrameAccessible::child(int index) const
{
    return 0;
}

int QWebFrameAccessible::indexOfChild(const QAccessibleInterface*) const
{
    return 0;
}

QAccessible::State QWebFrameAccessible::state() const
{
    return QAccessible::State();
}

QAccessible::Role QWebFrameAccessible::role() const
{
    return QAccessible::Client;
}

int QWebFrameAccessible::navigate(QAccessible::RelationFlag, int, QAccessibleInterface** target) const
{
    *target = 0;
    return -1;
}

QWebPageAccessible::QWebPageAccessible(QWebPage* page)
    : QAccessibleObject(page)
{
}

QWebPage* QWebPageAccessible::page() const
{
    return qobject_cast<QWebPage*>(object());
}

QString QWebPageAccessible::text(QAccessible::Text t) const
{
    return QString();
}

QAccessibleInterface* QWebPageAccessible::parent() const
{
    return QAccessible::queryAccessibleInterface(object()->parent());
}

QAccessibleInterface* QWebPageAccessible::child(int index) const
{
    if (!index && page()->mainFrame())
        return new QWebFrameAccessible(page()->mainFrame());
    return 0;
}

int QWebPageAccessible::childCount() const
{
    return page()->mainFrame() ? 1 : 0;
}

int QWebPageAccessible::indexOfChild(const QAccessibleInterface*) const
{
    return 0;
}

int QWebPageAccessible::navigate(QAccessible::RelationFlag, int, QAccessibleInterface** target) const
{
    *target = 0;
    return -1;
}

QAccessible::Role QWebPageAccessible::role() const
{
    return QAccessible::Client;
}

QAccessible::State QWebPageAccessible::state() const
{
    return QAccessible::State();
}

QWebViewAccessible::QWebViewAccessible(QWebView* view)
    : QAccessibleWidget(view, QAccessible::Document)
{
}

QWebView* QWebViewAccessible::view() const
{
    return qobject_cast<QWebView*>(object());
}

int QWebViewAccessible::childCount() const
{
    return view()->page() ? 1 : 0;
}

QAccessibleInterface* QWebViewAccessible::child(int index) const
{
    if (!index && view()->page())
        return new QWebPageAccessible(view()->page());
    return 0;
}
