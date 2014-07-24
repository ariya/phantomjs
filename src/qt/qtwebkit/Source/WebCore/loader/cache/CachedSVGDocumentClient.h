/*
    Copyright (C) 2010 Rob Buis <rwlbuis@gmail.com>
    Copyright (C) 2011 Cosmin Truta <ctruta@gmail.com>
    Copyright (C) 2012 University of Szeged
    Copyright (C) 2012 Renata Hodovan <reni@webkit.org>

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
*/

#ifndef CachedSVGDocumentClient_h
#define CachedSVGDocumentClient_h

#if ENABLE(SVG)

#include "CachedResourceClient.h"

namespace WebCore {

class CachedSVGDocument;

class CachedSVGDocumentClient : public CachedResourceClient {
public:
    virtual ~CachedSVGDocumentClient() { }
    static CachedResourceClientType expectedType() { return SVGDocumentType; }
    virtual CachedResourceClientType resourceClientType() const { return expectedType(); }
};

} // namespace WebCore

#endif // ENABLE(SVG)

#endif // CachedSVGDocumentClient_h
