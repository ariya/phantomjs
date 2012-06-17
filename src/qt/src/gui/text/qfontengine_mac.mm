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

#include "qfontengine_mac_p.h"

#include <private/qapplication_p.h>
#include <private/qfontengine_p.h>
#include <private/qpainter_p.h>
#include <private/qtextengine_p.h>
#include <qbitmap.h>
#include <private/qpaintengine_mac_p.h>
#include <private/qprintengine_mac_p.h>
#include <qglobal.h>
#include <qpixmap.h>
#include <qpixmapcache.h>
#include <qvarlengtharray.h>
#include <qdebug.h>
#include <qendian.h>
#include <qmath.h>
#include <private/qimage_p.h>

#include <ApplicationServices/ApplicationServices.h>
#include <AppKit/AppKit.h>

QT_BEGIN_NAMESPACE

/*****************************************************************************
  QFontEngine debug facilities
 *****************************************************************************/
//#define DEBUG_ADVANCES

extern int qt_antialiasing_threshold; // QApplication.cpp

#ifndef FixedToQFixed
#define FixedToQFixed(a) QFixed::fromFixed((a) >> 10)
#define QFixedToFixed(x) ((x).value() << 10)
#endif

class QMacFontPath
{
    float x, y;
    QPainterPath *path;
public:
    inline QMacFontPath(float _x, float _y, QPainterPath *_path) : x(_x), y(_y), path(_path) { }
    inline void setPosition(float _x, float _y) { x = _x; y = _y; }
    inline void advance(float _x) { x += _x; }
    static OSStatus lineTo(const Float32Point *, void *);
    static OSStatus cubicTo(const Float32Point *, const Float32Point *,
                            const Float32Point *, void *);
    static OSStatus moveTo(const Float32Point *, void *);
    static OSStatus closePath(void *);
};

OSStatus QMacFontPath::lineTo(const Float32Point *pt, void *data)

{
    QMacFontPath *p = static_cast<QMacFontPath*>(data);
    p->path->lineTo(p->x + pt->x, p->y + pt->y);
    return noErr;
}

OSStatus QMacFontPath::cubicTo(const Float32Point *cp1, const Float32Point *cp2,
                               const Float32Point *ep, void *data)

{
    QMacFontPath *p = static_cast<QMacFontPath*>(data);
    p->path->cubicTo(p->x + cp1->x, p->y + cp1->y,
                     p->x + cp2->x, p->y + cp2->y,
                     p->x + ep->x, p->y + ep->y);
    return noErr;
}

OSStatus QMacFontPath::moveTo(const Float32Point *pt, void *data)
{
    QMacFontPath *p = static_cast<QMacFontPath*>(data);
    p->path->moveTo(p->x + pt->x, p->y + pt->y);
    return noErr;
}

OSStatus QMacFontPath::closePath(void *data)
{
    static_cast<QMacFontPath*>(data)->path->closeSubpath();
    return noErr;
}


#ifndef QT_MAC_USE_COCOA
QFontEngineMacMulti::QFontEngineMacMulti(const ATSFontFamilyRef &atsFamily, const ATSFontRef &atsFontRef, const QFontDef &fontDef, bool kerning)
    : QFontEngineMulti(0)
{
    this->fontDef = fontDef;
    this->kerning = kerning;

    // hopefully (CTFontCreateWithName or CTFontCreateWithFontDescriptor) + CTFontCreateCopyWithSymbolicTraits
    // (or CTFontCreateWithQuickdrawInstance)
    FMFontFamily fmFamily;
    FMFontStyle fntStyle = 0;
    fmFamily = FMGetFontFamilyFromATSFontFamilyRef(atsFamily);
    if (fmFamily == kInvalidFontFamily) {
        // Use the ATSFont then...
        fontID = FMGetFontFromATSFontRef(atsFontRef);
    } else {
        if (fontDef.weight >= QFont::Bold)
            fntStyle |= ::bold;
        if (fontDef.style != QFont::StyleNormal)
            fntStyle |= ::italic;

        FMFontStyle intrinsicStyle;
        FMFont fnt = 0;
        if (FMGetFontFromFontFamilyInstance(fmFamily, fntStyle, &fnt, &intrinsicStyle) == noErr)
           fontID = FMGetATSFontRefFromFont(fnt);
    }

    // CFDictionaryRef, <CTStringAttributes.h>
    OSStatus status;

    status = ATSUCreateTextLayout(&textLayout);
    Q_ASSERT(status == noErr);

    const int maxAttributeCount = 5;
    ATSUAttributeTag tags[maxAttributeCount + 1];
    ByteCount sizes[maxAttributeCount + 1];
    ATSUAttributeValuePtr values[maxAttributeCount + 1];
    int attributeCount = 0;

    Fixed size = FixRatio(fontDef.pixelSize, 1);
    tags[attributeCount] = kATSUSizeTag;
    sizes[attributeCount] = sizeof(size);
    values[attributeCount] = &size;
    ++attributeCount;

    tags[attributeCount] = kATSUFontTag;
    sizes[attributeCount] = sizeof(fontID);
    values[attributeCount] = &this->fontID;
    ++attributeCount;

    transform = CGAffineTransformIdentity;
    if (fontDef.stretch != 100) {
        transform = CGAffineTransformMakeScale(float(fontDef.stretch) / float(100), 1);
        tags[attributeCount] = kATSUFontMatrixTag;
        sizes[attributeCount] = sizeof(transform);
        values[attributeCount] = &transform;
        ++attributeCount;
    }

    status = ATSUCreateStyle(&style);
    Q_ASSERT(status == noErr);

    Q_ASSERT(attributeCount < maxAttributeCount + 1);
    status = ATSUSetAttributes(style, attributeCount, tags, sizes, values);
    Q_ASSERT(status == noErr);

    QFontEngineMac *fe = new QFontEngineMac(style, fontID, fontDef, this);
    fe->ref.ref();
    engines.append(fe);
}

