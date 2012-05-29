/*
 * Copyright (C) 2010 Google, Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY GOOGLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GOOGLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef DocumentLoadTiming_h
#define DocumentLoadTiming_h

namespace WebCore {

struct DocumentLoadTiming {
    DocumentLoadTiming()
        : navigationStart(0.0)
        , unloadEventStart(0.0)
        , unloadEventEnd(0.0)
        , redirectStart(0.0)
        , redirectEnd(0.0)
        , redirectCount(0)
        , fetchStart(0.0)
        , responseEnd(0.0)
        , loadEventStart(0.0)
        , loadEventEnd(0.0)
        , hasCrossOriginRedirect(false)
        , hasSameOriginAsPreviousDocument(false)
    {
    }

    double navigationStart;
    double unloadEventStart;
    double unloadEventEnd;
    double redirectStart;
    double redirectEnd;
    short redirectCount;
    double fetchStart;
    double responseEnd;
    double loadEventStart;
    double loadEventEnd;
    bool hasCrossOriginRedirect;
    bool hasSameOriginAsPreviousDocument;
};

}

#endif
