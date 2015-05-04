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

#ifndef QEGLCONVENIENCE_H
#define QEGLCONVENIENCE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/QSurfaceFormat>
#include <QtCore/QVector>
#include <QtCore/QSizeF>
#include <EGL/egl.h>

QT_BEGIN_NAMESPACE

QVector<EGLint> q_createConfigAttributesFromFormat(const QSurfaceFormat &format);
bool q_reduceConfigAttributes(QVector<EGLint> *configAttributes);
EGLConfig q_configFromGLFormat(EGLDisplay display, const QSurfaceFormat &format, bool highestPixelFormat = false, int surfaceType = EGL_WINDOW_BIT);
QSurfaceFormat q_glFormatFromConfig(EGLDisplay display, const EGLConfig config, const QSurfaceFormat &referenceFormat = QSurfaceFormat());
bool q_hasEglExtension(EGLDisplay display,const char* extensionName);
void q_printEglConfig(EGLDisplay display, EGLConfig config);

#ifdef Q_OS_UNIX
QSizeF q_physicalScreenSizeFromFb(int framebufferDevice, const QSize &screenSize = QSize());
QSize q_screenSizeFromFb(int framebufferDevice);
int q_screenDepthFromFb(int framebufferDevice);
#endif

class QEglConfigChooser
{
public:
    QEglConfigChooser(EGLDisplay display);
    virtual ~QEglConfigChooser();

    EGLDisplay display() const { return m_display; }

    void setSurfaceType(EGLint surfaceType) { m_surfaceType = surfaceType; }
    EGLint surfaceType() const { return m_surfaceType; }

    void setSurfaceFormat(const QSurfaceFormat &format) { m_format = format; }
    QSurfaceFormat surfaceFormat() const { return m_format; }

    void setIgnoreColorChannels(bool ignore) { m_ignore = ignore; }
    bool ignoreColorChannels() const { return m_ignore; }

    EGLConfig chooseConfig();

protected:
    virtual bool filterConfig(EGLConfig config) const;

private:
    QSurfaceFormat m_format;
    EGLDisplay m_display;
    EGLint m_surfaceType;
    bool m_ignore;

    int m_confAttrRed;
    int m_confAttrGreen;
    int m_confAttrBlue;
    int m_confAttrAlpha;
};


QT_END_NAMESPACE

#endif //QEGLCONVENIENCE_H