QFontEngineMacMulti::~QFontEngineMacMulti()
{
    ATSUDisposeTextLayout(textLayout);
    ATSUDisposeStyle(style);

    for (int i = 0; i < engines.count(); ++i) {
        QFontEngineMac *fe = const_cast<QFontEngineMac *>(static_cast<const QFontEngineMac *>(engines.at(i)));
        fe->multiEngine = 0;
        if (!fe->ref.deref())
            delete fe;
    }
    engines.clear();
}

struct QGlyphLayoutInfo
{
    QGlyphLayout *glyphs;
    int *numGlyphs;
    bool callbackCalled;
    int *mappedFonts;
    QTextEngine::ShaperFlags flags;
    QFontEngineMacMulti::ShaperItem *shaperItem;
    unsigned int styleStrategy;
};

static OSStatus atsuPostLayoutCallback(ATSULayoutOperationSelector selector, ATSULineRef lineRef, URefCon refCon,
                                 void *operationExtraParameter, ATSULayoutOperationCallbackStatus *callbackStatus)
{
    Q_UNUSED(selector);
    Q_UNUSED(operationExtraParameter);

    QGlyphLayoutInfo *nfo = reinterpret_cast<QGlyphLayoutInfo *>(refCon);
    nfo->callbackCalled = true;

    ATSLayoutRecord *layoutData = 0;
    ItemCount itemCount = 0;

    OSStatus e = noErr;
    e = ATSUDirectGetLayoutDataArrayPtrFromLineRef(lineRef, kATSUDirectDataLayoutRecordATSLayoutRecordCurrent,
                                                   /*iCreate =*/ false,
                                                   (void **) &layoutData,
                                                   &itemCount);
    if (e != noErr)
        return e;

    *nfo->numGlyphs = itemCount - 1;

    Fixed *baselineDeltas = 0;

    e = ATSUDirectGetLayoutDataArrayPtrFromLineRef(lineRef, kATSUDirectDataBaselineDeltaFixedArray,
                                                   /*iCreate =*/ true,
                                                   (void **) &baselineDeltas,
                                                   &itemCount);
    if (e != noErr)
        return e;

    int nextCharStop = -1;
    int currentClusterGlyph = -1; // first glyph in log cluster
    QFontEngineMacMulti::ShaperItem *item = nfo->shaperItem;
    if (item->charAttributes) {
        item = nfo->shaperItem;
#if !defined(QT_NO_DEBUG)
        int surrogates = 0;
        const QChar *str = item->string;
        for (int i = item->from; i < item->from + item->length - 1; ++i)
            surrogates += (str[i].isHighSurrogate() && str[i+1].isLowSurrogate());
#endif
        for (nextCharStop = item->from; nextCharStop < item->from + item->length; ++nextCharStop)
            if (item->charAttributes[nextCharStop].charStop)
                break;
        nextCharStop -= item->from;
    }

    nfo->glyphs->attributes[0].clusterStart = true;
    int glyphIdx = 0;
    int glyphIncrement = 1;
    if (nfo->flags & QTextEngine::RightToLeft) {
        glyphIdx  = itemCount - 2;
        glyphIncrement = -1;
    }
    for (int i = 0; i < *nfo->numGlyphs; ++i, glyphIdx += glyphIncrement) {

        int charOffset = layoutData[glyphIdx].originalOffset / sizeof(UniChar);
        const int fontIdx = nfo->mappedFonts[charOffset];

        ATSGlyphRef glyphId = layoutData[glyphIdx].glyphID;

        QFixed yAdvance = FixedToQFixed(baselineDeltas[glyphIdx]);
        QFixed xAdvance = FixedToQFixed(layoutData[glyphIdx + 1].realPos - layoutData[glyphIdx].realPos);

        if (nfo->styleStrategy & QFont::ForceIntegerMetrics) {
            yAdvance = yAdvance.round();
            xAdvance = xAdvance.round();
        }

        if (glyphId != 0xffff || i == 0) {
            if (i < nfo->glyphs->numGlyphs)
            {
                nfo->glyphs->glyphs[i] = (glyphId & 0x00ffffff) | (fontIdx << 24);

                nfo->glyphs->advances_y[i] = yAdvance;
                nfo->glyphs->advances_x[i] = xAdvance;
            }
        } else {
            // ATSUI gives us 0xffff as glyph id at the index in the glyph array for
            // a character position that maps to a ligtature. Such a glyph id does not
            // result in any visual glyph, but it may have an advance, which is why we
            // sum up the glyph advances.
            --i;
            nfo->glyphs->advances_y[i] += yAdvance;
            nfo->glyphs->advances_x[i] += xAdvance;
            *nfo->numGlyphs -= 1;
        }

        if (item->log_clusters) {
            if (charOffset >= nextCharStop) {
                nfo->glyphs->attributes[i].clusterStart = true;
                currentClusterGlyph = i;

                ++nextCharStop;
                for (; nextCharStop < item->length; ++nextCharStop)
                    if (item->charAttributes[item->from + nextCharStop].charStop)
                        break;
            } else {
                if (currentClusterGlyph == -1)
                    currentClusterGlyph = i;
            }
            item->log_clusters[charOffset] = currentClusterGlyph;

            // surrogate handling
            if (charOffset < item->length - 1) {
                QChar current = item->string[item->from + charOffset];
                QChar next = item->string[item->from + charOffset + 1];
                if (current.isHighSurrogate() && next.isLowSurrogate())
                    item->log_clusters[charOffset + 1] = currentClusterGlyph;
            }
        }
    }

    /*
    if (item) {
        qDebug() << "resulting logclusters:";
        for (int i = 0; i < item->length; ++i)
            qDebug() << "logClusters[" << i << "] =" << item->log_clusters[i];
        qDebug() << "clusterstarts:";
        for (int i = 0; i < *nfo->numGlyphs; ++i)
            qDebug() << "clusterStart[" << i << "] =" << nfo->glyphs[i].attributes.clusterStart;
    }
    */

    ATSUDirectReleaseLayoutDataArrayPtr(lineRef, kATSUDirectDataBaselineDeltaFixedArray,
                                        (void **) &baselineDeltas);

    ATSUDirectReleaseLayoutDataArrayPtr(lineRef, kATSUDirectDataLayoutRecordATSLayoutRecordCurrent,
                                        (void **) &layoutData);

    *callbackStatus = kATSULayoutOperationCallbackStatusHandled;
    return noErr;
}

