/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QTEXTENGINE_P_H
#define QTEXTENGINE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "QtCore/qglobal.h"
#include "QtCore/qstring.h"
#include "QtCore/qvarlengtharray.h"
#include "QtCore/qnamespace.h"
#include "QtGui/qtextlayout.h"
#include "private/qtextformat_p.h"
#include "private/qfont_p.h"
#include "QtCore/qvector.h"
#include "QtGui/qpaintengine.h"
#include "QtGui/qtextobject.h"
#include "QtGui/qtextoption.h"
#include "QtGui/qtextcursor.h"
#include "QtCore/qset.h"
#include "QtCore/qdebug.h"
#ifndef QT_BUILD_COMPAT_LIB
#include "private/qtextdocument_p.h"
#endif

#include "private/qfixed_p.h"

#include <private/qunicodetools_p.h>

#include <stdlib.h>

QT_BEGIN_NAMESPACE

class QFontPrivate;
class QFontEngine;

class QString;
class QPainter;

class QAbstractTextDocumentLayout;

typedef quint32 glyph_t;

// this uses the same coordinate system as Qt, but a different one to freetype.
// * y is usually negative, and is equal to the ascent.
// * negative yoff means the following stuff is drawn higher up.
// the characters bounding rect is given by QRect(x,y,width,height), its advance by
// xoo and yoff
struct Q_GUI_EXPORT glyph_metrics_t
{
    inline glyph_metrics_t()
        : x(100000),  y(100000) {}
    inline glyph_metrics_t(QFixed _x, QFixed _y, QFixed _width, QFixed _height, QFixed _xoff, QFixed _yoff)
        : x(_x),
          y(_y),
          width(_width),
          height(_height),
          xoff(_xoff),
          yoff(_yoff)
        {}
    QFixed x;
    QFixed y;
    QFixed width;
    QFixed height;
    QFixed xoff;
    QFixed yoff;

    glyph_metrics_t transformed(const QTransform &xform) const;
    inline bool isValid() const {return x != 100000 && y != 100000;}
};
Q_DECLARE_TYPEINFO(glyph_metrics_t, Q_PRIMITIVE_TYPE);

struct Q_AUTOTEST_EXPORT QScriptAnalysis
{
    enum Flags {
        None = 0,
        Lowercase = 1,
        Uppercase = 2,
        SmallCaps = 3,
        LineOrParagraphSeparator = 4,
        Space = 5,
        SpaceTabOrObject = Space,
        Tab = 6,
        TabOrObject = Tab,
        Object = 7
    };
    unsigned short script    : 7;
    unsigned short bidiLevel : 6;  // Unicode Bidi algorithm embedding level (0-61)
    unsigned short flags     : 3;
    inline bool operator == (const QScriptAnalysis &other) const {
        return script == other.script && bidiLevel == other.bidiLevel && flags == other.flags;
    }
};
Q_DECLARE_TYPEINFO(QScriptAnalysis, Q_PRIMITIVE_TYPE);

struct QGlyphJustification
{
    inline QGlyphJustification()
        : type(0), nKashidas(0), space_18d6(0)
    {}

    enum JustificationType {
        JustifyNone,
        JustifySpace,
        JustifyKashida
    };

    uint type :2;
    uint nKashidas : 6; // more do not make sense...
    uint space_18d6 : 24;
};
Q_DECLARE_TYPEINFO(QGlyphJustification, Q_PRIMITIVE_TYPE);

struct QGlyphAttributes {
    uchar clusterStart  : 1;
    uchar dontPrint     : 1;
    uchar justification : 4;
    uchar reserved      : 2;
};
Q_STATIC_ASSERT(sizeof(QGlyphAttributes) == 1);

struct QGlyphLayout
{
    enum {
        SpaceNeeded = sizeof(glyph_t) + sizeof(QFixed) + sizeof(QFixedPoint)
                    + sizeof(QGlyphAttributes) + sizeof(QGlyphJustification)
    };

    // init to 0 not needed, done when shaping
    QFixedPoint *offsets; // 8 bytes per element
    glyph_t *glyphs; // 4 bytes per element
    QFixed *advances; // 4 bytes per element
    QGlyphJustification *justifications; // 4 bytes per element
    QGlyphAttributes *attributes; // 1 byte per element

    int numGlyphs;

    inline QGlyphLayout() : numGlyphs(0) {}

