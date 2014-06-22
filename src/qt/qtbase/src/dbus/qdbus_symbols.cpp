/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtDBus module of the Qt Toolkit.
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

#include <QtCore/qglobal.h>
#ifndef QT_BOOTSTRAPPED
#include <QtCore/qlibrary.h>
#endif
#include <QtCore/qmutex.h>
#include <private/qmutexpool_p.h>

#ifndef QT_NO_DBUS

extern "C" void dbus_shutdown();

QT_BEGIN_NAMESPACE

void (*qdbus_resolve_me(const char *name))();

#if !defined QT_LINKED_LIBDBUS

#ifndef QT_BOOTSTRAPPED
static QLibrary *qdbus_libdbus = 0;

void qdbus_unloadLibDBus()
{
    if (qdbus_libdbus) {
        if (qEnvironmentVariableIsSet("QDBUS_FORCE_SHUTDOWN"))
            qdbus_libdbus->resolve("dbus_shutdown")();
        qdbus_libdbus->unload();
    }
    delete qdbus_libdbus;
    qdbus_libdbus = 0;
}
#endif

bool qdbus_loadLibDBus()
{
#ifndef QT_BOOTSTRAPPED
#ifdef QT_BUILD_INTERNAL
    // this is to simulate a library load failure for our autotest suite.
    if (!qEnvironmentVariableIsEmpty("QT_SIMULATE_DBUS_LIBFAIL"))
        return false;
#endif

    static bool triedToLoadLibrary = false;
#ifndef QT_NO_THREAD
    QMutexLocker locker(QMutexPool::globalInstanceGet((void *)&qdbus_resolve_me));
#endif
    QLibrary *&lib = qdbus_libdbus;
    if (triedToLoadLibrary)
        return lib && lib->isLoaded();

    lib = new QLibrary;
    triedToLoadLibrary = true;

    static int majorversions[] = { 3, 2, -1 };
    lib->unload();
    lib->setFileName(QLatin1String("dbus-1"));
    for (uint i = 0; i < sizeof(majorversions) / sizeof(majorversions[0]); ++i) {
        lib->setFileNameAndVersion(lib->fileName(), majorversions[i]);
        if (lib->load() && lib->resolve("dbus_connection_open_private"))
            return true;

        lib->unload();
    }

    delete lib;
    lib = 0;
    return false;
#else
    return true;
#endif
}

#ifndef QT_BOOTSTRAPPED
void (*qdbus_resolve_conditionally(const char *name))()
{
    if (qdbus_loadLibDBus())
        return qdbus_libdbus->resolve(name);
    return 0;
}
#endif

void (*qdbus_resolve_me(const char *name))()
{
#ifndef QT_BOOTSTRAPPED
    if (!qdbus_loadLibDBus())
        qFatal("Cannot find libdbus-1 in your system to resolve symbol '%s'.", name);

    QFunctionPointer ptr = qdbus_libdbus->resolve(name);
    if (!ptr)
        qFatal("Cannot resolve '%s' in your libdbus-1.", name);

    return ptr;
#else
    Q_UNUSED(name);
    return 0;
#endif
}

#else  // QT_LINKED_LIBDBUS
static void qdbus_unloadLibDBus()
{
    if (qEnvironmentVariableIsSet("QDBUS_FORCE_SHUTDOWN"))
        dbus_shutdown();
}

#endif // !QT_LINKED_LIBDBUS

#ifndef QT_BOOTSTRAPPED
Q_DESTRUCTOR_FUNCTION(qdbus_unloadLibDBus)
#endif

QT_END_NAMESPACE

#endif // QT_NO_DBUS
