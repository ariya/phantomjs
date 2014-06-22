/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
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

#ifndef ewk_file_chooser_request_private_h
#define ewk_file_chooser_request_private_h

#include "APIObject.h"
#include "WKRetainPtr.h"
#include "ewk_object_private.h"
#include <WebKit2/WKBase.h>
#include <wtf/PassRefPtr.h>

class EwkFileChooserRequest : public EwkObject {
public:
    EWK_OBJECT_DECLARE(EwkFileChooserRequest)

    static PassRefPtr<EwkFileChooserRequest> create(WKOpenPanelParametersRef parameters, WKOpenPanelResultListenerRef listener)
    {
        return adoptRef(new EwkFileChooserRequest(parameters, listener));
    }

    ~EwkFileChooserRequest();

    bool allowMultipleFiles() const;
    WKRetainPtr<WKArrayRef> acceptedMIMETypes() const;
    inline bool wasHandled() const { return m_wasRequestHandled; }
    void cancel();
    void chooseFiles(WKArrayRef fileURLs);

private:
    EwkFileChooserRequest(WKOpenPanelParametersRef parameters, WKOpenPanelResultListenerRef listener);

    WKRetainPtr<WKOpenPanelParametersRef> m_parameters;
    WKRetainPtr<WKOpenPanelResultListenerRef> m_listener;
    bool m_wasRequestHandled;
};

#endif // ewk_file_chooser_request_private_h
