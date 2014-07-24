/*
 * Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "PlatformUtilities.h"

#include <WebKit2/WKStringQt.h>
#include <WebKit2/WKNativeEvent.h>
#include <WebKit2/WKURLQt.h>

#include <QCoreApplication>
#include <QDir>
#include <QUrl>
#include <QThread>

namespace TestWebKitAPI {
namespace Util {

void run(bool* done)
{
    while (!*done)
        QCoreApplication::processEvents();
}

void sleep(double seconds)
{
    QThread::sleep(seconds);
}

WKStringRef createInjectedBundlePath()
{
    QString path = QFileInfo(QStringLiteral(ROOT_BUILD_DIR "/lib/libTestWebKitAPIInjectedBundle")).absoluteFilePath();

    return WKStringCreateWithQString(path);
}

WKURLRef createURLForResource(const char* resource, const char* extension)
{
    QDir path(QStringLiteral(APITEST_SOURCE_DIR));
    QString filename = QString::fromLocal8Bit(resource) + QStringLiteral(".") + QString::fromLocal8Bit(extension);

    return WKURLCreateWithQUrl(QUrl::fromLocalFile(path.absoluteFilePath(filename)));
}

WKURLRef URLForNonExistentResource()
{
    return WKURLCreateWithUTF8CString("file:///does-not-exist.html");
}

bool isKeyDown(WKNativeEventPtr event)
{
    return event->type() == QEvent::KeyPress;
}

} // namespace Util
} // namespace TestWebKitAPI
