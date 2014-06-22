/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
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

#ifndef QDISTANCEFIELD_H
#define QDISTANCEFIELD_H

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

#include <qrawfont.h>
#include <private/qfontengine_p.h>
#include <QtCore/qshareddata.h>

QT_BEGIN_NAMESPACE

#define QT_DISTANCEFIELD_DEFAULT_BASEFONTSIZE 54
#define QT_DISTANCEFIELD_DEFAULT_TILESIZE 64
#define QT_DISTANCEFIELD_DEFAULT_SCALE 16
#define QT_DISTANCEFIELD_DEFAULT_RADIUS 80
#define QT_DISTANCEFIELD_HIGHGLYPHCOUNT 2000

#define QT_DISTANCEFIELD_BASEFONTSIZE(NarrowOutlineFont) \
    (NarrowOutlineFont ? QT_DISTANCEFIELD_DEFAULT_BASEFONTSIZE * 2 : \
                         QT_DISTANCEFIELD_DEFAULT_BASEFONTSIZE)
#define QT_DISTANCEFIELD_TILESIZE(NarrowOutlineFont) \
    (NarrowOutlineFont ? QT_DISTANCEFIELD_DEFAULT_TILESIZE * 2 : \
                         QT_DISTANCEFIELD_DEFAULT_TILESIZE)
#define QT_DISTANCEFIELD_SCALE(NarrowOutlineFont) \
    (NarrowOutlineFont ? QT_DISTANCEFIELD_DEFAULT_SCALE / 2 : \
                         QT_DISTANCEFIELD_DEFAULT_SCALE)
#define QT_DISTANCEFIELD_RADIUS(NarrowOutlineFont) \
    (NarrowOutlineFont ? QT_DISTANCEFIELD_DEFAULT_RADIUS / 2 : \
                         QT_DISTANCEFIELD_DEFAULT_RADIUS)

bool Q_GUI_EXPORT qt_fontHasNarrowOutlines(const QRawFont &f);
bool Q_GUI_EXPORT qt_fontHasNarrowOutlines(QFontEngine *fontEngine);

class Q_GUI_EXPORT QDistanceFieldData : public QSharedData
{
public:
    QDistanceFieldData() : glyph(0), width(0), height(0), nbytes(0), data(0) {}
    QDistanceFieldData(const QDistanceFieldData &other);
    ~QDistanceFieldData();

    static QDistanceFieldData *create(const QSize &size);
    static QDistanceFieldData *create(const QPainterPath &path, bool doubleResolution);

    glyph_t glyph;
    int width;
    int height;
    int nbytes;
    uchar *data;
};

class Q_GUI_EXPORT QDistanceField
{
public:
    QDistanceField();
    QDistanceField(int width, int height);
    QDistanceField(const QRawFont &font, glyph_t glyph, bool doubleResolution = false);
    QDistanceField(QFontEngine *fontEngine, glyph_t glyph, bool doubleResolution = false);
    QDistanceField(const QDistanceField &other);

    bool isNull() const;

    glyph_t glyph() const;
    void setGlyph(const QRawFont &font, glyph_t glyph, bool doubleResolution = false);
    void setGlyph(QFontEngine *fontEngine, glyph_t glyph, bool doubleResolution = false);

    int width() const;
    int height() const;

    QDistanceField copy(const QRect &rect = QRect()) const;
    inline QDistanceField copy(int x, int y, int w, int h) const
        { return copy(QRect(x, y, w, h)); }

    uchar *bits();
    const uchar *bits() const;
    const uchar *constBits() const;

    uchar *scanLine(int);
    const uchar *scanLine(int) const;
    const uchar *constScanLine(int) const;

    QImage toImage(QImage::Format format = QImage::Format_ARGB32_Premultiplied) const;

private:
    QDistanceField(QDistanceFieldData *data);
    QSharedDataPointer<QDistanceFieldData> d;

    friend class QDistanceFieldData;
};

QT_END_NAMESPACE

#endif // QDISTANCEFIELD_H
