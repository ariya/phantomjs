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

#ifndef QPLATFORMINTEGRATION_DIRECTFB_H
#define QPLATFORMINTEGRATION_DIRECTFB_H

#include "qdirectfbinput.h"
#include "qdirectfbscreen.h"

#include <qpa/qplatformintegration.h>
#include <qpa/qplatformnativeinterface.h>
#include <directfb.h>
#include <directfb_version.h>

QT_BEGIN_NAMESPACE

class QThread;
class QAbstractEventDispatcher;

class QDirectFbIntegration : public QPlatformIntegration, public QPlatformNativeInterface
{
public:
    QDirectFbIntegration();
    ~QDirectFbIntegration();

    void connectToDirectFb();

    bool hasCapability(Capability cap) const;
    QPlatformPixmap *createPlatformPixmap(QPlatformPixmap::PixelType type) const;
    QPlatformWindow *createPlatformWindow(QWindow *window) const;
    QPlatformBackingStore *createPlatformBackingStore(QWindow *window) const;
    QAbstractEventDispatcher *createEventDispatcher() const;

    QPlatformFontDatabase *fontDatabase() const;
    QPlatformServices *services() const;
    QPlatformInputContext *inputContext() const { return m_inputContext; }
    QPlatformNativeInterface *nativeInterface() const;

protected:
    virtual void initializeDirectFB();
    virtual void initializeScreen();
    virtual void initializeInput();

protected:
    QDirectFBPointer<IDirectFB> m_dfb;
    QScopedPointer<QDirectFbScreen> m_primaryScreen;
    QScopedPointer<QDirectFbInput> m_input;
    QScopedPointer<QThread> m_inputRunner;
    QScopedPointer<QPlatformFontDatabase> m_fontDb;
    QScopedPointer<QPlatformServices> m_services;
    QPlatformInputContext *m_inputContext;
};

QT_END_NAMESPACE

#endif
