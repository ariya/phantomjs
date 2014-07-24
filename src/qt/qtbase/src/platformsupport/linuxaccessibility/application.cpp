/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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


#include "application_p.h"

#include <QtCore/qcoreapplication.h>
#include <QtDBus/qdbuspendingreply.h>
#include <qdebug.h>

#include "deviceeventcontroller_adaptor.h"

//#define KEYBOARD_DEBUG

QT_BEGIN_NAMESPACE

/*!
    \class QSpiApplicationAdaptor
    \internal

    \brief QSpiApplicationAdaptor

    QSpiApplicationAdaptor
*/

QSpiApplicationAdaptor::QSpiApplicationAdaptor(const QDBusConnection &connection, QObject *parent)
    : QObject(parent), dbusConnection(connection)
{
}

enum QSpiKeyEventType {
      QSPI_KEY_EVENT_PRESS,
      QSPI_KEY_EVENT_RELEASE,
      QSPI_KEY_EVENT_LAST_DEFINED
};

void QSpiApplicationAdaptor::sendEvents(bool active)
{
    if (active) {
        qApp->installEventFilter(this);
    } else {
        qApp->removeEventFilter(this);
    }
}


bool QSpiApplicationAdaptor::eventFilter(QObject *target, QEvent *event)
{
    if (!event->spontaneous())
        return false;

    switch (event->type()) {
    case QEvent::WindowActivate:
        emit windowActivated(target, true);
        break;
    case QEvent::WindowDeactivate:
        emit windowActivated(target, false);
        break;
    case QEvent::KeyPress:
    case QEvent::KeyRelease: {
        QKeyEvent *keyEvent = static_cast <QKeyEvent *>(event);
        QSpiDeviceEvent de;

        if (event->type() == QEvent::KeyPress)
            de.type = QSPI_KEY_EVENT_PRESS;
        else
            de.type = QSPI_KEY_EVENT_RELEASE;

        de.id = keyEvent->nativeVirtualKey();
        de.hardwareCode = keyEvent->nativeScanCode();

        de.modifiers = keyEvent->nativeModifiers();
        de.timestamp = QDateTime::currentMSecsSinceEpoch();

        if (keyEvent->key() == Qt::Key_Tab)
            de.text = QStringLiteral("Tab");
        else if (keyEvent->key() == Qt::Key_Backtab)
            de.text = QStringLiteral("Backtab");
        else if (keyEvent->key() == Qt::Key_Left)
            de.text = (keyEvent->modifiers() & Qt::KeypadModifier) ? QStringLiteral("KP_Left") : QStringLiteral("Left");
        else if (keyEvent->key() == Qt::Key_Right)
            de.text = (keyEvent->modifiers() & Qt::KeypadModifier) ? QStringLiteral("KP_Right") : QStringLiteral("Right");
        else if (keyEvent->key() == Qt::Key_Up)
            de.text = (keyEvent->modifiers() & Qt::KeypadModifier) ? QStringLiteral("KP_Up") : QStringLiteral("Up");
        else if (keyEvent->key() == Qt::Key_Down)
            de.text = (keyEvent->modifiers() & Qt::KeypadModifier) ? QStringLiteral("KP_Down") : QStringLiteral("Down");
        else if (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return)
            de.text = QStringLiteral("Return");
        else if (keyEvent->key() == Qt::Key_Backspace)
            de.text = QStringLiteral("BackSpace");
        else if (keyEvent->key() == Qt::Key_Delete)
            de.text = QStringLiteral("Delete");
        else if (keyEvent->key() == Qt::Key_PageUp)
            de.text = (keyEvent->modifiers() & Qt::KeypadModifier) ? QStringLiteral("KP_Page_Up") : QStringLiteral("Page_Up");
        else if (keyEvent->key() == Qt::Key_PageDown)
            de.text = (keyEvent->modifiers() & Qt::KeypadModifier) ? QStringLiteral("KP_Page_Up") : QStringLiteral("Page_Down");
        else if (keyEvent->key() == Qt::Key_Home)
            de.text = (keyEvent->modifiers() & Qt::KeypadModifier) ? QStringLiteral("KP_Home") : QStringLiteral("Home");
        else if (keyEvent->key() == Qt::Key_End)
            de.text = (keyEvent->modifiers() & Qt::KeypadModifier) ? QStringLiteral("KP_End") : QStringLiteral("End");
        else if (keyEvent->key() == Qt::Key_Clear && (keyEvent->modifiers() & Qt::KeypadModifier))
            de.text = QStringLiteral("KP_Begin"); // Key pad 5
        else if (keyEvent->key() == Qt::Key_Escape)
            de.text = QStringLiteral("Escape");
        else if (keyEvent->key() == Qt::Key_Space)
            de.text = QStringLiteral("space");
        else if (keyEvent->key() == Qt::Key_CapsLock)
            de.text = QStringLiteral("Caps_Lock");
        else if (keyEvent->key() == Qt::Key_NumLock)
            de.text = QStringLiteral("Num_Lock");
        else if (keyEvent->key() == Qt::Key_Insert)
            de.text = QStringLiteral("Insert");
        else
            de.text = keyEvent->text();

        // This is a bit dubious, Gnome uses some gtk function here.
        // Long term the spec will hopefully change to just use keycodes.
        de.isText = !de.text.isEmpty();

#ifdef KEYBOARD_DEBUG
        qDebug() << QStringLiteral("Key event text: ") << event->type() << de.isText << QStringLiteral(" ") << de.text
                 << QStringLiteral(" hardware code: ") << de.hardwareCode
                 << QStringLiteral(" native sc: ") << keyEvent->nativeScanCode()
                 << QStringLiteral(" native mod: ") << keyEvent->nativeModifiers()
                 << QStringLiteral("native virt: ") << keyEvent->nativeVirtualKey();
#endif

        QDBusMessage m = QDBusMessage::createMethodCall(QStringLiteral("org.a11y.atspi.Registry"),
                                                        QStringLiteral("/org/a11y/atspi/registry/deviceeventcontroller"),
                                                        QStringLiteral("org.a11y.atspi.DeviceEventController"), QStringLiteral("NotifyListenersSync"));
        m.setArguments(QVariantList() <<QVariant::fromValue(de));

        // FIXME: this is critical, the timeout should probably be pretty low to allow normal processing
        int timeout = 100;
        bool sent = dbusConnection.callWithCallback(m, this, SLOT(notifyKeyboardListenerCallback(QDBusMessage)),
                        SLOT(notifyKeyboardListenerError(QDBusError,QDBusMessage)), timeout);
        if (sent) {
            //queue the event and send it after callback
            keyEvents.enqueue(QPair<QPointer<QObject>, QKeyEvent*> (QPointer<QObject>(target), copyKeyEvent(keyEvent)));
#ifdef KEYBOARD_DEBUG
            qDebug() << QStringLiteral("Sent key: ") << de.text;
#endif
            return true;
        }
    }
    default:
        break;
    }
    return false;
}

