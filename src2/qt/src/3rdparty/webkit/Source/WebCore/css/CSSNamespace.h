/*
 * Copyright (C) 1999-2003 Lars Knoll (knoll@kde.org)
 *               1999 Waldo Bastian (bastian@kde.org)
 * Copyright (C) 2004, 2006, 2010 Apple Inc. All rights reserved.
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

#ifndef CSSNamespace_h
#define CSSNamespace_h

#include <wtf/text/AtomicString.h>

namespace WebCore {

    struct CSSNamespace {
        WTF_MAKE_NONCOPYABLE(CSSNamespace); WTF_MAKE_FAST_ALLOCATED;
    public:
        AtomicString prefix;
        AtomicString uri;
        OwnPtr<CSSNamespace> parent;

        CSSNamespace(const AtomicString& prefix, const AtomicString& uri, PassOwnPtr<CSSNamespace> parent)
            : prefix(prefix)
            , uri(uri)
            , parent(parent)
        {
        }

        CSSNamespace* namespaceForPrefix(const AtomicString& prefix)
        {
            for (CSSNamespace* candidate = this; candidate; candidate = candidate->parent.get()) {
                if (candidate->prefix == prefix)
                    return candidate;
            }
            return 0;
        }
    };

} // namespace WebCore

#endif // CSSNamespace_h
