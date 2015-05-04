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

#include "qfontengine_qpf2_p.h"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QBuffer>
#include <QtCore/private/qstringiterator_p.h>

#include <QtGui/private/qpaintengine_raster_p.h>
#include <QtGui/private/qguiapplication_p.h>
#include <qpa/qplatformfontdatabase.h>
#include <qpa/qplatformintegration.h>

QT_BEGIN_NAMESPACE

//#define DEBUG_HEADER
//#define DEBUG_FONTENGINE

static QFontEngineQPF2::TagType tagTypes[QFontEngineQPF2::NumTags] = {
    QFontEngineQPF2::StringType, // FontName
    QFontEngineQPF2::StringType, // FileName
    QFontEngineQPF2::UInt32Type, // FileIndex
    QFontEngineQPF2::UInt32Type, // FontRevision
    QFontEngineQPF2::StringType, // FreeText
    QFontEngineQPF2::FixedType,  // Ascent
    QFontEngineQPF2::FixedType,  // Descent
    QFontEngineQPF2::FixedType,  // Leading
    QFontEngineQPF2::FixedType,  // XHeight
    QFontEngineQPF2::FixedType,  // AverageCharWidth
    QFontEngineQPF2::FixedType,  // MaxCharWidth
    QFontEngineQPF2::FixedType,  // LineThickness
    QFontEngineQPF2::FixedType,  // MinLeftBearing
    QFontEngineQPF2::FixedType,  // MinRightBearing
    QFontEngineQPF2::FixedType,  // UnderlinePosition
    QFontEngineQPF2::UInt8Type,  // GlyphFormat
    QFontEngineQPF2::UInt8Type,  // PixelSize
    QFontEngineQPF2::UInt8Type,  // Weight
    QFontEngineQPF2::UInt8Type,  // Style
    QFontEngineQPF2::StringType, // EndOfHeader
    QFontEngineQPF2::BitFieldType// WritingSystems
};


#if defined(DEBUG_HEADER)
# define DEBUG_VERIFY qDebug
#else
# define DEBUG_VERIFY if (0) qDebug
#endif

#define READ_VERIFY(type, variable) \
    if (tagPtr + sizeof(type) > endPtr) { \
        DEBUG_VERIFY() << "read verify failed in line" << __LINE__; \
        return 0; \
    } \
    variable = qFromBigEndian<type>(tagPtr); \
    DEBUG_VERIFY() << "read value" << variable << "of type " #type; \
    tagPtr += sizeof(type)

template <typename T>
T readValue(const uchar *&data)
{
    T value = qFromBigEndian<T>(data);
    data += sizeof(T);
    return value;
}

#define VERIFY(condition) \
    if (!(condition)) { \
        DEBUG_VERIFY() << "condition " #condition " failed in line" << __LINE__; \
        return 0; \
    }

#define VERIFY_TAG(condition) \
    if (!(condition)) { \
        DEBUG_VERIFY() << "verifying tag condition " #condition " failed in line" << __LINE__ << "with tag" << tag; \
        return 0; \
    }

static inline const uchar *verifyTag(const uchar *tagPtr, const uchar *endPtr)
{
    quint16 tag, length;
    READ_VERIFY(quint16, tag);
    READ_VERIFY(quint16, length);
    if (tag == QFontEngineQPF2::Tag_EndOfHeader)
        return endPtr;
    if (tag < QFontEngineQPF2::NumTags) {
        switch (tagTypes[tag]) {
            case QFontEngineQPF2::BitFieldType:
            case QFontEngineQPF2::StringType:
                // can't do anything...
                break;
            case QFontEngineQPF2::UInt32Type:
                VERIFY_TAG(length == sizeof(quint32));
                break;
            case QFontEngineQPF2::FixedType:
                VERIFY_TAG(length == sizeof(quint32));
                break;
            case QFontEngineQPF2::UInt8Type:
                VERIFY_TAG(length == sizeof(quint8));
                break;
        }
#if defined(DEBUG_HEADER)
        if (length == 1)
            qDebug() << "tag data" << hex << *tagPtr;
        else if (length == 4)
            qDebug() << "tag data" << hex << tagPtr[0] << tagPtr[1] << tagPtr[2] << tagPtr[3];
#endif
    }
    return tagPtr + length;
}

