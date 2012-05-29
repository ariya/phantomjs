/*
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
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

#ifndef SourceAlpha_h
#define SourceAlpha_h

#if ENABLE(FILTERS)
#include "FilterEffect.h"

#include "PlatformString.h"
#include "Filter.h"

namespace WebCore {

class SourceAlpha : public FilterEffect {
public:        
    static PassRefPtr<SourceAlpha> create(Filter*);

    static const AtomicString& effectName();

    virtual void apply();
    virtual void dump();

    virtual void determineAbsolutePaintRect();

    virtual FilterEffectType filterEffectType() const { return FilterEffectTypeSourceInput; }

    virtual TextStream& externalRepresentation(TextStream&, int indention) const;

private:
    SourceAlpha(Filter* filter)
        : FilterEffect(filter)
    {
    }
};

} //namespace WebCore

#endif // ENABLE(FILTERS)

#endif // SourceAlpha_h
