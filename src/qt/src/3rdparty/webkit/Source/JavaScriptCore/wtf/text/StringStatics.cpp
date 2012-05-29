/*
 * Copyright (C) 2010 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#ifdef SKIP_STATIC_CONSTRUCTORS_ON_GCC
#define ATOMICSTRING_HIDE_GLOBALS 1
#endif

#include "AtomicString.h"
#include "DynamicAnnotations.h"
#include "StaticConstructors.h"
#include "StringImpl.h"

namespace WTF {

StringImpl* StringImpl::empty()
{
    // FIXME: This works around a bug in our port of PCRE, that a regular expression
    // run on the empty string may still perform a read from the first element, and
    // as such we need this to be a valid pointer. No code should ever be reading
    // from a zero length string, so this should be able to be a non-null pointer
    // into the zero-page.
    // Replace this with 'reinterpret_cast<UChar*>(static_cast<intptr_t>(1))' once
    // PCRE goes away.
    static UChar emptyUCharData = 0;
    DEFINE_STATIC_LOCAL(StringImpl, emptyString, (&emptyUCharData, 0, ConstructStaticString));
    WTF_ANNOTATE_BENIGN_RACE(&emptyString, "Benign race on StringImpl::emptyString reference counter");
    return &emptyString;
}

#if COMPILER(WINSCW)
static AtomicString nullAtom1;
static AtomicString emptyAtom1;
static AtomicString textAtom1;
static AtomicString commentAtom1;
static AtomicString starAtom1;
static AtomicString xmlAtom1;
static AtomicString xmlnsAtom1;
#else
JS_EXPORTDATA DEFINE_GLOBAL(AtomicString, nullAtom)
JS_EXPORTDATA DEFINE_GLOBAL(AtomicString, emptyAtom, "")
JS_EXPORTDATA DEFINE_GLOBAL(AtomicString, textAtom, "#text")
JS_EXPORTDATA DEFINE_GLOBAL(AtomicString, commentAtom, "#comment")
JS_EXPORTDATA DEFINE_GLOBAL(AtomicString, starAtom, "*")
JS_EXPORTDATA DEFINE_GLOBAL(AtomicString, xmlAtom, "xml")
JS_EXPORTDATA DEFINE_GLOBAL(AtomicString, xmlnsAtom, "xmlns")
#endif

void AtomicString::init()
{
    static bool initialized;
    if (!initialized) {
        // Initialization is not thread safe, so this function must be called from the main thread first.
        ASSERT(isMainThread());

        // Use placement new to initialize the globals.
#if COMPILER(WINSCW)
        new ((void*)&nullAtom1) AtomicString;
        new ((void*)&emptyAtom1) AtomicString("");
        new ((void*)&textAtom1) AtomicString("#text");
        new ((void*)&commentAtom1) AtomicString("#comment");
        new ((void*)&starAtom1) AtomicString("*");
        new ((void*)&xmlAtom1) AtomicString("xml");
        new ((void*)&xmlnsAtom1) AtomicString("xmlns");
#else
        new ((void*)&nullAtom) AtomicString;
        new ((void*)&emptyAtom) AtomicString("");
        new ((void*)&textAtom) AtomicString("#text");
        new ((void*)&commentAtom) AtomicString("#comment");
        new ((void*)&starAtom) AtomicString("*");
        new ((void*)&xmlAtom) AtomicString("xml");
        new ((void*)&xmlnsAtom) AtomicString("xmlns");
#endif
        initialized = true;
    }
}

#if COMPILER(WINSCW)
const AtomicString& AtomicString::nullAtom2() { return nullAtom1;}
const AtomicString& AtomicString::emptyAtom2() { return emptyAtom1;}
const AtomicString& AtomicString::textAtom2() { return textAtom1;}
const AtomicString& AtomicString::commentAtom2() { return commentAtom1;}
const AtomicString& AtomicString::starAtom2() { return starAtom1;}
const AtomicString& AtomicString::xmlAtom2() { return xmlAtom1;}
const AtomicString& AtomicString::xmlnsAtom2() { return xmlnsAtom1;}
#endif

}