const QFontEngineQPF2::Glyph *QFontEngineQPF2::findGlyph(glyph_t g) const
{
    if (!g || g >= glyphMapEntries)
        return 0;
    const quint32 *gmapPtr = reinterpret_cast<const quint32 *>(fontData + glyphMapOffset);
    quint32 glyphPos = qFromBigEndian<quint32>(gmapPtr[g]);
    if (glyphPos > glyphDataSize) {
        if (glyphPos == 0xffffffff)
            return 0;
#if defined(DEBUG_FONTENGINE)
        qDebug() << "glyph" << g << "outside of glyphData, remapping font file";
#endif
        if (glyphPos > glyphDataSize)
            return 0;
    }
    return reinterpret_cast<const Glyph *>(fontData + glyphDataOffset + glyphPos);
}

bool QFontEngineQPF2::verifyHeader(const uchar *data, int size)
{
    VERIFY(quintptr(data) % Q_ALIGNOF(Header) == 0);
    VERIFY(size >= int(sizeof(Header)));
    const Header *header = reinterpret_cast<const Header *>(data);
    if (header->magic[0] != 'Q'
        || header->magic[1] != 'P'
        || header->magic[2] != 'F'
        || header->magic[3] != '2')
        return false;

    VERIFY(header->majorVersion <= CurrentMajorVersion);
    const quint16 dataSize = qFromBigEndian<quint16>(header->dataSize);
    VERIFY(size >= int(sizeof(Header)) + dataSize);

    const uchar *tagPtr = data + sizeof(Header);
    const uchar *tagEndPtr = tagPtr + dataSize;
    while (tagPtr < tagEndPtr - 3) {
        tagPtr = verifyTag(tagPtr, tagEndPtr);
        VERIFY(tagPtr);
    }

    VERIFY(tagPtr <= tagEndPtr);
    return true;
}

QVariant QFontEngineQPF2::extractHeaderField(const uchar *data, HeaderTag requestedTag)
{
    const Header *header = reinterpret_cast<const Header *>(data);
    const uchar *tagPtr = data + sizeof(Header);
    const uchar *endPtr = tagPtr + qFromBigEndian<quint16>(header->dataSize);
    while (tagPtr < endPtr - 3) {
        quint16 tag = readValue<quint16>(tagPtr);
        quint16 length = readValue<quint16>(tagPtr);
        if (tag == requestedTag) {
            switch (tagTypes[requestedTag]) {
                case StringType:
                    return QVariant(QString::fromUtf8(reinterpret_cast<const char *>(tagPtr), length));
                case UInt32Type:
                    return QVariant(readValue<quint32>(tagPtr));
                case UInt8Type:
                    return QVariant(uint(*tagPtr));
                case FixedType:
                    return QVariant(QFixed::fromFixed(readValue<quint32>(tagPtr)).toReal());
                case BitFieldType:
                    return QVariant(QByteArray(reinterpret_cast<const char *>(tagPtr), length));
            }
            return QVariant();
        } else if (tag == Tag_EndOfHeader) {
            break;
        }
        tagPtr += length;
    }

    return QVariant();
}


QFontEngineQPF2::QFontEngineQPF2(const QFontDef &def, const QByteArray &data)
    : QFontEngine(QPF2),
      fontData(reinterpret_cast<const uchar *>(data.constData())), dataSize(data.size())
{
    fontDef = def;
    cache_cost = 100;
    cmap = 0;
    cmapOffset = 0;
    cmapSize = 0;
    glyphMapOffset = 0;
    glyphMapEntries = 0;
    glyphDataOffset = 0;
    glyphDataSize = 0;
    kerning_pairs_loaded = false;
    readOnly = true;

#if defined(DEBUG_FONTENGINE)
    qDebug() << "QFontEngineQPF2::QFontEngineQPF2( fd =" << fd << ", renderingFontEngine =" << renderingFontEngine << ')';
#endif

    if (!verifyHeader(fontData, dataSize)) {
#if defined(DEBUG_FONTENGINE)
        qDebug() << "verifyHeader failed!";
#endif
        return;
    }

    const Header *header = reinterpret_cast<const Header *>(fontData);

    readOnly = (header->lock == 0xffffffff);

    const uchar *imgData = fontData + sizeof(Header) + qFromBigEndian<quint16>(header->dataSize);
    const uchar *endPtr = fontData + dataSize;
    while (imgData <= endPtr - 8) {
        quint16 blockTag = readValue<quint16>(imgData);
        imgData += 2; // skip padding
        quint32 blockSize = readValue<quint32>(imgData);

        if (blockTag == CMapBlock) {
            cmapOffset = imgData - fontData;
            cmapSize = blockSize;
        } else if (blockTag == GMapBlock) {
            glyphMapOffset = imgData - fontData;
            glyphMapEntries = blockSize / 4;
        } else if (blockTag == GlyphBlock) {
            glyphDataOffset = imgData - fontData;
            glyphDataSize = blockSize;
        }

        imgData += blockSize;
    }

    face_id.filename = QFile::encodeName(extractHeaderField(fontData, Tag_FileName).toString());
    face_id.index = extractHeaderField(fontData, Tag_FileIndex).toInt();

    // get the real cmap
    if (cmapOffset) {
        cmap = QFontEngine::getCMap(fontData + cmapOffset, cmapSize, &symbol, &cmapSize);
        cmapOffset = cmap ? cmap - fontData : 0;
    }

    // verify all the positions in the glyphMap
    if (glyphMapOffset) {
        const quint32 *gmapPtr = reinterpret_cast<const quint32 *>(fontData + glyphMapOffset);
        for (uint i = 0; i < glyphMapEntries; ++i) {
            quint32 glyphDataPos = qFromBigEndian<quint32>(gmapPtr[i]);
            if (glyphDataPos == 0xffffffff)
                continue;
            if (glyphDataPos >= glyphDataSize) {
                // error
                glyphMapOffset = 0;
                glyphMapEntries = 0;
                break;
            }
        }
    }

#if defined(DEBUG_FONTENGINE)
    if (!isValid())
        qDebug() << "fontData" <<  fontData << "dataSize" << dataSize
                 << "cmap" << cmap << "cmapOffset" << cmapOffset
                 << "glyphMapOffset" << glyphMapOffset << "glyphDataOffset" << glyphDataOffset
                 << "fd" << fd << "glyphDataSize" << glyphDataSize;
#endif
}

