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

#ifndef QWIDGETWINDOW_P_H
#define QWIDGETWINDOW_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/qwindow.h>

#include <QtCore/private/qobject_p.h>
#include <QtGui/private/qevent_p.h>

QT_BEGIN_NAMESPACE


class QCloseEvent;
class QMoveEvent;

class QWidgetWindow : public QWindow
{
    Q_OBJECT
public:
    QWidgetWindow(QWidget *widget);
    ~QWidgetWindow();

    QWidget *widget() const { return m_widget; }
#ifndef QT_NO_ACCESSIBILITY
    QAccessibleInterface *accessibleRoot() const;
#endif

    QObject *focusObject() const;
protected:
    bool event(QEvent *);

    void handleCloseEvent(QCloseEvent *);
    void handleEnterLeaveEvent(QEvent *);
    void handleFocusInEvent(QFocusEvent *);
    void handleKeyEvent(QKeyEvent *);
    void handleMouseEvent(QMouseEvent *);
    void handleNonClientAreaMouseEvent(QMouseEvent *);
    void handleTouchEvent(QTouchEvent *);
    void handleMoveEvent(QMoveEvent *);
    void handleResizeEvent(QResizeEvent *);
#ifndef QT_NO_WHEELEVENT
    void handleWheelEvent(QWheelEvent *);
#endif
#ifndef QT_NO_DRAGANDDROP
    void handleDragEnterMoveEvent(QDragMoveEvent *);
    void handleDragLeaveEvent(QDragLeaveEvent *);
    void handleDropEvent(QDropEvent *);
#endif
    void handleExposeEvent(QExposeEvent *);
    void handleWindowStateChangedEvent(QWindowStateChangeEvent *event);
    bool nativeEvent(const QByteArray &eventType, void *message, long *result);
#ifndef QT_NO_TABLETEVENT
    void handleTabletEvent(QTabletEvent *);
#endif
#ifndef QT_NO_GESTURES
    void handleGestureEvent(QNativeGestureEvent *);
#endif
#ifndef QT_NO_CONTEXTMENU
    void handleContextMenuEvent(QContextMenuEvent *);
#endif

private slots:
    void updateObjectName();
    void handleScreenChange();

private:
    void repaintWindow();
    bool updateSize();
    bool updatePos();
    void updateMargins();
    void updateNormalGeometry();

    enum FocusWidgets {
        FirstFocusWidget,
        LastFocusWidget
    };
    QWidget *getFocusWidget(FocusWidgets fw);

    QWidget *m_widget;
    QPointer<QWidget> m_implicit_mouse_grabber;
#ifndef QT_NO_DRAGANDDROP
    QPointer<QWidget> m_dragTarget;
#endif
};

QT_END_NAMESPACE

#endif // QWIDGETWINDOW_P_H
