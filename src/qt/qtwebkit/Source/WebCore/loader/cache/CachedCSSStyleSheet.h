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

#ifndef CachedCSSStyleSheet_h
#define CachedCSSStyleSheet_h

#include "CachedResource.h"
#include <wtf/Vector.h>

namespace WebCore {

    class CachedResourceClient;
    class StyleSheetContents;
    class TextResourceDecoder;
    struct CSSParserContext;

    class CachedCSSStyleSheet FINAL : public CachedResource {
    public:
        CachedCSSStyleSheet(const ResourceRequest&, const String& charset);
        virtual ~CachedCSSStyleSheet();

        const String sheetText(bool enforceMIMEType = true, bool* hasValidMIMEType = 0) const;

        PassRefPtr<StyleSheetContents> restoreParsedStyleSheet(const CSSParserContext&);
        void saveParsedStyleSheet(PassRefPtr<StyleSheetContents>);

    private:
        bool canUseSheet(bool enforceMIMEType, bool* hasValidMIMEType) const;
        virtual PurgePriority purgePriority() const OVERRIDE { return PurgeLast; }
        virtual bool mayTryReplaceEncodedData() const OVERRIDE { return true; }

        virtual void didAddClient(CachedResourceClient*) OVERRIDE;

        virtual void setEncoding(const String&) OVERRIDE;
        virtual String encoding() const OVERRIDE;
        virtual void finishLoading(ResourceBuffer*) OVERRIDE;
        virtual void destroyDecodedData() OVERRIDE;

    protected:
        virtual void checkNotify();

        RefPtr<TextResourceDecoder> m_decoder;
        String m_decodedSheetText;

        RefPtr<StyleSheetContents> m_parsedStyleSheetCache;
    };

}

#endif