QFontEngineQPF2::~QFontEngineQPF2()
{
}

bool QFontEngineQPF2::getSfntTableData(uint tag, uchar *buffer, uint *length) const
{
    if (tag != MAKE_TAG('c', 'm', 'a', 'p') || !cmap)
        return false;

    if (buffer && int(*length) >= cmapSize)
        memcpy(buffer, cmap, cmapSize);
    *length = cmapSize;
    Q_ASSERT(int(*length) > 0);
    return true;
}

glyph_t QFontEngineQPF2::glyphIndex(uint ucs4) const
{
    glyph_t glyph = getTrueTypeGlyphIndex(cmap, ucs4);
    if (glyph == 0 && symbol && ucs4 < 0x100)
        glyph = getTrueTypeGlyphIndex(cmap, ucs4 + 0xf000);
    if (!findGlyph(glyph))
        glyph = 0;

    return glyph;
}

bool QFontEngineQPF2::stringToCMap(const QChar *str, int len, QGlyphLayout *glyphs, int *nglyphs, QFontEngine::ShaperFlags flags) const
{
    Q_ASSERT(glyphs->numGlyphs >= *nglyphs);
    if (*nglyphs < len) {
        *nglyphs = len;
        return false;
    }

#if defined(DEBUG_FONTENGINE)
    QSet<QChar> seenGlyphs;
#endif

    int glyph_pos = 0;
    if (symbol) {
        QStringIterator it(str, str + len);
        while (it.hasNext()) {
            const uint uc = it.next();
            glyphs->glyphs[glyph_pos] = getTrueTypeGlyphIndex(cmap, uc);
            if(!glyphs->glyphs[glyph_pos] && uc < 0x100)
                glyphs->glyphs[glyph_pos] = getTrueTypeGlyphIndex(cmap, uc + 0xf000);
            ++glyph_pos;
        }
    } else {
        QStringIterator it(str, str + len);
        while (it.hasNext()) {
            const uint uc = it.next();
            glyphs->glyphs[glyph_pos] = getTrueTypeGlyphIndex(cmap, uc);
#if 0 && defined(DEBUG_FONTENGINE)
            QChar c(uc);
            if (!findGlyph(glyphs[glyph_pos].glyph) && !seenGlyphs.contains(c))
                qDebug() << "glyph for character" << c << '/' << hex << uc << "is" << dec << glyphs[glyph_pos].glyph;

            seenGlyphs.insert(c);
#endif
            ++glyph_pos;
        }
    }

    *nglyphs = glyph_pos;
    glyphs->numGlyphs = glyph_pos;

    if (!(flags & GlyphIndicesOnly))
        recalcAdvances(glyphs, flags);

    return true;
}

void QFontEngineQPF2::recalcAdvances(QGlyphLayout *glyphs, QFontEngine::ShaperFlags) const
{
    for (int i = 0; i < glyphs->numGlyphs; ++i) {
        const Glyph *g = findGlyph(glyphs->glyphs[i]);
        if (!g)
            continue;
        glyphs->advances[i] = g->advance;
    }
}

