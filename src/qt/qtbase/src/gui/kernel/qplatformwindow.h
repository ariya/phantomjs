/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef QPLATFORMWINDOW_H
#define QPLATFORMWINDOW_H

//
//  W A R N I N G
//  -------------
//
// This file is part of the QPA API and is not meant to be used
// in applications. Usage of this API may make your code
// source and binary incompatible with future versions of Qt.
//

#include <QtCore/qscopedpointer.h>
#include <QtCore/qrect.h>
#include <QtCore/qmargins.h>
#include <QtCore/qstring.h>
#include <QtGui/qwindowdefs.h>
#include <QtGui/qwindow.h>
#include <qpa/qplatformopenglcontext.h>
#include <qpa/qplatformsurface.h>

QT_BEGIN_NAMESPACE


class QPlatformScreen;
class QPlatformWindowPrivate;
class QWindow;
class QIcon;
class QRegion;

class Q_GUI_EXPORT QPlatformWindow : public QPlatformSurface
{
    Q_DECLARE_PRIVATE(QPlatformWindow)
public:
    explicit QPlatformWindow(QWindow *window);
    virtual ~QPlatformWindow();

    QWindow *window() const;
    QPlatformWindow *parent() const;

    QPlatformScreen *screen() const;

    virtual QSurfaceFormat format() const;

    virtual void setGeometry(const QRect &rect);
    virtual QRect geometry() const;
    virtual QRect normalGeometry() const;

    virtual QMargins frameMargins() const;

    virtual void setVisible(bool visible);
    virtual void setWindowFlags(Qt::WindowFlags flags);
    virtual void setWindowState(Qt::WindowState state);

    virtual WId winId() const;
    virtual void setParent(const QPlatformWindow *window);

    virtual void setWindowTitle(const QString &title);
    virtual void setWindowFilePath(const QString &title);
    virtual void setWindowIcon(const QIcon &icon);
    virtual void raise();
    virtual void lower();

    virtual bool isExposed() const;
    virtual bool isActive() const;
    virtual bool isEmbedded(const QPlatformWindow *parentWindow) const;
    virtual QPoint mapToGlobal(const QPoint &pos) const;
    virtual QPoint mapFromGlobal(const QPoint &pos) const;

    virtual void propagateSizeHints();

    virtual void setOpacity(qreal level);
    virtual void setMask(const QRegion &region);
    virtual void requestActivateWindow();

    virtual void handleContentOrientationChange(Qt::ScreenOrientation orientation);

    virtual qreal devicePixelRatio() const;

    virtual bool setKeyboardGrabEnabled(bool grab);
    virtual bool setMouseGrabEnabled(bool grab);

    virtual bool setWindowModified(bool modified);

    virtual void windowEvent(QEvent *event);

    virtual bool startSystemResize(const QPoint &pos, Qt::Corner corner);

    virtual void setFrameStrutEventsEnabled(bool enabled);
    virtual bool frameStrutEventsEnabled() const;

    virtual void setAlertState(bool enabled);
    virtual bool isAlertState() const;

    virtual void invalidateSurface();

    static QRect initialGeometry(const QWindow *w,
        const QRect &initialGeometry, int defaultWidth, int defaultHeight);

protected:
    static QString formatWindowTitle(const QString &title, const QString &separator);
    QPlatformScreen *screenForGeometry(const QRect &newGeometry) const;

    QScopedPointer<QPlatformWindowPrivate> d_ptr;
private:
    Q_DISABLE_COPY(QPlatformWindow)
};

QT_END_NAMESPACE

#endif //QPLATFORMWINDOW_H
