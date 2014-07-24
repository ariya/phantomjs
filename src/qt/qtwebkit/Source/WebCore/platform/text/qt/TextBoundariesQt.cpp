/*
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "TextBoundaries.h"

#include "NotImplemented.h"

#include <QChar>
#include <QString>

#include <QDebug>
#include <stdio.h>

#include <qtextboundaryfinder.h>

namespace WebCore {

int findNextWordFromIndex(UChar const* buffer, int len, int position, bool forward)
{
    QString str(reinterpret_cast<QChar const*>(buffer), len);
    QTextBoundaryFinder iterator(QTextBoundaryFinder::Word, str);
    iterator.setPosition(position >= len ? len - 1 : position);
    if (forward) {
        int pos = iterator.toNextBoundary();
        while (pos > 0) {
            if (QChar(buffer[pos-1]).isLetterOrNumber())
                return pos;
            pos = iterator.toNextBoundary();
        }
        return len;
    } else {
        int pos = iterator.toPreviousBoundary();
        while (pos > 0) {
            if (QChar(buffer[pos]).isLetterOrNumber())
                return pos;
            pos = iterator.toPreviousBoundary();
        }
        return 0;
    }
}

void findWordBoundary(UChar const* buffer, int len, int position, int* start, int* end)
{
    QString str(reinterpret_cast<QChar const*>(buffer), len);
    QTextBoundaryFinder iterator(QTextBoundaryFinder::Word, str);
    iterator.setPosition(position);
    *start = position > 0 ? iterator.toPreviousBoundary() : 0;
    *end = position == len ? len : iterator.toNextBoundary();
}

}

