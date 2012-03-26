/*
 * Copyright (C) 2005 Apple Computer, Inc.
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

#ifndef break_lines_h
#define break_lines_h

#include <wtf/unicode/Unicode.h>

namespace WebCore {

class LazyLineBreakIterator;

int nextBreakablePosition(LazyLineBreakIterator&, int pos, bool breakNBSP = false);

inline bool isBreakable(LazyLineBreakIterator& lazyBreakIterator, int pos, int& nextBreakable, bool breakNBSP = false)
{
    if (pos > nextBreakable)
        nextBreakable = nextBreakablePosition(lazyBreakIterator, pos, breakNBSP);
    return pos == nextBreakable;
}

} // namespace WebCore

#endif // break_lines_h
