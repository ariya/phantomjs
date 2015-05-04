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

#include  "qwindowsscaling.h"
#include  "qwindowsscreen.h"

#include <QtCore/QDebug>
#include <QtCore/QCoreApplication>

QT_BEGIN_NAMESPACE

/*!
    \class QWindowsScaling
    \brief Windows scaling utilities

    \internal
    \ingroup qt-lighthouse-win
*/

int QWindowsScaling::m_factor = 1;

static const char devicePixelRatioEnvVar[] = "QT_DEVICE_PIXEL_RATIO";

// Suggest a scale factor by checking monitor sizes.
int QWindowsScaling::determineUiScaleFactor()
{
    if (!qEnvironmentVariableIsSet(devicePixelRatioEnvVar))
        return 1;
    const QByteArray envDevicePixelRatioEnv = qgetenv(devicePixelRatioEnvVar);
    // Auto: Suggest a scale factor by checking monitor resolution.
    if (envDevicePixelRatioEnv == "auto") {
        const int maxResolution = QWindowsScreen::maxMonitorHorizResolution();
        return maxResolution > 180 ? maxResolution / 96 : 1;
    }
    // Get factor from environment
    bool ok = false;
    const int envFactor = envDevicePixelRatioEnv.toInt(&ok);
    return ok && envFactor > 0 ? envFactor : 1;
}

QT_END_NAMESPACE
