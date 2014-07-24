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

#include "config.h"
#include "WebFormClient.h"

#include "ImmutableDictionary.h"
#include "WKAPICast.h"
#include "WebString.h"
#include <wtf/text/WTFString.h>

namespace WebKit {

bool WebFormClient::willSubmitForm(WebPageProxy* page, WebFrameProxy* frame, WebFrameProxy* sourceFrame, const Vector<std::pair<String, String> >& textFieldValues, APIObject* userData, WebFormSubmissionListenerProxy* listener)
{
    if (!m_client.willSubmitForm)
        return false;

    ImmutableDictionary::MapType map;
    for (size_t i = 0; i < textFieldValues.size(); ++i)
        map.set(textFieldValues[i].first, WebString::create(textFieldValues[i].second));
    RefPtr<ImmutableDictionary> textFieldsMap = ImmutableDictionary::adopt(map);

    m_client.willSubmitForm(toAPI(page), toAPI(frame), toAPI(sourceFrame), toAPI(textFieldsMap.get()), toAPI(userData), toAPI(listener), m_client.clientInfo);
    return true;
}

} // namespace WebKit
