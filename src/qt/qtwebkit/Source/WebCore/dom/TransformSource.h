/*
 * Copyright (C) 2009 Jakub Wieczorek <faw217@gmail.com>
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

#ifndef TransformSource_h
#define TransformSource_h

#if ENABLE(XSLT)

#include <wtf/FastAllocBase.h>
#include <wtf/Forward.h>
#include <wtf/Noncopyable.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

#if USE(QXMLQUERY)
    typedef String PlatformTransformSource;
#else
    typedef void* PlatformTransformSource;
#endif

    class TransformSource {
        WTF_MAKE_NONCOPYABLE(TransformSource); WTF_MAKE_FAST_ALLOCATED;
    public:
        explicit TransformSource(const PlatformTransformSource&);
        ~TransformSource();

        PlatformTransformSource platformSource() const { return m_source; }

    private:
        PlatformTransformSource m_source;
    };

} // namespace WebCore

#endif

#endif // TransformSource_h
