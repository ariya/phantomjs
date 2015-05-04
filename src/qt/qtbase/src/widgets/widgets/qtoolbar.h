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

#ifndef QDYNAMICTOOLBAR_H
#define QDYNAMICTOOLBAR_H

#include <QtWidgets/qwidget.h>

QT_BEGIN_NAMESPACE


#ifndef QT_NO_TOOLBAR

class QToolBarPrivate;

class QAction;
class QIcon;
class QMainWindow;
class QStyleOptionToolBar;

class Q_WIDGETS_EXPORT QToolBar : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(bool movable READ isMovable WRITE setMovable
               DESIGNABLE (qobject_cast<QMainWindow *>(parentWidget()) != 0)
               NOTIFY movableChanged)
    Q_PROPERTY(Qt::ToolBarAreas allowedAreas READ allowedAreas WRITE setAllowedAreas
               DESIGNABLE (qobject_cast<QMainWindow *>(parentWidget()) != 0)
               NOTIFY allowedAreasChanged)
    Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation
               DESIGNABLE (qobject_cast<QMainWindow *>(parentWidget()) == 0)
               NOTIFY orientationChanged)
    Q_PROPERTY(QSize iconSize READ iconSize WRITE setIconSize NOTIFY iconSizeChanged)
    Q_PROPERTY(Qt::ToolButtonStyle toolButtonStyle READ toolButtonStyle WRITE setToolButtonStyle
               NOTIFY toolButtonStyleChanged)
    Q_PROPERTY(bool floating READ isFloating)
    Q_PROPERTY(bool floatable READ isFloatable WRITE setFloatable)

public:
    explicit QToolBar(const QString &title, QWidget *parent = 0);
    explicit QToolBar(QWidget *parent = 0);
    ~QToolBar();

    void setMovable(bool movable);
    bool isMovable() const;

    void setAllowedAreas(Qt::ToolBarAreas areas);
    Qt::ToolBarAreas allowedAreas() const;

    inline bool isAreaAllowed(Qt::ToolBarArea area) const
    { return (allowedAreas() & area) == area; }

    void setOrientation(Qt::Orientation orientation);
    Qt::Orientation orientation() const;

    void clear();

#ifdef Q_NO_USING_KEYWORD
    inline void addAction(QAction *action)
    { QWidget::addAction(action); }
#else
    using QWidget::addAction;
#endif

    QAction *addAction(const QString &text);
    QAction *addAction(const QIcon &icon, const QString &text);
    QAction *addAction(const QString &text, const QObject *receiver, const char* member);
    QAction *addAction(const QIcon &icon, const QString &text,
                       const QObject *receiver, const char* member);

    QAction *addSeparator();
    QAction *insertSeparator(QAction *before);

    QAction *addWidget(QWidget *widget);
    QAction *insertWidget(QAction *before, QWidget *widget);

    QRect actionGeometry(QAction *action) const;
    QAction *actionAt(const QPoint &p) const;
    inline QAction *actionAt(int x, int y) const;

    QAction *toggleViewAction() const;

    QSize iconSize() const;
    Qt::ToolButtonStyle toolButtonStyle() const;

    QWidget *widgetForAction(QAction *action) const;

    bool isFloatable() const;
    void setFloatable(bool floatable);
    bool isFloating() const;

public Q_SLOTS:
    void setIconSize(const QSize &iconSize);
    void setToolButtonStyle(Qt::ToolButtonStyle toolButtonStyle);

Q_SIGNALS:
    void actionTriggered(QAction *action);
    void movableChanged(bool movable);
    void allowedAreasChanged(Qt::ToolBarAreas allowedAreas);
    void orientationChanged(Qt::Orientation orientation);
    void iconSizeChanged(const QSize &iconSize);
    void toolButtonStyleChanged(Qt::ToolButtonStyle toolButtonStyle);
    void topLevelChanged(bool topLevel);
    void visibilityChanged(bool visible);

protected:
    void actionEvent(QActionEvent *event);
    void changeEvent(QEvent *event);
    void paintEvent(QPaintEvent *event);
    bool event(QEvent *event);
    void initStyleOption(QStyleOptionToolBar *option) const;


private:
    Q_DECLARE_PRIVATE(QToolBar)
    Q_DISABLE_COPY(QToolBar)
    Q_PRIVATE_SLOT(d_func(), void _q_toggleView(bool))
    Q_PRIVATE_SLOT(d_func(), void _q_updateIconSize(const QSize &))
    Q_PRIVATE_SLOT(d_func(), void _q_updateToolButtonStyle(Qt::ToolButtonStyle))

    friend class QMainWindow;
    friend class QMainWindowLayout;
    friend class QToolBarLayout;
    friend class QToolBarAreaLayout;
};

inline QAction *QToolBar::actionAt(int ax, int ay) const
{ return actionAt(QPoint(ax, ay)); }

#endif // QT_NO_TOOLBAR

QT_END_NAMESPACE

#endif // QDYNAMICTOOLBAR_H
