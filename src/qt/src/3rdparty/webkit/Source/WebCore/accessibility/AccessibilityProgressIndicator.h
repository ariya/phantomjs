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

#ifndef AccessibilityProgressIndicator_h
#define AccessibilityProgressIndicator_h

#if ENABLE(PROGRESS_TAG)

#include "AccessibilityRenderObject.h"

namespace WebCore {

class HTMLProgressElement;
class RenderProgress;

class AccessibilityProgressIndicator : public AccessibilityRenderObject {
public:
    static PassRefPtr<AccessibilityProgressIndicator> create(RenderProgress*);

private:
    virtual AccessibilityRole roleValue() const { return ProgressIndicatorRole; }

    virtual bool isProgressIndicator() const { return true; }

    virtual float valueForRange() const;
    virtual float maxValueForRange() const;
    virtual float minValueForRange() const;

    AccessibilityProgressIndicator(RenderProgress*);

    HTMLProgressElement* element() const;
    virtual bool accessibilityIsIgnored() const;
};


} // namespace WebCore

#endif // ENABLE(PROGRESS_TAG)

#endif // AccessibilityProgressIndicator_h