QKeyEvent* QSpiApplicationAdaptor::copyKeyEvent(QKeyEvent* old)
{
    return new QKeyEvent(old->type(), old->key(), old->modifiers(), old->text(), old->isAutoRepeat(), old->count());
}

void QSpiApplicationAdaptor::notifyKeyboardListenerCallback(const QDBusMessage& message)
{
    if (!keyEvents.length()) {
        qWarning() << QStringLiteral("QSpiApplication::notifyKeyboardListenerCallback with no queued key called");
        return;
    }
    Q_ASSERT(message.arguments().length() == 1);
    if (message.arguments().at(0).toBool() == true) {
        QPair<QPointer<QObject>, QKeyEvent*> event = keyEvents.dequeue();
        delete event.second;
    } else {
        QPair<QPointer<QObject>, QKeyEvent*> event = keyEvents.dequeue();
        if (event.first)
            QCoreApplication::postEvent(event.first.data(), event.second);
    }
}

void QSpiApplicationAdaptor::notifyKeyboardListenerError(const QDBusError& error, const QDBusMessage& /*message*/)
{
    qWarning() << QStringLiteral("QSpiApplication::keyEventError ") << error.name() << error.message();
    while (!keyEvents.isEmpty()) {
        QPair<QPointer<QObject>, QKeyEvent*> event = keyEvents.dequeue();
        if (event.first)
            QCoreApplication::postEvent(event.first.data(), event.second);
    }
}

QT_END_NAMESPACE
