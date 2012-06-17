/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef QPLATFORMWINDOWFORMAT_QPA_H
#define QPLATFORMWINDOWFORMAT_QPA_H

#include <QtGui/QPlatformWindow>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QPlatformWindowFormatPrivate;

class Q_GUI_EXPORT QPlatformWindowFormat
{
public:
    enum FormatOption {
        DoubleBuffer            = 0x0001,
        DepthBuffer             = 0x0002,
        Rgba                    = 0x0004,
        AlphaChannel            = 0x0008,
        AccumBuffer             = 0x0010,
        StencilBuffer           = 0x0020,
        StereoBuffers           = 0x0040,
        DirectRendering         = 0x0080,
        HasOverlay              = 0x0100,
        SampleBuffers           = 0x0200,
        DeprecatedFunctions     = 0x0400,
        HasWindowSurface        = 0x0800,
        SingleBuffer            = DoubleBuffer    << 16,
        NoDepthBuffer           = DepthBuffer     << 16,
        ColorIndex              = Rgba            << 16,
        NoAlphaChannel          = AlphaChannel    << 16,
        NoAccumBuffer           = AccumBuffer     << 16,
        NoStencilBuffer         = StencilBuffer   << 16,
        NoStereoBuffers         = StereoBuffers   << 16,
        IndirectRendering       = DirectRendering << 16,
        NoOverlay               = HasOverlay      << 16,
        NoSampleBuffers         = SampleBuffers   << 16,
        NoDeprecatedFunctions   = DeprecatedFunctions << 16,
        NoWindowSurface         = HasWindowSurface << 16

    };
    Q_DECLARE_FLAGS(FormatOptions, FormatOption)

    enum WindowApi {
        Raster,
        OpenGL,
        OpenVG
    };

    QPlatformWindowFormat();
    QPlatformWindowFormat(FormatOptions options);
    QPlatformWindowFormat(const QPlatformWindowFormat &other);
    QPlatformWindowFormat &operator=(const QPlatformWindowFormat &other);
    ~QPlatformWindowFormat();

    void setDepthBufferSize(int size);
    int  depthBufferSize() const;

    void setAccumBufferSize(int size);
    int  accumBufferSize() const;

    void setRedBufferSize(int size);
    int  redBufferSize() const;

    void setGreenBufferSize(int size);
    int  greenBufferSize() const;

    void setBlueBufferSize(int size);
    int  blueBufferSize() const;

    void setAlphaBufferSize(int size);
    int  alphaBufferSize() const;

    void setStencilBufferSize(int size);
    int  stencilBufferSize() const;

    void setSampleBuffers(bool enable);
    bool sampleBuffers() const;

    void setSamples(int numSamples);
    int  samples() const;

    void setSwapInterval(int interval);
    int  swapInterval() const;

    void setWindowApi(QPlatformWindowFormat::WindowApi api);
    WindowApi windowApi() const;

    void setSharedContext(QPlatformGLContext *context);
    QPlatformGLContext *sharedGLContext() const;

    bool doubleBuffer() const;
    void setDoubleBuffer(bool enable);
    bool depth() const;
    void setDepth(bool enable);
    bool rgba() const;
    void setRgba(bool enable);
    bool alpha() const;
    void setAlpha(bool enable);
    bool accum() const;
    void setAccum(bool enable);
    bool stencil() const;
    void setStencil(bool enable);
    bool stereo() const;
    void setStereo(bool enable);
    bool directRendering() const;
    void setDirectRendering(bool enable);
    bool hasWindowSurface() const;
    void setWindowSurface(bool enable);

    void setOption(QPlatformWindowFormat::FormatOptions opt);
    bool testOption(QPlatformWindowFormat::FormatOptions opt) const;

    static QPlatformWindowFormat defaultFormat();
    static void setDefaultFormat(const QPlatformWindowFormat& f);

private:
    QPlatformWindowFormatPrivate *d;

    void detach();

    friend Q_GUI_EXPORT bool operator==(const QPlatformWindowFormat&, const QPlatformWindowFormat&);
    friend Q_GUI_EXPORT bool operator!=(const QPlatformWindowFormat&, const QPlatformWindowFormat&);
#ifndef QT_NO_DEBUG_STREAM
    friend Q_GUI_EXPORT QDebug operator<<(QDebug, const QPlatformWindowFormat &);
#endif
};

Q_GUI_EXPORT bool operator==(const QPlatformWindowFormat&, const QPlatformWindowFormat&);
Q_GUI_EXPORT bool operator!=(const QPlatformWindowFormat&, const QPlatformWindowFormat&);

#ifndef QT_NO_DEBUG_STREAM
Q_OPENGL_EXPORT QDebug operator<<(QDebug, const QPlatformWindowFormat &);
#endif

Q_DECLARE_OPERATORS_FOR_FLAGS(QPlatformWindowFormat::FormatOptions)

inline bool QPlatformWindowFormat::doubleBuffer() const
{
    return testOption(QPlatformWindowFormat::DoubleBuffer);
}

inline bool QPlatformWindowFormat::depth() const
{
    return testOption(QPlatformWindowFormat::DepthBuffer);
}

inline bool QPlatformWindowFormat::rgba() const
{
    return testOption(QPlatformWindowFormat::Rgba);
}

inline bool QPlatformWindowFormat::alpha() const
{
    return testOption(QPlatformWindowFormat::AlphaChannel);
}

inline bool QPlatformWindowFormat::accum() const
{
    return testOption(QPlatformWindowFormat::AccumBuffer);
}

inline bool QPlatformWindowFormat::stencil() const
{
    return testOption(QPlatformWindowFormat::StencilBuffer);
}

inline bool QPlatformWindowFormat::stereo() const
{
    return testOption(QPlatformWindowFormat::StereoBuffers);
}

inline bool QPlatformWindowFormat::directRendering() const
{
    return testOption(QPlatformWindowFormat::DirectRendering);
}

inline bool QPlatformWindowFormat::hasWindowSurface() const
{
    return testOption(QPlatformWindowFormat::HasWindowSurface);
}

inline bool QPlatformWindowFormat::sampleBuffers() const
{
    return testOption(QPlatformWindowFormat::SampleBuffers);
}

QT_END_NAMESPACE

QT_END_HEADER

#endif //QPLATFORMWINDOWFORMAT_QPA_H
