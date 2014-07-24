/*
 * Copyright (C) 2012 Adobe Systems Incorporated. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(CSS_SHADERS)
#include "CustomFilterParameterList.h"

#include "CustomFilterParameter.h"
#include <wtf/text/StringHash.h>

namespace WebCore {

CustomFilterParameterList::CustomFilterParameterList()
{
}

CustomFilterParameterList::CustomFilterParameterList(size_t size)
    : CustomFilterParameterListBase(size) 
{
}

bool CustomFilterParameterList::operator==(const CustomFilterParameterList& other) const
{
    if (size() != other.size())
        return false;
    for (size_t i = 0; i < size(); ++i) {
        if (at(i).get() != other.at(i).get() 
            && *at(i).get() != *other.at(i).get())
            return false;
    }
    return true;
}

bool CustomFilterParameterList::checkAlphabeticalOrder() const
{
    for (unsigned i = 1; i < size(); ++i) {
        // Break for equal or not-sorted parameters.
        if (!codePointCompareLessThan(at(i - 1)->name(), at(i)->name()))
            return false;
    }
    return true;
}

void CustomFilterParameterList::blend(const CustomFilterParameterList& fromList, 
    double progress, const LayoutSize& frameSize, CustomFilterParameterList& resultList) const
{
    // This method expects both lists to be sorted by parameter name and the result list is also sorted.
    ASSERT(checkAlphabeticalOrder());
    ASSERT(fromList.checkAlphabeticalOrder());
    size_t fromListIndex = 0, toListIndex = 0;
    while (fromListIndex < fromList.size() && toListIndex < size()) {
        CustomFilterParameter* paramFrom = fromList.at(fromListIndex).get();
        CustomFilterParameter* paramTo = at(toListIndex).get();
        if (paramFrom->name() == paramTo->name()) {
            resultList.append(paramTo->blend(paramFrom, progress, frameSize));
            ++fromListIndex;
            ++toListIndex;
            continue;
        }
        if (codePointCompareLessThan(paramFrom->name(), paramTo->name())) {
            resultList.append(paramFrom);
            ++fromListIndex;
            continue;
        }
        resultList.append(paramTo);
        ++toListIndex;
    }
    for (; fromListIndex < fromList.size(); ++fromListIndex)
        resultList.append(fromList.at(fromListIndex));
    for (; toListIndex < size(); ++toListIndex)
        resultList.append(at(toListIndex));
    ASSERT(resultList.checkAlphabeticalOrder());
}

} // namespace WebCore

#endif // ENABLE(CSS_SHADERS)
