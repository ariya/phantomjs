/****************************************************************************
**
** Copyright (C) 2015 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QWINDOWSOPENGLTESTER_H
#define QWINDOWSOPENGLTESTER_H

#include <qtwindowsglobal.h>

#include <QtCore/QByteArray>
#include <QtCore/QFlags>

QT_BEGIN_NAMESPACE

class QDebug;
class QVariant;

struct GpuDriverVersion // ### fixme: Use QVersionNumber in Qt 5.5?
{
    GpuDriverVersion(int p = 0, int v = 0, int sv =0, int b = 0) : product(p), version(v), subVersion(sv), build(b) {}
    QString toString() const;
    int compare(const GpuDriverVersion &rhs) const;

    int product;
    int version;
    int subVersion;
    int build;
};

inline bool operator==(const GpuDriverVersion &v1, const GpuDriverVersion &v2)
    { return !v1.compare(v2); }
inline bool operator!=(const GpuDriverVersion &v1, const GpuDriverVersion &v2)
    { return v1.compare(v2); }
inline bool operator< (const GpuDriverVersion &v1, const GpuDriverVersion &v2)
    { return v1.compare(v2) < 0; }
inline bool operator<=(const GpuDriverVersion &v1, const GpuDriverVersion &v2)
    { return v1.compare(v2) <= 0; }
inline bool operator> (const GpuDriverVersion &v1, const GpuDriverVersion &v2)
    { return v1.compare(v2) > 0; }
inline bool operator>=(const GpuDriverVersion &v1, const GpuDriverVersion &v2)
    { return v1.compare(v2) >= 0; }

QDebug operator<<(QDebug d, const GpuDriverVersion &gd);

struct GpuDescription
{
    GpuDescription() :  vendorId(0), deviceId(0), revision(0), subSysId(0) {}

    static GpuDescription detect();
    QString toString() const;
    QVariant toVariant() const;

    int vendorId;
    int deviceId;
    int revision;
    int subSysId;
    GpuDriverVersion driverVersion;
    QByteArray driverName;
    QByteArray description;
};

QDebug operator<<(QDebug d, const GpuDescription &gd);

class QWindowsOpenGLTester
{
public:
    enum Renderer {
        InvalidRenderer         = 0x0000,
        DesktopGl               = 0x0001,
        AngleRendererD3d11      = 0x0002,
        AngleRendererD3d9       = 0x0004,
        AngleRendererD3d11Warp  = 0x0008, // "Windows Advanced Rasterization Platform"
        AngleBackendMask        = AngleRendererD3d11 | AngleRendererD3d9 | AngleRendererD3d11Warp,
        Gles                    = 0x0010, // ANGLE/unspecified or Generic GLES for Windows CE.
        GlesMask                = Gles | AngleBackendMask,
        SoftwareRasterizer      = 0x0020
    };
    Q_DECLARE_FLAGS(Renderers, Renderer)

    static Renderer requestedGlesRenderer();
    static Renderer requestedRenderer();
    static Renderers supportedGlesRenderers();
    static Renderers supportedRenderers();

    static bool testDesktopGL();
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QWindowsOpenGLTester::Renderers)

QT_END_NAMESPACE

#endif // QWINDOWSOPENGLTESTER_H
