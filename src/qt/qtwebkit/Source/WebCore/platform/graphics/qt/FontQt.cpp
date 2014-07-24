/*
    Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)
    Copyright (C) 2008, 2010 Holger Hans Peter Freyther
    Copyright (C) 2009 Dirk Schulze <krit@webkit.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "Font.h"

#include "AffineTransform.h"
#include "FontDescription.h"
#include "FontGlyphs.h"
#include "FontSelector.h"
#include "GlyphBuffer.h"
#include "Gradient.h"
#include "GraphicsContext.h"
#include "NotImplemented.h"
#include "Pattern.h"
#include "ShadowBlur.h"
#include "TextRun.h"

#include <QBrush>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPointF>
#include <QTextLayout>
#include <qalgorithms.h>

#include <limits.h>

namespace WebCore {

static const QString fromRawDataWithoutRef(const String& string, int start = 0, int len = -1)
{
    if (len < 0)
        len = string.length() - start;
    Q_ASSERT(start + len <= string.length());

    // We don't detach. This assumes the WebCore string data will stay valid for the
    // lifetime of the QString we pass back, since we don't ref the WebCore string.
    return QString::fromRawData(reinterpret_cast<const QChar*>(string.characters() + start), len);
}

static QTextLine setupLayout(QTextLayout* layout, const TextRun& style)
{
    int flags = style.rtl() ? Qt::TextForceRightToLeft : Qt::TextForceLeftToRight;
    if (style.expansion())
        flags |= Qt::TextJustificationForced;
    layout->setFlags(flags);
    layout->beginLayout();
    QTextLine line = layout->createLine();
    line.setLineWidth(INT_MAX/256);
    if (style.expansion())
        line.setLineWidth(line.naturalTextWidth() + style.expansion());
    layout->endLayout();
    return line;
}

static QPen fillPenForContext(GraphicsContext* ctx)
{
    if (ctx->fillGradient()) {
        QBrush brush(*ctx->fillGradient()->platformGradient());
        brush.setTransform(ctx->fillGradient()->gradientSpaceTransform());
        return QPen(brush, 0);
    }

    if (ctx->fillPattern()) {
        return QPen(QBrush(ctx->fillPattern()->createPlatformPattern()), 0);
    }

    return QPen(QColor(ctx->fillColor()), 0);
}

static QPen strokePenForContext(GraphicsContext* ctx)
{
    if (ctx->strokeGradient()) {
        QBrush brush(*ctx->strokeGradient()->platformGradient());
        brush.setTransform(ctx->strokeGradient()->gradientSpaceTransform());
        return QPen(brush, ctx->strokeThickness());
    }

    if (ctx->strokePattern()) {
        QBrush brush(ctx->strokePattern()->createPlatformPattern());
        return QPen(brush, ctx->strokeThickness());
    }

    return QPen(QColor(ctx->strokeColor()), ctx->strokeThickness());
}

static QPainterPath pathForGlyphs(const QGlyphRun& glyphRun, const QPointF& offset)
{
    QPainterPath path;
    const QRawFont rawFont(glyphRun.rawFont());
    const QVector<quint32> glyphIndices = glyphRun.glyphIndexes();
    const QVector<QPointF> positions = glyphRun.positions();
    for (int i = 0; i < glyphIndices.size(); ++i) {
        QPainterPath glyphPath = rawFont.pathForGlyph(glyphIndices.at(i));
        glyphPath.translate(positions.at(i) + offset);
        path.addPath(glyphPath);
    }
    return path;
}

static void drawQtGlyphRun(GraphicsContext* context, const QGlyphRun& qtGlyphRun, const QPointF& point, int baseLineOffset)
{
    QPainter* painter = context->platformContext();

    QPainterPath textStrokePath;
    if (context->textDrawingMode() & TextModeStroke)
        textStrokePath = pathForGlyphs(qtGlyphRun, point);

    if (context->hasShadow()) {
        const GraphicsContextState& state = context->state();
        if (context->mustUseShadowBlur()) {
            ShadowBlur shadow(state);
            const int width = qtGlyphRun.boundingRect().width();
            const QRawFont& font = qtGlyphRun.rawFont();
            const int height = font.ascent() + font.descent();
            const QRectF boundingRect(point.x(), point.y() - font.ascent() + baseLineOffset, width, height);
            GraphicsContext* shadowContext = shadow.beginShadowLayer(context, boundingRect);
            if (shadowContext) {
                QPainter* shadowPainter = shadowContext->platformContext();
                shadowPainter->setPen(state.shadowColor);
                if (shadowContext->textDrawingMode() & TextModeFill)
                    shadowPainter->drawGlyphRun(point, qtGlyphRun);
                else if (shadowContext->textDrawingMode() & TextModeStroke)
                    shadowPainter->strokePath(textStrokePath, shadowPainter->pen());
                shadow.endShadowLayer(context);
            }
        } else {
            QPen previousPen = painter->pen();
            painter->setPen(state.shadowColor);
            const QPointF shadowOffset(state.shadowOffset.width(), state.shadowOffset.height());
            painter->translate(shadowOffset);
            if (context->textDrawingMode() & TextModeFill)
                painter->drawGlyphRun(point, qtGlyphRun);
            else if (context->textDrawingMode() & TextModeStroke)
                painter->strokePath(textStrokePath, painter->pen());
            painter->translate(-shadowOffset);
            painter->setPen(previousPen);
        }
    }

    if (context->textDrawingMode() & TextModeStroke)
        painter->strokePath(textStrokePath, strokePenForContext(context));

    if (context->textDrawingMode() & TextModeFill) {
        QPen previousPen = painter->pen();
        painter->setPen(fillPenForContext(context));
        painter->drawGlyphRun(point, qtGlyphRun);
        painter->setPen(previousPen);
    }
}

void Font::drawComplexText(GraphicsContext* ctx, const TextRun& run, const FloatPoint& point, int from, int to) const
{
    String sanitized = Font::normalizeSpaces(run.characters16(), run.length());
    const QString string = fromRawDataWithoutRef(sanitized);
    QTextLayout layout(string);
    layout.setRawFont(rawFont());
    initFormatForTextLayout(&layout, run);
    QTextLine line = setupLayout(&layout, run);
    const QPointF adjustedPoint(point.x(), point.y() - line.ascent());

    QList<QGlyphRun> runs = line.glyphRuns(from, to - from);
    Q_FOREACH(QGlyphRun glyphRun, runs)
        drawQtGlyphRun(ctx, glyphRun, adjustedPoint, line.ascent());
}

float Font::floatWidthForComplexText(const TextRun& run, HashSet<const SimpleFontData*>*, GlyphOverflow*) const
{
    if (!primaryFont()->platformData().size())
        return 0;

    if (!run.length())
        return 0;

    if (run.length() == 1 && treatAsSpace(run[0]))
        return primaryFont()->spaceWidth() + run.expansion();
    String sanitized = Font::normalizeSpaces(run.characters16(), run.length());
    QString string = fromRawDataWithoutRef(sanitized);

    QTextLayout layout(string);
    layout.setRawFont(rawFont());
    initFormatForTextLayout(&layout, run);
    QTextLine line = setupLayout(&layout, run);
    float x1 = line.cursorToX(0);
    float x2 = line.cursorToX(run.length());
    float width = qAbs(x2 - x1);

    return width + run.expansion();
}

int Font::offsetForPositionForComplexText(const TextRun& run, float position, bool) const
{
    String sanitized = Font::normalizeSpaces(run.characters16(), run.length());
    QString string = fromRawDataWithoutRef(sanitized);

    QTextLayout layout(string);
    layout.setRawFont(rawFont());
    initFormatForTextLayout(&layout, run);
    QTextLine line = setupLayout(&layout, run);
    return line.xToCursor(position);
}

FloatRect Font::selectionRectForComplexText(const TextRun& run, const FloatPoint& pt, int h, int from, int to) const
{
    String sanitized = Font::normalizeSpaces(run.characters16(), run.length());
    QString string = fromRawDataWithoutRef(sanitized);

    QTextLayout layout(string);
    layout.setRawFont(rawFont());
    initFormatForTextLayout(&layout, run);
    QTextLine line = setupLayout(&layout, run);

    float x1 = line.cursorToX(from);
    float x2 = line.cursorToX(to);
    if (x2 < x1)
        qSwap(x1, x2);

    return FloatRect(pt.x() + x1, pt.y(), x2 - x1, h);
}

void Font::initFormatForTextLayout(QTextLayout* layout, const TextRun& run) const
{
    QTextLayout::FormatRange range;
    // WebCore expects word-spacing to be ignored on leading spaces contrary to what Qt does.
    // To avoid word-spacing on any leading spaces, we exclude them from FormatRange which
    // word-spacing along with other options would be applied to. This is safe since the other
    // formatting options does not affect spaces.
    unsigned length = run.length();
    for (range.start = 0; range.start < length && treatAsSpace(run[range.start]); ++range.start) { }
    range.length = length - range.start;

    if (m_wordSpacing)
        range.format.setFontWordSpacing(m_wordSpacing);
    if (m_letterSpacing)
        range.format.setFontLetterSpacing(m_letterSpacing);
    if (typesettingFeatures() & Kerning)
        range.format.setFontKerning(true);
    if (isSmallCaps())
        range.format.setFontCapitalization(QFont::SmallCaps);

    if (range.format.propertyCount() && range.length)
        layout->setAdditionalFormats(QList<QTextLayout::FormatRange>() << range);
}

bool Font::canReturnFallbackFontsForComplexText()
{
    return false;
}

void Font::drawEmphasisMarksForComplexText(GraphicsContext* /* context */, const TextRun& /* run */, const AtomicString& /* mark */, const FloatPoint& /* point */, int /* from */, int /* to */) const
{
    notImplemented();
}

