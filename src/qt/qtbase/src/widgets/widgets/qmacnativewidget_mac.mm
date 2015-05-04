/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#import <Cocoa/Cocoa.h>
#include "qmacnativewidget_mac.h"

#include <QtCore/qdebug.h>
#include <QtGui/qwindow.h>
#include <QtGui/qguiapplication.h>
#include <qpa/qplatformnativeinterface.h>

/*!
    \class QMacNativeWidget
    \since 4.5
    \brief The QMacNativeWidget class provides a widget for Mac OS X that provides
    a way to put Qt widgets into Cocoa hierarchies.

    \ingroup advanced
    \inmodule QtWidgets

    On Mac OS X, there is a difference between a window and view;
    normally expressed as widgets in Qt.  Qt makes assumptions about its
    parent-child hierarchy that make it complex to put an arbitrary Qt widget
    into a hierarchy of "normal" views from Apple frameworks. QMacNativeWidget
    bridges the gap between views and windows and makes it possible to put a
    hierarchy of Qt widgets into a non-Qt window or view.

    QMacNativeWidget pretends it is a window (i.e. isWindow() will return true),
    but it cannot be shown on its own. It needs to be put into a window
    when it is created or later through a native call.

    QMacNativeWidget works for either Carbon or Cocoa depending on how Qt was configured. If Qt is
    using Carbon, QMacNativeWidget will embed into Carbon hierarchies. If Qt is
    using Cocoa, QMacNativeWidget embeds into Cocoa hierarchies.

    Here is an example of putting a QPushButton into a NSWindow:

    \snippet qmacnativewidget/main.mm 0

    Note that QMacNativeWidget requires knowledge of Carbon or Cocoa. All it
    does is get the Qt hierarchy into a window not owned by Qt. It is then up
    to the programmer to ensure it is placed correctly in the window and
    responds correctly to events.
*/

QT_BEGIN_NAMESPACE

namespace {
// TODO use QtMacExtras copy of this function when available.
inline QPlatformNativeInterface::NativeResourceForIntegrationFunction resolvePlatformFunction(const QByteArray &functionName)
{
    QPlatformNativeInterface *nativeInterface = QGuiApplication::platformNativeInterface();
    QPlatformNativeInterface::NativeResourceForIntegrationFunction function =
        nativeInterface->nativeResourceFunctionForIntegration(functionName);
    if (!function)
         qWarning() << "Qt could not resolve function" << functionName
                    << "from QGuiApplication::platformNativeInterface()->nativeResourceFunctionForIntegration()";
    return function;
}
} //namespsace

NSView *getEmbeddableView(QWindow *qtWindow)
{
    // Make sure the platform window is created
    qtWindow->create();

    // Inform the window that it's a subwindow of a non-Qt window. This must be
    // done after create() because we need to have a QPlatformWindow instance.
    // The corresponding NSWindow will not be shown and can be deleted later.
    typedef void (*SetEmbeddedInForeignViewFunction)(QPlatformWindow *window, bool embedded);
    reinterpret_cast<SetEmbeddedInForeignViewFunction>(resolvePlatformFunction("setEmbeddedInForeignView"))(qtWindow->handle(), true);

    // Get the Qt content NSView for the QWindow from the Qt platform plugin
    QPlatformNativeInterface *platformNativeInterface = QGuiApplication::platformNativeInterface();
    NSView *qtView = (NSView *)platformNativeInterface->nativeResourceForWindow("nsview", qtWindow);
    return qtView; // qtView is ready for use.
}

/*!
    Create a QMacNativeWidget with \a parentView as its "superview" (i.e.,
    parent). The \a parentView is  a NSView pointer.
*/
QMacNativeWidget::QMacNativeWidget(NSView *parentView)
    : QWidget(0)
{
    Q_UNUSED(parentView);

    //d_func()->topData()->embedded = true;
    setPalette(QPalette(Qt::transparent));
    setAttribute(Qt::WA_SetPalette, false);
    setAttribute(Qt::WA_LayoutUsesWidgetRect);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground, false);
}

/*!
    Destroy the QMacNativeWidget.
*/
QMacNativeWidget::~QMacNativeWidget()
{
}

/*!
    \reimp
*/
QSize QMacNativeWidget::sizeHint() const
{
    // QMacNativeWidget really does not have any other choice
    // than to fill its designated area.
    if (windowHandle())
        return windowHandle()->size();
    return QWidget::sizeHint();
}

NSView *QMacNativeWidget::nativeView() const
{
    winId();
    return getEmbeddableView(windowHandle());
}

/*!
    \reimp
*/
bool QMacNativeWidget::event(QEvent *ev)
{
    return QWidget::event(ev);
}

QT_END_NAMESPACE
