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

#ifndef QTOOLBUTTON_H
#define QTOOLBUTTON_H

#include <QtWidgets/qabstractbutton.h>

QT_BEGIN_NAMESPACE


#ifndef QT_NO_TOOLBUTTON

class QToolButtonPrivate;
class QMenu;
class QStyleOptionToolButton;

class Q_WIDGETS_EXPORT QToolButton : public QAbstractButton
{
    Q_OBJECT
    Q_ENUMS(Qt::ToolButtonStyle Qt::ArrowType ToolButtonPopupMode)
#ifndef QT_NO_MENU
    Q_PROPERTY(ToolButtonPopupMode popupMode READ popupMode WRITE setPopupMode)
#endif
    Q_PROPERTY(Qt::ToolButtonStyle toolButtonStyle READ toolButtonStyle WRITE setToolButtonStyle)
    Q_PROPERTY(bool autoRaise READ autoRaise WRITE setAutoRaise)
    Q_PROPERTY(Qt::ArrowType arrowType READ arrowType WRITE setArrowType)

public:
    enum ToolButtonPopupMode {
        DelayedPopup,
        MenuButtonPopup,
        InstantPopup
    };

    explicit QToolButton(QWidget * parent=0);
    ~QToolButton();

    QSize sizeHint() const;
    QSize minimumSizeHint() const;

    Qt::ToolButtonStyle toolButtonStyle() const;

    Qt::ArrowType arrowType() const;
    void setArrowType(Qt::ArrowType type);

#ifndef QT_NO_MENU
    void setMenu(QMenu* menu);
    QMenu* menu() const;

    void setPopupMode(ToolButtonPopupMode mode);
    ToolButtonPopupMode popupMode() const;
#endif

    QAction *defaultAction() const;

    void setAutoRaise(bool enable);
    bool autoRaise() const;

public Q_SLOTS:
#ifndef QT_NO_MENU
    void showMenu();
#endif
    void setToolButtonStyle(Qt::ToolButtonStyle style);
    void setDefaultAction(QAction *);

Q_SIGNALS:
    void triggered(QAction *);

protected:
    bool event(QEvent *e);
    void mousePressEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void paintEvent(QPaintEvent *);
    void actionEvent(QActionEvent *);

    void enterEvent(QEvent *);
    void leaveEvent(QEvent *);
    void timerEvent(QTimerEvent *);
    void changeEvent(QEvent *);

    bool hitButton(const QPoint &pos) const;
    void nextCheckState();
    void initStyleOption(QStyleOptionToolButton *option) const;

private:
    Q_DISABLE_COPY(QToolButton)
    Q_DECLARE_PRIVATE(QToolButton)
#ifndef QT_NO_MENU
    Q_PRIVATE_SLOT(d_func(), void _q_buttonPressed())
    Q_PRIVATE_SLOT(d_func(), void _q_updateButtonDown())
    Q_PRIVATE_SLOT(d_func(), void _q_menuTriggered(QAction*))
#endif
    Q_PRIVATE_SLOT(d_func(), void _q_actionTriggered())

};

#endif // QT_NO_TOOLBUTTON

QT_END_NAMESPACE

#endif // QTOOLBUTTON_H
