/*
 * Copyright (C) 2008, 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
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

#ifndef Location_h
#define Location_h

#include "DOMStringList.h"
#include "DOMWindowProperty.h"
#include "ScriptWrappable.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class DOMWindow;
class Frame;
class KURL;

typedef int ExceptionCode;

class Location : public ScriptWrappable, public RefCounted<Location>, public DOMWindowProperty {
public:
    static PassRefPtr<Location> create(Frame* frame) { return adoptRef(new Location(frame)); }

    void setHref(const String&, DOMWindow* activeWindow, DOMWindow* firstWindow);
    String href() const;

    void assign(const String&, DOMWindow* activeWindow, DOMWindow* firstWindow);
    void replace(const String&, DOMWindow* activeWindow, DOMWindow* firstWindow);
    void reload(DOMWindow* activeWindow);

    void setProtocol(const String&, DOMWindow* activeWindow, DOMWindow* firstWindow, ExceptionCode&);
    String protocol() const;
    void setHost(const String&, DOMWindow* activeWindow, DOMWindow* firstWindow); 
    String host() const;
    void setHostname(const String&, DOMWindow* activeWindow, DOMWindow* firstWindow); 
    String hostname() const;
    void setPort(const String&, DOMWindow* activeWindow, DOMWindow* firstWindow); 
    String port() const;
    void setPathname(const String&, DOMWindow* activeWindow, DOMWindow* firstWindow); 
    String pathname() const;
    void setSearch(const String&, DOMWindow* activeWindow, DOMWindow* firstWindow); 
    String search() const;
    void setHash(const String&, DOMWindow* activeWindow, DOMWindow* firstWindow); 
    String hash() const;
    String origin() const;

    String toString() const { return href(); }

    PassRefPtr<DOMStringList> ancestorOrigins() const;

private:
    explicit Location(Frame*);

    void setLocation(const String&, DOMWindow* activeWindow, DOMWindow* firstWindow);

    const KURL& url() const;
};

} // namespace WebCore

#endif // Location_h
