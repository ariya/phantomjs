/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GOOGLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "DOMCoreException.h"

namespace WebCore {

static struct CoreException {
    const char* const name;
    const char* const description;
} coreExceptions[] = {
    { "IndexSizeError", "Index or size was negative, or greater than the allowed value." },
    { 0, 0 }, // DOMStringSizeError
    { "HierarchyRequestError", "A Node was inserted somewhere it doesn't belong." },
    { "WrongDocumentError", "A Node was used in a different document than the one that created it (that doesn't support it)." },
    { "InvalidCharacterError", "An invalid or illegal character was specified, such as in an XML name." },
    { 0, 0 }, // NoDataAllowedError
    { "NoModificationAllowedError", "An attempt was made to modify an object where modifications are not allowed." },
    { "NotFoundError", "An attempt was made to reference a Node in a context where it does not exist." },
    { "NotSupportedError", "The implementation did not support the requested type of object or operation." },
    { "InUseAttributeError", "An attempt was made to add an attribute that is already in use elsewhere." },
    { "InvalidStateError", "An attempt was made to use an object that is not, or is no longer, usable." },
    { "SyntaxError", "An invalid or illegal string was specified." },
    { "InvalidModificationError", "An attempt was made to modify the type of the underlying object." },
    { "NamespaceError", "An attempt was made to create or change an object in a way which is incorrect with regard to namespaces." },
    { "InvalidAccessError", "A parameter or an operation was not supported by the underlying object." },
    { 0, 0 }, // ValidationError
    { "TypeMismatchError", "The type of an object was incompatible with the expected type of the parameter associated to the object." },
    { "SecurityError", "An attempt was made to break through the security policy of the user agent." },
    // FIXME: Couldn't find a description in the HTML/DOM specifications for NETWORK_ERR, ABORT_ERR, URL_MISMATCH_ERR, and QUOTA_EXCEEDED_ERR
    { "NetworkError", "A network error occurred." },
    { "AbortError", "The user aborted a request." },
    { "URLMismatchError", "A worker global scope represented an absolute URL that is not equal to the resulting absolute URL." },
    { "QuotaExceededError", "An attempt was made to add something to storage that exceeded the quota." },
    { "TimeoutError", "A timeout occurred." },
    { "InvalidNodeTypeError", "The supplied node is invalid or has an invalid ancestor for this operation." },
    { "DataCloneError", "An object could not be cloned." }
};

bool DOMCoreException::initializeDescription(ExceptionCode ec, ExceptionCodeDescription* description)
{
    description->typeName = "DOM";
    description->code = ec;
    description->type = DOMCoreExceptionType;

    size_t tableSize = WTF_ARRAY_LENGTH(coreExceptions);
    size_t tableIndex = ec - INDEX_SIZE_ERR;

    description->name = tableIndex < tableSize ? coreExceptions[tableIndex].name : 0;
    description->description = tableIndex < tableSize ? coreExceptions[tableIndex].description : 0;

    return true;
}

} // namespace WebCore
