/*
 * Copyright (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
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

#include "config.h"
#include "AnimationList.h"

namespace WebCore {

#define FILL_UNSET_PROPERTY(test, propGet, propSet) \
for (i = 0; i < size() && animation(i)->test(); ++i) { } \
if (i < size() && i != 0) { \
    for (size_t j = 0; i < size(); ++i, ++j) \
        animation(i)->propSet(animation(j)->propGet()); \
}

AnimationList::AnimationList(const AnimationList& o)
{
    for (size_t i = 0; i < o.size(); ++i)
        m_animations.append(Animation::create(o.animation(i)));
}

void AnimationList::fillUnsetProperties()
{
    size_t i;
    FILL_UNSET_PROPERTY(isDelaySet, delay, setDelay);
    FILL_UNSET_PROPERTY(isDirectionSet, direction, setDirection);
    FILL_UNSET_PROPERTY(isDurationSet, duration, setDuration);
    FILL_UNSET_PROPERTY(isFillModeSet, fillMode, setFillMode);
    FILL_UNSET_PROPERTY(isIterationCountSet, iterationCount, setIterationCount);
    FILL_UNSET_PROPERTY(isPlayStateSet, playState, setPlayState);
    FILL_UNSET_PROPERTY(isNameSet, name, setName);
    FILL_UNSET_PROPERTY(isTimingFunctionSet, timingFunction, setTimingFunction);
    FILL_UNSET_PROPERTY(isPropertySet, property, setProperty);
}

bool AnimationList::operator==(const AnimationList& o) const
{
    if (size() != o.size())
        return false;
    for (size_t i = 0; i < size(); ++i)
        if (*animation(i) != *o.animation(i))
            return false;
    return true;
}

} // namespace WebCore
