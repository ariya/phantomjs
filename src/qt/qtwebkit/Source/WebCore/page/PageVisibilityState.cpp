/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "PageVisibilityState.h"

#if ENABLE(PAGE_VISIBILITY_API)

namespace WebCore {

String pageVisibilityStateString(PageVisibilityState state)
{
    DEFINE_STATIC_LOCAL(const String, visible, (ASCIILiteral("visible")));
    DEFINE_STATIC_LOCAL(const String, hidden, (ASCIILiteral("hidden")));
    DEFINE_STATIC_LOCAL(const String, prerender, (ASCIILiteral("prerender")));
    DEFINE_STATIC_LOCAL(const String, unloaded, (ASCIILiteral("unloaded")));

    switch (state) {
    case PageVisibilityStateVisible:
        return visible;
    case PageVisibilityStateHidden:
        return hidden;
    case PageVisibilityStatePrerender:
        return prerender;
    case PageVisibilityStateUnloaded:
        return unloaded;
    }

    ASSERT_NOT_REACHED();
    return String();
}

} // namespace WebCore

#endif // if ENABLE(PAGE_VISIBILITY_API)
