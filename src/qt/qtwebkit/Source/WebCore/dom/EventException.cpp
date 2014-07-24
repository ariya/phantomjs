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
#include "EventException.h"

namespace WebCore {

static struct EventExceptionNameDescription {
    const char* const name;
    const char* const description;
} eventExceptions[] = {
    { "UNSPECIFIED_EVENT_TYPE_ERR", "The Event's type was not specified by initializing the event before the method was called." },
    { "DISPATCH_REQUEST_ERR", "The Event object is already being dispatched." }
};

bool EventException::initializeDescription(ExceptionCode ec, ExceptionCodeDescription* description)
{
    if (ec < EventExceptionOffset || ec > EventExceptionMax)
        return false;

    description->typeName = "DOM Events";
    description->code = ec - EventExceptionOffset;
    description->type = EventExceptionType;

    size_t tableSize = WTF_ARRAY_LENGTH(eventExceptions);
    size_t tableIndex = ec - UNSPECIFIED_EVENT_TYPE_ERR;

    description->name = tableIndex < tableSize ? eventExceptions[tableIndex].name : 0;
    description->description = tableIndex < tableSize ? eventExceptions[tableIndex].description : 0;

    return true;
}

} // namespace WebCore
