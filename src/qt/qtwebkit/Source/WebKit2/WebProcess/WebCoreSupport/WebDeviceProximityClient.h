/*
 * Copyright 2012 Samsung Electronics. All rights reserved.
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

#ifndef WebDeviceProximityClient_h
#define WebDeviceProximityClient_h

#if ENABLE(PROXIMITY_EVENTS)

#include <WebCore/DeviceProximityClient.h>

namespace WebKit {

class WebPage;

class WebDeviceProximityClient : public WebCore::DeviceProximityClient {
public:
    explicit WebDeviceProximityClient(WebPage*);
    virtual ~WebDeviceProximityClient() { }

    void startUpdating();
    void stopUpdating();

    virtual bool hasLastData() OVERRIDE;
    virtual double value() OVERRIDE { return m_value; }
    virtual double min() OVERRIDE { return m_min; }
    virtual double max() OVERRIDE { return m_max; }

private:
    WebPage* m_page;
    
    bool m_isUpdating;
    double m_value;
    double m_min;
    double m_max;
};

} // namespace WebKit

#endif // PROXIMITY_EVENTS
#endif // WebDeviceProximityClient_h

