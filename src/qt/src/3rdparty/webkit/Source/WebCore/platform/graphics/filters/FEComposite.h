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

#ifndef FEComposite_h
#define FEComposite_h

#if ENABLE(FILTERS)
#include "FilterEffect.h"

#include "PlatformString.h"
#include "Filter.h"

namespace WebCore {

enum CompositeOperationType {
    FECOMPOSITE_OPERATOR_UNKNOWN    = 0, 
    FECOMPOSITE_OPERATOR_OVER       = 1,
    FECOMPOSITE_OPERATOR_IN         = 2,
    FECOMPOSITE_OPERATOR_OUT        = 3,
    FECOMPOSITE_OPERATOR_ATOP       = 4,
    FECOMPOSITE_OPERATOR_XOR        = 5,
    FECOMPOSITE_OPERATOR_ARITHMETIC = 6
};

class FEComposite : public FilterEffect {
public:
    static PassRefPtr<FEComposite> create(Filter*, const CompositeOperationType&, float, float, float, float);

    CompositeOperationType operation() const;
    bool setOperation(CompositeOperationType);

    float k1() const;
    bool setK1(float);

    float k2() const;
    bool setK2(float);

    float k3() const;
    bool setK3(float);

    float k4() const;
    bool setK4(float);

    virtual void apply();
    virtual void dump();
    
    virtual void determineAbsolutePaintRect();

    virtual TextStream& externalRepresentation(TextStream&, int indention) const;

private:
    FEComposite(Filter*, const CompositeOperationType&, float, float, float, float);

    CompositeOperationType m_type;
    float m_k1;
    float m_k2;
    float m_k3;
    float m_k4;
};

} // namespace WebCore

#endif // ENABLE(FILTERS)

#endif // FEComposite_h
