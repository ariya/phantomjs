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

#include "qfontengine_qpa_p.h"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QBuffer>

#include <QtGui/private/qapplication_p.h>
#include <QtGui/QPlatformFontDatabase>
#include <QtGui/private/qpaintengine_raster_p.h>

QT_BEGIN_NAMESPACE

//#define DEBUG_HEADER
//#define DEBUG_FONTENGINE

static QFontEngineQPA::TagType tagTypes[QFontEngineQPA::NumTags] = {
    QFontEngineQPA::StringType, // FontName
    QFontEngineQPA::StringType, // FileName
    QFontEngineQPA::UInt32Type, // FileIndex
    QFontEngineQPA::UInt32Type, // FontRevision
    QFontEngineQPA::StringType, // FreeText
    QFontEngineQPA::FixedType,  // Ascent
    QFontEngineQPA::FixedType,  // Descent
    QFontEngineQPA::FixedType,  // Leading
    QFontEngineQPA::FixedType,  // XHeight
    QFontEngineQPA::FixedType,  // AverageCharWidth
    QFontEngineQPA::FixedType,  // MaxCharWidth
    QFontEngineQPA::FixedType,  // LineThickness
    QFontEngineQPA::FixedType,  // MinLeftBearing
    QFontEngineQPA::FixedType,  // MinRightBearing
    QFontEngineQPA::FixedType,  // UnderlinePosition
    QFontEngineQPA::UInt8Type,  // GlyphFormat
    QFontEngineQPA::UInt8Type,  // PixelSize
    QFontEngineQPA::UInt8Type,  // Weight
    QFontEngineQPA::UInt8Type,  // Style
    QFontEngineQPA::StringType, // EndOfHeader
    QFontEngineQPA::BitFieldType// WritingSystems
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
    if (tag == QFontEngineQPA::Tag_EndOfHeader)
        return endPtr;
    if (tag < QFontEngineQPA::NumTags) {
        switch (tagTypes[tag]) {
            case QFontEngineQPA::BitFieldType:
            case QFontEngineQPA::StringType:
                // can't do anything...
                break;
            case QFontEngineQPA::UInt32Type:
                VERIFY_TAG(length == sizeof(quint32));
                break;
            case QFontEngineQPA::FixedType:
                VERIFY_TAG(length == sizeof(quint32));
                break;
            case QFontEngineQPA::UInt8Type:
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

const QFontEngineQPA::Glyph *QFontEngineQPA::findGlyph(glyph_t g) const
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

bool QFontEngineQPA::verifyHeader(const uchar *data, int size)
{
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

QVariant QFontEngineQPA::extractHeaderField(const uchar *data, HeaderTag requestedTag)
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



static inline unsigned int getChar(const QChar *str, int &i, const int len)
{
    uint ucs4 = str[i].unicode();
    if (str[i].isHighSurrogate() && i < len-1 && str[i+1].isLowSurrogate()) {
        ++i;
        ucs4 = QChar::surrogateToUcs4(ucs4, str[i].unicode());
    }
    return ucs4;
}

QFontEngineQPA::QFontEngineQPA(const QFontDef &def, const QByteArray &data)
    : fontData(reinterpret_cast<const uchar *>(data.constData())), dataSize(data.size())
{
    fontDef = def;
    cache_cost = 100;
    externalCMap = 0;
    cmapOffset = 0;
    cmapSize = 0;
    glyphMapOffset = 0;
    glyphMapEntries = 0;
    glyphDataOffset = 0;
    glyphDataSize = 0;
    kerning_pairs_loaded = false;
    readOnly = true;

#if defined(DEBUG_FONTENGINE)
    qDebug() << "QFontEngineQPA::QFontEngineQPA( fd =" << fd << ", renderingFontEngine =" << renderingFontEngine << ')';
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
        int tableSize = cmapSize;
        const uchar *cmapPtr = getCMap(fontData + cmapOffset, tableSize, &symbol, &cmapSize);
        if (cmapPtr)
            cmapOffset = cmapPtr - fontData;
        else
            cmapOffset = 0;
    } else if (externalCMap) {
        int tableSize = cmapSize;
        externalCMap = getCMap(externalCMap, tableSize, &symbol, &cmapSize);
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
                 << "externalCMap" << externalCMap << "cmapOffset" << cmapOffset
                 << "glyphMapOffset" << glyphMapOffset << "glyphDataOffset" << glyphDataOffset
                 << "fd" << fd << "glyphDataSize" << glyphDataSize;
#endif
}

QFontEngineQPA::~QFontEngineQPA()
{
}

bool QFontEngineQPA::getSfntTableData(uint tag, uchar *buffer, uint *length) const
{
    Q_UNUSED(tag);
    Q_UNUSED(buffer);
    *length = 0;
    return false;
}

bool QFontEngineQPA::stringToCMap(const QChar *str, int len, QGlyphLayout *glyphs, int *nglyphs, QTextEngine::ShaperFlags flags) const
{
    if (*nglyphs < len) {
        *nglyphs = len;
        return false;
    }

#if defined(DEBUG_FONTENGINE)
    QSet<QChar> seenGlyphs;
#endif

    const uchar *cmap = externalCMap ? externalCMap : (fontData + cmapOffset);

    bool mirrored = flags & QTextEngine::RightToLeft;
    int glyph_pos = 0;
    if (symbol) {
        for (int i = 0; i < len; ++i) {
            unsigned int uc = getChar(str, i, len);
            if (mirrored)
                uc = QChar::mirroredChar(uc);
            glyphs->glyphs[glyph_pos] = getTrueTypeGlyphIndex(cmap, uc);
            if(!glyphs->glyphs[glyph_pos] && uc < 0x100)
                glyphs->glyphs[glyph_pos] = getTrueTypeGlyphIndex(cmap, uc + 0xf000);
            ++glyph_pos;
        }
    } else {
        for (int i = 0; i < len; ++i) {
            unsigned int uc = getChar(str, i, len);
            if (mirrored)
                uc = QChar::mirroredChar(uc);
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
    recalcAdvances(glyphs, flags);
    return true;
}

void QFontEngineQPA::recalcAdvances(QGlyphLayout *glyphs, QTextEngine::ShaperFlags) const
{
    for (int i = 0; i < glyphs->numGlyphs; ++i) {
        const Glyph *g = findGlyph(glyphs->glyphs[i]);
        if (!g) {
            glyphs->glyphs[i] = 0;
            continue;
        }
        glyphs->advances_x[i] = g->advance;
        glyphs->advances_y[i] = 0;
    }
}

QImage QFontEngineQPA::alphaMapForGlyph(glyph_t g)
{
    const Glyph *glyph = findGlyph(g);
    if (!glyph)
        return QImage();

    const uchar *bits = ((const uchar *) glyph) + sizeof(Glyph);

    QImage image(bits,glyph->width, glyph->height, glyph->bytesPerLine, QImage::Format_Indexed8);

    return image;
}

void QFontEngineQPA::addOutlineToPath(qreal x, qreal y, const QGlyphLayout &glyphs, QPainterPath *path, QTextItem::RenderFlags flags)
{
    addBitmapFontToPath(x, y, glyphs, path, flags);
}

glyph_metrics_t QFontEngineQPA::boundingBox(const QGlyphLayout &glyphs)
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

glyph_metrics_t QFontEngineQPA::boundingBox(glyph_t glyph)
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

QFixed QFontEngineQPA::ascent() const
{
    return QFixed::fromReal(extractHeaderField(fontData, Tag_Ascent).value<qreal>());
}

QFixed QFontEngineQPA::descent() const
{
    return QFixed::fromReal(extractHeaderField(fontData, Tag_Descent).value<qreal>());
}

QFixed QFontEngineQPA::leading() const
{
    return QFixed::fromReal(extractHeaderField(fontData, Tag_Leading).value<qreal>());
}

qreal QFontEngineQPA::maxCharWidth() const
{
    return extractHeaderField(fontData, Tag_MaxCharWidth).value<qreal>();
}

qreal QFontEngineQPA::minLeftBearing() const
{
    return extractHeaderField(fontData, Tag_MinLeftBearing).value<qreal>();
}

qreal QFontEngineQPA::minRightBearing() const
{
    return extractHeaderField(fontData, Tag_MinRightBearing).value<qreal>();
}

QFixed QFontEngineQPA::underlinePosition() const
{
    return QFixed::fromReal(extractHeaderField(fontData, Tag_UnderlinePosition).value<qreal>());
}

QFixed QFontEngineQPA::lineThickness() const
{
    return QFixed::fromReal(extractHeaderField(fontData, Tag_LineThickness).value<qreal>());
}

QFontEngine::Type QFontEngineQPA::type() const
{
    return QFontEngine::QPF2;
}

bool QFontEngineQPA::canRender(const QChar *string, int len)
{
    const uchar *cmap = externalCMap ? externalCMap : (fontData + cmapOffset);

    if (symbol) {
        for (int i = 0; i < len; ++i) {
            unsigned int uc = getChar(string, i, len);
            glyph_t g = getTrueTypeGlyphIndex(cmap, uc);
            if(!g && uc < 0x100)
                g = getTrueTypeGlyphIndex(cmap, uc + 0xf000);
            if (!g)
                return false;
        }
    } else {
        for (int i = 0; i < len; ++i) {
            unsigned int uc = getChar(string, i, len);
            if (!getTrueTypeGlyphIndex(cmap, uc))
                return false;
        }
    }
    return true;
}

bool QFontEngineQPA::isValid() const
{
    return fontData && dataSize && (cmapOffset || externalCMap)
           && glyphMapOffset && glyphDataOffset && glyphDataSize > 0;
}

void QPAGenerator::generate()
{
    writeHeader();
    writeGMap();
    writeBlock(QFontEngineQPA::GlyphBlock, QByteArray());

    dev->seek(4); // position of header.lock
    writeUInt32(0);
}

void QPAGenerator::writeHeader()
{
    QFontEngineQPA::Header header;

    header.magic[0] = 'Q';
    header.magic[1] = 'P';
    header.magic[2] = 'F';
    header.magic[3] = '2';
    header.lock = 1;
    header.majorVersion = QFontEngineQPA::CurrentMajorVersion;
    header.minorVersion = QFontEngineQPA::CurrentMinorVersion;
    header.dataSize = 0;
    dev->write((const char *)&header, sizeof(header));

    writeTaggedString(QFontEngineQPA::Tag_FontName, fe->fontDef.family.toUtf8());

    QFontEngine::FaceId face = fe->faceId();
    writeTaggedString(QFontEngineQPA::Tag_FileName, face.filename);
    writeTaggedUInt32(QFontEngineQPA::Tag_FileIndex, face.index);

    {
        uchar data[4];
        uint len = 4;
        bool ok = fe->getSfntTableData(MAKE_TAG('h', 'e', 'a', 'd'), data, &len);
        if (ok) {
            const quint32 revision = qFromBigEndian<quint32>(data);
            writeTaggedUInt32(QFontEngineQPA::Tag_FontRevision, revision);
        }
    }

    writeTaggedQFixed(QFontEngineQPA::Tag_Ascent, fe->ascent());
    writeTaggedQFixed(QFontEngineQPA::Tag_Descent, fe->descent());
    writeTaggedQFixed(QFontEngineQPA::Tag_Leading, fe->leading());
    writeTaggedQFixed(QFontEngineQPA::Tag_XHeight, fe->xHeight());
    writeTaggedQFixed(QFontEngineQPA::Tag_AverageCharWidth, fe->averageCharWidth());
    writeTaggedQFixed(QFontEngineQPA::Tag_MaxCharWidth, QFixed::fromReal(fe->maxCharWidth()));
    writeTaggedQFixed(QFontEngineQPA::Tag_LineThickness, fe->lineThickness());
    writeTaggedQFixed(QFontEngineQPA::Tag_MinLeftBearing, QFixed::fromReal(fe->minLeftBearing()));
    writeTaggedQFixed(QFontEngineQPA::Tag_MinRightBearing, QFixed::fromReal(fe->minRightBearing()));
    writeTaggedQFixed(QFontEngineQPA::Tag_UnderlinePosition, fe->underlinePosition());
    writeTaggedUInt8(QFontEngineQPA::Tag_PixelSize, fe->fontDef.pixelSize);
    writeTaggedUInt8(QFontEngineQPA::Tag_Weight, fe->fontDef.weight);
    writeTaggedUInt8(QFontEngineQPA::Tag_Style, fe->fontDef.style);

    writeTaggedUInt8(QFontEngineQPA::Tag_GlyphFormat, QFontEngineQPA::AlphamapGlyphs);

    writeTaggedString(QFontEngineQPA::Tag_EndOfHeader, QByteArray());
    align4();

    const quint64 size = dev->pos();
    header.dataSize = qToBigEndian<quint16>(size - sizeof(header));
    dev->seek(0);
    dev->write((const char *)&header, sizeof(header));
    dev->seek(size);
}

void QPAGenerator::writeGMap()
{
    const quint16 glyphCount = fe->glyphCount();

    writeUInt16(QFontEngineQPA::GMapBlock);
    writeUInt16(0); // padding
    writeUInt32(glyphCount * 4);

    QByteArray &buffer = dev->buffer();
    const int numBytes = glyphCount * sizeof(quint32);
    qint64 pos = buffer.size();
    buffer.resize(pos + numBytes);
    qMemSet(buffer.data() + pos, 0xff, numBytes);
    dev->seek(pos + numBytes);
}

void QPAGenerator::writeBlock(QFontEngineQPA::BlockTag tag, const QByteArray &data)
{
    writeUInt16(tag);
    writeUInt16(0); // padding
    const int padSize = ((data.size() + 3) / 4) * 4 - data.size();
    writeUInt32(data.size() + padSize);
    dev->write(data);
    for (int i = 0; i < padSize; ++i)
        writeUInt8(0);
}

void QPAGenerator::writeTaggedString(QFontEngineQPA::HeaderTag tag, const QByteArray &string)
{
    writeUInt16(tag);
    writeUInt16(string.length());
    dev->write(string);
}

void QPAGenerator::writeTaggedUInt32(QFontEngineQPA::HeaderTag tag, quint32 value)
{
    writeUInt16(tag);
    writeUInt16(sizeof(value));
    writeUInt32(value);
}

void QPAGenerator::writeTaggedUInt8(QFontEngineQPA::HeaderTag tag, quint8 value)
{
    writeUInt16(tag);
    writeUInt16(sizeof(value));
    writeUInt8(value);
}

void QPAGenerator::writeTaggedQFixed(QFontEngineQPA::HeaderTag tag, QFixed value)
{
    writeUInt16(tag);
    writeUInt16(sizeof(quint32));
    writeUInt32(value.value());
}


/*
    Creates a new multi QPA engine.

    This function takes ownership of the QFontEngine, increasing it's refcount.
*/
QFontEngineMultiQPA::QFontEngineMultiQPA(QFontEngine *fe, int _script, const QStringList &fallbacks)
    : QFontEngineMulti(fallbacks.size() + 1),
      fallbackFamilies(fallbacks), script(_script)
{
    engines[0] = fe;
    fe->ref.ref();
    fontDef = engines[0]->fontDef;
}

void QFontEngineMultiQPA::loadEngine(int at)
{
    Q_ASSERT(at < engines.size());
    Q_ASSERT(engines.at(at) == 0);

    QFontDef request = fontDef;
    request.styleStrategy |= QFont::NoFontMerging;
    request.family = fallbackFamilies.at(at-1);
    engines[at] = QFontDatabase::findFont(script,
                                          /*fontprivate*/0,
                                          request);
    Q_ASSERT(engines[at]);
    engines[at]->ref.ref();
    engines[at]->fontDef = request;
}

QT_END_NAMESPACE