    inline explicit QGlyphLayout(char *address, int totalGlyphs)
    {
        offsets = reinterpret_cast<QFixedPoint *>(address);
        int offset = totalGlyphs * sizeof(QFixedPoint);
        glyphs = reinterpret_cast<glyph_t *>(address + offset);
        offset += totalGlyphs * sizeof(glyph_t);
        advances = reinterpret_cast<QFixed *>(address + offset);
        offset += totalGlyphs * sizeof(QFixed);
        justifications = reinterpret_cast<QGlyphJustification *>(address + offset);
        offset += totalGlyphs * sizeof(QGlyphJustification);
        attributes = reinterpret_cast<QGlyphAttributes *>(address + offset);
        numGlyphs = totalGlyphs;
    }

    inline QGlyphLayout mid(int position, int n = -1) const {
        QGlyphLayout copy = *this;
        copy.glyphs += position;
        copy.advances += position;
        copy.offsets += position;
        copy.justifications += position;
        copy.attributes += position;
        if (n == -1)
            copy.numGlyphs -= position;
        else
            copy.numGlyphs = n;
        return copy;
    }

    inline QFixed effectiveAdvance(int item) const
    { return (advances[item] + QFixed::fromFixed(justifications[item].space_18d6)) * !attributes[item].dontPrint; }

    inline void clear(int first = 0, int last = -1) {
        if (last == -1)
            last = numGlyphs;
        if (first == 0 && last == numGlyphs
            && reinterpret_cast<char *>(offsets + numGlyphs) == reinterpret_cast<char *>(glyphs)) {
            memset(offsets, 0, (numGlyphs * SpaceNeeded));
        } else {
            const int num = last - first;
            memset(offsets + first, 0, num * sizeof(QFixedPoint));
            memset(glyphs + first, 0, num * sizeof(glyph_t));
            memset(advances + first, 0, num * sizeof(QFixed));
            memset(justifications + first, 0, num * sizeof(QGlyphJustification));
            memset(attributes + first, 0, num * sizeof(QGlyphAttributes));
        }
    }

    inline char *data() {
        return reinterpret_cast<char *>(offsets);
    }

    void grow(char *address, int totalGlyphs);
};

class QVarLengthGlyphLayoutArray : private QVarLengthArray<void *>, public QGlyphLayout
{
private:
    typedef QVarLengthArray<void *> Array;
public:
    QVarLengthGlyphLayoutArray(int totalGlyphs)
        : Array((totalGlyphs * SpaceNeeded) / sizeof(void *) + 1)
        , QGlyphLayout(reinterpret_cast<char *>(Array::data()), totalGlyphs)
    {
        memset(Array::data(), 0, Array::size() * sizeof(void *));
    }

    void resize(int totalGlyphs)
    {
        Array::resize((totalGlyphs * SpaceNeeded) / sizeof(void *) + 1);

        *((QGlyphLayout *)this) = QGlyphLayout(reinterpret_cast<char *>(Array::data()), totalGlyphs);
        memset(Array::data(), 0, Array::size() * sizeof(void *));
    }
};

template <int N> struct QGlyphLayoutArray : public QGlyphLayout
{
public:
    QGlyphLayoutArray()
        : QGlyphLayout(reinterpret_cast<char *>(buffer), N)
    {
        memset(buffer, 0, sizeof(buffer));
    }

private:
    void *buffer[(N * SpaceNeeded) / sizeof(void *) + 1];
};

struct QScriptItem;
/// Internal QTextItem
class QTextItemInt : public QTextItem
{
public:
    inline QTextItemInt()
        : justified(false), underlineStyle(QTextCharFormat::NoUnderline), num_chars(0), chars(0),
          logClusters(0), f(0), fontEngine(0)
    {}
    QTextItemInt(const QScriptItem &si, QFont *font, const QTextCharFormat &format = QTextCharFormat());
    QTextItemInt(const QGlyphLayout &g, QFont *font, const QChar *chars, int numChars, QFontEngine *fe,
                 const QTextCharFormat &format = QTextCharFormat());

    /// copy the structure items, adjusting the glyphs arrays to the right subarrays.
    /// the width of the returned QTextItemInt is not adjusted, for speed reasons
    QTextItemInt midItem(QFontEngine *fontEngine, int firstGlyphIndex, int numGlyphs) const;
    void initWithScriptItem(const QScriptItem &si);

    QFixed descent;
    QFixed ascent;
    QFixed width;

    RenderFlags flags;
    bool justified;
    QTextCharFormat::UnderlineStyle underlineStyle;
    const QTextCharFormat charFormat;
    int num_chars;
    const QChar *chars;
    const unsigned short *logClusters;
    const QFont *f;

    QGlyphLayout glyphs;
    QFontEngine *fontEngine;
};

