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

#ifndef QIOSWINDOW_H
#define QIOSWINDOW_H

#include <qpa/qplatformwindow.h>
#include <qpa/qwindowsysteminterface.h>

#import <UIKit/UIKit.h>

class QIOSContext;
class QIOSWindow;

QT_BEGIN_NAMESPACE

@class QUIView;

class QIOSWindow : public QObject, public QPlatformWindow
{
    Q_OBJECT

public:
    explicit QIOSWindow(QWindow *window);
    ~QIOSWindow();

    void setGeometry(const QRect &rect);

    void setWindowState(Qt::WindowState state);
    void setParent(const QPlatformWindow *window);
    void handleContentOrientationChange(Qt::ScreenOrientation orientation);
    void setVisible(bool visible);
    void setOpacity(qreal level) Q_DECL_OVERRIDE;

    bool isExposed() const Q_DECL_OVERRIDE;
    void propagateSizeHints() Q_DECL_OVERRIDE {}

    void raise() { raiseOrLower(true); }
    void lower() { raiseOrLower(false); }

    bool shouldAutoActivateWindow() const;
    void requestActivateWindow();

    qreal devicePixelRatio() const;

    bool setMouseGrabEnabled(bool grab) { return grab; }
    bool setKeyboardGrabEnabled(bool grab) { return grab; }

    WId winId() const { return WId(m_view); };

    void clearAccessibleCache();

private:
    void applicationStateChanged(Qt::ApplicationState state);
    void applyGeometry(const QRect &rect);

    QUIView *m_view;

    QRect m_normalGeometry;
    int m_windowLevel;

    void raiseOrLower(bool raise);
    void updateWindowLevel();
    bool blockedByModal();

    friend class QIOSScreen;
};

QT_END_NAMESPACE

#endif // QIOSWINDOW_H