int QFontEngineMacMulti::fontIndexForFontID(ATSUFontID id) const
{
    for (int i = 0; i < engines.count(); ++i) {
        if (engineAt(i)->fontID == id)
            return i;
    }

    QFontEngineMacMulti *that = const_cast<QFontEngineMacMulti *>(this);
    QFontEngineMac *fe = new QFontEngineMac(style, id, fontDef, that);
    fe->ref.ref();
    that->engines.append(fe);
    return engines.count() - 1;
}

bool QFontEngineMacMulti::stringToCMap(const QChar *str, int len, QGlyphLayout *glyphs, int *nglyphs, QTextEngine::ShaperFlags flags) const
{
    return stringToCMap(str, len, glyphs, nglyphs, flags, /*logClusters=*/0, /*charAttributes=*/0, /*si=*/0);
}

bool QFontEngineMacMulti::stringToCMap(const QChar *str, int len, QGlyphLayout *glyphs, int *nglyphs, QTextEngine::ShaperFlags flags,
                                       unsigned short *logClusters, const HB_CharAttributes *charAttributes, QScriptItem *) const
{
    if (*nglyphs < len) {
        *nglyphs = len;
        return false;
    }

    ShaperItem shaperItem;
    shaperItem.string = str;
    shaperItem.from = 0;
    shaperItem.length = len;
    shaperItem.glyphs = *glyphs;
    shaperItem.glyphs.numGlyphs = *nglyphs;
    shaperItem.flags = flags;
    shaperItem.log_clusters = logClusters;
    shaperItem.charAttributes = charAttributes;

    const int maxChars = qMax(1,
                              int(SHRT_MAX / maxCharWidth())
                              - 10 // subtract a few to be on the safe side
                             );
    if (len < maxChars || !charAttributes)
        return stringToCMapInternal(str, len, glyphs, nglyphs, flags, &shaperItem);

    int charIdx = 0;
    int glyphIdx = 0;
    ShaperItem tmpItem = shaperItem;

    do {
        tmpItem.from = shaperItem.from + charIdx;

        int charCount = qMin(maxChars, len - charIdx);

        int lastWhitespace = tmpItem.from + charCount - 1;
        int lastSoftBreak = lastWhitespace;
        int lastCharStop = lastSoftBreak;
        for (int i = lastCharStop; i >= tmpItem.from; --i) {
            if (tmpItem.charAttributes[i].whiteSpace) {
                lastWhitespace = i;
                break;
            } if (tmpItem.charAttributes[i].lineBreakType != HB_NoBreak) {
                lastSoftBreak = i;
            } if (tmpItem.charAttributes[i].charStop) {
                lastCharStop = i;
            }
        }
        charCount = qMin(lastWhitespace, qMin(lastSoftBreak, lastCharStop)) - tmpItem.from + 1;

        int glyphCount = shaperItem.glyphs.numGlyphs - glyphIdx;
        if (glyphCount <= 0)
            return false;
        tmpItem.length = charCount;
        tmpItem.glyphs = shaperItem.glyphs.mid(glyphIdx, glyphCount);
        tmpItem.log_clusters = shaperItem.log_clusters + charIdx;
        if (!stringToCMapInternal(tmpItem.string + tmpItem.from, tmpItem.length,
                                  &tmpItem.glyphs, &glyphCount, flags,
                                  &tmpItem)) {
            *nglyphs = glyphIdx + glyphCount;
            return false;
	}
        for (int i = 0; i < charCount; ++i)
            tmpItem.log_clusters[i] += glyphIdx;
        glyphIdx += glyphCount;
        charIdx += charCount;
    } while (charIdx < len);
    *nglyphs = glyphIdx;
    glyphs->numGlyphs = glyphIdx;

    return true;
}

