/*
    Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies)

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

#ifndef QWEBINSPECTOR_H
#define QWEBINSPECTOR_H

#include "qwebkitglobal.h"
#include "qwebpage.h"

#include "qwebview.h"

class QWebInspectorPrivate;

class QWEBKIT_EXPORT QWebInspector : public QWidget {
    Q_OBJECT
public:
    QWebInspector(QWidget* parent = 0);
    ~QWebInspector();

    void setPage(QWebPage* page);
    QWebPage* page() const;

    QSize sizeHint() const;
    bool event(QEvent*);

protected:
    void resizeEvent(QResizeEvent* event);
    void showEvent(QShowEvent* event);
    void hideEvent(QHideEvent* event);
    void closeEvent(QCloseEvent* event);

private:
    QWebInspectorPrivate* d;

    friend class QWebInspectorPrivate;
    friend class QWebPage;
    friend class QWebPagePrivate;
    friend class WebCore::InspectorClientQt;
    friend class WebCore::InspectorFrontendClientQt;
};
#endif
