/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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
#include "qibusplatforminputcontext.h"

#include <QtDebug>
#include <QTextCharFormat>
#include <QGuiApplication>
#include <qwindow.h>
#include <qevent.h>

#include "qibusproxy.h"
#include "qibusinputcontextproxy.h"
#include "qibustypes.h"

#include <sys/types.h>
#include <signal.h>

#include <QtDBus>

QT_BEGIN_NAMESPACE

enum { debug = 0 };

class QIBusPlatformInputContextPrivate
{
public:
    QIBusPlatformInputContextPrivate();
    ~QIBusPlatformInputContextPrivate()
    {
        delete context;
        delete bus;
        delete connection;
    }

    static QDBusConnection *createConnection();

    QDBusConnection *connection;
    QIBusProxy *bus;
    QIBusInputContextProxy *context;

    bool valid;
    QString predit;
};


QIBusPlatformInputContext::QIBusPlatformInputContext ()
    : d(new QIBusPlatformInputContextPrivate())
{
    if (d->context) {
        connect(d->context, SIGNAL(CommitText(QDBusVariant)), SLOT(commitText(QDBusVariant)));
        connect(d->context, SIGNAL(UpdatePreeditText(QDBusVariant,uint,bool)), this, SLOT(updatePreeditText(QDBusVariant,uint,bool)));
    }
    QInputMethod *p = qApp->inputMethod();
    connect(p, SIGNAL(cursorRectangleChanged()), this, SLOT(cursorRectChanged()));
}

QIBusPlatformInputContext::~QIBusPlatformInputContext (void)
{
    delete d;
}

bool QIBusPlatformInputContext::isValid() const
{
    return d->valid;
}

void QIBusPlatformInputContext::invokeAction(QInputMethod::Action a, int)
{
    if (!d->valid)
        return;

    if (a == QInputMethod::Click)
        commit();
}

void QIBusPlatformInputContext::reset()
{
    QPlatformInputContext::reset();

    if (!d->valid)
        return;

    d->context->Reset();
    d->predit = QString();
}

void QIBusPlatformInputContext::commit()
{
    QPlatformInputContext::commit();

    if (!d->valid)
        return;

    QObject *input = qApp->focusObject();
    if (!input) {
        d->predit = QString();
        return;
    }

    QInputMethodEvent event;
    event.setCommitString(d->predit);
    QCoreApplication::sendEvent(input, &event);

    d->context->Reset();
    d->predit = QString();
}


void QIBusPlatformInputContext::update(Qt::InputMethodQueries q)
{
    QPlatformInputContext::update(q);
}

void QIBusPlatformInputContext::cursorRectChanged()
{
    if (!d->valid)
        return;

    QRect r = qApp->inputMethod()->cursorRectangle().toRect();
    if(!r.isValid())
        return;

    QWindow *inputWindow = qApp->focusWindow();
    if (!inputWindow)
        return;
    r.moveTopLeft(inputWindow->mapToGlobal(r.topLeft()));
    if (debug)
        qDebug() << "microFocus" << r;
    d->context->SetCursorLocation(r.x(), r.y(), r.width(), r.height());
}

void QIBusPlatformInputContext::setFocusObject(QObject *object)
{
    if (!d->valid)
        return;

    if (debug)
        qDebug() << "setFocusObject" << object;
    if (object)
        d->context->FocusIn();
    else
        d->context->FocusOut();
}

void QIBusPlatformInputContext::commitText(const QDBusVariant &text)
{
    QObject *input = qApp->focusObject();
    if (!input)
        return;

    const QDBusArgument arg = text.variant().value<QDBusArgument>();

    QIBusText t;
    if (debug)
        qDebug() << arg.currentSignature();
    t.fromDBusArgument(arg);
    if (debug)
        qDebug() << "commit text:" << t.text;

    QInputMethodEvent event;
    event.setCommitString(t.text);
    QCoreApplication::sendEvent(input, &event);

    d->predit = QString();
}