bool QFontEngineMacMulti::stringToCMapInternal(const QChar *str, int len, QGlyphLayout *glyphs, int *nglyphs, QTextEngine::ShaperFlags flags,ShaperItem *shaperItem) const
{
    //qDebug() << "stringToCMap" << QString(str, len);

    OSStatus e = noErr;

    e = ATSUSetTextPointerLocation(textLayout, (UniChar *)(str), 0, len, len);
    if (e != noErr) {
        qWarning("Qt: internal: %ld: Error ATSUSetTextPointerLocation %s: %d", long(e), __FILE__, __LINE__);
        return false;
    }

    QGlyphLayoutInfo nfo;
    nfo.glyphs = glyphs;
    nfo.numGlyphs = nglyphs;
    nfo.callbackCalled = false;
    nfo.flags = flags;
    nfo.shaperItem = shaperItem;
    nfo.styleStrategy = fontDef.styleStrategy;

    int prevNumGlyphs = *nglyphs;

    QVarLengthArray<int> mappedFonts(len);
    for (int i = 0; i < len; ++i)
        mappedFonts[i] = 0;
    nfo.mappedFonts = mappedFonts.data();

    Q_ASSERT(sizeof(void *) <= sizeof(URefCon));
    e = ATSUSetTextLayoutRefCon(textLayout, (URefCon)&nfo);
    if (e != noErr) {
        qWarning("Qt: internal: %ld: Error ATSUSetTextLayoutRefCon %s: %d", long(e), __FILE__, __LINE__);
        return false;
    }

    {
        const int maxAttributeCount = 3;
        ATSUAttributeTag tags[maxAttributeCount + 1];
        ByteCount sizes[maxAttributeCount + 1];
        ATSUAttributeValuePtr values[maxAttributeCount + 1];
        int attributeCount = 0;

        tags[attributeCount] = kATSULineLayoutOptionsTag;
        ATSLineLayoutOptions layopts = kATSLineHasNoOpticalAlignment
                                       | kATSLineIgnoreFontLeading
                                       | kATSLineNoSpecialJustification // we do kashidas ourselves
                                       | kATSLineDisableAllJustification
                                       ;

        if (fontDef.styleStrategy & QFont::NoAntialias)
            layopts |= kATSLineNoAntiAliasing;

        if (!kerning)
            layopts |= kATSLineDisableAllKerningAdjustments;

        values[attributeCount] = &layopts;
        sizes[attributeCount] = sizeof(layopts);
        ++attributeCount;

        tags[attributeCount] = kATSULayoutOperationOverrideTag;
        ATSULayoutOperationOverrideSpecifier spec;
        spec.operationSelector = kATSULayoutOperationPostLayoutAdjustment;
        spec.overrideUPP = atsuPostLayoutCallback;
        values[attributeCount] = &spec;
        sizes[attributeCount] = sizeof(spec);
        ++attributeCount;

        // CTWritingDirection
        Boolean direction;
        if (flags & QTextEngine::RightToLeft)
            direction = kATSURightToLeftBaseDirection;
        else
            direction = kATSULeftToRightBaseDirection;
        tags[attributeCount] = kATSULineDirectionTag;
        values[attributeCount] = &direction;
        sizes[attributeCount] = sizeof(direction);
        ++attributeCount;

        Q_ASSERT(attributeCount < maxAttributeCount + 1);
        e = ATSUSetLayoutControls(textLayout, attributeCount, tags, sizes, values);
        if (e != noErr) {
            qWarning("Qt: internal: %ld: Error ATSUSetLayoutControls %s: %d", long(e), __FILE__, __LINE__);
            return false;
        }

    }

    e = ATSUSetRunStyle(textLayout, style, 0, len);
    if (e != noErr) {
        qWarning("Qt: internal: %ld: Error ATSUSetRunStyle %s: %d", long(e), __FILE__, __LINE__);
        return false;
    }

    if (!(fontDef.styleStrategy & QFont::NoFontMerging)) {
        int pos = 0;
        do {
            ATSUFontID substFont = 0;
            UniCharArrayOffset changedOffset = 0;
            UniCharCount changeCount = 0;

            e = ATSUMatchFontsToText(textLayout, pos, len - pos,
                                     &substFont, &changedOffset,
                                     &changeCount);
            if (e == kATSUFontsMatched) {
                int fontIdx = fontIndexForFontID(substFont);
                for (uint i = 0; i < changeCount; ++i)
                    mappedFonts[changedOffset + i] = fontIdx;
                pos = changedOffset + changeCount;
                ATSUSetRunStyle(textLayout, engineAt(fontIdx)->style, changedOffset, changeCount);
            } else if (e == kATSUFontsNotMatched) {
                pos = changedOffset + changeCount;
            }
        } while (pos < len && e != noErr);
    }
    {    // trigger the a layout
        // CFAttributedStringCreate, CTFramesetterCreateWithAttributedString (or perhaps Typesetter)
        Rect rect;
        e = ATSUMeasureTextImage(textLayout, kATSUFromTextBeginning, kATSUToTextEnd,
                                 /*iLocationX =*/ 0, /*iLocationY =*/ 0,
                                 &rect);
        if (e != noErr) {
            qWarning("Qt: internal: %ld: Error ATSUMeasureTextImage %s: %d", long(e), __FILE__, __LINE__);
            return false;
        }
    }

    if (!nfo.callbackCalled) {
            qWarning("Qt: internal: %ld: Error ATSUMeasureTextImage did not trigger callback %s: %d", long(e), __FILE__, __LINE__);
            return false;
    }

    ATSUClearLayoutCache(textLayout, kATSUFromTextBeginning);
    if (prevNumGlyphs < *nfo.numGlyphs)
        return false;
    return true;
}

void QFontEngineMacMulti::recalcAdvances(QGlyphLayout *glyphs, QTextEngine::ShaperFlags flags) const
{
    Q_ASSERT(false);
    Q_UNUSED(glyphs);
    Q_UNUSED(flags);
}

void QFontEngineMacMulti::doKerning(QGlyphLayout *, QTextEngine::ShaperFlags) const
{
    //Q_ASSERT(false);
}

void QFontEngineMacMulti::loadEngine(int /*at*/)
{
    // should never be called!
    Q_ASSERT(false);
}

bool QFontEngineMacMulti::canRender(const QChar *string, int len)
{
    ATSUSetTextPointerLocation(textLayout, reinterpret_cast<const UniChar *>(string), 0, len, len);
    ATSUSetRunStyle(textLayout, style, 0, len);

    OSStatus e = noErr;
    int pos = 0;
    do {
        FMFont substFont = 0;
        UniCharArrayOffset changedOffset = 0;
        UniCharCount changeCount = 0;

        // CTFontCreateForString
        e = ATSUMatchFontsToText(textLayout, pos, len - pos,
                                 &substFont, &changedOffset,
                                 &changeCount);
        if (e == kATSUFontsMatched) {
            pos = changedOffset + changeCount;
        } else if (e == kATSUFontsNotMatched) {
            break;
        }
    } while (pos < len && e != noErr);

    return e == noErr || e == kATSUFontsMatched;
}

