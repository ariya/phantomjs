/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
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

#ifndef PlatformCAFilters_h
#define PlatformCAFilters_h

#if USE(ACCELERATED_COMPOSITING)
#if ENABLE(CSS_FILTERS)

#include "FilterOperations.h"
#include <wtf/RetainPtr.h>

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1080
#define USE_CA_FILTERS 1
#else
#define USE_CA_FILTERS 0
#endif

OBJC_CLASS NSValue;

namespace WebCore {
class PlatformCALayer;

class PlatformCAFilters {
public:
    static void setFiltersOnLayer(PlatformCALayer*, const FilterOperations&);
    static int numAnimatedFilterProperties(FilterOperation::OperationType);
    static const char* animatedFilterPropertyName(FilterOperation::OperationType, int internalFilterPropertyIndex);

#if PLATFORM(MAC)
    static RetainPtr<NSValue> filterValueForOperation(const FilterOperation*, int internalFilterPropertyIndex);
#endif

#ifdef USE_CA_FILTERS
    static RetainPtr<NSValue> colorMatrixValueForFilter(const FilterOperation&);
#endif
};

}

#endif // ENABLE(CSS_FILTERS)
#endif // USE(ACCELERATED_COMPOSITING)

#endif // PlatformCAFilters_h