struct Q_AUTOTEST_EXPORT QScriptItem
{
    inline QScriptItem()
        : position(0),
          num_glyphs(0), descent(-1), ascent(-1), leading(-1), width(-1),
          glyph_data_offset(0) {}
    inline QScriptItem(int p, const QScriptAnalysis &a)
        : position(p), analysis(a),
          num_glyphs(0), descent(-1), ascent(-1), leading(-1), width(-1),
          glyph_data_offset(0) {}

    int position;
    QScriptAnalysis analysis;
    unsigned short num_glyphs;
    QFixed descent;
    QFixed ascent;
    QFixed leading;
    QFixed width;
    int glyph_data_offset;
    QFixed height() const { return ascent + descent; }
};


Q_DECLARE_TYPEINFO(QScriptItem, Q_MOVABLE_TYPE);

typedef QVector<QScriptItem> QScriptItemArray;

struct Q_AUTOTEST_EXPORT QScriptLine
{
    // created and filled in QTextLine::layout_helper
    QScriptLine()
        : from(0), trailingSpaces(0), length(0),
        justified(0), gridfitted(0),
        hasTrailingSpaces(0), leadingIncluded(0) {}
    QFixed descent;
    QFixed ascent;
    QFixed leading;
    QFixed x;
    QFixed y;
    QFixed width;
    QFixed textWidth;
    QFixed textAdvance;
    int from;
    unsigned short trailingSpaces;
    signed int length : 28;
    mutable uint justified : 1;
    mutable uint gridfitted : 1;
    uint hasTrailingSpaces : 1;
    uint leadingIncluded : 1;
    QFixed height() const { return ascent + descent
                            + (leadingIncluded?  qMax(QFixed(),leading) : QFixed()); }
    QFixed base() const { return ascent
                          + (leadingIncluded ? qMax(QFixed(),leading) : QFixed()); }
    void setDefaultHeight(QTextEngine *eng);
    void operator+=(const QScriptLine &other);
};
Q_DECLARE_TYPEINFO(QScriptLine, Q_PRIMITIVE_TYPE);


inline void QScriptLine::operator+=(const QScriptLine &other)
{
    leading= qMax(leading + ascent, other.leading + other.ascent) - qMax(ascent, other.ascent);
    descent = qMax(descent, other.descent);
    ascent = qMax(ascent, other.ascent);
    textWidth += other.textWidth;
    length += other.length;
}

typedef QVector<QScriptLine> QScriptLineArray;

class QFontPrivate;
class QTextFormatCollection;

class Q_GUI_EXPORT QTextEngine {
public:
    enum LayoutState {
        LayoutEmpty,
        InLayout,
        LayoutFailed
    };
    struct Q_GUI_EXPORT LayoutData {
        LayoutData(const QString &str, void **stack_memory, int mem_size);
        LayoutData();
        ~LayoutData();
        mutable QScriptItemArray items;
        int allocated;
        int available_glyphs;
        void **memory;
        unsigned short *logClustersPtr;
        QGlyphLayout glyphLayout;
        mutable int used;
        uint hasBidi : 1;
        uint layoutState : 2;
        uint memory_on_stack : 1;
        uint haveCharAttributes : 1;
        QString string;
        bool reallocate(int totalGlyphs);
    };

    struct ItemDecoration {
        ItemDecoration(qreal x1, qreal x2, qreal y, const QPen &pen):
            x1(x1), x2(x2), y(y), pen(pen) {}

        qreal x1;
        qreal x2;
        qreal y;
        QPen pen;
    };

    typedef QList<ItemDecoration> ItemDecorationList;

    QTextEngine();
    QTextEngine(const QString &str, const QFont &f);
    ~QTextEngine();

    enum Mode {
        WidthOnly = 0x07
    };

    void invalidate();
    void clearLineData();

    void validate() const;
    void itemize() const;

    bool isRightToLeft() const;
    static void bidiReorder(int numRuns, const quint8 *levels, int *visualOrder);

    const QCharAttributes *attributes() const;

    void shape(int item) const;

    void justify(const QScriptLine &si);
    QFixed alignLine(const QScriptLine &line);

    QFixed width(int charFrom, int numChars) const;
    glyph_metrics_t boundingBox(int from,  int len) const;
    glyph_metrics_t tightBoundingBox(int from,  int len) const;

    int length(int item) const {
        const QScriptItem &si = layoutData->items[item];
        int from = si.position;
        item++;
        return (item < layoutData->items.size() ? layoutData->items[item].position : layoutData->string.length()) - from;
    }
    int length(const QScriptItem *si) const {
        int end;
        if (si + 1 < layoutData->items.constData()+ layoutData->items.size())
            end = (si+1)->position;
        else
            end = layoutData->string.length();
        return end - si->position;
    }