void QIBusPlatformInputContext::updatePreeditText(const QDBusVariant &text, uint cursorPos, bool visible)
{
    QObject *input = qApp->focusObject();
    if (!input)
        return;

    const QDBusArgument arg = text.variant().value<QDBusArgument>();

    QIBusText t;
    t.fromDBusArgument(arg);
    if (debug)
        qDebug() << "preedit text:" << t.text;

    QList<QInputMethodEvent::Attribute> attributes = t.attributes.imAttributes();
    if (!t.text.isEmpty())
        attributes += QInputMethodEvent::Attribute(QInputMethodEvent::Cursor, cursorPos, visible ? 1 : 0, QVariant());

    QInputMethodEvent event(t.text, attributes);
    QCoreApplication::sendEvent(input, &event);

    d->predit = t.text;
}


bool
QIBusPlatformInputContext::x11FilterEvent(uint keyval, uint keycode, uint state, bool press)
{
    if (!d->valid)
        return false;

    if (!inputMethodAccepted())
        return false;

    if (!press)
        return false;

    keycode -= 8; // ###
    QDBusReply<bool> reply = d->context->ProcessKeyEvent(keyval, keycode, state);

//    qDebug() << "x11FilterEvent return" << reply.value();

    return reply.value();
}

QIBusPlatformInputContextPrivate::QIBusPlatformInputContextPrivate()
    : connection(createConnection()),
      bus(0),
      context(0),
      valid(false)
{
    if (!connection || !connection->isConnected())
        return;

    bus = new QIBusProxy(QLatin1String("org.freedesktop.IBus"),
                         QLatin1String("/org/freedesktop/IBus"),
                         *connection);
    if (!bus->isValid()) {
        qWarning("QIBusPlatformInputContext: invalid bus.");
        return;
    }

    QDBusReply<QDBusObjectPath> ic = bus->CreateInputContext(QLatin1String("QIBusInputContext"));
    if (!ic.isValid()) {
        qWarning("QIBusPlatformInputContext: CreateInputContext failed.");
        return;
    }

    context = new QIBusInputContextProxy(QLatin1String("org.freedesktop.IBus"), ic.value().path(), *connection);

    if (!context->isValid()) {
        qWarning("QIBusPlatformInputContext: invalid input context.");
        return;
    }

    enum Capabilities {
        IBUS_CAP_PREEDIT_TEXT       = 1 << 0,
        IBUS_CAP_AUXILIARY_TEXT     = 1 << 1,
        IBUS_CAP_LOOKUP_TABLE       = 1 << 2,
        IBUS_CAP_FOCUS              = 1 << 3,
        IBUS_CAP_PROPERTY           = 1 << 4,
        IBUS_CAP_SURROUNDING_TEXT   = 1 << 5
    };
    context->SetCapabilities(IBUS_CAP_PREEDIT_TEXT|IBUS_CAP_FOCUS);

    if (debug)
        qDebug(">>>> valid!");
    valid = true;
}

QDBusConnection *QIBusPlatformInputContextPrivate::createConnection()
{
    QByteArray display(getenv("DISPLAY"));
    QByteArray host = "unix";
    QByteArray displayNumber = "0";

    int pos = display.indexOf(':');
    if (pos > 0)
        host = display.left(pos);
    ++pos;
    int pos2 = display.indexOf('.', pos);
    if (pos2 > 0)
        displayNumber = display.mid(pos, pos2 - pos);
    if (debug)
        qDebug() << "host=" << host << "displayNumber" << displayNumber;

    QFile file(QDir::homePath() + QLatin1String("/.config/ibus/bus/") +
               QLatin1String(QDBusConnection::localMachineId()) +
               QLatin1Char('-') + QString::fromLocal8Bit(host) + QLatin1Char('-') + QString::fromLocal8Bit(displayNumber));

    if (!file.open(QFile::ReadOnly))
        return 0;

    QByteArray address;
    int pid = -1;

    while (!file.atEnd()) {
        QByteArray line = file.readLine().trimmed();
        if (line.startsWith('#'))
            continue;

        if (line.startsWith("IBUS_ADDRESS="))
            address = line.mid(sizeof("IBUS_ADDRESS=") - 1);
        if (line.startsWith("IBUS_DAEMON_PID="))
            pid = line.mid(sizeof("IBUS_DAEMON_PID=") - 1).toInt();
    }

    if (debug)
        qDebug() << "IBUS_ADDRESS=" << address << "PID=" << pid;
    if (address.isEmpty() || pid < 0 || kill(pid, 0) != 0)
        return 0;

    return new QDBusConnection(QDBusConnection::connectToBus(QString::fromLatin1(address), QLatin1String("QIBusProxy")));
}

QT_END_NAMESPACE