QImage QFontEngineQPF2::alphaMapForGlyph(glyph_t g)
{
    const Glyph *glyph = findGlyph(g);
    if (!glyph)
        return QImage();

    const uchar *bits = ((const uchar *) glyph) + sizeof(Glyph);

    QImage image(bits,glyph->width, glyph->height, glyph->bytesPerLine, QImage::Format_Indexed8);

    return image;
}

void QFontEngineQPF2::addOutlineToPath(qreal x, qreal y, const QGlyphLayout &glyphs, QPainterPath *path, QTextItem::RenderFlags flags)
{
    addBitmapFontToPath(x, y, glyphs, path, flags);
}

glyph_metrics_t QFontEngineQPF2::boundingBox(const QGlyphLayout &glyphs)
{
    glyph_metrics_t overall;
    // initialize with line height, we get the same behaviour on all platforms
    overall.y = -ascent();
    overall.height = ascent() + descent() + 1;

    QFixed ymax = 0;
    QFixed xmax = 0;
    for (int i = 0; i < glyphs.numGlyphs; i++) {
        const Glyph *g = findGlyph(glyphs.glyphs[i]);
        if (!g)
            continue;

        QFixed x = overall.xoff + glyphs.offsets[i].x + g->x;
        QFixed y = overall.yoff + glyphs.offsets[i].y + g->y;
        overall.x = qMin(overall.x, x);
        overall.y = qMin(overall.y, y);
        xmax = qMax(xmax, x + g->width);
        ymax = qMax(ymax, y + g->height);
        overall.xoff += g->advance;
    }
    overall.height = qMax(overall.height, ymax - overall.y);
    overall.width = xmax - overall.x;

    return overall;
}

glyph_metrics_t QFontEngineQPF2::boundingBox(glyph_t glyph)
{
    glyph_metrics_t overall;
    const Glyph *g = findGlyph(glyph);
    if (!g)
        return overall;
    overall.x = g->x;
    overall.y = g->y;
    overall.width = g->width;
    overall.height = g->height;
    overall.xoff = g->advance;
    return overall;
}

QFixed QFontEngineQPF2::ascent() const
{
    return QFixed::fromReal(extractHeaderField(fontData, Tag_Ascent).value<qreal>());
}

QFixed QFontEngineQPF2::descent() const
{
    return QFixed::fromReal(extractHeaderField(fontData, Tag_Descent).value<qreal>());
}

QFixed QFontEngineQPF2::leading() const
{
    return QFixed::fromReal(extractHeaderField(fontData, Tag_Leading).value<qreal>());
}

qreal QFontEngineQPF2::maxCharWidth() const
{
    return extractHeaderField(fontData, Tag_MaxCharWidth).value<qreal>();
}

qreal QFontEngineQPF2::minLeftBearing() const
{
    return extractHeaderField(fontData, Tag_MinLeftBearing).value<qreal>();
}

qreal QFontEngineQPF2::minRightBearing() const
{
    return extractHeaderField(fontData, Tag_MinRightBearing).value<qreal>();
}

QFixed QFontEngineQPF2::underlinePosition() const
{
    return QFixed::fromReal(extractHeaderField(fontData, Tag_UnderlinePosition).value<qreal>());
}

QFixed QFontEngineQPF2::lineThickness() const
{
    return QFixed::fromReal(extractHeaderField(fontData, Tag_LineThickness).value<qreal>());
}

bool QFontEngineQPF2::isValid() const
{
    return fontData && dataSize && cmapOffset
           && glyphMapOffset && glyphDataOffset && glyphDataSize > 0;
}

void QPF2Generator::generate()
{
    writeHeader();
    writeGMap();
    writeBlock(QFontEngineQPF2::GlyphBlock, QByteArray());

    dev->seek(4); // position of header.lock
    writeUInt32(0);
}

