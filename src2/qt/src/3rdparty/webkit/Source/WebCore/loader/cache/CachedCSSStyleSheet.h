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
#include "TextEncoding.h"
#include <wtf/Vector.h>

namespace WebCore {

    class CachedResourceLoader;
    class TextResourceDecoder;

    class CachedCSSStyleSheet : public CachedResource {
    public:
        CachedCSSStyleSheet(const String& URL, const String& charset);
        virtual ~CachedCSSStyleSheet();

        const String sheetText(bool enforceMIMEType = true, bool* hasValidMIMEType = 0) const;

        virtual void didAddClient(CachedResourceClient*);
        
        virtual void allClientsRemoved();

        virtual void setEncoding(const String&);
        virtual String encoding() const;
        virtual void data(PassRefPtr<SharedBuffer> data, bool allDataReceived);
        virtual void error(CachedResource::Status);

        void checkNotify();
    
    private:
        bool canUseSheet(bool enforceMIMEType, bool* hasValidMIMEType) const;
        virtual PurgePriority purgePriority() const { return PurgeLast; }

    protected:
        RefPtr<TextResourceDecoder> m_decoder;
        String m_decodedSheetText;
    };

}

#endif
