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

#ifndef qwebviewaccessible_p_h
#define qwebviewaccessible_p_h

#include <qaccessible.h>
#include <qaccessibleobject.h>
#if QT_VERSION >= QT_VERSION_CHECK(5, 1, 0) && QT_VERSION < QT_VERSION_CHECK(5, 2, 0)
#include <private/qaccessiblewidget_p.h>
#else
#include <qaccessiblewidget.h>
#endif

class QWebFrame;
class QWebPage;
class QWebView;

/*
 * Classes representing accessible objects for View, Frame and Page.
 *
 * Each of these just returns one child which lets the accessibility
 * framwork navigate towards the actual contents in the frame.
 */

class QWebFrameAccessible : public QAccessibleObject {
public:
    QWebFrameAccessible(QWebFrame*);

    QWebFrame* frame() const;

    QAccessibleInterface* parent() const;
    int childCount() const;
    QAccessibleInterface* child(int index) const;
    int indexOfChild(const QAccessibleInterface*) const;
    int navigate(QAccessible::RelationFlag, int, QAccessibleInterface** target) const;

    QString text(QAccessible::Text) const;
    QAccessible::Role role() const;
    QAccessible::State state() const;
};

class QWebPageAccessible : public QAccessibleObject {
public:
    QWebPageAccessible(QWebPage*);

    QWebPage* page() const;

    QAccessibleInterface* parent() const;
    int childCount() const;
    QAccessibleInterface* child(int index) const;
    int indexOfChild(const QAccessibleInterface*) const;
    int navigate(QAccessible::RelationFlag, int, QAccessibleInterface** target) const;

    QString text(QAccessible::Text) const;
    QAccessible::Role role() const;
    QAccessible::State state() const;
};

class QWebViewAccessible : public QAccessibleWidget {
public:
    QWebViewAccessible(QWebView*);

    QWebView* view() const;

    int childCount() const;
    QAccessibleInterface* child(int index) const;
};

#endif