    QFontEngine *fontEngine(const QScriptItem &si, QFixed *ascent = 0, QFixed *descent = 0, QFixed *leading = 0) const;
    QFont font(const QScriptItem &si) const;
    inline QFont font() const { return fnt; }

    /**
     * Returns a pointer to an array of log clusters, offset at the script item.
     * Each item in the array is a unsigned short.  For each character in the original string there is an entry in the table
     * so there is a one to one correlation in indexes between the original text and the index in the logcluster.
     * The value of each item is the position in the glyphs array. Multiple similar pointers in the logclusters array imply
     * that one glyph is used for more than one character.
     * \sa glyphs()
     */
    inline unsigned short *logClusters(const QScriptItem *si) const
        { return layoutData->logClustersPtr+si->position; }
    /**
     * Returns an array of QGlyphLayout items, offset at the script item.
     * Each item in the array matches one glyph in the text, storing the advance, position etc.
     * The returned item's length equals to the number of available glyphs. This may be more
     * than what was actually shaped.
     * \sa logClusters()
     */
    inline QGlyphLayout availableGlyphs(const QScriptItem *si) const {
        return layoutData->glyphLayout.mid(si->glyph_data_offset);
    }
    /**
     * Returns an array of QGlyphLayout items, offset at the script item.
     * Each item in the array matches one glyph in the text, storing the advance, position etc.
     * The returned item's length equals to the number of shaped glyphs.
     * \sa logClusters()
     */
    inline QGlyphLayout shapedGlyphs(const QScriptItem *si) const {
        return layoutData->glyphLayout.mid(si->glyph_data_offset, si->num_glyphs);
    }

    inline bool ensureSpace(int nGlyphs) const {
        if (layoutData->glyphLayout.numGlyphs - layoutData->used < nGlyphs)
            return layoutData->reallocate((((layoutData->used + nGlyphs)*3/2 + 15) >> 4) << 4);
        return true;
    }

    void freeMemory();

    int findItem(int strPos) const;
    inline QTextFormatCollection *formats() const {
        if (block.docHandle())
            return block.docHandle()->formatCollection();
        return specialData ? specialData->formats.data() : 0;
    }
    QTextCharFormat format(const QScriptItem *si) const;
    inline QAbstractTextDocumentLayout *docLayout() const {
        Q_ASSERT(block.docHandle());
        return block.docHandle()->document()->documentLayout();
    }
    int formatIndex(const QScriptItem *si) const;

    /// returns the width of tab at index (in the tabs array) with the tab-start at position x
    QFixed calculateTabWidth(int index, QFixed x) const;

    mutable QScriptLineArray lines;

private:
    struct FontEngineCache {
        FontEngineCache();
        mutable QFontEngine *prevFontEngine;
        mutable QFontEngine *prevScaledFontEngine;
        mutable int prevScript;
        mutable int prevPosition;
        mutable int prevLength;
        inline void reset() {
            prevFontEngine = 0;
            prevScaledFontEngine = 0;
            prevScript = -1;
            prevPosition = -1;
            prevLength = -1;
        }
    };
    mutable FontEngineCache feCache;

public:
    QString text;
    mutable QFont fnt;
#ifndef QT_NO_RAWFONT
    QRawFont rawFont;
#endif
    QTextBlock block;

    QTextOption option;

    QFixed minWidth;
    QFixed maxWidth;
    QPointF position;
    uint ignoreBidi : 1;
    uint cacheGlyphs : 1;
    uint stackEngine : 1;
    uint forceJustification : 1;
    uint visualMovement : 1;
    uint delayDecorations: 1;
#ifndef QT_NO_RAWFONT
    uint useRawFont : 1;
#endif

    int *underlinePositions;

    mutable LayoutData *layoutData;

    ItemDecorationList underlineList;
    ItemDecorationList strikeOutList;
    ItemDecorationList overlineList;

    inline bool visualCursorMovement() const
    {
        return (visualMovement ||
                (block.docHandle() ? block.docHandle()->defaultCursorMoveStyle == Qt::VisualMoveStyle : false));
    }

    inline int preeditAreaPosition() const { return specialData ? specialData->preeditPosition : -1; }
    inline QString preeditAreaText() const { return specialData ? specialData->preeditText : QString(); }
    void setPreeditArea(int position, const QString &text);

    inline bool hasFormats() const { return block.docHandle() || (specialData && !specialData->addFormats.isEmpty()); }
    inline QList<QTextLayout::FormatRange> additionalFormats() const
    { return specialData ? specialData->addFormats : QList<QTextLayout::FormatRange>(); }
    void setAdditionalFormats(const QList<QTextLayout::FormatRange> &formatList);

private:
    static void init(QTextEngine *e);

