/*
 * This file is part of the DOM implementation for WebCore.
 *
 * Copyright (C) 2006 Apple Computer, Inc.
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

#ifndef DocumentMarker_h
#define DocumentMarker_h

#include "PlatformString.h"
#include <wtf/Forward.h>


namespace WebCore {

// A range of a node within a document that is "marked", such as the range of a misspelled word.
// It optionally includes a description that could be displayed in the user interface.
// It also optionally includes a flag specifying whether the match is active, which is ignored
// for all types other than type TextMatch.
struct DocumentMarker {
    enum MarkerType {
        Spelling = 1 << 0,
        Grammar = 1 << 1,
        TextMatch = 1 << 2,
        // Text has been modified by spell correction, reversion of spell correction or other type of substitution. 
        // On some platforms, this prevents the text from being autocorrected again. On post Snow Leopard Mac OS X, 
        // if a Replacement marker contains non-empty description, a reversion UI will be shown.
        Replacement = 1 << 3,
        // Renderer needs to add underline indicating that the text has been modified by spell
        // correction. Text with Replacement marker doesn't necessarily has CorrectionIndicator
        // marker. For instance, after some text has been corrected, it will have both Replacement
        // and CorrectionIndicator. However, if user further modifies such text, we would remove
        // CorrectionIndicator marker, but retain Replacement marker.
        CorrectionIndicator = 1 << 4,
        // Correction suggestion has been offered, but got rejected by user.
        RejectedCorrection = 1 << 5,
        // Text has been modified by autocorrection. The description of this marker is the original text before autocorrection.
        Autocorrected = 1 << 6,
        // On some platforms, this prevents the text from being spellchecked again.
        SpellCheckingExemption = 1 << 7,
    };

    class MarkerTypes {
    public:
        // The constructor is intentionally implicit to allow conversion from the bit-wise sum of above types
        MarkerTypes(unsigned mask) : m_mask(mask) { }

        bool contains(MarkerType type) const { return m_mask & type; }
        bool intersects(const MarkerTypes& types) const { return (m_mask & types.m_mask); }
        bool operator==(const MarkerTypes& other) const { return m_mask == other.m_mask; }

        void add(const MarkerTypes& types) { m_mask |= types.m_mask; }
        void remove(const MarkerTypes& types) { m_mask &= ~types.m_mask; }

    private:
        unsigned m_mask;
    };

    class AllMarkers : public MarkerTypes {
    public:
        AllMarkers()
            : MarkerTypes(Spelling | Grammar | TextMatch | Replacement | CorrectionIndicator | RejectedCorrection | Autocorrected | SpellCheckingExemption)
        {
        }
    };
    
    MarkerType type;
    unsigned startOffset;
    unsigned endOffset;
    String description;
    bool activeMatch;

    bool operator==(const DocumentMarker& o) const
    {
        return type == o.type && startOffset == o.startOffset && endOffset == o.endOffset;
    }

    bool operator!=(const DocumentMarker& o) const
    {
        return !(*this == o);
    }
};

} // namespace WebCore

#endif // DocumentMarker_h
