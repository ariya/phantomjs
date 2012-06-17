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

#ifndef QDIALOGBUTTONBOX_H
#define QDIALOGBUTTONBOX_H

#include <QtGui/qwidget.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QAbstractButton;
class QPushButton;
class QDialogButtonBoxPrivate;

class Q_GUI_EXPORT QDialogButtonBox : public QWidget
{
    Q_OBJECT
    Q_FLAGS(StandardButtons)
    Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation)
    Q_PROPERTY(StandardButtons standardButtons READ standardButtons WRITE setStandardButtons)
    Q_PROPERTY(bool centerButtons READ centerButtons WRITE setCenterButtons)

public:
    enum ButtonRole {
        // keep this in sync with QMessageBox::ButtonRole
        InvalidRole = -1,
        AcceptRole,
        RejectRole,
        DestructiveRole,
        ActionRole,
        HelpRole,
        YesRole,
        NoRole,
        ResetRole,
        ApplyRole,

        NRoles
    };

    enum StandardButton {
        // keep this in sync with QMessageBox::StandardButton
        NoButton           = 0x00000000,
        Ok                 = 0x00000400,
        Save               = 0x00000800,
        SaveAll            = 0x00001000,
        Open               = 0x00002000,
        Yes                = 0x00004000,
        YesToAll           = 0x00008000,
        No                 = 0x00010000,
        NoToAll            = 0x00020000,
        Abort              = 0x00040000,
        Retry              = 0x00080000,
        Ignore             = 0x00100000,
        Close              = 0x00200000,
        Cancel             = 0x00400000,
        Discard            = 0x00800000,
        Help               = 0x01000000,
        Apply              = 0x02000000,
        Reset              = 0x04000000,
        RestoreDefaults    = 0x08000000,

#ifndef Q_MOC_RUN
        FirstButton        = Ok,
        LastButton         = RestoreDefaults
#endif
    };

    Q_DECLARE_FLAGS(StandardButtons, StandardButton)

    enum ButtonLayout {
        WinLayout,
        MacLayout,
        KdeLayout,
        GnomeLayout
    };

    QDialogButtonBox(QWidget *parent = 0);
    QDialogButtonBox(Qt::Orientation orientation, QWidget *parent = 0);
    QDialogButtonBox(StandardButtons buttons, Qt::Orientation orientation = Qt::Horizontal,
                     QWidget *parent = 0);
    ~QDialogButtonBox();

    void setOrientation(Qt::Orientation orientation);
    Qt::Orientation orientation() const;

    void addButton(QAbstractButton *button, ButtonRole role);
    QPushButton *addButton(const QString &text, ButtonRole role);
    QPushButton *addButton(StandardButton button);
    void removeButton(QAbstractButton *button);
    void clear();

    QList<QAbstractButton *> buttons() const;
    ButtonRole buttonRole(QAbstractButton *button) const;

    void setStandardButtons(StandardButtons buttons);
    StandardButtons standardButtons() const;
    StandardButton standardButton(QAbstractButton *button) const;
    QPushButton *button(StandardButton which) const;

    void setCenterButtons(bool center);
    bool centerButtons() const;

Q_SIGNALS:
    void clicked(QAbstractButton *button);
    void accepted();
    void helpRequested();
    void rejected();

protected:
    void changeEvent(QEvent *event);
    bool event(QEvent *event);

private:
    Q_DISABLE_COPY(QDialogButtonBox)
    Q_DECLARE_PRIVATE(QDialogButtonBox)
    Q_PRIVATE_SLOT(d_func(), void _q_handleButtonClicked())
    Q_PRIVATE_SLOT(d_func(), void _q_handleButtonDestroyed())
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QDialogButtonBox::StandardButtons)

QT_END_NAMESPACE

QT_END_HEADER

#endif // QDIALOGBUTTONBOX_H
