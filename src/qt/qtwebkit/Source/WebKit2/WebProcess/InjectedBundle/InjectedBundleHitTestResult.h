/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#ifndef InjectedBundleHitTestResult_h
#define InjectedBundleHitTestResult_h

#include "APIObject.h"
#include "InjectedBundleHitTestResultMediaType.h"
#include <WebCore/HitTestResult.h>
#include <wtf/Forward.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>

namespace WebKit {

class InjectedBundleNodeHandle;
class WebFrame;

class InjectedBundleHitTestResult : public TypedAPIObject<APIObject::TypeBundleHitTestResult> {
public:
    static PassRefPtr<InjectedBundleHitTestResult> create(const WebCore::HitTestResult&);

    const WebCore::HitTestResult& coreHitTestResult() const { return m_hitTestResult; }

    PassRefPtr<InjectedBundleNodeHandle> nodeHandle() const; 
    WebFrame* frame() const;
    WebFrame* targetFrame() const;

    String absoluteImageURL() const;
    String absolutePDFURL() const;
    String absoluteLinkURL() const;
    String absoluteMediaURL() const;
    bool mediaIsInFullscreen() const;
    bool mediaHasAudio() const;
    BundleHitTestResultMediaType mediaType() const;

    String linkLabel() const;
    String linkTitle() const;
    
    WebCore::IntRect imageRect() const;
    
    bool isSelected() const;

private:
    explicit InjectedBundleHitTestResult(const WebCore::HitTestResult& hitTestResult)
        : m_hitTestResult(hitTestResult)
    {
    }

    WebCore::HitTestResult m_hitTestResult;
};

} // namespace WebKit

#endif // InjectedBundleHitTestResult_h
