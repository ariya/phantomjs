/*
 *  Copyright (C) 2006, 2007 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef ExceptionCode_h
#define ExceptionCode_h

namespace WebCore {

    // The DOM standards use unsigned short for exception codes.
    // In our DOM implementation we use int instead, and use different
    // numerical ranges for different types of DOM exception, so that
    // an exception of any type can be expressed with a single integer.
    typedef int ExceptionCode;

    enum {
        INDEX_SIZE_ERR = 1,
        DOMSTRING_SIZE_ERR = 2,
        HIERARCHY_REQUEST_ERR = 3,
        WRONG_DOCUMENT_ERR = 4,
        INVALID_CHARACTER_ERR = 5,
        NO_DATA_ALLOWED_ERR = 6,
        NO_MODIFICATION_ALLOWED_ERR = 7,
        NOT_FOUND_ERR = 8,
        NOT_SUPPORTED_ERR = 9,
        INUSE_ATTRIBUTE_ERR = 10,

        // Introduced in DOM Level 2:
        INVALID_STATE_ERR = 11,
        SYNTAX_ERR = 12,
        INVALID_MODIFICATION_ERR = 13,
        NAMESPACE_ERR = 14,
        INVALID_ACCESS_ERR = 15,

        // Introduced in DOM Level 3:
        VALIDATION_ERR = 16,
        TYPE_MISMATCH_ERR = 17,

        // XMLHttpRequest extension:
        SECURITY_ERR = 18,

        // Others introduced in HTML5:
        NETWORK_ERR = 19,
        ABORT_ERR = 20,
        URL_MISMATCH_ERR = 21,
        QUOTA_EXCEEDED_ERR = 22

        // Introduced in File API:
        // http://www.w3.org/TR/file-upload/#dfn-fileerror
#if ENABLE(BLOB) || ENABLE(FILE_SYSTEM)
        , NOT_READABLE_ERR = 24
        , ENCODING_ERR = 26
#endif
    };

    enum ExceptionType {
        DOMExceptionType,
        RangeExceptionType,
        EventExceptionType,
        XMLHttpRequestExceptionType
#if ENABLE(XPATH)
        , XPathExceptionType
#endif
#if ENABLE(SVG)
        , SVGExceptionType
#endif
#if ENABLE(DATABASE)
        , SQLExceptionType
#endif
#if ENABLE(BLOB) || ENABLE(FILE_SYSTEM)
        , FileExceptionType
#endif
#if ENABLE(INDEXED_DATABASE)
        , IDBDatabaseExceptionType
#endif
    };


    struct ExceptionCodeDescription {
        const char* typeName; // has spaces and is suitable for use in exception description strings; maximum length is 10 characters
        const char* name; // exception name, also intended for use in exception description strings; 0 if name not known; maximum length is 27 characters
        const char* description; // exception description, intended for use in exception strings; more readable explanation of error
        int code; // numeric value of the exception within a particular type
        ExceptionType type;
    };
    void getExceptionCodeDescription(ExceptionCode, ExceptionCodeDescription&);

} // namespace WebCore

#endif // ExceptionCode_h
