/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2012 Klaralvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Christoph Schleifenbaum <christoph.schleifenbaum@kdab.com>
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

#include "qplatformsystemtrayicon.h"

#ifndef QT_NO_SYSTEMTRAYICON

QT_BEGIN_NAMESPACE

/*!
    \class QPlatformSystemTrayIcon
    \inmodule QtGui
    \brief The QPlatformSystemTrayIcon class abstracts the system tray icon and interaction.

    \sa QSystemTrayIcon
*/

/*!
    \enum QPlatformSystemTrayIcon::MessageIcon

    This enum describes the icon that is shown when a balloon message is displayed.

    \value NoIcon      No icon is shown.
    \value Information An information icon is shown.
    \value Warning     A standard warning icon is shown.
    \value Critical    A critical warning icon is shown.

    \sa updateIcon(), showMessage(), QMessageBox
*/

/*!
    \enum QPlatformSystemTrayIcon::ActivationReason

     This enum describes the reason the system tray was activated.

     \value Unknown     Unknown reason
     \value Context     The context menu for the system tray entry was requested
     \value DoubleClick The system tray entry was double clicked
     \value Trigger     The system tray entry was clicked
     \value MiddleClick The system tray entry was clicked with the middle mouse button

     \sa activated()
*/

/*!
    \internal
 */
QPlatformSystemTrayIcon::QPlatformSystemTrayIcon()
{
}

/*!
    \internal
 */
QPlatformSystemTrayIcon::~QPlatformSystemTrayIcon()
{
}

/*!
    \fn void QPlatformSystemTrayIcon::init()
    This method is called to initialize the platform dependent implementation.
*/

/*!
    \fn void QPlatformSystemTrayIcon::cleanup()
    This method is called to cleanup the platform dependent implementation.
*/

/*!
    \fn void QPlatformSystemTrayIcon::updateIcon(const QIcon &icon)
    This method is called when the \a icon did change.
*/

/*!
    \fn void QPlatformSystemTrayIcon::updateToolTip(const QString &tooltip)
    This method is called when the \a tooltip text did change.
*/

/*!
    \fn void QPlatformSystemTrayIcon::updateMenu(QPlatformMenu *menu)
    This method is called when the system tray \a menu did change.
*/

/*!
    \fn QRect QPlatformSystemTrayIcon::geometry() const
    This method returns the geometry of the platform dependent system tray icon on the screen.
*/

/*!
    \fn void QPlatformSystemTrayIcon::showMessage(const QString &msg, const QString &title,
                                                  const QIcon &icon, MessageIcon iconType, int secs)
    Shows a balloon message for the entry with the given \a title, message \a msg and \a icon for
    the time specified in \a secs. \a iconType is used as a hint for the implementing platform.
    \sa QSystemTrayIcon::showMessage()
*/

/*!
    \fn bool QPlatformSystemTrayIcon::isSystemTrayAvailable() const
    Returns \c true if the system tray is available on the platform.
*/

/*!
    \fn bool QPlatformSystemTrayIcon::supportsMessages() const
    Returns \c true if the system tray supports messages on the platform.
*/

/*!
    \fn void QPlatformSystemTrayIcon::activated(QPlatformSystemTrayIcon::ActivationReason reason)
    This signal is emitted when the user activates the system tray icon.
    \a reason specifies the reason for activation.
    \sa QSystemTrayIcon::ActivationReason
*/

/*!
    \fn void QPlatformSystemTrayIcon::messageClicked()

    This signal is emitted when the message displayed using showMessage()
    was clicked by the user.

    \sa activated()
*/

/*!
    This method is called in case there is no QPlatformMenu available when
    updating the menu. This allows the abstraction to provide a menu for the
    system tray icon even if normally a non-native menu is used.

    The default implementation returns a null pointer.

    \sa updateMenu()
    \since 5.3
 */

QPlatformMenu *QPlatformSystemTrayIcon::createMenu() const
{
    return Q_NULLPTR;
}

QT_END_NAMESPACE

#include "moc_qplatformsystemtrayicon.cpp"

#endif // QT_NO_SYSTEMTRAYICON
