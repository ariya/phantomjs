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

#ifndef QWINDOWSNATIVEINTERFACE_H
#define QWINDOWSNATIVEINTERFACE_H

#include "qtwindows_additional.h"
#include <QtGui/qpa/qplatformnativeinterface.h>

QT_BEGIN_NAMESPACE

/*!
    \class QWindowsNativeInterface
    \brief Provides access to native handles.

    Currently implemented keys
    \list
    \li handle (HWND)
    \li getDC (DC)
    \li releaseDC Releases the previously acquired DC and returns 0.
    \endlist

    \internal
    \ingroup qt-lighthouse-win
*/

class QWindowsNativeInterface : public QPlatformNativeInterface
{
    Q_OBJECT
    Q_PROPERTY(bool asyncExpose READ asyncExpose WRITE setAsyncExpose)
public:
#ifndef QT_NO_OPENGL
    virtual void *nativeResourceForContext(const QByteArray &resource, QOpenGLContext *context);
#endif
    virtual void *nativeResourceForWindow(const QByteArray &resource, QWindow *window);

    Q_INVOKABLE void *createMessageWindow(const QString &classNameTemplate,
                                          const QString &windowName,
                                          void *eventProc) const;

    Q_INVOKABLE QString registerWindowClass(const QString &classNameIn, void *eventProc) const;

    Q_INVOKABLE void beep() { MessageBeep(MB_OK); } // For QApplication

    bool asyncExpose() const;
    void setAsyncExpose(bool value);

    QVariantMap windowProperties(QPlatformWindow *window) const;
    QVariant windowProperty(QPlatformWindow *window, const QString &name) const;
    QVariant windowProperty(QPlatformWindow *window, const QString &name, const QVariant &defaultValue) const;
    void setWindowProperty(QPlatformWindow *window, const QString &name, const QVariant &value);
};

QT_END_NAMESPACE

#endif // QWINDOWSNATIVEINTERFACE_H
