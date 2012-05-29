/*
 * Copyright (C) 2006 Lars Knoll <lars@trolltech.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "TextBreakIterator.h"

#include <QtCore/qtextboundaryfinder.h>
#include <algorithm>
#include <qdebug.h>

// #define DEBUG_TEXT_ITERATORS
#ifdef DEBUG_TEXT_ITERATORS
#define DEBUG qDebug
#else
#define DEBUG if (1) {} else qDebug
#endif

using namespace std;

namespace WebCore {

#if USE(QT_ICU_TEXT_BREAKING)
const char* currentTextBreakLocaleID()
{
    return QLocale::system().name().toLatin1();
}
#else
    class TextBreakIterator : public QTextBoundaryFinder {
    public:
        TextBreakIterator(QTextBoundaryFinder::BoundaryType type, const QString& string)
            : QTextBoundaryFinder(type, string)
        { }
        TextBreakIterator()
            : QTextBoundaryFinder()
        { }
    };

    TextBreakIterator* setUpIterator(TextBreakIterator& iterator, QTextBoundaryFinder::BoundaryType type, const UChar* characters, int length)
    {
        if (!characters || !length)
            return 0;

        if (iterator.isValid() && type == iterator.type() && iterator.string() == QString::fromRawData(reinterpret_cast<const QChar*>(characters), length)) {
            iterator.toStart();
            return &iterator;
        }

        iterator = TextBreakIterator(type, QString(reinterpret_cast<const QChar*>(characters), length));
        return &iterator;
    }

    TextBreakIterator* wordBreakIterator(const UChar* string, int length)
    {
        static TextBreakIterator staticWordBreakIterator;
        return setUpIterator(staticWordBreakIterator, QTextBoundaryFinder::Word, string, length);
    }

    TextBreakIterator* characterBreakIterator(const UChar* string, int length)
    {
        static TextBreakIterator staticCharacterBreakIterator;
        return setUpIterator(staticCharacterBreakIterator, QTextBoundaryFinder::Grapheme, string, length);
    }

    TextBreakIterator* cursorMovementIterator(const UChar* string, int length)
    {
        return characterBreakIterator(string, length);
    }

    static TextBreakIterator* staticLineBreakIterator;

    TextBreakIterator* acquireLineBreakIterator(const UChar* string, int length)
    {
        TextBreakIterator* lineBreakIterator = 0;
        if (staticLineBreakIterator) {
            setUpIterator(*staticLineBreakIterator, QTextBoundaryFinder::Line, string, length);
            std::swap(staticLineBreakIterator, lineBreakIterator);
        }

        if (!lineBreakIterator && string && length)
            lineBreakIterator = new TextBreakIterator(QTextBoundaryFinder::Line, QString(reinterpret_cast<const QChar*>(string), length));

        return lineBreakIterator;
    }

    void releaseLineBreakIterator(TextBreakIterator* iterator)
    {
        ASSERT(iterator);

        if (!staticLineBreakIterator)
            staticLineBreakIterator = iterator;
        else
            delete iterator;
    }

    TextBreakIterator* sentenceBreakIterator(const UChar* string, int length)
    {
        static TextBreakIterator staticSentenceBreakIterator;
        return setUpIterator(staticSentenceBreakIterator, QTextBoundaryFinder::Sentence, string, length);

    }

    int textBreakFirst(TextBreakIterator* bi)
    {
        bi->toStart();
        DEBUG() << "textBreakFirst" << bi->position();
        return bi->position();
    }

    int textBreakNext(TextBreakIterator* bi)
    {
        int pos = bi->toNextBoundary();
        DEBUG() << "textBreakNext" << pos;
        return pos;
    }

    int textBreakPreceding(TextBreakIterator* bi, int pos)
    {
        bi->setPosition(pos);
        int newpos = bi->toPreviousBoundary();
        DEBUG() << "textBreakPreceding" << pos << newpos;
        return newpos;
    }

    int textBreakFollowing(TextBreakIterator* bi, int pos)
    {
        bi->setPosition(pos);
        int newpos = bi->toNextBoundary();
        DEBUG() << "textBreakFollowing" << pos << newpos;
        return newpos;
    }

    int textBreakCurrent(TextBreakIterator* bi)
    {
        return bi->position();
    }

    bool isTextBreak(TextBreakIterator*, int)
    {
        return true;
    }
#endif

}
