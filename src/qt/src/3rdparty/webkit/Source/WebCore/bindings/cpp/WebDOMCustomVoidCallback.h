/*
 * Copyright (C) Kevin Ollivier <kevino@theolliviers.com>. All rights reserved.
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

#ifndef WebDOMCustomVoidCallback_h
#define WebDOMCustomVoidCallback_h

#include "VoidCallback.h"
#include <wtf/PassRefPtr.h>

// FIXME: This is just a stub to keep compilation working. We need to revisit 
// this when we add support for these callbacks to the WebDOM bindings.

class WebDOMCustomVoidCallback : public WebCore::VoidCallback {
public: 
    static PassRefPtr<WebDOMCustomVoidCallback> create()
    {
        return adoptRef(new WebDOMCustomVoidCallback());
    }
    
    virtual ~WebDOMCustomVoidCallback();
    
    virtual void handleEvent();
    
private:
    WebDOMCustomVoidCallback();
};

WebCore::VoidCallback* toWebCore(const WebDOMCustomVoidCallback&);

#endif // WebDOMCustomVoidCallback_h
