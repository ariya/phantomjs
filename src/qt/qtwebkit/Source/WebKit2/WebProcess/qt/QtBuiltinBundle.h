/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef QtBuiltinBundle_h
#define QtBuiltinBundle_h

#include "WKBundle.h"
#include "WKBundlePage.h"
#include <wtf/HashMap.h>
#include <wtf/OwnPtr.h>

namespace WebKit {

class QtBuiltinBundlePage;

class QtBuiltinBundle {
public:
    ~QtBuiltinBundle();

    static QtBuiltinBundle& shared();
    void initialize(WKBundleRef);

    WKBundleRef toRef() const { return m_bundle; }

    // Bundle Client.
    static void didCreatePage(WKBundleRef, WKBundlePageRef, const void*);
    static void willDestroyPage(WKBundleRef, WKBundlePageRef, const void*);
    static void didReceiveMessageToPage(WKBundleRef, WKBundlePageRef, WKStringRef messageName, WKTypeRef messageBody, const void*);

    void didCreatePage(WKBundlePageRef);
    void willDestroyPage(WKBundlePageRef);
    void didReceiveMessageToPage(WKBundlePageRef, WKStringRef messageName, WKTypeRef messageBody);

private:
    void handleMessageToNavigatorQtObject(WKBundlePageRef, WKTypeRef messageBody);
    void handleSetNavigatorQtObjectEnabled(WKBundlePageRef, WKTypeRef messageBody);

    HashMap<WKBundlePageRef, OwnPtr<QtBuiltinBundlePage> > m_pages;
    WKBundleRef m_bundle;
};

} // namespace WebKit

#endif // QtBuiltinBundle_h
