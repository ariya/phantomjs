/*
 * Copyright (C) 2004, 2006, 2007 Apple Inc. All rights reserved.
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
#include "ExceptionHandlers.h"

#include "ExceptionCode.h"

NSString *DOMException = @"DOMException";
NSString *DOMRangeException = @"DOMRangeException";
NSString *DOMEventException = @"DOMEventException";
NSString *DOMXPathException = @"DOMXPathException";

namespace WebCore {

void raiseDOMException(ExceptionCode ec)
{
    ASSERT(ec);

    ExceptionCodeDescription description(ec);

    NSString *exceptionName;
    // FIXME: We can use the enum to do these comparisons faster.
    if (strcmp(description.typeName, "DOM Range") == 0)
        exceptionName = DOMRangeException;
    else if (strcmp(description.typeName, "DOM Events") == 0)
        exceptionName = DOMEventException;
    else if (strcmp(description.typeName, "DOM XPath") == 0)
        exceptionName = DOMXPathException;
    else
        exceptionName = DOMException;

    NSString *reason;
    if (description.name)
        reason = [[NSString alloc] initWithFormat:@"*** %s: %@ %d", description.name, exceptionName, description.code];
    else
        reason = [[NSString alloc] initWithFormat:@"*** %@ %d", exceptionName, description.code];

    NSDictionary *userInfo = [[NSDictionary alloc] initWithObjectsAndKeys:[NSNumber numberWithInt:description.code], exceptionName, nil];

    NSException *exception = [NSException exceptionWithName:exceptionName reason:reason userInfo:userInfo];

    [reason release];
    [userInfo release];

    [exception raise];
}

} // namespace WebCore
