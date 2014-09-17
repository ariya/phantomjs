/*
 * Copyright (C) 2008 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
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
 *
 */

#ifndef FormDataBuilder_h
#define FormDataBuilder_h

#include "PlatformString.h"
#include <wtf/Forward.h>

namespace WebCore {

class Document;
class TextEncoding;

class FormDataBuilder {
    WTF_MAKE_NONCOPYABLE(FormDataBuilder);
public:
    static TextEncoding encodingFromAcceptCharset(const String& acceptCharset, Document* document);

    // Helper functions used by HTMLFormElement for multi-part form data
    static Vector<char> generateUniqueBoundaryString();
    static void beginMultiPartHeader(Vector<char>&, const CString& boundary, const CString& name);
    static void addBoundaryToMultiPartHeader(Vector<char>&, const CString& boundary, bool isLastBoundary = false);
    static void addFilenameToMultiPartHeader(Vector<char>&, const TextEncoding&, const String& filename);
    static void addContentTypeToMultiPartHeader(Vector<char>&, const CString& mimeType);
    static void finishMultiPartHeader(Vector<char>&);

    // Helper functions used by HTMLFormElement for non multi-part form data
    static void addKeyValuePairAsFormData(Vector<char>&, const CString& key, const CString& value);
    static void encodeStringAsFormData(Vector<char>&, const CString&);

private:
    FormDataBuilder() {}
};

}

#endif
