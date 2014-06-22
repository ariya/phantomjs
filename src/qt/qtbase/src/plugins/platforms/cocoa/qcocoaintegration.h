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

#ifndef QPLATFORMINTEGRATION_COCOA_H
#define QPLATFORMINTEGRATION_COCOA_H

#include <Cocoa/Cocoa.h>

#include "qcocoaautoreleasepool.h"
#include "qcocoacursor.h"
#include "qcocoawindow.h"
#include "qcocoanativeinterface.h"
#include "qcocoainputcontext.h"
#include "qcocoaaccessibility.h"
#include "qcocoaclipboard.h"
#include "qcocoadrag.h"
#include "qcocoaservices.h"
#include "qcocoakeymapper.h"

#include <QtCore/QScopedPointer>
#include <qpa/qplatformintegration.h>
#include <QtPlatformSupport/private/qcoretextfontdatabase_p.h>

QT_BEGIN_NAMESPACE

class QCocoaScreen : public QPlatformScreen
{
public:
    QCocoaScreen(int screenIndex);
    ~QCocoaScreen();

    // ----------------------------------------------------
    // Virtual methods overridden from QPlatformScreen
    QPixmap grabWindow(WId window, int x, int y, int width, int height) const;
    QRect geometry() const { return m_geometry; }
    QRect availableGeometry() const { return m_availableGeometry; }
    int depth() const { return m_depth; }
    QImage::Format format() const { return m_format; }
    qreal devicePixelRatio() const;
    QSizeF physicalSize() const { return m_physicalSize; }
    QDpi logicalDpi() const { return m_logicalDpi; }
    qreal refreshRate() const { return m_refreshRate; }
    QString name() const { return m_name; }
    QPlatformCursor *cursor() const  { return m_cursor; }
    QWindow *topLevelAt(const QPoint &point) const;
    QList<QPlatformScreen *> virtualSiblings() const { return m_siblings; }

    // ----------------------------------------------------
    // Additional methods
    void setVirtualSiblings(QList<QPlatformScreen *> siblings) { m_siblings = siblings; }
    NSScreen *osScreen() const;
    void updateGeometry();

public:
    int m_screenIndex;
    QRect m_geometry;
    QRect m_availableGeometry;
    QDpi m_logicalDpi;
    qreal m_refreshRate;
    int m_depth;
    QString m_name;
    QImage::Format m_format;
    QSizeF m_physicalSize;
    QCocoaCursor *m_cursor;
    QList<QPlatformScreen *> m_siblings;
};

class QCocoaIntegration : public QPlatformIntegration
{
public:
    QCocoaIntegration();
    ~QCocoaIntegration();

    static QCocoaIntegration *instance();

    bool hasCapability(QPlatformIntegration::Capability cap) const;
    QPlatformWindow *createPlatformWindow(QWindow *window) const;
#ifndef QT_NO_OPENGL
    QPlatformOpenGLContext *createPlatformOpenGLContext(QOpenGLContext *context) const;
#endif
    QPlatformBackingStore *createPlatformBackingStore(QWindow *widget) const;

    QAbstractEventDispatcher *createEventDispatcher() const;

    QCoreTextFontDatabase *fontDatabase() const;
    QCocoaNativeInterface *nativeInterface() const;
    QCocoaInputContext *inputContext() const;
    QCocoaAccessibility *accessibility() const;
    QCocoaClipboard *clipboard() const;
    QCocoaDrag *drag() const;

    QStringList themeNames() const;
    QPlatformTheme *createPlatformTheme(const QString &name) const;
    QCocoaServices *services() const;
    QVariant styleHint(StyleHint hint) const;

    Qt::KeyboardModifiers queryKeyboardModifiers() const;
    QList<int> possibleKeys(const QKeyEvent *event) const;

    void updateScreens();
    QCocoaScreen *screenAtIndex(int index);

    void setToolbar(QWindow *window, NSToolbar *toolbar);
    NSToolbar *toolbar(QWindow *window) const;
    void clearToolbars();
    void setWindow(NSWindow* nsWindow, QCocoaWindow *window);
    QCocoaWindow *window(NSWindow *window);
private:
    static QCocoaIntegration *mInstance;

    QScopedPointer<QCoreTextFontDatabase> mFontDb;

    QScopedPointer<QCocoaInputContext> mInputContext;
#ifndef QT_NO_ACCESSIBILITY
    QScopedPointer<QCocoaAccessibility> mAccessibility;
#endif
    QScopedPointer<QPlatformTheme> mPlatformTheme;
    QList<QCocoaScreen *> mScreens;
    QCocoaClipboard  *mCocoaClipboard;
    QScopedPointer<QCocoaDrag> mCocoaDrag;
    QScopedPointer<QCocoaNativeInterface> mNativeInterface;
    QScopedPointer<QCocoaServices> mServices;
    QScopedPointer<QCocoaKeyMapper> mKeyboardMapper;

    QHash<QWindow *, NSToolbar *> mToolbars;
    QHash<NSWindow *, QCocoaWindow*> mWindows;
};

QT_END_NAMESPACE

#endif