void QPF2Generator::writeHeader()
{
    QFontEngineQPF2::Header header;

    header.magic[0] = 'Q';
    header.magic[1] = 'P';
    header.magic[2] = 'F';
    header.magic[3] = '2';
    header.lock = 1;
    header.majorVersion = QFontEngineQPF2::CurrentMajorVersion;
    header.minorVersion = QFontEngineQPF2::CurrentMinorVersion;
    header.dataSize = 0;
    dev->write((const char *)&header, sizeof(header));

    writeTaggedString(QFontEngineQPF2::Tag_FontName, fe->fontDef.family.toUtf8());

    QFontEngine::FaceId face = fe->faceId();
    writeTaggedString(QFontEngineQPF2::Tag_FileName, face.filename);
    writeTaggedUInt32(QFontEngineQPF2::Tag_FileIndex, face.index);

    {
        const QByteArray head = fe->getSfntTable(MAKE_TAG('h', 'e', 'a', 'd'));
        if (head.size() >= 4) {
            const quint32 revision = qFromBigEndian<quint32>(reinterpret_cast<const uchar *>(head.constData()));
            writeTaggedUInt32(QFontEngineQPF2::Tag_FontRevision, revision);
        }
    }

    writeTaggedQFixed(QFontEngineQPF2::Tag_Ascent, fe->ascent());
    writeTaggedQFixed(QFontEngineQPF2::Tag_Descent, fe->descent());
    writeTaggedQFixed(QFontEngineQPF2::Tag_Leading, fe->leading());
    writeTaggedQFixed(QFontEngineQPF2::Tag_XHeight, fe->xHeight());
    writeTaggedQFixed(QFontEngineQPF2::Tag_AverageCharWidth, fe->averageCharWidth());
    writeTaggedQFixed(QFontEngineQPF2::Tag_MaxCharWidth, QFixed::fromReal(fe->maxCharWidth()));
    writeTaggedQFixed(QFontEngineQPF2::Tag_LineThickness, fe->lineThickness());
    writeTaggedQFixed(QFontEngineQPF2::Tag_MinLeftBearing, QFixed::fromReal(fe->minLeftBearing()));
    writeTaggedQFixed(QFontEngineQPF2::Tag_MinRightBearing, QFixed::fromReal(fe->minRightBearing()));
    writeTaggedQFixed(QFontEngineQPF2::Tag_UnderlinePosition, fe->underlinePosition());
    writeTaggedUInt8(QFontEngineQPF2::Tag_PixelSize, fe->fontDef.pixelSize);
    writeTaggedUInt8(QFontEngineQPF2::Tag_Weight, fe->fontDef.weight);
    writeTaggedUInt8(QFontEngineQPF2::Tag_Style, fe->fontDef.style);

    writeTaggedUInt8(QFontEngineQPF2::Tag_GlyphFormat, QFontEngineQPF2::AlphamapGlyphs);

    writeTaggedString(QFontEngineQPF2::Tag_EndOfHeader, QByteArray());
    align4();

    const quint64 size = dev->pos();
    header.dataSize = qToBigEndian<quint16>(size - sizeof(header));
    dev->seek(0);
    dev->write((const char *)&header, sizeof(header));
    dev->seek(size);
}

void QPF2Generator::writeGMap()
{
    const quint16 glyphCount = fe->glyphCount();

    writeUInt16(QFontEngineQPF2::GMapBlock);
    writeUInt16(0); // padding
    writeUInt32(glyphCount * 4);

    QByteArray &buffer = dev->buffer();
    const int numBytes = glyphCount * sizeof(quint32);
    qint64 pos = buffer.size();
    buffer.resize(pos + numBytes);
    memset(buffer.data() + pos, 0xff, numBytes);
    dev->seek(pos + numBytes);
}

void QPF2Generator::writeBlock(QFontEngineQPF2::BlockTag tag, const QByteArray &data)
{
    writeUInt16(tag);
    writeUInt16(0); // padding
    const int padSize = ((data.size() + 3) / 4) * 4 - data.size();
    writeUInt32(data.size() + padSize);
    dev->write(data);
    for (int i = 0; i < padSize; ++i)
        writeUInt8(0);
}

void QPF2Generator::writeTaggedString(QFontEngineQPF2::HeaderTag tag, const QByteArray &string)
{
    writeUInt16(tag);
    writeUInt16(string.length());
    dev->write(string);
}

void QPF2Generator::writeTaggedUInt32(QFontEngineQPF2::HeaderTag tag, quint32 value)
{
    writeUInt16(tag);
    writeUInt16(sizeof(value));
    writeUInt32(value);
}

void QPF2Generator::writeTaggedUInt8(QFontEngineQPF2::HeaderTag tag, quint8 value)
{
    writeUInt16(tag);
    writeUInt16(sizeof(value));
    writeUInt8(value);
}

void QPF2Generator::writeTaggedQFixed(QFontEngineQPF2::HeaderTag tag, QFixed value)
{
    writeUInt16(tag);
    writeUInt16(sizeof(quint32));
    writeUInt32(value.value());
}

QT_END_NAMESPACE