QFontEngineMac::QFontEngineMac(ATSUStyle baseStyle, ATSUFontID fontID, const QFontDef &def, QFontEngineMacMulti *multiEngine)
    : fontID(fontID), multiEngine(multiEngine), cmap(0), symbolCMap(false)
{
    fontDef = def;
    ATSUCreateAndCopyStyle(baseStyle, &style);
    ATSFontRef atsFont = FMGetATSFontRefFromFont(fontID);
    cgFont = CGFontCreateWithPlatformFont(&atsFont);

    const int maxAttributeCount = 4;
    ATSUAttributeTag tags[maxAttributeCount + 1];
    ByteCount sizes[maxAttributeCount + 1];
    ATSUAttributeValuePtr values[maxAttributeCount + 1];
    int attributeCount = 0;

    synthesisFlags = 0;

    // synthesizing using CG is not recommended
    quint16 macStyle = 0;
    {
        uchar data[4];
        ByteCount len = 4;
        if (ATSFontGetTable(atsFont, MAKE_TAG('h', 'e', 'a', 'd'), 44, 4, &data, &len) == noErr)
            macStyle = qFromBigEndian<quint16>(data);
    }

    Boolean atsuBold = false;
    Boolean atsuItalic = false;
    if (fontDef.weight >= QFont::Bold) {
        if (!(macStyle & 1)) {
            synthesisFlags |= SynthesizedBold;
            atsuBold = true;
            tags[attributeCount] = kATSUQDBoldfaceTag;
            sizes[attributeCount] = sizeof(atsuBold);
            values[attributeCount] = &atsuBold;
            ++attributeCount;
        }
    }
    if (fontDef.style != QFont::StyleNormal) {
        if (!(macStyle & 2)) {
            synthesisFlags |= SynthesizedItalic;
            atsuItalic = true;
            tags[attributeCount] = kATSUQDItalicTag;
            sizes[attributeCount] = sizeof(atsuItalic);
            values[attributeCount] = &atsuItalic;
            ++attributeCount;
        }
    }

    tags[attributeCount] = kATSUFontTag;
    values[attributeCount] = &fontID;
    sizes[attributeCount] = sizeof(fontID);
    ++attributeCount;

    Q_ASSERT(attributeCount < maxAttributeCount + 1);
    OSStatus err = ATSUSetAttributes(style, attributeCount, tags, sizes, values);
    Q_ASSERT(err == noErr);
    Q_UNUSED(err);

    // CTFontCopyTable
    quint16 tmpFsType;
    if (ATSFontGetTable(atsFont, MAKE_TAG('O', 'S', '/', '2'), 8, 2, &tmpFsType, 0) == noErr)
       fsType = qFromBigEndian<quint16>(tmpFsType);
    else
        fsType = 0;

    if (multiEngine)
	transform = multiEngine->transform;
    else
	transform = CGAffineTransformIdentity;

    ATSUTextMeasurement metric;

    ATSUGetAttribute(style, kATSUAscentTag, sizeof(metric), &metric, 0);
    m_ascent = FixRound(metric);

    ATSUGetAttribute(style, kATSUDescentTag, sizeof(metric), &metric, 0);
    m_descent = FixRound(metric);

    ATSUGetAttribute(style, kATSULeadingTag, sizeof(metric), &metric, 0);
    m_leading = FixRound(metric);

    ATSFontMetrics metrics;

    ATSFontGetHorizontalMetrics(FMGetATSFontRefFromFont(fontID), kATSOptionFlagsDefault, &metrics);
    m_maxCharWidth = metrics.maxAdvanceWidth * fontDef.pointSize;

    ATSFontGetHorizontalMetrics(FMGetATSFontRefFromFont(fontID), kATSOptionFlagsDefault, &metrics);
    m_xHeight = QFixed::fromReal(metrics.xHeight * fontDef.pointSize);

    ATSFontGetHorizontalMetrics(FMGetATSFontRefFromFont(fontID), kATSOptionFlagsDefault, &metrics);
    m_averageCharWidth = QFixed::fromReal(metrics.avgAdvanceWidth * fontDef.pointSize);

    // Use width of 'X' if ATSFontGetHorizontalMetrics returns 0 for avgAdvanceWidth.
    if (m_averageCharWidth == QFixed(0)) {
        QChar c('X');
        QGlyphLayoutArray<1> glyphs;
        int nglyphs = 1;
        stringToCMap(&c, 1, &glyphs, &nglyphs, 0);
        glyph_metrics_t metrics = boundingBox(glyphs);
        m_averageCharWidth =  metrics.width;
    }
}

