/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QMDISUBWINDOW_H
#define QMDISUBWINDOW_H

#include <QtGui/qwidget.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#ifndef QT_NO_MDIAREA

class QMenu;
class QMdiArea;

namespace QMdi { class ControlContainer; }
class QMdiSubWindowPrivate;
class Q_GUI_EXPORT QMdiSubWindow : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int keyboardSingleStep READ keyboardSingleStep WRITE setKeyboardSingleStep)
    Q_PROPERTY(int keyboardPageStep READ keyboardPageStep WRITE setKeyboardPageStep)
public:
    enum SubWindowOption {
        AllowOutsideAreaHorizontally = 0x1, // internal
        AllowOutsideAreaVertically = 0x2, // internal
        RubberBandResize = 0x4,
        RubberBandMove = 0x8
    };
    Q_DECLARE_FLAGS(SubWindowOptions, SubWindowOption)

    QMdiSubWindow(QWidget *parent = 0, Qt::WindowFlags flags = 0);
    ~QMdiSubWindow();

    QSize sizeHint() const;
    QSize minimumSizeHint() const;

    void setWidget(QWidget *widget);
    QWidget *widget() const;

    QWidget *maximizedButtonsWidget() const; // internal
    QWidget *maximizedSystemMenuIconWidget() const; // internal

    bool isShaded() const;

    void setOption(SubWindowOption option, bool on = true);
    bool testOption(SubWindowOption) const;

    void setKeyboardSingleStep(int step);
    int keyboardSingleStep() const;

    void setKeyboardPageStep(int step);
    int keyboardPageStep() const;

#ifndef QT_NO_MENU
    void setSystemMenu(QMenu *systemMenu);
    QMenu *systemMenu() const;
#endif

    QMdiArea *mdiArea() const;

Q_SIGNALS:
    void windowStateChanged(Qt::WindowStates oldState, Qt::WindowStates newState);
    void aboutToActivate();

public Q_SLOTS:
#ifndef QT_NO_MENU
    void showSystemMenu();
#endif
    void showShaded();

protected:
    bool eventFilter(QObject *object, QEvent *event);
    bool event(QEvent *event);
    void showEvent(QShowEvent *showEvent);
    void hideEvent(QHideEvent *hideEvent);
    void changeEvent(QEvent *changeEvent);
    void closeEvent(QCloseEvent *closeEvent);
    void leaveEvent(QEvent *leaveEvent);
    void resizeEvent(QResizeEvent *resizeEvent);
    void timerEvent(QTimerEvent *timerEvent);
    void moveEvent(QMoveEvent *moveEvent);
    void paintEvent(QPaintEvent *paintEvent);
    void mousePressEvent(QMouseEvent *mouseEvent);
    void mouseDoubleClickEvent(QMouseEvent *mouseEvent);
    void mouseReleaseEvent(QMouseEvent *mouseEvent);
    void mouseMoveEvent(QMouseEvent *mouseEvent);
    void keyPressEvent(QKeyEvent *keyEvent);
#ifndef QT_NO_CONTEXTMENU
    void contextMenuEvent(QContextMenuEvent *contextMenuEvent);
#endif
    void focusInEvent(QFocusEvent *focusInEvent);
    void focusOutEvent(QFocusEvent *focusOutEvent);
    void childEvent(QChildEvent *childEvent);

private:
    Q_DISABLE_COPY(QMdiSubWindow)
    Q_DECLARE_PRIVATE(QMdiSubWindow)
    Q_PRIVATE_SLOT(d_func(), void _q_updateStaysOnTopHint())
    Q_PRIVATE_SLOT(d_func(), void _q_enterInteractiveMode())
    Q_PRIVATE_SLOT(d_func(), void _q_processFocusChanged(QWidget *, QWidget *))
    friend class QMdiAreaPrivate;
#ifndef QT_NO_TABBAR
    friend class QMdiAreaTabBar;
#endif
    friend class QMdi::ControlContainer;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QMdiSubWindow::SubWindowOptions)

QT_END_NAMESPACE

QT_END_HEADER

#endif // QT_NO_MDIAREA

#endif // QMDISUBWINDOW_H
