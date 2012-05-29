/*
 *  Copyright (C) 2006, 2007 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "ExceptionCode.h"

#include "EventException.h"
#include "IDBDatabaseException.h"
#include "RangeException.h"
#include "XMLHttpRequestException.h"

#if ENABLE(SVG)
#include "SVGException.h"
#endif

#if ENABLE(XPATH)
#include "XPathException.h"
#endif

#if ENABLE(DATABASE)
#include "SQLException.h"
#endif

#if ENABLE(BLOB) || ENABLE(FILE_SYSTEM)
#include "FileException.h"
#endif

namespace WebCore {

static const char* const exceptionNames[] = {
    "INDEX_SIZE_ERR",
    "DOMSTRING_SIZE_ERR",
    "HIERARCHY_REQUEST_ERR",
    "WRONG_DOCUMENT_ERR",
    "INVALID_CHARACTER_ERR",
    "NO_DATA_ALLOWED_ERR",
    "NO_MODIFICATION_ALLOWED_ERR",
    "NOT_FOUND_ERR",
    "NOT_SUPPORTED_ERR",
    "INUSE_ATTRIBUTE_ERR",
    "INVALID_STATE_ERR",
    "SYNTAX_ERR",
    "INVALID_MODIFICATION_ERR",
    "NAMESPACE_ERR",
    "INVALID_ACCESS_ERR",
    "VALIDATION_ERR",
    "TYPE_MISMATCH_ERR",
    "SECURITY_ERR",
    "NETWORK_ERR",
    "ABORT_ERR",
    "URL_MISMATCH_ERR",
    "QUOTA_EXCEEDED_ERR"
};

static const char* const exceptionDescriptions[] = {
    "Index or size was negative, or greater than the allowed value.",
    "The specified range of text did not fit into a DOMString.",
    "A Node was inserted somewhere it doesn't belong.",
    "A Node was used in a different document than the one that created it (that doesn't support it).",
    "An invalid or illegal character was specified, such as in an XML name.",
    "Data was specified for a Node which does not support data.",
    "An attempt was made to modify an object where modifications are not allowed.",
    "An attempt was made to reference a Node in a context where it does not exist.",
    "The implementation did not support the requested type of object or operation.",
    "An attempt was made to add an attribute that is already in use elsewhere.",
    "An attempt was made to use an object that is not, or is no longer, usable.",
    "An invalid or illegal string was specified.",
    "An attempt was made to modify the type of the underlying object.",
    "An attempt was made to create or change an object in a way which is incorrect with regard to namespaces.",
    "A parameter or an operation was not supported by the underlying object.",
    "A call to a method such as insertBefore or removeChild would make the Node invalid with respect to \"partial validity\", this exception would be raised and the operation would not be done.",
    "The type of an object was incompatible with the expected type of the parameter associated to the object.",
    "An attempt was made to break through the security policy of the user agent.",
    // FIXME: Couldn't find a description in the HTML/DOM specifications for NETWORK_ERR, ABORT_ERR, URL_MISMATCH_ERR, and QUOTA_EXCEEDED_ERR
    "A network error occured.",
    "The user aborted a request.",
    "A worker global scope represented an absolute URL that is not equal to the resulting absolute URL.",
    "An attempt was made to add something to storage that exceeded the quota."
};

static const char* const rangeExceptionNames[] = {
    "BAD_BOUNDARYPOINTS_ERR",
    "INVALID_NODE_TYPE_ERR"
};

static const char* const rangeExceptionDescriptions[] = {
    "The boundary-points of a Range did not meet specific requirements.",
    "The container of an boundary-point of a Range was being set to either a node of an invalid type or a node with an ancestor of an invalid type."
};

static const char* const eventExceptionNames[] = {
    "UNSPECIFIED_EVENT_TYPE_ERR"
};

static const char* const eventExceptionDescriptions[] = {
    "The Event's type was not specified by initializing the event before the method was called."
};

static const char* const xmlHttpRequestExceptionNames[] = {
    "NETWORK_ERR",
    "ABORT_ERR"
};

static const char* const xmlHttpRequestExceptionDescriptions[] = {
    "A network error occured in synchronous requests.",
    "The user aborted a request in synchronous requests."
};

#if ENABLE(XPATH)
static const char* const xpathExceptionNames[] = {
    "INVALID_EXPRESSION_ERR",
    "TYPE_ERR"
};

static const char* const xpathExceptionDescriptions[] = {
    "The expression had a syntax error or otherwise is not a legal expression according to the rules of the specific XPathEvaluator.",
    "The expression could not be converted to return the specified type."
};
#endif

#if ENABLE(SVG)
static const char* const svgExceptionNames[] = {
    "SVG_WRONG_TYPE_ERR",
    "SVG_INVALID_VALUE_ERR",
    "SVG_MATRIX_NOT_INVERTABLE"
};

static const char* const svgExceptionDescriptions[] = {
    "An object of the wrong type was passed to an operation.",
    "An invalid value was passed to an operation or assigned to an attribute.",
    "An attempt was made to invert a matrix that is not invertible."
};
#endif

#if ENABLE(DATABASE)
static const char* const sqlExceptionNames[] = {
    "UNKNOWN_ERR",
    "DATABASE_ERR",
    "VERSION_ERR",
    "TOO_LARGE_ERR",
    "QUOTA_ERR",
    "SYNTAX_ERR",
    "CONSTRAINT_ERR",
    "TIMEOUT_ERR"
};

static const char* const sqlExceptionDescriptions[] = {
    "The operation failed for reasons unrelated to the database.",
    "The operation failed for some reason related to the database.",
    "The actual database version did not match the expected version.",
    "Data returned from the database is too large.",
    "Quota was exceeded.",
    "Invalid or unauthorized statement; or the number of arguments did not match the number of ? placeholders.",
    "A constraint was violated.",
    "A transaction lock could not be acquired in a reasonable time."
};
#endif

#if ENABLE(BLOB) || ENABLE(FILE_SYSTEM)
static const char* const fileExceptionNames[] = {
    "NOT_FOUND_ERR",
    "SECURITY_ERR",
    "ABORT_ERR",
    "NOT_READABLE_ERR",
    "ENCODING_ERR",
    "NO_MODIFICATION_ALLOWED_ERR",
    "INVALID_STATE_ERR",
    "SYNTAX_ERR",
    "INVALID_MODIFICATION_ERR",
    "QUOTA_EXCEEDED_ERR",
    "TYPE_MISMATCH_ERR",
    "PATH_EXISTS_ERR"
};

static const char* const fileExceptionDescriptions[] = {
    "A requested file or directory could not be found at the time an operation was processed.",
    "It was determined that certain files are unsafe for access within a Web application, or that too many calls are being made on file resources.",
    "An ongoing operation was aborted, typically with a call to abort().",
    "The requested file could not be read, typically due to permission problems that have occured after a reference to a file was acquired.",
    "A URI supplied to the API was malformed, or the resulting Data URL has exceeded the URL length limitations for Data URLs.",
    "An attempt was made to write to a file or directory which could not be modified due to the state of the underlying filesystem.",
    "An operation that depends on state cached in an interface object was made but the state had changed since it was read from disk.",
    "An invalid or unsupported argument was given, like an invalid line ending specifier.",
    "The modification request was illegal.",
    "The operation failed because it would cause the application to exceed its storage quota.",
    "The path supplied exists, but was not an entry of requested type.",
    "An attempt was made to create a file or directory where an element already exists."
};
#endif

#if ENABLE(INDEXED_DATABASE)
static const char* const idbDatabaseExceptionNames[] = {
    "UNKNOWN_ERR",
    "NON_TRANSIENT_ERR",
    "NOT_FOUND_ERR",
    "CONSTRAINT_ERR",
    "DATA_ERR",
    "NOT_ALLOWED_ERR",
    "SERIAL_ERR",
    "RECOVERABLE_ERR",
    "TRANSIENT_ERR",
    "TIMEOUT_ERR",
    "DEADLOCK_ERR",
    "READ_ONLY_ERR",
    "ABORT_ERR"
};

static const char* const idbDatabaseExceptionDescriptions[] = {
    "An unknown error occurred within Indexed Database.",
    "NON_TRANSIENT_ERR", // FIXME: Write a better message if it's ever possible this is thrown.
    "The name supplied does not match any existing item.",
    "The request cannot be completed due to a failed constraint.",
    "The data provided does not meet the requirements of the function.",
    "This function is not allowed to be called in such a context.",
    "The data supplied cannot be serialized according to the structured cloning algorithm.",
    "RECOVERABLE_ERR", // FIXME: This isn't even used.
    "TRANSIENT_ERR", // FIXME: This isn't even used.
    "TIMEOUT_ERR", // This can't be thrown.
    "DEADLOCK_ERR", // This can't be thrown.
    "Write operations cannot be preformed on a read-only transaction.",
    "The transaction was aborted, so the request cannot be fulfilled."
};
#endif

void getExceptionCodeDescription(ExceptionCode ec, ExceptionCodeDescription& description)
{
    ASSERT(ec);

    const char* typeName;
    int code = ec;
    const char* const* nameTable;
    const char* const* descriptionTable;
    int nameTableSize;
    int nameTableOffset;
    ExceptionType type;

    if (code >= RangeException::RangeExceptionOffset && code <= RangeException::RangeExceptionMax) {
        type = RangeExceptionType;
        typeName = "DOM Range";
        code -= RangeException::RangeExceptionOffset;
        nameTable = rangeExceptionNames;
        descriptionTable = rangeExceptionDescriptions;
        nameTableSize = WTF_ARRAY_LENGTH(rangeExceptionNames);
        nameTableOffset = RangeException::BAD_BOUNDARYPOINTS_ERR;
    } else if (code >= EventException::EventExceptionOffset && code <= EventException::EventExceptionMax) {
        type = EventExceptionType;
        typeName = "DOM Events";
        code -= EventException::EventExceptionOffset;
        nameTable = eventExceptionNames;
        descriptionTable = eventExceptionDescriptions;
        nameTableSize = WTF_ARRAY_LENGTH(eventExceptionNames);
        nameTableOffset = EventException::UNSPECIFIED_EVENT_TYPE_ERR;
    } else if (code >= XMLHttpRequestException::XMLHttpRequestExceptionOffset && code <= XMLHttpRequestException::XMLHttpRequestExceptionMax) {
        type = XMLHttpRequestExceptionType;
        typeName = "XMLHttpRequest";
        code -= XMLHttpRequestException::XMLHttpRequestExceptionOffset;
        nameTable = xmlHttpRequestExceptionNames;
        descriptionTable = xmlHttpRequestExceptionDescriptions;
        nameTableSize = WTF_ARRAY_LENGTH(xmlHttpRequestExceptionNames);
        // XMLHttpRequest exception codes start with 101 and we don't want 100 empty elements in the name array
        nameTableOffset = XMLHttpRequestException::NETWORK_ERR;
#if ENABLE(XPATH)
    } else if (code >= XPathException::XPathExceptionOffset && code <= XPathException::XPathExceptionMax) {
        type = XPathExceptionType;
        typeName = "DOM XPath";
        code -= XPathException::XPathExceptionOffset;
        nameTable = xpathExceptionNames;
        descriptionTable = xpathExceptionDescriptions;
        nameTableSize = WTF_ARRAY_LENGTH(xpathExceptionNames);
        // XPath exception codes start with 51 and we don't want 51 empty elements in the name array
        nameTableOffset = XPathException::INVALID_EXPRESSION_ERR;
#endif
#if ENABLE(SVG)
    } else if (code >= SVGException::SVGExceptionOffset && code <= SVGException::SVGExceptionMax) {
        type = SVGExceptionType;
        typeName = "DOM SVG";
        code -= SVGException::SVGExceptionOffset;
        nameTable = svgExceptionNames;
        descriptionTable = svgExceptionDescriptions;
        nameTableSize = WTF_ARRAY_LENGTH(svgExceptionNames);
        nameTableOffset = SVGException::SVG_WRONG_TYPE_ERR;
#endif
#if ENABLE(DATABASE)
    } else if (code >= SQLException::SQLExceptionOffset && code <= SQLException::SQLExceptionMax) {
        type = SQLExceptionType;
        typeName = "DOM SQL";
        code -= SQLException::SQLExceptionOffset;
        nameTable = sqlExceptionNames;
        descriptionTable = sqlExceptionDescriptions;
        nameTableSize = WTF_ARRAY_LENGTH(sqlExceptionNames);
        nameTableOffset = SQLException::UNKNOWN_ERR;
#endif
#if ENABLE(BLOB) || ENABLE(FILE_SYSTEM)
    } else if (code >= FileException::FileExceptionOffset && code <= FileException::FileExceptionMax) {
        type = FileExceptionType;
        typeName = "DOM File";
        code -= FileException::FileExceptionOffset;
        nameTable = fileExceptionNames;
        descriptionTable = fileExceptionDescriptions;
        nameTableSize = WTF_ARRAY_LENGTH(fileExceptionNames);
        nameTableOffset = FileException::NOT_FOUND_ERR;
#endif
#if ENABLE(INDEXED_DATABASE)
    } else if (code >= IDBDatabaseException::IDBDatabaseExceptionOffset && code <= IDBDatabaseException::IDBDatabaseExceptionMax) {
        type = IDBDatabaseExceptionType;
        typeName = "DOM IDBDatabase";
        code -= IDBDatabaseException::IDBDatabaseExceptionOffset;
        nameTable = idbDatabaseExceptionNames;
        descriptionTable = idbDatabaseExceptionDescriptions;
        nameTableSize = WTF_ARRAY_LENGTH(idbDatabaseExceptionNames);
        nameTableOffset = IDBDatabaseException::UNKNOWN_ERR;
#endif
    } else {
        type = DOMExceptionType;
        typeName = "DOM";
        nameTable = exceptionNames;
        descriptionTable = exceptionDescriptions;
        nameTableSize = WTF_ARRAY_LENGTH(exceptionNames);
        nameTableOffset = INDEX_SIZE_ERR;
    }

    description.typeName = typeName;
    description.name = (ec >= nameTableOffset && ec - nameTableOffset < nameTableSize) ? nameTable[ec - nameTableOffset] : 0;
    description.description = (ec >= nameTableOffset && ec - nameTableOffset < nameTableSize) ? descriptionTable[ec - nameTableOffset] : 0;
    description.code = code;
    description.type = type;

    // All exceptions used in the DOM code should have names.
    ASSERT(description.name);
    ASSERT(description.description);
}

} // namespace WebCore