QFontEngineMac::~QFontEngineMac()
{
    ATSUDisposeStyle(style);
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

// Not used directly for shaping, only used to calculate m_averageCharWidth
bool QFontEngineMac::stringToCMap(const QChar *str, int len, QGlyphLayout *glyphs, int *nglyphs, QTextEngine::ShaperFlags flags) const
{
    if (!cmap) {
        cmapTable = getSfntTable(MAKE_TAG('c', 'm', 'a', 'p'));
        int size = 0;
        cmap = getCMap(reinterpret_cast<const uchar *>(cmapTable.constData()), cmapTable.size(), &symbolCMap, &size);
        if (!cmap)
            return false;
    }
    if (symbolCMap) {
        for (int i = 0; i < len; ++i) {
            unsigned int uc = getChar(str, i, len);
            glyphs->glyphs[i] = getTrueTypeGlyphIndex(cmap, uc);
            if(!glyphs->glyphs[i] && uc < 0x100)
                glyphs->glyphs[i] = getTrueTypeGlyphIndex(cmap, uc + 0xf000);
        }
    } else {
        for (int i = 0; i < len; ++i) {
            unsigned int uc = getChar(str, i, len);
            glyphs->glyphs[i] = getTrueTypeGlyphIndex(cmap, uc);
        }
    }

    *nglyphs = len;
    glyphs->numGlyphs = *nglyphs;

    if (!(flags & QTextEngine::GlyphIndicesOnly))
        recalcAdvances(glyphs, flags);

    return true;
}

void QFontEngineMac::recalcAdvances(QGlyphLayout *glyphs, QTextEngine::ShaperFlags flags) const
{
    Q_UNUSED(flags)

    QVarLengthArray<GlyphID> atsuGlyphs(glyphs->numGlyphs);
    for (int i = 0; i < glyphs->numGlyphs; ++i)
        atsuGlyphs[i] = glyphs->glyphs[i];

    QVarLengthArray<ATSGlyphScreenMetrics> metrics(glyphs->numGlyphs);

    ATSUGlyphGetScreenMetrics(style, glyphs->numGlyphs, atsuGlyphs.data(), sizeof(GlyphID),
                              /* iForcingAntiAlias =*/ false,
                              /* iAntiAliasSwitch =*/true,
                              metrics.data());

    for (int i = 0; i < glyphs->numGlyphs; ++i) {
        glyphs->advances_x[i] = QFixed::fromReal(metrics[i].deviceAdvance.x);
        glyphs->advances_y[i] = QFixed::fromReal(metrics[i].deviceAdvance.y);

        if (fontDef.styleStrategy & QFont::ForceIntegerMetrics) {
            glyphs->advances_x[i] = glyphs->advances_x[i].round();
            glyphs->advances_y[i] = glyphs->advances_y[i].round();
        }
    }
}

glyph_metrics_t QFontEngineMac::boundingBox(const QGlyphLayout &glyphs)
{
    QFixed w;
    bool round = fontDef.styleStrategy & QFont::ForceIntegerMetrics;
    for (int i = 0; i < glyphs.numGlyphs; ++i) {
        w += round ? glyphs.effectiveAdvance(i).round()
                   : glyphs.effectiveAdvance(i);
    }
    return glyph_metrics_t(0, -(ascent()), w - lastRightBearing(glyphs, round), ascent()+descent(), w, 0);
}

glyph_metrics_t QFontEngineMac::boundingBox(glyph_t glyph)
{
    GlyphID atsuGlyph = glyph;

    ATSGlyphScreenMetrics metrics;

    ATSUGlyphGetScreenMetrics(style, 1, &atsuGlyph, 0,
                              /* iForcingAntiAlias =*/ false,
                              /* iAntiAliasSwitch =*/true,
                              &metrics);

    // ### check again

    glyph_metrics_t gm;
    gm.width = int(metrics.width);
    gm.height = int(metrics.height);
    gm.x = QFixed::fromReal(metrics.topLeft.x);
    gm.y = -QFixed::fromReal(metrics.topLeft.y);
    gm.xoff = QFixed::fromReal(metrics.deviceAdvance.x);
    gm.yoff = QFixed::fromReal(metrics.deviceAdvance.y);

    if (fontDef.styleStrategy & QFont::ForceIntegerMetrics) {
        gm.x = gm.x.floor();
        gm.y = gm.y.floor();
        gm.xoff = gm.xoff.round();
        gm.yoff = gm.yoff.round();
    }

    return gm;
}

QFixed QFontEngineMac::ascent() const
{
    return (fontDef.styleStrategy & QFont::ForceIntegerMetrics)
            ? m_ascent.round()
            : m_ascent;
}

QFixed QFontEngineMac::descent() const
{
    // subtract a pixel to even out the historical +1 in QFontMetrics::height().
    // Fix in Qt 5.
    return (fontDef.styleStrategy & QFont::ForceIntegerMetrics)
            ? m_descent.round() - 1
            : m_descent;
}

QFixed QFontEngineMac::leading() const
{
    return (fontDef.styleStrategy & QFont::ForceIntegerMetrics)
            ? m_leading.round()
            : m_leading;
}

qreal QFontEngineMac::maxCharWidth() const
{
    return (fontDef.styleStrategy & QFont::ForceIntegerMetrics)
            ? qRound(m_maxCharWidth)
            : m_maxCharWidth;
}

QFixed QFontEngineMac::xHeight() const
{
    return (fontDef.styleStrategy & QFont::ForceIntegerMetrics)
            ? m_xHeight.round()
            : m_xHeight;
}

QFixed QFontEngineMac::averageCharWidth() const
{
    return (fontDef.styleStrategy & QFont::ForceIntegerMetrics)
            ? m_averageCharWidth.round()
            : m_averageCharWidth;
}

static void addGlyphsToPathHelper(ATSUStyle style, glyph_t *glyphs, QFixedPoint *positions, int numGlyphs, QPainterPath *path)
{
    if (!numGlyphs)
        return;

    OSStatus e;

    QMacFontPath fontpath(0, 0, path);
    ATSCubicMoveToUPP moveTo = NewATSCubicMoveToUPP(QMacFontPath::moveTo);
    ATSCubicLineToUPP lineTo = NewATSCubicLineToUPP(QMacFontPath::lineTo);
    ATSCubicCurveToUPP cubicTo = NewATSCubicCurveToUPP(QMacFontPath::cubicTo);
    ATSCubicClosePathUPP closePath = NewATSCubicClosePathUPP(QMacFontPath::closePath);

    // CTFontCreatePathForGlyph
    for (int i = 0; i < numGlyphs; ++i) {
        GlyphID glyph = glyphs[i];

        fontpath.setPosition(positions[i].x.toReal(), positions[i].y.toReal());
        ATSUGlyphGetCubicPaths(style, glyph, moveTo, lineTo,
                               cubicTo, closePath, &fontpath, &e);
    }

    DisposeATSCubicMoveToUPP(moveTo);
    DisposeATSCubicLineToUPP(lineTo);
    DisposeATSCubicCurveToUPP(cubicTo);
    DisposeATSCubicClosePathUPP(closePath);
}

void QFontEngineMac::addGlyphsToPath(glyph_t *glyphs, QFixedPoint *positions, int numGlyphs, QPainterPath *path,
                                           QTextItem::RenderFlags)
{
    addGlyphsToPathHelper(style, glyphs, positions, numGlyphs, path);
}


/*!
  Helper function for alphaMapForGlyph and alphaRGBMapForGlyph. The two are identical, except for
  the subpixel antialiasing...
*/
QImage QFontEngineMac::imageForGlyph(glyph_t glyph, int margin, bool colorful)
{
    const glyph_metrics_t br = boundingBox(glyph);
    QImage im(qRound(br.width)+2, qRound(br.height)+4, QImage::Format_RGB32);
    im.fill(0xff000000);

    CGColorSpaceRef colorspace = QCoreGraphicsPaintEngine::macGenericColorSpace();
    uint cgflags = kCGImageAlphaNoneSkipFirst;
#ifdef kCGBitmapByteOrder32Host //only needed because CGImage.h added symbols in the minor version
    cgflags |= kCGBitmapByteOrder32Host;
#endif
    CGContextRef ctx = CGBitmapContextCreate(im.bits(), im.width(), im.height(),
                                             8, im.bytesPerLine(), colorspace,
                                             cgflags);
    CGContextSetFontSize(ctx, fontDef.pixelSize);
    CGContextSetShouldAntialias(ctx, fontDef.pointSize > qt_antialiasing_threshold && !(fontDef.styleStrategy & QFont::NoAntialias));
    // turn off sub-pixel hinting - no support for that in OpenGL
    CGContextSetShouldSmoothFonts(ctx, colorful);
    CGAffineTransform oldTextMatrix = CGContextGetTextMatrix(ctx);
    CGAffineTransform cgMatrix = CGAffineTransformMake(1, 0, 0, 1, 0, 0);
    CGAffineTransformConcat(cgMatrix, oldTextMatrix);

    if (synthesisFlags & QFontEngine::SynthesizedItalic)
        cgMatrix = CGAffineTransformConcat(cgMatrix, CGAffineTransformMake(1, 0, tanf(14 * acosf(0) / 90), 1, 0, 0));

    cgMatrix = CGAffineTransformConcat(cgMatrix, transform);

    CGContextSetTextMatrix(ctx, cgMatrix);
    CGContextSetRGBFillColor(ctx, 1, 1, 1, 1);
    CGContextSetTextDrawingMode(ctx, kCGTextFill);
    CGContextSetFont(ctx, cgFont);

    qreal pos_x = -br.x.toReal() + 1;
    qreal pos_y = im.height() + br.y.toReal() - 2;
    CGContextSetTextPosition(ctx, pos_x, pos_y);

    CGSize advance;
    advance.width = 0;
    advance.height = 0;
    CGGlyph cgGlyph = glyph;
    CGContextShowGlyphsWithAdvances(ctx, &cgGlyph, &advance, 1);

    if (synthesisFlags & QFontEngine::SynthesizedBold) {
        CGContextSetTextPosition(ctx, pos_x + 0.5 * lineThickness().toReal(), pos_y);
        CGContextShowGlyphsWithAdvances(ctx, &cgGlyph, &advance, 1);
    }

    CGContextRelease(ctx);

    return im;
}

QImage QFontEngineMac::alphaMapForGlyph(glyph_t glyph)
{
    QImage im = imageForGlyph(glyph, 2, false);

    QImage indexed(im.width(), im.height(), QImage::Format_Indexed8);
    QVector<QRgb> colors(256);
    for (int i=0; i<256; ++i)
        colors[i] = qRgba(0, 0, 0, i);
    indexed.setColorTable(colors);

    for (int y=0; y<im.height(); ++y) {
        uint *src = (uint*) im.scanLine(y);
        uchar *dst = indexed.scanLine(y);
        for (int x=0; x<im.width(); ++x) {
            *dst = qGray(*src);
            ++dst;
            ++src;
        }
    }

    return indexed;
}

QImage QFontEngineMac::alphaRGBMapForGlyph(glyph_t glyph, QFixed, int margin, const QTransform &t)
{
    QImage im = imageForGlyph(glyph, margin, true);

    if (t.type() >= QTransform::TxScale) {
        im = im.transformed(t);
    }

    qGamma_correct_back_to_linear_cs(&im);

    return im;
}


bool QFontEngineMac::canRender(const QChar *string, int len)
{
    Q_ASSERT(false);
    Q_UNUSED(string);
    Q_UNUSED(len);
    return false;
}

void QFontEngineMac::draw(CGContextRef ctx, qreal x, qreal y, const QTextItemInt &ti, int paintDeviceHeight)
{
    QVarLengthArray<QFixedPoint> positions;
    QVarLengthArray<glyph_t> glyphs;
    QTransform matrix;
    matrix.translate(x, y);
    getGlyphPositions(ti.glyphs, matrix, ti.flags, glyphs, positions);
    if (glyphs.size() == 0)
        return;

    CGContextSetFontSize(ctx, fontDef.pixelSize);

    CGAffineTransform oldTextMatrix = CGContextGetTextMatrix(ctx);

    CGAffineTransform cgMatrix = CGAffineTransformMake(1, 0, 0, -1, 0, -paintDeviceHeight);

    CGAffineTransformConcat(cgMatrix, oldTextMatrix);

    if (synthesisFlags & QFontEngine::SynthesizedItalic)
        cgMatrix = CGAffineTransformConcat(cgMatrix, CGAffineTransformMake(1, 0, -tanf(14 * acosf(0) / 90), 1, 0, 0));

    cgMatrix = CGAffineTransformConcat(cgMatrix, transform);

    CGContextSetTextMatrix(ctx, cgMatrix);

    CGContextSetTextDrawingMode(ctx, kCGTextFill);


    QVarLengthArray<CGSize> advances(glyphs.size());
    QVarLengthArray<CGGlyph> cgGlyphs(glyphs.size());

    for (int i = 0; i < glyphs.size() - 1; ++i) {
        advances[i].width = (positions[i + 1].x - positions[i].x).toReal();
        advances[i].height = (positions[i + 1].y - positions[i].y).toReal();
        cgGlyphs[i] = glyphs[i];
    }
    advances[glyphs.size() - 1].width = 0;
    advances[glyphs.size() - 1].height = 0;
    cgGlyphs[glyphs.size() - 1] = glyphs[glyphs.size() - 1];

    CGContextSetFont(ctx, cgFont);

    CGContextSetTextPosition(ctx, positions[0].x.toReal(), positions[0].y.toReal());

    CGContextShowGlyphsWithAdvances(ctx, cgGlyphs.data(), advances.data(), glyphs.size());

    if (synthesisFlags & QFontEngine::SynthesizedBold) {
        CGContextSetTextPosition(ctx, positions[0].x.toReal() + 0.5 * lineThickness().toReal(),
                                      positions[0].y.toReal());

        CGContextShowGlyphsWithAdvances(ctx, cgGlyphs.data(), advances.data(), glyphs.size());
    }

    CGContextSetTextMatrix(ctx, oldTextMatrix);
}

QFontEngine::FaceId QFontEngineMac::faceId() const
{
    FaceId ret;
#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5)
if (QSysInfo::MacintoshVersion >= QSysInfo::MV_10_5) {
    // CTFontGetPlatformFont
    FSRef ref;
    if (ATSFontGetFileReference(FMGetATSFontRefFromFont(fontID), &ref) != noErr)
        return ret;
    ret.filename = QByteArray(128, 0);
    ret.index = fontID;
    FSRefMakePath(&ref, (UInt8 *)ret.filename.data(), ret.filename.size());
}else
#endif
{
    FSSpec spec;
    if (ATSFontGetFileSpecification(FMGetATSFontRefFromFont(fontID), &spec) != noErr)
        return ret;

    FSRef ref;
    FSpMakeFSRef(&spec, &ref);
    ret.filename = QByteArray(128, 0);
    ret.index = fontID;
    FSRefMakePath(&ref, (UInt8 *)ret.filename.data(), ret.filename.size());
}
    return ret;
}

QByteArray QFontEngineMac::getSfntTable(uint tag) const
{
    ATSFontRef atsFont = FMGetATSFontRefFromFont(fontID);

    ByteCount length;
    OSStatus status = ATSFontGetTable(atsFont, tag, 0, 0, 0, &length);
    if (status != noErr)
        return QByteArray();
    QByteArray table(length, 0);
    // CTFontCopyTable
    status = ATSFontGetTable(atsFont, tag, 0, table.length(), table.data(), &length);
    if (status != noErr)
        return QByteArray();
    return table;
}

QFontEngine::Properties QFontEngineMac::properties() const
{
    QFontEngine::Properties props;
    ATSFontRef atsFont = FMGetATSFontRefFromFont(fontID);
    quint16 tmp;
    // CTFontGetUnitsPerEm
    if (ATSFontGetTable(atsFont, MAKE_TAG('h', 'e', 'a', 'd'), 18, 2, &tmp, 0) == noErr)
       props.emSquare = qFromBigEndian<quint16>(tmp);
    struct {
        qint16 xMin;
        qint16 yMin;
        qint16 xMax;
        qint16 yMax;
    } bbox;
    bbox.xMin = bbox.xMax = bbox.yMin = bbox.yMax = 0;
    // CTFontGetBoundingBox
    if (ATSFontGetTable(atsFont, MAKE_TAG('h', 'e', 'a', 'd'), 36, 8, &bbox, 0) == noErr) {
        bbox.xMin = qFromBigEndian<quint16>(bbox.xMin);
        bbox.yMin = qFromBigEndian<quint16>(bbox.yMin);
        bbox.xMax = qFromBigEndian<quint16>(bbox.xMax);
        bbox.yMax = qFromBigEndian<quint16>(bbox.yMax);
    }
    struct {
        qint16 ascender;
        qint16 descender;
        qint16 linegap;
    } metrics;
    metrics.ascender = metrics.descender = metrics.linegap = 0;
    // CTFontGetAscent, etc.
    if (ATSFontGetTable(atsFont, MAKE_TAG('h', 'h', 'e', 'a'), 4, 6, &metrics, 0) == noErr) {
        metrics.ascender = qFromBigEndian<quint16>(metrics.ascender);
        metrics.descender = qFromBigEndian<quint16>(metrics.descender);
        metrics.linegap = qFromBigEndian<quint16>(metrics.linegap);
    }
    props.ascent = metrics.ascender;
    props.descent = -metrics.descender;
    props.leading = metrics.linegap;
    props.boundingBox = QRectF(bbox.xMin, -bbox.yMax,
                           bbox.xMax - bbox.xMin,
                           bbox.yMax - bbox.yMin);
    props.italicAngle = 0;
    props.capHeight = props.ascent;

    qint16 lw = 0;
    // fonts lie
    if (ATSFontGetTable(atsFont, MAKE_TAG('p', 'o', 's', 't'), 10, 2, &lw, 0) == noErr)
       lw = qFromBigEndian<quint16>(lw);
    props.lineWidth = lw;

    // CTFontCopyPostScriptName
    QCFString psName;
    if (ATSFontGetPostScriptName(FMGetATSFontRefFromFont(fontID), kATSOptionFlagsDefault, &psName) == noErr)
        props.postscriptName = QString(psName).toUtf8();
    props.postscriptName = QFontEngine::convertToPostscriptFontFamilyName(props.postscriptName);
    return props;
}

void QFontEngineMac::getUnscaledGlyph(glyph_t glyph, QPainterPath *path, glyph_metrics_t *metrics)
{
    ATSUStyle unscaledStyle;
    ATSUCreateAndCopyStyle(style, &unscaledStyle);

    int emSquare = properties().emSquare.toInt();

    const int maxAttributeCount = 4;
    ATSUAttributeTag tags[maxAttributeCount + 1];
    ByteCount sizes[maxAttributeCount + 1];
    ATSUAttributeValuePtr values[maxAttributeCount + 1];
    int attributeCount = 0;

    Fixed size = FixRatio(emSquare, 1);
    tags[attributeCount] = kATSUSizeTag;
    sizes[attributeCount] = sizeof(size);
    values[attributeCount] = &size;
    ++attributeCount;

    Q_ASSERT(attributeCount < maxAttributeCount + 1);
    OSStatus err = ATSUSetAttributes(unscaledStyle, attributeCount, tags, sizes, values);
    Q_ASSERT(err == noErr);
    Q_UNUSED(err);

    // various CTFont metrics functions: CTFontGetBoundingRectsForGlyphs, CTFontGetAdvancesForGlyphs
    GlyphID atsuGlyph = glyph;
    ATSGlyphScreenMetrics atsuMetrics;
    ATSUGlyphGetScreenMetrics(unscaledStyle, 1, &atsuGlyph, 0,
                              /* iForcingAntiAlias =*/ false,
                              /* iAntiAliasSwitch =*/true,
                              &atsuMetrics);

    metrics->width = int(atsuMetrics.width);
    metrics->height = int(atsuMetrics.height);
    metrics->x = QFixed::fromReal(atsuMetrics.topLeft.x);
    metrics->y = -QFixed::fromReal(atsuMetrics.topLeft.y);
    metrics->xoff = QFixed::fromReal(atsuMetrics.deviceAdvance.x);
    metrics->yoff = QFixed::fromReal(atsuMetrics.deviceAdvance.y);

    QFixedPoint p;
    addGlyphsToPathHelper(unscaledStyle, &glyph, &p, 1, path);

    ATSUDisposeStyle(unscaledStyle);
}
#endif // !QT_MAC_USE_COCOA

QT_END_NAMESPACE
