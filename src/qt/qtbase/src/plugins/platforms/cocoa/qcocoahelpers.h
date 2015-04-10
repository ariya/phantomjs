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

#ifndef QCOCOAHELPERS_H
#define QCOCOAHELPERS_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It provides helper functions
// for the Cocoa lighthouse plugin. This header file may
// change from version to version without notice, or even be removed.
//
// We mean it.
//
#include "qt_mac_p.h"
#include <private/qguiapplication_p.h>
#include <QtGui/qscreen.h>

QT_BEGIN_NAMESPACE

class QPixmap;
class QString;

// Conversion functions
QStringList qt_mac_NSArrayToQStringList(void *nsarray);
void *qt_mac_QStringListToNSMutableArrayVoid(const QStringList &list);

inline NSMutableArray *qt_mac_QStringListToNSMutableArray(const QStringList &qstrlist)
{ return reinterpret_cast<NSMutableArray *>(qt_mac_QStringListToNSMutableArrayVoid(qstrlist)); }

NSImage *qt_mac_cgimage_to_nsimage(CGImageRef iamge);
NSImage *qt_mac_create_nsimage(const QPixmap &pm);
NSImage *qt_mac_create_nsimage(const QIcon &icon);
CGImageRef qt_mac_toCGImage(const QImage &qImage);
CGImageRef qt_mac_toCGImageMask(const QImage &qImage);
QImage qt_mac_toQImage(CGImageRef image);

NSSize qt_mac_toNSSize(const QSize &qtSize);
NSRect qt_mac_toNSRect(const QRect &rect);
QRect qt_mac_toQRect(const NSRect &rect);

QColor qt_mac_toQColor(const NSColor *color);


// Creates a mutable shape, it's the caller's responsibility to release.
HIMutableShapeRef qt_mac_QRegionToHIMutableShape(const QRegion &region);

OSStatus qt_mac_drawCGImage(CGContextRef inContext, const CGRect *inBounds, CGImageRef inImage);

QChar qt_mac_qtKey2CocoaKey(Qt::Key key);
Qt::Key qt_mac_cocoaKey2QtKey(QChar keyCode);

NSDragOperation qt_mac_mapDropAction(Qt::DropAction action);
NSDragOperation qt_mac_mapDropActions(Qt::DropActions actions);
Qt::DropAction qt_mac_mapNSDragOperation(NSDragOperation nsActions);
Qt::DropActions qt_mac_mapNSDragOperations(NSDragOperation nsActions);

// Misc
void qt_mac_transformProccessToForegroundApplication();
QString qt_mac_removeMnemonics(const QString &original);
CGColorSpaceRef qt_mac_genericColorSpace();
CGColorSpaceRef qt_mac_displayColorSpace(const QWidget *widget);
CGColorSpaceRef qt_mac_colorSpaceForDeviceType(const QPaintDevice *paintDevice);
QString qt_mac_applicationName();

int qt_mac_flipYCoordinate(int y);
qreal qt_mac_flipYCoordinate(qreal y);
QPointF qt_mac_flipPoint(const NSPoint &p);
NSPoint qt_mac_flipPoint(const QPoint &p);
NSPoint qt_mac_flipPoint(const QPointF &p);

NSRect qt_mac_flipRect(const QRect &rect);

Qt::MouseButton cocoaButton2QtButton(NSInteger buttonNum);

bool qt_mac_execute_apple_script(const char *script, long script_len, AEDesc *ret);
bool qt_mac_execute_apple_script(const char *script, AEDesc *ret);
bool qt_mac_execute_apple_script(const QString &script, AEDesc *ret);

// strip out '&' characters, and convert "&&" to a single '&', in menu
// text - since menu text is sometimes decorated with these for Windows
// accelerators.
QString qt_mac_removeAmpersandEscapes(QString s);

enum {
    QtCocoaEventSubTypeWakeup       = SHRT_MAX,
    QtCocoaEventSubTypePostMessage  = SHRT_MAX-1
};

class QCocoaPostMessageArgs {
public:
    id target;
    SEL selector;
    int argCount;
    id arg1;
    id arg2;
    QCocoaPostMessageArgs(id target, SEL selector, int argCount=0, id arg1=0, id arg2=0)
        : target(target), selector(selector), argCount(argCount), arg1(arg1), arg2(arg2)
    {
        [target retain];
        [arg1 retain];
        [arg2 retain];
    }

    ~QCocoaPostMessageArgs()
    {
        [arg2 release];
        [arg1 release];
        [target release];
    }
};

CGContextRef qt_mac_cg_context(QPaintDevice *pdev);

template<typename T>
T qt_mac_resolveOption(const T &fallback, const QByteArray &environment)
{
    // check for environment variable
    if (!environment.isEmpty()) {
        QByteArray env = qgetenv(environment);
        if (!env.isEmpty())
            return T(env.toInt()); // works when T is bool, int.
    }

    return fallback;
}

template<typename T>
T qt_mac_resolveOption(const T &fallback, QWindow *window, const QByteArray &property, const QByteArray &environment)
{
    // check for environment variable
    if (!environment.isEmpty()) {
        QByteArray env = qgetenv(environment);
        if (!env.isEmpty())
            return T(env.toInt()); // works when T is bool, int.
    }

    // check for window property
    if (window && !property.isNull()) {
        QVariant windowProperty = window->property(property);
        if (windowProperty.isValid())
            return windowProperty.value<T>();
    }

    // return default value.
    return fallback;
}
QT_END_NAMESPACE

#endif //QCOCOAHELPERS_H