    struct SpecialData {
        int preeditPosition;
        QString preeditText;
        QList<QTextLayout::FormatRange> addFormats;
        QVector<QTextCharFormat> resolvedFormats;
        // only used when no docHandle is available
        QScopedPointer<QTextFormatCollection> formats;
    };
    SpecialData *specialData;

    void indexAdditionalFormats();
    void resolveAdditionalFormats() const;

public:
    bool atWordSeparator(int position) const;
    bool atSpace(int position) const;

    QString elidedText(Qt::TextElideMode mode, const QFixed &width, int flags = 0, int from = 0, int count = -1) const;

    void shapeLine(const QScriptLine &line);
    QFixed leadingSpaceWidth(const QScriptLine &line);

    QFixed offsetInLigature(const QScriptItem *si, int pos, int max, int glyph_pos);
    int positionInLigature(const QScriptItem *si, int end, QFixed x, QFixed edge, int glyph_pos, bool cursorOnCharacter);
    int previousLogicalPosition(int oldPos) const;
    int nextLogicalPosition(int oldPos) const;
    int lineNumberForTextPosition(int pos);
    int positionAfterVisualMovement(int oldPos, QTextCursor::MoveOperation op);
    void insertionPointsForLine(int lineNum, QVector<int> &insertionPoints);
    void resetFontEngineCache();

    void enableDelayDecorations(bool enable = true) { delayDecorations = enable; }

    void addUnderline(QPainter *painter, const QLineF &line);
    void addStrikeOut(QPainter *painter, const QLineF &line);
    void addOverline(QPainter *painter, const QLineF &line);

    void drawDecorations(QPainter *painter);
    void clearDecorations();
    void adjustUnderlines();

private:
    void addItemDecoration(QPainter *painter, const QLineF &line, ItemDecorationList *decorationList);
    void adjustUnderlines(ItemDecorationList::iterator start,
                          ItemDecorationList::iterator end,
                          qreal underlinePos, qreal penWidth);
    void drawItemDecorationList(QPainter *painter, const ItemDecorationList &decorationList);
    void setBoundary(int strPos) const;
    void addRequiredBoundaries() const;
    void shapeText(int item) const;
#ifdef QT_ENABLE_HARFBUZZ_NG
    int shapeTextWithHarfbuzzNG(const QScriptItem &si, const ushort *string, int itemLength, QFontEngine *fontEngine, const QVector<uint> &itemBoundaries, bool kerningEnabled) const;
#endif
    int shapeTextWithHarfbuzz(const QScriptItem &si, const ushort *string, int itemLength, QFontEngine *fontEngine, const QVector<uint> &itemBoundaries, bool kerningEnabled) const;
    void splitItem(int item, int pos) const;

    int endOfLine(int lineNum);
    int beginningOfLine(int lineNum);
    int getClusterLength(unsigned short *logClusters, const QCharAttributes *attributes, int from, int to, int glyph_pos, int *start);
};

class Q_GUI_EXPORT QStackTextEngine : public QTextEngine {
public:
    enum { MemSize = 256*40/sizeof(void *) };
    QStackTextEngine(const QString &string, const QFont &f);
    LayoutData _layoutData;
    void *_memory[MemSize];
};

struct QTextLineItemIterator
{
    QTextLineItemIterator(QTextEngine *eng, int lineNum, const QPointF &pos = QPointF(),
                          const QTextLayout::FormatRange *_selection = 0);

    inline bool atEnd() const { return logicalItem >= nItems - 1; }
    inline bool atBeginning() const { return logicalItem <= 0; }
    QScriptItem &next();

    bool getSelectionBounds(QFixed *selectionX, QFixed *selectionWidth) const;
    inline bool isOutsideSelection() const {
        QFixed tmp1, tmp2;
        return !getSelectionBounds(&tmp1, &tmp2);
    }

    QTextEngine *eng;

    QFixed x;
    QFixed pos_x;
    const QScriptLine &line;
    QScriptItem *si;

    int lineNum;
    int lineEnd;
    int firstItem;
    int lastItem;
    int nItems;
    int logicalItem;
    int item;
    int itemLength;

    int glyphsStart;
    int glyphsEnd;
    int itemStart;
    int itemEnd;

    QFixed itemWidth;

    QVarLengthArray<int> visualOrder;
    QVarLengthArray<uchar> levels;

    const QTextLayout::FormatRange *selection;
};

QT_END_NAMESPACE

#endif // QTEXTENGINE_P_H
