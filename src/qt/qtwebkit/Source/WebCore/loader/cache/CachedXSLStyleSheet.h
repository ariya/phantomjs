/*
    Copyright (C) 1998 Lars Knoll (knoll@mpi-hd.mpg.de)
    Copyright (C) 2001 Dirk Mueller <mueller@kde.org>
    Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
    Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.

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

#ifndef CachedXSLStyleSheet_h
#define CachedXSLStyleSheet_h

#include "CachedResource.h"
#include <wtf/Vector.h>

namespace WebCore {

    class CachedResourceLoader;
    class TextResourceDecoder;

#if ENABLE(XSLT)
    class CachedXSLStyleSheet FINAL : public CachedResource {
    public:
        CachedXSLStyleSheet(const ResourceRequest&);

        const String& sheet() const { return m_sheet; }

    protected:
        virtual void checkNotify();

        String m_sheet;
        RefPtr<TextResourceDecoder> m_decoder;

    private:
        virtual bool mayTryReplaceEncodedData() const OVERRIDE { return true; }

        virtual void didAddClient(CachedResourceClient*) OVERRIDE;

        virtual void setEncoding(const String&) OVERRIDE;
        virtual String encoding() const OVERRIDE;
        virtual void finishLoading(ResourceBuffer*) OVERRIDE;
    };

#endif

}

#endif
