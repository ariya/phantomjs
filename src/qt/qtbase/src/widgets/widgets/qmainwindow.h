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

#ifndef QDYNAMICMAINWINDOW_H
#define QDYNAMICMAINWINDOW_H

#include <QtWidgets/qwidget.h>
#include <QtWidgets/qtabwidget.h>

QT_BEGIN_NAMESPACE


#ifndef QT_NO_MAINWINDOW

class QDockWidget;
class QMainWindowPrivate;
class QMenuBar;
class QStatusBar;
class QToolBar;
class QMenu;

class Q_WIDGETS_EXPORT QMainWindow : public QWidget
{
    Q_OBJECT

    Q_ENUMS(DockOption)
    Q_FLAGS(DockOptions)
    Q_PROPERTY(QSize iconSize READ iconSize WRITE setIconSize)
    Q_PROPERTY(Qt::ToolButtonStyle toolButtonStyle READ toolButtonStyle WRITE setToolButtonStyle)
#ifndef QT_NO_DOCKWIDGET
    Q_PROPERTY(bool animated READ isAnimated WRITE setAnimated)
#ifndef QT_NO_TABBAR
    Q_PROPERTY(bool documentMode READ documentMode WRITE setDocumentMode)
#endif // QT_NO_TABBAR
#ifndef QT_NO_TABWIDGET
    Q_PROPERTY(QTabWidget::TabShape tabShape READ tabShape WRITE setTabShape)
#endif // QT_NO_TABWIDGET
    Q_PROPERTY(bool dockNestingEnabled READ isDockNestingEnabled WRITE setDockNestingEnabled)
#endif // QT_NO_DOCKWIDGET
    Q_PROPERTY(DockOptions dockOptions READ dockOptions WRITE setDockOptions)
#ifndef QT_NO_TOOLBAR
    Q_PROPERTY(bool unifiedTitleAndToolBarOnMac READ unifiedTitleAndToolBarOnMac WRITE setUnifiedTitleAndToolBarOnMac)
#endif

public:
    enum DockOption {
        AnimatedDocks = 0x01,
        AllowNestedDocks = 0x02,
        AllowTabbedDocks = 0x04,
        ForceTabbedDocks = 0x08,  // implies AllowTabbedDocks, !AllowNestedDocks
        VerticalTabs = 0x10       // implies AllowTabbedDocks
    };
    Q_DECLARE_FLAGS(DockOptions, DockOption)

    explicit QMainWindow(QWidget *parent = 0, Qt::WindowFlags flags = 0);
    ~QMainWindow();

    QSize iconSize() const;
    void setIconSize(const QSize &iconSize);

    Qt::ToolButtonStyle toolButtonStyle() const;
    void setToolButtonStyle(Qt::ToolButtonStyle toolButtonStyle);

#ifndef QT_NO_DOCKWIDGET
    bool isAnimated() const;
    bool isDockNestingEnabled() const;
#endif

#ifndef QT_NO_TABBAR
    bool documentMode() const;
    void setDocumentMode(bool enabled);
#endif

#ifndef QT_NO_TABWIDGET
    QTabWidget::TabShape tabShape() const;
    void setTabShape(QTabWidget::TabShape tabShape);
    QTabWidget::TabPosition tabPosition(Qt::DockWidgetArea area) const;
    void setTabPosition(Qt::DockWidgetAreas areas, QTabWidget::TabPosition tabPosition);
#endif // QT_NO_TABWIDGET

    void setDockOptions(DockOptions options);
    DockOptions dockOptions() const;

    bool isSeparator(const QPoint &pos) const;

#ifndef QT_NO_MENUBAR
    QMenuBar *menuBar() const;
    void setMenuBar(QMenuBar *menubar);

    QWidget  *menuWidget() const;
    void setMenuWidget(QWidget *menubar);
#endif

#ifndef QT_NO_STATUSBAR
    QStatusBar *statusBar() const;
    void setStatusBar(QStatusBar *statusbar);
#endif

    QWidget *centralWidget() const;
    void setCentralWidget(QWidget *widget);

    QWidget *takeCentralWidget();

#ifndef QT_NO_DOCKWIDGET
    void setCorner(Qt::Corner corner, Qt::DockWidgetArea area);
    Qt::DockWidgetArea corner(Qt::Corner corner) const;
#endif

#ifndef QT_NO_TOOLBAR
    void addToolBarBreak(Qt::ToolBarArea area = Qt::TopToolBarArea);
    void insertToolBarBreak(QToolBar *before);

    void addToolBar(Qt::ToolBarArea area, QToolBar *toolbar);
    void addToolBar(QToolBar *toolbar);
    QToolBar *addToolBar(const QString &title);
    void insertToolBar(QToolBar *before, QToolBar *toolbar);
    void removeToolBar(QToolBar *toolbar);
    void removeToolBarBreak(QToolBar *before);

    bool unifiedTitleAndToolBarOnMac() const;

    Qt::ToolBarArea toolBarArea(QToolBar *toolbar) const;
    bool toolBarBreak(QToolBar *toolbar) const;
#endif
#ifndef QT_NO_DOCKWIDGET
    void addDockWidget(Qt::DockWidgetArea area, QDockWidget *dockwidget);
    void addDockWidget(Qt::DockWidgetArea area, QDockWidget *dockwidget,
                       Qt::Orientation orientation);
    void splitDockWidget(QDockWidget *after, QDockWidget *dockwidget,
                         Qt::Orientation orientation);
    void tabifyDockWidget(QDockWidget *first, QDockWidget *second);
    QList<QDockWidget*> tabifiedDockWidgets(QDockWidget *dockwidget) const;
    void removeDockWidget(QDockWidget *dockwidget);
    bool restoreDockWidget(QDockWidget *dockwidget);

    Qt::DockWidgetArea dockWidgetArea(QDockWidget *dockwidget) const;
#endif // QT_NO_DOCKWIDGET

    QByteArray saveState(int version = 0) const;
    bool restoreState(const QByteArray &state, int version = 0);

#ifndef QT_NO_MENU
    virtual QMenu *createPopupMenu();
#endif

public Q_SLOTS:
#ifndef QT_NO_DOCKWIDGET
    void setAnimated(bool enabled);
    void setDockNestingEnabled(bool enabled);
#endif
#ifndef QT_NO_TOOLBAR
    void setUnifiedTitleAndToolBarOnMac(bool set);
#endif

Q_SIGNALS:
    void iconSizeChanged(const QSize &iconSize);
    void toolButtonStyleChanged(Qt::ToolButtonStyle toolButtonStyle);

protected:
#ifndef QT_NO_CONTEXTMENU
    void contextMenuEvent(QContextMenuEvent *event);
#endif
    bool event(QEvent *event);

private:
    Q_DECLARE_PRIVATE(QMainWindow)
    Q_DISABLE_COPY(QMainWindow)
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QMainWindow::DockOptions)

#endif // QT_NO_MAINWINDOW

QT_END_NAMESPACE

#endif // QDYNAMICMAINWINDOW_H
