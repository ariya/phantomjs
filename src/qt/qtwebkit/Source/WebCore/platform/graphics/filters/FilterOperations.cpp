/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "FilterOperations.h"

#include "FEGaussianBlur.h"
#include "IntSize.h"
#include "LengthFunctions.h"

#if ENABLE(CSS_FILTERS)

namespace WebCore {

static inline IntSize outsetSizeForBlur(float stdDeviation)
{
    unsigned kernelSizeX = 0;
    unsigned kernelSizeY = 0;
    FEGaussianBlur::calculateUnscaledKernelSize(kernelSizeX, kernelSizeY, stdDeviation, stdDeviation);

    IntSize outset;
    // We take the half kernel size and multiply it with three, because we run box blur three times.
    outset.setWidth(3 * kernelSizeX * 0.5f);
    outset.setHeight(3 * kernelSizeY * 0.5f);

    return outset;
}

FilterOperations::FilterOperations()
{
}

FilterOperations& FilterOperations::operator=(const FilterOperations& other)
{
    m_operations = other.m_operations;
    return *this;
}

bool FilterOperations::operator==(const FilterOperations& o) const
{
    if (m_operations.size() != o.m_operations.size())
        return false;
        
    unsigned s = m_operations.size();
    for (unsigned i = 0; i < s; i++) {
        if (*m_operations[i] != *o.m_operations[i])
            return false;
    }
    
    return true;
}

bool FilterOperations::operationsMatch(const FilterOperations& other) const
{
    size_t numOperations = operations().size();
    // If the sizes of the function lists don't match, the lists don't match
    if (numOperations != other.operations().size())
        return false;
    
    // If the types of each function are not the same, the lists don't match
    for (size_t i = 0; i < numOperations; ++i) {
        if (!operations()[i]->isSameType(*other.operations()[i]))
            return false;
    }
    return true;
}

#if ENABLE(CSS_SHADERS)
bool FilterOperations::hasCustomFilter() const
{
    for (size_t i = 0; i < m_operations.size(); ++i) {
        FilterOperation::OperationType type = m_operations.at(i)->getOperationType();
        if (type == FilterOperation::CUSTOM || type == FilterOperation::VALIDATED_CUSTOM)
            return true;
    }
    return false;
}
#endif

bool FilterOperations::hasReferenceFilter() const
{
    for (size_t i = 0; i < m_operations.size(); ++i) {
        if (m_operations.at(i)->getOperationType() == FilterOperation::REFERENCE)
            return true;
    }
    return false;
}

bool FilterOperations::hasOutsets() const
{
    for (size_t i = 0; i < m_operations.size(); ++i) {
        FilterOperation::OperationType operationType = m_operations.at(i).get()->getOperationType();
        if (operationType == FilterOperation::BLUR || operationType == FilterOperation::DROP_SHADOW)
            return true;
    }
    return false;
}

FilterOutsets FilterOperations::outsets() const
{
    FilterOutsets totalOutsets;
    for (size_t i = 0; i < m_operations.size(); ++i) {
        FilterOperation* filterOperation = m_operations.at(i).get();
        switch (filterOperation->getOperationType()) {
        case FilterOperation::BLUR: {
            BlurFilterOperation* blurOperation = static_cast<BlurFilterOperation*>(filterOperation);
            float stdDeviation = floatValueForLength(blurOperation->stdDeviation(), 0);
            IntSize outsetSize = outsetSizeForBlur(stdDeviation);
            FilterOutsets outsets(outsetSize.height(), outsetSize.width(), outsetSize.height(), outsetSize.width());
            totalOutsets += outsets;
            break;
        }
        case FilterOperation::DROP_SHADOW: {
            DropShadowFilterOperation* dropShadowOperation = static_cast<DropShadowFilterOperation*>(filterOperation);
            IntSize outsetSize = outsetSizeForBlur(dropShadowOperation->stdDeviation());
            FilterOutsets outsets(
                std::max(0, outsetSize.height() - dropShadowOperation->y()),
                std::max(0, outsetSize.width() + dropShadowOperation->x()),
                std::max(0, outsetSize.height() + dropShadowOperation->y()),
                std::max(0, outsetSize.width() - dropShadowOperation->x())
            );
            totalOutsets += outsets;
            break;
        }
#if ENABLE(CSS_SHADERS)
        case FilterOperation::CUSTOM:
        case FilterOperation::VALIDATED_CUSTOM: {
            // FIXME: Need to include the filter margins here.
            // https://bugs.webkit.org/show_bug.cgi?id=71400
            break;
        }
#endif
        default:
            break;
        }
    }
    return totalOutsets;
}

bool FilterOperations::hasFilterThatAffectsOpacity() const
{
    for (size_t i = 0; i < m_operations.size(); ++i)
        if (m_operations[i]->affectsOpacity())
            return true;
    return false;
}

bool FilterOperations::hasFilterThatMovesPixels() const
{
    for (size_t i = 0; i < m_operations.size(); ++i)
        if (m_operations[i]->movesPixels())
            return true;
    return false;
}

} // namespace WebCore

#endif // ENABLE(CSS_FILTERS)
