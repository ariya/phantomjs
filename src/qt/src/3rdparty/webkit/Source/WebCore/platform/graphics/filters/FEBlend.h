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

#ifndef FEBlend_h
#define FEBlend_h

#if ENABLE(FILTERS)
#include "FilterEffect.h"

#include "Filter.h"

namespace WebCore {

enum BlendModeType {
    FEBLEND_MODE_UNKNOWN = 0,
    FEBLEND_MODE_NORMAL = 1,
    FEBLEND_MODE_MULTIPLY = 2,
    FEBLEND_MODE_SCREEN = 3,
    FEBLEND_MODE_DARKEN = 4,
    FEBLEND_MODE_LIGHTEN = 5
};

class FEBlend : public FilterEffect {
public:
    static PassRefPtr<FEBlend> create(Filter*, BlendModeType);

    BlendModeType blendMode() const;
    bool setBlendMode(BlendModeType);

    virtual void apply();
    virtual void dump();

    virtual TextStream& externalRepresentation(TextStream&, int indention) const;

private:
    FEBlend(Filter*, BlendModeType);

    BlendModeType m_mode;
};

} // namespace WebCore

#endif // ENABLE(FILTERS)

#endif // FEBlend_h
