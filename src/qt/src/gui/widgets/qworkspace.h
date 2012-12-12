/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QWORKSPACE_H
#define QWORKSPACE_H

#include <QtGui/qwidget.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#ifndef QT_NO_WORKSPACE

class QAction;
class QWorkspaceChild;
class QShowEvent;
class QWorkspacePrivate;

class Q_GUI_EXPORT QWorkspace : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(bool scrollBarsEnabled READ scrollBarsEnabled WRITE setScrollBarsEnabled)
    Q_PROPERTY(QBrush background READ background WRITE setBackground)

public:
    explicit QWorkspace(QWidget* parent=0);
    ~QWorkspace();

    enum WindowOrder { CreationOrder, StackingOrder };

    QWidget* activeWindow() const;
    QWidgetList windowList(WindowOrder order = CreationOrder) const;

    QWidget * addWindow(QWidget *w, Qt::WindowFlags flags = 0);

    QSize sizeHint() const;

    bool scrollBarsEnabled() const;
    void setScrollBarsEnabled(bool enable);

#ifdef QT3_SUPPORT
    QT3_SUPPORT_CONSTRUCTOR QWorkspace(QWidget* parent, const char* name);
    QT3_SUPPORT void setPaletteBackgroundColor(const QColor &);
    QT3_SUPPORT void setPaletteBackgroundPixmap(const QPixmap &);
#endif

    void setBackground(const QBrush &background);
    QBrush background() const;

Q_SIGNALS:
    void windowActivated(QWidget* w);

public Q_SLOTS:
    void setActiveWindow(QWidget *w);
    void cascade();
    void tile();
    void arrangeIcons();
    void closeActiveWindow();
    void closeAllWindows();
    void activateNextWindow();
    void activatePreviousWindow();

protected:
    bool event(QEvent *e);
    void paintEvent(QPaintEvent *e);
    void changeEvent(QEvent *);
    void childEvent(QChildEvent *);
    void resizeEvent(QResizeEvent *);
    bool eventFilter(QObject *, QEvent *);
    void showEvent(QShowEvent *e);
    void hideEvent(QHideEvent *e);
#ifndef QT_NO_WHEELEVENT
    void wheelEvent(QWheelEvent *e);
#endif

private:
    Q_DECLARE_PRIVATE(QWorkspace)
    Q_DISABLE_COPY(QWorkspace)
    Q_PRIVATE_SLOT(d_func(), void _q_normalizeActiveWindow())
    Q_PRIVATE_SLOT(d_func(), void _q_minimizeActiveWindow())
    Q_PRIVATE_SLOT(d_func(), void _q_showOperationMenu())
    Q_PRIVATE_SLOT(d_func(), void _q_popupOperationMenu(const QPoint&))
    Q_PRIVATE_SLOT(d_func(), void _q_operationMenuActivated(QAction *))
    Q_PRIVATE_SLOT(d_func(), void _q_updateActions())
    Q_PRIVATE_SLOT(d_func(), void _q_scrollBarChanged())

    friend class QWorkspaceChild;
};

#endif // QT_NO_WORKSPACE

QT_END_NAMESPACE

QT_END_HEADER

#endif // QWORKSPACE_H
