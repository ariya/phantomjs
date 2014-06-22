/*
 * Copyright (C) 2011 Google, Inc. All rights reserved.
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
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef DOMSecurityPolicy_h
#define DOMSecurityPolicy_h

#include "ContextDestructionObserver.h"
#include <wtf/PassOwnPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class ContentSecurityPolicy;
class DOMStringList;
class Frame;

class DOMSecurityPolicy : public RefCounted<DOMSecurityPolicy>, public ContextDestructionObserver {
public:
    static PassRefPtr<DOMSecurityPolicy> create(ScriptExecutionContext* context)
    {
        return adoptRef(new DOMSecurityPolicy(context));
    }
    ~DOMSecurityPolicy();

    bool isActive() const;
    PassRefPtr<DOMStringList> reportURIs() const;

    bool allowsInlineScript() const;
    bool allowsInlineStyle() const;
    bool allowsEval() const;

    bool allowsConnectionTo(const String& url) const;
    bool allowsFontFrom(const String& url) const;
    bool allowsFormAction(const String& url) const;
    bool allowsFrameFrom(const String& url) const;
    bool allowsImageFrom(const String& url) const;
    bool allowsMediaFrom(const String& url) const;
    bool allowsObjectFrom(const String& url) const;
    bool allowsPluginType(const String& type) const;
    bool allowsScriptFrom(const String& url) const;
    bool allowsStyleFrom(const String& url) const;

private:
    explicit DOMSecurityPolicy(ScriptExecutionContext*);
};

}

#endif
