/*
 * Copyright (C) 2011, 2012 Research In Motion Limited. All rights reserved.
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

#ifndef LayerTileIndex_h
#define LayerTileIndex_h

#include <limits>
#include <wtf/HashMap.h>

namespace WebCore {

class TileIndex {
public:
    TileIndex()
        : m_i(std::numeric_limits<unsigned>::max())
        , m_j(std::numeric_limits<unsigned>::max())
    {
    }
    TileIndex(unsigned i, unsigned j)
        : m_i(i)
        , m_j(j)
    {
    }
    ~TileIndex() { }

    unsigned i() const { return m_i; }
    unsigned j() const { return m_j; }
    void setIndex(unsigned i, unsigned j)
    {
        m_i = i;
        m_j = j;
    }

private:
    unsigned m_i;
    unsigned m_j;
};

inline bool operator==(const TileIndex& a, const TileIndex& b)
{
    return a.i() == b.i() && a.j() == b.j();
}

inline bool operator!=(const TileIndex& a, const TileIndex& b)
{
    return a.i() != b.i() || a.j() != b.j();
}

}

namespace WTF {

template<> struct IntHash<WebCore::TileIndex> {
    static unsigned hash(const WebCore::TileIndex& key) { return intHash((static_cast<uint64_t>(key.i()) << 32 | key.j())); }
    static bool equal(const WebCore::TileIndex& a, const WebCore::TileIndex& b) { return a == b; }
    static const bool safeToCompareToEmptyOrDeleted = true;
};
template<> struct DefaultHash<WebCore::TileIndex> {
    typedef IntHash<WebCore::TileIndex> Hash;
};

template<> struct HashTraits<WebCore::TileIndex> : GenericHashTraits<WebCore::TileIndex> {
    static const bool emptyValueIsZero = false;
    static const bool needsDestruction = false;
    static WebCore::TileIndex emptyValue() { return WebCore::TileIndex(); }
    static void constructDeletedValue(WebCore::TileIndex& slot)
    {
        new (&slot) WebCore::TileIndex(std::numeric_limits<unsigned>::max() - 1, std::numeric_limits<unsigned>::max() - 1);
    }
    static bool isDeletedValue(const WebCore::TileIndex& value)
    {
        return value.i() == std::numeric_limits<unsigned>::max() - 1 && value.j() == std::numeric_limits<unsigned>::max() - 1;
    }
};

} // namespace WTF

#endif // LayerTileIndex_h