void Font::drawGlyphs(GraphicsContext* context, const SimpleFontData* fontData, const GlyphBuffer& glyphBuffer, int from, int numGlyphs, const FloatPoint& point) const
{
    if (!fontData->platformData().size())
        return;

    if (context->paintingDisabled())
        return;

    bool shouldFill = context->textDrawingMode() & TextModeFill;
    bool shouldStroke = context->textDrawingMode() & TextModeStroke;

    if (!shouldFill && !shouldStroke)
        return;

    QVector<quint32> glyphIndexes;
    QVector<QPointF> positions;

    glyphIndexes.reserve(numGlyphs);
    positions.reserve(numGlyphs);
    const QRawFont& font(fontData->getQtRawFont());

    float width = 0;

    for (int i = 0; i < numGlyphs; ++i) {
        Glyph glyph = glyphBuffer.glyphAt(from + i);
        float advance = glyphBuffer.advanceAt(from + i).width();
        if (!glyph)
            continue;
        glyphIndexes.append(glyph);
        positions.append(QPointF(width, 0));
        width += advance;
    }

    QGlyphRun qtGlyphs;
    qtGlyphs.setGlyphIndexes(glyphIndexes);
    qtGlyphs.setPositions(positions);
    qtGlyphs.setRawFont(font);

    drawQtGlyphRun(context, qtGlyphs, point, /* baselineOffset = */0);
}


bool Font::canExpandAroundIdeographsInComplexText()
{
    return false;
}

QFont Font::syntheticFont() const
{
    QRawFont rawFont(primaryFont()->getQtRawFont());
    QFont f(rawFont.familyName());
    if (rawFont.pixelSize())
        f.setPixelSize(rawFont.pixelSize());
    f.setWeight(rawFont.weight());
    f.setStyle(rawFont.style());
    if (m_letterSpacing)
        f.setLetterSpacing(QFont::AbsoluteSpacing, m_letterSpacing);
    if (m_wordSpacing)
        f.setWordSpacing(m_wordSpacing);
    return f;
}


QRawFont Font::rawFont() const
{
    return primaryFont()->getQtRawFont();
}

}
