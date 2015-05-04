/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QIOSSCREEN_H
#define QIOSSCREEN_H

#include <UIKit/UIKit.h>

#include <qpa/qplatformscreen.h>

@class QIOSOrientationListener;

QT_BEGIN_NAMESPACE

class QIOSScreen : public QObject, public QPlatformScreen
{
    Q_OBJECT

public:
    QIOSScreen(UIScreen *screen);
    ~QIOSScreen();

    QRect geometry() const;
    QRect availableGeometry() const;
    int depth() const;
    QImage::Format format() const;
    QSizeF physicalSize() const;
    QDpi logicalDpi() const;
    qreal devicePixelRatio() const;

    Qt::ScreenOrientation nativeOrientation() const;
    Qt::ScreenOrientation orientation() const;
    void setOrientationUpdateMask(Qt::ScreenOrientations mask);

    UIScreen *uiScreen() const;
    UIWindow *uiWindow() const;

    void updateProperties();

private:
    UIScreen *m_uiScreen;
    UIWindow *m_uiWindow;
    QRect m_geometry;
    QRect m_availableGeometry;
    int m_depth;
    uint m_unscaledDpi;
    QSizeF m_physicalSize;
    QIOSOrientationListener *m_orientationListener;
};

QT_END_NAMESPACE

#endif
