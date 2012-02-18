/*
 * Copyright (C) 2004, 2005, 2006, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
 * Copyright (C) 2005 Eric Seidel <eric@webkit.org>
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
 */

#ifndef FEDisplacementMap_h
#define FEDisplacementMap_h

#if ENABLE(FILTERS)
#include "PlatformString.h"
#include "FilterEffect.h"
#include "Filter.h"

namespace WebCore {

enum ChannelSelectorType {
    CHANNEL_UNKNOWN = 0,
    CHANNEL_R = 1,
    CHANNEL_G = 2,
    CHANNEL_B = 3,
    CHANNEL_A = 4
};

class FEDisplacementMap : public FilterEffect {
public:
    static PassRefPtr<FEDisplacementMap> create(Filter*, ChannelSelectorType xChannelSelector, ChannelSelectorType yChannelSelector, float);

    ChannelSelectorType xChannelSelector() const;
    bool setXChannelSelector(const ChannelSelectorType);

    ChannelSelectorType yChannelSelector() const;
    bool setYChannelSelector(const ChannelSelectorType);

    float scale() const;
    bool setScale(float);

    virtual void apply();
    virtual void dump();

    virtual void determineAbsolutePaintRect() { setAbsolutePaintRect(maxEffectRect()); }

    virtual TextStream& externalRepresentation(TextStream&, int indention) const;

private:
    FEDisplacementMap(Filter*, ChannelSelectorType xChannelSelector, ChannelSelectorType yChannelSelector, float);

    ChannelSelectorType m_xChannelSelector;
    ChannelSelectorType m_yChannelSelector;
    float m_scale;
};

} // namespace WebCore

#endif // ENABLE(FILTERS)

#endif // FEDisplacementMap_h
