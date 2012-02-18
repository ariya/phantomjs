/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef HTMLMeterElement_h
#define HTMLMeterElement_h

#if ENABLE(METER_TAG)
#include "HTMLFormControlElement.h"

namespace WebCore {

class MeterValueElement;

class HTMLMeterElement : public HTMLFormControlElement {
public:
    static PassRefPtr<HTMLMeterElement> create(const QualifiedName&, Document*, HTMLFormElement*);

    enum GaugeRegion {
        GaugeRegionOptimum,
        GaugeRegionSuboptimal,
        GaugeRegionEvenLessGood
    };

    double min() const;
    void setMin(double, ExceptionCode&);

    double max() const;
    void setMax(double, ExceptionCode&);

    double value() const;
    void setValue(double, ExceptionCode&);

    double low() const;
    void setLow(double, ExceptionCode&);

    double high() const;
    void setHigh(double, ExceptionCode&);

    double optimum() const;
    void setOptimum(double, ExceptionCode&);

    double valueRatio() const;
    GaugeRegion gaugeRegion() const;

private:
    HTMLMeterElement(const QualifiedName&, Document*, HTMLFormElement*);
    virtual ~HTMLMeterElement();

    virtual bool recalcWillValidate() const { return false; }
    virtual const AtomicString& formControlType() const;
    virtual RenderObject* createRenderer(RenderArena*, RenderStyle*);
    virtual void parseMappedAttribute(Attribute*);
    virtual void attach();

    void didElementStateChange();
    void createShadowSubtree();

    RefPtr<MeterValueElement> m_value;
};

} // namespace

#endif
#endif
