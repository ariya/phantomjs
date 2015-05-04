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

#ifndef QBLITTABLE_P_H
#define QBLITTABLE_P_H

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

#include <QtCore/qsize.h>
#include <QtGui/private/qpixmap_blitter_p.h>


#ifndef QT_NO_BLITTABLE
QT_BEGIN_NAMESPACE

class QImage;
class QBlittablePrivate;

class Q_GUI_EXPORT QBlittable
{
    Q_DECLARE_PRIVATE(QBlittable);
public:
    enum Capability {

        SolidRectCapability             = 0x0001,
        SourcePixmapCapability          = 0x0002,
        SourceOverPixmapCapability      = 0x0004,
        SourceOverScaledPixmapCapability = 0x0008,
        AlphaFillRectCapability         = 0x0010,
        OpacityPixmapCapability         = 0x0020,
        DrawScaledCachedGlyphsCapability = 0x0040,
        SubPixelGlyphsCapability         = 0x0080,
        ComplexClipCapability            = 0x0100,

        // Internal ones
        OutlineCapability               = 0x0001000
    };
    Q_DECLARE_FLAGS (Capabilities, Capability)

    QBlittable(const QSize &size, Capabilities caps);
    virtual ~QBlittable();

    Capabilities capabilities() const;
    QSize size() const;

    virtual void fillRect(const QRectF &rect, const QColor &color) = 0;
    virtual void drawPixmap(const QRectF &rect, const QPixmap &pixmap, const QRectF &subrect) = 0;
    virtual void alphaFillRect(const QRectF &rect, const QColor &color, QPainter::CompositionMode cmode) {
        Q_UNUSED(rect);
        Q_UNUSED(color);
        Q_UNUSED(cmode);
        qWarning("Please implement alphaFillRect function in your platform or remove AlphaFillRectCapability from it");
    }
    virtual void drawPixmapOpacity(const QRectF &rect, const QPixmap &pixmap, const QRectF &subrect, QPainter::CompositionMode cmode, qreal opacity) {
        Q_UNUSED(rect);
        Q_UNUSED(pixmap);
        Q_UNUSED(subrect);
        Q_UNUSED(cmode);
        Q_UNUSED(opacity);
        qWarning("Please implement drawPixmapOpacity function in your platform or remove OpacityPixmapCapability from it");
    }
    virtual bool drawCachedGlyphs(const QPaintEngineState *state, QFontEngine::GlyphFormat glyphFormat, int numGlyphs, const glyph_t *glyphs, const QFixedPoint *positions, QFontEngine *fontEngine) {
        Q_UNUSED(state);
        Q_UNUSED(glyphFormat);
        Q_UNUSED(numGlyphs);
        Q_UNUSED(glyphs);
        Q_UNUSED(positions);
        Q_UNUSED(fontEngine);
        qWarning("Please implement drawCachedGlyphs function in your platform or remove DrawCachedGlyphsCapability from it");
        return true;
    }


    QImage *lock();
    void unlock();

    bool isLocked() const;

protected:
    virtual QImage *doLock() = 0;
    virtual void doUnlock() = 0;
    QBlittablePrivate *d_ptr;
};

QT_END_NAMESPACE
#endif //QT_NO_BLITTABLE
#endif //QBLITTABLE_P_H
