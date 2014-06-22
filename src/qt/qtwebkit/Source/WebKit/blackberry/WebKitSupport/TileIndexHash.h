/*
 * Copyright (C) 2009, 2010, 2011, 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef TileIndexHash_h
#define TileIndexHash_h

#include "TileIndex.h"
#include <limits>
#include <wtf/HashMap.h>

using BlackBerry::WebKit::TileIndex;

namespace WTF {

template<> struct IntHash<TileIndex> {
    static unsigned hash(const TileIndex& key) { return pairIntHash(key.i(), key.j()); }
    static bool equal(const TileIndex& a, const TileIndex& b) { return a == b; }
    static const bool safeToCompareToEmptyOrDeleted = true;
};
template<> struct DefaultHash<TileIndex> {
    typedef IntHash<TileIndex> Hash;
};

template<> struct HashTraits<TileIndex> : GenericHashTraits<TileIndex> {
    static const bool emptyValueIsZero = false;
    static const bool needsDestruction = false;
    static TileIndex emptyValue() { return TileIndex(); }
    static void constructDeletedValue(TileIndex& slot)
    {
        new (&slot) TileIndex(std::numeric_limits<unsigned>::max() - 1, std::numeric_limits<unsigned>::max() - 1);
    }
    static bool isDeletedValue(const TileIndex& value)
    {
        return value.i() == (std::numeric_limits<unsigned>::max() - 1) && value.j() == (std::numeric_limits<unsigned>::max() - 1);
    }
};
} // namespace WTF

#endif // TileIndexHash_h
