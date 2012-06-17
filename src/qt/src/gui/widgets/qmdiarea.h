/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QMDIAREA_H
#define QMDIAREA_H

#include <QtGui/qabstractscrollarea.h>
#include <QtGui/qtabwidget.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#ifndef QT_NO_MDIAREA

class QMdiSubWindow;

class QMdiAreaPrivate;
class Q_GUI_EXPORT QMdiArea : public QAbstractScrollArea
{
    Q_OBJECT
    Q_ENUMS(ViewMode)
    Q_PROPERTY(QBrush background READ background WRITE setBackground)
    Q_PROPERTY(WindowOrder activationOrder READ activationOrder WRITE setActivationOrder)
    Q_PROPERTY(ViewMode viewMode READ viewMode WRITE setViewMode)
#ifndef QT_NO_TABBAR
    Q_PROPERTY(bool documentMode READ documentMode WRITE setDocumentMode)
    Q_PROPERTY(bool tabsClosable READ tabsClosable WRITE setTabsClosable)
    Q_PROPERTY(bool tabsMovable READ tabsMovable WRITE setTabsMovable)
#endif
#ifndef QT_NO_TABWIDGET
    Q_PROPERTY(QTabWidget::TabShape tabShape READ tabShape WRITE setTabShape)
    Q_PROPERTY(QTabWidget::TabPosition tabPosition READ tabPosition WRITE setTabPosition)
#endif
    Q_ENUMS(WindowOrder)
public:
    enum AreaOption {
        DontMaximizeSubWindowOnActivation = 0x1
    };
    Q_DECLARE_FLAGS(AreaOptions, AreaOption)

    enum WindowOrder {
        CreationOrder,
        StackingOrder,
        ActivationHistoryOrder
    };

    enum ViewMode {
        SubWindowView,
        TabbedView
    };

    QMdiArea(QWidget *parent = 0);
    ~QMdiArea();

    QSize sizeHint() const;
    QSize minimumSizeHint() const;

    QMdiSubWindow *currentSubWindow() const;
    QMdiSubWindow *activeSubWindow() const;
    QList<QMdiSubWindow *> subWindowList(WindowOrder order = CreationOrder) const;

    QMdiSubWindow *addSubWindow(QWidget *widget, Qt::WindowFlags flags = 0);
    void removeSubWindow(QWidget *widget);

    QBrush background() const;
    void setBackground(const QBrush &background);

    WindowOrder activationOrder() const;
    void setActivationOrder(WindowOrder order);

    void setOption(AreaOption option, bool on = true);
    bool testOption(AreaOption opton) const;

    void setViewMode(ViewMode mode);
    ViewMode viewMode() const;

#ifndef QT_NO_TABBAR
    bool documentMode() const;
    void setDocumentMode(bool enabled);

    void setTabsClosable(bool closable);
    bool tabsClosable() const;

    void setTabsMovable(bool movable);
    bool tabsMovable() const;
#endif
#ifndef QT_NO_TABWIDGET
    void setTabShape(QTabWidget::TabShape shape);
    QTabWidget::TabShape tabShape() const;

    void setTabPosition(QTabWidget::TabPosition position);
    QTabWidget::TabPosition tabPosition() const;
#endif

Q_SIGNALS:
    void subWindowActivated(QMdiSubWindow *);

public Q_SLOTS:
    void setActiveSubWindow(QMdiSubWindow *window);
    void tileSubWindows();
    void cascadeSubWindows();
    void closeActiveSubWindow();
    void closeAllSubWindows();
    void activateNextSubWindow();
    void activatePreviousSubWindow();

protected Q_SLOTS:
    void setupViewport(QWidget *viewport);

protected:
    bool event(QEvent *event);
    bool eventFilter(QObject *object, QEvent *event);
    void paintEvent(QPaintEvent *paintEvent);
    void childEvent(QChildEvent *childEvent);
    void resizeEvent(QResizeEvent *resizeEvent);
    void timerEvent(QTimerEvent *timerEvent);
    void showEvent(QShowEvent *showEvent);
    bool viewportEvent(QEvent *event);
    void scrollContentsBy(int dx, int dy);

private:
    Q_DISABLE_COPY(QMdiArea)
    Q_DECLARE_PRIVATE(QMdiArea)
    Q_PRIVATE_SLOT(d_func(), void _q_deactivateAllWindows())
    Q_PRIVATE_SLOT(d_func(), void _q_processWindowStateChanged(Qt::WindowStates, Qt::WindowStates))
    Q_PRIVATE_SLOT(d_func(), void _q_currentTabChanged(int))
    Q_PRIVATE_SLOT(d_func(), void _q_closeTab(int))
    Q_PRIVATE_SLOT(d_func(), void _q_moveTab(int, int))
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QMdiArea::AreaOptions)

QT_END_NAMESPACE

QT_END_HEADER

#endif // QT_NO_MDIAREA
#endif // QMDIAREA_H
