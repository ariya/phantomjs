/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#ifndef APIClientTraits_h
#define APIClientTraits_h

#include "WKBundle.h"
#include "WKBundlePage.h"
#include "WKContext.h"
#include "WKIconDatabase.h"
#include "WKPage.h"

namespace WebKit {

template <typename ClientInterface> struct APIClientTraits {
    static const size_t interfaceSizesByVersion[1];
};
template <typename ClientInterface> const size_t APIClientTraits<ClientInterface>::interfaceSizesByVersion[] = { sizeof(ClientInterface) };

template<> struct APIClientTraits<WKBundleClient> {
    static const size_t interfaceSizesByVersion[2];
};

template<> struct APIClientTraits<WKBundlePageLoaderClient> {
    static const size_t interfaceSizesByVersion[7];
};

template<> struct APIClientTraits<WKBundlePageResourceLoadClient> {
    static const size_t interfaceSizesByVersion[2];
};

template<> struct APIClientTraits<WKBundlePageFullScreenClient> {
    static const size_t interfaceSizesByVersion[2];
};

template<> struct APIClientTraits<WKBundlePageUIClient> {
    static const size_t interfaceSizesByVersion[3];
};

template<> struct APIClientTraits<WKPageContextMenuClient> {
    static const size_t interfaceSizesByVersion[4];
};

template<> struct APIClientTraits<WKPageLoaderClient> {
    static const size_t interfaceSizesByVersion[4];
};

template<> struct APIClientTraits<WKPageUIClient> {
    static const size_t interfaceSizesByVersion[3];
};

template<> struct APIClientTraits<WKBundlePageFormClient> {
    static const size_t interfaceSizesByVersion[3];
};

template<> struct APIClientTraits<WKBundlePageEditorClient> {
    static const size_t interfaceSizesByVersion[2];
};

template<> struct APIClientTraits<WKContextInjectedBundleClient> {
    static const size_t interfaceSizesByVersion[2];
};

template<> struct APIClientTraits<WKIconDatabaseClient> {
    static const size_t interfaceSizesByVersion[2];
};

} // namespace WebKit

#endif // APIClientTraits_h
