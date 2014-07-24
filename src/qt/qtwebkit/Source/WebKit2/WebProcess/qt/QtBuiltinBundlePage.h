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

#ifndef QtBuiltinBundlePage_h
#define QtBuiltinBundlePage_h

#include "JSObjectRef.h"
#include "WKBundlePage.h"
#include "WKBundleScriptWorld.h"

namespace WebKit {

class QtBuiltinBundle;

class QtBuiltinBundlePage {
public:
    QtBuiltinBundlePage(QtBuiltinBundle*, WKBundlePageRef);
    ~QtBuiltinBundlePage();

    WKBundlePageRef page() const { return m_page; }

    // Loader Client.
    static void didClearWindowForFrame(WKBundlePageRef, WKBundleFrameRef, WKBundleScriptWorldRef, const void*);

    void didClearWindowForFrame(WKBundleFrameRef, WKBundleScriptWorldRef);

    void postMessageFromNavigatorQtObject(WKStringRef message);
    void didReceiveMessageToNavigatorQtObject(WKStringRef message);

    bool navigatorQtObjectEnabled() const { return m_navigatorQtObjectEnabled; }
    void setNavigatorQtObjectEnabled(bool);

private:
    void registerNavigatorQtObject(JSGlobalContextRef);

    static JSClassRef navigatorQtObjectClass();

    QtBuiltinBundle* m_bundle;
    WKBundlePageRef m_page;
    JSObjectRef m_navigatorQtObject;
    bool m_navigatorQtObjectEnabled;
};

} // namespace WebKit

#endif // QtBuiltinBundlePage_h
