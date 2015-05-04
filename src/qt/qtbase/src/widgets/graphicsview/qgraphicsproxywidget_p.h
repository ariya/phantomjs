/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtWidgets module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QGRAPHICSPROXYWIDGET_P_H
#define QGRAPHICSPROXYWIDGET_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qgraphicsproxywidget.h"
#include "private/qgraphicswidget_p.h"

#if !defined(QT_NO_GRAPHICSVIEW)

QT_BEGIN_NAMESPACE

class QGraphicsProxyWidgetPrivate : public QGraphicsWidgetPrivate
{
    Q_DECLARE_PUBLIC(QGraphicsProxyWidget)
public:
    QGraphicsProxyWidgetPrivate()
        : dragDropWidget(0),
          posChangeMode(NoMode),
          sizeChangeMode(NoMode),
          visibleChangeMode(NoMode),
          enabledChangeMode(NoMode),
          styleChangeMode(NoMode),
          paletteChangeMode(NoMode),
          tooltipChangeMode(NoMode),
          focusFromWidgetToProxy(0)
    { }
    void init();
    void sendWidgetMouseEvent(QGraphicsSceneMouseEvent *event);
    void sendWidgetMouseEvent(QGraphicsSceneHoverEvent *event);
    void sendWidgetKeyEvent(QKeyEvent *event);
    void setWidget_helper(QWidget *widget, bool autoShow);

    QWidget *findFocusChild(QWidget *child, bool next) const;
    void removeSubFocusHelper(QWidget *widget, Qt::FocusReason reason);

    void _q_removeWidgetSlot();

    void embedSubWindow(QWidget *);
    void unembedSubWindow(QWidget *);

    bool isProxyWidget() const;

    QPointer<QWidget> widget;
    QPointer<QWidget> lastWidgetUnderMouse;
    QPointer<QWidget> embeddedMouseGrabber;
    QWidget *dragDropWidget;
    Qt::DropAction lastDropAction;

    void updateWidgetGeometryFromProxy();
    void updateProxyGeometryFromWidget();

    void updateProxyInputMethodAcceptanceFromWidget();

    QPointF mapToReceiver(const QPointF &pos, const QWidget *receiver) const;

    enum ChangeMode {
        NoMode,
        ProxyToWidgetMode,
        WidgetToProxyMode
    };
    quint32 posChangeMode : 2;
    quint32 sizeChangeMode : 2;
    quint32 visibleChangeMode : 2;
    quint32 enabledChangeMode : 2;
    quint32 styleChangeMode : 2;
    quint32 paletteChangeMode : 2;
    quint32 tooltipChangeMode : 2;
    quint32 focusFromWidgetToProxy : 1;
    quint32 proxyIsGivingFocus : 1;
};

QT_END_NAMESPACE

#endif

#endif
