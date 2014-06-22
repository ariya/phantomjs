/****************************************************************************
**
** Copyright (C) 2013 Samuel Gaist <samuel.gaist@edeltech.ch>
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

#ifndef QWINDOWSINTEGRATION_H
#define QWINDOWSINTEGRATION_H

#include <QtCore/qconfig.h>
#include <qpa/qplatformintegration.h>
#include <QtCore/QScopedPointer>

QT_BEGIN_NAMESPACE

struct QWindowsIntegrationPrivate;
struct QWindowsWindowData;
class QWindowsWindow;

class QWindowsIntegration : public QPlatformIntegration
{
public:
    enum Options { // Options to be passed on command line.
        FontDatabaseFreeType = 0x1,
        FontDatabaseNative = 0x2,
        DisableArb = 0x4,
        NoNativeDialogs = 0x8,
        XpNativeDialogs = 0x10,
        DontPassOsMouseEventsSynthesizedFromTouch = 0x20 // Do not pass OS-generated mouse events from touch.
    };

    explicit QWindowsIntegration(const QStringList &paramList);
    virtual ~QWindowsIntegration();

    bool hasCapability(QPlatformIntegration::Capability cap) const;

    QWindowsWindowData createWindowData(QWindow *window) const;
    QPlatformWindow *createPlatformWindow(QWindow *window) const;
#ifndef QT_NO_OPENGL
    virtual QPlatformOpenGLContext *createPlatformOpenGLContext(QOpenGLContext *context) const;
#endif
    virtual QAbstractEventDispatcher *createEventDispatcher() const;
    void initialize() Q_DECL_OVERRIDE;
#ifndef QT_NO_CLIPBOARD
    virtual QPlatformClipboard *clipboard() const;
#  ifndef QT_NO_DRAGANDDROP
    virtual QPlatformDrag *drag() const;
#  endif
#endif // !QT_NO_CLIPBOARD
    virtual QPlatformInputContext *inputContext() const;
#ifndef QT_NO_ACCESSIBILITY
    virtual QPlatformAccessibility *accessibility() const;
#endif
    virtual QPlatformFontDatabase *fontDatabase() const;
    virtual QStringList themeNames() const;
    virtual QPlatformTheme *createPlatformTheme(const QString &name) const;
    QPlatformServices *services() const;
    virtual QVariant styleHint(StyleHint hint) const;

    virtual Qt::KeyboardModifiers queryKeyboardModifiers() const;
    virtual QList<int> possibleKeys(const QKeyEvent *e) const;

    static QWindowsIntegration *instance();

    inline void emitScreenAdded(QPlatformScreen *s) { screenAdded(s); }

    unsigned options() const;

#if !defined(Q_OS_WINCE) && !defined(QT_NO_SESSIONMANAGER)
    virtual QPlatformSessionManager *createPlatformSessionManager(const QString &id, const QString &key) const;
#endif

private:
    QScopedPointer<QWindowsIntegrationPrivate> d;
};

QT_END_NAMESPACE

#endif
