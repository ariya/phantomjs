/*
 *  Copyright (C) 2007-2009 Torch Mobile, Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#ifndef WinCEGraphicsExtras_h
#define WinCEGraphicsExtras_h

// This file is used to contain small utilities used by WINCE graphics code.

namespace WebCore {
    // Always round to same direction. 0.5 is rounded to 1,
    // and -0.5 (0.5 - 1) is rounded to 0 (1 - 1), so that it
    // is consistent when transformation shifts.
    static inline int stableRound(double d)
    {
        if (d > 0)
            return static_cast<int>(d + 0.5);

        int i = static_cast<int>(d);
        return i - d > 0.5 ? i - 1 : i;
    }
}

#endif WinCEGraphicsExtras_h
