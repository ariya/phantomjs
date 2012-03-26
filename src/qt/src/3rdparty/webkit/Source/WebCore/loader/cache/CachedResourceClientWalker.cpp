/*
    Copyright (C) 1998 Lars Knoll (knoll@mpi-hd.mpg.de)
    Copyright (C) 2001 Dirk Mueller (mueller@kde.org)
    Copyright (C) 2002 Waldo Bastian (bastian@kde.org)
    Copyright (C) 2004, 2005, 2006, 2007 Apple Inc. All rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

    This class provides all functionality needed for loading images, style sheets and html
    pages from the web. It has a memory cache for these objects.
*/

#include "config.h"
#include "CachedResourceClientWalker.h"

namespace WebCore {

CachedResourceClientWalker::CachedResourceClientWalker(const HashCountedSet<CachedResourceClient*>& set)
    : m_clientSet(set), m_clientVector(set.size()), m_index(0)
{
    typedef HashCountedSet<CachedResourceClient*>::const_iterator Iterator;
    Iterator end = set.end();
    size_t clientIndex = 0;
    for (Iterator current = set.begin(); current != end; ++current)
        m_clientVector[clientIndex++] = current->first;
}

CachedResourceClient* CachedResourceClientWalker::next()
{
    size_t size = m_clientVector.size();
    while (m_index < size) {
        CachedResourceClient* next = m_clientVector[m_index++];
        if (m_clientSet.contains(next))
            return next;
    }

    return 0;
}

}
