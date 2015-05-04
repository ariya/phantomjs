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

#ifndef QMENU_H
#define QMENU_H

#include <QtWidgets/qwidget.h>
#include <QtCore/qstring.h>
#include <QtGui/qicon.h>
#include <QtWidgets/qaction.h>

#ifdef Q_OS_WINCE
#include <windef.h> // for HMENU
#endif
#ifdef Q_OS_OSX
Q_FORWARD_DECLARE_OBJC_CLASS(NSMenu);
#endif

QT_BEGIN_NAMESPACE


#ifndef QT_NO_MENU

class QMenuPrivate;
class QStyleOptionMenuItem;
class QPlatformMenu;

class Q_WIDGETS_EXPORT QMenu : public QWidget
{
private:
    Q_OBJECT
    Q_DECLARE_PRIVATE(QMenu)

    Q_PROPERTY(bool tearOffEnabled READ isTearOffEnabled WRITE setTearOffEnabled)
    Q_PROPERTY(QString title READ title WRITE setTitle)
    Q_PROPERTY(QIcon icon READ icon WRITE setIcon)
    Q_PROPERTY(bool separatorsCollapsible READ separatorsCollapsible WRITE setSeparatorsCollapsible)
    Q_PROPERTY(bool toolTipsVisible READ toolTipsVisible WRITE setToolTipsVisible)

public:
    explicit QMenu(QWidget *parent = 0);
    explicit QMenu(const QString &title, QWidget *parent = 0);
    ~QMenu();

#ifdef Q_NO_USING_KEYWORD
    inline void addAction(QAction *action) { QWidget::addAction(action); }
#else
    using QWidget::addAction;
#endif
    QAction *addAction(const QString &text);
    QAction *addAction(const QIcon &icon, const QString &text);
    QAction *addAction(const QString &text, const QObject *receiver, const char* member, const QKeySequence &shortcut = 0);
    QAction *addAction(const QIcon &icon, const QString &text, const QObject *receiver, const char* member, const QKeySequence &shortcut = 0);

    QAction *addMenu(QMenu *menu);
    QMenu *addMenu(const QString &title);
    QMenu *addMenu(const QIcon &icon, const QString &title);

    QAction *addSeparator();

    QAction *addSection(const QString &text);
    QAction *addSection(const QIcon &icon, const QString &text);

    QAction *insertMenu(QAction *before, QMenu *menu);
    QAction *insertSeparator(QAction *before);
    QAction *insertSection(QAction *before, const QString &text);
    QAction *insertSection(QAction *before, const QIcon &icon, const QString &text);

    bool isEmpty() const;
    void clear();

    void setTearOffEnabled(bool);
    bool isTearOffEnabled() const;

    bool isTearOffMenuVisible() const;
    void hideTearOffMenu();

    void setDefaultAction(QAction *);
    QAction *defaultAction() const;

    void setActiveAction(QAction *act);
    QAction *activeAction() const;

    void popup(const QPoint &pos, QAction *at=0);
    QAction *exec();
    QAction *exec(const QPoint &pos, QAction *at=0);

    static QAction *exec(QList<QAction*> actions, const QPoint &pos, QAction *at=0, QWidget *parent=0);

    QSize sizeHint() const;

    QRect actionGeometry(QAction *) const;
    QAction *actionAt(const QPoint &) const;

    QAction *menuAction() const;

    QString title() const;
    void setTitle(const QString &title);

    QIcon icon() const;
    void setIcon(const QIcon &icon);

    void setNoReplayFor(QWidget *widget);
    QPlatformMenu *platformMenu();
    void setPlatformMenu(QPlatformMenu *platformMenu);

#ifdef Q_OS_WINCE
    HMENU wceMenu();
#endif
#ifdef Q_OS_OSX
    NSMenu* toNSMenu();
    void setAsDockMenu();
#endif

    bool separatorsCollapsible() const;
    void setSeparatorsCollapsible(bool collapse);

    bool toolTipsVisible() const;
    void setToolTipsVisible(bool visible);

Q_SIGNALS:
    void aboutToShow();
    void aboutToHide();
    void triggered(QAction *action);
    void hovered(QAction *action);

protected:
    int columnCount() const;

    void changeEvent(QEvent *);
    void keyPressEvent(QKeyEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void mousePressEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
#ifndef QT_NO_WHEELEVENT
    void wheelEvent(QWheelEvent *);
#endif
    void enterEvent(QEvent *);
    void leaveEvent(QEvent *);
    void hideEvent(QHideEvent *);
    void paintEvent(QPaintEvent *);
    void actionEvent(QActionEvent *);
    void timerEvent(QTimerEvent *);
    bool event(QEvent *);
    bool focusNextPrevChild(bool next);
    void initStyleOption(QStyleOptionMenuItem *option, const QAction *action) const;

#ifdef Q_OS_WINCE
    QAction* wceCommands(uint command);
#endif

private Q_SLOTS:
    void internalSetSloppyAction();
    void internalDelayedPopup();

private:
    Q_PRIVATE_SLOT(d_func(), void _q_actionTriggered())
    Q_PRIVATE_SLOT(d_func(), void _q_actionHovered())
    Q_PRIVATE_SLOT(d_func(), void _q_overrideMenuActionDestroyed())
    Q_PRIVATE_SLOT(d_func(), void _q_platformMenuAboutToShow())

protected:
    QMenu(QMenuPrivate &dd, QWidget* parent = 0);

private:
    Q_DISABLE_COPY(QMenu)

    friend class QMenuBar;
    friend class QMenuBarPrivate;
    friend class QTornOffMenu;
    friend class QComboBox;
    friend class QAction;
    friend class QToolButtonPrivate;
    friend void qt_mac_emit_menuSignals(QMenu *menu, bool show);
    friend void qt_mac_menu_emit_hovered(QMenu *menu, QAction *action);
};

#ifdef Q_OS_OSX
// ### Qt 4 compatibility; remove in Qt 6
inline QT_DEPRECATED void qt_mac_set_dock_menu(QMenu *menu) { menu->setAsDockMenu(); }
#endif

#endif // QT_NO_MENU

QT_END_NAMESPACE

#endif // QMENU_H
