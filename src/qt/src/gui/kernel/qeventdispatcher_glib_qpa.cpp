/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "qeventdispatcher_glib_qpa_p.h"

#include "qapplication.h"

#include "qplatformdefs.h"
#include "qapplication.h"

#include <glib.h>
#include "qapplication_p.h"

#include <qdebug.h>

QT_BEGIN_NAMESPACE

struct GUserEventSource
{
    GSource source;
    QPAEventDispatcherGlib *q;
};

static gboolean userEventSourcePrepare(GSource *s, gint *timeout)
{
    Q_UNUSED(s)
    Q_UNUSED(timeout)

    return QWindowSystemInterfacePrivate::windowSystemEventsQueued() > 0;
}

static gboolean userEventSourceCheck(GSource *source)
{
    return userEventSourcePrepare(source, 0);
}

static gboolean userEventSourceDispatch(GSource *s, GSourceFunc, gpointer)
{
    GUserEventSource * source = reinterpret_cast<GUserEventSource *>(s);

    QWindowSystemInterfacePrivate::WindowSystemEvent * event;
    while (QWindowSystemInterfacePrivate::windowSystemEventsQueued()) {
        event = QWindowSystemInterfacePrivate::getWindowSystemEvent();
        if (!event)
            break;

        // send through event filter
        if (source->q->filterEvent(event)) {
            delete event;
            continue;
        }
        QApplicationPrivate::processWindowSystemEvent(event);
        delete event;
    }

    return true;
}


static GSourceFuncs userEventSourceFuncs = {
    userEventSourcePrepare,
    userEventSourceCheck,
    userEventSourceDispatch,
    NULL,
    NULL,
    NULL
};

QPAEventDispatcherGlibPrivate::QPAEventDispatcherGlibPrivate(GMainContext *context)
    : QEventDispatcherGlibPrivate(context)
{
    userEventSource = reinterpret_cast<GUserEventSource *>(g_source_new(&userEventSourceFuncs,
                                                                       sizeof(GUserEventSource)));
    userEventSource->q = 0;
    g_source_set_can_recurse(&userEventSource->source, true);
    g_source_attach(&userEventSource->source, mainContext);
}


QPAEventDispatcherGlib::QPAEventDispatcherGlib(QObject *parent)
    : QEventDispatcherGlib(*new QPAEventDispatcherGlibPrivate, parent)
{
    Q_D(QPAEventDispatcherGlib);
    d->userEventSource->q = this;
}

QPAEventDispatcherGlib::~QPAEventDispatcherGlib()
{
    Q_D(QPAEventDispatcherGlib);

    g_source_destroy(&d->userEventSource->source);
    g_source_unref(&d->userEventSource->source);
    d->userEventSource = 0;
}

bool QPAEventDispatcherGlib::processEvents(QEventLoop::ProcessEventsFlags flags)
{
    static bool init = false;
    if (!init) {
        if (QApplicationPrivate::platformIntegration()->createEventLoopIntegration()) {
            qWarning("Eventloop integration is not supported by the glib event dispatcher");
            qWarning("Use the UNIX event dispatcher by defining environment variable QT_NO_GLIB=1");
        }
        init = true;
    }
    return QEventDispatcherGlib::processEvents(flags);
}

QT_END_NAMESPACE
